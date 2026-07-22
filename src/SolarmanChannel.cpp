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
    _profile = Spv::profileFor(ParamSPV_CHProfile);
    _pollIntervalS = ParamSPV_CHPollInterval;

    const SolarmanV5Client::Transport transport =
        (ParamSPV_CHTransport == Spv::ModbusTcp) ? SolarmanV5Client::ModbusTcp : SolarmanV5Client::V5;

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

    logDebugP("Kanal %d: %s, %s, %s:%d, Intervall %ds", _channelIndex + 1, _profile.name,
              (transport == SolarmanV5Client::ModbusTcp) ? "Modbus TCP" : "Solarman V5",
              (ip != nullptr && ip[0]) ? ip : "(keine IP)", (int)ParamSPV_CHLoggerPort,
              (int)_pollIntervalS);
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
    if (_blockPhase != BLOCK_IDLE && _blockPhase < _profile.blockCount)
    {
        const Spv::Block& b = _profile.blocks[_blockPhase];
        _client.beginRead(3, b.start, b.count);
        return;
    }

    if (_pollIntervalS == 0)
        return;
    const uint32_t now = millis();
    if (_lastPollMs != 0 && (now - _lastPollMs) < (uint32_t)_pollIntervalS * 1000UL)
        return;
    _lastPollMs = now;

    _blockPhase = 0;
    const Spv::Block& b = _profile.blocks[0];
    _client.beginRead(3, b.start, b.count);
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

    if (_blockPhase != BLOCK_IDLE && _blockPhase < _profile.blockCount)
    {
        const uint8_t phase = _blockPhase;
        if (ok && _client.registerCount() <= Spv::MAX_BLOCK_WORDS)
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
        if (_blockPhase >= _profile.blockCount)
        {
            _blockPhase = BLOCK_IDLE;
            publishValues();
        }
    }
}

bool SolarmanChannel::rawWord(uint16_t reg, uint16_t& out) const
{
    for (uint8_t b = 0; b < _profile.blockCount; b++)
    {
        if (!_blockOk[b])
            continue;
        const Spv::Block& blk = _profile.blocks[b];
        if (reg >= blk.start && reg < (uint16_t)(blk.start + blk.count))
        {
            out = _raw[b][reg - blk.start];
            return true;
        }
    }
    return false;
}

bool SolarmanChannel::valueOf(uint8_t index, float& out) const
{
    if (index >= _profile.count)
        return false;
    const Spv::Def& def = _profile.defs[index];

    // Berechneter Wert: Produkt der beiden vorangehenden Eintraege (Spannung * Strom).
    if (def.words == 0)
    {
        if (index < 2)
            return false;
        float u = 0.0f, i = 0.0f;
        if (!valueOf((uint8_t)(index - 2), u) || !valueOf((uint8_t)(index - 1), i))
            return false;
        out = u * i;
        return true;
    }

    uint16_t low = 0;
    if (!rawWord(def.reg, low))
        return false;

    if (def.words == 1 && def.isSigned)
    {
        out = (float)(int16_t)low * def.scale - def.offset;
        return true;
    }

    uint32_t raw = low;
    if (def.words == 2)
    {
        // 32 Bit, LOW-WORD ZUERST - bei beiden Geraeten am Objekt bestaetigt.
        uint16_t high = 0;
        if (!rawWord((uint16_t)(def.reg + 1), high))
            return false;
        raw |= ((uint32_t)high) << 16;
    }

    out = (float)raw * def.scale - def.offset;
    return true;
}

