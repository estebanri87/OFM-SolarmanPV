# -*- coding: utf-8 -*-
"""Erzeugt src/SolarmanPV.assistant.parts.xml - die globale Seite "Assistent".

Der Assistent misst ein unbekanntes Gerät ein: Transport und Seriennummer erkennen,
Register lesen, Bedeutungen zuordnen, Skalierung ableiten und das Ergebnis in ein Gerät
übernehmen.

Die Seite ist GLOBAL, nicht je Gerät. Man misst ohnehin ein Gerät zur Zeit; so existiert
die Messtabelle nur einmal im Speicher (448 statt 2688 Byte) und man springt beim Messen
nicht zwischen Seiten.

Aufruf über tools/generate.py
"""
import io
import os

from slots import SLOTS

# ---------------------------------------------------------------- Speicherlayout (Byte)
# Byte 0 ist "Aktive Geräte" (in share.xml von Hand gepflegt).
IP_OFFSET = 4          # 32 Byte IPv4-Literal
PORT_OFFSET = 36
SLAVE_OFFSET = 38
STARTREG_OFFSET = 40
COUNT_OFFSET = 42
# Transport und Seriennummer sind MASCHINENWERTE: das Skript schreibt sie, "Übernehmen" liest
# sie. Angezeigt werden sie nicht - dafuer gibt es den Ergebnistext. Ein Auswahlfeld wuerde
# behaupten, hier sei etwas zu entscheiden; tatsaechlich ist es ein Messergebnis.
RESULT_TRANSPORT = 43
TARGET_CHANNEL = 45
ROW_COUNT = 46
RESULT_SERIAL = 48     # 16 Byte Text, verborgen
RESULT_TEXT = 64       # 48 Byte Text, das was der Anwender liest
TABLE_OFFSET = 112     # 64 Zeilen a 10 Byte
ROW_BYTES = 12
BLOCK_END = TABLE_OFFSET + len(SLOTS) * ROW_BYTES

# Je Zeile 5 Parameter: Rohwert, Bedeutung, Datentyp, Skalierung, Offset, Sollwert.
# Datentyp/Skalierung/Offset sind das ERGEBNIS der Ableitung und stehen bewusst in der
# Tabelle: sonst koennte man das Ergebnis nicht pruefen, bevor es ins Gerät wandert.
ROW_BASE = 300
ROW_STRIDE = 7

P = "%"
MID = P + "MID" + P
PS = P + "AID" + P + "_PS-nnn"


def gid(n):
    return P + "AID" + P + "_UP-" + P + "TT" + P + "%05d" % n


def gref(n):
    return gid(n) + "_R-" + P + "TT" + P + "%05d01" % n


def gref_ro(n):
    """Zweite Referenz auf denselben Parameter, nur lesbar - fuer Ergebnisfelder, die das
    Skript schreibt und der Anwender nur ansehen soll. Die Endung muss dem OpenKNX-Schema
    tcccnnn_R-tcccnnnrr folgen, deshalb 02 statt eines sprechenden Suffixes."""
    return gid(n) + "_R-" + P + "TT" + P + "%05d02" % n


def row_ids(r):
    """(Register, Rohwert, Bedeutung, Datentyp, Skalierung, Offset, Sollwert)

    Die Registeradresse steht je Zeile, nicht als Startwert plus Zeilennummer: der Suchlauf
    überspringt leere Register, danach waere die Zeilennummer keine Adresse mehr."""
    b = ROW_BASE + r * ROW_STRIDE
    return b, b + 1, b + 2, b + 3, b + 4, b + 5, b + 6


L = []
A = L.append
A('<?xml version="1.0" encoding="utf-8"?>')
A('<?xml-model href="../../Organization/knxprod-support/knx_project_14/knx-editor.xsd" type="application/xml" schematypens="http://www.w3.org/2001/XMLSchema"?>')
A('<!-- ERZEUGT von tools/gen_assistant.py - nicht von Hand bearbeiten. -->')
A('<KNX xmlns:op="http://github.com/OpenKNX/OpenKNXproducer" xmlns="http://knx.org/xml/project/14" CreatedBy="KNX MT" ToolVersion="5.1.255.16695">')
A('  <ManufacturerData>')
A('    <Manufacturer>')
A('      <ApplicationPrograms>')
A('        <ApplicationProgram>')
A('          <Static>')

