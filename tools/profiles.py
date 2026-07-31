# -*- coding: utf-8 -*-
"""Geraeteprofile: Vorlagen, die das ETS-Skript in die Messwerttabelle eines Geraets schreibt.

Ein Profil ist reine ETS-Sache - die Firmware kennt keine Profile mehr. Ein neues Profil
kostet deshalb nur einen Eintrag hier und einen Producer-Lauf, kein Flashen.

Zeilenformat: (Slot-Name aus slots.py, Register, Datentyp, Skalierung, Offset)
  Datentyp:    u16 | s16 | u32 | s32 | product
  Skalierung:  1 | 0.1 | 0.01 | 0.001 | 10
  Offset:      wird NACH der Skalierung abgezogen, ganzzahlig -128..127

Bei "product" ist das Register bedeutungslos: der Wert ist das Produkt der beiden im
Katalog VORANGEHENDEN Slots (Spannung * Strom). Die beiden muessen deshalb im Profil
enthalten sein.

'verified' unterscheidet am Geraet gepruefte Profile von uebernommenen. Uebernommene koennen
falsche Skalierungen enthalten, ohne dass es auffaellt - der Assistent prueft sie mit einem
einzigen abgelesenen Sollwert nach.
"""

PROFILES = [
    {
        "name": "Deye SUN-M80G3",
        "transport": 0,  # Solarman V5
        "port": 8899,
        "verified": True,
        # Der Name nennt bewusst das Geraet, an dem gemessen wurde, nicht die Baureihe. Laut
        # ha-solarman gilt dieselbe Registerkarte auch fuer SUN-M60G3, SUN-800-G3 und
        # SUN2000G3 - geprueft haben wir aber nur den SUN-M80G3.
        #
        # Am Geraet SUN-M80G3 aufgenommen (2026-07-22): fuer jeden Wert wurden Rohregister
        # und Anzeige der Solarman-Integration im selben Moment verglichen. Die Stroeme
        # stimmten auf die Nachkommastelle (0x006E = 113 <-> 11,30 A).
        # Zwei Fallen: die Energiezaehler sind 32 Bit LOW-WORD ZUERST (als 16 Bit laufen sie
        # bei 6553,5 kWh ueber), und die Temperatur hat den Offset 10 - ohne Vergleichswert
        # haette 6160 wie 61,6 statt 51,6 Grad ausgesehen.
        "rows": [
            # 32 Bit, nicht 16: bei unserer Messung war das High-Word 0, weil ein 800-W-Geraet
            # nie ueber 6553,5 W kommt. Die Herstellerdefinition (ha-solarman, rule 3) nennt
            # 0x0056+0x0057 - fuer groessere Geraete derselben Baureihe waere u16 falsch.
            ("Power",         0x0056, "u32", 0.1,  0),
            ("Today",         0x003C, "u16", 0.1,  0),
            ("Total",         0x003F, "u32", 0.1,  0),
            ("Today1",        0x0041, "u16", 0.1,  0),
            ("Today2",        0x0042, "u16", 0.1,  0),
            ("Total1",        0x0045, "u32", 0.1,  0),
            ("Total2",        0x0047, "u32", 0.1,  0),
            ("GridVoltage",   0x0049, "u16", 0.1,  0),
            ("GridCurrent",   0x004C, "u16", 0.1,  0),
            ("GridFrequency", 0x004F, "u16", 0.01, 0),
            ("Temperature",   0x005A, "u16", 0.01, 10),
            ("Pv1Voltage",    0x006D, "u16", 0.1,  0),
            ("Pv1Current",    0x006E, "u16", 0.1,  0),
            ("Pv1Power",      0,      "product", 1, 0),
            ("Pv2Voltage",    0x006F, "u16", 0.1,  0),
            ("Pv2Current",    0x0070, "u16", 0.1,  0),
            ("Pv2Power",      0,      "product", 1, 0),
        ],
    },
    {
        "name": "Pylontech Force H1/H2/H3",
        "transport": 1,  # reines Modbus TCP, NICHT Solarman V5
        "port": 8899,
        "verified": True,
        # Am Geraet Force H3 verifiziert (2026-07-22, 192.168.20.31): Registerkarte aus
        # pylontech_force.yaml (ha-solarman, MIT) uebernommen und Wert fuer Wert gegen die
        # Anzeige geprueft - SoC 80 %, Zyklen 6, Restkapazitaet 8,9 kWh, SOH 100 %,
        # Temperatur 35,0 Grad, Spannung 213 V. Quervalidierung: 2 Module a 107 V.
        # Zell- und Modulwerte (64 Zellen ab 5376) sind bewusst nicht enthalten; die gehoeren
        # in ein Monitoring-System, nicht auf den KNX-Bus.
        "rows": [
            ("Soc",               5127, "u16", 1,     0),
            ("Soh",               5152, "u16", 1,     0),
            ("BattVoltage",       5123, "u16", 0.1,   0),
            ("BattCurrent",       5125, "s16", 0.01,  0),
            ("BattPower",         0,    "product", 1, 0),
            ("BattTemperature",   5126, "s16", 0.1,   0),
            ("CycleTimes",        5128, "u16", 1,     0),
            ("RemainingCapacity", 5154, "u32", 0.001, 0),
            ("TodayCharge",       5160, "u32", 0.001, 0),
            ("TodayDischarge",    5162, "u32", 0.001, 0),
            ("TotalCharge",       5164, "u32", 1,     0),
            ("TotalDischarge",    5166, "u32", 1,     0),
        ],
    },
]

