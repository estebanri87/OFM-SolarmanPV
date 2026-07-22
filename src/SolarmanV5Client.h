#pragma once
#include <stddef.h>
#include <stdint.h>

// Solarman-V5-Client: TCP + V5-Framing + Modbus RTU.
//
// Diese Uebersetzungseinheit ist bewusst KNX-FREI (keine OpenKNX-/knx-Header) und bindet
// WiFi.h nicht im Header ein - bewaehrtes Muster aus OFM-EnergyEdgeModule/EnergyEdgeServer.cpp
// bzw. OFM-Network MQTT::Client.
//
// Protokoll am realen Geraet verifiziert (Deye SUN-M80G3, 192.168.30.201:8899, 2026-07-22):
//
//   Offset 0      0xA5 (Start)
//   Offset 1..2   Payload-Laenge, Little Endian
//   Offset 3..4   Control: 0x4510 Request / 0x1510 Response, Little Endian
//   Offset 5..6   Sequenznummer (frei waehlbar)
//   Offset 7..10  Logger-Seriennummer, 4 Byte Little Endian
//   Offset 11..   Payload
//   Ende -2       Pruefsumme = sum(frame[1 .. n-3]) & 0xFF
//   Ende -1       0x15
//
// ACHTUNG, Payload ist asymmetrisch:
//   Request : Frametyp(1)=0x02 + Sensortyp(2) + 3x4 Byte Zeit = 15 Byte, dann RTU-Frame
//   Response: Frametyp(1)      + Status(1)    + 3x4 Byte Zeit = 14 Byte, dann RTU-Frame
// Wer beide gleich behandelt, liest die Antwort um ein Byte verschoben. Genau dieser Fehler
// ist beim Prototyping passiert.
//
// Referenz-Testvektor (SN 0xE93A5998, Sequenz 1, Slave 1, FC 3, Start 0x003B, Count 16):
//   a517001045010098593ae90200000000000000000000000000000103003b001035cbd215

class SolarmanV5Client
{
  public:
    enum Result : uint8_t
    {
        Ok = 0,
        ErrNotConfigured,
        ErrConnect,
        ErrTimeout,
        ErrFrame,      // kein gueltiger V5-Rahmen (Start/Ende/Laenge)
        ErrChecksum,   // V5-Pruefsumme falsch
        ErrRejected,   // V5-Antwort ohne Modbus-Nutzlast (z.B. falsche Logger-SN)
        ErrCrc,        // Modbus-CRC falsch
        ErrModbus,     // Modbus-Exception vom Geraet
        ErrTooMany,    // count == 0 oder > MAX_REGISTERS
    };

    static const uint16_t MAX_REGISTERS = 125;

    void configure(const char* host, uint16_t port, uint32_t loggerSerial, uint8_t slaveId);

    // Ermittelt die Logger-Seriennummer. Der Logger validiert sie zwar (falsche SN liefert
    // einen Fehlerrahmen ohne Daten), sendet sie aber in JEDER Antwort im Header mit - auch
    // im Fehlerrahmen. Also: Dummy-Frame mit SN 0 senden und die SN aus der Antwort lesen.
    // Eine UDP-Discovery auf Port 48899 gibt es nicht (am Geraet geprueft: Port geschlossen).
    bool discoverSerial(uint32_t& serialOut);

    // Liest count Register ab start. functionCode 3 (Holding) oder 4 (Input).
    Result readRegisters(uint8_t functionCode, uint16_t start, uint16_t count, uint16_t* out);

    uint32_t loggerSerial() const { return _serial; }
    void setLoggerSerial(uint32_t serial) { _serial = serial; }
    bool configured() const { return _host[0] != 0; }

    static const char* resultText(Result result);

  private:
    char _host[40] = {0};
    uint16_t _port = 8899;
    uint32_t _serial = 0;
    uint8_t _slaveId = 1;
    uint16_t _sequence = 0;

    size_t buildRequest(uint32_t serial, uint8_t functionCode, uint16_t start, uint16_t count, uint8_t* out);

    // Baut den Rahmen, sendet ihn und liest die Antwort.
    // HINWEIS: Diese Fassung ist SYNCHRON und blockiert bis Timeout. Fuer den produktiven
    // Einsatz im OpenKNX-Loop muss sie in eine Zustandsmaschine ueberfuehrt werden, sonst
    // reisst der KNX-Stack sein Timing. Timeouts sind deshalb bewusst kurz gehalten.
    Result transact(const uint8_t* request, size_t requestLen,
                    uint8_t* response, size_t responseCap, size_t& responseLen);

    static uint16_t crc16(const uint8_t* data, size_t len);
};
