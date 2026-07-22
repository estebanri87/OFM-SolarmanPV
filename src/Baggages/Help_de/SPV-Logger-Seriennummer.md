### Logger-Seriennummer

Nur bei **Solarman V5** relevant; der MBAP-Rahmen kennt keine Seriennummer.

Der Logger prüft die Seriennummer und antwortet auf Anfragen mit falscher Nummer entweder gar
nicht oder mit einem Fehlerrahmen ohne Daten. Er sendet sie allerdings in **jeder** Antwort im
Kopf mit — auch im Fehlerfall. Deshalb kann sie meist automatisch ermittelt werden.

- **automatisch ermitteln** (Standard): Das Modul sendet beim Start eine Anfrage mit der
  Seriennummer 0 und übernimmt die Nummer aus der Antwort.
- **manuell eintragen**: Nötig bei Loggern, die auf eine falsche Seriennummer überhaupt nicht
  reagieren. Die Nummer ist zehnstellig und wird als Text eingegeben.

