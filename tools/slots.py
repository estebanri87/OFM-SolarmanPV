# -*- coding: utf-8 -*-
"""Slot-Katalog: die feste Bedeutung jedes Messwert-Slots.

ACHTUNG - das ist eine Schnittstelle, keine Liste.
Die Reihenfolge bestimmt die KO-Nummern. Ein Eintrag darf ergaenzt (hinten angehaengt) oder
umbenannt werden; ihn zu verschieben oder zu loeschen verschiebt die KO-Nummern aller
folgenden Slots und macht bestehende ETS-Projekte kaputt. Frei gewordene Plaetze bleiben
als Platzhalter stehen.

Warum feste Bedeutungen? Ein frei waehlbarer KO-Name je Zeile ist in der ETS nicht
darstellbar: TextParameterRefId setzt bei Aufzaehlungs-Parametern den Rohwert ein, nicht den
Anzeigetext (am Gerät gemessen, siehe doc/Assistent-Umbau.md). Ein Textfeld je Zeile waere
bezahlbar nur mit ~24 Slots.

Felder: (Name, Anzeigetext, DPT, ObjectSize)
Der Name landet in den knxprod-Makros (KoSPV_CH<Name>), der Anzeigetext im KO.
"""

# Erzeugung und Netz
GRID = [
    ("Power",            "Wirkleistung",            "DPST-14-56", "4 Bytes"),
    ("ApparentPower",    "Scheinleistung",          "DPST-14-56", "4 Bytes"),
    ("GridVoltage",      "Netzspannung",            "DPST-14-27", "4 Bytes"),
    ("GridVoltageL2",    "Netzspannung L2",         "DPST-14-27", "4 Bytes"),
    ("GridVoltageL3",    "Netzspannung L3",         "DPST-14-27", "4 Bytes"),
    ("GridCurrent",      "Netzstrom",               "DPST-14-19", "4 Bytes"),
    ("GridFrequency",    "Netzfrequenz",            "DPST-14-33", "4 Bytes"),
    ("OperatingState",   "Betriebszustand",         "DPST-5-10",  "1 Byte"),
]

# Erträge
YIELD = [
    ("Today",            "Tagesertrag",             "DPST-13-10", "4 Bytes"),
    ("Total",            "Gesamtertrag",            "DPST-13-10", "4 Bytes"),
    ("Month",            "Monatsertrag",            "DPST-13-10", "4 Bytes"),
    ("Year",             "Jahresertrag",            "DPST-13-10", "4 Bytes"),
    ("Today1",           "Tagesertrag String 1",    "DPST-13-10", "4 Bytes"),
    ("Today2",           "Tagesertrag String 2",    "DPST-13-10", "4 Bytes"),
    ("Total1",           "Gesamtertrag String 1",   "DPST-13-10", "4 Bytes"),
    ("Total2",           "Gesamtertrag String 2",   "DPST-13-10", "4 Bytes"),
]

# PV-Strings 1-4, je Spannung/Strom/Leistung/Tagesertrag
PV = []
for _s in (1, 2, 3, 4):
    PV += [
        ("Pv%dVoltage" % _s, "PV%d Spannung"    % _s, "DPST-14-27", "4 Bytes"),
        ("Pv%dCurrent" % _s, "PV%d Strom"       % _s, "DPST-14-19", "4 Bytes"),
        ("Pv%dPower"   % _s, "PV%d Leistung"    % _s, "DPST-14-56", "4 Bytes"),
        ("Pv%dToday"   % _s, "PV%d Tagesertrag" % _s, "DPST-13-10", "4 Bytes"),
    ]