# ---------------------------------------------------------------- ParameterTypes
A('            <ParameterTypes>')
A('              <!-- Bedeutung einer Messzeile: leer = Zeile wird beim Übernehmen übergangen.')
A('                   Anders als in der Gerätetabelle ist die Bedeutung hier ein echter')
A('                   Parameter - sie erzeugt kein KO, sondern sagt nur, in welche Zeile des')
A('                   Zielgeraets der gemessene Wert wandert. -->')
A('              <ParameterType Id="%AID%_PT-SPVMeaning" Name="SPVMeaning">')
A('                <TypeRestriction Base="Value" SizeInBit="8" UIHint="DropDown">')
A('                  <Enumeration Text="(nicht zuordnen)" Value="0" Id="%AID%_PT-SPVMeaning_EN-0" />')
for i, (name, label, _, _) in enumerate(SLOTS, start=1):
    A('                  <Enumeration Text="%s" Value="%d" Id="%%AID%%_PT-SPVMeaning_EN-%d" />' % (label, i, i))
A('                </TypeRestriction>')
A('              </ParameterType>')
A('              <!-- Der am Gerät abgelesene Wert. Aus ihm werden Datentyp, Skalierung und')
A('                   Offset berechnet - ohne ihn bleibt nur Raten. -->')
A('              <ParameterType Id="%AID%_PT-SPVReference" Name="SPVReference">')
A('                <TypeFloat Encoding="IEEE-754 Single" minInclusive="-1000000" maxInclusive="1000000" />')
A('              </ParameterType>')
A('              <ParameterType Id="%AID%_PT-SPVRawValue" Name="SPVRawValue">')
A('                <TypeNumber SizeInBit="16" Type="unsignedInt" minInclusive="0" maxInclusive="65535" />')
A('              </ParameterType>')
A('              <ParameterType Id="%AID%_PT-SPVRowCount" Name="SPVRowCount">')
A('                <TypeNumber SizeInBit="8" Type="unsignedInt" minInclusive="0" maxInclusive="%d" />' % len(SLOTS))
A('              </ParameterType>')
A('              <ParameterType Id="%AID%_PT-SPVReadCount" Name="SPVReadCount">')
A('                <TypeNumber SizeInBit="8" Type="unsignedInt" minInclusive="1" maxInclusive="%d" />' % len(SLOTS))
A('              </ParameterType>')
A('              <ParameterType Id="%AID%_PT-SPVTargetChannel" Name="SPVTargetChannel">')
A('                <TypeNumber SizeInBit="8" Type="unsignedInt" minInclusive="1" maxInclusive="%N%" />')
A('              </ParameterType>')
A('            </ParameterTypes>')

