#include "SolarmanChannel.h"
#include "NetworkModule.h"
#include "knxprod.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

SolarmanChannel::SolarmanChannel(uint8_t channelIndex)
    : _channelIndex(channelIndex)
{
}

const std::string SolarmanChannel::name()
{
    return "SolarmanPV";
}

void SolarmanChannel::setup()
{
    const uint8_t _channelIndex = this->_channelIndex; // fuer die Param-/KO-Makros
    _pollIntervalS = ParamSPV_CHPollInterval;

    const SolarmanV5Client::Transport transport =
        (ParamSPV_CHTransport == 1) ? SolarmanV5Client::ModbusTcp : SolarmanV5Client::V5;

    // Seriennummer als Text: sie ist zehnstellig und kann groesser als 2^31-1 sein, was ETS
    // als Zahlenfeld nicht zuverlaessig darstellt.
    uint32_t serial = 0;
    if (transport == SolarmanV5Client::V5 && ParamSPV_CHSerialMode == 1)
    {
        const char* text = (const char*)ParamSPV_CHLoggerSerialText;
        if (text != nullptr && text[0] != 0)
            serial = (uint32_t)strtoul(text, nullptr, 10);
    }

    const char* ip = (const char*)ParamSPV_CHLoggerIp;
    _client.configure(ip, (uint16_t)ParamSPV_CHLoggerPort, serial, (uint8_t)ParamSPV_CHSlaveId, transport);

    // Modbus TCP kennt keine Seriennummer, dort ist nichts zu ermitteln.
    _serialKnown = (transport == SolarmanV5Client::ModbusTcp) || (serial != 0);

    readTable();
    buildBlocks();

    logDebugP("Kanal %d: %s, %s:%d, Intervall %ds, %d Messwerte in %d Bloecken",
              _channelIndex + 1,
              (transport == SolarmanV5Client::ModbusTcp) ? "Modbus TCP" : "Solarman V5",
              (ip != nullptr && ip[0]) ? ip : "(keine IP)", (int)ParamSPV_CHLoggerPort,
              (int)_pollIntervalS, (int)_activeCount, (int)_blockCount);
}

// Liest die Messwerttabelle aus dem Parameterspeicher. Die Zeilen liegen als Feld fester
// Schrittweite hintereinander, deshalb reicht Adressrechnung statt 64 einzelner Makros.
void SolarmanChannel::readTable()
{
    const uint8_t _channelIndex = this->_channelIndex;
    _activeCount = 0;

    for (uint8_t slot = 0; slot < Spv::SLOT_COUNT; slot++)
    {
        const uint16_t base = Spv::TABLE_OFFSET + (uint16_t)slot * Spv::ROW_BYTES;
        const uint8_t flags = knx.paramByte(SPV_ParamCalcIndex(base + 2));
        if ((flags & 0x02) == 0) // Bit 1 = "aktiv"
            continue;

        Row& row = _rows[_activeCount++];
        row.slot = slot;
        row.reg = knx.paramWord(SPV_ParamCalcIndex(base));
        row.type = (uint8_t)((flags >> 5) & 0x07);
        row.scale = Spv::SCALE[(flags >> 2) & 0x07];
        row.offset = (int8_t)knx.paramByte(SPV_ParamCalcIndex(base + 3));
    }
}

