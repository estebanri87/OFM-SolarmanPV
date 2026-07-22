### Nur ein Client gleichzeitig

Ein Solarman-Logger beantwortet immer nur **eine** Verbindung zuverlässig. Läuft parallel
eine andere Abfrage — etwa eine Home-Assistant-Integration oder die Hersteller-App im lokalen
Modus — liefert das Gerät unvollständige, verzögerte oder fremde Antworten: Anfragen werden
dann mit Daten beantwortet, die zu einer anderen Anfrage gehören.

Das ist kein Fehler des Moduls und lässt sich auch nicht umgehen. Entweder fragt dieses Gerät
den Logger ab oder der andere Client.

