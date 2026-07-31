// ERZEUGT von tools/gen_profiles.py - nicht von Hand bearbeiten.
// Logik: tools/script.template.js   Daten: tools/profiles.py

// Namen aller Slots in Katalogreihenfolge. Der Index ist zugleich die Zeilennummer der
// Messwerttabelle; die Parameter heissen SPV_CH<Kanal><Feld><Slotname>.
var spvSlotNames = [
    "Power", "ApparentPower", "GridVoltage", "GridVoltageL2", "GridVoltageL3", "GridCurrent",
    "GridFrequency", "OperatingState", "Today", "Total", "Month", "Year",
    "Today1", "Today2", "Total1", "Total2", "Pv1Voltage", "Pv1Current",
    "Pv1Power", "Pv1Today", "Pv2Voltage", "Pv2Current", "Pv2Power", "Pv2Today",
    "Pv3Voltage", "Pv3Current", "Pv3Power", "Pv3Today", "Pv4Voltage", "Pv4Current",
    "Pv4Power", "Pv4Today", "Soc", "Soh", "BattVoltage", "BattCurrent",
    "BattPower", "BattTemperature", "CycleTimes", "RemainingCapacity", "TodayCharge", "TodayDischarge",
    "TotalCharge", "TotalDischarge", "CellVoltageMax", "CellVoltageMin", "CellTempMax", "CellTempMin",
    "HousePower", "ImportPower", "ExportPower", "ImportTotal", "ExportTotal", "HouseToday",
    "ImportToday", "ExportToday", "Temperature", "HeatsinkTemp", "ErrorCode", "OperatingHours",
    "Spare1", "Spare2", "Spare3", "Spare4"
];

// Anzeigetexte, nur fuer Meldungen an den Anwender.
var spvSlotLabels = [
    "Wirkleistung", "Scheinleistung", "Netzspannung", "Netzspannung L2",
    "Netzspannung L3", "Netzstrom", "Netzfrequenz", "Betriebszustand",
    "Tagesertrag", "Gesamtertrag", "Monatsertrag", "Jahresertrag",
    "Tagesertrag String 1", "Tagesertrag String 2", "Gesamtertrag String 1", "Gesamtertrag String 2",
    "PV1 Spannung", "PV1 Strom", "PV1 Leistung", "PV1 Tagesertrag",
    "PV2 Spannung", "PV2 Strom", "PV2 Leistung", "PV2 Tagesertrag",
    "PV3 Spannung", "PV3 Strom", "PV3 Leistung", "PV3 Tagesertrag",
    "PV4 Spannung", "PV4 Strom", "PV4 Leistung", "PV4 Tagesertrag",
    "Batterie Ladezustand", "Batterie Alterungszustand", "Batteriespannung", "Batteriestrom",
    "Batterieleistung", "Batterietemperatur", "Ladezyklen", "Restkapazität",
    "Heute geladen", "Heute entladen", "Gesamt geladen", "Gesamt entladen",
    "Zellspannung maximal", "Zellspannung minimal", "Zelltemperatur maximal", "Zelltemperatur minimal",
    "Hausverbrauch", "Netzbezug Leistung", "Einspeisung Leistung", "Netzbezug gesamt",
    "Einspeisung gesamt", "Hausverbrauch heute", "Netzbezug heute", "Einspeisung heute",
    "Gerätetemperatur", "Kühlkörpertemperatur", "Fehlercode", "Betriebsstunden",
    "Freier Wert 1", "Freier Wert 2", "Freier Wert 3", "Freier Wert 4"
];

