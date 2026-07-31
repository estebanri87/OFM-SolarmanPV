// ---------------------------------------------------------------------------------------
// Logikteil des ETS-Skripts. Wird von tools/gen_profiles.py hinter die Profildatenbank
// gehaengt; bearbeitet wird diese Datei, nicht src/SolarmanPV.script.js.
//
// Hinweis: Der Producer meldet jede Funktion, die kein EventHandler in der XML aufruft
// (WARN 003). Hilfsfunktionen brauchen deshalb einen op:nowarn-Eintrag in SolarmanPV.share.xml
// - so macht es auch OFM-ConfigTransfer.
// ---------------------------------------------------------------------------------------

// Schreibt ein Profil in die Messwerttabelle eines Geraets. Danach ist die Tabelle
// massgeblich; das Profil selbst wird nicht mehr gebraucht und darf von Hand nachgebessert
// werden. Die Parameternamen folgen der Bildungsregel aus tools/gen_templ.py:
// SPV_CH<Kanal><Feld><Slotname>, also zum Beispiel SPV_CH1RegPower.
function spvBtnLoadProfile(device, online, progress, context) {
    var channel = parseInt(context.channel);
    var prefix = "SPV_CH" + channel;
    var selection = device.getParameterByName(prefix + "Profile").value;

    // Offline-Knoepfe bekommen kein progress-Objekt (wie ConfigTransfer/PresenceModule).
    // Rueckmeldung geht deshalb in ein Anzeige-Parameter, nicht ueber progress.setText.
    var msg = device.getParameterByName(prefix + "ProfileMsg");

    if (!selection || selection <= 0 || selection >= spvProfiles.length) {
        msg.value = "Kein Profil gewählt.";
        return;
    }
    var profile = spvProfiles[selection];

    // Erst alles zuruecksetzen: sonst blieben beim Profilwechsel Zeilen des alten Geraets
    // stehen und wuerden Register lesen, die es gar nicht gibt.
    var i;
    for (i = 0; i < spvSlotNames.length; i++) {
        device.getParameterByName(prefix + "En" + spvSlotNames[i]).value = 0;
        device.getParameterByName(prefix + "Reg" + spvSlotNames[i]).value = 0;
        device.getParameterByName(prefix + "Type" + spvSlotNames[i]).value = 0;
        device.getParameterByName(prefix + "Scale" + spvSlotNames[i]).value = 0;
        device.getParameterByName(prefix + "Offset" + spvSlotNames[i]).value = 0;
    }

    // profile.rows ist flach: je Messwert fuenf Werte (Slot, Register, Typ, Skalierung,
    // Offset). Bewusst kein Array aus Arrays - siehe Kommentar bei spvProfiles.
    for (i = 0; i + SPV_ROW <= profile.rows.length; i += SPV_ROW) {
        var slot = spvSlotNames[profile.rows[i]];
        device.getParameterByName(prefix + "Reg" + slot).value = profile.rows[i + 1];
        device.getParameterByName(prefix + "Type" + slot).value = profile.rows[i + 2];
        device.getParameterByName(prefix + "Scale" + slot).value = profile.rows[i + 3];
        device.getParameterByName(prefix + "Offset" + slot).value = profile.rows[i + 4];
        device.getParameterByName(prefix + "En" + slot).value = 1;
    }

    // Transport und Port gehoeren zum Profil: der Pylontech spricht reines Modbus TCP, der
    // Deye Solarman V5 - beide auf Port 8899. Das ist von aussen nicht zu erkennen.
    device.getParameterByName(prefix + "Transport").value = profile.transport;
    device.getParameterByName(prefix + "LoggerPort").value = profile.port;

    // Kurz halten: das Meldungsfeld fasst 48 Byte, Umlaute zaehlen doppelt. Der Profilname
    // steht ohnehin im Dropdown darueber.
    var note = profile.verified ? "" : " (ungeprüft!)";
    msg.value = (profile.rows.length / SPV_ROW) + " Messwerte übernommen." + note;
}

