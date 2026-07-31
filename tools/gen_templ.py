# -*- coding: utf-8 -*-
"""Erzeugt src/SolarmanPV.templ.xml aus dem Slot-Katalog.

Ein Kanal ist ein Gerät. Jedes Gerät hat 64 Messwert-Slots mit fest vergebener Bedeutung
(tools/slots.py). Der Anwender konfiguriert je Slot nur noch Register, Datentyp, Skalierung
und Offset - und ob der Slot überhaupt ein KO erzeugt.

Damit braucht das Gerät keine einkompilierten Profile mehr: ein unbekannter Wechselrichter
lässt sich allein über die ETS einbinden. Profile sind nur noch Vorlagen, die das
ETS-Skript in diese Tabelle schreibt.

Aufruf:  python tools/gen_templ.py
"""
import io
import os

from slots import GROUPS, SLOTS

# ---------------------------------------------------------------- Speicherlayout (Byte)
IP_OFFSET = 0        # 32 Byte IPv4-Literal
PORT_OFFSET = 32
TRANSPORT_OFFSET = 34
PROFILE_OFFSET = 35
SERIALMODE_OFFSET = 36
SLAVEID_OFFSET = 37
INTERVAL_OFFSET = 38
SERIALTEXT_OFFSET = 40   # 16 Byte
VIEW_OFFSET = 56
TABLE_OFFSET = 57        # 64 Zeilen a 4 Byte
ROW_BYTES = 4
SEND_OFFSET = TABLE_OFFSET + len(SLOTS) * ROW_BYTES   # 313
MSG_OFFSET = SEND_OFFSET + 3                          # 316: Meldungstext von "Profil laden"
BLOCK_BYTES = MSG_OFFSET + 48

# Parameternummern
ROW_BASE = 200       # je Zeile 5 Parameter: Register, Datentyp, Skalierung, Offset, aktiv
ROW_STRIDE = 5

P = "%"
CH = P + "C" + P
MID = P + "MID" + P
PS = P + "AID" + P + "_PS-nnn"


def uid(n):
    return P + "AID" + P + "_UP-" + P + "TT" + P + P + "CC" + P + "%03d" % n


def uref(n):
    return uid(n) + "_R-" + P + "TT" + P + P + "CC" + P + "%03d01" % n


def oid(n):
    return P + "AID" + P + "_O-" + P + "TT" + P + P + "CC" + P + "%03d" % n


def oref(n):
    return oid(n) + "_R-" + P + "TT" + P + P + "CC" + P + "%03d01" % n


def row_ids(r):
    """(Register, Datentyp, Skalierung, Offset, aktiv) fuer Zeile r."""
    b = ROW_BASE + r * ROW_STRIDE
    return b, b + 1, b + 2, b + 3, b + 4


NAME = P + "AID" + P + "_P-" + P + "TT" + P + P + "CC" + P + "000"
NAMEREF = NAME + "_R-" + P + "TT" + P + P + "CC" + P + "00001"

