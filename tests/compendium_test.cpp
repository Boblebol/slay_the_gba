// Compendium and statistics (#273): what a profile has met, the scrolling of the lists, the stats per hero.
#include <cassert>
#include <cstdint>
#include <iostream>
#include "engine/card.h"
#include "engine/combat.h"
#include "engine/compendium.h"
#include "engine/monster.h"
#include "engine/potion.h"
#include "engine/relic.h"
#include "engine/run.h"
#include "engine/save_system.h"

using namespace engine;

namespace {

void test_totals_cover_the_databases() {
    assert(compendium_total(CompendiumSection::CARDS) == static_cast<uint16_t>(CardId::COUNT) - 1);
    assert(compendium_total(CompendiumSection::RELICS) == get_all_relics().size());
    assert(compendium_total(CompendiumSection::MONSTERS) == get_all_monsters().size());
    assert(compendium_total(CompendiumSection::POTIONS) == get_all_potions().size());
    // Every entry fits the discovery bitsets of the save.
    assert(static_cast<size_t>(CardId::COUNT) <= CARD_BITSET_BYTES * 8);
    assert(get_all_relics().size() <= RELIC_BITSET_BYTES * 8);
    assert(get_all_monsters().size() <= MONSTER_BITSET_BYTES * 8);
    assert(get_all_potions().size() <= POTION_BITSET_BYTES * 8);
    for (int s = 0; s < COMPENDIUM_SECTION_COUNT; ++s) {
        assert(! compendium_section_name(static_cast<CompendiumSection>(s)).empty());
    }
    std::cout << "  totals: OK\n";
}

void test_new_profile_has_met_nothing() {
    SaveProfile profile;
    uint16_t entries[8] = {};
    for (int s = 0; s < COMPENDIUM_SECTION_COUNT; ++s) {
        CompendiumSection section = static_cast<CompendiumSection>(s);
        assert(compendium_seen_count(profile, section) == 0);
        assert(compendium_seen_entries(profile, section, entries) == 0);
    }
    std::cout << "  empty profile: OK\n";
}

void test_lists_hold_what_was_met_in_order() {
    SaveProfile profile;
    profile.discover_card(static_cast<uint16_t>(CardId::NONE)); // never listed
    profile.discover_card(40);
    profile.discover_card(3);
    profile.discover_relic(7);
    profile.discover_monster(0);
    profile.discover_monster(111);
    profile.discover_potion(63);

    uint16_t entries[600] = {};
    assert(compendium_seen_count(profile, CompendiumSection::CARDS) == 2);
    assert(compendium_seen_entries(profile, CompendiumSection::CARDS, entries) == 2);
    assert(entries[0] == 3 && entries[1] == 40);
    assert(compendium_seen(profile, CompendiumSection::CARDS, 40));
    assert(! compendium_seen(profile, CompendiumSection::CARDS, 0));

    assert(compendium_seen_entries(profile, CompendiumSection::RELICS, entries) == 1 && entries[0] == 7);
    assert(compendium_seen_entries(profile, CompendiumSection::MONSTERS, entries) == 2);
    assert(entries[0] == 0 && entries[1] == 111);
    assert(compendium_seen_entries(profile, CompendiumSection::POTIONS, entries) == 1 && entries[0] == 63);

    // A short buffer is filled without overflowing.
    uint16_t one[1] = {};
    assert(compendium_seen_entries(profile, CompendiumSection::CARDS, one) == 1 && one[0] == 3);
    std::cout << "  lists of what was met: OK\n";
}

void test_a_run_discovers_its_deck_relics_and_potions() {
    SaveManager& save = SaveManager::instance();
    save.reset_to_defaults();
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 7);
    run.potions.push_back(get_all_potions()[0].id);
    save.autosave_run(run);