// ---------------------------------------------------------------------------------------
// Assistent. Die Gegenstelle im Geraet ist SolarmanPVModule::processFunctionProperty,
// objectIndex 0xA1, propertyId 1. Erstes Datenbyte ist das Kommando:
//   1 = analysieren   2 = Status   3 = Register lesen   4 = Ergebnis abholen
// Jede Antwort beginnt mit [0] Status (0 = angenommen) und [1] fertig (0 = laeuft noch).
//
// Kurze Aufrufe gehen direkt ueber invokeFunctionProperty. Fuer das Abholen der Register
// (bis zu 131 Byte) MUSS BASE_invokeFunctionPropertyWrapper genommen werden: eine APDU kann
// 15 Byte klein sein, der Wrapper aus OGM-Common stueckelt Anfrage und Antwort. Dasselbe
// Muster nutzt das OFM-PresenceModule.
//
// invokeFunctionProperty ist synchron, eine Messung dauert aber Sekunden - deshalb starten
// und pollen.
// ---------------------------------------------------------------------------------------

function spvSleep(milliseconds) {
    var currentTime = new Date().getTime();
    while (currentTime + milliseconds >= new Date().getTime()) {
    }
}

// Sucht den Index eines Slotnamens. Bewusst als Schleife statt Array.indexOf: die
// ETS-Skript-Engine ist ES3-nah und kennt Array.prototype.indexOf nicht ("Das Objekt
// unterstuetzt die Methode indexOf nicht"). Kein funktionierendes OpenKNX-Skript nutzt sie.
function spvSlotIndex(name) {
    for (var i = 0; i < spvSlotNames.length; i++)
        if (spvSlotNames[i] === name) return i;
    return -1;
}

// Zerlegt eine IPv4-Adresse in vier Bytes. Gibt null zurueck, wenn sie nicht taugt.
function spvIp4(device) {
    var parts = ("" + device.getParameterByName("SPV_AsstIp").value).split(".");
    if (parts.length != 4) return null;
    var out = [];
    for (var i = 0; i < 4; i++) {
        var n = parseInt(parts[i]);
        if (isNaN(n) || n < 0 || n > 255) return null;
        out.push(n);
    }
    return out;
}

// Pollt den Status, bis das Geraet fertig meldet. Gibt die Antwort zurueck oder null bei
// Zeitablauf beziehungsweise Abbruch durch den Anwender.
function spvAwaitResult(online, progress, timeoutMs, from, to) {
    var waited = 0;
    var resp = [0, 0];
    while (resp[0] === 0 && resp[1] === 0 && waited < timeoutMs) {
        if (progress.isCanceled()) return null;
        spvSleep(500);
        waited += 500;
        if (to > from) progress.setProgress(from + (to - from) * waited / timeoutMs);
        resp = online.invokeFunctionProperty(161, 1, [2]);
    }
    return (resp[0] === 0 && resp[1] === 1) ? resp : null;
}

// Vorschlaege fuer die Bedeutung, wenn der Wertebereich eindeutig ist. Bewusst kurz gehalten:
// Netzspannung und Netzfrequenz sind aus dem Rohwert erkennbar, ein Strom nicht (113 kann
// 11,3 A oder 1,13 A sein). Lieber zwei sichere Vorschlaege als zehn geratene.
var spvGuesses = [
    {min: 4700, max: 5300, slot: "GridFrequency"},
    {min: 1900, max: 2600, slot: "GridVoltage"}
];

