#pragma once
#include "SolarmanV5Client.h"
#include <stdint.h>

// Einmess-Hilfe fuer die ETS: erkennt den Transport eines unbekannten Geraets und liest
// Registerbloecke aus. Arbeitet mit einem EIGENEN Client, unabhaengig von den Kanaelen -
// die gemessene Adresse ist eine gerade eingetippte, noch nicht heruntergeladene.
//
// Aufruf ueber processFunctionProperty (objectIndex 0xA1). invokeFunctionProperty ist in der
// ETS synchron, eine Messung dauert aber Sekunden: deshalb starten und pollen, nach dem
// Muster des OFM-PresenceModule.
class SolarmanAssistant
{
  public:
    enum State : uint8_t
    {
        Idle = 0,
        ProbeV5,      // Solarman-V5-Rahmen versuchen
        ProbeTcp,     // reines Modbus TCP versuchen
        Reading,      // Registerblock lesen
        Done,
    };

    enum Error : uint8_t
    {
        ErrNone = 0,
        ErrNoAnswer,      // beide Transporte ohne verwertbare Antwort
        ErrNotStarted,    // Ergebnis abgefragt, ohne dass etwas lief
        ErrNoNetwork,
    };

    // Anzahl Register, die ein Durchgang liefert. Die Antwort auf ein Abholkommando ist mit
    // 3 + 2*64 = 131 Byte groesser als jede APDU; die ETS holt sie deshalb ueber
    // BASE_invokeFunctionPropertyWrapper ab, der stueckelt. Der Puffer in OGM-Common fasst
    // 256 Byte, ein Durchgang genuegt also.
    static const uint8_t MAX_REGISTERS = 64;
    static const uint8_t CHUNK_REGISTERS = MAX_REGISTERS;

    void loop();

    // Transport ermitteln. Adresse kommt aus dem Aufruf, nicht aus den Parametern.
    void startProbe(uint32_t ip, uint16_t port, uint8_t slaveId, uint16_t probeReg);
    // Registerblock lesen. Der Transport muss bekannt sein (oder wird mitgegeben).
    void startRead(uint32_t ip, uint16_t port, uint8_t slaveId, uint8_t transport,
                   uint16_t start, uint8_t count);

    bool running() const { return _state != Idle && _state != Done; }
    bool finished() const { return _state == Done; }

    uint8_t transport() const { return _transport; }
    uint32_t serial() const { return _serial; }
    uint8_t error() const { return _error; }
    uint16_t readStart() const { return _readStart; }
    uint8_t registerCount() const { return _regCount; }
    const uint16_t* registers() const { return _regs; }

  private:
    SolarmanV5Client _client;
    State _state = Idle;
    uint8_t _transport = SolarmanV5Client::V5;
    uint32_t _serial = 0;
    uint8_t _error = ErrNotStarted;

    char _host[16] = {0};
    uint16_t _port = 8899;
    uint8_t _slaveId = 1;
    uint16_t _probeReg = 0;

    uint16_t _readStart = 0;
    uint8_t _readCount = 0;
    uint16_t _regs[MAX_REGISTERS] = {};
    uint8_t _regCount = 0;

    void setHost(uint32_t ip);
    void beginTcpProbe();
};