// Profildatenbank. Reihenfolge = Werte der Auswahlliste, 0 = ohne Profil (eigene Tabelle).
//
// "rows" ist bewusst ein FLACHES Zahlenarray, je Messwert fuenf Werte hintereinander:
//   Slot-Index, Register, Datentyp, Skalierung, Offset
// Kein Array aus Arrays: die ES3-nahe ETS-Skript-Engine verarbeitet verschachtelte
// Literale nicht zuverlaessig. Alle erprobten OpenKNX-Skripte (PresenceModule, IPCamera)
// nutzen ebenfalls nur flache Arrays.
var SPV_ROW = 5;
var spvProfiles = [
    null,
    {
        name: "Deye SUN-M80G3",
        transport: 0,
        port: 8899,
        verified: true,
        rows: [0, 86, 2, 1, 0, 8, 60, 0, 1, 0, 9, 63, 2, 1, 0, 12, 65, 0, 1, 0, 13, 66, 0, 1, 0, 14, 69, 2, 1, 0, 15, 71, 2, 1, 0, 2, 73, 0, 1, 0, 5, 76, 0, 1, 0, 6, 79, 0, 2, 0, 56, 90, 0, 2, 10, 16, 109, 0, 1, 0, 17, 110, 0, 1, 0, 18, 0, 4, 0, 0, 20, 111, 0, 1, 0, 21, 112, 0, 1, 0, 22, 0, 4, 0, 0]
    },
    {
        name: "Pylontech Force H1/H2/H3",
        transport: 1,
        port: 8899,
        verified: true,
        rows: [32, 5127, 0, 0, 0, 33, 5152, 0, 0, 0, 34, 5123, 0, 1, 0, 35, 5125, 1, 2, 0, 36, 0, 4, 0, 0, 37, 5126, 1, 1, 0, 38, 5128, 0, 0, 0, 39, 5154, 2, 3, 0, 40, 5160, 2, 3, 0, 41, 5162, 2, 3, 0, 42, 5164, 2, 0, 0, 43, 5166, 2, 0, 0]
    },
    {
        name: "Afore 2MPPT",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [8, 1000, 0, 1, 0, 16, 555, 0, 1, 0, 17, 556, 0, 2, 0, 18, 557, 0, 0, 0, 20, 558, 0, 1, 0, 21, 559, 0, 2, 0, 22, 556, 0, 0, 0, 56, 2514, 1, 1, 0, 57, 2515, 1, 1, 0]
    },
    {
        name: "Afore BNTXXXKTL-2MPPT",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [2, 1, 0, 1, 0, 3, 2, 0, 1, 0, 4, 3, 0, 1, 0, 5, 4, 0, 1, 0, 6, 13, 0, 1, 0, 16, 7, 0, 1, 0, 17, 8, 0, 1, 0, 20, 9, 0, 1, 0, 21, 10, 0, 1, 0, 24, 11, 0, 1, 0, 25, 12, 0, 1, 0, 56, 15, 0, 1, 0, 57, 14, 0, 1, 0]
    },
    {
        name: "Afore HYBRID",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [2, 507, 0, 1, 0, 3, 508, 0, 1, 0, 4, 509, 0, 1, 0, 16, 555, 0, 1, 0, 17, 556, 0, 2, 0, 18, 557, 0, 0, 0, 20, 558, 0, 1, 0, 21, 559, 0, 2, 0, 22, 560, 0, 0, 0, 32, 2002, 0, 0, 0, 33, 2003, 0, 0, 0, 34, 2004, 0, 1, 0, 35, 2005, 1, 2, 0, 36, 2008, 1, 0, 0, 37, 2001, 0, 1, 0, 40, 2009, 0, 1, 0, 41, 2010, 0, 1, 0, 53, 1004, 0, 1, 0, 54, 1003, 0, 1, 0, 55, 1002, 0, 1, 0, 56, 2515, 0, 1, 0, 57, 2514, 0, 1, 0]
    },
    {
        name: "ANENJI Hybrid",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [0, 208, 1, 0, 0, 32, 229, 0, 0, 0]
    },
    {
        name: "Astro-Energy TM-L series",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [0, 15, 1, 1, 0, 16, 0, 0, 1, 0, 17, 5, 0, 2, 0, 20, 1, 0, 1, 0, 21, 6, 0, 2, 0, 24, 2, 0, 1, 0, 25, 7, 0, 2, 0, 28, 3, 0, 1, 0, 29, 8, 0, 2, 0, 56, 22, 0, 2, 0]
    },
    {
        name: "CHINT CPS-SCETL",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [2, 4890, 0, 1, 0, 3, 4891, 0, 1, 0, 4, 4892, 0, 1, 0, 6, 4920, 0, 2, 0, 16, 4112, 0, 1, 0, 17, 4113, 0, 2, 0, 20, 4116, 0, 1, 0, 21, 4117, 0, 2, 0, 24, 4120, 0, 1, 0, 25, 4121, 0, 2, 0, 28, 4158, 0, 1, 0, 29, 4159, 0, 2, 0, 32, 8192, 0, 0, 0, 34, 8198, 0, 1, 0, 37, 8193, 1, 0, 0, 57, 4124, 1, 0, 0]
    },
    {
        name: "Deye SG0*LP1",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [0, 175, 1, 0, 0, 2, 150, 0, 1, 0, 3, 151, 0, 1, 0, 5, 160, 1, 2, 0, 6, 79, 0, 2, 0, 8, 108, 0, 1, 0, 9, 96, 2, 1, 0, 16, 109, 0, 1, 0, 17, 110, 0, 1, 0, 18, 186, 0, 0, 0, 20, 111, 0, 1, 0, 21, 112, 0, 1, 0, 22, 187, 0, 0, 0, 24, 113, 0, 1, 0, 25, 114, 0, 1, 0, 26, 188, 0, 0, 0, 28, 115, 0, 1, 0, 29, 116, 0, 1, 0, 30, 189, 0, 0, 0, 32, 184, 0, 0, 0, 34, 183, 0, 2, 0, 36, 190, 1, 0, 0, 37, 182, 0, 1, 100, 39, 204, 0, 0, 0, 40, 70, 0, 1, 0, 41, 71, 0, 1, 0, 42, 72, 2, 1, 0, 43, 74, 2, 1, 0, 52, 81, 2, 1, 0, 53, 84, 0, 1, 0, 54, 76, 0, 1, 0, 55, 77, 0, 1, 0, 56, 91, 1, 1, 100, 57, 90, 1, 1, 100]
    },
    {
        name: "Deye SG0*LP3 / SG0*HP3",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [2, 598, 0, 1, 0, 3, 599, 0, 1, 0, 4, 600, 0, 1, 0, 6, 609, 0, 2, 0, 16, 676, 0, 1, 0, 17, 677, 0, 1, 0, 20, 678, 0, 1, 0, 21, 679, 0, 1, 0, 24, 680, 0, 1, 0, 25, 681, 0, 1, 0, 28, 682, 0, 1, 0, 29, 683, 0, 1, 0, 32, 588, 0, 0, 0, 37, 586, 0, 1, 100, 39, 102, 0, 0, 0, 56, 541, 1, 1, 100, 57, 540, 1, 1, 100]
    },
    {
        name: "Deye G0*",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [0, 86, 2, 1, 0, 2, 73, 0, 1, 0, 3, 74, 0, 1, 0, 4, 75, 0, 1, 0, 5, 76, 1, 1, 0, 6, 79, 0, 2, 0, 8, 60, 0, 1, 0, 9, 63, 2, 1, 0, 16, 109, 0, 1, 0, 17, 110, 0, 1, 0, 20, 111, 0, 1, 0, 21, 112, 0, 1, 0, 24, 113, 0, 1, 0, 25, 114, 0, 1, 0, 28, 115, 0, 1, 0, 29, 116, 0, 1, 0, 52, 206, 2, 1, 0, 53, 200, 0, 2, 0, 54, 208, 0, 2, 0, 55, 205, 0, 2, 0, 56, 91, 1, 1, 100, 57, 90, 1, 1, 100]
    },
    {
        name: "Hinen H5000",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [2, 29, 0, 1, 0, 5, 30, 0, 1, 0, 6, 56, 1, 2, 0, 16, 3, 0, 1, 0, 17, 4, 0, 1, 0, 20, 8, 0, 1, 0, 21, 9, 0, 1, 0, 24, 13, 0, 1, 0, 25, 14, 0, 1, 0, 28, 18, 0, 1, 0, 29, 19, 0, 1, 0, 32, 128, 0, 0, 0, 34, 127, 0, 1, 0, 37, 136, 0, 1, 0, 56, 132, 1, 1, 0]
    },
    {
        name: "Invt XD-TL",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [0, 12313, 0, 1, 0, 2, 14353, 0, 1, 0, 3, 14354, 0, 1, 0, 4, 14355, 0, 1, 0, 5, 14359, 1, 1, 0, 6, 14362, 0, 2, 0, 8, 14383, 0, 1, 0, 16, 14390, 0, 1, 0, 17, 14391, 0, 2, 0, 18, 15105, 0, 1, 0, 20, 14392, 0, 1, 0, 21, 14393, 0, 2, 0, 22, 15106, 0, 1, 0, 24, 14394, 0, 1, 0, 25, 14395, 0, 2, 0, 26, 15107, 0, 1, 0, 28, 14396, 0, 1, 0, 29, 14397, 0, 2, 0, 30, 15108, 0, 1, 0, 32, 14651, 0, 0, 0, 33, 14652, 0, 0, 0, 34, 14602, 0, 1, 0, 37, 14669, 0, 1, 0, 40, 14491, 0, 1, 0, 41, 14494, 0, 1, 0, 54, 14488, 0, 1, 0, 55, 14485, 0, 1, 0, 56, 14374, 1, 1, 0]
    },
    {
        name: "KSTAR Hybrid Inverter",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [2, 3097, 0, 1, 0, 3, 3101, 0, 1, 0, 4, 3105, 0, 1, 0, 5, 3099, 1, 3, 0, 8, 3036, 0, 1, 0, 16, 3000, 0, 1, 0, 17, 3012, 1, 2, 0, 18, 3024, 1, 0, 0, 20, 3001, 0, 1, 0, 21, 3013, 1, 2, 0, 22, 3025, 1, 0, 0, 32, 3066, 0, 1, 0, 33, 3075, 0, 1, 0, 34, 3063, 0, 2, 0, 35, 3064, 1, 1, 0, 36, 3065, 1, 0, 0, 37, 3067, 1, 1, 0, 40, 3301, 0, 1, 0, 41, 3294, 0, 1, 0, 48, 3144, 0, 0, 0, 53, 3147, 0, 1, 0, 54, 3109, 0, 1, 0, 55, 3116, 0, 1, 0]
    },
    {
        name: "Maxge STRING",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [16, 518, 0, 1, 0, 17, 519, 0, 1, 0, 20, 525, 0, 1, 0, 21, 526, 0, 1, 0]
    },
    {
        name: "Megarevo R-3H",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [2, 12560, 0, 1, 0, 3, 12563, 0, 1, 0, 4, 12566, 0, 1, 0, 5, 12561, 1, 1, 0, 6, 12569, 1, 2, 0, 8, 12627, 2, 3, 0, 9, 12645, 2, 3, 0, 16, 12592, 0, 1, 0, 17, 12593, 0, 1, 0, 18, 12594, 0, 0, 0, 20, 12595, 0, 1, 0, 21, 12596, 0, 1, 0, 22, 12597, 0, 0, 0, 24, 12598, 0, 1, 0, 25, 12599, 0, 1, 0, 26, 12600, 0, 0, 0, 28, 12601, 0, 1, 0, 29, 12602, 0, 1, 0, 30, 12603, 0, 0, 0, 32, 12613, 0, 1, 0, 34, 12608, 0, 1, 0, 35, 12609, 1, 1, 0, 36, 12618, 1, 0, 0, 37, 12614, 1, 1, 0, 40, 12653, 2, 3, 0, 41, 12655, 2, 3, 0, 42, 12671, 2, 3, 0, 43, 12673, 2, 3, 0, 51, 12669, 2, 3, 0, 52, 12647, 2, 3, 0, 53, 12631, 2, 3, 0, 54, 12651, 2, 3, 0, 55, 12629, 2, 3, 0, 56, 12570, 1, 0, 0, 57, 12626, 1, 0, 0]
    },
    {
        name: "Renon IFL",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [2, 12560, 0, 1, 0, 3, 12563, 0, 1, 0, 4, 12566, 0, 1, 0, 5, 12561, 0, 1, 0, 6, 12569, 0, 2, 0, 8, 12627, 2, 3, 0, 9, 12645, 2, 3, 0, 16, 5776, 0, 1, 0, 17, 5777, 0, 1, 0, 18, 8831, 0, 0, 0, 20, 5778, 0, 1, 0, 21, 5779, 0, 1, 0, 22, 8833, 0, 0, 0, 32, 12613, 0, 1, 0, 34, 12608, 0, 1, 0, 35, 12609, 1, 2, 0, 36, 12618, 1, 0, 0, 37, 12614, 1, 1, 0, 56, 5798, 1, 1, 0]
    },
    {
        name: "Sofar G3 (sofar_g3)",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [0, 1157, 1, 4, 0, 16, 1412, 0, 1, 0, 17, 1413, 0, 2, 0, 18, 1414, 0, 4, 0, 20, 1415, 0, 1, 0, 21, 1416, 0, 2, 0, 22, 1417, 0, 4, 0, 56, 1667, 1, 1, 0]
    },
    {
        name: "Sofar G3 (sofar_g3hyd)",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [6, 1156, 0, 2, 0, 16, 1412, 0, 1, 0, 17, 1413, 0, 2, 0, 18, 1414, 0, 4, 0, 20, 1415, 0, 1, 0, 21, 1416, 0, 2, 0, 22, 1417, 0, 4, 0, 24, 1418, 0, 1, 0, 25, 1419, 0, 2, 0, 26, 1420, 0, 4, 0, 28, 1421, 0, 1, 0, 29, 1422, 0, 2, 0, 30, 1423, 0, 4, 0, 32, 1544, 0, 0, 0, 33, 1545, 0, 0, 0, 34, 1540, 0, 1, 0, 35, 1541, 1, 2, 0, 37, 1543, 1, 0, 0]
    },
    {
        name: "Sofar HYBRID",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [0, 533, 0, 4, 0, 2, 518, 0, 1, 0, 3, 520, 0, 1, 0, 4, 522, 0, 1, 0, 5, 519, 1, 2, 0, 6, 524, 0, 2, 0, 8, 536, 0, 2, 0, 32, 528, 0, 0, 0, 33, 567, 0, 0, 0, 35, 527, 1, 2, 0, 36, 525, 1, 4, 0, 37, 529, 0, 0, 0, 39, 4273, 0, 0, 0, 40, 548, 0, 2, 0, 41, 549, 0, 2, 0, 48, 531, 0, 4, 0, 53, 539, 0, 2, 0, 54, 538, 0, 2, 0, 55, 537, 0, 2, 0, 56, 568, 1, 0, 0, 57, 569, 1, 0, 0]
    },
    {
        name: "Sofar STRING",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [0, 12, 0, 4, 0, 8, 25, 0, 2, 0, 16, 6, 0, 1, 0, 17, 7, 0, 2, 0, 18, 10, 0, 4, 0, 20, 8, 0, 1, 0, 21, 9, 0, 2, 0, 22, 11, 0, 4, 0]
    },
    {
        name: "Solis 1P-5G",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [8, 3014, 0, 1, 0, 16, 3021, 0, 1, 0, 17, 3022, 0, 1, 0, 20, 3023, 0, 1, 0, 21, 3024, 0, 1, 0]
    },
    {
        name: "Solis 3P-4G",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [8, 3014, 0, 1, 0, 16, 3021, 0, 1, 0, 17, 3022, 0, 1, 0, 20, 3023, 0, 1, 0, 21, 3024, 0, 1, 0, 24, 3025, 0, 1, 0, 25, 3026, 0, 1, 0]
    },
    {
        name: "Solis 3P-5G",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [8, 33035, 0, 1, 0, 16, 33049, 0, 1, 0, 17, 33050, 0, 1, 0, 20, 33051, 0, 1, 0, 21, 33052, 0, 1, 0, 32, 33139, 0, 0, 0, 33, 33140, 0, 0, 0, 34, 33133, 0, 1, 0, 35, 33134, 1, 1, 0, 40, 33163, 0, 1, 0, 41, 33167, 0, 1, 0, 53, 33179, 0, 1, 0, 54, 33171, 0, 1, 0, 55, 33175, 0, 1, 0]
    },
    {
        name: "Solis HYBRID",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [8, 33035, 0, 1, 0, 16, 33049, 0, 1, 0, 17, 33050, 0, 1, 0, 20, 33051, 0, 1, 0, 21, 33052, 0, 1, 0, 32, 33139, 0, 0, 0, 33, 33140, 0, 0, 0, 34, 33133, 0, 1, 0, 35, 33134, 1, 1, 0, 40, 33163, 0, 1, 0, 41, 33167, 0, 1, 0, 53, 33179, 0, 1, 0, 54, 33171, 0, 1, 0, 55, 33175, 0, 1, 0, 56, 33093, 1, 1, 0]
    },
    {
        name: "Solis S6-GR1P",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [8, 3014, 0, 1, 0, 16, 3021, 0, 1, 0, 17, 3022, 0, 1, 0, 20, 3023, 0, 1, 0, 21, 3024, 0, 1, 0]
    },
    {
        name: "SRNE ASF",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [0, 266, 0, 0, 0, 2, 531, 0, 1, 0, 3, 554, 0, 1, 0, 4, 555, 0, 1, 0, 5, 532, 0, 1, 0, 6, 533, 0, 2, 0, 8, 61487, 0, 1, 0, 9, 61496, 2, 1, 0, 16, 263, 0, 1, 0, 17, 264, 0, 1, 0, 18, 265, 0, 0, 0, 20, 271, 0, 1, 0, 21, 272, 0, 1, 0, 22, 273, 0, 0, 0, 32, 256, 0, 0, 0, 34, 257, 0, 1, 0, 35, 258, 1, 1, 0, 37, 259, 1, 1, 0, 51, 61512, 2, 1, 0, 52, 61490, 2, 1, 0, 53, 61488, 0, 1, 0, 54, 61501, 0, 1, 0, 55, 61484, 0, 1, 0, 56, 545, 0, 1, 0, 57, 544, 0, 1, 0]
    },
    {
        name: "Swatten SIH-TH",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [0, 4067, 2, 0, 0, 2, 4069, 0, 1, 0, 3, 4070, 0, 1, 0, 4, 4071, 0, 1, 0, 5, 4072, 0, 2, 0, 6, 4198, 1, 2, 0, 8, 10002, 0, 1, 0, 9, 10003, 2, 1, 0, 16, 4061, 1, 1, 0, 17, 4062, 1, 1, 0, 20, 4063, 1, 1, 0, 21, 4064, 1, 1, 0, 32, 10023, 1, 0, 0, 33, 10024, 1, 0, 0, 34, 10020, 0, 1, 0, 35, 10021, 1, 1, 0, 36, 10022, 1, 0, 0, 37, 10025, 1, 1, 0, 40, 10040, 0, 1, 0, 41, 10026, 0, 1, 0, 42, 10041, 2, 1, 0, 43, 10027, 2, 1, 0, 48, 10008, 1, 0, 0, 51, 10037, 2, 1, 0, 52, 10046, 2, 1, 0, 54, 10036, 0, 1, 0, 55, 10045, 0, 1, 0, 56, 4058, 1, 1, 0]
    },
    {
        name: "Tsun TSOL-MS",
        transport: 0,
        port: 8899,
        verified: false,
        rows: [2, 12297, 0, 1, 0, 5, 12298, 1, 2, 0, 6, 12299, 0, 2, 0, 8, 12316, 0, 2, 0, 9, 12318, 0, 2, 0, 12, 12319, 0, 1, 0, 13, 12322, 0, 2, 0, 16, 12304, 0, 1, 0, 17, 12305, 0, 2, 0, 18, 12306, 0, 1, 0, 20, 12307, 0, 1, 0, 21, 12308, 0, 2, 0, 22, 12309, 0, 1, 0, 56, 12300, 0, 0, 40]
    }
];

