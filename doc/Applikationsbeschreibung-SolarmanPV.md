<!-- SPDX-License-Identifier: GPL-3.0-only -->
<!-- Copyright (C) 2026 OpenKNX -->

<!-- KEINE MARKDOWN-TABELLEN in den DOC-Bloecken: die ETS zeigt sie als rohe Pipe-Zeichen an.
     Aufzaehlungen verwenden. Die Bloecke zwischen DOC und DOCEND werden von
     "openknxproducer baggages" zu den Hilfetexten in src/Baggages/Help_de/ verarbeitet. -->

# Applikationsbeschreibung SolarmanPV

Das Modul liest Wechselrichter und Batteriespeicher, die einen **Solarman-Logger** besitzen,
**lokal über das Netzwerk** aus und stellt die Messwerte als KNX-Gruppenobjekte bereit.
Keine Cloud, kein Konto, keine Abfragegrenzen — die Werte stehen im Sekundenbereich zur
Verfügung und eignen sich damit auch für Überschussregelung und Laststeuerung.

Je Kanal wird **ein Gerät** angebunden. Kanal und Gerät sind dasselbe.

## Wichtige Hinweise

* Diese KNXprod wird nicht von der KNX Association offiziell unterstützt!
* Die Erzeugung der KNXprod geschieht auf eure eigene Verantwortung!

# Allgemein

<!-- DOC -->
## Allgemein

Auf dieser Seite wird nur festgelegt, wie viele Geräte angebunden werden. Die eigentliche
Konfiguration erfolgt je Gerät auf einer eigenen Seite.

<!-- DOCEND -->

<!-- DOC -->
## Geräteauswahl

**Aktive Geräte** (0–6): Anzahl der eingeblendeten Geräteseiten. Jedes Gerät bekommt eine
eigene Verbindung, ein eigenes Profil und einen eigenen Satz Kommunikationsobjekte.

<!-- DOCEND -->

<!-- DOC -->
## Nur ein Client gleichzeitig

Ein Solarman-Logger beantwortet immer nur **eine** Verbindung zuverlässig. Läuft parallel
eine andere Abfrage — etwa eine Home-Assistant-Integration oder die Hersteller-App im lokalen
Modus — liefert das Gerät unvollständige, verzögerte oder fremde Antworten: Anfragen werden
dann mit Daten beantwortet, die zu einer anderen Anfrage gehören.

Das ist kein Fehler des Moduls und lässt sich auch nicht umgehen. Entweder fragt dieses Gerät
den Logger ab oder der andere Client.

<!-- DOCEND -->

<!-- DOC -->
## Assistent

Der Assistent bindet ein Gerät ein, für das es **kein Profil** gibt. Er ermittelt Transport
und Seriennummer, liest Register aus, leitet die Umrechnung ab und überträgt das Ergebnis in
ein Gerät. Passt ein Profil, ist er nicht nötig.

Er ist **global**, nicht je Gerät — man misst ohnehin ein Gerät zur Zeit.

> Der Assistent spricht mit dem Gerät und setzt deshalb voraus, dass es **bereits mit der ETS
> programmiert** wurde. Andernfalls erst programmieren, dann eine andere Seite wählen und
> hierher zurückkehren.

> Ein Solarman-Logger beantwortet nur **einen** Client zuverlässig. Home Assistant oder andere
> Abfragen für das Testgerät währenddessen abschalten, sonst kommen fremde oder
> unvollständige Antworten.

**1. Gerät.** IP-Adresse, Port und Modbus-Slave-ID eintragen, dann *Gerät analysieren*. Der
Assistent probiert zuerst Solarman V5, dann reines Modbus TCP. Beide laufen auf Port 8899 und
sind von außen nicht unterscheidbar — genau deshalb gibt es diesen Schritt.

**2. Erkannt.** Bei Solarman V5 erscheint auch die Logger-Seriennummer; Modbus TCP kennt
keine. Antwortet keiner der beiden Transporte, stimmt entweder die Adresse nicht, oder ein
anderer Client belegt das Gerät.

**3. Register lesen.** Zwei Wege:

