# -*- coding: utf-8 -*-
"""Uebersetzt die Geraetedefinitionen von ha-solarman in Eintraege fuer tools/profiles.py.

Quelle: https://github.com/davidrapan/ha-solarman (MIT-Lizenz),
        custom_components/solarman/inverter_definitions/*.yaml

Aufruf:
    python tools/yaml2profile.py <verzeichnis-mit-yaml> > tools/profiles_yaml.py

Das Ergebnis wird NICHT automatisch eingebunden - es ist erzeugter Code, den man ansieht,
bevor er ins Projekt wandert. Die zwei am Geraet vermessenen Profile bleiben davon
unberuehrt in tools/profiles.py.

WICHTIG: Diese Profile sind UNGEPRUEFT. Sie stammen aus fremder Quelle; niemand hat sie an
unserer Firmware gegen ein echtes Geraet gehalten. Eine falsche Skalierung faellt auf dem
KNX-Bus nicht auf - der Wert sieht nur falsch aus. Deshalb tragen sie verified=False, und
der Hilfetext verweist auf den Assistenten zum Gegenpruefen.

Was uebersetzt wird und was nicht
---------------------------------
rule 1 = 16 Bit vorzeichenlos    -> u16
rule 2 = 16 Bit vorzeichenbehaftet -> s16
rule 3 = 32 Bit vorzeichenlos    -> u32   (Register LOW-WORD ZUERST)
rule 4 = 32 Bit vorzeichenbehaftet -> s32
Alles andere (5-10: Text, Version, Datum, Bitfelder) hat auf dem KNX-Bus nichts verloren
und wird verworfen.

Ebenfalls verworfen: Eintraege mit lookup (Aufzaehlungen), mask (Bitmasken), nicht
zusammenhaengenden Registern, nicht darstellbarer Skalierung oder Offset ausserhalb int8.
Jeder verworfene Eintrag wird gezaehlt und gemeldet - stilles Weglassen waere schlimmer als
ein fehlendes Profil.
"""
import glob
import os
import sys

import yaml

from slots import SLOTS
from profiles import SCALES

SLOT_INDEX = {name: i for i, (name, _, _, _) in enumerate(SLOTS)}

# Zuordnung der ha-solarman-Namen auf unseren Slot-Katalog. Nur was hier steht, wird
# uebernommen: die YAMLs enthalten ueber 1600 verschiedene Namen, das meiste davon
# Diagnose- und Einstellwerte, fuer die niemand eine Gruppenadresse vergibt.
NAME_MAP = {
    "PV Power": "Power",
    "Production Power": "Power",
    "Power": "Power",
    "Total Power": "Power",
    "Apparent Power": "ApparentPower",
    "Grid Voltage": "GridVoltage",
    "AC Voltage": "GridVoltage",
    "Grid L1 Voltage": "GridVoltage",
    "Grid L2 Voltage": "GridVoltageL2",
    "Grid L3 Voltage": "GridVoltageL3",
    "Grid Current": "GridCurrent",
    "AC Current": "GridCurrent",
    "Grid L1 Current": "GridCurrent",
    "Grid Frequency": "GridFrequency",
    "AC Frequency": "GridFrequency",

    "Today Production": "Today",
    "Total Production": "Total",
    "Monthly Production": "Month",
    "Yearly Production": "Year",
    "Today Production 1": "Today1",
    "Today Production 2": "Today2",
    "Total Production 1": "Total1",
    "Total Production 2": "Total2",

    "PV1 Voltage": "Pv1Voltage",
    "PV1 Current": "Pv1Current",
    "PV1 Power": "Pv1Power",
    "PV2 Voltage": "Pv2Voltage",
    "PV2 Current": "Pv2Current",
    "PV2 Power": "Pv2Power",
    "PV3 Voltage": "Pv3Voltage",
    "PV3 Current": "Pv3Current",
    "PV3 Power": "Pv3Power",
    "PV4 Voltage": "Pv4Voltage",
    "PV4 Current": "Pv4Current",
    "PV4 Power": "Pv4Power",

    "Battery SOC": "Soc",
    "Battery": "Soc",
    "Battery SOH": "Soh",
    "Battery Voltage": "BattVoltage",
    "Battery Current": "BattCurrent",
    "Battery Power": "BattPower",
    "Battery Temperature": "BattTemperature",
    "Battery Cycle": "CycleTimes",
    "Battery Capacity": "RemainingCapacity",
    "Today Battery Charge": "TodayCharge",
    "Today Battery Discharge": "TodayDischarge",
    "Total Battery Charge": "TotalCharge",
    "Total Battery Discharge": "TotalDischarge",

    "Load Power": "HousePower",
    "Today Load Consumption": "HouseToday",
    "Today Energy Import": "ImportToday",
    "Today Energy Export": "ExportToday",
    "Total Energy Import": "ImportTotal",
    "Total Energy Export": "ExportTotal",

    "Temperature": "Temperature",
    "Device Temperature": "Temperature",
    "AC Temperature": "Temperature",
    "DC Temperature": "HeatsinkTemp",
    "Radiator Temperature": "HeatsinkTemp",
}

