### Sendeverhalten

Gilt für alle Messwerte des Geräts gemeinsam:

- **zyklisch senden alle** (Zeit + Zeitbasis, 0 = aus): Alle aktivierten Werte werden in
  diesem Abstand gesendet.
- **zusätzlich bei Änderung um** (Prozent, 0 = aus): Ein Wert wird außerdem gesendet, sobald
  er sich gegenüber dem zuletzt gesendeten um mindestens diesen Anteil verändert hat.

Sind beide Einstellungen 0, wird nichts gesendet. Nach dem Start wird jeder aktivierte Wert
einmal gesendet, sobald er zum ersten Mal gelesen wurde.