bool SolarmanChannel::valueEnabled(uint8_t index) const
{
    const uint8_t _channelIndex = this->_channelIndex;
    if (ParamSPV_CHProfile == Spv::PylontechForceId)
    {
        switch (index)
        {
            case PylontechForce::Soc:               return ParamSPV_CHEnPSoc;
            case PylontechForce::Voltage:           return ParamSPV_CHEnPVoltage;
            case PylontechForce::Current:           return ParamSPV_CHEnPCurrent;
            case PylontechForce::Power:             return ParamSPV_CHEnPPower;
            case PylontechForce::Temperature:       return ParamSPV_CHEnPTemperature;
            case PylontechForce::Soh:               return ParamSPV_CHEnPSoh;
            case PylontechForce::RemainingCapacity: return ParamSPV_CHEnPRemainingCapacity;
            case PylontechForce::CycleTimes:        return ParamSPV_CHEnPCycleTimes;
            case PylontechForce::TodayCharge:       return ParamSPV_CHEnPTodayCharge;
            case PylontechForce::TodayDischarge:    return ParamSPV_CHEnPTodayDischarge;
            case PylontechForce::TotalCharge:       return ParamSPV_CHEnPTotalCharge;
            case PylontechForce::TotalDischarge:    return ParamSPV_CHEnPTotalDischarge;
            default:                                return false;
        }
    }
    switch (index)
    {
        case DeyeMicro::Power:         return ParamSPV_CHEnDPower;
        case DeyeMicro::Today:         return ParamSPV_CHEnDToday;
        case DeyeMicro::Total:         return ParamSPV_CHEnDTotal;
        case DeyeMicro::GridVoltage:   return ParamSPV_CHEnDGridVoltage;
        case DeyeMicro::GridCurrent:   return ParamSPV_CHEnDGridCurrent;
        case DeyeMicro::GridFrequency: return ParamSPV_CHEnDGridFrequency;
        case DeyeMicro::Temperature:   return ParamSPV_CHEnDTemperature;
        case DeyeMicro::Pv1Voltage:    return ParamSPV_CHEnDPv1Voltage;
        case DeyeMicro::Pv1Current:    return ParamSPV_CHEnDPv1Current;
        case DeyeMicro::Pv1Power:      return ParamSPV_CHEnDPv1Power;
        case DeyeMicro::Pv2Voltage:    return ParamSPV_CHEnDPv2Voltage;
        case DeyeMicro::Pv2Current:    return ParamSPV_CHEnDPv2Current;
        case DeyeMicro::Pv2Power:      return ParamSPV_CHEnDPv2Power;
        case DeyeMicro::Today1:        return ParamSPV_CHEnDToday1;
        case DeyeMicro::Today2:        return ParamSPV_CHEnDToday2;
        case DeyeMicro::Total1:        return ParamSPV_CHEnDTotal1;
        case DeyeMicro::Total2:        return ParamSPV_CHEnDTotal2;
        default:                       return false;
    }
}

void SolarmanChannel::sendValue(uint8_t index, float value)
{
    const uint8_t _channelIndex = this->_channelIndex;

    // Slot 1..17 entspricht Wertindex 0..16 - die Reihenfolge im Profil legt fest, welcher
    // Messwert auf welchem KO landet. Die ETS zeigt dafuer den passenden Namen und DPT.
    GroupObject* ko = nullptr;
    switch (index)
    {
        case 0:  ko = &KoSPV_CHValue01; break;
        case 1:  ko = &KoSPV_CHValue02; break;
        case 2:  ko = &KoSPV_CHValue03; break;
        case 3:  ko = &KoSPV_CHValue04; break;
        case 4:  ko = &KoSPV_CHValue05; break;
        case 5:  ko = &KoSPV_CHValue06; break;
        case 6:  ko = &KoSPV_CHValue07; break;
        case 7:  ko = &KoSPV_CHValue08; break;
        case 8:  ko = &KoSPV_CHValue09; break;
        case 9:  ko = &KoSPV_CHValue10; break;
        case 10: ko = &KoSPV_CHValue11; break;
        case 11: ko = &KoSPV_CHValue12; break;
        case 12: ko = &KoSPV_CHValue13; break;
        case 13: ko = &KoSPV_CHValue14; break;
        case 14: ko = &KoSPV_CHValue15; break;
        case 15: ko = &KoSPV_CHValue16; break;
        case 16: ko = &KoSPV_CHValue17; break;
        default: return;
    }

    switch (_profile.defs[index].knx)
    {
        case Spv::EnergyWh:
            // Profil liefert kWh; als DPT 13.010 gehen Wh auf den Bus, sonst ginge die
            // Nachkommastelle des Tagesertrags verloren.
            ko->value((int32_t)lroundf(value * 1000.0f), DPT_ActiveEnergy);
            break;
        case Spv::Percent:
            ko->value(value, DPT_Scaling);
            break;
        case Spv::Temp2:
            ko->value(value, DPT_Value_Temp);
            break;
        case Spv::Counter16:
            ko->value((uint16_t)lroundf(value), DPT_Value_2_Ucount);
            break;
        case Spv::Float32:
        default:
            ko->value(value, DPT_Value_Power);
            break;
    }
}

void SolarmanChannel::publishValues()
{
    const uint8_t _channelIndex = this->_channelIndex;
    const uint32_t cyclicMs = ParamSPV_CHSendDelayTimeMS;
    const uint8_t changePercent = ParamSPV_CHSendChangePercent;
    const bool cyclicDue = (cyclicMs != 0) && delayCheck(_lastCyclicMs, cyclicMs);

    for (uint8_t i = 0; i < _profile.count && i < Spv::MAX_VALUES; i++)
    {
        if (!valueEnabled(i))
            continue;
        float value = 0.0f;
        if (!valueOf(i, value))
            continue;

        bool send = !_sentOnce[i] || cyclicDue;
        if (!send && changePercent != 0)
        {
            const float reference = fabsf(_lastSent[i]);
            const float delta = fabsf(value - _lastSent[i]);
            // Bezugsgroesse 0 -> jede Aenderung zaehlt, sonst gaebe es keine Prozentbasis.
            send = (reference < 0.001f) ? (delta > 0.0f) : ((delta / reference * 100.0f) >= changePercent);
        }

        if (send)
        {
            sendValue(i, value);
            _lastSent[i] = value;
            _sentOnce[i] = true;
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