// Fasst die benoetigten Register zu moeglichst wenigen Leseblocken zusammen. Ohne das
// entstuende je Messwert eine eigene Anfrage - bei 17 Werten also 17 statt zwei.
void SolarmanChannel::buildBlocks()
{
    _blockCount = 0;

    // Alle benoetigten Adressen einsammeln (32-Bit-Werte belegen zwei Register).
    uint16_t needed[Spv::SLOT_COUNT * 2];
    uint16_t count = 0;
    for (uint8_t i = 0; i < _activeCount; i++)
    {
        if (_rows[i].type == Spv::TypeProduct)
            continue; // wird gerechnet, nicht gelesen
        needed[count++] = _rows[i].reg;
        if (_rows[i].type == Spv::TypeU32 || _rows[i].type == Spv::TypeS32)
            needed[count++] = (uint16_t)(_rows[i].reg + 1);
    }
    if (count == 0)
        return;

    // Aufsteigend sortieren (Einfuegesortieren genuegt: hoechstens 128 Eintraege, einmalig).
    for (uint16_t i = 1; i < count; i++)
    {
        const uint16_t key = needed[i];
        int16_t j = (int16_t)i - 1;
        while (j >= 0 && needed[j] > key)
        {
            needed[j + 1] = needed[j];
            j--;
        }
        needed[j + 1] = key;
    }

    for (uint16_t i = 0; i < count; i++)
    {
        if (i > 0 && needed[i] == needed[i - 1])
            continue; // Doppelte ueberspringen

        const bool fits = (_blockCount > 0) &&
                          (needed[i] - _blockStart[_blockCount - 1]) < MAX_BLOCK_WORDS;
        if (fits)
        {
            _blockWords[_blockCount - 1] = (uint8_t)(needed[i] - _blockStart[_blockCount - 1] + 1);
            continue;
        }

        if (_blockCount >= MAX_BLOCKS)
        {
            logInfoP("Kanal %d: mehr als %d Leseblocke noetig, Register ab 0x%04X werden ignoriert",
                     _channelIndex + 1, (int)MAX_BLOCKS, (unsigned)needed[i]);
            return;
        }
        _blockStart[_blockCount] = needed[i];
        _blockWords[_blockCount] = 1;
        _blockCount++;
    }
}

void SolarmanChannel::loop()
{
    if (!_client.configured())
        return;

    // Die Zustandsmaschine muss in jedem Durchlauf getaktet werden; sie kehrt sofort zurueck.
    _client.poll();

    if (_client.finished())
    {
        handleFinished();
        _client.clear();
    }
    else if (!_client.busy())
    {
        startNextRequest();
    }

    updateStatusKo();
}

void SolarmanChannel::startNextRequest()
{
    if (!openknxNetwork.established())
        return;

    // Ohne Seriennummer weist ein V5-Logger jede Anfrage ab. Sie steht aber im Header jeder
    // Antwort - auch im Fehlerrahmen. Das muss VOR allem anderen passieren.
    if (!_serialKnown)
    {
        _client.beginDiscoverSerial();
        return;
    }

    if (_dumpPending)
    {
        _dumpPending = false;
        _dumpInFlight = true;
        if (!_client.beginRead(3, _dumpStart, _dumpCount))
        {
            _dumpInFlight = false;
            logInfoP("spvread: abgelehnt (%s)", SolarmanV5Client::resultText(_client.result()));
        }
        return;
    }

    // Laufenden Lesezyklus fortsetzen.
    if (_blockPhase != BLOCK_IDLE && _blockPhase < _blockCount)
    {
        _client.beginRead(3, _blockStart[_blockPhase], _blockWords[_blockPhase]);
        return;
    }

    if (_pollIntervalS == 0 || _blockCount == 0)
        return;
    const uint32_t now = millis();
    if (_lastPollMs != 0 && (now - _lastPollMs) < (uint32_t)_pollIntervalS * 1000UL)
        return;
    _lastPollMs = now;

    _blockPhase = 0;
    _client.beginRead(3, _blockStart[0], _blockWords[0]);
}

