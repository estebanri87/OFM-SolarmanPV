#pragma once
#include "OpenKNX.h"
#include "Profiles/Profiles.h"
#include "SolarmanV5Client.h"

// Ein Kanal = ein Geraet (Wechselrichter oder Batteriespeicher) mit eigener Verbindung,
// eigenem Transport und eigenem Profil. Der Kanal kennt die konkreten Geraete nicht, er
// arbeitet nur gegen die Profilbeschreibung aus Spv::Info.
class SolarmanChannel : public OpenKNX::Channel
{
  public:
    explicit SolarmanChannel(uint8_t channelIndex);

    const std::string name() override;
    void setup() override;
    void loop() override;

    // Diagnose fuer die Konsole
    bool configured() const { return _client.configured(); }
    bool reachable() const { return _reachable; }
    uint32_t loggerSerial() const { return _client.loggerSerial(); }
    const char* profileName() const { return _profile.name; }
    bool serialKnown() const { return _serialKnown; }
    bool busy() const { return _client.busy(); }

    // Registerdump anfordern (Konsole). false, wenn schon einer laeuft.
    bool requestDump(uint16_t start, uint16_t count);

  private:
    uint8_t _channelIndex;
    SolarmanV5Client _client;
    Spv::Info _profile{};

    uint16_t _pollIntervalS = 0;
    uint32_t _lastPollMs = 0;
    bool _serialKnown = false;

    bool _reachable = false;
    bool _lastReachable = false;
    bool _statusSent = false;

    static const uint8_t BLOCK_IDLE = 0xFF;
    uint8_t _blockPhase = BLOCK_IDLE;
    uint16_t _raw[4][Spv::MAX_BLOCK_WORDS] = {};
    bool _blockOk[4] = {false};

    float _lastSent[Spv::MAX_VALUES] = {0.0f};
    bool _sentOnce[Spv::MAX_VALUES] = {false};
    uint32_t _lastCyclicMs = 0;

    bool _dumpPending = false;
    bool _dumpInFlight = false;
    uint16_t _dumpStart = 0;
    uint16_t _dumpCount = 0;

    void startNextRequest();
    void handleFinished();
    void updateStatusKo();

    bool rawWord(uint16_t reg, uint16_t& out) const;
    bool valueOf(uint8_t index, float& out) const;
    bool valueEnabled(uint8_t index) const;
    void sendValue(uint8_t index, float value);
    void publishValues();
};