function spvBtnAnalyse(device, online, progress, context) {
    var ip = spvIp4(device);
    if (!ip) {
        progress.setText("Bitte eine IPv4-Adresse eintragen, zum Beispiel 192.168.30.201.");
        return;
    }
    var port = parseInt(device.getParameterByName("SPV_AsstPort").value);
    var slave = parseInt(device.getParameterByName("SPV_AsstSlaveId").value);
    var probe = parseInt(device.getParameterByName("SPV_AsstStartReg").value);

    online.connect();
    try {
        progress.setText("Suche Gerät " + ip.join(".") + ":" + port + " …");
        online.invokeFunctionProperty(161, 1, [1, ip[0], ip[1], ip[2], ip[3],
            (port >> 8) & 0xFF, port & 0xFF, slave, (probe >> 8) & 0xFF, probe & 0xFF]);

        // Beide Transporte werden nacheinander versucht, jeder mit eigenem Zeitlimit.
        var resp = spvAwaitResult(online, progress, 30000, 0, 95);
        if (!resp) {
            device.getParameterByName("SPV_AsstResultText").value = "Keine Antwort vom Gerät.";
            progress.setText("Das Gerät hat nicht geantwortet. Ist es programmiert und im Netz?");
            return;
        }
        if (resp[7] !== 0) {
            device.getParameterByName("SPV_AsstResultText").value = "Nicht erreichbar.";
            progress.setText("Unter " + ip.join(".") + ":" + port + " antwortet weder ein "
                             + "Solarman-V5-Logger noch ein Modbus-TCP-Gerät. Prüfe Adresse, "
                             + "Port und ob eine andere Abfrage (etwa Home Assistant) das "
                             + "Gerät gerade belegt.");
            return;
        }

        var transport = resp[2];
        var serial = (resp[3] * 16777216) + (resp[4] * 65536) + (resp[5] * 256) + resp[6];
        device.getParameterByName("SPV_AsstTransport").value = transport;
        device.getParameterByName("SPV_AsstSerial").value = serial ? ("" + serial) : "";

        // Der Ergebnistext ist die einzige Anzeige. Transport und Seriennummer bleiben als
        // Maschinenwerte verborgen - sie sind Messergebnis, nicht Auswahl.
        var text = transport === 0
            ? "Solarman V5, Logger-Seriennummer " + serial
            : "Modbus TCP (ohne Logger-Seriennummer)";
        device.getParameterByName("SPV_AsstResultText").value = text;
        progress.setProgress(100);
        progress.setText(text + " erkannt.");
    } finally {
        online.disconnect();
    }
}

// Liest einen Block und traegt ihn ab Zeile "row" ein. Gibt die neue Zeilennummer zurueck,
// -1 bei Abbruch. onlyUsed=true ueberspringt leere Register (Suchlauf). timeoutMs bestimmt,
// wie lange auf eine Antwort gewartet wird - beim Suchlauf kurz, damit leere Bereiche schnell
// uebersprungen werden.
function spvReadBlock(device, online, progress, ip, port, slave, transport,
                      from, count, row, onlyUsed, state, timeoutMs) {
    online.invokeFunctionProperty(161, 1, [3, ip[0], ip[1], ip[2], ip[3],
        (port >> 8) & 0xFF, port & 0xFF, slave, transport,
        (from >> 8) & 0xFF, from & 0xFF, count]);

    var resp = spvAwaitResult(online, progress, timeoutMs, 0, 0);
    if (!resp && progress.isCanceled()) return -1;
    // Ein unbelegter Bereich wird oft gar nicht beantwortet - das ist kein Fehler, sondern
    // die Antwort "hier steht nichts".
    if (!resp || resp[7] !== 0 || resp[8] === 0) return row;
    state.answered++;

    // Ueber den Wrapper, weil die Antwort mit 64 Registern 131 Byte lang wird und eine APDU
    // 15 Byte klein sein kann. Der Wrapper stueckelt beides.
    var data = BASE_invokeFunctionPropertyWrapper(161, 1, [4, 0], device, online, progress);
    var n = data[2];
    for (var i = 0; i < n && row < spvSlotNames.length; i++) {
        var value = (data[3 + i * 2] * 256) + data[4 + i * 2];
        // Ein leeres Register wird uebersprungen - AUSSER es folgt direkt auf ein belegtes.
        // Ein 32-Bit-Wert steht low-word-first, sein High-Word ist bei normalen Groessen 0;
        // wuerde es wegfallen, waere die Zeile nicht mehr als 32-Bit-Wert erkennbar.
        if (onlyUsed && value === 0 && !state.lastWasSet) { state.empty++; continue; }
        state.lastWasSet = (value !== 0);

        var pad = row < 10 ? "0" : "";
        device.getParameterByName("SPV_AsstReg" + pad + row).value = from + i;
        device.getParameterByName("SPV_AsstRaw" + pad + row).value = value;
        device.getParameterByName("SPV_AsstRef" + pad + row).value = 0;

        var guessed = 0;
        if (onlyUsed) {
            for (var g = 0; g < spvGuesses.length; g++) {
                if (value >= spvGuesses[g].min && value <= spvGuesses[g].max) {
                    guessed = spvSlotIndex(spvGuesses[g].slot) + 1;
                    break;
                }
            }
        }
        device.getParameterByName("SPV_AsstMeaning" + pad + row).value = guessed;
        row++;
    }
    return row;
}

