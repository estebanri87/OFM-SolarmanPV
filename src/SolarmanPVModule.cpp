#include "SolarmanPVModule.h"
#include "SolarmanChannel.h"
#include "knxprod.h"
#include <stdio.h>

SolarmanPVModule::SolarmanPVModule()
    : SPVChannelOwnerModule(SPV_ChannelCount)
{
}

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

OpenKNX::Channel* SolarmanPVModule::createChannel(uint8_t _channelIndex /* in Makros verwendet */)
{
    // Nur konfigurierte Geraete anlegen. Kanaele jenseits von "Aktive Geraete" sind in der
    // ETS nicht sichtbar und haben keine gueltige Konfiguration.
    if (_channelIndex >= ParamSPV_SPVVisibleChannels)
        return nullptr;
    return new SolarmanChannel(_channelIndex);
}

SolarmanChannel* SolarmanPVModule::channelAt(uint8_t index)
{
    if (index >= getNumberOfChannels())
        return nullptr;
    return static_cast<SolarmanChannel*>(getChannel(index));
}

void SolarmanPVModule::showHelp()
{
    openknx.console.printHelpLine("spv", "SolarmanPV: Status aller Geräte");
    openknx.console.printHelpLine("spvread", "SolarmanPV: Register lesen, z.B. 'spvread 1 3b 10' (Gerät, Start hex, Anzahl hex)");
}

bool SolarmanPVModule::processCommand(const std::string cmd, bool diagnoseKo)
{
    if (cmd == "spv")
    {
        logInfoP("SolarmanPV: %d von %d Geräten aktiv", (int)getNumberOfUsedChannels(),
                 (int)getNumberOfChannels());
        logIndentUp();
        for (uint8_t i = 0; i < getNumberOfChannels(); i++)
        {
            SolarmanChannel* ch = channelAt(i);
            if (ch == nullptr)
                continue;
            logInfoP("Gerät %d: %s, %s, SN %u%s%s", i + 1, ch->profileName(),
                     ch->reachable() ? "erreichbar" : "nicht erreichbar",
                     (unsigned)ch->loggerSerial(),
                     ch->serialKnown() ? "" : " (wird ermittelt)",
                     ch->busy() ? ", Anfrage läuft" : "");
        }
        logIndentDown();
        return true;
    }

    // "spvread <Gerät> <StartHex> [AnzahlHex]" - Registerdump am echten Gerät. Damit
    // entstehen neue Profiltabellen.
    if (cmd.rfind("spvread", 0) == 0)
    {
        unsigned device = 0, start = 0, count = 1;
        const int parsed = sscanf(cmd.c_str(), "spvread %u %x %x", &device, &start, &count);
        if (parsed < 2)
        {
            logInfoP("Aufruf: spvread <Gerät> <StartHex> [AnzahlHex], z.B. 'spvread 1 3b 10'");
            return true;
        }
        if (device < 1 || device > getNumberOfChannels())
        {
            logInfoP("Gerät muss zwischen 1 und %d liegen", (int)getNumberOfChannels());
            return true;
        }
        if (count == 0 || count > SolarmanV5Client::MAX_REGISTERS)
        {
            logInfoP("Anzahl muss zwischen 1 und %d liegen", (int)SolarmanV5Client::MAX_REGISTERS);
            return true;
        }

        SolarmanChannel* ch = channelAt((uint8_t)(device - 1));
        if (ch == nullptr || !ch->configured())
        {
            logInfoP("Gerät %u ist nicht konfiguriert", device);
            return true;
        }
        if (!ch->requestDump((uint16_t)start, (uint16_t)count))
        {
            logInfoP("Gerät %u: Dump läuft bereits, bitte Ergebnis abwarten", device);
            return true;
        }
        logInfoP("Gerät %u: lese %u Register ab 0x%04X ...", device, count, start);
        return true;
    }

    return false;
}
