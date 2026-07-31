### Geräteprofil

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