L = []
A = L.append
A('<?xml version="1.0" encoding="utf-8"?>')
A('<?xml-model href="../../Organization/knxprod-support/knx_project_14/knx-editor.xsd" type="application/xml" schematypens="http://www.w3.org/2001/XMLSchema"?>')
A('<!-- ERZEUGT von tools/gen_templ.py - nicht von Hand bearbeiten. -->')
A('<KNX xmlns:op="http://github.com/OpenKNX/OpenKNXproducer" xmlns="http://knx.org/xml/project/14" CreatedBy="KNX MT" ToolVersion="5.1.255.16695">')
A('  <ManufacturerData>')
A('    <Manufacturer>')
A('      <ApplicationPrograms>')
A('        <ApplicationProgram>')
A('          <Static>')
A('            <Parameters>')
A('              <!-- Ein Kanal = ein Gerät (Wechselrichter oder Batteriespeicher). -->')
A('              <Parameter Id="%s" Name="Device%sName" ParameterType="%sAID%s_PT-Text40Byte" Text="Bezeichnung" Value="" />' % (NAME, CH, P, P))
A('              <!-- Nur IPv4-Literale: eine Namensaufloesung wuerde blockieren. -->')
A('              <Union SizeInBit="256">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, IP_OFFSET))
A('                <Parameter Id="%s" Name="CH%sLoggerIp" ParameterType="%sAID%s_PT-SPVString32" Offset="0" BitOffset="0" Text="IP-Adresse" Value="" />' % (uid(1), CH, P, P))
A('              </Union>')
A('              <Union SizeInBit="16">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, PORT_OFFSET))
A('                <Parameter Id="%s" Name="CH%sLoggerPort" ParameterType="%sAID%s_PT-SPVPort" Offset="0" BitOffset="0" Text="Port" Value="8899" />' % (uid(2), CH, P, P))
A('              </Union>')
A('              <Union SizeInBit="8">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, TRANSPORT_OFFSET))
A('                <Parameter Id="%s" Name="CH%sTransport" ParameterType="%sAID%s_PT-SPVTransport" Offset="0" BitOffset="0" Text="Transportprotokoll" Value="0" />' % (uid(3), CH, P, P))
A('              </Union>')
A('              <Union SizeInBit="8">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, PROFILE_OFFSET))
A('                <Parameter Id="%s" Name="CH%sProfile" ParameterType="%sAID%s_PT-SPVProfile" Offset="0" BitOffset="0" Text="Geräteprofil" Value="0" />' % (uid(4), CH, P, P))
A('              </Union>')
A('              <Union SizeInBit="8">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, SERIALMODE_OFFSET))
A('                <Parameter Id="%s" Name="CH%sSerialMode" ParameterType="%sAID%s_PT-SPVSerialMode" Offset="0" BitOffset="0" Text="Logger-Seriennummer" Value="0" />' % (uid(5), CH, P, P))
A('              </Union>')
A('              <Union SizeInBit="8">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, SLAVEID_OFFSET))
A('                <Parameter Id="%s" Name="CH%sSlaveId" ParameterType="%sAID%s_PT-SPVSlaveId" Offset="0" BitOffset="0" Text="Modbus-Slave-ID" Value="1" />' % (uid(6), CH, P, P))
A('              </Union>')
A('              <Union SizeInBit="16">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, INTERVAL_OFFSET))
A('                <Parameter Id="%s" Name="CH%sPollInterval" ParameterType="%sAID%s_PT-SPVInterval" Offset="0" BitOffset="0" Text="Abfrageintervall (0 = aus)" SuffixText=" s" Value="10" />' % (uid(7), CH, P, P))
A('              </Union>')
A('              <Union SizeInBit="128">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, SERIALTEXT_OFFSET))
A('                <Parameter Id="%s" Name="CH%sLoggerSerialText" ParameterType="%sAID%s_PT-SPVString16" Offset="0" BitOffset="0" Text="Seriennummer" Value="" />' % (uid(8), CH, P, P))
A('              </Union>')
A('              <Union SizeInBit="8">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, VIEW_OFFSET))
A('                <Parameter Id="%s" Name="CH%sView" ParameterType="%sAID%s_PT-CheckBox" Offset="0" BitOffset="0" Text="Erweiterter Modus: Register und Umrechnung anzeigen" Value="0" />' % (uid(9), CH, P, P))
A('              </Union>')
A('              <!-- Messwerttabelle, Byte %d-%d: %d Zeilen a %d Byte.' % (TABLE_OFFSET, SEND_OFFSET - 1, len(SLOTS), ROW_BYTES))
A('                   Die Bedeutung jeder Zeile ist fest (tools/slots.py) und bestimmt KO-Name und DPT;')
A('                   sie ist deshalb kein Parameter. BitOffset zählt innerhalb des Bytes (0..7). -->')
A('              <Union SizeInBit="%d">' % (len(SLOTS) * ROW_BYTES * 8))
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, TABLE_OFFSET))
for r, (name, label, _, _) in enumerate(SLOTS):
    reg, typ, sca, off, act = row_ids(r)
    b = r * ROW_BYTES
    A('                <Parameter Id="%s" Name="CH%sReg%s" ParameterType="%sAID%s_PT-SPVRegister" Offset="%d" BitOffset="0" Text="Register" Value="0" />'
      % (uid(reg), CH, name, P, P, b))
    A('                <Parameter Id="%s" Name="CH%sType%s" ParameterType="%sAID%s_PT-SPVDataType" Offset="%d" BitOffset="0" Text="Datentyp" Value="0" />'
      % (uid(typ), CH, name, P, P, b + 2))
    A('                <Parameter Id="%s" Name="CH%sScale%s" ParameterType="%sAID%s_PT-SPVScale" Offset="%d" BitOffset="3" Text="Skalierung" Value="0" />'
      % (uid(sca), CH, name, P, P, b + 2))
    A('                <Parameter Id="%s" Name="CH%sEn%s" ParameterType="%sAID%s_PT-CheckBox" Offset="%d" BitOffset="6" Text="%s" Value="0" />'
      % (uid(act), CH, name, P, P, b + 2, label))
    A('                <Parameter Id="%s" Name="CH%sOffset%s" ParameterType="%sAID%s_PT-SPVOffset" Offset="%d" BitOffset="0" Text="Offset" Value="0" />'
      % (uid(off), CH, name, P, P, b + 3))
A('              </Union>')
A('              <!-- Sendeverhalten (Byte %d-%d) -->' % (SEND_OFFSET, SEND_OFFSET + 2))
A('              <Union SizeInBit="16">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, SEND_OFFSET))
A('                <Parameter Id="%s" Name="CH%sSendDelayBase" ParameterType="%sAID%s_PT-DelayBase" Offset="0" BitOffset="0" Text="Zeitbasis" Value="1" />' % (uid(50), CH, P, P))
A('                <Parameter Id="%s" Name="CH%sSendDelayTime" ParameterType="%sAID%s_PT-DelayTime" Offset="0" BitOffset="2" Text="zyklisch senden alle (0 = aus)" Value="5" />' % (uid(51), CH, P, P))
A('              </Union>')
A('              <Union SizeInBit="8">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, SEND_OFFSET + 2))
A('                <Parameter Id="%s" Name="CH%sSendChangePercent" ParameterType="%sAID%s_PT-SPVPercent" Offset="0" BitOffset="0" Text="zusätzlich bei Änderung um (0 = aus)" SuffixText=" %s" Value="5" />' % (uid(52), CH, P, P, P))
A('              </Union>')
A('              <!-- Meldung von "Profil in Tabelle übernehmen". Offline-Knoepfe bekommen kein')
A('                   progress-Objekt (wie ConfigTransfer/PresenceModule), daher Ausgabe hier. -->')
A('              <Union SizeInBit="384">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, MSG_OFFSET))
A('                <Parameter Id="%s" Name="CH%sProfileMsg" ParameterType="%sAID%s_PT-SPVString48" Offset="0" BitOffset="0" Text="Meldung" Value="" />' % (uid(53), CH, P, P))
A('              </Union>')
A('            </Parameters>')
A('            <ParameterRefs>')
A('              <ParameterRef Id="%s" RefId="%s" />' % (NAMEREF, NAME))
nums = list(range(1, 10)) + [50, 51, 52, 53]
for r in range(len(SLOTS)):
    nums += list(row_ids(r))
for n in nums:
    A('              <ParameterRef Id="%s" RefId="%s" />' % (uref(n), uid(n)))
A('            </ParameterRefs>')
A('            <ComObjectTable>')
A('              <ComObject Id="%s" Number="%sK0%s" Name="CH%sReachable" Text="" FunctionText="" ObjectSize="1 Bit" DatapointType="DPST-1-11" ReadFlag="Enabled" WriteFlag="Disabled" CommunicationFlag="Enabled" TransmitFlag="Enabled" UpdateFlag="Disabled" ReadOnInitFlag="Disabled" />' % (oid(0), P, P, CH))
A('              <!-- %d Messwert-Slots mit fester Bedeutung; Name, Größe und DPT sind statisch. -->' % len(SLOTS))
for k, (name, label, dpt, size) in enumerate(SLOTS, start=1):
    A('              <ComObject Id="%s" Number="%sK%d%s" Name="CH%s%s" Text="" FunctionText="" ObjectSize="%s" DatapointType="%s" ReadFlag="Enabled" WriteFlag="Disabled" CommunicationFlag="Enabled" TransmitFlag="Enabled" UpdateFlag="Disabled" ReadOnInitFlag="Disabled" />'
      % (oid(k), P, k, P, CH, name, size, dpt))
A('            </ComObjectTable>')
A('            <ComObjectRefs>')
A('              <ComObjectRef Id="%s" RefId="%s" Text="{{0:Gerät %s}}: erreichbar" FunctionText="Gerät %s: Status" TextParameterRefId="%s" />' % (oref(0), oid(0), CH, CH, NAMEREF))
for k, (name, label, _, _) in enumerate(SLOTS, start=1):
    A('              <ComObjectRef Id="%s" RefId="%s" Text="{{0:Gerät %s}}: %s" FunctionText="Gerät %s: %s" TextParameterRefId="%s" />'
      % (oref(k), oid(k), CH, label, CH, label, NAMEREF))
A('            </ComObjectRefs>')
A('          </Static>')
A('          <Dynamic>')
A('            <ChannelIndependentBlock>')
A('              <choose ParamRefId="%sAID%s_UP-%sTT%s00000_R-%sTT%s0000001">' % (P, P, P, P, P, P))
A('                <when test="&gt;=%s">' % CH)
A('                  <ParameterBlock Id="%sAID%s_PB-nnn" Name="f%sCC%sSPV" Text="Gerät %s: {{0: ...}}" TextParameterRefId="%s" Icon="white-balance-sunny" ShowInComObjectTree="true" HelpContext="SPV-Geraeteprofil">' % (P, P, P, P, CH, NAMEREF))
A('                    <ParameterSeparator Id="%s" Text="Gerät %s" UIHint="Headline" />' % (PS, CH))
A('                    <ParameterRefRef RefId="%s" />' % NAMEREF)
A('                    <ParameterRefRef RefId="%s" HelpContext="SPV-Geraeteprofil" />' % uref(4))
A('                    <Button Id="%sAID%s_B-%sTT%s%sCC%s001" Text="Profil in Tabelle übernehmen" EventHandler="spvBtnLoadProfile" EventHandlerParameters="{ &quot;channel&quot;:&quot;%s&quot; }" />'
  % (P, P, P, P, P, P, CH))
A('                    <ParameterSeparator Id="%s" Text="{{0:}}" TextParameterRefId="%s" />' % (PS, uref(53)))
A('                    <ParameterSeparator Id="%s" Text="" UIHint="HorizontalRuler" />' % PS)
A('                    <ParameterSeparator Id="%s" Text="Verbindung" UIHint="Headline" />' % PS)
HELP_CONN = {1: "SPV-Verbindung", 2: "SPV-Verbindung", 3: "SPV-Transportprotokoll", 7: "SPV-Verbindung"}
for n in (1, 2, 3, 7):
    A('                    <ParameterRefRef IndentLevel="1" RefId="%s" HelpContext="%s" />' % (uref(n), HELP_CONN[n]))
A('                    <choose ParamRefId="%s">' % uref(3))
A('                      <when test="=0">')
A('                        <ParameterSeparator Id="%s" Text="Die Seriennummer wird bei Solarman V5 normalerweise automatisch ermittelt. Manuell nur eintragen, wenn das fehlschlägt - manche Logger antworten auf Anfragen mit falscher Seriennummer gar nicht." UIHint="Information" />' % PS)
A('                        <ParameterRefRef IndentLevel="1" RefId="%s" HelpContext="SPV-Logger-Seriennummer" />' % uref(5))
A('                        <choose ParamRefId="%s">' % uref(5))
A('                          <when test="=1">')
A('                            <ParameterRefRef IndentLevel="2" RefId="%s" HelpContext="SPV-Logger-Seriennummer" />' % uref(8))
A('                          </when>')
A('                        </choose>')
A('                      </when>')
A('                      <when test="=1">')
A('                        <ParameterSeparator Id="%s" Text="Modbus TCP kennt keine Logger-Seriennummer." UIHint="Information" />' % PS)
A('                      </when>')
A('                    </choose>')
A('                    <ParameterSeparator Id="%s" Text="" UIHint="HorizontalRuler" />' % PS)
A('                    <ParameterSeparator Id="%s" Text="Erweitert" UIHint="Headline" />' % PS)
A('                    <ParameterRefRef IndentLevel="1" RefId="%s" HelpContext="SPV-Modbus-Slave-ID" />' % uref(6))
A('                    <ParameterSeparator Id="%s" Text="" UIHint="HorizontalRuler" />' % PS)
A('                    <ParameterSeparator Id="%s" Text="Sendeverhalten" UIHint="Headline" />' % PS)
for n in (51, 50, 52):
    A('                    <ParameterRefRef IndentLevel="1" RefId="%s" HelpContext="SPV-Sendeverhalten" />' % uref(n))
A('                    <ParameterSeparator Id="%s" Text="" UIHint="HorizontalRuler" />' % PS)
A('                    <ParameterSeparator Id="%s" Text="Messwerte" UIHint="Headline" />' % PS)
A('                    <ParameterSeparator Id="%s" Text="Angehakt wird gesendet. Register und Umrechnung kommen aus dem Geräteprofil oder vom Assistenten und lassen sich hier von Hand nachbessern." UIHint="Information" />' % PS)

# KO-Sichtbarkeit: haengt am aktiv-Haken.
A('                    <ComObjectRefRef RefId="%s" />' % oref(0))
for r in range(len(SLOTS)):
    _, _, _, _, act = row_ids(r)
    A('                    <choose ParamRefId="%s">' % uref(act))
    A('                      <when test="=1"><ComObjectRefRef RefId="%s" /></when>' % oref(r + 1))
    A('                    </choose>')

# Konfiguration: je Kategorie eine echte Rastertabelle, immer sichtbar. Das Aktivieren
# geschieht ueber die Spalte "aktiv" - ein Anzeige-Umschalter ist nicht noetig.
slot_index = 0
for gi, (title, group) in enumerate(GROUPS):
    tid = P + 'AID' + P + '_PB-%sCC%sMT%d' % (P, P, gi)
    A('                    <ParameterSeparator Id="%s" Text="%s" UIHint="Headline" />' % (PS, title))
    A('                    <ParameterBlock Id="%s" Name="MTbl%d" Text="%s" Inline="true" Layout="Table" HelpContext="SPV-Messwerte">' % (tid, gi, title))
    A('                      <Rows>')
    for j, (name, label, _, _) in enumerate(group, start=1):
        A('                        <Row Id="%s_R-%d" Name="R%d" Text="%s" />' % (tid, j, j, label))
    A('                      </Rows>')
    A('                      <Columns>')
    A('                        <Column Id="%s_C-1" Name="C1" Text="aktiv" Width="12%s" />' % (tid, P))
    A('                        <Column Id="%s_C-2" Name="C2" Text="Register" Width="16%s" />' % (tid, P))
    A('                        <Column Id="%s_C-3" Name="C3" Text="Datentyp" Width="32%s" />' % (tid, P))
    A('                        <Column Id="%s_C-4" Name="C4" Text="Skalierung" Width="22%s" />' % (tid, P))
    A('                        <Column Id="%s_C-5" Name="C5" Text="Offset" Width="18%s" />' % (tid, P))
    A('                      </Columns>')
    for j in range(len(group)):
        reg, typ, sca, off, act = row_ids(slot_index)
        slot_index += 1
        row = j + 1
        A('                      <ParameterRefRef RefId="%s" Cell="%d,1" />' % (uref(act), row))
        A('                      <ParameterRefRef RefId="%s" Cell="%d,2" />' % (uref(reg), row))
        A('                      <ParameterRefRef RefId="%s" Cell="%d,3" />' % (uref(typ), row))
        A('                      <ParameterRefRef RefId="%s" Cell="%d,4" />' % (uref(sca), row))
        A('                      <ParameterRefRef RefId="%s" Cell="%d,5" />' % (uref(off), row))
    A('                    </ParameterBlock>')
assert slot_index == len(SLOTS)
A('                  </ParameterBlock>')
A('                </when>')
A('              </choose>')
A('            </ChannelIndependentBlock>')
A('          </Dynamic>')
A('        </ApplicationProgram>')
A('      </ApplicationPrograms>')
A('    </Manufacturer>')
A('  </ManufacturerData>')
A('</KNX>')

here = os.path.dirname(os.path.abspath(__file__))
out = os.path.join(here, "..", "src", "SolarmanPV.templ.xml")
io.open(out, "w", encoding="utf-8", newline="").write("\n".join(L) + "\n")

# ---------------------------------------------------------------- C++-Katalog
# Bedeutung, DPT und Speicherlayout stehen damit nur an einer Stelle: in slots.py.
# Der Kanal greift rein rechnerisch auf die Tabelle zu und braucht keinen 64-fach-switch.

ENCODING = {"14": "EncFloat", "13": "EncEnergyWh", "9": "EncTemp", "7": "EncU16"}


def encoding_of(dpt):
    main, sub = dpt.replace("DPST-", "").split("-")
    if main == "5":
        return "EncPercent" if sub == "1" else "EncU8"
    return ENCODING[main], main, sub


H = []
B = H.append
B('#pragma once')
B('#include <stdint.h>')
B('')
B('// ERZEUGT von tools/gen_templ.py aus tools/slots.py - nicht von Hand bearbeiten.')
B('//')
B('// Die Bedeutung jedes Slots ist fest vergeben; sie bestimmt KO-Name und DPT und ist')
B('// deshalb kein Parameter (Begruendung in doc/Assistent-Umbau.md). Konfigurierbar sind')
B('// je Zeile nur Register, Datentyp, Skalierung, Offset und ob der Slot sendet.')
B('')
B('namespace Spv')
B('{')
B('    enum Encoding : uint8_t')
B('    {')
B('        EncFloat,     // DPT 14.x, Gleitkomma mit slot-eigenem Subtyp')
B('        EncEnergyWh,  // DPT 13.010, Wattstunden ganzzahlig')
B('        EncPercent,   // DPT 5.001')
B('        EncTemp,      // DPT 9.001')
B('        EncU16,       // DPT 7.x')
B('        EncU8,        // DPT 5.010')
B('    };')
B('')
B('    struct Slot')
B('    {')
B('        const char* label;')
B('        uint8_t encoding;')
B('        uint8_t dptMain;')
B('        uint8_t dptSub;')
B('    };')
B('')
B('    // Zeilenlayout im Parameterspeicher, muss zu gen_templ.py passen.')
B('    static const uint16_t TABLE_OFFSET = %d;' % TABLE_OFFSET)
B('    static const uint8_t ROW_BYTES = %d;' % ROW_BYTES)
B('    static const uint8_t SLOT_COUNT = %d;' % len(SLOTS))
B('')
B('    // Datentyp der Zeile (Feld "Datentyp").')
B('    enum RowType : uint8_t')
B('    {')
B('        TypeU16 = 0,')
B('        TypeS16 = 1,')
B('        TypeU32 = 2,')
B('        TypeS32 = 3,')
B('        TypeProduct = 4, // Produkt der zwei vorangehenden Zeilen (Spannung * Strom)')
B('    };')
B('')
B('    // Faktoren zum Feld "Skalierung", gleiche Reihenfolge wie PT-SPVScale.')
B('    static const float SCALE[8] = {1.0f, 0.1f, 0.01f, 0.001f, 10.0f, 1.0f, 1.0f, 1.0f};')
B('')
B('    static const Slot SLOTS[SLOT_COUNT] = {')
for i, (name, label, dpt, _) in enumerate(SLOTS):
    enc = encoding_of(dpt)
    if isinstance(enc, tuple):
        enc, main, sub = enc
    else:
        main, sub = dpt.replace("DPST-", "").split("-")
    B('        {"%s", %-11s %2s, %2s}, // %2d %s' % (label, enc + ",", main, sub, i + 1, name))
B('    };')
B('} // namespace Spv')

hout = os.path.join(here, "..", "src", "SlotCatalog.h")
io.open(hout, "w", encoding="utf-8", newline="").write("\n".join(H) + "\n")

print("templ.xml: %d Slots, Tabelle Byte %d-%d, Sendeverhalten ab %d, Block %d Byte, %d KOs je Kanal"
      % (len(SLOTS), TABLE_OFFSET, SEND_OFFSET - 1, SEND_OFFSET, BLOCK_BYTES, len(SLOTS) + 1))
print("SlotCatalog.h: %d Slots" % len(SLOTS))
