<!-- SPDX-License-Identifier: GPL-3.0-only -->
<!-- Copyright (C) 2026 OpenKNX -->

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
## Geraeteauswahl

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

# Geraeteeinstellungen

<!-- DOC -->
## Geraeteprofil

Das Profil bestimmt, welche Register gelesen werden und welche Messwerte damit zur Verfügung
stehen. Es legt außerdem Namen, Einheit und Datentyp der Kommunikationsobjekte fest — im
Kanal ist deshalb nichts über Registeradressen einzustellen.

- **Deye Mikrowechselrichter** — 17 Messwerte: Wirkleistung, Tages- und Gesamtertrag
  (gesamt und je String), Netzspannung, Netzstrom, Netzfrequenz, Temperatur sowie Spannung,
  Strom und Leistung beider PV-Eingänge.
- **Pylontech Force** — 12 Messwerte: Ladezustand, Spannung, Strom, Leistung, Temperatur,
  Alterungszustand, Restkapazität, Ladezyklen sowie geladene und entladene Energie für
  heute und gesamt.

Wird das Profil gewechselt, ändern sich die angebotenen Messwerte und die Bedeutung der
Kommunikationsobjekte entsprechend.

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
## Messwerte

Für jeden Messwert des gewählten Profils gibt es eine Checkbox. Ist sie gesetzt, erscheint das
zugehörige Kommunikationsobjekt mit passendem Namen und Datentyp; andernfalls entfällt es.

Energiewerte werden in **Wattstunden** gesendet (DPT 13.010), nicht in Kilowattstunden. Das
erhält die Auflösung: Ein Tagesertrag von 5,3 kWh würde als ganzzahliger kWh-Wert auf 5 kWh
gerundet, in Wattstunden bleibt er exakt.

Die PV-Leistungen des Wechselrichters und die Batterieleistung liefert die Hardware nicht
direkt; sie werden aus Spannung mal Strom berechnet.

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

- `spv` — Status aller Geräte: Profil, Erreichbarkeit, ermittelte Seriennummer.
- `spvread <Gerät> <StartHex> [AnzahlHex]` — liest Register direkt aus, zum Beispiel
  `spvread 1 3b 10` für 16 Register ab 0x003B am ersten Gerät. Die Ausgabe zeigt jeden Wert
  dezimal, hexadezimal und vorzeichenbehaftet.

`spvread` ist das Werkzeug, mit dem neue Geräteprofile entstehen: Registerwerte auslesen und
mit bekannten Anzeigewerten desselben Moments vergleichen. Nur so lassen sich Skalierung und
Vorzeichen zweifelsfrei bestimmen — geraten führt zu plausibel aussehenden, aber falschen
Werten auf dem Bus.

Die Blockgröße ist begrenzt: Anfragen über etwa 16 Register hinaus können ins Leere laufen.

<!-- DOCEND -->
