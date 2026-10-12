#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include "engine/character.h"
#include "engine/save_system.h"

// Compendium and statistics screens (#273): what a profile has met, read from the discovery bitsets of
// SaveProfile, and the scrolling of the lists the GBA scene shows.
namespace engine {

class CombatEngine;

enum class CompendiumSection : uint8_t {
    CARDS,    // indexed by CardId (as SaveManager::autosave_run discovers them), CardId::NONE left out
    RELICS,   // indexed in get_all_relics()
    MONSTERS, // indexed in get_all_monsters()
    POTIONS,  // indexed in get_all_potions()
    COUNT
};

inline constexpr int COMPENDIUM_SECTION_COUNT = static_cast<int>(CompendiumSection::COUNT);

// "Cartes", "Reliques", "Monstres", "Potions".
[[nodiscard]] std::string_view compendium_section_name(CompendiumSection section);

// Number of entries the section can list once everything is met.
[[nodiscard]] uint16_t compendium_total(CompendiumSection section);

// Whether the profile has met entry `index` of the section.
[[nodiscard]] bool compendium_seen(const SaveProfile& profile, CompendiumSection section, uint16_t index);

// Entries of the section the profile has met.
[[nodiscard]] uint16_t compendium_seen_count(const SaveProfile& profile, CompendiumSection section);

// Writes the indexes of the entries met, in the order of the database, into `out`; returns how many were
// written (at most out.size()).
size_t compendium_seen_entries(const SaveProfile& profile, CompendiumSection section, std::span<uint16_t> out);

// Marks every enemy of the fight that has a sheet in the monster database as met.
void discover_enemies(SaveProfile& profile, const CombatEngine& combat);

// First row shown of a list of `count` rows of which `rows` fit on the screen, keeping `cursor` in view and
// near the middle.
[[nodiscard]] int compendium_first_row(int cursor, int count, int rows);

// Cursor after a move of `step` rows (a page with the left and right keys), held between 0 and count - 1.
[[nodiscard]] int compendium_move(int cursor, int step, int count);

// Statistics of a profile. Runs and victories are counted for the whole profile; the Ascension is per hero.
struct ProfileStats {
    uint16_t runs = 0;
    uint16_t victories = 0;
    uint16_t defeats = 0;
    uint8_t win_percent = 0; // victories * 100 / runs, 0 before the first run
};

[[nodiscard]] ProfileStats profile_stats(const SaveProfile& profile);

// Highest Ascension level the profile has unlocked with that hero (0 to 10).
[[nodiscard]] uint8_t hero_ascension(const SaveProfile& profile, CharacterId hero);

} // namespace engine