TYPES = {"u16": 0, "s16": 1, "u32": 2, "s32": 3, "product": 4}
SCALES = {1: 0, 0.1: 1, 0.01: 2, 0.001: 3, 10: 4}

# Uebernommene Profile aus der ha-solarman-Sammlung, erzeugt von tools/yaml2profile.py.
# Sie stehen HINTER den gemessenen: die Reihenfolge bestimmt die Werte der Auswahlliste, und
# bestehende ETS-Projekte duerfen sich nicht verschieben. Neue Profile also immer anhaengen.
try:
    from profiles_yaml import PROFILES as _IMPORTED
except ImportError:  # der Konverter wurde noch nicht ausgefuehrt
    _IMPORTED = []

# Was wir selbst am Geraet vermessen haben, gewinnt: fuer Deye SUN-M80G3 und
# Pylontech Force H1/H2/H3 gibt es auch YAML-Definitionen, die wollen wir nicht doppelt im Dropdown.
_SUPERSEDED = ("Deye Microinverter", "Pylontech Force H1 / Force H2 / Force H3")
PROFILES += [p for p in _IMPORTED if p["name"] not in _SUPERSEDED]
assert len(PROFILES) == len(_IMPORTED) or not _IMPORTED, \
    "Dubletten-Filter greift nicht mehr - Namen in _SUPERSEDED gegen profiles_yaml.py pruefen"


def validate(slot_names):
    """Prueft jedes Profil gegen den Slot-Katalog. Ein Tippfehler im Slot-Namen oder eine
    unmoegliche Skalierung faellt hier auf, nicht erst als stiller Fehlwert auf dem Bus."""
    index = {name: i for i, name in enumerate(slot_names)}
    for p in PROFILES:
        seen = set()
        for name, reg, typ, scale, offset in p["rows"]:
            assert name in index, "%s: unbekannter Slot '%s'" % (p["name"], name)
            assert name not in seen, "%s: Slot '%s' doppelt" % (p["name"], name)
            seen.add(name)
            assert typ in TYPES, "%s/%s: unbekannter Datentyp '%s'" % (p["name"], name, typ)
            assert scale in SCALES, "%s/%s: Skalierung %s nicht darstellbar" % (p["name"], name, scale)
            assert -128 <= offset <= 127, "%s/%s: Offset %d ausserhalb int8" % (p["name"], name, offset)
            assert 0 <= reg <= 0xFFFF, "%s/%s: Register %d ausserhalb 16 Bit" % (p["name"], name, reg)

            # Ein berechneter Wert braucht seine beiden Faktoren, sonst bleibt er stumm.
            if typ == "product":
                i = index[name]
                assert i >= 2, "%s/%s: berechneter Wert ohne zwei Vorgaenger" % (p["name"], name)
                for f in (slot_names[i - 2], slot_names[i - 1]):
                    assert f in seen, \
                        "%s/%s: Faktor '%s' fehlt im Profil" % (p["name"], name, f)
    return True
