#pragma once
#include "Profile.h"

// Registertabelle Pylontech Force H3 (Hochvolt-Batteriespeicher).
//
// AM REALEN GERAET VERIFIZIERT (2026-07-22, 192.168.20.31): Registerkarte aus
// pylontech_force.yaml (ha-solarman, MIT-Lizenz) uebernommen und Wert fuer Wert gegen die
// Anzeige der Solarman-Integration geprueft:
//   SoC 80 %, Strom 0,00 A, Zyklen 6, Restkapazitaet 8,9 kWh, SOH 100 %,
//   Temperatur 35,0 °C, Spannung 213 V  -- alle exakt getroffen.
// Quervalidierung: 2 Module a 107 V = 214 V gegen gemessene 213,4 V.
//
// WICHTIG: Dieses Geraet spricht **reines Modbus TCP** (MBAP-Header), NICHT Solarman V5.
// Auf einen korrekten V5-Rahmen antwortet es mit MBAP. Die Seriennummer-Autoerkennung greift
// hier nicht - das Geraet verwirft Frames mit falscher SN stillschweigend, sie muss also
// manuell hinterlegt werden (Testgeraet: 640).
//
// Bewusst NICHT aufgenommen: Zell- und Modulwerte (64 Zellen, ab Register 5376 bzw. 6144).
// Die gehoeren in ein Monitoring-System, nicht auf den KNX-Bus.

namespace PylontechForce
{
    enum Value : uint8_t
    {
        Soc = 0,
        Voltage,
        Current,
        Power, // berechnet U*I
        Temperature,
        Soh,
        RemainingCapacity,
        CycleTimes,
        TodayCharge,
        TodayDischarge,
        TotalCharge,
        TotalDischarge,
        Count
    };

    static const Spv::Def DEFS[Count] = {
        /* Soc               */ {5127, 1, false, 1.0f, 0.0f, Spv::Percent},
        /* Voltage           */ {5123, 1, false, 0.1f, 0.0f, Spv::Float32},
        /* Current           */ {5125, 1, true, 0.01f, 0.0f, Spv::Float32},
        /* Power             */ {0, 0, false, 0.0f, 0.0f, Spv::Float32},
        /* Temperature       */ {5126, 1, true, 0.1f, 0.0f, Spv::Temp2},
        /* Soh               */ {5152, 1, false, 1.0f, 0.0f, Spv::Percent},
        /* RemainingCapacity */ {5154, 2, false, 0.001f, 0.0f, Spv::EnergyWh},
        /* CycleTimes        */ {5128, 1, false, 1.0f, 0.0f, Spv::Counter16},
        /* TodayCharge       */ {5160, 2, false, 0.001f, 0.0f, Spv::EnergyWh},
        /* TodayDischarge    */ {5162, 2, false, 0.001f, 0.0f, Spv::EnergyWh},
        /* TotalCharge       */ {5164, 2, false, 1.0f, 0.0f, Spv::EnergyWh},
        /* TotalDischarge    */ {5166, 2, false, 1.0f, 0.0f, Spv::EnergyWh},
    };

    static const Spv::Block BLOCKS[] = {
        {5120, 16}, // Spannung, Strom, Temperatur, SoC, Zyklen
        {5152, 16}, // SOH, Restkapazitaet, Tages- und Gesamtwerte
    };
    static const uint8_t BLOCK_COUNT = sizeof(BLOCKS) / sizeof(BLOCKS[0]);
} // namespace PylontechForce
