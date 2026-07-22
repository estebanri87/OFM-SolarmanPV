// KNX-frei halten: hier darf kein OpenKNX-/knx-Header hinein.
#include "SolarmanV5Client.h"
#include <Arduino.h>
#include <WiFiClient.h>
#include <string.h>

namespace
{
constexpr uint8_t V5_START = 0xA5;
constexpr uint8_t V5_END = 0x15;
constexpr uint16_t V5_CTRL_REQUEST = 0x4510;
constexpr uint16_t V5_CTRL_RESPONSE = 0x1510;

constexpr size_t V5_HEADER_LEN = 11;  // Start + Laenge + Control + Sequenz + Logger-SN
constexpr size_t V5_REQ_PREFIX = 15;  // Frametyp(1) + Sensortyp(2) + 3x4 Byte Zeit
constexpr size_t V5_RESP_PREFIX = 14; // Frametyp(1) + Status(1)   + 3x4 Byte Zeit
constexpr size_t V5_TRAILER_LEN = 2;  // Pruefsumme + Ende
constexpr size_t RTU_MIN_LEN = 5;     // Addr + FC + ByteCount + CRC

constexpr uint32_t CONNECT_TIMEOUT_MS = 1000;
constexpr uint32_t READ_TIMEOUT_MS = 1000;
} // namespace

uint16_t SolarmanV5Client::crc16(const uint8_t* data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++)
            crc = (crc & 1) ? (uint16_t)((crc >> 1) ^ 0xA001) : (uint16_t)(crc >> 1);
    }
    return crc;
}

void SolarmanV5Client::configure(const char* host, uint16_t port, uint32_t loggerSerial, uint8_t slaveId)
{
    if (host != nullptr)
    {
        strncpy(_host, host, sizeof(_host) - 1);
        _host[sizeof(_host) - 1] = 0;
    }
    if (port != 0)
        _port = port;
    _serial = loggerSerial;
    _slaveId = (slaveId == 0) ? 1 : slaveId;
}

size_t SolarmanV5Client::buildRequest(uint32_t serial, uint8_t functionCode,
                                      uint16_t start, uint16_t count, uint8_t* out)
{
    // Modbus-RTU-Frame: Big Endian, CRC-16 als Little Endian angehaengt.
    uint8_t rtu[8];
    rtu[0] = _slaveId;
    rtu[1] = functionCode;
    rtu[2] = (uint8_t)(start >> 8);
    rtu[3] = (uint8_t)(start & 0xFF);
    rtu[4] = (uint8_t)(count >> 8);
    rtu[5] = (uint8_t)(count & 0xFF);
    const uint16_t crc = crc16(rtu, 6);
    rtu[6] = (uint8_t)(crc & 0xFF);
    rtu[7] = (uint8_t)(crc >> 8);

    const uint16_t payloadLen = (uint16_t)(V5_REQ_PREFIX + sizeof(rtu));

    size_t i = 0;
    out[i++] = V5_START;
    out[i++] = (uint8_t)(payloadLen & 0xFF); // Laenge LE
    out[i++] = (uint8_t)(payloadLen >> 8);
    out[i++] = (uint8_t)(V5_CTRL_REQUEST & 0xFF); // Control LE
    out[i++] = (uint8_t)(V5_CTRL_REQUEST >> 8);
    _sequence++;
    out[i++] = (uint8_t)(_sequence & 0xFF); // Sequenz LE
    out[i++] = (uint8_t)(_sequence >> 8);
    out[i++] = (uint8_t)(serial & 0xFF); // Logger-SN LE
    out[i++] = (uint8_t)((serial >> 8) & 0xFF);
    out[i++] = (uint8_t)((serial >> 16) & 0xFF);
    out[i++] = (uint8_t)((serial >> 24) & 0xFF);

    out[i++] = 0x02; // Frametyp
    out[i++] = 0x00; // Sensortyp
    out[i++] = 0x00;
    for (uint8_t z = 0; z < 12; z++) // Working / PowerOn / Offset Time, alle 0
        out[i++] = 0x00;

    memcpy(out + i, rtu, sizeof(rtu));
    i += sizeof(rtu);

    uint8_t sum = 0;
    for (size_t k = 1; k < i; k++)
        sum = (uint8_t)(sum + out[k]);
    out[i++] = sum;
    out[i++] = V5_END;
    return i;
}

SolarmanV5Client::Result SolarmanV5Client::transact(const uint8_t* request, size_t requestLen,
                                                    uint8_t* response, size_t responseCap,
                                                    size_t& responseLen)
{
    // Verbindung bewusst kurz halten: verbinden, lesen, trennen. Viele Logger werfen bei
    // Dauerverbindung nach Inaktivitaet raus.
    WiFiClient client;
    if (!client.connect(_host, _port, CONNECT_TIMEOUT_MS))
        return ErrConnect;

    if (client.write(request, requestLen) != requestLen)
    {
        client.stop();
        return ErrConnect;
    }

    responseLen = 0;
    const uint32_t deadline = millis() + READ_TIMEOUT_MS;
    while ((int32_t)(millis() - deadline) < 0 && responseLen < responseCap)
    {
        const int avail = client.available();
        if (avail <= 0)
        {
            if (!client.connected())
                break;
            delay(2);
            continue;
        }
        size_t room = responseCap - responseLen;
        size_t want = ((size_t)avail < room) ? (size_t)avail : room;
        int got = client.read(response + responseLen, want);
        if (got > 0)
            responseLen += (size_t)got;

        // Vollstaendig, sobald die im Header angekuendigte Laenge erreicht ist.
        if (responseLen >= 3)
        {
            const size_t expected = V5_HEADER_LEN + (size_t)(response[1] | (response[2] << 8)) + V5_TRAILER_LEN;
            if (responseLen >= expected)
                break;
        }
    }
    client.stop();

    return (responseLen == 0) ? ErrTimeout : Ok;
}