# Batterie
BATTERY = [
    ("Soc",              "Batterie Ladezustand",    "DPST-5-1",   "1 Byte"),
    ("Soh",              "Batterie Alterungszustand", "DPST-5-1", "1 Byte"),
    ("BattVoltage",      "Batteriespannung",        "DPST-14-27", "4 Bytes"),
    ("BattCurrent",      "Batteriestrom",           "DPST-14-19", "4 Bytes"),
    ("BattPower",        "Batterieleistung",        "DPST-14-56", "4 Bytes"),
    ("BattTemperature",  "Batterietemperatur",      "DPST-9-1",   "2 Bytes"),
    ("CycleTimes",       "Ladezyklen",              "DPST-7-1",   "2 Bytes"),
    ("RemainingCapacity", "Restkapazität",         "DPST-13-10", "4 Bytes"),
    ("TodayCharge",      "Heute geladen",           "DPST-13-10", "4 Bytes"),
    ("TodayDischarge",   "Heute entladen",          "DPST-13-10", "4 Bytes"),
    ("TotalCharge",      "Gesamt geladen",          "DPST-13-10", "4 Bytes"),
    ("TotalDischarge",   "Gesamt entladen",         "DPST-13-10", "4 Bytes"),
    ("CellVoltageMax",   "Zellspannung maximal",    "DPST-14-27", "4 Bytes"),
    ("CellVoltageMin",   "Zellspannung minimal",    "DPST-14-27", "4 Bytes"),
    ("CellTempMax",      "Zelltemperatur maximal",  "DPST-9-1",   "2 Bytes"),
    ("CellTempMin",      "Zelltemperatur minimal",  "DPST-9-1",   "2 Bytes"),
]

# Haus und Netzübergabe
HOUSE = [
    ("HousePower",       "Hausverbrauch",           "DPST-14-56", "4 Bytes"),
    ("ImportPower",      "Netzbezug Leistung",      "DPST-14-56", "4 Bytes"),
    ("ExportPower",      "Einspeisung Leistung",    "DPST-14-56", "4 Bytes"),
    ("ImportTotal",      "Netzbezug gesamt",        "DPST-13-10", "4 Bytes"),
    ("ExportTotal",      "Einspeisung gesamt",      "DPST-13-10", "4 Bytes"),
    ("HouseToday",       "Hausverbrauch heute",     "DPST-13-10", "4 Bytes"),
    ("ImportToday",      "Netzbezug heute",         "DPST-13-10", "4 Bytes"),
    ("ExportToday",      "Einspeisung heute",       "DPST-13-10", "4 Bytes"),
]

# Diagnose
DIAG = [
    ("Temperature",      "Gerätetemperatur",       "DPST-9-1",   "2 Bytes"),
    ("HeatsinkTemp",     "Kühlkörpertemperatur",  "DPST-9-1",   "2 Bytes"),
    ("ErrorCode",        "Fehlercode",              "DPST-7-1",   "2 Bytes"),
    ("OperatingHours",   "Betriebsstunden",         "DPST-7-7",   "2 Bytes"),
]

# Reserve fuer Geräte, die der Katalog nicht abdeckt. Generisch als Gleitkomma-Wert.
SPARE = [
    ("Spare%d" % _n, "Freier Wert %d" % _n, "DPST-14-56", "4 Bytes")
    for _n in (1, 2, 3, 4)
]

# Gruppen erscheinen als Überschriften auf der Geräteseite.
GROUPS = [
    ("Erzeugung und Netz", GRID),
    ("Erträge",           YIELD),
    ("PV-Strings",         PV),
    ("Batterie",           BATTERY),
    ("Haus und Netzübergabe", HOUSE),
    ("Diagnose",           DIAG),
    ("Reserve",            SPARE),
]

SLOTS = [s for _, group in GROUPS for s in group]

assert len(SLOTS) == 64, "Slot-Katalog muss genau 64 Eintraege haben, hat %d" % len(SLOTS)
assert len(set(n for n, _, _, _ in SLOTS)) == 64, "Slot-Namen muessen eindeutig sein"

if __name__ == "__main__":
    i = 0
    for title, group in GROUPS:
        print("%s (%d)" % (title, len(group)))
        for name, label, dpt, size in group:
            i += 1
            print("  %2d  %-20s %-26s %-12s %s" % (i, name, label, dpt, size))
