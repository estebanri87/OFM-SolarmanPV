### Diagnose

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

