# OFM-SolarmanPV

OpenKNX-Modul zur **lokalen** Anbindung von Wechselrichtern und Batteriespeichern mit
Solarman-Logger an den KNX-Bus. Kommunikation über TCP zum Logger (Port 8899, Modbus RTU im
Solarman-V5-Rahmen) — **keine Cloud**, keine Rate-Limits, Sekunden-Aktualität.

> **Status: in Entwicklung.** Aktuell ist der Protokoll-Client implementiert und am realen
> Gerät verifiziert. Die ETS-Applikation und die Geräteprofile folgen.

## Stand

| Baustein | Status |
|---|---|
| Solarman-V5-Framing, Modbus RTU, CRC-16 | ✅ implementiert, am Gerät verifiziert |
| Logger-Seriennummer automatisch ermitteln | ✅ implementiert |
| Nicht-blockierende Zustandsmaschine | ⬜ offen (aktuell synchron) |
| ETS-Applikation (Geräte + Messwertkanäle) | ⬜ offen |
| Geräteprofile (Registertabellen) | ⬜ offen |

## Protokoll

Am realen Gerät verifiziert (Deye SUN-M80G3, integrierter Logger, 2026-07-22):

```
Offset 0      0xA5 (Start)
Offset 1..2   Payload-Länge, Little Endian
Offset 3..4   Control: 0x4510 Request / 0x1510 Response, Little Endian
Offset 5..6   Sequenznummer
Offset 7..10  Logger-Seriennummer, 4 Byte Little Endian
Offset 11..   Payload
Ende -2       Prüfsumme = sum(frame[1 .. n-3]) & 0xFF
Ende -1       0x15
```

**Der Payload ist asymmetrisch** — die häufigste Fehlerquelle:

- **Request:** `Frametyp(1)=0x02 + Sensortyp(2) + 3×4 Byte Zeit` = **15 Byte**, dann RTU-Frame
- **Response:** `Frametyp(1) + Status(1) + 3×4 Byte Zeit` = **14 Byte**, dann RTU-Frame

Die V5-Felder sind Little Endian, der eingebettete Modbus-Frame Big Endian.

### Logger-Seriennummer

Der Logger **validiert** die Seriennummer: Mit falscher SN antwortet er mit einem Fehlerrahmen
ohne Nutzdaten. Er sendet sie aber in **jeder** Antwort im Header mit — auch im Fehlerrahmen.
Daraus folgt die Autoerkennung in `discoverSerial()`: Dummy-Frame mit SN `0` senden, SN aus den
Antwort-Bytes 7…10 lesen.

Eine UDP-Discovery auf Port 48899 gibt es **nicht** (am Gerät geprüft: Port geschlossen).

## Bekannte Einschränkung

`SolarmanV5Client::transact()` ist derzeit **synchron** und blockiert bis zum Timeout
(1 s verbinden, 1 s lesen). Für den produktiven Einsatz im OpenKNX-Loop muss das in eine
Zustandsmaschine überführt werden, sonst reißt der KNX-Stack sein Timing.

## Hardware-Unterstützung

|Prozessor | Status | Anmerkung |
|----------|--------|-----------|
|ESP32     | in Entwicklung | `WiFiClient` |
|RP2040    | –      | nicht vorgesehen |

## Lizenz

[GNU GPL v3](LICENSE)