# ---------------------------------------------------------------- Parameter
A('            <Parameters>')
A('              <Union SizeInBit="256">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, IP_OFFSET))
A('                <Parameter Id="%s" Name="AsstIp" ParameterType="%sAID%s_PT-SPVString32" Offset="0" BitOffset="0" Text="IP-Adresse" Value="" />' % (gid(1), P, P))
A('              </Union>')
A('              <Union SizeInBit="16">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, PORT_OFFSET))
A('                <Parameter Id="%s" Name="AsstPort" ParameterType="%sAID%s_PT-SPVPort" Offset="0" BitOffset="0" Text="Port" Value="8899" />' % (gid(2), P, P))
A('              </Union>')
A('              <Union SizeInBit="8">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, SLAVE_OFFSET))
A('                <Parameter Id="%s" Name="AsstSlaveId" ParameterType="%sAID%s_PT-SPVSlaveId" Offset="0" BitOffset="0" Text="Modbus-Slave-ID" Value="1" />' % (gid(3), P, P))
A('              </Union>')
A('              <Union SizeInBit="16">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, STARTREG_OFFSET))
A('                <Parameter Id="%s" Name="AsstStartReg" ParameterType="%sAID%s_PT-SPVRegister" Offset="0" BitOffset="0" Text="ab Register" Value="0" />' % (gid(4), P, P))
A('              </Union>')
A('              <Union SizeInBit="8">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, COUNT_OFFSET))
A('                <Parameter Id="%s" Name="AsstReadCount" ParameterType="%sAID%s_PT-SPVReadCount" Offset="0" BitOffset="0" Text="Anzahl Register" Value="16" />' % (gid(5), P, P))
A('              </Union>')
A('              <Union SizeInBit="8">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, RESULT_TRANSPORT))
A('                <Parameter Id="%s" Name="AsstTransport" ParameterType="%sAID%s_PT-SPVTransport" Offset="0" BitOffset="0" Text="erkannter Transport" Value="0" />' % (gid(6), P, P))
A('              </Union>')
A('              <Union SizeInBit="8">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, TARGET_CHANNEL))
A('                <Parameter Id="%s" Name="AsstTargetChannel" ParameterType="%sAID%s_PT-SPVTargetChannel" Offset="0" BitOffset="0" Text="Zielgerät" Value="1" />' % (gid(8), P, P))
A('              </Union>')
A('              <Union SizeInBit="8">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, ROW_COUNT))
A('                <Parameter Id="%s" Name="AsstRowCount" ParameterType="%sAID%s_PT-SPVRowCount" Offset="0" BitOffset="0" Text="gelesene Zeilen" Value="0" />' % (gid(9), P, P))
A('              </Union>')
A('              <Union SizeInBit="128">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, RESULT_SERIAL))
A('                <Parameter Id="%s" Name="AsstSerial" ParameterType="%sAID%s_PT-SPVString16" Offset="0" BitOffset="0" Text="Seriennummer" Value="" />' % (gid(10), P, P))
A('              </Union>')
A('              <Union SizeInBit="384">')
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, RESULT_TEXT))
A('                <Parameter Id="%s" Name="AsstResultText" ParameterType="%sAID%s_PT-SPVString48" Offset="0" BitOffset="0" Text="Ergebnis" Value="" />' % (gid(7), P, P))
A('              </Union>')
A('              <!-- Messtabelle, Byte %d-%d: %d Zeilen a %d Byte. Existiert nur EINMAL -->' % (TABLE_OFFSET, BLOCK_END - 1, len(SLOTS), ROW_BYTES))
A('              <Union SizeInBit="%d">' % (len(SLOTS) * ROW_BYTES * 8))
A('                <Memory CodeSegment="%s" Offset="%d" BitOffset="0" />' % (MID, TABLE_OFFSET))
for r in range(len(SLOTS)):
    reg, raw, mean, typ, sca, off, ref = row_ids(r)
    b = r * ROW_BYTES
    A('                <Parameter Id="%s" Name="AsstReg%02d" ParameterType="%sAID%s_PT-SPVRegister" Offset="%d" BitOffset="0" Text="Register" Value="0" />'
      % (gid(reg), r, P, P, b))
    A('                <Parameter Id="%s" Name="AsstRaw%02d" ParameterType="%sAID%s_PT-SPVRawValue" Offset="%d" BitOffset="0" Text="Rohwert" Value="0" />'
      % (gid(raw), r, P, P, b + 2))
    A('                <Parameter Id="%s" Name="AsstMeaning%02d" ParameterType="%sAID%s_PT-SPVMeaning" Offset="%d" BitOffset="0" Text="Bedeutung" Value="0" />'
      % (gid(mean), r, P, P, b + 4))
    A('                <Parameter Id="%s" Name="AsstType%02d" ParameterType="%sAID%s_PT-SPVDataType" Offset="%d" BitOffset="0" Text="Datentyp" Value="0" />'
      % (gid(typ), r, P, P, b + 5))
    A('                <Parameter Id="%s" Name="AsstScale%02d" ParameterType="%sAID%s_PT-SPVScale" Offset="%d" BitOffset="3" Text="Skalierung" Value="0" />'
      % (gid(sca), r, P, P, b + 5))
    A('                <Parameter Id="%s" Name="AsstOffset%02d" ParameterType="%sAID%s_PT-SPVOffset" Offset="%d" BitOffset="0" Text="Offset" Value="0" />'
      % (gid(off), r, P, P, b + 6))
    A('                <Parameter Id="%s" Name="AsstRef%02d" ParameterType="%sAID%s_PT-SPVReference" Offset="%d" BitOffset="0" Text="abgelesener Wert" Value="0" />'
      % (gid(ref), r, P, P, b + 8))
A('              </Union>')
A('            </Parameters>')

# ---------------------------------------------------------------- ParameterRefs
# Zwei Refs je Ergebnisfeld: eine beschreibbare fuers Skript, eine nur lesbare fuer die
# Anzeige. Muster aus PMModul.hlk-table-row-*.parts.xml.
A('            <ParameterRefs>')
for n in range(1, 11):
    A('              <ParameterRef Id="%s" RefId="%s" />' % (gref(n), gid(n)))
for r in range(len(SLOTS)):
    reg, raw, mean, typ, sca, off, ref = row_ids(r)
    # Alle Refs ReadWrite: Access="Read" laesst die Tabellenzelle in der ETS LEER - das Skript
    # kann den Wert zwar setzen, angezeigt wird er dann aber nicht. Register und Rohwert sind
    # damit technisch editierbar; das ist wie im OFM-PresenceModule (gemessene Werte in
    # gelben, editierbaren Feldern) und harmlos.
    for n in (reg, raw, mean, typ, sca, off, ref):
        A('              <ParameterRef Id="%s" RefId="%s" />' % (gref(n), gid(n)))
A('            </ParameterRefs>')
A('          </Static>')

# ---------------------------------------------------------------- Dynamic
A('          <Dynamic>')
A('            <Channel>')
A('              <ParameterBlock Id="%AID%_PB-nnn" Name="SPVAssistant" Text="Assistent" Icon="loupe" HelpContext="SPV-Assistent">')
A('                <ParameterSeparator Id="%s" Text="Assistent" UIHint="Headline" />' % PS)
A('                <ParameterSeparator Id="%s" Text="Dieser Assistent ermittelt Transport, Seriennummer und Registerbelegung eines unbekannten Geräts und überträgt das Ergebnis in ein Gerät. Wird ein passendes Profil angeboten, ist er nicht nötig." UIHint="Information" />' % PS)
A('                <ParameterSeparator Id="%s" Text="Er arbeitet nur, wenn das Gerät bereits mit der ETS programmiert wurde. Andernfalls erst programmieren, dann eine andere Seite wählen und hierher zurückkehren." UIHint="Information" />' % PS)
A('                <ParameterSeparator Id="%s" Text="Ein Solarman-Logger beantwortet nur einen Client zuverlässig. Home Assistant oder andere Abfragen für das Testgerät währenddessen abschalten." UIHint="Information" />' % PS)
A('                <ParameterSeparator Id="%s" Text="Nichts davon verändert ein Gerät. Erst Schritt 4 ändert die Konfiguration - und auch die wird erst beim nächsten Download wirksam." UIHint="Information" />' % PS)
A('                <ParameterSeparator Id="%s" Text="" UIHint="HorizontalRuler" />' % PS)

A('                <ParameterSeparator Id="%s" Text="1. Gerät" UIHint="Headline" />' % PS)
for n in (1, 2, 3):
    A('                <ParameterRefRef IndentLevel="1" RefId="%s" HelpContext="SPV-Assistent" />' % gref(n))
A('                <Button Id="%AID%_B-%TT%00001" Text="Gerät analysieren" EventHandler="spvBtnAnalyse" EventHandlerOnline="ConnectionOriented" EventHandlerParameters="{ }" />')

A('                <ParameterSeparator Id="%s" Text="2. Erkannt" UIHint="Headline" />' % PS)
A('                <ParameterSeparator Id="%s" Text="{{0:Noch nichts analysiert.}}" TextParameterRefId="%s" />' % (PS, gref(7)))

A('                <ParameterSeparator Id="%s" Text="3. Register lesen" UIHint="Headline" />' % PS)
A('                <ParameterSeparator Id="%s" Text="Ein Klick genügt: Der Suchlauf durchsucht selbstständig alle Registerbereiche, die von den bekannten Geräten belegt werden, und trägt jeden gefundenen Wert ein. Wo diese Bereiche liegen, muss man nicht wissen." UIHint="Information" />' % PS)
A('                <ParameterSeparator Id="%s" Text="Er beginnt bei den niedrigen Adressen. Die gefundenen Register erscheinen laufend in der Tabelle — sobald deine dabei sind, kannst du abbrechen. Bei manchen Loggern dauert der vollständige Lauf einige Minuten, weil leere Bereiche in ein Zeitlimit laufen." UIHint="Information" />' % PS)
A('                <Button Id="%AID%_B-%TT%00005" Text="Belegte Register suchen" EventHandler="spvBtnScan" EventHandlerOnline="ConnectionOriented" EventHandlerParameters="{ }" />')
A('                <ParameterSeparator Id="%s" Text="" UIHint="HorizontalRuler" />' % PS)
A('                <ParameterSeparator Id="%s" Text="Nur bei Bedarf: einen bestimmten Bereich gezielt lesen, etwa um ein einzelnes Register nachzutragen." UIHint="Information" />' % PS)
A('                <ParameterRefRef IndentLevel="1" RefId="%s" HelpContext="SPV-Assistent" />' % gref(4))
A('                <ParameterRefRef IndentLevel="1" RefId="%s" HelpContext="SPV-Assistent" />' % gref(5))
A('                <Button Id="%AID%_B-%TT%00002" Text="Bereich lesen" EventHandler="spvBtnRead" EventHandlerOnline="ConnectionOriented" EventHandlerParameters="{ }" />')
A('                <ParameterSeparator Id="%s" Text="" UIHint="HorizontalRuler" />' % PS)
A('                <ParameterSeparator Id="%s" Text="4. Bedeutung zuordnen" UIHint="Headline" />' % PS)
A('                <ParameterSeparator Id="%s" Text="Welches Register welcher Messwert ist, sagt kein Rohwert von selbst — das ist der eine Schritt, der Handarbeit bleibt. Trage die Bedeutung zu jeder Zeile ein, die auf den Bus soll." UIHint="Information" />' % PS)
A('                <ParameterSeparator Id="%s" Text="Nur wo der Wertebereich eindeutig ist (Netzspannung, Netzfrequenz), schlägt der Suchlauf etwas vor. Solche Vorschläge sind geraten und zu prüfen." UIHint="Information" />' % PS)
A('                <ParameterSeparator Id="%s" Text="Der abgelesene Wert ist freiwillig, aber der einzige Weg zu einer sicheren Umrechnung: ohne ihn kann 113 sowohl 11,3 A als auch 1,13 A bedeuten. Er muss aus demselben Moment stammen wie der Rohwert." UIHint="Information" />' % PS)
A('                <Button Id="%AID%_B-%TT%00003" Text="Skalierung ermitteln" EventHandler="spvBtnDeriveScale" EventHandlerParameters="{ }" />')
A('                <ParameterSeparator Id="%s" Text="{{0:}}" TextParameterRefId="%s" />' % (PS, gref(7)))
# Echte Rastertabelle (Layout="Table"), Muster aus OFM-LightManager.
#
# WICHTIG: Alle Zellen stehen DIREKT im Tabellenblock, ohne <choose>. Zellen in einem
# <choose> blieben leer - skriptgeschriebene Werte wurden dort nicht angezeigt, auch nicht
# nach erneutem Laden der Seite. Die Geraetetabelle (gen_templ.py) macht es genauso und
# funktioniert; ebenso das OFM-PresenceModule mit festen, immer sichtbaren Messzellen.
# Preis: Es sind immer alle 64 Zeilen sichtbar, auch leere.
# Aufgeteilt in Bloecke zu ROWS_PER_TABLE Zeilen. EINE Tabelle mit allen 64 Zeilen (448 Zellen)
# rendert in der ETS nur das Geruest - Kopfzeile und Zeilennummern erscheinen, die Zellen
# bleiben leer. Die Geraetetabelle in gen_templ.py (max. 16 Zeilen je Kategorie) rendert
# dagegen einwandfrei, ebenso OFM-LightManager und OFM-PresenceModule mit je 10 Zeilen.
# Deshalb hier dieselbe Groessenordnung.
ROWS_PER_TABLE = 16
for t0 in range(0, len(SLOTS), ROWS_PER_TABLE):
    t1 = min(t0 + ROWS_PER_TABLE, len(SLOTS))
    ti = t0 // ROWS_PER_TABLE
    TID = P + 'AID' + P + '_PB-%sTT%sASTBL%d' % (P, P, ti)
    A('                <ParameterSeparator Id="%s" Text="Register %d bis %d" UIHint="Headline" />' % (PS, t0 + 1, t1))
    A('                <ParameterBlock Id="%s" Name="AsstTbl%d" Text="#" Inline="true" Layout="Table" HelpContext="SPV-Assistent">' % (TID, ti))
    A('                  <Rows>')
    for r in range(t0, t1):
        A('                    <Row Id="%s_R-%d" Name="R%d" Text="%d" />' % (TID, r - t0 + 1, r - t0 + 1, r + 1))
    A('                  </Rows>')
    A('                  <Columns>')
    A('                    <Column Id="%s_C-1" Name="C1" Text="Register" Width="14%s" />' % (TID, P))
    A('                    <Column Id="%s_C-2" Name="C2" Text="Rohwert" Width="12%s" />' % (TID, P))
    A('                    <Column Id="%s_C-3" Name="C3" Text="Bedeutung" Width="22%s" />' % (TID, P))
    A('                    <Column Id="%s_C-4" Name="C4" Text="Sollwert" Width="12%s" />' % (TID, P))
    A('                    <Column Id="%s_C-5" Name="C5" Text="Datentyp" Width="21%s" />' % (TID, P))
    A('                    <Column Id="%s_C-6" Name="C6" Text="Skal." Width="9%s" />' % (TID, P))
    A('                    <Column Id="%s_C-7" Name="C7" Text="Offset" Width="10%s" />' % (TID, P))
    A('                  </Columns>')
    for r in range(t0, t1):
        reg, raw, mean, typ, sca, off, ref = row_ids(r)
        row = r - t0 + 1
        A('                  <ParameterRefRef RefId="%s" Cell="%d,1" />' % (gref(reg), row))
        A('                  <ParameterRefRef RefId="%s" Cell="%d,2" />' % (gref(raw), row))
        A('                  <ParameterRefRef RefId="%s" Cell="%d,3" />' % (gref(mean), row))
        A('                  <ParameterRefRef RefId="%s" Cell="%d,4" />' % (gref(ref), row))
        A('                  <ParameterRefRef RefId="%s" Cell="%d,5" />' % (gref(typ), row))
        A('                  <ParameterRefRef RefId="%s" Cell="%d,6" />' % (gref(sca), row))
        A('                  <ParameterRefRef RefId="%s" Cell="%d,7" />' % (gref(off), row))
    A('                </ParameterBlock>')

A('                <ParameterSeparator Id="%s" Text="" UIHint="HorizontalRuler" />' % PS)
A('                <ParameterSeparator Id="%s" Text="5. Übernehmen" UIHint="Headline" />' % PS)
A('                <ParameterRefRef IndentLevel="1" RefId="%s" HelpContext="SPV-Assistent" />' % gref(8))
A('                <ParameterSeparator Id="%s" Text="Übernommen wird alles von dieser Seite: IP-Adresse, Port, Slave-ID, Transport, Seriennummer und jede zugeordnete Zeile. Auf der Geräteseite ist nichts nachzutragen. Die bisherige Messwerttabelle des Geräts wird dabei vollständig überschrieben." UIHint="Information" />' % PS)
A('                <Button Id="%AID%_B-%TT%00004" Text="In Zielgerät übernehmen" EventHandler="spvBtnApply" EventHandlerParameters="{ }" />')
A('              </ParameterBlock>')
A('            </Channel>')
A('          </Dynamic>')
A('        </ApplicationProgram>')
A('      </ApplicationPrograms>')
A('    </Manufacturer>')
A('  </ManufacturerData>')
A('</KNX>')

here = os.path.dirname(os.path.abspath(__file__))
io.open(os.path.join(here, "..", "src", "SolarmanPV.assistant.parts.xml"),
        "w", encoding="utf-8", newline="").write("\n".join(L) + "\n")

print("assistant.parts.xml: %d Messzeilen, Tabelle Byte %d-%d, Block %d Byte"
      % (len(SLOTS), TABLE_OFFSET, BLOCK_END - 1, BLOCK_END))