void SolarmanChannel::handleFinished()
{
    const uint8_t _channelIndex = this->_channelIndex;
    const SolarmanV5Client::Result result = _client.result();
    const bool ok = (result == SolarmanV5Client::Ok);
    _reachable = ok;

    // Registerdump zuerst: das Flag gehoert immer zurueckgesetzt, sonst wuerde eine spaetere
    // zyklische Antwort faelschlich als Dump ausgegeben.
    if (_dumpInFlight)
    {
        _dumpInFlight = false;
        if (ok)
        {
            logInfoP("Kanal %d: %d Register ab 0x%04X", _channelIndex + 1,
                     (int)_client.registerCount(), (int)_dumpStart);
            logIndentUp();
            for (uint16_t i = 0; i < _client.registerCount(); i++)
            {
                const uint16_t raw = _client.registers()[i];
                logInfoP("0x%04X = %5u (0x%04X, signed %6d)", (unsigned)(_dumpStart + i),
                         (unsigned)raw, (unsigned)raw, (int)(int16_t)raw);
            }
            logIndentDown();
        }
        else
        {
            logInfoP("Kanal %d: Dump fehlgeschlagen (%s)", _channelIndex + 1,
                     SolarmanV5Client::resultText(result));
        }
        return;
    }

    if (!_serialKnown)
    {
        if (ok && _client.loggerSerial() != 0)
        {
            _serialKnown = true;
            logInfoP("Kanal %d: Logger-Seriennummer ermittelt: %u", _channelIndex + 1,
                     (unsigned)_client.loggerSerial());
        }
        return;
    }

    if (_blockPhase != BLOCK_IDLE && _blockPhase < _blockCount)
    {
        const uint8_t phase = _blockPhase;
        if (ok && _client.registerCount() <= MAX_BLOCK_WORDS)
        {
            for (uint16_t i = 0; i < _client.registerCount(); i++)
                _raw[phase][i] = _client.registers()[i];
            _blockOk[phase] = true;
        }
        else
        {
            _blockOk[phase] = false;
            logDebugP("Kanal %d: Block %d fehlgeschlagen (%s)", _channelIndex + 1, (int)phase,
                      SolarmanV5Client::resultText(result));
        }

        _blockPhase = (uint8_t)(phase + 1);
        if (_blockPhase >= _blockCount)
        {
            _blockPhase = BLOCK_IDLE;
            publishValues();
        }
    }
}

bool SolarmanChannel::rawWord(uint16_t reg, uint16_t& out) const
{
    for (uint8_t b = 0; b < _blockCount; b++)
    {
        if (!_blockOk[b])
            continue;
        if (reg >= _blockStart[b] && reg < (uint16_t)(_blockStart[b] + _blockWords[b]))
        {
            out = _raw[b][reg - _blockStart[b]];
            return true;
        }
    }
    return false;
}

// Sucht die aktive Zeile zu einem Slot des Katalogs. Wird nur fuer berechnete Werte
// gebraucht, deren Faktoren ueber die Slot-Nummer festgelegt sind.
bool SolarmanChannel::valueOfSlot(uint8_t slot, float& out) const
{
    for (uint8_t i = 0; i < _activeCount; i++)
        if (_rows[i].slot == slot)
            return valueOfRow(i, out);
    return false;
}

bool SolarmanChannel::valueOfRow(uint8_t rowIndex, float& out) const
{
    if (rowIndex >= _activeCount)
        return false;
    const Row& row = _rows[rowIndex];

    // Berechneter Wert: Produkt der beiden im KATALOG vorangehenden Slots (Spannung * Strom).
    // Bezug ist bewusst der Slot-Katalog und nicht die Reihenfolge der aktiven Zeilen: waehlt
    // jemand die Spannung ab, waere sonst stillschweigend eine fremde Zeile der Faktor.
    if (row.type == Spv::TypeProduct)
    {
        if (row.slot < 2)
            return false;
        float u = 0.0f, i = 0.0f;
        if (!valueOfSlot((uint8_t)(row.slot - 2), u) || !valueOfSlot((uint8_t)(row.slot - 1), i))
            return false;
        out = u * i;
        return true;
    }

    uint16_t low = 0;
    if (!rawWord(row.reg, low))
        return false;

    if (row.type == Spv::TypeS16)
    {
        out = (float)(int16_t)low * row.scale - row.offset;
        return true;
    }
    if (row.type == Spv::TypeU16)
    {
        out = (float)low * row.scale - row.offset;
        return true;
    }

    // 32 Bit, LOW-WORD ZUERST - bei beiden Geraeten am Objekt bestaetigt.
    uint16_t high = 0;
    if (!rawWord((uint16_t)(row.reg + 1), high))
        return false;
    const uint32_t raw = (uint32_t)low | (((uint32_t)high) << 16);

    if (row.type == Spv::TypeS32)
        out = (float)(int32_t)raw * row.scale - row.offset;
    else
        out = (float)raw * row.scale - row.offset;
    return true;
}

