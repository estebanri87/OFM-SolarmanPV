#include "SolarmanPVModule.h"
#include "NetworkModule.h"
#include "knxprod.h"
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
    const uint32_t serial = ParamSPV_LoggerSerial;
    _pollIntervalS = ParamSPV_PollInterval;

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

    // Diagnoseanfrage von der Konsole hat Vorrang.
    if (_diagPending)
    {
        _diagPending = false;
        if (!_client.beginRead(_diagFc, _diagStart, _diagCount))
            logInfoP("spvread: Anfrage abgelehnt (%s)", SolarmanV5Client::resultText(_client.result()));
        return;
    }

    // Ohne Seriennummer geht nichts: der Logger weist Anfragen mit falscher SN ab. Sie steht
    // aber im Header JEDER Antwort - auch im Fehlerrahmen.
    if (!_serialKnown)
    {
        _client.beginDiscoverSerial();
        return;
    }

    if (_pollIntervalS == 0)
        return;
    const uint32_t now = millis();
    if (_lastPollMs != 0 && (now - _lastPollMs) < (uint32_t)_pollIntervalS * 1000UL)
        return;
    _lastPollMs = now;

    // Phase 1: ein fester Erreichbarkeits-Ping. Die eigentlichen Messwertbloecke kommen mit
    // den Profilen.
    _client.beginRead(3, 0x003B, 1);
}

void SolarmanPVModule::handleFinished()
{
    const SolarmanV5Client::Result result = _client.result();
    const bool ok = (result == SolarmanV5Client::Ok);
    _reachable = ok;

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

    if (_diagCount > 0)
    {
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
        _diagCount = 0;
        return;
    }

    if (!ok)
        logDebugP("SolarmanPV: Abfrage fehlgeschlagen (%s)", SolarmanV5Client::resultText(result));
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
