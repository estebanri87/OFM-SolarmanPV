#include "SolarmanPVModule.h"
#include "NetworkModule.h"
#include "knxprod.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

const std::string SolarmanPVModule::name()
{
    return "SolarmanPV";
}

const std::string SolarmanPVModule::version()
{
#ifdef MODULE_SolarmanPV_Version
    return MODULE_SolarmanPV_Version;
#else
    return "";
#endif
}

void SolarmanPVModule::setup()
{
    const char* ip = (const char*)ParamSPV_LoggerIp;
    _pollIntervalS = ParamSPV_PollInterval;

    // Seriennummer als Text: sie ist zehnstellig und kann groesser als 2^31-1 sein
    // (Testgeraet 3912915352), was ETS als Zahlenfeld nicht zuverlaessig darstellt.
    uint32_t serial = 0;
    if (ParamSPV_SerialMode == 1) // manuell eintragen
    {
        const char* text = (const char*)ParamSPV_LoggerSerialText;
        if (text != nullptr && text[0] != 0)
            serial = (uint32_t)strtoul(text, nullptr, 10);
    }

    _client.configure(ip, (uint16_t)ParamSPV_LoggerPort, serial, (uint8_t)ParamSPV_SlaveId);
    _serialKnown = (serial != 0);

    logInfoP("SolarmanPV: %s:%d slave=%d intervall=%ds serial=%s",
             (ip != nullptr && ip[0]) ? ip : "(keine IP)", (int)ParamSPV_LoggerPort,
             (int)ParamSPV_SlaveId, (int)_pollIntervalS,
             _serialKnown ? "konfiguriert" : "wird ermittelt");
}

void SolarmanPVModule::loop()
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

void SolarmanPVModule::startNextRequest()
{
    if (!openknxNetwork.established())
        return;

    // Ohne Seriennummer geht nichts: der Logger weist Anfragen mit falscher SN ab. Sie steht
    // aber im Header JEDER Antwort - auch im Fehlerrahmen. Das muss VOR der Diagnoseanfrage
    // passieren, sonst liefe deren Antwort ins Leere.
    if (!_serialKnown)
    {
        _client.beginDiscoverSerial();
        return;
    }

    // Diagnoseanfrage von der Konsole hat Vorrang vor der zyklischen Abfrage.
    if (_diagPending)
    {
        _diagPending = false;
        _diagInFlight = true;
        if (!_client.beginRead(_diagFc, _diagStart, _diagCount))
        {
            _diagInFlight = false;
            logInfoP("spvread: Anfrage abgelehnt (%s)", SolarmanV5Client::resultText(_client.result()));
        }
        return;
    }

    // Laufenden Lesezyklus fortsetzen: die Bloecke des Profils nacheinander.
    if (_blockPhase != BLOCK_IDLE && _blockPhase < DeyeMicro::BLOCK_COUNT)
    {
        const Spv::Block& b = DeyeMicro::BLOCKS[_blockPhase];
        _client.beginRead(3, b.start, b.count);
        return;
    }

    if (_pollIntervalS == 0)
        return;
    const uint32_t now = millis();
    if (_lastPollMs != 0 && (now - _lastPollMs) < (uint32_t)_pollIntervalS * 1000UL)
        return;
    _lastPollMs = now;

    // Neuen Zyklus starten.
    _blockPhase = 0;
    const Spv::Block& b = DeyeMicro::BLOCKS[0];
    _client.beginRead(3, b.start, b.count);
}

bool SolarmanPVModule::rawWord(uint16_t reg, uint16_t& out) const
{
    for (uint8_t b = 0; b < DeyeMicro::BLOCK_COUNT; b++)
    {
        if (!_blockOk[b])
            continue;
        const Spv::Block& blk = DeyeMicro::BLOCKS[b];
        if (reg >= blk.start && reg < (uint16_t)(blk.start + blk.count))
        {
            out = _raw[b][reg - blk.start];
            return true;
        }
    }
    return false;
}

bool SolarmanPVModule::valueOf(uint8_t index, float& out) const
{
    const Spv::Def& def = DeyeMicro::DEFS[index];

    // Berechnete Werte: die PV-Leistungen liefert das Geraet nicht, sie ergeben sich aus U*I
    // (so macht es auch die Solarman-Integration).
    if (def.words == 0)
    {
        float u = 0.0f, i = 0.0f;
        const bool first = (index == DeyeMicro::Pv1Power);
        if (!valueOf(first ? DeyeMicro::Pv1Voltage : DeyeMicro::Pv2Voltage, u) ||
            !valueOf(first ? DeyeMicro::Pv1Current : DeyeMicro::Pv2Current, i))
            return false;
        out = u * i;
        return true;
    }

    uint16_t low = 0;
    if (!rawWord(def.reg, low))
        return false;

    uint32_t raw = low;
    if (def.words == 1 && def.isSigned)
    {
        // 16 Bit vorzeichenbehaftet (Batteriestrom, Temperatur): Vorzeichen erweitern.
        out = (float)(int16_t)low * def.scale - def.offset;
        return true;
    }
    if (def.words == 2)
    {
        // 32 Bit, LOW-WORD ZUERST - am Geraet bestaetigt (High-Word steht dahinter).
        uint16_t high = 0;
        if (!rawWord((uint16_t)(def.reg + 1), high))
            return false;
        raw |= ((uint32_t)high) << 16;
    }

    out = (float)raw * def.scale - def.offset;
    return true;
}

