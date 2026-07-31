# OFM-SolarmanPV

OpenKNX-Modul zur **lokalen** Anbindung von Wechselrichtern und Batteriespeichern mit
Solarman-Logger an den KNX-Bus. Kommunikation über TCP zum Logger (Port 8899, Modbus RTU im
Solarman-V5-Rahmen) — **keine Cloud**, keine Rate-Limits, Sekunden-Aktualität.

> **Status: Beta.** Protokoll, Kanalmodell und zwei Geräteprofile sind implementiert und am
> realen Gerät verifiziert. Noch offen: Hilfetexte und der Praxistest über längere Zeit.

## Stand

| Baustein | Status |
|---|---|
| Solarman-V5-Framing, Modbus RTU, CRC-16 | ✅ implementiert, am Gerät verifiziert |
| Logger-Seriennummer automatisch ermitteln | ✅ implementiert |
| Nicht-blockierende Zustandsmaschine | ✅ implementiert und kompiliert |
| Transport Solarman V5 **und** Modbus TCP | ✅ beide am Gerät verifiziert |
| Kanalmodell: 6 Geräte, Transport + Profil je Gerät | ✅ |
| Messwerttabelle in der ETS statt Profile in der Firmware | ✅ 64 Zeilen je Gerät |
| Profil Deye SUN-M80G3 (17 Werte) | ✅ am Gerät vermessen |
| Profil Pylontech Force H1/H2/H3 (12 Werte) | ✅ am Gerät verifiziert |
| Diagnose `spv` / `spvread` | ✅ Registerdump am echten Gerät |
| Assistent: Gerät einmessen aus der ETS heraus | ✅ implementiert, am Gerät noch ungetestet |
| 27 weitere Profile aus der ha-solarman-Sammlung | ✅ übernommen, ungeprüft |

## Ein neues Gerät einbinden

Die Firmware kennt **keine Geräte**. Was gelesen und wie es umgerechnet wird, steht
vollständig in den ETS-Parametern: 64 Messwertzeilen je Gerät mit Register, Datentyp,
Skalierung und Offset. Ein unbekannter Wechselrichter ist damit **ohne Firmware-Änderung**
einbindbar.

- **Passendes Profil vorhanden**: Profil wählen, *Profil in Tabelle übernehmen* drücken —
  fertig. Das Profil trägt auch Transport und Port ein, die von außen nicht erkennbar sind.
- **Kein Profil**: Tabelle im *Erweiterten Modus* von Hand füllen. Die
  Registerwerte liefert `spvread` über die Konsole.

Ein neues Profil kostet einen Eintrag in `tools/profiles.py` und einen Producer-Lauf — **kein
Flashen**, denn Profile leben ausschließlich in der ETS.

## Generierte Quellen

Vier Dateien werden erzeugt und dürfen nicht von Hand bearbeitet werden:

```
python tools/generate.py
```

| Quelle | erzeugt |
|---|---|
| `tools/slots.py` | `src/SolarmanPV.templ.xml`, `src/SlotCatalog.h`, `src/SolarmanPV.assistant.parts.xml` |
| `tools/profiles.py` | `src/SolarmanPV.profiles.parts.xml` |
| `tools/profiles.py` + `tools/script.template.js` | `src/SolarmanPV.script.js` |

`tools/profiles_yaml.py` ist ebenfalls erzeugt — aus den Gerätedefinitionen von
[ha-solarman](https://github.com/davidrapan/ha-solarman) (MIT):

```
python tools/yaml2profile.py <verzeichnis-mit-yaml> > tools/profiles_yaml.py
```

Der Konverter übernimmt nur, was sich verlustfrei abbilden lässt, und **verwirft** Einträge mit
Aufzählung, Bitmaske, Division oder mehreren Quellen (`sensors`) — letztere lesen das
angegebene Register gar nicht und ergäben eine plausible, aber falsche Zahl. Jeder verworfene
Eintrag wird gemeldet.

`tools/slots.py` ist der Katalog der 64 Messwert-Bedeutungen und **eine Schnittstelle**: Die
Reihenfolge legt die KO-Nummern fest. Einträge dürfen hinten angehängt oder umbenannt, aber
nicht verschoben oder gelöscht werden — das würde bestehende ETS-Projekte verschieben.

## Diagnose

Über die serielle Konsole:

```
spv                → Status aller Geräte
spvread 1 3b 10    → Gerät 1: 16 Register ab 0x003B (Start und Anzahl hexadezimal)
```

`spvread` gibt jedes Register dezimal, hexadezimal **und** vorzeichenbehaftet aus — die
Darstellung, die man braucht, um eine Registertabelle gegen bekannte Anzeigewerte abzugleichen.

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

## Nicht-blockierend

Der Client ist eine Zustandsmaschine (`Idle → Connecting → Sending → Receiving → Complete`)
auf rohen lwIP-Sockets mit `O_NONBLOCK`. `poll()` kehrt **immer sofort** zurück:

- `connect()` läuft über `EINPROGRESS`, der Fortschritt wird per `select()` mit Timeout `0` geprüft
- `send()`/`recv()` behandeln `EWOULDBLOCK` als „später weiter", nie als Fehler
- ein Gesamtbudget von 4 s je Transaktion wird nur über `millis()` geprüft, nie abgewartet

Damit behält der KNX-Stack sein Timing. Bewusst **keine** zusätzliche Abhängigkeit (kein
AsyncTCP, kein eModbus).

Verwendung:

```cpp
client.configure("192.168.30.201", 8899, serial, 1);
client.beginRead(3, 0x003B, 16);
// in jedem loop():
client.poll();
if (client.finished()) { /* client.result(), client.registers() */ client.clear(); }
```

**Einschränkung:** Nur IPv4-Literale, keine Hostnamen — eine Namensauflösung würde blockieren.

## Hardware-Unterstützung

|Prozessor | Status | Anmerkung |
|----------|--------|-----------|
|ESP32     | in Entwicklung | `WiFiClient` |
|RP2040    | –      | nicht vorgesehen |

## Lizenz

[GNU GPL v3](LICENSE)
