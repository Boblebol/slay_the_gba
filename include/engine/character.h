#pragma once
#include <cstdint>
#include <string_view>
#include <span>

namespace engine {

enum class CharacterId : uint8_t {
    IRONCLAD = 0,
    SILENT,
    DEFECT,
    REGENT,
    NECROBINDER,
    COUNT
};

struct StarterDeckCard {
    std::string_view card_id;
    uint8_t count;
};

struct CharacterDef {
    CharacterId id;
    std::string_view name;
    std::string_view english_name;
    int16_t starting_hp;
    int16_t starting_gold;
    uint8_t starting_energy;
    uint8_t potion_slots;
    std::string_view starting_relic_id;
    std::string_view special_mechanic;
    uint16_t theme_color_bgr555; // 15-bit GBA color
    std::span<const StarterDeckCard> starter_deck;
};

enum class PaletteVariant : uint8_t {
    CLASSIC = 0, // Teinte canonique du héros
    SHINY,       // Teinte chromatique alternative
    SHADOW,      // Teinte sombre / ténébreuse
    GOLDEN,      // Teinte dorée / prestige
    COUNT
};

struct PaletteDef {
    PaletteVariant variant;
    std::string_view name;
    std::string_view description;
    uint16_t primary_color_bgr555;
    uint16_t secondary_color_bgr555;
};

const CharacterDef& get_character(CharacterId id);
std::span<const CharacterDef> get_all_characters();
std::span<const PaletteDef> get_character_palettes(CharacterId id);

} // namespace engine