RULE_TYPE = {1: "u16", 2: "s16", 3: "u32", 4: "s32"}

# Schluessel, bei denen ha-solarman den Wert anders bildet, als wir es koennen. Am
# gefaehrlichsten ist "sensors": solche Eintraege rechnen aus mehreren Quellen und lesen das
# angegebene Register gar nicht - uebernaehmen wir sie, stuende eine plausible, aber falsche
# Zahl auf dem Bus. Reihenfolge = Meldungstext.
UNSUPPORTED_KEYS = (
    ("lookup", "Aufzaehlung"),
    ("bitmask", "Bitmaske"),
    ("sensors", "aus mehreren Quellen berechnet"),
    ("divide", "Division"),
    ("magnitude", "Betragsbildung"),
    ("pack", "eigenes Packformat"),
    ("uint", "erzwungen vorzeichenlos"),
)


def convert(path):
    """Ein YAML -> (Profileintrag, Statistik). Gibt None zurueck, wenn nichts uebrig bleibt."""
    doc = yaml.safe_load(open(path, encoding="utf-8"))
    info = doc.get("info", {}) or {}
    # model kann eine Liste sein ("['Force H1', 'Force H2']") - das darf nicht so im
    # Auswahlfeld landen.
    model = info.get("model", "")
    if isinstance(model, (list, tuple)):
        model = " / ".join(str(m) for m in model)
    name = ("%s %s" % (info.get("manufacturer", ""), model)).strip()
    if not name:
        # Ohne info-Block aus dem Dateinamen bauen: "tsun_tsol-ms" -> "Tsun TSOL-MS".
        stem = os.path.splitext(os.path.basename(path))[0]
        parts = stem.split("_")
        name = " ".join([parts[0].capitalize()] + [p.upper() for p in parts[1:]])

    rows = {}
    skipped = {}

    def skip(reason):
        skipped[reason] = skipped.get(reason, 0) + 1

    for group in doc.get("parameters", []) or []:
        for item in group.get("items", []) or []:
            slot = NAME_MAP.get(item.get("name"))
            if slot is None:
                skip("kein passender Slot")
                continue
            if slot in rows:
                skip("Slot doppelt belegt")
                continue

            rule = item.get("rule")
            if rule not in RULE_TYPE:
                skip("Regel %s nicht darstellbar" % rule)
                continue
            # Schluessel, die die Auswertung veraendern und die wir nicht abbilden. Ohne
            # diese Pruefung wuerde etwa ein "sensors"-Eintrag (Summe mehrerer Quellen) als
            # einfaches Register uebernommen - er liest das angegebene Register gar nicht.
            unsupported = next((why for key, why in UNSUPPORTED_KEYS
                                if item.get(key) is not None), None)
            if unsupported:
                skip(unsupported)
                continue

            registers = item.get("registers") or []
            if not registers or not all(isinstance(r, int) for r in registers):
                skip("ohne brauchbare Register")
                continue
            # Zusammenhaengend? Ein 32-Bit-Wert steht LOW-WORD ZUERST in zwei
            # aufeinanderfolgenden Registern; alles andere koennen wir nicht abbilden.
            expected = 2 if rule in (3, 4) else 1
            if len(registers) != expected:
                skip("unerwartete Registerzahl")
                continue
            if expected == 2 and registers[1] != registers[0] + 1:
                skip("Register nicht zusammenhaengend")
                continue

            # scale kann eine Liste sein (je Register eigene Skalierung) - das bilden wir
            # nicht ab.
            scale = item.get("scale", 1)
            if not isinstance(scale, (int, float)) or scale not in SCALES:
                skip("Skalierung %s nicht darstellbar" % scale)
                continue

            # ha-solarman zieht offset VOR der Skalierung ab, wir danach. Umrechnen und
            # nur uebernehmen, wenn dabei ein ganzzahliger int8 herauskommt.
            raw_offset = item.get("offset", 0)
            if not isinstance(raw_offset, (int, float)):
                skip("Offset nicht numerisch")
                continue
            offset = raw_offset * scale
            if offset != int(offset) or not (-128 <= offset <= 127):
                skip("Offset nicht als int8 darstellbar")
                continue

            rows[slot] = (registers[0], RULE_TYPE[rule], scale, int(offset))

    if not rows:
        return None, skipped

    # In Katalogreihenfolge ausgeben - das ist zugleich die Reihenfolge der KOs.
    ordered = [(s, ) + rows[s] for s in sorted(rows, key=lambda s: SLOT_INDEX[s])]
    return {"name": name, "rows": ordered}, skipped


