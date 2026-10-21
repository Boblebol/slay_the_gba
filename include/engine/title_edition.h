#pragma once
// Identity of the title screen of each cartridge (#3): edition name under the logo, colour theme of the key art and
// version tag. Kept free of any Butano type so that the engine tests cover it.
#include <cstdint>
#include <string_view>

#include "engine/character.h"

namespace engine::title_edition {

// Colour of the edition label; the GBA scene maps it to its own text colours.
enum class Accent : uint8_t { RED, GREEN, BLUE, GOLD, PURPLE };

struct Theme {
    std::string_view edition;   // "Édition Soldat de Fer", same names as the website and the booklets
    uint16_t tint_bgr555;       // colour the key art and the Spire halo are faded towards
    uint8_t tint_strength;      // 0..32: how far (32nds), kept low so that the key art stays readable
    Accent accent;
};

// Palette per edition. Ironclad: red / hell (gold label: red text vanishes on the red art). Silent: toxic green / night. Defect: neon blue / electric.
// Regent: royal violet / gold. Necrobinder: deep violet / bone. (The issue names a "Watcher" edition that does
// not exist in this game: the two violet editions stand in for it.)
constexpr Theme theme(CharacterId hero) {
    switch (hero) {
        case CharacterId::SILENT:      return { "Édition La Silencieuse", 0x0B40, 10, Accent::GREEN };
        case CharacterId::DEFECT:      return { "Édition Le Défectueux",  0x7E80, 10, Accent::BLUE };
        case CharacterId::REGENT:      return { "Édition Le Régent",      0x5C0F, 10, Accent::GOLD };
        case CharacterId::NECROBINDER: return { "Édition La Nécrolieuse", 0x4C0E, 10, Accent::PURPLE };
        default:                       return { "Édition Soldat de Fer",  0x0C1F, 10, Accent::GOLD };
    }
}

// Version tag shown in the corner: "v0.98.6". A build without a version (SLAY_VERSION undefined) shows "dev".
// Writes into `out` (at most `capacity` chars, no terminator) and returns the length.
constexpr int format_version(std::string_view version, char* out, int capacity) {
    if (version.empty()) {
        version = "dev";
    }
    int length = 0;
    if (version != "dev" && version[0] != 'v') {
        if (capacity > 0) { out[length++] = 'v'; }
    }
    for (char c : version) {
        if (length >= capacity) { break; }
        out[length++] = c;
    }
    return length;
}

}  // namespace engine::title_edition
