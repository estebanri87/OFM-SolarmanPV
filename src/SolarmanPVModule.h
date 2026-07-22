#pragma once
#include "ChannelOwnerModule.h"
#include "OpenKNX.h"

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

    void showHelp() override;
    bool processCommand(const std::string cmd, bool diagnoseKo) override;

  private:
    SolarmanChannel* channelAt(uint8_t index);
};

extern SolarmanPVModule openknxSolarmanPVModule;