- *Belegte Register suchen* liest 256 Register ab der eingestellten Adresse und übernimmt nur
  die, die überhaupt einen Wert liefern. Das dauert etwa eine Minute und lässt sich jederzeit
  abbrechen; die bis dahin gefundenen Register bleiben eingetragen. Das ist der Weg, wenn man
  nicht weiß, wo beim Gerät etwas steht.
- *Bereich lesen* holt gezielt die eingestellte Anzahl ab der Startadresse. Manche Logger
  lehnen Blöcke über 16 Register ab.

**4. Bedeutung zuordnen.** Welches Register welcher Messwert ist, sagt kein Rohwert von selbst
— das ist der eine Schritt, der Handarbeit bleibt. Zu jeder Zeile, die auf den Bus soll, die
Bedeutung eintragen.

Nur wo der Wertebereich eindeutig ist, macht der Suchlauf einen Vorschlag: 4700–5300 deutet
auf die Netzfrequenz, 1900–2600 auf die Netzspannung. Diese Vorschläge sind geraten und zu
prüfen. Bei Strömen unterbleibt ein Vorschlag bewusst — 113 kann 11,3 A oder 1,13 A sein, und
ein falscher Vorschlag wäre schlimmer als keiner.

Der **abgelesene Wert** ist optional, aber der einzige Weg zu einer sicheren Umrechnung — er
löst genau die Mehrdeutigkeit auf, an der ein Vorschlag scheitert. *Skalierung ermitteln*
probiert dann alle Kombinationen aus Datentyp und Skalierung durch und behält die, die einen
ganzzahligen Offset ergibt.

Ein Beispiel: Register 0x005A liefert den Rohwert 6160, am Gerät stehen 51,6 °C. Geprüft wird
`Rohwert × Skalierung − 51,6`:

- ×1 ergibt 6108,4 — kein ganzzahliger Offset
- ×0,1 ergibt 564,4 — kein ganzzahliger Offset
- **×0,01 ergibt genau 10,0** — passt
- ×0,001 ergibt −45,44 — kein ganzzahliger Offset

Genau eine Kombination geht auf, und es ist die richtige. Ohne den Sollwert hätte „61,6 °C"
plausibel ausgesehen und wäre falsch gewesen.

**Wichtig:** Rohwert und abgelesener Wert müssen aus **demselben Moment** stammen. Ein während
des Ablesens gestiegener Wert führt zu einer Kombination, die zufällig passt — oder zu keiner.

**5. Übernehmen.** Zielgerät wählen und übernehmen. Es wandert **alles** von dieser Seite in
das Gerät: IP-Adresse, Port, Slave-ID, Transport, Seriennummer und jede zugeordnete Zeile. Auf
der Geräteseite ist nichts nachzutragen. Dessen bisherige Messwerttabelle wird dabei
**vollständig überschrieben**, und das Geräteprofil wird geleert — die Tabelle stammt nun aus
der Messung, kein Profilname soll etwas anderes behaupten.

Nichts davon verändert das Gerät sofort — die Konfiguration wird erst beim nächsten Download
wirksam.

<!-- DOCEND -->

# Geräteeinstellungen

<!-- DOC -->
## Geräteprofil

Ein Profil ist eine **Vorlage**: Es trägt die Registeradressen und Umrechnungen eines
bekannten Geräts in die Messwerttabelle ein. Danach ist die Tabelle maßgeblich — das Profil
wird nicht mehr gebraucht und kann von Hand nachgebessert werden.

- **Deye SUN-M80G3** — 17 Messwerte: Wirkleistung, Tages- und Gesamtertrag
  (gesamt und je String), Netzspannung, Netzstrom, Netzfrequenz, Temperatur sowie Spannung,
  Strom und Leistung beider PV-Eingänge.
- **Pylontech Force H1/H2/H3** — 12 Messwerte: Ladezustand, Spannung, Strom, Leistung, Temperatur,
  Alterungszustand, Restkapazität, Ladezyklen sowie geladene und entladene Energie für
  heute und gesamt.

Nur diese beiden Profile wurden an echten Geräten gegen deren Anzeige geprüft. Weitere Profile
stammen aus fremder Quelle und können falsche Skalierungen enthalten, ohne dass es auffällt;
sie lassen sich über die Seite **Assistent** mit einem einzigen abgelesenen Sollwert
gegenprüfen.

