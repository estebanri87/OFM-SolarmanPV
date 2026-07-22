#pragma once
#include "OpenKNX.h"
#include "SolarmanV5Client.h"

// Lokale Anbindung eines Wechselrichters mit Solarman-Logger (V5 ueber TCP, Port 8899).
//
// Phase 1: Verbindung, automatische Ermittlung der Logger-Seriennummer, Status-KO und ein
// Diagnose-Kommando zum Auslesen beliebiger Register. Damit laesst sich die Registertabelle
// des konkreten Wechselrichters am echten Geraet aufnehmen, bevor Profile und Messwertkanaele
// gebaut werden.
class SolarmanPVModule : public OpenKNX::Module
{
  public:
    const std::string name() override;
    const std::string version() override;

    void setup() override;
    void loop() override;

    void showHelp() override;
    bool processCommand(const std::string cmd, bool diagnoseKo) override;

  private:
    SolarmanV5Client _client;

    uint16_t _pollIntervalS = 0;
    uint32_t _lastPollMs = 0;
    bool _serialKnown = false;

    // Status-KO (send-on-change)
    bool _reachable = false;
    bool _lastReachable = false;
    bool _statusSent = false;

    // Diagnose: von der Konsole angestossene Leseanfrage.
    bool _diagPending = false;
    uint16_t _diagStart = 0;
    uint16_t _diagCount = 0;
    uint8_t _diagFc = 3;

    void startNextRequest();
    void handleFinished();
    void updateStatusKo();
};

extern SolarmanPVModule openknxSolarmanPVModule;
