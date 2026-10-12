#include "engine/compendium.h"
#include "engine/card.h"
#include "engine/combat.h"
#include "engine/monster.h"
#include "engine/potion.h"
#include "engine/relic.h"

namespace engine {

namespace {

// Index in the save bitset of entry `position` of the section: cards start after CardId::NONE.
uint16_t entry_index(CompendiumSection section, uint16_t position) {
    return section == CompendiumSection::CARDS ? static_cast<uint16_t>(position + 1) : position;
}

} // namespace

std::string_view compendium_section_name(CompendiumSection section) {
    switch (section) {
        case CompendiumSection::CARDS:    return "Cartes";
        case CompendiumSection::RELICS:   return "Reliques";
        case CompendiumSection::MONSTERS: return "Monstres";
        case CompendiumSection::POTIONS:  return "Potions";
        default:                          break;
    }
    return "";
}

uint16_t compendium_total(CompendiumSection section) {
    switch (section) {
        case CompendiumSection::CARDS:    return static_cast<uint16_t>(static_cast<uint16_t>(CardId::COUNT) - 1);
        case CompendiumSection::RELICS:   return static_cast<uint16_t>(get_all_relics().size());
        case CompendiumSection::MONSTERS: return static_cast<uint16_t>(get_all_monsters().size());
        case CompendiumSection::POTIONS:  return static_cast<uint16_t>(get_all_potions().size());
        default:                          break;
    }
    return 0;
}

bool compendium_seen(const SaveProfile& profile, CompendiumSection section, uint16_t index) {
    switch (section) {
        case CompendiumSection::CARDS:
            return index != static_cast<uint16_t>(CardId::NONE) && index < static_cast<uint16_t>(CardId::COUNT) &&
                   profile.is_card_discovered(index);
        case CompendiumSection::RELICS:
            return index < get_all_relics().size() && profile.is_relic_discovered(index);
        case CompendiumSection::MONSTERS:
            return index < get_all_monsters().size() && profile.is_monster_discovered(index);
        case CompendiumSection::POTIONS:
            return index < get_all_potions().size() && profile.is_potion_discovered(index);
        default:
            break;
    }
    return false;
}

uint16_t compendium_seen_count(const SaveProfile& profile, CompendiumSection section) {
    uint16_t total = compendium_total(section);
    uint16_t count = 0;
    for (uint16_t position = 0; position < total; ++position) {
        if (compendium_seen(profile, section, entry_index(section, position))) ++count;
    }
    return count;
}

size_t compendium_seen_entries(const SaveProfile& profile, CompendiumSection section, std::span<uint16_t> out) {
    uint16_t total = compendium_total(section);
    size_t written = 0;
    for (uint16_t position = 0; position < total && written < out.size(); ++position) {
        uint16_t index = entry_index(section, position);
        if (compendium_seen(profile, section, index)) out[written++] = index;
    }
    return written;
}

void discover_enemies(SaveProfile& profile, const CombatEngine& combat) {
    std::span<const MonsterData> monsters = get_all_monsters();
    for (size_t i = 0; i < combat.enemies.size(); ++i) {
        const MonsterData* sheet = monster_sheet(combat.enemies[i].type);
        if (sheet != nullptr && sheet >= monsters.data() && sheet < monsters.data() + monsters.size()) {
            profile.discover_monster(static_cast<uint16_t>(sheet - monsters.data()));
        }
    }
}

int compendium_first_row(int cursor, int count, int rows) {
    if (count <= rows || rows <= 0) return 0;
    int first = cursor - rows / 2;
    if (first > count - rows) first = count - rows;
    if (first < 0) first = 0;
    return first;
}

int compendium_move(int cursor, int step, int count) {
    if (count <= 0) return 0;
    int moved = cursor + step;
    if (moved < 0) moved = 0;
    if (moved > count - 1) moved = count - 1;
    return moved;
}

ProfileStats profile_stats(const SaveProfile& profile) {
    ProfileStats stats;
    stats.runs = profile.total_runs;
    stats.victories = profile.total_victories <= profile.total_runs ? profile.total_victories : profile.total_runs;
    stats.defeats = static_cast<uint16_t>(stats.runs - stats.victories);
    stats.win_percent = stats.runs == 0 ? 0 : static_cast<uint8_t>(uint32_t(stats.victories) * 100u / stats.runs);
    return stats;
}

uint8_t hero_ascension(const SaveProfile& profile, CharacterId hero) {
    uint8_t cid = static_cast<uint8_t>(hero);
    if (cid >= static_cast<uint8_t>(CharacterId::COUNT) || cid >= 5) return 0;
    uint8_t level = profile.max_ascension[cid];
    return level > 10 ? 10 : level;
}

} // namespace engine
