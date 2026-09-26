#include "engine/save_system.h"
#include "engine/card_db.h"
#include "engine/relic.h"
#include "engine/monster.h"
#include "engine/potion.h"
#include <cstring>
#include <algorithm>

namespace engine {

namespace {

inline void set_bit(uint8_t* bitset, size_t max_bytes, uint16_t index) {
    size_t byte_idx = index / 8;
    if (byte_idx < max_bytes) {
        bitset[byte_idx] |= (1 << (index % 8));
    }
}

inline bool get_bit(const uint8_t* bitset, size_t max_bytes, uint16_t index) {
    size_t byte_idx = index / 8;
    if (byte_idx < max_bytes) {
        return (bitset[byte_idx] & (1 << (index % 8))) != 0;
    }
    return false;
}

inline uint16_t count_bits(const uint8_t* bitset, size_t max_bytes) {
    uint16_t count = 0;
    for (size_t i = 0; i < max_bytes; ++i) {
        uint8_t b = bitset[i];
        while (b) {
            count += (b & 1);
            b >>= 1;
        }
    }
    return count;
}

} // namespace

void SaveProfile::discover_card(uint16_t card_index) {
    set_bit(discovered_cards, CARD_BITSET_BYTES, card_index);
}

bool SaveProfile::is_card_discovered(uint16_t card_index) const {
    return get_bit(discovered_cards, CARD_BITSET_BYTES, card_index);
}

uint16_t SaveProfile::count_discovered_cards() const {
    return count_bits(discovered_cards, CARD_BITSET_BYTES);
}

void SaveProfile::discover_relic(uint16_t relic_index) {
    set_bit(discovered_relics, RELIC_BITSET_BYTES, relic_index);
}

bool SaveProfile::is_relic_discovered(uint16_t relic_index) const {
    return get_bit(discovered_relics, RELIC_BITSET_BYTES, relic_index);
}

uint16_t SaveProfile::count_discovered_relics() const {
    return count_bits(discovered_relics, RELIC_BITSET_BYTES);
}

void SaveProfile::discover_monster(uint16_t monster_index) {
    set_bit(discovered_monsters, MONSTER_BITSET_BYTES, monster_index);
}

bool SaveProfile::is_monster_discovered(uint16_t monster_index) const {
    return get_bit(discovered_monsters, MONSTER_BITSET_BYTES, monster_index);
}

uint16_t SaveProfile::count_discovered_monsters() const {
    return count_bits(discovered_monsters, MONSTER_BITSET_BYTES);
}

void SaveProfile::discover_potion(uint16_t potion_index) {
    set_bit(discovered_potions, POTION_BITSET_BYTES, potion_index);
}

bool SaveProfile::is_potion_discovered(uint16_t potion_index) const {
    return get_bit(discovered_potions, POTION_BITSET_BYTES, potion_index);
}

uint16_t SaveProfile::count_discovered_potions() const {
    return count_bits(discovered_potions, POTION_BITSET_BYTES);
}

void SaveProfile::discover_ancient(uint8_t ancient_index) {
    set_bit(discovered_ancients, ANCIENT_BITSET_BYTES, ancient_index);
}

bool SaveProfile::is_ancient_discovered(uint8_t ancient_index) const {
    return get_bit(discovered_ancients, ANCIENT_BITSET_BYTES, ancient_index);
}

uint8_t SaveProfile::count_discovered_ancients() const {
    return static_cast<uint8_t>(count_bits(discovered_ancients, ANCIENT_BITSET_BYTES));
}

SaveManager& SaveManager::instance() {
    static SaveManager mgr;
    if (!mgr.initialized_) {
        mgr.init();
    }
    return mgr;
}

void SaveManager::init() {
    if (initialized_) return;
    initialized_ = true;

    // Initialize 3 default profiles
    for (size_t i = 0; i < PROFILE_COUNT; ++i) {
        profiles_[i].is_created = (i == 0); // Profile 1 active by default
        char def_name[16] = { 'P', 'r', 'o', 'f', 'i', 'l', ' ', static_cast<char>('1' + i), '\0' };
        std::memcpy(profiles_[i].name, def_name, 16);
        profiles_[i].total_runs = 0;
        profiles_[i].total_victories = 0;
        for (int c = 0; c < 5; ++c) {
            profiles_[i].max_ascension[c] = 0;
        }
        profiles_[i].active_run.has_active_run = false;
    }

    active_slot_ = 0;
}

void SaveManager::select_profile(uint8_t slot_index) {
    if (slot_index < PROFILE_COUNT) {
        active_slot_ = slot_index;
        profiles_[active_slot_].is_created = true;
    }
}

SaveProfile& SaveManager::get_active_profile() {
    return profiles_[active_slot_];
}

const SaveProfile& SaveManager::get_profile(uint8_t slot_index) const {
    if (slot_index >= PROFILE_COUNT) return profiles_[0];
    return profiles_[slot_index];
}

void SaveManager::create_profile(uint8_t slot_index, std::string_view name) {
    if (slot_index >= PROFILE_COUNT) return;
    profiles_[slot_index].is_created = true;
    size_t len = std::min<size_t>(name.size(), 15);
    std::memcpy(profiles_[slot_index].name, name.data(), len);
    profiles_[slot_index].name[len] = '\0';
    profiles_[slot_index].total_runs = 0;
    profiles_[slot_index].total_victories = 0;
    for (int c = 0; c < 5; ++c) {
        profiles_[slot_index].max_ascension[c] = 0;
    }
    profiles_[slot_index].active_run.has_active_run = false;
}

void SaveManager::delete_profile(uint8_t slot_index) {
    if (slot_index >= PROFILE_COUNT) return;
    profiles_[slot_index] = SaveProfile();
    char def_name[16] = { 'P', 'r', 'o', 'f', 'i', 'l', ' ', static_cast<char>('1' + slot_index), '\0' };
    std::memcpy(profiles_[slot_index].name, def_name, 16);
    profiles_[slot_index].is_created = false;
}

void SaveManager::autosave_run(const RunManager& run) {
    SaveProfile& p = get_active_profile();
    auto& r = p.active_run;

    r.has_active_run = true;
    r.character = static_cast<uint8_t>(run.character);
    r.ascension = run.ascension_level;
    r.palette = static_cast<uint8_t>(run.palette);
    r.act = run.current_act;
    r.floor = run.current_floor;
    r.hp = run.hp;
    r.max_hp = run.max_hp;
    r.gold = run.gold;
    r.current_node_index = static_cast<int8_t>(run.current_node_index);
    r.phase = static_cast<uint8_t>(run.phase);
    r.card_removals_count = run.card_removals_count;

    // Deck
    r.deck_count = static_cast<uint8_t>(run.master_deck.size());
    for (size_t i = 0; i < run.master_deck.size() && i < 50; ++i) {
        r.deck[i].card_id = static_cast<uint8_t>(run.master_deck[i].id);
        r.deck[i].upgraded = run.master_deck[i].upgraded;

        // Auto-discover in compendium
        p.discover_card(r.deck[i].card_id);
    }

    // Relics
    r.relic_count = static_cast<uint8_t>(run.relics.size());
    for (size_t i = 0; i < run.relics.size() && i < 25; ++i) {
        size_t len = std::min<size_t>(run.relics[i].size(), 23);
        std::memcpy(r.relics[i], run.relics[i].data(), len);
        r.relics[i][len] = '\0';

        // Auto-discover in compendium
        discover_relic_by_id(run.relics[i]);
    }

    // Potions
    r.potion_count = static_cast<uint8_t>(run.potions.size());
    for (size_t i = 0; i < run.potions.size() && i < 3; ++i) {
        size_t len = std::min<size_t>(run.potions[i].size(), 23);
        std::memcpy(r.potions[i], run.potions[i].data(), len);
        r.potions[i][len] = '\0';

        discover_potion_by_id(run.potions[i]);
    }

    commit_to_storage();
}

bool SaveManager::restore_run(RunManager& run) {
    const SaveProfile& p = get_active_profile();
    const auto& r = p.active_run;

    if (!r.has_active_run) return false;

    run.start_run(static_cast<CharacterId>(r.character), r.ascension, static_cast<PaletteVariant>(r.palette), 12345);
    run.current_act = r.act;
    run.current_floor = r.floor;
    run.hp = r.hp;
    run.max_hp = r.max_hp;
    run.gold = r.gold;
    run.current_node_index = r.current_node_index;
    run.phase = static_cast<RunPhase>(r.phase);
    run.card_removals_count = r.card_removals_count;

    // Restore Deck
    run.master_deck.clear();
    for (uint8_t i = 0; i < r.deck_count; ++i) {
        run.master_deck.push_back(DeckCard(static_cast<CardId>(r.deck[i].card_id), r.deck[i].upgraded));
    }

    return true;
}

void SaveManager::clear_active_run() {
    get_active_profile().active_run.has_active_run = false;
    commit_to_storage();
}

void SaveManager::record_victory(CharacterId char_id, uint8_t ascension) {
    SaveProfile& p = get_active_profile();
    p.total_runs++;
    p.total_victories++;

    uint8_t cid = static_cast<uint8_t>(char_id);
    if (cid < 5) {
        if (ascension >= p.max_ascension[cid] && p.max_ascension[cid] < 10) {
            p.max_ascension[cid] = ascension + 1;
        }
    }

    p.active_run.has_active_run = false;
    commit_to_storage();
}

void SaveManager::record_defeat() {
    SaveProfile& p = get_active_profile();
    p.total_runs++;
    p.active_run.has_active_run = false;
    commit_to_storage();
}

void SaveManager::discover_card_by_id(std::string_view card_id) {
    auto cards = get_all_card_records();
    for (size_t i = 0; i < cards.size(); ++i) {
        if (cards[i].id == card_id) {
            get_active_profile().discover_card(static_cast<uint16_t>(i));
            break;
        }
    }
}

void SaveManager::discover_relic_by_id(std::string_view relic_id) {
    auto relics = get_all_relics();
    for (size_t i = 0; i < relics.size(); ++i) {
        if (relics[i].id == relic_id) {
            get_active_profile().discover_relic(static_cast<uint16_t>(i));
            break;
        }
    }
}

void SaveManager::discover_monster_by_id(std::string_view monster_id) {
    auto monsters = get_all_monsters();
    for (size_t i = 0; i < monsters.size(); ++i) {
        if (monsters[i].id == monster_id) {
            get_active_profile().discover_monster(static_cast<uint16_t>(i));
            break;
        }
    }
}

void SaveManager::discover_potion_by_id(std::string_view potion_id) {
    auto potions = get_all_potions();
    for (size_t i = 0; i < potions.size(); ++i) {
        if (potions[i].id == potion_id) {
            get_active_profile().discover_potion(static_cast<uint16_t>(i));
            break;
        }
    }
}

void SaveManager::discover_ancient_by_id(uint8_t ancient_index) {
    if (ancient_index < 8) {
        get_active_profile().discover_ancient(ancient_index);
    }
}

void SaveManager::commit_to_storage() {
    // On GBA, can serialize into SRAM (0x0E000000)
    // On host / CLI runs, in-memory profiles are updated.
}

void SaveManager::load_from_storage() {
    // On GBA, read from SRAM if valid magic header exists
}

} // namespace engine
