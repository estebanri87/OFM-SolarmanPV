#pragma once
#include <stddef.h>
#include <stdint.h>

// Solarman-V5-Client: TCP + V5-Framing + Modbus RTU, als NICHT-BLOCKIERENDE Zustandsmaschine.
//
// Diese Uebersetzungseinheit ist bewusst KNX-FREI (keine OpenKNX-/knx-Header) - bewaehrtes
// Muster aus OFM-EnergyEdgeModule/EnergyEdgeServer.cpp.
//
// Verwendung:
//     client.configure("192.168.30.201", 8899, serial, 1);
//     if (client.beginRead(3, 0x003B, 16)) { ... }
//     // in jedem loop():
//     client.poll();
//     if (client.finished()) { auto r = client.result(); ... client.clear(); }
//
// poll() kehrt immer sofort zurueck. Es wird an keiner Stelle gewartet: der Socket ist auf
// O_NONBLOCK gesetzt, connect() laeuft ueber EINPROGRESS + select() mit Timeout 0. Damit
// behaelt der KNX-Stack sein Timing.
//
// Protokoll am realen Geraet verifiziert (Deye SUN-M80G3, 192.168.30.201:8899, 2026-07-22):
//
//   Offset 0      0xA5 (Start)
//   Offset 1..2   Payload-Laenge, Little Endian
//   Offset 3..4   Control: 0x4510 Request / 0x1510 Response, Little Endian
//   Offset 5..6   Sequenznummer
//   Offset 7..10  Logger-Seriennummer, 4 Byte Little Endian
//   Offset 11..   Payload
//   Ende -2       Pruefsumme = sum(frame[1 .. n-3]) & 0xFF
//   Ende -1       0x15
//
// ACHTUNG, Payload ist asymmetrisch:
//   Request : Frametyp(1)=0x02 + Sensortyp(2) + 3x4 Byte Zeit = 15 Byte, dann RTU-Frame
//   Response: Frametyp(1)      + Status(1)    + 3x4 Byte Zeit = 14 Byte, dann RTU-Frame
// Wer beide gleich behandelt, liest die Antwort um ein Byte verschoben.
//
// Referenz-Testvektor (SN 0xE93A5998, Sequenz 1, Slave 1, FC 3, Start 0x003B, Count 16):
//   a517001045010098593ae90200000000000000000000000000000103003b001035cbd215

class SolarmanV5Client
{
  public:
    enum Result : uint8_t
    {
        Ok = 0,
        Pending,          // laeuft noch
        ErrNotConfigured,
        ErrBusy,
        ErrSocket,        // Socket konnte nicht angelegt/verbunden werden
        ErrConnect,       // Verbindungsaufbau abgelehnt
        ErrTimeout,
        ErrFrame,         // kein gueltiger V5-Rahmen (Start/Ende/Laenge/Control)
        ErrChecksum,      // V5-Pruefsumme falsch
        ErrRejected,      // V5-Antwort ohne Modbus-Nutzlast (z.B. falsche Logger-SN)
        ErrCrc,           // Modbus-CRC falsch
        ErrModbus,        // Modbus-Exception vom Geraet
        ErrTooMany,       // count == 0 oder > MAX_REGISTERS
    };

    static const uint16_t MAX_REGISTERS = 125;

    // Nur IPv4-Adressen, keine Hostnamen: Namensaufloesung wuerde blockieren.
    void configure(const char* host, uint16_t port, uint32_t loggerSerial, uint8_t slaveId);

    // Startet eine Leseanfrage (functionCode 3 = Holding, 4 = Input).
    bool beginRead(uint8_t functionCode, uint16_t startReg, uint16_t count);

    // Startet die Ermittlung der Logger-Seriennummer. Der Logger validiert die SN zwar
    // (falsche SN -> Fehlerrahmen ohne Daten), sendet sie aber in JEDER Antwort im Header
    // mit. Also: Dummy-Frame mit SN 0 senden und die SN aus der Antwort lesen. Eine
    // UDP-Discovery auf Port 48899 gibt es nicht (am Geraet geprueft: Port geschlossen).
    bool beginDiscoverSerial();

    // In jedem loop() aufrufen. Kehrt immer sofort zurueck.
    void poll();

    bool busy() const { return _state != Idle && _state != Complete; }
    bool finished() const { return _state == Complete; }
    Result result() const { return _result; }

    // Nach finished(): Ergebnis abholen und die Maschine wieder freigeben.
    void clear();

    const uint16_t* registers() const { return _regs; }
    uint16_t registerCount() const { return _regCount; }

    uint32_t loggerSerial() const { return _serial; }
    void setLoggerSerial(uint32_t serial) { _serial = serial; }
    bool configured() const { return _host[0] != 0; }

    static const char* resultText(Result result);

  private:
    enum State : uint8_t
    {
        Idle = 0,
        Connecting,
        Sending,
        Receiving,
        Complete,
    };

    static const size_t REQUEST_CAP = 64;
    static const size_t RESPONSE_CAP = 11 + 14 + 5 + 2 * MAX_REGISTERS + 2;

    char _host[40] = {0};
    uint16_t _port = 8899;
    uint32_t _serial = 0;
    uint8_t _slaveId = 1;
    uint16_t _sequence = 0;

    State _state = Idle;
    Result _result = Ok;
    int _sock = -1;
    uint32_t _deadline = 0;

    uint8_t _request[REQUEST_CAP];
    size_t _requestLen = 0;
    size_t _sent = 0;

    uint8_t _response[RESPONSE_CAP];
    size_t _received = 0;

    bool _discoverOnly = false;
    uint16_t _expectCount = 0;
    uint8_t _expectFc = 3;
    uint16_t _regs[MAX_REGISTERS];
    uint16_t _regCount = 0;

    bool beginTransaction(uint32_t serial, uint8_t functionCode, uint16_t startReg, uint16_t count, bool discoverOnly);
    bool openSocket();
    void closeSocket();
    void fail(Result reason);
    void parseResponse();

    size_t buildRequest(uint32_t serial, uint8_t functionCode, uint16_t start, uint16_t count, uint8_t* out);
    static uint16_t crc16(const uint8_t* data, size_t len);
};