bool SolarmanV5Client::discoverSerial(uint32_t& serialOut)
{
    if (!configured())
        return false;

    uint8_t request[64];
    // Absichtlich Seriennummer 0: der Logger lehnt die Anfrage ab, verraet aber im
    // Antwort-Header seine echte Seriennummer.
    const size_t requestLen = buildRequest(0, 3, 0x0003, 1, request);

    uint8_t response[64];
    size_t responseLen = 0;
    if (transact(request, requestLen, response, sizeof(response), responseLen) != Ok)
        return false;
    if (responseLen < V5_HEADER_LEN || response[0] != V5_START)
        return false;

    serialOut = (uint32_t)response[7] | ((uint32_t)response[8] << 8) |
                ((uint32_t)response[9] << 16) | ((uint32_t)response[10] << 24);
    return serialOut != 0;
}

SolarmanV5Client::Result SolarmanV5Client::readRegisters(uint8_t functionCode, uint16_t start,
                                                         uint16_t count, uint16_t* out)
{
    if (count == 0 || count > MAX_REGISTERS)
        return ErrTooMany;
    if (!configured())
        return ErrNotConfigured;

    uint8_t request[64];
    const size_t requestLen = buildRequest(_serial, functionCode, start, count, request);

    uint8_t response[V5_HEADER_LEN + V5_RESP_PREFIX + RTU_MIN_LEN + 2 * MAX_REGISTERS + V5_TRAILER_LEN];
    size_t responseLen = 0;
    const Result transactResult = transact(request, requestLen, response, sizeof(response), responseLen);
    if (transactResult != Ok)
        return transactResult;

    // --- V5-Rahmen pruefen ---
    if (responseLen < V5_HEADER_LEN + V5_TRAILER_LEN || response[0] != V5_START ||
        response[responseLen - 1] != V5_END)
        return ErrFrame;

    const size_t payloadLen = (size_t)(response[1] | (response[2] << 8));
    if (responseLen != V5_HEADER_LEN + payloadLen + V5_TRAILER_LEN)
        return ErrFrame;

    const uint16_t control = (uint16_t)(response[3] | (response[4] << 8));
    if (control != V5_CTRL_RESPONSE)
        return ErrFrame;

    uint8_t sum = 0;
    for (size_t k = 1; k < responseLen - 2; k++)
        sum = (uint8_t)(sum + response[k]);
    if (sum != response[responseLen - 2])
        return ErrChecksum;

    // --- Modbus-RTU-Frame auswerten (Response-Prefix ist 14, nicht 15) ---
    if (payloadLen < V5_RESP_PREFIX + RTU_MIN_LEN)
        return ErrRejected; // z.B. Fehlerrahmen bei falscher Logger-Seriennummer

    const uint8_t* rtu = response + V5_HEADER_LEN + V5_RESP_PREFIX;
    const size_t rtuLen = payloadLen - V5_RESP_PREFIX;

    if (rtu[1] & 0x80)
        return ErrModbus; // Exception-Response

    const uint8_t byteCount = rtu[2];
    if (byteCount != (uint8_t)(count * 2) || rtuLen < (size_t)(3 + byteCount + 2))
        return ErrRejected;

    const uint16_t crcCalc = crc16(rtu, (size_t)(3 + byteCount));
    const uint16_t crcRecv = (uint16_t)(rtu[3 + byteCount] | (rtu[4 + byteCount] << 8));
    if (crcCalc != crcRecv)
        return ErrCrc;

    // Registerwerte sind Big Endian.
    for (uint16_t i = 0; i < count; i++)
        out[i] = (uint16_t)((rtu[3 + 2 * i] << 8) | rtu[4 + 2 * i]);

    return Ok;
}

const char* SolarmanV5Client::resultText(Result result)
{
    switch (result)
    {
        case Ok: return "OK";
        case ErrNotConfigured: return "nicht konfiguriert";
        case ErrConnect: return "Verbindung fehlgeschlagen";
        case ErrTimeout: return "Zeitueberschreitung";
        case ErrFrame: return "ungueltiger V5-Rahmen";
        case ErrChecksum: return "V5-Pruefsumme falsch";
        case ErrRejected: return "abgewiesen (Logger-Seriennummer?)";
        case ErrCrc: return "Modbus-CRC falsch";
        case ErrModbus: return "Modbus-Exception";
        case ErrTooMany: return "ungueltige Registeranzahl";
    }
    return "unbekannt";
}