def main():
    if len(sys.argv) < 2:
        sys.stderr.write(__doc__)
        return 1
    files = sorted(glob.glob(os.path.join(sys.argv[1], "*.yaml")))

    out = []
    out.append("# -*- coding: utf-8 -*-")
    out.append('"""ERZEUGT von tools/yaml2profile.py - nicht von Hand bearbeiten.')
    out.append("")
    out.append("Quelle: https://github.com/davidrapan/ha-solarman (MIT-Lizenz).")
    out.append("Diese Profile sind UNGEPRUEFT - sie wurden nicht an einem echten Geraet gegen")
    out.append("die Anzeige gehalten. Eine falsche Skalierung faellt auf dem KNX-Bus nicht auf.")
    out.append("Zum Gegenpruefen genuegt ein abgelesener Wert im Assistenten.")
    out.append('"""')
    out.append("")
    out.append("PROFILES = [")

    # Erst alle einlesen, dann Namensdubletten aufloesen: "Sofar G3" kommt aus zwei Dateien
    # und waere im Auswahlfeld nicht auseinanderzuhalten.
    collected = []
    for path in files:
        profile, _ = convert(path)
        base = os.path.basename(path)
        if profile is None:
            sys.stderr.write("  uebersprungen: %-28s kein verwertbarer Wert\n" % base)
            continue
        collected.append((profile, base))

    seen = {}
    for profile, base in collected:
        seen[profile["name"]] = seen.get(profile["name"], 0) + 1
    for profile, base in collected:
        if seen[profile["name"]] > 1:
            profile["name"] += " (%s)" % os.path.splitext(base)[0]

    total_rows = 0
    for profile, base in collected:
        total_rows += len(profile["rows"])
        out.append("    {")
        out.append('        "name": "%s",' % profile["name"])
        out.append('        "transport": 0,')
        out.append('        "port": 8899,')
        out.append('        "verified": False,')
        out.append('        # aus %s' % base)
        out.append('        "rows": [')
        for slot, reg, typ, scale, offset in profile["rows"]:
            out.append('            ("%s", %d, "%s", %s, %d),' % (slot, reg, typ, scale, offset))
        out.append("        ],")
        out.append("    },")
        sys.stderr.write("  %-28s %2d Werte\n" % (base, len(profile["rows"])))

    out.append("]")
    sys.stdout.write("\n".join(out) + "\n")
    sys.stderr.write("\n%d von %d Dateien uebernommen, %d Werte insgesamt\n"
                     % (len(collected), len(files), total_rows))
    return 0


if __name__ == "__main__":
    sys.exit(main())