**Passt kein Profil**, ist das Gerät trotzdem nutzbar: Die Messwerttabelle lässt sich von Hand
ausfüllen oder vom Assistenten ermitteln. Eine Firmware-Änderung ist dafür nicht nötig.

<!-- DOCEND -->

<!-- DOC -->
## Verbindung

- **IP-Adresse**: Adresse des Loggers beziehungsweise des Geräts, in dem er verbaut ist.
  Es sind nur IPv4-Adressen zulässig, **keine Hostnamen** — eine Namensauflösung würde den
  KNX-Stack blockieren.
- **Port**: üblicherweise 8899, sowohl bei Solarman V5 als auch bei Modbus TCP.
- **Abfrageintervall** (Sekunden, 0 = aus): Abstand zwischen zwei vollständigen Lesezyklen.

<!-- DOCEND -->

<!-- DOC -->
## Transportprotokoll

Beide Verfahren transportieren denselben Modbus-Inhalt, nur unterschiedlich verpackt. Von
außen sind sie **nicht unterscheidbar** — beide laufen über Port 8899, und auch andere
Integrationen zeigen für beide schlicht „TCP" an. Welches Verfahren ein Gerät spricht, zeigt
sich erst im Betrieb.

- **Solarman V5** — der Modbus-Rahmen steckt in einem herstellereigenen Rahmen, der die
  Logger-Seriennummer enthält. Typisch für Solarman-Sticks und für Wechselrichter mit
  integriertem Logger.
- **Modbus TCP** — der übliche MBAP-Header ohne Seriennummer. Typisch für Ethernet-Logger,
  ESP-Adapter und manche Batteriespeicher.

Antwortet ein Gerät nicht, ist ein Wechsel des Transportprotokolls der erste sinnvolle
Versuch. Der Diagnosebefehl `spv` zeigt, ob eine Verbindung zustande kommt.

<!-- DOCEND -->

<!-- DOC -->
## Logger-Seriennummer

Nur bei **Solarman V5** relevant; der MBAP-Rahmen kennt keine Seriennummer.

Der Logger prüft die Seriennummer und antwortet auf Anfragen mit falscher Nummer entweder gar
nicht oder mit einem Fehlerrahmen ohne Daten. Er sendet sie allerdings in **jeder** Antwort im
Kopf mit — auch im Fehlerfall. Deshalb kann sie meist automatisch ermittelt werden.

- **automatisch ermitteln** (Standard): Das Modul sendet beim Start eine Anfrage mit der
  Seriennummer 0 und übernimmt die Nummer aus der Antwort.
- **manuell eintragen**: Nötig bei Loggern, die auf eine falsche Seriennummer überhaupt nicht
  reagieren. Die Nummer ist zehnstellig und wird als Text eingegeben.

<!-- DOCEND -->

<!-- DOC -->
## Modbus-Slave-ID

Die Adresse im eingebetteten Modbus-Rahmen, üblicherweise 1. Ein abweichender Wert ist nur
nötig, wenn mehrere Geräte hinter einem Logger kaskadiert sind.

<!-- DOCEND -->

<!-- DOC -->
## Sendeverhalten

Gilt für alle Messwerte des Geräts gemeinsam:

- **zyklisch senden alle** (Zeit + Zeitbasis, 0 = aus): Alle aktivierten Werte werden in
  diesem Abstand gesendet.
- **zusätzlich bei Änderung um** (Prozent, 0 = aus): Ein Wert wird außerdem gesendet, sobald
  er sich gegenüber dem zuletzt gesendeten um mindestens diesen Anteil verändert hat.

Sind beide Einstellungen 0, wird nichts gesendet. Nach dem Start wird jeder aktivierte Wert
einmal gesendet, sobald er zum ersten Mal gelesen wurde.

<!-- DOCEND -->

<!-- DOC -->
## Anzeige

Der Schalter **Erweiterter Modus** steht in der Sektion *Messwerte*, weil er ausschließlich
deren Darstellung betrifft.