// Suchplan fuer "Belegte Register suchen": 16er-Blockadressen ueber alle bekannten
// Registerbereiche aller Profile. So findet der Suchlauf die Werte jedes Geraets, ohne
// dass der Anwender die Startadresse kennen muss.
var spvScanBlocks = [0x0000, 0x0010, 0x0030, 0x0040, 0x0050, 0x0060, 0x0070, 0x0080, 0x0090, 0x00A0, 0x00B0, 0x00C0, 0x00D0, 0x00E0, 0x0100, 0x0110, 0x01F0, 0x0200, 0x0210, 0x0220, 0x0230, 0x0240, 0x0250, 0x0260, 0x02A0, 0x03E0, 0x0480, 0x0580, 0x0600, 0x0680, 0x07D0, 0x09D0, 0x0BB0, 0x0BC0, 0x0BD0, 0x0BF0, 0x0C00, 0x0C10, 0x0C20, 0x0C40, 0x0CD0, 0x0CE0, 0x0FD0, 0x0FE0, 0x1010, 0x1030, 0x1060, 0x10B0, 0x1310, 0x1330, 0x1400, 0x1420, 0x1690, 0x16A0, 0x2000, 0x2270, 0x2280, 0x2710, 0x2720, 0x2730, 0x3000, 0x3010, 0x3020, 0x3110, 0x3130, 0x3140, 0x3150, 0x3160, 0x3170, 0x3180, 0x3810, 0x3820, 0x3830, 0x3890, 0x3900, 0x3930, 0x3940, 0x3B00, 0x8100, 0x8110, 0x8140, 0x8160, 0x8170, 0x8180, 0x8190, 0xF020, 0xF030, 0xF040];

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
