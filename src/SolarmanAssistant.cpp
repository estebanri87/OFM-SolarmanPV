#include "SolarmanAssistant.h"
#include "NetworkModule.h"
#include <stdio.h>

void SolarmanAssistant::setHost(uint32_t ip)
{
    snprintf(_host, sizeof(_host), "%u.%u.%u.%u", (unsigned)((ip >> 24) & 0xFF),
             (unsigned)((ip >> 16) & 0xFF), (unsigned)((ip >> 8) & 0xFF), (unsigned)(ip & 0xFF));
}

void SolarmanAssistant::startProbe(uint32_t ip, uint16_t port, uint8_t slaveId, uint16_t probeReg)
{
    setHost(ip);
    _port = port;
    _slaveId = slaveId;
    _probeReg = probeReg;
    _serial = 0;
    _error = ErrNone;
    _regCount = 0;

    if (!openknxNetwork.established())
    {
        _error = ErrNoNetwork;
        _state = Done;
        return;
    }

    // Solarman V5 zuerst: der Logger sendet seine Seriennummer im Kopf JEDER Antwort mit,
    // auch im Fehlerrahmen. Antwortet er ueberhaupt im V5-Format, ist der Transport belegt.
    _client.configure(_host, _port, 0, _slaveId, SolarmanV5Client::V5);
    _client.beginDiscoverSerial();
    _state = ProbeV5;
}

void SolarmanAssistant::beginTcpProbe()
{
    _client.clear();
    _client.configure(_host, _port, 0, _slaveId, SolarmanV5Client::ModbusTcp);
    _client.beginRead(3, _probeReg, 1);
    _state = ProbeTcp;
}

void SolarmanAssistant::startRead(uint32_t ip, uint16_t port, uint8_t slaveId, uint8_t transport,
                                  uint16_t start, uint8_t count)
{
    setHost(ip);
    _port = port;
    _slaveId = slaveId;
    _transport = transport;
    _readStart = start;
    _readCount = (count > MAX_REGISTERS) ? MAX_REGISTERS : count;
    _regCount = 0;
    _error = ErrNone;

    if (!openknxNetwork.established())
    {
        _error = ErrNoNetwork;
        _state = Done;
        return;
    }

    _client.configure(_host, _port, _serial, _slaveId,
                      (transport == SolarmanV5Client::ModbusTcp) ? SolarmanV5Client::ModbusTcp
                                                                 : SolarmanV5Client::V5);
    _client.beginRead(3, _readStart, _readCount);
    _state = Reading;
}

void SolarmanAssistant::loop()
{
    if (_state == Idle || _state == Done)
        return;

    _client.poll();
    if (!_client.finished())
        return;

    const SolarmanV5Client::Result result = _client.result();

    switch (_state)
    {
        case ProbeV5:
            // Auch ErrRejected zaehlt: das ist ein gueltiger V5-Rahmen, nur ohne Nutzlast
            // (der Logger pruegt die Seriennummer). Die Seriennummer steht trotzdem drin -
            // und genau die wollen wir.
            if ((result == SolarmanV5Client::Ok || result == SolarmanV5Client::ErrRejected) &&
                _client.loggerSerial() != 0)
            {
                _transport = SolarmanV5Client::V5;
                _serial = _client.loggerSerial();
                _error = ErrNone;
                _client.clear();
                _state = Done;
            }
            else
            {
                beginTcpProbe();
            }
            break;

        case ProbeTcp:
            // ErrModbus heisst: das Geraet hat den MBAP-Rahmen verstanden und eine
            // Modbus-Exception geschickt - etwa weil das Probe-Register nicht existiert.
            // Fuer die Transporterkennung ist das ein Erfolg.
            if (result == SolarmanV5Client::Ok || result == SolarmanV5Client::ErrModbus)
            {
                _transport = SolarmanV5Client::ModbusTcp;
                _serial = 0; // Modbus TCP kennt keine Logger-Seriennummer
                _error = ErrNone;
            }
            else
            {
                _error = ErrNoAnswer;
            }
            _client.clear();
            _state = Done;
            break;

        case Reading:
            if (result == SolarmanV5Client::Ok)
            {
                _regCount = (uint8_t)((_client.registerCount() > MAX_REGISTERS)
                                          ? MAX_REGISTERS
                                          : _client.registerCount());
                for (uint8_t i = 0; i < _regCount; i++)
                    _regs[i] = _client.registers()[i];
                _error = ErrNone;
            }
            else
            {
                _regCount = 0;
                _error = ErrNoAnswer;
            }
            _client.clear();
            _state = Done;
            break;

        default:
            _client.clear();
            _state = Done;
            break;
    }
}
