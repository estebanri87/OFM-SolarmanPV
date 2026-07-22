// KNX-frei halten: hier darf kein OpenKNX-/knx-Header hinein.
#include "SolarmanV5Client.h"
#include <Arduino.h>
#include <errno.h>
#include <fcntl.h>
#include <lwip/sockets.h>
#include <string.h>
#include <unistd.h> // close()

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

// Gesamtbudget je Transaktion. Wird nur ueber millis() geprueft, nie gewartet.
constexpr uint32_t TRANSACTION_TIMEOUT_MS = 4000;
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

void SolarmanV5Client::configure(const char* host, uint16_t port, uint32_t loggerSerial,
                                 uint8_t slaveId, Transport transport)
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
    _transport = transport;
}

size_t SolarmanV5Client::buildModbusTcp(uint8_t functionCode, uint16_t start, uint16_t count, uint8_t* out)
{
    // MBAP-Header: Transaction-ID, Protokoll 0, Laenge, Unit-ID. Danach die reine PDU -
    // ohne CRC, das uebernimmt hier TCP.
    _sequence++;
    size_t i = 0;
    out[i++] = (uint8_t)(_sequence >> 8); // MBAP ist Big Endian
    out[i++] = (uint8_t)(_sequence & 0xFF);
    out[i++] = 0x00; // Protokoll-ID
    out[i++] = 0x00;
    out[i++] = 0x00; // Laenge = Unit + PDU = 6
    out[i++] = 0x06;
    out[i++] = _slaveId;
    out[i++] = functionCode;
    out[i++] = (uint8_t)(start >> 8);
    out[i++] = (uint8_t)(start & 0xFF);
    out[i++] = (uint8_t)(count >> 8);
    out[i++] = (uint8_t)(count & 0xFF);
    return i;
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

bool SolarmanV5Client::openSocket()
{
    _sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_sock < 0)
        return false;

    // Non-Blocking: connect() kehrt sofort mit EINPROGRESS zurueck.
    const int flags = fcntl(_sock, F_GETFL, 0);
    if (flags < 0 || fcntl(_sock, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        closeSocket();
        return false;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(_port);
    if (inet_pton(AF_INET, _host, &addr.sin_addr) != 1)
    {
        closeSocket();
        return false; // nur IPv4-Literale, Namensaufloesung wuerde blockieren
    }

    const int rc = connect(_sock, (struct sockaddr*)&addr, sizeof(addr));
    if (rc == 0 || errno == EINPROGRESS || errno == EALREADY)
        return true;

    closeSocket();
    return false;
}

void SolarmanV5Client::closeSocket()
{
    if (_sock >= 0)
    {
        close(_sock);
        _sock = -1;
    }
}

void SolarmanV5Client::fail(Result reason)
{
    closeSocket();
    _result = reason;
    _state = Complete;
}

bool SolarmanV5Client::beginTransaction(uint32_t serial, uint8_t functionCode, uint16_t startReg,
                                        uint16_t count, bool discoverOnly)
{
    if (busy())
        return false;
    if (!configured())
    {
        _result = ErrNotConfigured;
        _state = Complete;
        return false;
    }

    _discoverOnly = discoverOnly;
    _expectCount = count;
    _expectFc = functionCode;
    _regCount = 0;
    _received = 0;
    _sent = 0;
    _result = Pending;

    _requestLen = (_transport == ModbusTcp)
                      ? buildModbusTcp(functionCode, startReg, count, _request)
                      : buildRequest(serial, functionCode, startReg, count, _request);

    if (!openSocket())
    {
        _result = ErrSocket;
        _state = Complete;
        return false;
    }

    _deadline = millis() + TRANSACTION_TIMEOUT_MS;
    _state = Connecting;
    return true;
}

bool SolarmanV5Client::beginRead(uint8_t functionCode, uint16_t startReg, uint16_t count)
{
    if (count == 0 || count > MAX_REGISTERS)
    {
        _result = ErrTooMany;
        _state = Complete;
        return false;
    }
    return beginTransaction(_serial, functionCode, startReg, count, false);
}

bool SolarmanV5Client::beginDiscoverSerial()
{
    // Nur bei V5 sinnvoll: der MBAP-Rahmen kennt keine Seriennummer.
    if (_transport == ModbusTcp)
    {
        _result = ErrNotConfigured;
        _state = Complete;
        return false;
    }
    // Absichtlich Seriennummer 0 - der Logger lehnt ab, verraet die echte SN aber im Header.
    // ACHTUNG: geraeteabhaengig. Der Pylontech verwirft solche Frames stillschweigend, dort
    // muss die Seriennummer manuell hinterlegt werden.
    return beginTransaction(0, 3, 0x0003, 1, true);
}

void SolarmanV5Client::poll()
{
    if (_state == Idle || _state == Complete)
        return;

    if ((int32_t)(millis() - _deadline) >= 0)
    {
        fail(ErrTimeout);
        return;
    }

    switch (_state)
    {
        case Connecting:
        {
            fd_set wfds;
            FD_ZERO(&wfds);
            FD_SET(_sock, &wfds);
            struct timeval tv = {0, 0}; // nie warten
            const int rc = select(_sock + 1, nullptr, &wfds, nullptr, &tv);
            if (rc < 0)
            {
                fail(ErrSocket);
                return;
            }
            if (rc == 0 || !FD_ISSET(_sock, &wfds))
                return; // noch nicht verbunden, naechster loop()

            int soErr = 0;
            socklen_t len = sizeof(soErr);
            if (getsockopt(_sock, SOL_SOCKET, SO_ERROR, &soErr, &len) < 0 || soErr != 0)
            {
                fail(ErrConnect);
                return;
            }
            _state = Sending;
            return;
        }

        case Sending:
        {
            const int n = send(_sock, _request + _sent, _requestLen - _sent, 0);
            if (n > 0)
                _sent += (size_t)n;
            else if (n < 0 && errno != EWOULDBLOCK && errno != EAGAIN)
            {
                fail(ErrSocket);
                return;
            }
            if (_sent >= _requestLen)
                _state = Receiving;
            return;
        }

        case Receiving:
        {
            const int n = recv(_sock, _response + _received, sizeof(_response) - _received, 0);
            if (n > 0)
            {
                _received += (size_t)n;
            }
            else if (n == 0)
            {
                // Gegenstelle hat geschlossen - auswerten, was da ist.
                parseResponse();
                return;
            }
            else if (errno != EWOULDBLOCK && errno != EAGAIN)
            {
                fail(ErrSocket);
                return;
            }

            // Vollstaendig, sobald die im Header angekuendigte Laenge erreicht ist.
            const size_t expected = expectedLength();
            if (expected != 0 && _received >= expected)
                parseResponse();
            return;
        }

        default:
            return;
    }
}

size_t SolarmanV5Client::expectedLength() const
{
    if (_transport == ModbusTcp)
    {
        // MBAP: Laengenfeld (Byte 4-5, Big Endian) zaehlt ab Unit-ID.
        if (_received < 6)
            return 0;
        return 6 + (size_t)((_response[4] << 8) | _response[5]);
    }
    if (_received < 3)
        return 0;
    return V5_HEADER_LEN + (size_t)(_response[1] | (_response[2] << 8)) + V5_TRAILER_LEN;
}

void SolarmanV5Client::parseResponse()
{
    closeSocket();
    _state = Complete;

    if (_transport == ModbusTcp)
        parseModbusTcp();
    else
        parseV5();
}

void SolarmanV5Client::parseModbusTcp()
{
    // MBAP: TID(2) Proto(2) Len(2) Unit(1) | FC(1) ByteCount(1) Daten... - kein CRC.
    if (_received < 9)
    {
        _result = ErrFrame;
        return;
    }
    const uint16_t proto = (uint16_t)((_response[2] << 8) | _response[3]);
    if (proto != 0)
    {
        _result = ErrFrame;
        return;
    }

    const uint8_t fc = _response[7];
    if (fc & 0x80)
    {
        _result = ErrModbus; // Exception-Response
        return;
    }

    const uint8_t byteCount = _response[8];
    if (byteCount != (uint8_t)(_expectCount * 2) || _received < (size_t)(9 + byteCount))
    {
        // Der Pylontech liefert gelegentlich mehr Register als angefragt bzw. Frames, die
        // nicht zur Anfrage gehoeren. Solche Antworten sind nicht zuzuordnen -> verwerfen.
        _result = ErrRejected;
        return;
    }

    for (uint16_t i = 0; i < _expectCount; i++)
        _regs[i] = (uint16_t)((_response[9 + 2 * i] << 8) | _response[10 + 2 * i]);
    _regCount = _expectCount;
    _result = Ok;
}

void SolarmanV5Client::parseV5()
{
    // --- V5-Rahmen pruefen ---
    if (_received < V5_HEADER_LEN + V5_TRAILER_LEN || _response[0] != V5_START)
    {
        _result = ErrFrame;
        return;
    }

    const size_t payloadLen = (size_t)(_response[1] | (_response[2] << 8));
    if (_received != V5_HEADER_LEN + payloadLen + V5_TRAILER_LEN ||
        _response[_received - 1] != V5_END)
    {
        _result = ErrFrame;
        return;
    }

    const uint16_t control = (uint16_t)(_response[3] | (_response[4] << 8));
    if (control != V5_CTRL_RESPONSE)
    {
        _result = ErrFrame;
        return;
    }

    uint8_t sum = 0;
    for (size_t k = 1; k < _received - 2; k++)
        sum = (uint8_t)(sum + _response[k]);
    if (sum != _response[_received - 2])
    {
        _result = ErrChecksum;
        return;
    }

    // Die Logger-Seriennummer steht in JEDER Antwort - auch im Fehlerrahmen.
    _serial = (uint32_t)_response[7] | ((uint32_t)_response[8] << 8) |
              ((uint32_t)_response[9] << 16) | ((uint32_t)_response[10] << 24);

    if (_discoverOnly)
    {
        _result = (_serial != 0) ? Ok : ErrRejected;
        return;
    }

    // --- Modbus-RTU-Frame auswerten (Response-Prefix ist 14, nicht 15) ---
    if (payloadLen < V5_RESP_PREFIX + RTU_MIN_LEN)
    {
        _result = ErrRejected; // z.B. Fehlerrahmen bei falscher Logger-Seriennummer
        return;
    }

    const uint8_t* rtu = _response + V5_HEADER_LEN + V5_RESP_PREFIX;
    const size_t rtuLen = payloadLen - V5_RESP_PREFIX;

    if (rtu[1] & 0x80)
    {
        _result = ErrModbus; // Exception-Response
        return;
    }

    const uint8_t byteCount = rtu[2];
    if (byteCount != (uint8_t)(_expectCount * 2) || rtuLen < (size_t)(3 + byteCount + 2))
    {
        _result = ErrRejected;
        return;
    }

    const uint16_t crcCalc = crc16(rtu, (size_t)(3 + byteCount));
    const uint16_t crcRecv = (uint16_t)(rtu[3 + byteCount] | (rtu[4 + byteCount] << 8));
    if (crcCalc != crcRecv)
    {
        _result = ErrCrc;
        return;
    }

    // Registerwerte sind Big Endian.
    for (uint16_t i = 0; i < _expectCount; i++)
        _regs[i] = (uint16_t)((rtu[3 + 2 * i] << 8) | rtu[4 + 2 * i]);
    _regCount = _expectCount;
    _result = Ok;
}

void SolarmanV5Client::clear()
{
    closeSocket();
    _state = Idle;
    _received = 0;
    _sent = 0;
}

const char* SolarmanV5Client::resultText(Result result)
{
    switch (result)
    {
        case Ok: return "OK";
        case Pending: return "laeuft";
        case ErrNotConfigured: return "nicht konfiguriert";
        case ErrBusy: return "bereits aktiv";
        case ErrSocket: return "Socket-Fehler";
        case ErrConnect: return "Verbindung abgelehnt";
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
