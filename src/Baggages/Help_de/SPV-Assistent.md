### Assistent

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

