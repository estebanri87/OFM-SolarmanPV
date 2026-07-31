# -*- coding: utf-8 -*-
"""Erzeugt aus tools/profiles.py:

  src/SolarmanPV.profiles.parts.xml   die Auswahlliste PT-SPVProfile
  src/SolarmanPV.script.js            das ETS-Skript samt Profildatenbank

Beides aus einer Quelle, damit Dropdown und Daten nicht auseinanderlaufen koennen.
Die Profile leben ausschliesslich in der ETS - die Firmware kennt keine. Ein neues Profil
kostet deshalb einen Producer-Lauf, kein Flashen.

Aufruf:  python tools/gen_profiles.py
"""
import io
import os

from slots import SLOTS
import profiles

SLOT_NAMES = [n for n, _, _, _ in SLOTS]
profiles.validate(SLOT_NAMES)

here = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------- Auswahlliste
X = []
A = X.append
A('<?xml version="1.0" encoding="utf-8"?>')
A('<?xml-model href="../../Organization/knxprod-support/knx_project_14/knx-editor.xsd" type="application/xml" schematypens="http://www.w3.org/2001/XMLSchema"?>')
A('<!-- ERZEUGT von tools/gen_profiles.py aus tools/profiles.py - nicht von Hand bearbeiten. -->')
A('<KNX xmlns:op="http://github.com/OpenKNX/OpenKNXproducer" xmlns="http://knx.org/xml/project/14" CreatedBy="KNX MT" ToolVersion="5.1.255.16695">')
A('  <ManufacturerData>')
A('    <Manufacturer>')
A('      <ApplicationPrograms>')
A('        <ApplicationProgram>')
A('          <Static>')
A('            <ParameterTypes>')
A('              <ParameterType Id="%AID%_PT-SPVProfile" Name="SPVProfile">')
A('                <TypeRestriction Base="Value" SizeInBit="8" UIHint="DropDown">')
A('                  <!-- Wert 0 ist kein Sonderfall, sondern der Normalzustand nach dem')
A('                       Assistenten: dann gilt die Messwerttabelle so, wie sie dasteht.')
A('                       Der Eintrag benennt deshalb, was zutrifft, nicht was fehlt. -->')
A('                  <Enumeration Text="ohne Profil – eigene Messwerttabelle" Value="0" Id="%AID%_PT-SPVProfile_EN-0" />')
for i, p in enumerate(profiles.PROFILES, start=1):
    A('                  <Enumeration Text="%s" Value="%d" Id="%%AID%%_PT-SPVProfile_EN-%d" />' % (p["name"], i, i))
A('                </TypeRestriction>')
A('              </ParameterType>')
A('            </ParameterTypes>')
A('          </Static>')
A('        </ApplicationProgram>')
A('      </ApplicationPrograms>')
A('    </Manufacturer>')
A('  </ManufacturerData>')
A('</KNX>')

io.open(os.path.join(here, "..", "src", "SolarmanPV.profiles.parts.xml"),
        "w", encoding="utf-8", newline="").write("\n".join(X) + "\n")

# ---------------------------------------------------------------- Skript
index = {name: i for i, name in enumerate(SLOT_NAMES)}

J = []
B = J.append
B('// ERZEUGT von tools/gen_profiles.py - nicht von Hand bearbeiten.')
B('// Logik: tools/script.template.js   Daten: tools/profiles.py')
B('')
# KEINE Trailing Commas vor einer schliessenden Klammer: die ETS-Skript-Engine ist ES3-nah
# (Array.indexOf fehlt), und altes JScript stolpert ueber "[a, b,]" bzw. "{a,}". Deshalb
# werden Elemente gesammelt und mit ",\n" verbunden, statt an jedes ein Komma zu haengen.
B('// Namen aller Slots in Katalogreihenfolge. Der Index ist zugleich die Zeilennummer der')
B('// Messwerttabelle; die Parameter heissen SPV_CH<Kanal><Feld><Slotname>.')
B('var spvSlotNames = [')
name_lines = ['    ' + ', '.join('"%s"' % n for n in SLOT_NAMES[i:i + 6])
              for i in range(0, len(SLOT_NAMES), 6)]
