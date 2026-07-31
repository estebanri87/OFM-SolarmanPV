#pragma once
#include <stdint.h>

// ERZEUGT von tools/gen_templ.py aus tools/slots.py - nicht von Hand bearbeiten.
//
// Die Bedeutung jedes Slots ist fest vergeben; sie bestimmt KO-Name und DPT und ist
// deshalb kein Parameter (Begruendung in doc/Assistent-Umbau.md). Konfigurierbar sind
// je Zeile nur Register, Datentyp, Skalierung, Offset und ob der Slot sendet.

namespace Spv
{
    enum Encoding : uint8_t
    {
        EncFloat,     // DPT 14.x, Gleitkomma mit slot-eigenem Subtyp
        EncEnergyWh,  // DPT 13.010, Wattstunden ganzzahlig
        EncPercent,   // DPT 5.001
        EncTemp,      // DPT 9.001
        EncU16,       // DPT 7.x
        EncU8,        // DPT 5.010
    };

    struct Slot
    {
        const char* label;
        uint8_t encoding;
        uint8_t dptMain;
        uint8_t dptSub;
    };

    // Zeilenlayout im Parameterspeicher, muss zu gen_templ.py passen.
    static const uint16_t TABLE_OFFSET = 57;
    static const uint8_t ROW_BYTES = 4;
    static const uint8_t SLOT_COUNT = 64;

    // Datentyp der Zeile (Feld "Datentyp").
    enum RowType : uint8_t
    {
        TypeU16 = 0,
        TypeS16 = 1,
        TypeU32 = 2,
        TypeS32 = 3,
        TypeProduct = 4, // Produkt der zwei vorangehenden Zeilen (Spannung * Strom)
    };

    // Faktoren zum Feld "Skalierung", gleiche Reihenfolge wie PT-SPVScale.
    static const float SCALE[8] = {1.0f, 0.1f, 0.01f, 0.001f, 10.0f, 1.0f, 1.0f, 1.0f};