void SolarmanChannel::sendValue(uint8_t slot, float value)
{
    const uint8_t _channelIndex = this->_channelIndex;

    // KO 0 ist der Status, die Messwerte folgen in Katalogreihenfolge.
    GroupObject& ko = knx.getGroupObject(SPV_KoCalcNumber(slot + 1));
    const Spv::Slot& def = Spv::SLOTS[slot];

    switch (def.encoding)
    {
        case Spv::EncEnergyWh:
            // Die Tabelle liefert kWh; als DPT 13.010 gehen Wh auf den Bus, sonst ginge die
            // Nachkommastelle des Tagesertrags verloren.
            ko.value((int32_t)lroundf(value * 1000.0f), Dpt(def.dptMain, def.dptSub));
            break;
        case Spv::EncPercent:
            ko.value(value, Dpt(def.dptMain, def.dptSub));
            break;
        case Spv::EncTemp:
            ko.value(value, Dpt(def.dptMain, def.dptSub));
            break;
        case Spv::EncU16:
            ko.value((uint16_t)lroundf(value), Dpt(def.dptMain, def.dptSub));
            break;
        case Spv::EncU8:
            ko.value((uint8_t)lroundf(value), Dpt(def.dptMain, def.dptSub));
            break;
        case Spv::EncFloat:
        default:
            ko.value(value, Dpt(def.dptMain, def.dptSub));
            break;
    }
}

void SolarmanChannel::publishValues()
{
    const uint8_t _channelIndex = this->_channelIndex;
    const uint32_t cyclicMs = ParamSPV_CHSendDelayTimeMS;
    const uint8_t changePercent = ParamSPV_CHSendChangePercent;
    const bool cyclicDue = (cyclicMs != 0) && delayCheck(_lastCyclicMs, cyclicMs);

    for (uint8_t i = 0; i < _activeCount; i++)
    {
        float value = 0.0f;
        if (!valueOfRow(i, value))
            continue;

        const uint8_t slot = _rows[i].slot;
        bool send = !_sentOnce[slot] || cyclicDue;
        if (!send && changePercent != 0)
        {
            const float reference = fabsf(_lastSent[slot]);
            const float delta = fabsf(value - _lastSent[slot]);
            // Bezugsgroesse 0 -> jede Aenderung zaehlt, sonst gaebe es keine Prozentbasis.
            send = (reference < 0.001f) ? (delta > 0.0f) : ((delta / reference * 100.0f) >= changePercent);
        }

        if (send)
        {
            sendValue(slot, value);
            _lastSent[slot] = value;
            _sentOnce[slot] = true;
        }
    }

    if (cyclicDue)
        _lastCyclicMs = millis();
}

void SolarmanChannel::updateStatusKo()
{
    const uint8_t _channelIndex = this->_channelIndex;
    if (!_statusSent || _reachable != _lastReachable)
    {
        _lastReachable = _reachable;
        _statusSent = true;
        KoSPV_CHReachable.value(_reachable, DPT_Switch);
    }
}

bool SolarmanChannel::requestDump(uint16_t start, uint16_t count)
{
    if (_dumpPending || _dumpInFlight)
        return false;
    _dumpStart = start;
    _dumpCount = count;
    _dumpPending = true;
    return true;
}