B(',\n'.join(name_lines))
B('];')
B('')
B('// Anzeigetexte, nur fuer Meldungen an den Anwender.')
B('var spvSlotLabels = [')
label_lines = ['    ' + ', '.join('"%s"' % lab for _, lab, _, _ in SLOTS[i:i + 4])
               for i in range(0, len(SLOTS), 4)]
B(',\n'.join(label_lines))
B('];')
B('')
B('// Profildatenbank. Reihenfolge = Werte der Auswahlliste, 0 = ohne Profil (eigene Tabelle).')
B('//')
B('// "rows" ist bewusst ein FLACHES Zahlenarray, je Messwert fuenf Werte hintereinander:')
B('//   Slot-Index, Register, Datentyp, Skalierung, Offset')
B('// Kein Array aus Arrays: die ES3-nahe ETS-Skript-Engine verarbeitet verschachtelte')
B('// Literale nicht zuverlaessig. Alle erprobten OpenKNX-Skripte (PresenceModule, IPCamera)')
B('// nutzen ebenfalls nur flache Arrays.')
B('var SPV_ROW = 5;')
B('var spvProfiles = [')
blocks = ['    null']
for p in profiles.PROFILES:
    flat = ", ".join("%d, %d, %d, %d, %d"
                     % (index[n], reg, profiles.TYPES[t], profiles.SCALES[s], off)
                     for n, reg, t, s, off in p["rows"])
    blocks.append('\n'.join([
        '    {',
        '        name: "%s",' % p["name"],
        '        transport: %d,' % p["transport"],
        '        port: %d,' % p["port"],
        '        verified: %s,' % ("true" if p["verified"] else "false"),
        '        rows: [%s]' % flat,
        '    }']))
B(',\n'.join(blocks))
B('];')
B('')

# Suchplan: 16er-Blockadressen ueber alle in den Profilen belegten Registerbereiche. Der
# Assistent laeuft diese Liste automatisch ab - der Anwender muss NICHT wissen, wo beim
# jeweiligen Geraet die Register liegen (Deye ~0x003B, Pylontech ~0x1400, Sofar/Solis ganz
# woanders). Genau diese Arbeit soll der Assistent abnehmen.
scan_regs = set()
for p in profiles.PROFILES:
    for n, reg, t, s, off in p["rows"]:
        if t == "product":
            continue
        scan_regs.add(reg)
        if t in ("u32", "s32"):
            scan_regs.add(reg + 1)
scan_regs = sorted(scan_regs)
# In 16er-Bloecke zusammenfassen: je Bereich Bloecke ab dem ausgerichteten Start.
scan_blocks = []
i = 0
while i < len(scan_regs):
    base = scan_regs[i] & ~0x0F  # auf 16 ausrichten
    scan_blocks.append(base)
    # alle Register, die dieser Block (16 breit) abdeckt, ueberspringen
    while i < len(scan_regs) and scan_regs[i] < base + 16:
        i += 1
B('// Suchplan fuer "Belegte Register suchen": 16er-Blockadressen ueber alle bekannten')
B('// Registerbereiche aller Profile. So findet der Suchlauf die Werte jedes Geraets, ohne')
B('// dass der Anwender die Startadresse kennen muss.')
B('var spvScanBlocks = [%s];'
  % ", ".join("0x%04X" % b for b in scan_blocks))
B('')

template = io.open(os.path.join(here, "script.template.js"), "r", encoding="utf-8").read()
J.append(template)

io.open(os.path.join(here, "..", "src", "SolarmanPV.script.js"),
        "w", encoding="utf-8", newline="").write("\n".join(J))

verified = sum(1 for p in profiles.PROFILES if p["verified"])
print("profiles.parts.xml + script.js: %d Profile (%d am Geraet geprueft), %d Slots"
      % (len(profiles.PROFILES), verified, len(SLOTS)))
