#pragma once
#include <stdint.h>

// Gemeinsame Struktur aller Geraeteprofile.
//
// Ein Profil beschreibt, welche Register gelesen werden (BLOCKS) und wie die Rohwerte in
// physikalische Groessen umzurechnen sind (DEFS). Alle Angaben stammen aus Messungen am
// realen Geraet, nicht aus uebernommenen Registerkarten.

namespace Spv
{
    // words == 0 -> berechneter Wert (kein Register), z.B. Leistung aus U*I.
    // Bei words == 2 gilt LOW-WORD ZUERST: das High-Word steht im Register dahinter.
    // Diese Konvention ist bei beiden vermessenen Geraeten bestaetigt.
    struct Def
    {
        uint16_t reg;
        uint8_t words;
        bool isSigned; // 16 Bit vorzeichenbehaftet (z.B. Batteriestrom, Temperatur)
        float scale;
        float offset; // wird NACH der Skalierung abgezogen
    };

    struct Block
    {
        uint16_t start;
        uint8_t count; // max. 16 - groessere Bloecke liefen am Geraet in einen Timeout
    };

    // Transport je Geraet. Von aussen nicht unterscheidbar: beide laufen ueber Port 8899.
    enum Transport : uint8_t
    {
        SolarmanV5 = 0, // Modbus RTU im Solarman-V5-Rahmen (Deye SUN-M80G3)
        ModbusTcp = 1,  // Modbus RTU mit MBAP-Header (Pylontech Force H3)
    };

    enum ProfileId : uint8_t
    {
        DeyeMicroId = 0,
        PylontechForceId = 1,
    };
} // namespace Spv