// Leert die Messtabelle. Noetig, weil immer alle 64 Zeilen sichtbar sind: ohne das blieben
// Reste eines frueheren Laufs unter den neuen Werten stehen.
function spvClearTable(device) {
    for (var i = 0; i < spvSlotNames.length; i++) {
        var pad = i < 10 ? "0" : "";
        device.getParameterByName("SPV_AsstReg" + pad + i).value = 0;
        device.getParameterByName("SPV_AsstRaw" + pad + i).value = 0;
        device.getParameterByName("SPV_AsstMeaning" + pad + i).value = 0;
        device.getParameterByName("SPV_AsstRef" + pad + i).value = 0;
        device.getParameterByName("SPV_AsstType" + pad + i).value = 0;
        device.getParameterByName("SPV_AsstScale" + pad + i).value = 0;
        device.getParameterByName("SPV_AsstOffset" + pad + i).value = 0;
    }
}

// Laeuft den vorbereiteten Suchplan (spvScanBlocks) ab und uebernimmt jedes Register, das
// einen Wert liefert. Der Anwender muss NICHT wissen, wo beim jeweiligen Geraet die Register
// liegen - genau das nimmt der Suchplan ab. Leere Bereiche werden mit kurzem Zeitlimit schnell
// uebersprungen.
function spvBtnScan(device, online, progress, context) {
    var ip = spvIp4(device);
    if (!ip) { progress.setText("Bitte eine IPv4-Adresse eintragen."); return; }

    var port = parseInt(device.getParameterByName("SPV_AsstPort").value);
    var slave = parseInt(device.getParameterByName("SPV_AsstSlaveId").value);
    var transport = parseInt(device.getParameterByName("SPV_AsstTransport").value);

    online.connect();
    try {
        spvClearTable(device);
        var state = {answered: 0, empty: 0, lastWasSet: false};
        var row = 0, cancelled = false;
        var total = spvScanBlocks.length;

        for (var b = 0; b < total && row < spvSlotNames.length; b++) {
            var from = spvScanBlocks[b];
            progress.setProgress(Math.round(b * 100 / total));
            progress.setText("Suche … 0x" + from.toString(16).toUpperCase()
                             + " (" + b + "/" + total + ", " + row + " gefunden)");
            // Neuer Bereich: die low-word-first-Erkennung darf nicht ueber die Luecke greifen.
            state.lastWasSet = false;
            // Zeitlimit > Firmware-Budget (4 s): sonst gibt die ETS auf, waehrend die Firmware
            // noch liest, und der naechste Blockbefehl traefe eine beschaeftigte Firmware.
            var next = spvReadBlock(device, online, progress, ip, port, slave, transport,
                                    from, 16, row, true, state, 5000);
            if (next < 0) { cancelled = true; break; }
            row = next;
        }

        device.getParameterByName("SPV_AsstRowCount").value = row;
        progress.setProgress(100);

        if (cancelled) {
            progress.setText("Abgebrochen. " + row + " Register wurden bereits eingetragen.");
        } else if (!state.answered) {
            progress.setText("Das Gerät hat auf keinen Bereich geantwortet. Stimmt der erkannte "
                             + "Transport? Belegt eine andere Abfrage (etwa Home Assistant) "
                             + "gerade das Gerät?");
        } else {
            var text = row + " belegte Register gefunden. ";
            if (row >= spvSlotNames.length) {
                text += "Die Tabelle ist voll. ";
            }
            text += "Wo der Wertebereich eindeutig war, ist eine Bedeutung vorgeschlagen — "
                  + "bitte prüfen, sie ist geraten. Alle übrigen Zeilen musst du selbst "
                  + "zuordnen.";
            progress.setText(text);
        }
    } finally {
        online.disconnect();
    }
}

