### Diagnose

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

