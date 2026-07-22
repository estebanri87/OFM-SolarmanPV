#pragma once
#include "Profile.h"

// Registertabelle Deye SUN-M80G3 (Mikrowechselrichter mit integriertem Logger).
//
// VOLLSTAENDIG AM REALEN GERAET AUFGENOMMEN (2026-07-22): Fuer jeden Wert wurden Rohregister
// und Anzeigewert der Solarman-Integration im SELBEN Moment verglichen. Keine Uebernahme aus
// fremden Registerkarten, kein Raten. Die Stroeme stimmten auf die Nachkommastelle
// (0x006E = 113 <-> 11,30 A), damit sind die Faktoren belegt.
//
// Zwei Besonderheiten:
//   - Die Energiezaehler sind 32 Bit, LOW-WORD ZUERST (das High-Word steht dahinter und war
//     jeweils 0). Als 16 Bit gelesen laufen sie bei 6553,5 kWh ueber.
//   - Die Temperatur hat einen Offset: Roh/100 - 10. Ohne Vergleichswert nicht zu erraten
//     (6160 -> 51,6 °C, nicht 61,6 °C).
//
// Die PV-Leistungen liefert das Geraet nicht; sie werden wie in der Solarman-Integration
// aus U*I berechnet.

namespace DeyeMicro
{
    enum Value : uint8_t
    {
        Power = 0,      // Wirkleistung
        Today,          // Tagesertrag gesamt
        Total,          // Gesamtertrag
        GridVoltage,
        GridCurrent,
        GridFrequency,
        Temperature,
        Pv1Voltage,
        Pv1Current,
        Pv1Power,       // berechnet
        Pv2Voltage,
        Pv2Current,
        Pv2Power,       // berechnet
        Today1,
        Today2,
        Total1,
        Total2,
        Count
    };

    static const Spv::Def DEFS[Count] = {
        /* Power         */ {0x0056, 1, false, 0.1f, 0.0f},
        /* Today         */ {0x003C, 1, false, 0.1f, 0.0f},
        /* Total         */ {0x003F, 2, false, 0.1f, 0.0f},
        /* GridVoltage   */ {0x0049, 1, false, 0.1f, 0.0f},
        /* GridCurrent   */ {0x004C, 1, false, 0.1f, 0.0f},
        /* GridFrequency */ {0x004F, 1, false, 0.01f, 0.0f},
        /* Temperature   */ {0x005A, 1, false, 0.01f, 10.0f},
        /* Pv1Voltage    */ {0x006D, 1, false, 0.1f, 0.0f},
        /* Pv1Current    */ {0x006E, 1, false, 0.1f, 0.0f},
        /* Pv1Power      */ {0x0000, 0, false, 0.0f, 0.0f},
        /* Pv2Voltage    */ {0x006F, 1, false, 0.1f, 0.0f},
        /* Pv2Current    */ {0x0070, 1, false, 0.1f, 0.0f},
        /* Pv2Power      */ {0x0000, 0, false, 0.0f, 0.0f},
        /* Today1        */ {0x0041, 1, false, 0.1f, 0.0f},
        /* Today2        */ {0x0042, 1, false, 0.1f, 0.0f},
        /* Total1        */ {0x0045, 2, false, 0.1f, 0.0f},
        /* Total2        */ {0x0047, 2, false, 0.1f, 0.0f},
    };

    // Leseblöcke: je <= 16 Register. Groessere Anfragen (32) liefen am Geraet in einen
    // Timeout - der Logger begrenzt die Blockgroesse fuer belegte Bereiche.
    static const Spv::Block BLOCKS[] = {
        {0x003B, 16}, // Ertraege + Netzspannung
        {0x004B, 16}, // Netzstrom, Frequenz, Wirkleistung, Temperatur
        {0x006D, 4},  // PV1/PV2 Spannung und Strom
    };
    static const uint8_t BLOCK_COUNT = sizeof(BLOCKS) / sizeof(BLOCKS[0]);
} // namespace DeyeMicro