function spvBtnRead(device, online, progress, context) {
    var ip = spvIp4(device);
    if (!ip) { progress.setText("Bitte eine IPv4-Adresse eintragen."); return; }

    var port = parseInt(device.getParameterByName("SPV_AsstPort").value);
    var slave = parseInt(device.getParameterByName("SPV_AsstSlaveId").value);
    var transport = parseInt(device.getParameterByName("SPV_AsstTransport").value);
    var start = parseInt(device.getParameterByName("SPV_AsstStartReg").value);
    var count = parseInt(device.getParameterByName("SPV_AsstReadCount").value);

    online.connect();
    try {
        progress.setText("Lese " + count + " Register ab 0x"
                         + start.toString(16).toUpperCase() + " …");
        spvClearTable(device);
        var state = {answered: 0, empty: 0, lastWasSet: false};
        var row = spvReadBlock(device, online, progress, ip, port, slave, transport,
                               start, count, 0, false, state, 12000);
        if (row <= 0) {
            device.getParameterByName("SPV_AsstRowCount").value = 0;
            progress.setText("Keine Register gelesen. Manche Logger lehnen Blöcke über 16 "
                             + "Register ab oder antworten auf unbelegte Adressen gar nicht.");
            return;
        }
        device.getParameterByName("SPV_AsstRowCount").value = row;
        progress.setProgress(100);
        progress.setText(row + " Register ab 0x" + start.toString(16).toUpperCase()
                         + " gelesen. Trage jetzt zu den Zeilen, die du brauchst, die "
                         + "Bedeutung und möglichst den am Gerät abgelesenen Wert ein.");
    } finally {
        online.disconnect();
    }
}

function spvBtnDeriveScale(device, online, progress, context) {
    // Ueber ALLE Zeilen laufen, nicht bis SPV_AsstRowCount: dieser Parameter wird nirgends
    // angezeigt, seine Persistenz ist unsicher. Zeilen ohne Bedeutung werden ohnehin
    // uebersprungen - die Grenze ist damit ueberfluessig.
    var rows = spvSlotNames.length;
    var scaleFactors = [1, 0.1, 0.01, 0.001, 10];
    var found = 0, unsure = 0;

    for (var r = 0; r < rows; r++) {
        var pad = r < 10 ? "0" : "";
        var meaning = parseInt(device.getParameterByName("SPV_AsstMeaning" + pad + r).value);
        if (!meaning) continue;

        var reference = parseFloat(device.getParameterByName("SPV_AsstRef" + pad + r).value);
        if (!reference) { unsure++; continue; }

        var raw = parseInt(device.getParameterByName("SPV_AsstRaw" + pad + r).value);
        // Vorzeichenbehaftet: ein grosser Rohwert bei negativem Sollwert kann nur ein
        // Zweierkomplement sein (60536 <-> -5000).
        var signed = raw > 32767 ? raw - 65536 : raw;
        // Datentyp-Kandidaten als flache Parallel-Arrays (Typcode / Wert), kein Array aus
        // Arrays - die ETS (Jint) verarbeitet verschachtelte Literale nicht zuverlaessig.
        // Typcodes: 0 = u16, 1 = s16 (Zweierkomplement), 2 = u32 low-word-first.
        var candType = [0, 1];
        var candValue = [raw, signed];
        // 32 Bit nur, wenn die naechste Zeile wirklich das Folgeregister ist: der Suchlauf
        // kann Luecken gelassen haben, dann waere ein fremdes Register das High-Word.
        if (r + 1 < rows) {
            var padNext = (r + 1) < 10 ? "0" : "";
            var regHere = parseInt(device.getParameterByName("SPV_AsstReg" + pad + r).value);
            var regNext = parseInt(device.getParameterByName("SPV_AsstReg" + padNext + (r + 1)).value);
            if (regNext === regHere + 1) {
                var next = parseInt(device.getParameterByName("SPV_AsstRaw" + padNext + (r + 1)).value);
                candType.push(2);
                candValue.push(raw + next * 65536);
            }
        }
        var bestType = -1, bestScale = -1, bestOffset = 0;
        for (var c = 0; c < candType.length; c++) {
            for (var s = 0; s < scaleFactors.length; s++) {
                var offset = candValue[c] * scaleFactors[s] - reference;
                var rounded = Math.round(offset);
                if (Math.abs(offset - rounded) > 0.0005) continue;
                if (rounded < -128 || rounded > 127) continue;
                // Bei mehreren Treffern gewinnt der kleinere Offset: ein Geraet, das ohne
                // Offset auskommt, ist wahrscheinlicher als eines mit krummem Versatz.
                if (bestType < 0 || Math.abs(rounded) < Math.abs(bestOffset)) {
                    bestType = candType[c]; bestScale = s; bestOffset = rounded;
                }
            }
        }

        if (bestType < 0) { unsure++; continue; }
        device.getParameterByName("SPV_AsstType" + pad + r).value = bestType;
        device.getParameterByName("SPV_AsstScale" + pad + r).value = bestScale;
        device.getParameterByName("SPV_AsstOffset" + pad + r).value = bestOffset;
        found++;
    }

    // Kurz halten (Feld 48 Byte). Die ausfuehrliche Erklaerung steht im Hilfetext.
    var text = found + " bestimmt";
    if (unsure) text += ", " + unsure + " ohne Ergebnis";
    text += ".";
    // Offline-Knopf: Rueckmeldung ins Anzeige-Parameter, nicht ueber progress.
    device.getParameterByName("SPV_AsstResultText").value = text;
}

