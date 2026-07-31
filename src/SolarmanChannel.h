#pragma once
#include "OpenKNX.h"
#include "SlotCatalog.h"
#include "SolarmanV5Client.h"

// Ein Kanal = ein Geraet (Wechselrichter oder Batteriespeicher) mit eigener Verbindung und
// eigenem Transport. Welche Register gelesen und wie sie umgerechnet werden, steht
// vollstaendig in den ETS-Parametern - der Kanal kennt keine Geraete und keine Profile.
// Ein unbekanntes Geraet laesst sich damit ohne Firmware-Aenderung einbinden.
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
    uint8_t activeSlots() const { return _activeCount; }
    uint8_t blockCount() const { return _blockCount; }
    bool serialKnown() const { return _serialKnown; }
    bool busy() const { return _client.busy(); }

    // Registerdump anfordern (Konsole). false, wenn schon einer laeuft.
    bool requestDump(uint16_t start, uint16_t count);

  private:
    // Ein Leseblock deckt hoechstens MAX_BLOCK_WORDS aufeinanderfolgende Register ab.
    // 16 ist der am Deye bestaetigte Wert; groessere Anfragen lehnen manche Logger ab.
    static const uint8_t MAX_BLOCK_WORDS = 16;
    static const uint8_t MAX_BLOCKS = 16;
    static const uint8_t BLOCK_IDLE = 0xFF;

    struct Row
    {
        uint8_t slot;    // Index in Spv::SLOTS
        uint16_t reg;    // Startregister
        uint8_t type;    // Spv::RowType
        float scale;
        int8_t offset;
    };

    uint8_t _channelIndex;
    SolarmanV5Client _client;

    Row _rows[Spv::SLOT_COUNT] = {};
    uint8_t _activeCount = 0;

    uint16_t _blockStart[MAX_BLOCKS] = {};
    uint8_t _blockWords[MAX_BLOCKS] = {};
    uint8_t _blockCount = 0;

    uint16_t _pollIntervalS = 0;
    uint32_t _lastPollMs = 0;
    bool _serialKnown = false;

    bool _reachable = false;
    bool _lastReachable = false;
    bool _statusSent = false;

    uint8_t _blockPhase = BLOCK_IDLE;
    uint16_t _raw[MAX_BLOCKS][MAX_BLOCK_WORDS] = {};
    bool _blockOk[MAX_BLOCKS] = {false};

    float _lastSent[Spv::SLOT_COUNT] = {0.0f};
    bool _sentOnce[Spv::SLOT_COUNT] = {false};
    uint32_t _lastCyclicMs = 0;

    bool _dumpPending = false;
    bool _dumpInFlight = false;
    uint16_t _dumpStart = 0;
    uint16_t _dumpCount = 0;

    void readTable();
    void buildBlocks();
    void startNextRequest();
    void handleFinished();
    void updateStatusKo();

    bool rawWord(uint16_t reg, uint16_t& out) const;
    bool valueOfRow(uint8_t rowIndex, float& out) const;
    bool valueOfSlot(uint8_t slot, float& out) const;
    void sendValue(uint8_t slot, float value);
    void publishValues();
};
