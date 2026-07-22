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
- `SolarmanPVModule` (Phase 1): Modulrumpf mit ETS-Grundkonfiguration (IP, Port, Slave-ID,
  Abfrageintervall, optionale Seriennummer), Status-KO „Wechselrichter erreichbar" und den
  Diagnosebefehlen `spv` und `spvread`.
- In OAM-NetworkService eingebunden: `ModuleType 28`, `KoSingleOffset 809`,
  `openknx.addModule(11, …)`. Kompiliert für `release_REG1_LAN_TP_BASE`.

### Changed (Kanalmodell)
- Umbau auf ein **Kanalmodell**: ein Kanal = ein Gerät, 6 Kanäle. Transport (Solarman V5 /
  Modbus TCP) und Geräteprofil werden je Kanal gewählt; die Gerätekonfiguration ist von der
  `share.xml` in eine Kanalvorlage gewandert. **Verschiebt alle KO-Nummern** (jetzt 809–916,
  18 KOs je Kanal).
- Die 17 Messwert-KOs je Kanal sind profilabhängig belegt: `ComObjectRef` überschreibt Name,
  `ObjectSize` und `DatapointType`, sodass Slot 1 beim Deye „Wirkleistung" (DPT 14.056) und
  beim Pylontech „Ladezustand" (DPT 5.001) ist — ohne zusätzliche KO-Nummern.
- `spvread` erwartet jetzt zusätzlich die Gerätenummer: `spvread <Gerät> <StartHex> [AnzahlHex]`.

### Added
- Profil **Pylontech Force H3** (12 Werte, Register ab 5120), am Gerät verifiziert.
- Transport **Modbus TCP** (MBAP-Header) neben Solarman V5.
