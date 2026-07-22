#pragma once
#include "DeyeMicro.h"
#include "PylontechForce.h"

// Laufzeit-Auswahl des Profils. Der Kanal arbeitet nur noch gegen diese Beschreibung und
// kennt die konkreten Geraete nicht - ein weiteres Profil kostet damit nur eine Tabelle
// und einen Eintrag hier.

namespace Spv
{
    struct Info
    {
        const Def* defs;
        uint8_t count;
        const Block* blocks;
        uint8_t blockCount;
        const char* name;
    };

    inline Info profileFor(uint8_t profileId)
    {
        if (profileId == PylontechForceId)
            return {PylontechForce::DEFS, (uint8_t)PylontechForce::Count,
                    PylontechForce::BLOCKS, PylontechForce::BLOCK_COUNT, "Pylontech Force"};
        return {DeyeMicro::DEFS, (uint8_t)DeyeMicro::Count,
                DeyeMicro::BLOCKS, DeyeMicro::BLOCK_COUNT, "Deye Mikrowechselrichter"};
    }

    // Groesster Block ueber alle Profile - bestimmt den Puffer im Kanal.
    static const uint8_t MAX_BLOCK_WORDS = 16;
    // Ausbaureserve, muss mit SLOTS im Template-Generator uebereinstimmen: so viele
    // Messwert-KOs hat jeder Kanal. Ein Profil darf hoechstens so viele Werte haben.
    static const uint8_t MAX_VALUES = 24;
    // Reservierte Profilplaetze (Enable-Bits im Parameterspeicher). Profile bis zu dieser
    // Zahl lassen sich ergaenzen, ohne Parameteradressen oder KO-Nummern zu verschieben.
    static const uint8_t MAX_PROFILES = 10;
} // namespace Spv
