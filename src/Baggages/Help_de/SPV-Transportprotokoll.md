### Transportprotokoll

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

