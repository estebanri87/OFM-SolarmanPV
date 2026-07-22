# Changelog OFM-SolarmanPV

## Unveröffentlicht

### Added
- `SolarmanV5Client`: TCP-Client mit Solarman-V5-Framing, Modbus RTU (FC 3/4) und CRC-16.
  Bewusst KNX-frei gehalten, damit die Übersetzungseinheit unabhängig testbar bleibt.
- `discoverSerial()`: ermittelt die Logger-Seriennummer automatisch aus dem Antwort-Header.

### Notes
- Protokoll am realen Gerät verifiziert (Deye SUN-M80G3 mit integriertem Logger). Der
  Frame-Aufbau wurde gegen einen Referenz-Testvektor byteweise gegengeprüft:
  `a517001045010098593ae90200000000000000000000000000000103003b001035cbd215`
  (SN `0xE93A5998`, Sequenz 1, Slave 1, FC 3, Start `0x003B`, Count 16).
- Request- und Response-Payload sind **asymmetrisch** (15 bzw. 14 Byte vor dem RTU-Frame).
- UDP-Discovery auf Port 48899 existiert bei diesem Logger nicht; die Seriennummer wird
  stattdessen aus dem Antwort-Header gelesen.
- Der Client ist als **nicht-blockierende Zustandsmaschine** auf rohen lwIP-Sockets
  (`O_NONBLOCK`, `EINPROGRESS` + `select()` mit Timeout 0) umgesetzt. `poll()` kehrt immer
  sofort zurück, damit der KNX-Stack sein Timing behält — ohne zusätzliche Abhängigkeit.
- Nur IPv4-Literale, keine Hostnamen (Namensauflösung würde blockieren).
- Noch **nicht kompiliert**: Das Modul ist noch nicht in `lib/` eingehängt, es gibt bisher
  keinen OpenKNX-Modulrumpf, der die Übersetzungseinheit in den Build zieht.
