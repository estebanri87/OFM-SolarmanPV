#pragma once
#include "ChannelOwnerModule.h"
#include "OpenKNX.h"
#include "SolarmanAssistant.h"

class SolarmanChannel;

// Lokale Anbindung von Wechselrichtern und Batteriespeichern mit Solarman-Logger.
//
// Ein Kanal = ein Geraet, mit eigenem Transport (Solarman V5 oder Modbus TCP) und eigenem
// Profil. Die gesamte Geraetelogik liegt im Kanal; das Modul verwaltet nur die Kanaele und
// die Diagnosebefehle.
class SolarmanPVModule : public SPVChannelOwnerModule
{
  public:
    SolarmanPVModule();

    const std::string name() override;
    const std::string version() override;

    OpenKNX::Channel* createChannel(uint8_t _channelIndex /* in Makros verwendet, nicht umbenennen */) override;

    void loop(bool configured) override;

    void showHelp() override;
    bool processCommand(const std::string cmd, bool diagnoseKo) override;

    // Gegenstelle der Einmess-Seite in der ETS. objectIndex 0x9E ist von OGM-Common belegt,
    // 0xA0 vom HueGateway - SolarmanPV nimmt 0xA1.
    bool processFunctionProperty(uint8_t objectIndex, uint8_t propertyId, uint8_t length,
                                 uint8_t* data, uint8_t* resultData, uint8_t& resultLength) override;

  private:
    static const uint8_t FUNCTION_OBJECT = 0xA1;
    static const uint8_t FUNCTION_PROPERTY = 1;

    enum Command : uint8_t
    {
        CmdProbe = 1,   // Transport und Seriennummer ermitteln
        CmdStatus = 2,  // laeuft noch / fertig + Ergebnis
        CmdRead = 3,    // Registerblock lesen
        CmdFetch = 4,   // gelesene Register haeppchenweise abholen
    };

    SolarmanAssistant _assistant;

    SolarmanChannel* channelAt(uint8_t index);
};

extern SolarmanPVModule openknxSolarmanPVModule;