function spvBtnApply(device, online, progress, context) {
    var channel = parseInt(device.getParameterByName("SPV_AsstTargetChannel").value);
    var prefix = "SPV_CH" + channel;
    // Alle Zeilen pruefen (siehe Kommentar in spvBtnDeriveScale); ohne Bedeutung wird
    // uebersprungen.
    var rows = spvSlotNames.length;

    var i;
    for (i = 0; i < spvSlotNames.length; i++) {
        device.getParameterByName(prefix + "En" + spvSlotNames[i]).value = 0;
        device.getParameterByName(prefix + "Reg" + spvSlotNames[i]).value = 0;
        device.getParameterByName(prefix + "Type" + spvSlotNames[i]).value = 0;
        device.getParameterByName(prefix + "Scale" + spvSlotNames[i]).value = 0;
        device.getParameterByName(prefix + "Offset" + spvSlotNames[i]).value = 0;
    }

    var taken = 0;
    for (var r = 0; r < rows; r++) {
        var pad = r < 10 ? "0" : "";
        var meaning = parseInt(device.getParameterByName("SPV_AsstMeaning" + pad + r).value);
        if (!meaning) continue;

        var slot = spvSlotNames[meaning - 1]; // 0 ist "nicht zuordnen"
        // Die Adresse steht in der Zeile, nicht in der Zeilennummer: der Suchlauf hat leere
        // Register uebersprungen.
        device.getParameterByName(prefix + "Reg" + slot).value =
            parseInt(device.getParameterByName("SPV_AsstReg" + pad + r).value);
        device.getParameterByName(prefix + "Type" + slot).value =
            parseInt(device.getParameterByName("SPV_AsstType" + pad + r).value);
        device.getParameterByName(prefix + "Scale" + slot).value =
            parseInt(device.getParameterByName("SPV_AsstScale" + pad + r).value);
        device.getParameterByName(prefix + "Offset" + slot).value =
            parseInt(device.getParameterByName("SPV_AsstOffset" + pad + r).value);
        device.getParameterByName(prefix + "En" + slot).value = 1;
        taken++;
    }

    // Verbindungsdaten mitnehmen: gemessen wurde ja genau dieses Geraet.
    device.getParameterByName(prefix + "LoggerIp").value =
        device.getParameterByName("SPV_AsstIp").value;
    device.getParameterByName(prefix + "LoggerPort").value =
        parseInt(device.getParameterByName("SPV_AsstPort").value);
    device.getParameterByName(prefix + "SlaveId").value =
        parseInt(device.getParameterByName("SPV_AsstSlaveId").value);
    device.getParameterByName(prefix + "Transport").value =
        parseInt(device.getParameterByName("SPV_AsstTransport").value);

    var serial = device.getParameterByName("SPV_AsstSerial").value;
    if (serial) {
        device.getParameterByName(prefix + "SerialMode").value = 1;
        device.getParameterByName(prefix + "LoggerSerialText").value = serial;
    }

    // Das Profil wird bewusst geleert: die Tabelle stammt jetzt aus der Messung, ein
    // stehengebliebener Profilname wuerde etwas anderes behaupten.
    device.getParameterByName(prefix + "Profile").value = 0;
    // Rueckmeldung an beiden Orten, kurz halten (Felder je 48 Byte).
    device.getParameterByName(prefix + "ProfileMsg").value =
        taken + " Messwerte (Assistent).";
    device.getParameterByName("SPV_AsstResultText").value =
        taken + " Messwerte in Gerät " + channel + " übernommen.";
}