    const SaveProfile& profile = save.get_active_profile();
    assert(compendium_seen(profile, CompendiumSection::CARDS, static_cast<uint16_t>(run.master_deck[0].id)));
    assert(compendium_seen_count(profile, CompendiumSection::CARDS) >= 2); // Strike, Defend, Bash...
    assert(compendium_seen(profile, CompendiumSection::POTIONS, 0));
    for (std::string_view relic : run.relics) {
        const RelicData* data = find_relic(relic);
        if (data != nullptr) {
            assert(compendium_seen(profile, CompendiumSection::RELICS,
                                   static_cast<uint16_t>(data - get_all_relics().data())));
        }
    }
    save.reset_to_defaults();
    std::cout << "  a run discovers its cards, relics and potions: OK\n";
}

void test_a_fight_discovers_its_monsters() {
    SaveProfile profile;
    CombatEngine combat;
    combat.start_combat(EnemyType::FUZZY_WURM_CRAWLER);
    discover_enemies(profile, combat);

    const MonsterData* sheet = monster_sheet(EnemyType::FUZZY_WURM_CRAWLER);
    assert(sheet != nullptr);
    uint16_t index = static_cast<uint16_t>(sheet - get_all_monsters().data());
    assert(compendium_seen(profile, CompendiumSection::MONSTERS, index));
    assert(compendium_seen_count(profile, CompendiumSection::MONSTERS) == 1);

    // A monster without a sheet (kept from the first game) is not listed.
    SaveProfile other;
    CombatEngine legacy;
    legacy.start_combat(EnemyType::CULTIST);
    assert(monster_sheet(EnemyType::CULTIST) == nullptr);
    discover_enemies(other, legacy);
    assert(compendium_seen_count(other, CompendiumSection::MONSTERS) == 0);
    std::cout << "  a fight discovers its monsters: OK\n";
}

void test_scrolling() {
    // Short list: no scroll.
    assert(compendium_first_row(0, 4, 6) == 0);
    assert(compendium_first_row(3, 4, 6) == 0);
    // Long list: the cursor stays in view, near the middle, without scrolling past the ends.
    const int count = 40, rows = 6;
    for (int cursor = 0; cursor < count; ++cursor) {
        int first = compendium_first_row(cursor, count, rows);
        assert(first >= 0 && first <= count - rows);
        assert(cursor >= first && cursor < first + rows);
    }
    assert(compendium_first_row(0, count, rows) == 0);
    assert(compendium_first_row(count - 1, count, rows) == count - rows);
    assert(compendium_first_row(20, count, rows) == 17);

    assert(compendium_move(0, -1, count) == 0);
    assert(compendium_move(0, 1, count) == 1);
    assert(compendium_move(3, rows, count) == 9);
    assert(compendium_move(38, rows, count) == count - 1);
    assert(compendium_move(2, -rows, count) == 0);
    assert(compendium_move(5, 1, 0) == 0);
    std::cout << "  scrolling: OK\n";
}

void test_statistics() {
    SaveProfile profile;
    ProfileStats empty = profile_stats(profile);
    assert(empty.runs == 0 && empty.victories == 0 && empty.defeats == 0 && empty.win_percent == 0);

    SaveManager& save = SaveManager::instance();
    save.reset_to_defaults();
    save.record_victory(CharacterId::IRONCLAD, 0);
    save.record_victory(CharacterId::IRONCLAD, 1);
    save.record_defeat();
    save.record_victory(CharacterId::SILENT, 0);
    const SaveProfile& played = save.get_active_profile();

    ProfileStats stats = profile_stats(played);
    assert(stats.runs == 4 && stats.victories == 3 && stats.defeats == 1 && stats.win_percent == 75);
    assert(hero_ascension(played, CharacterId::IRONCLAD) == 2);
    assert(hero_ascension(played, CharacterId::SILENT) == 1);
    assert(hero_ascension(played, CharacterId::DEFECT) == 0);
    assert(hero_ascension(played, CharacterId::COUNT) == 0);
    save.reset_to_defaults();
    std::cout << "  statistics: OK\n";
}

} // namespace

int main() {
    std::cout << "compendium test\n";
    test_totals_cover_the_databases();
    test_new_profile_has_met_nothing();
    test_lists_hold_what_was_met_in_order();
    test_a_run_discovers_its_deck_relics_and_potions();
    test_a_fight_discovers_its_monsters();
    test_scrolling();
    test_statistics();
    std::cout << "ALL OK\n";
    return 0;
}
