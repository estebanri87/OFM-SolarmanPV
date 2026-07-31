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

void SolarmanPVModule::loop(bool configured)
{
    // Waehrend einer Assistenten-Messung ruhen die Kanaele. Ein Solarman-Logger beantwortet
    // nur einen Client zuverlaessig - und der Kanal-Poll ist selbst ein zweiter Client.
    // Ohne diese Pause konkurrieren beide um dasselbe Geraet.
    if (!_assistant.running())
        SPVChannelOwnerModule::loop(configured);

    // Der Assistent laeuft auch ohne gueltige Konfiguration: er ist das Werkzeug, mit dem
    // eine solche ueberhaupt erst entsteht.
    _assistant.loop();
}

// Alle Antworten beginnen mit [0]=Status (0 = angenommen) und [1]=fertig (0 = laeuft noch).
// Dieselbe Aufteilung nutzt das OFM-PresenceModule; das ETS-Skript pollt darauf.
bool SolarmanPVModule::processFunctionProperty(uint8_t objectIndex, uint8_t propertyId,
                                               uint8_t length, uint8_t* data,
                                               uint8_t* resultData, uint8_t& resultLength)
{
    if (objectIndex != FUNCTION_OBJECT || propertyId != FUNCTION_PROPERTY || length < 1)
        return false;

    const uint8_t command = data[0];

    switch (command)
    {
        // [1..4] IP, [5..6] Port, [7] Slave-ID, [8..9] Probe-Register
        // Die Adresse kommt aus dem Aufruf: das Geraet kennt nur die zuletzt HERUNTERGELADENEN
        // Parameter, nicht die gerade in der ETS eingetippte IP.
        case CmdProbe:
        {
            if (length < 10)
                return false;
            const uint32_t ip = ((uint32_t)data[1] << 24) | ((uint32_t)data[2] << 16) |
                                ((uint32_t)data[3] << 8) | data[4];
            const uint16_t port = (uint16_t)((data[5] << 8) | data[6]);
            const uint16_t probe = (uint16_t)((data[8] << 8) | data[9]);
            _assistant.startProbe(ip, port, data[7], probe);
            resultData[0] = 0;
            resultData[1] = 0;
            resultLength = 2;
            return true;
        }

        case CmdStatus:
        {
            resultData[0] = 0;
            if (_assistant.running())
            {
                resultData[1] = 0;
                resultLength = 2;
                return true;
            }
            resultData[1] = 1;
            resultData[2] = _assistant.transport();
            const uint32_t serial = _assistant.serial();
            resultData[3] = (uint8_t)(serial >> 24);
            resultData[4] = (uint8_t)(serial >> 16);
            resultData[5] = (uint8_t)(serial >> 8);
            resultData[6] = (uint8_t)serial;
            resultData[7] = _assistant.error();
            resultData[8] = _assistant.registerCount();
            resultLength = 9;
            return true;
        }

        // [1..4] IP, [5..6] Port, [7] Slave-ID, [8] Transport, [9..10] Startregister, [11] Anzahl
        case CmdRead:
        {
            if (length < 12)
                return false;
            const uint32_t ip = ((uint32_t)data[1] << 24) | ((uint32_t)data[2] << 16) |
                                ((uint32_t)data[3] << 8) | data[4];
            const uint16_t port = (uint16_t)((data[5] << 8) | data[6]);
            const uint16_t start = (uint16_t)((data[9] << 8) | data[10]);
            _assistant.startRead(ip, port, data[7], data[8], start, data[11]);
            resultData[0] = 0;
            resultData[1] = 0;
            resultLength = 2;
            return true;
        }

        // [1] Offset im Ergebnis. Haeppchenweise, damit die Antwort in eine APDU passt.
        case CmdFetch:
        {
            if (length < 2)
                return false;
            const uint8_t offset = data[1];
            const uint8_t available = _assistant.registerCount();
            uint8_t count = (offset >= available) ? 0 : (uint8_t)(available - offset);
            if (count > SolarmanAssistant::CHUNK_REGISTERS)
                count = SolarmanAssistant::CHUNK_REGISTERS;

            resultData[0] = 0;
            resultData[1] = 1;
            resultData[2] = count;
            for (uint8_t i = 0; i < count; i++)
            {
                const uint16_t value = _assistant.registers()[offset + i];
                resultData[3 + i * 2] = (uint8_t)(value >> 8);
                resultData[4 + i * 2] = (uint8_t)value;
            }
            resultLength = (uint8_t)(3 + count * 2);
            return true;
        }

        default:
            return false;
    }
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
            logInfoP("Gerät %d: %d Messwerte in %d Blöcken, %s, SN %u%s%s", i + 1,
                     (int)ch->activeSlots(), (int)ch->blockCount(),
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