**Aus** (Voreinstellung) zeigt je Messwert nur die Checkbox. Das genügt, wenn ein Geräteprofil
passt oder der Assistent die Tabelle bereits gefüllt hat: Man hakt an, was auf den Bus soll,
und ist fertig.

**Ein** blendet zusätzlich Register, Datentyp, Skalierung und Offset jeder angehakten Zeile
ein. Das braucht man beim Einrichten eines Geräts ohne Profil, beim Gegenprüfen eines
übernommenen Profils oder wenn ein einzelner Wert erkennbar falsch skaliert ankommt.

Die Einstellung ändert nur die Anzeige in der ETS — auf die Konfiguration des Geräts hat sie
keinen Einfluss.

<!-- DOCEND -->

<!-- DOC -->
## Messwerte

Jedes Gerät hat 64 Messwert-Zeilen mit **fest vergebener Bedeutung**. Ist eine Zeile angehakt,
erscheint das zugehörige Kommunikationsobjekt mit passendem Namen und Datentyp; andernfalls
entfällt es. Ein Gerät belegt üblicherweise nur einen Teil der Zeilen — ein Mikrowechselrichter
etwa 17, ein Batteriespeicher etwa 12.

Woher der Wert kommt, steht in den Feldern **Register, Datentyp, Skalierung und Offset**. Sie
werden normalerweise nicht von Hand ausgefüllt, sondern vom **Geräteprofil** oder vom
**Assistenten** eingetragen. Sichtbar sind sie nur bei angehaktem *Erweitertem Modus*.

Die Umrechnung lautet:

```
Wert = Rohwert × Skalierung − Offset
```

Beispiel Deye-Gerätetemperatur: Rohwert 6160, Skalierung ×0,01, Offset 10 ergibt 51,6 °C.

Der Datentyp **„Produkt der zwei Vorgänger"** bildet keinen eigenen Registerwert ab, sondern
multipliziert die beiden darüberliegenden Zeilen. So entstehen die PV-Leistungen und die
Batterieleistung, die die Hardware nicht direkt liefert (Spannung × Strom).

Energiewerte werden in **Wattstunden** gesendet (DPT 13.010), nicht in Kilowattstunden. Das
erhält die Auflösung: Ein Tagesertrag von 5,3 kWh würde als ganzzahliger kWh-Wert auf 5 kWh
gerundet, in Wattstunden bleibt er exakt.

Deckt der Katalog einen Wert deines Geräts nicht ab, stehen unter **Reserve** vier freie
Einträge bereit; sie senden als Gleitkommazahl ohne feste Einheit.

<!-- DOCEND -->

# Kommunikationsobjekte

<!-- DOC -->
## Statusobjekt

**erreichbar** (DPT 1.011) je Gerät: zeigt an, ob die letzte Abfrage erfolgreich war. Fällt
das Objekt ab, antwortet der Logger nicht mehr — mögliche Ursachen sind ein zweiter Client,
eine falsche Seriennummer oder ein falsch gewähltes Transportprotokoll.

<!-- DOCEND -->

# Diagnose

<!-- DOC -->
## Diagnose

Über die serielle Konsole:

- `spv` — Status aller Geräte: Zahl der aktiven Messwerte und Leseblöcke, Erreichbarkeit,
  ermittelte Seriennummer.
- `spvread <Gerät> <StartHex> [AnzahlHex]` — liest Register direkt aus, zum Beispiel
  `spvread 1 3b 10` für 16 Register ab 0x003B am ersten Gerät. Die Ausgabe zeigt jeden Wert
  dezimal, hexadezimal und vorzeichenbehaftet.

`spvread` macht dasselbe wie der Assistent, nur über die Konsole: Registerwerte auslesen und
mit bekannten Anzeigewerten desselben Moments vergleichen. Nur so lassen sich Skalierung und
Vorzeichen zweifelsfrei bestimmen — geraten führt zu plausibel aussehenden, aber falschen
Werten auf dem Bus. Für den Normalfall ist der Assistent der bequemere Weg; die Konsole bleibt
nützlich, wenn man ein Gerät im laufenden Betrieb beobachten will.

Die Blockgröße ist begrenzt: Anfragen über etwa 16 Register hinaus können ins Leere laufen.

<!-- DOCEND -->