    static const Slot SLOTS[SLOT_COUNT] = {
        {"Wirkleistung", EncFloat,   14, 56}, //  1 Power
        {"Scheinleistung", EncFloat,   14, 56}, //  2 ApparentPower
        {"Netzspannung", EncFloat,   14, 27}, //  3 GridVoltage
        {"Netzspannung L2", EncFloat,   14, 27}, //  4 GridVoltageL2
        {"Netzspannung L3", EncFloat,   14, 27}, //  5 GridVoltageL3
        {"Netzstrom", EncFloat,   14, 19}, //  6 GridCurrent
        {"Netzfrequenz", EncFloat,   14, 33}, //  7 GridFrequency
        {"Betriebszustand", EncU8,       5, 10}, //  8 OperatingState
        {"Tagesertrag", EncEnergyWh, 13, 10}, //  9 Today
        {"Gesamtertrag", EncEnergyWh, 13, 10}, // 10 Total
        {"Monatsertrag", EncEnergyWh, 13, 10}, // 11 Month
        {"Jahresertrag", EncEnergyWh, 13, 10}, // 12 Year
        {"Tagesertrag String 1", EncEnergyWh, 13, 10}, // 13 Today1
        {"Tagesertrag String 2", EncEnergyWh, 13, 10}, // 14 Today2
        {"Gesamtertrag String 1", EncEnergyWh, 13, 10}, // 15 Total1
        {"Gesamtertrag String 2", EncEnergyWh, 13, 10}, // 16 Total2
        {"PV1 Spannung", EncFloat,   14, 27}, // 17 Pv1Voltage
        {"PV1 Strom", EncFloat,   14, 19}, // 18 Pv1Current
        {"PV1 Leistung", EncFloat,   14, 56}, // 19 Pv1Power
        {"PV1 Tagesertrag", EncEnergyWh, 13, 10}, // 20 Pv1Today
        {"PV2 Spannung", EncFloat,   14, 27}, // 21 Pv2Voltage
        {"PV2 Strom", EncFloat,   14, 19}, // 22 Pv2Current
        {"PV2 Leistung", EncFloat,   14, 56}, // 23 Pv2Power
        {"PV2 Tagesertrag", EncEnergyWh, 13, 10}, // 24 Pv2Today
        {"PV3 Spannung", EncFloat,   14, 27}, // 25 Pv3Voltage
        {"PV3 Strom", EncFloat,   14, 19}, // 26 Pv3Current
        {"PV3 Leistung", EncFloat,   14, 56}, // 27 Pv3Power
        {"PV3 Tagesertrag", EncEnergyWh, 13, 10}, // 28 Pv3Today
        {"PV4 Spannung", EncFloat,   14, 27}, // 29 Pv4Voltage
        {"PV4 Strom", EncFloat,   14, 19}, // 30 Pv4Current
        {"PV4 Leistung", EncFloat,   14, 56}, // 31 Pv4Power
        {"PV4 Tagesertrag", EncEnergyWh, 13, 10}, // 32 Pv4Today
        {"Batterie Ladezustand", EncPercent,  5,  1}, // 33 Soc
        {"Batterie Alterungszustand", EncPercent,  5,  1}, // 34 Soh
        {"Batteriespannung", EncFloat,   14, 27}, // 35 BattVoltage
        {"Batteriestrom", EncFloat,   14, 19}, // 36 BattCurrent
        {"Batterieleistung", EncFloat,   14, 56}, // 37 BattPower
        {"Batterietemperatur", EncTemp,     9,  1}, // 38 BattTemperature
        {"Ladezyklen", EncU16,      7,  1}, // 39 CycleTimes
        {"Restkapazität", EncEnergyWh, 13, 10}, // 40 RemainingCapacity
        {"Heute geladen", EncEnergyWh, 13, 10}, // 41 TodayCharge
        {"Heute entladen", EncEnergyWh, 13, 10}, // 42 TodayDischarge
        {"Gesamt geladen", EncEnergyWh, 13, 10}, // 43 TotalCharge
        {"Gesamt entladen", EncEnergyWh, 13, 10}, // 44 TotalDischarge
        {"Zellspannung maximal", EncFloat,   14, 27}, // 45 CellVoltageMax
        {"Zellspannung minimal", EncFloat,   14, 27}, // 46 CellVoltageMin
        {"Zelltemperatur maximal", EncTemp,     9,  1}, // 47 CellTempMax
        {"Zelltemperatur minimal", EncTemp,     9,  1}, // 48 CellTempMin
        {"Hausverbrauch", EncFloat,   14, 56}, // 49 HousePower
        {"Netzbezug Leistung", EncFloat,   14, 56}, // 50 ImportPower
        {"Einspeisung Leistung", EncFloat,   14, 56}, // 51 ExportPower
        {"Netzbezug gesamt", EncEnergyWh, 13, 10}, // 52 ImportTotal
        {"Einspeisung gesamt", EncEnergyWh, 13, 10}, // 53 ExportTotal
        {"Hausverbrauch heute", EncEnergyWh, 13, 10}, // 54 HouseToday
        {"Netzbezug heute", EncEnergyWh, 13, 10}, // 55 ImportToday
        {"Einspeisung heute", EncEnergyWh, 13, 10}, // 56 ExportToday
        {"Gerätetemperatur", EncTemp,     9,  1}, // 57 Temperature
        {"Kühlkörpertemperatur", EncTemp,     9,  1}, // 58 HeatsinkTemp
        {"Fehlercode", EncU16,      7,  1}, // 59 ErrorCode
        {"Betriebsstunden", EncU16,      7,  7}, // 60 OperatingHours
        {"Freier Wert 1", EncFloat,   14, 56}, // 61 Spare1
        {"Freier Wert 2", EncFloat,   14, 56}, // 62 Spare2
        {"Freier Wert 3", EncFloat,   14, 56}, // 63 Spare3
        {"Freier Wert 4", EncFloat,   14, 56}, // 64 Spare4
    };
} // namespace Spv