void SolarmanPVModule::handleFinished()
{
    const SolarmanV5Client::Result result = _client.result();
    const bool ok = (result == SolarmanV5Client::Ok);
    _reachable = ok;

    // Zuerst die Diagnoseanfrage: das Flag gehoert immer zurueckgesetzt, sonst wuerde eine
    // spaetere zyklische Antwort faelschlich als Registerdump ausgegeben.
    if (_diagInFlight)
    {
        _diagInFlight = false;
        if (ok)
        {
            logInfoP("spvread: %d Register ab 0x%04X (FC %d)", (int)_client.registerCount(),
                     (int)_diagStart, (int)_diagFc);
            logIndentUp();
            for (uint16_t i = 0; i < _client.registerCount(); i++)
            {
                const uint16_t raw = _client.registers()[i];
                logInfoP("0x%04X = %5u (0x%04X, signed %6d)", (unsigned)(_diagStart + i),
                         (unsigned)raw, (unsigned)raw, (int)(int16_t)raw);
            }
            logIndentDown();
        }
        else
        {
            logInfoP("spvread: fehlgeschlagen (%s)", SolarmanV5Client::resultText(result));
        }
        return;
    }

    if (!_serialKnown)
    {
        if (ok && _client.loggerSerial() != 0)
        {
            _serialKnown = true;
            _client.setLoggerSerial(_client.loggerSerial());
            logInfoP("SolarmanPV: Logger-Seriennummer ermittelt: %u", (unsigned)_client.loggerSerial());
        }
        else
        {
            logDebugP("SolarmanPV: Seriennummer noch nicht ermittelt (%s)",
                      SolarmanV5Client::resultText(result));
        }
        return;
    }

    // Laufender Lesezyklus: Block ablegen und zum naechsten weitergehen.
    if (_blockPhase != BLOCK_IDLE && _blockPhase < DeyeMicro::BLOCK_COUNT)
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
            logDebugP("SolarmanPV: Block %d fehlgeschlagen (%s)", (int)phase,
                      SolarmanV5Client::resultText(result));
        }

        _blockPhase = (uint8_t)(phase + 1);
        if (_blockPhase >= DeyeMicro::BLOCK_COUNT)
        {
            _blockPhase = BLOCK_IDLE;
            publishValues();
        }
        return;
    }

    if (!ok)
        logDebugP("SolarmanPV: Abfrage fehlgeschlagen (%s)", SolarmanV5Client::resultText(result));
}

bool SolarmanPVModule::valueEnabled(uint8_t index) const
{
    switch (index)
    {
        case DeyeMicro::Power:         return (bool)ParamSPV_EnPower;
        case DeyeMicro::Today:         return (bool)ParamSPV_EnToday;
        case DeyeMicro::Total:         return (bool)ParamSPV_EnTotal;
        case DeyeMicro::GridVoltage:   return (bool)ParamSPV_EnGridVoltage;
        case DeyeMicro::GridCurrent:   return (bool)ParamSPV_EnGridCurrent;
        case DeyeMicro::GridFrequency: return (bool)ParamSPV_EnGridFrequency;
        case DeyeMicro::Temperature:   return (bool)ParamSPV_EnTemperature;
        case DeyeMicro::Pv1Voltage:    return (bool)ParamSPV_EnPv1Voltage;
        case DeyeMicro::Pv1Current:    return (bool)ParamSPV_EnPv1Current;
        case DeyeMicro::Pv1Power:      return (bool)ParamSPV_EnPv1Power;
        case DeyeMicro::Pv2Voltage:    return (bool)ParamSPV_EnPv2Voltage;
        case DeyeMicro::Pv2Current:    return (bool)ParamSPV_EnPv2Current;
        case DeyeMicro::Pv2Power:      return (bool)ParamSPV_EnPv2Power;
        case DeyeMicro::Today1:        return (bool)ParamSPV_EnToday1;
        case DeyeMicro::Today2:        return (bool)ParamSPV_EnToday2;
        case DeyeMicro::Total1:        return (bool)ParamSPV_EnTotal1;
        case DeyeMicro::Total2:        return (bool)ParamSPV_EnTotal2;
        default:                       return false;
    }
}

