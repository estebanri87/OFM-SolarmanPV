# Changelog OFM-SolarmanPV

## 0.2.0

Alles bis einschließlich dieser Fassung ist noch nicht veröffentlicht; die Abschnitte sind
chronologisch nach Entstehung geordnet.

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

### Added (Doku und Ausbaureserve)
- Applikationsbeschreibung mit 12 Hilfetexten, `createDoc.ps1` und `HelpContext`-Verweisen.
  Die Verweise stehen im Template-Generator, damit sie eine Neugenerierung überleben.
- Geräteprofil und Transportprotokoll werden als **Dropdown** dargestellt (`UIHint="DropDown"`),
  da die Profilliste mit jedem unterstützten Gerät wächst.
- **Ausbaureserve:** 24 Messwert-Slots je Kanal (statt 17) und fest reservierte Enable-Bits
  für 10 Geräteprofile (Byte 56–85). Ein neues Profil verschiebt dadurch weder
  Parameteradressen noch KO-Nummern — es braucht nur eine Registertabelle, einen Eintrag in
  `profileFor()`, einen in der Generator-Profilliste und eine Enum-Zeile.
  KO-Block je Kanal jetzt 25 (1 Status + 24 Messwerte), Obergrenze 958.

### Changed (Messwerttabelle statt einkompilierter Profile)

**Nicht abwärtskompatibel:** KO-Nummern und Parameteradressen verschieben sich erneut.
Bestehende Projekte müssen neu parametriert werden.

- Die Firmware kennt **keine Geräte mehr**. Was gelesen und wie es umgerechnet wird, steht
  vollständig in den ETS-Parametern: 64 Messwertzeilen je Gerät mit Register, Datentyp,
  Skalierung, Offset und einem Haken, ob die Zeile ein KO erzeugt. Damit ist ein unbekannter
  Wechselrichter **ohne Firmware-Änderung** einbindbar — die Forderung aus dem ursprünglichen
  Plan, die beim Profil-Umbau verlorengegangen war.
