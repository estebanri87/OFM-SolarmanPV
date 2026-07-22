#pragma once
#include "OpenKNX.h"
#include "Profiles/DeyeMicro.h"
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
    // _diagPending = angefordert, _diagInFlight = laeuft gerade. Ohne das zweite Flag wuerde
    // eine spaetere zyklische Antwort faelschlich als Registerdump ausgegeben.
    bool _diagPending = false;
    bool _diagInFlight = false;
    uint16_t _diagStart = 0;
    uint16_t _diagCount = 0;
    uint8_t _diagFc = 3;

    // Rohregister der drei Leseblöcke aus dem Profil.
    static const uint8_t MAX_BLOCK_WORDS = 16;
    uint16_t _raw[DeyeMicro::BLOCK_COUNT][MAX_BLOCK_WORDS];
    bool _blockOk[DeyeMicro::BLOCK_COUNT] = {false};
    static const uint8_t BLOCK_IDLE = 0xFF;
    uint8_t _blockPhase = BLOCK_IDLE; // welcher Block gerade gelesen wird

    // Sendeverhalten je Wert (zyklisch und/oder bei Aenderung).
    float _lastSent[DeyeMicro::Count] = {0.0f};
    bool _sentOnce[DeyeMicro::Count] = {false};
    uint32_t _lastCyclicMs = 0;

    void startNextRequest();
    void handleFinished();
    void updateStatusKo();

    bool rawWord(uint16_t reg, uint16_t& out) const;
    bool valueOf(uint8_t index, float& out) const;
    bool valueEnabled(uint8_t index) const;
    void sendValue(uint8_t index, float value);
    void publishValues();
};

extern SolarmanPVModule openknxSolarmanPVModule;