void SolarmanPVModule::sendValue(uint8_t index, float value)
{
    // Energie kommt aus dem Profil in kWh, geht aber als DPT 13.010 in Wh auf den Bus -
    // sonst gingen die 0,1 kWh Aufloesung des Tagesertrags verloren.
    const int32_t wh = (int32_t)lroundf(value * 1000.0f);

    switch (index)
    {
        case DeyeMicro::Power:         KoSPV_Power.value(value, DPT_Value_Power); break;
        case DeyeMicro::Today:         KoSPV_Today.value(wh, DPT_ActiveEnergy); break;
        case DeyeMicro::Total:         KoSPV_Total.value(wh, DPT_ActiveEnergy); break;
        case DeyeMicro::GridVoltage:   KoSPV_GridVoltage.value(value, DPT_Value_Electric_Potential); break;
        case DeyeMicro::GridCurrent:   KoSPV_GridCurrent.value(value, DPT_Value_Electric_Current); break;
        case DeyeMicro::GridFrequency: KoSPV_GridFrequency.value(value, DPT_Value_Frequency); break;
        case DeyeMicro::Temperature:   KoSPV_Temperature.value(value, DPT_Value_Temp); break;
        case DeyeMicro::Pv1Voltage:    KoSPV_Pv1Voltage.value(value, DPT_Value_Electric_Potential); break;
        case DeyeMicro::Pv1Current:    KoSPV_Pv1Current.value(value, DPT_Value_Electric_Current); break;
        case DeyeMicro::Pv1Power:      KoSPV_Pv1Power.value(value, DPT_Value_Power); break;
        case DeyeMicro::Pv2Voltage:    KoSPV_Pv2Voltage.value(value, DPT_Value_Electric_Potential); break;
        case DeyeMicro::Pv2Current:    KoSPV_Pv2Current.value(value, DPT_Value_Electric_Current); break;
        case DeyeMicro::Pv2Power:      KoSPV_Pv2Power.value(value, DPT_Value_Power); break;
        case DeyeMicro::Today1:        KoSPV_Today1.value(wh, DPT_ActiveEnergy); break;
        case DeyeMicro::Today2:        KoSPV_Today2.value(wh, DPT_ActiveEnergy); break;
        case DeyeMicro::Total1:        KoSPV_Total1.value(wh, DPT_ActiveEnergy); break;
        case DeyeMicro::Total2:        KoSPV_Total2.value(wh, DPT_ActiveEnergy); break;
        default: break;
    }
}

void SolarmanPVModule::publishValues()
{
    const uint32_t cyclicMs = ParamSPV_SendDelayTimeMS;
    const uint8_t changePercent = ParamSPV_SendChangePercent;
    const bool cyclicDue = (cyclicMs != 0) && delayCheck(_lastCyclicMs, cyclicMs);

    for (uint8_t i = 0; i < DeyeMicro::Count; i++)
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

void SolarmanPVModule::updateStatusKo()
{
    if (!_statusSent || _reachable != _lastReachable)
    {
        _lastReachable = _reachable;
        _statusSent = true;
        KoSPV_LoggerReachable.value(_reachable, DPT_Switch);
    }
}

void SolarmanPVModule::showHelp()
{
    openknx.console.printHelpLine("spv", "SolarmanPV: Verbindungsstatus und Logger-Seriennummer");
    openknx.console.printHelpLine("spvread", "SolarmanPV: Register lesen, z.B. 'spvread 3b 10' (hex Start, hex Anzahl)");
}

bool SolarmanPVModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd == "spv")
    {
        logInfoP("SolarmanPV: %s, Seriennummer %s (%u), %s",
                 _reachable ? "erreichbar" : "nicht erreichbar",
                 _serialKnown ? "bekannt" : "unbekannt",
                 (unsigned)_client.loggerSerial(),
                 _client.busy() ? "Anfrage laeuft" : "bereit");
        return true;
    }

    // "spvread <startHex> [countHex]" - Register am echten Geraet auslesen. Damit entsteht
    // die Registertabelle fuer das Profil.
    if (cmd.rfind("spvread", 0) == 0)
    {
        // Nur eine Anfrage gleichzeitig: es gibt einen Diagnose-Slot. Ein zweiter Befehl
        // wuerde Startadresse und Anzahl ueberschreiben, waehrend die erste Antwort noch
        // unterwegs ist - die Ausgabe traege dann falsche Registeradressen.
        if (_diagPending || _diagInFlight)
        {
            logInfoP("spvread: laeuft bereits, bitte Ergebnis abwarten");
            return true;
        }

        unsigned start = 0, count = 1;
        const int parsed = sscanf(cmd.c_str(), "spvread %x %x", &start, &count);
        if (parsed < 1)
        {
            logInfoP("Aufruf: spvread <StartHex> [AnzahlHex], z.B. 'spvread 3b 10'");
            return true;
        }
        if (count == 0 || count > SolarmanV5Client::MAX_REGISTERS)
        {
            logInfoP("Anzahl muss zwischen 1 und %d liegen", (int)SolarmanV5Client::MAX_REGISTERS);
            return true;
        }
        _diagStart = (uint16_t)start;
        _diagCount = (uint16_t)count;
        _diagFc = 3;
        _diagPending = true;
        logInfoP("spvread: lese %u Register ab 0x%04X ...", count, start);
        return true;
    }

    return false;
}
