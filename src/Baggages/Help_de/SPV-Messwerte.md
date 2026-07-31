### Messwerte

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