- Die **Bedeutung** jeder Zeile ist dagegen fest vergeben (`tools/slots.py`) und bestimmt
  KO-Name und DPT. Grund: Ein Versuch, den KO-Namen über `TextParameterRefId` an einen
  *Aufzählungs*-Parameter zu binden, schlug fehl — die ETS setzte statt des Anzeigetextes den
  Rohwert ein („0: Wirkleistung"). Bei 64 Slots × 30 Bedeutungen wären explizite
  ComObjectRefs nicht tragbar gewesen, deshalb die feste Zuordnung.
- KO-Block je Kanal 25 → **65** (1 Status + 64 Messwerte), `MAIN_MaxKoNumber` 958 → 1198,
  `MAIN_ParameterSize` 14242 → 16483. RAM unverändert (19,6 %), Flash 71,9 %.
- Die für 10 Profile reservierten Enable-Bits (Byte 56–85) entfallen — die Messwerttabelle
  ersetzt sie.
- `src/Profiles/*.h` wird nicht mehr eingebunden; die Registertabellen leben in
  `tools/profiles.py`.

### Added (Geräteprofile in der ETS)
- Profile sind jetzt reine **ETS-Vorlagen**: Der Knopf *Profil in Tabelle übernehmen* füllt
  die Messwerttabelle aus einer Datenbank im ETS-Skript. Ein neues Profil kostet einen
  Producer-Lauf, **kein Flashen**.
- **29 Profile**: Deye SUN-M80G3 und Pylontech Force H1/H2/H3 (beide am Gerät vermessen) sowie
  27 aus der Sammlung von [ha-solarman](https://github.com/davidrapan/ha-solarman) (MIT),
  übersetzt von `tools/yaml2profile.py`. Die übernommenen sind **ungeprüft**; beim Laden
  erscheint ein entsprechender Hinweis.
- `tools/yaml2profile.py` verwirft Einträge, die sich nicht verlustfrei abbilden lassen, und
  meldet jeden davon. Besonders `sensors` (Wert aus mehreren Quellen berechnet) ist heikel:
  solche Einträge lesen das angegebene Register gar nicht und ergäben eine plausible, aber
  falsche Zahl.

### Added (Assistent)
- Neue globale Seite **Assistent** zum Einmessen unbekannter Geräte: Transport und
  Seriennummer erkennen, Register lesen, Bedeutungen zuordnen, Umrechnung ableiten und alles
  in ein Gerät übernehmen — samt IP, Port, Slave-ID und Seriennummer.
- *Belegte Register suchen* liest 256 Register in Blöcken und übernimmt nur die belegten. Ein
  leeres Register direkt hinter einem belegten bleibt erhalten: Ein 32-Bit-Wert steht
  low-word-first, sein High-Word ist bei normalen Größen 0 und würde sonst wegfallen.
- *Skalierung ermitteln* probiert Datentyp, Skalierung und Offset gegen einen abgelesenen
  Sollwert durch. Der ganzzahlige Offsetbereich −128…127 siebt dabei die falschen Kandidaten
  aus: Aus Rohwert 6160 und 51,6 °C folgt nur bei ×0,01 ein runder Offset (10).
- Plausibilitätsvorschläge nur, wo der Wertebereich eindeutig ist (Netzfrequenz,
  Netzspannung). Bei Strömen bewusst keine — 113 kann 11,3 A oder 1,13 A sein.
- Gegenstelle in der Firmware: `SolarmanPVModule::processFunctionProperty`, `objectIndex 0xA1`,
  vier Kommandos (analysieren, Status, lesen, abholen) nach dem Start-und-Poll-Muster des
  OFM-PresenceModule. IP, Port und Slave-ID werden im Aufruf mitgegeben, weil das Gerät nur
  die zuletzt *heruntergeladenen* Parameter kennt.

### Added (Generatoren)
- `python tools/generate.py` erzeugt `SolarmanPV.templ.xml`, `SlotCatalog.h`,
  `SolarmanPV.profiles.parts.xml`, `SolarmanPV.assistant.parts.xml` und
  `SolarmanPV.script.js`. Quellen sind `tools/slots.py`, `tools/profiles.py` und
  `tools/script.template.js`.
- `tools/slots.py` ist eine **Schnittstelle**: Die Reihenfolge legt die KO-Nummern fest.
  Einträge dürfen angehängt oder umbenannt, aber nicht verschoben oder gelöscht werden.

### Fixed
- **Deye SUN-M80G3, Wirkleistung**: als 16-Bit-Wert auf 0x0056 eingetragen, laut
  Herstellerdefinition sind es 32 Bit über 0x0056+0x0057. Bei unserer Messung fiel das nicht
  auf, weil ein 800-W-Gerät nie über 6553,5 W kommt; bei größeren Geräten derselben Baureihe
  hätte die Leistung ab 6,5 kW von vorn gezählt. Aufgefallen beim Gegenlesen der
  YAML-Definition.
- **Berechnete Werte** (PV-Leistung = U × I) bezogen sich auf die *aktiven* Zeilen. Wurde die
  Spannung abgewählt, wäre stillschweigend eine fremde Zeile zum Faktor geworden. Bezug ist
  jetzt der Slot-Katalog; `tools/profiles.py` prüft beim Erzeugen, dass beide Faktoren im
  Profil enthalten sind.

### Fixed (Assistent zeigte Werte für nicht belegte Register)
Der Suchlauf am Pylontech Force H3 meldete 62 belegte Register — darunter Bereiche, die das
Gerät gar nicht kennt. Eine direkte Modbus-Abfrage von außen belegt es:

```
0x0030 (   48): EXC 2      (Illegal Data Address)
0x1400 ( 5120): [4098, 0, 0, 2095, 65535, 65278, 340, 69, 11, …]
0x1420 ( 5152): [100, 0, 7427, 0, 59, 0, 1551, 0, 59, 0, 1557, …]
0x3890 (14480): EXC 2      (Illegal Data Address)
```

Ursache: `parseModbusTcp()` prüfte die **Transaktionsnummer des MBAP-Headers nicht**. Wir setzen
sie beim Senden, verglichen sie aber nie mit der Antwort — jede fremde Antwort passender Länge
wurde akzeptiert. Da der Kanal-Poll denselben Logger parallel abfragte (ein zweiter Client!),
landete dessen Antwort beim Assistenten. Er zeigte damit **echte Messwerte unter Adressen an,
die es nicht gibt** — der gefährlichste Fehlerfall, weil das Ergebnis plausibel aussieht.

- `parseModbusTcp()` vergleicht die Transaktionsnummer und verwirft nicht zugeordnete Antworten
  (`ErrRejected`).
- `SolarmanPVModule::loop()` pausiert den Kanal-Poll, solange eine Assistenten-Messung läuft.
  Ein Solarman-Logger beantwortet nur einen Client zuverlässig — und der Kanal-Poll ist selbst
  einer.

**Bekannte Lücke:** `parseV5()` prüft die Sequenznummer ebenfalls nicht. Bei Solarman V5 wirkt
die Logger-Seriennummer als zusätzliche Zuordnung, und der Deye-Pfad arbeitet stabil; die
Semantik der Antwortsequenz ist aber nicht verifiziert und die Prüfung deshalb nicht ergänzt.

### Fixed (Assistententabelle blieb leer)
Die gelesenen Register erschienen nicht in der Tabelle — Kopfzeile und Zeilennummern wurden
gerendert, die Zellen blieben leer. Ursache war die **Größe der Tabelle**: 64 Zeilen × 7 Spalten
= 448 Zellen in einem `ParameterBlock`. Die Gerätetabelle (max. 16 Zeilen je Kategorie),
OFM-LightManager und OFM-PresenceModule (je 10 Zeilen) rendern einwandfrei. Die
Assistententabelle ist jetzt in **vier Blöcke à 16 Zeilen** aufgeteilt.

Ebenfalls entfernt: die Abhängigkeit von `SPV_AsstRowCount` in `spvBtnDeriveScale` und
`spvBtnApply`. Der Parameter wird nirgends angezeigt und seine Persistenz war ungeklärt; da
beide Funktionen Zeilen ohne Bedeutung ohnehin überspringen, ist die Schleifengrenze überflüssig.

### Changed (Rastertabellen statt gestapelter Listen)
Register, Datentyp, Skalierung und Offset standen je Messwert **untereinander** gestapelt —
bei 17 Werten eine sehr lange, unübersichtliche Seite. Beide Tabellen (Geräteseite im
erweiterten Modus, Assistent) verwenden jetzt eine echte **Rastertabelle**
(`ParameterBlock Layout="Table"` mit `<Rows>`, `<Columns>` und `Cell="Zeile,Spalte"`) — Muster
aus OFM-LightManager. Ein Messwert ist damit **eine Zeile** mit Spalten statt fünf gestapelter
Felder.

- Geräteseite: je Kategorie ein Raster (aktiv · Register · Datentyp · Skalierung · Offset);
  im Normalmodus bleibt die kompakte Häkchenliste. Die KO-Sichtbarkeit hängt weiter nur am
  aktiv-Haken, nicht am Anzeigemodus.
- Assistent: ein Raster (Register · Rohwert · Bedeutung · abgelesen · Datentyp · Skal. · Offset),
  Zeilen werden über den gefundenen Zählerstand aufgeblendet.

### Fixed (Zeilen der Assistenten-Tabelle unsichtbar)
Nach dem Suchlauf wurden die gefundenen Register nicht angezeigt. Ein ETS-`<choose>` rendert
genau **einen** `<when>` (den ersten passenden), nicht alle — die Zeilen lagen aber als flache
`<when>`-Geschwister nebeneinander. Sie sind jetzt **verschachtelt** (jede Zeile enthält das
`<choose>` der nächsten), Muster aus OFM-PresenceModule.

### Fixed (Absturz "setText eines Nullverweises")
„Profil in Tabelle übernehmen" brach mit „Die Eigenschaft „setText" eines Nullverweises …" ab.
**Offline-Knöpfe** (ohne `EventHandlerOnline`) erhalten in der ETS **kein** `progress`-Objekt —
`progress.setText(…)` läuft ins Leere. Weder ConfigTransfer noch PresenceModule nutzen `progress`
in Offline-Handlern; sie schreiben in ein Ausgabe-Parameter. Ebenso jetzt hier: `spvBtnLoadProfile`
schreibt in ein neues per-Gerät-Feld `ProfileMsg`, `spvBtnDeriveScale`/`spvBtnApply` in
`AsstResultText`. Die eigentliche Arbeit lief schon vorher korrekt — nur die Schlussmeldung stürzte
ab.

### Fixed (verschachtelte Arrays im ETS-Skript)
Beim Klick auf *Profil in Tabelle übernehmen* meldete die ETS „Die Eigenschaft „0" eines
undefinierten oder Nullverweises kann nicht abgerufen werden." Der einzige Code, der diese
Meldung erzeugt, ist ein Zugriff `row[0]` auf ein Array aus Arrays. Die Profildatenbank war als
`rows: [[slot,reg,typ,scale,off], …]` aufgebaut — die V8-Engine verarbeitet das fehlerfrei
(mit allen 29 Profilen geprüft), die ETS-Skript-Engine (Jint) offenbar nicht. Kein anderes
OpenKNX-Skript verwendet verschachtelte Array-Literale.

- `spvProfiles[].rows` ist jetzt ein **flaches** Zahlenarray (je Messwert fünf Werte
  hintereinander); `spvBtnLoadProfile` iteriert in Fünferschritten statt über `row[0..4]`.
- Ebenso in `spvBtnDeriveScale`: das `candidates`-Array aus Paaren wurde durch zwei flache
  Parallel-Arrays ersetzt.

### Fixed (Abgleich gegen OFM-PresenceModule)
Der ETS-Teil des Assistenten war nach dem Muster des OFM-PresenceModule gebaut, aber ohne
dessen Quelltext zu kennen. Der Abgleich brachte drei Fehler zutage, die erst am Gerät
aufgefallen wären:

- Die Knöpfe, die mit dem Gerät sprechen, hatten kein
  `EventHandlerOnline="ConnectionOriented"`. Ohne das gibt es keine Verbindung, und
  `online.connect()` hätte ins Leere gegriffen.
- Das Abholen der gelesenen Register lief über eine direkte `invokeFunctionProperty`. Deren
  Antwort ist auf die APDU begrenzt, die **15 Byte klein sein kann** — ein Block mit 24
  Registern braucht 51 Byte. Jetzt wird `BASE_invokeFunctionPropertyWrapper` aus OGM-Common
  benutzt, der Anfrage und Antwort stückelt; ein Durchgang holt alle 64 Register.
- `online.disconnect()` fehlte durchgängig. Die Verbindung blieb nach jedem Knopfdruck offen.

Außerdem übernommen: `progress.isCanceled()`, damit sich der Suchlauf abbrechen lässt (er
dauert etwa eine Minute), und `spvSleep()` in der erprobten Form. Hilfsfunktionen sind jetzt
echte Funktionen mit `op:nowarn`-Einträgen statt umständlicher Umgehungen — dasselbe Verfahren
nutzt OFM-ConfigTransfer.

### Changed (Oberfläche)
- Der Umschalter für die Registerspalten heißt **Erweiterter Modus** und steht in der Sektion
  *Messwerte*, nicht mehr in der Grundkonfiguration. Der vorherige Name „Hilfsfunktionen" war
  aus dem OFM-PresenceModule übernommen und traf hier nicht zu: Verborgen wird die eigentliche
  Konfiguration, keine Hilfsfunktion. `PT-SPVView` entfällt.
- Das Erkennungsergebnis wird als **Text** angezeigt statt als Auswahlfelder. Transport und
  Seriennummer sind Messergebnis, kein Auswahlpunkt; ein versehentlicher Klick hätte die
  Erkennung überschrieben.
- Der Eintrag „(kein Profil)" heißt **„ohne Profil – eigene Messwerttabelle"** — das ist der
  Normalzustand nach dem Assistenten, keine Abwesenheit.
- Das nie befüllte Feld „Profilvorschlag" wurde entfernt. Eine Profilerkennung ist nur für den
  Deye-Mikrowechselrichter belegt (Register 0x0000 = 4), beim Pylontech gibt es kein
  Typregister — zu wenig für einen Vorschlag.
- Alle ETS-Texte und die Applikationsbeschreibung verwenden **Umlaute** statt `ae`/`ue`/`oe`.
  Die `HelpContext`-Kennungen bleiben ASCII, weil der Baggage-Generator Umlaute für
  Dateinamen transliteriert.
- Die Hilfetexte enthalten **keine Markdown-Tabellen** mehr; die ETS stellt sie als rohe
  Pipe-Zeichen dar.
