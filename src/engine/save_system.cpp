#include "engine/save_system.h"
#include "engine/card_db.h"
#include "engine/options.h"
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

// Header written at the start of each save bank.
struct SaveHeader {
    char magic[8];
    uint16_t version;
    uint16_t reserved;
    uint32_t payload_size;
    uint32_t sequence;
    uint32_t checksum;
};

constexpr char SAVE_MAGIC[8] = { 'S', 'L', 'A', 'Y', 'G', 'B', 'A', '\0' };

static_assert(sizeof(SaveHeader) + sizeof(SaveData) <= SAVE_BANK_SIZE, "Save data does not fit in a bank");

// FNV-1a: cheap and good enough to detect a torn write or an uninitialised cartridge.
uint32_t checksum(const void* data, size_t size) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

bool is_valid_header(const SaveHeader& header) {
    return std::memcmp(header.magic, SAVE_MAGIC, sizeof(SAVE_MAGIC)) == 0 &&
           header.version >= SAVE_MIN_FORMAT_VERSION && header.version <= SAVE_FORMAT_VERSION &&
           header.payload_size == sizeof(SaveData);
}

// Id stored in a fixed char buffer. Bounded by hand: the GBA build links without libc's strlen.
template<size_t Size>
std::string_view stored_id(const char (&buffer)[Size]) {
    size_t length = 0;
    while (length < Size && buffer[length] != '\0') ++length;
    return std::string_view(buffer, length);
}

// Restored relic and potion ids must point to static strings: look them up in the databases.
std::string_view find_static_relic_id(std::string_view id, CharacterId character) {
    if (const RelicData* relic = find_relic(id)) return relic->id;
    std::string_view starter = get_character(character).starting_relic_id;
    return starter == id ? starter : std::string_view();
}

std::string_view find_static_potion_id(std::string_view id) {
    const PotionData* potion = find_potion(id);
    return potion ? potion->id : std::string_view();
}

// Little-endian 32-bit number kept in bytes, so it never changes the layout (or the padding) of a saved struct.
uint32_t read_u32(const uint8_t (&bytes)[4]) {
    return static_cast<uint32_t>(bytes[0]) | static_cast<uint32_t>(bytes[1]) << 8 |
           static_cast<uint32_t>(bytes[2]) << 16 | static_cast<uint32_t>(bytes[3]) << 24;
}

void write_u32(uint8_t (&bytes)[4], uint32_t value) {
    for (int i = 0; i < 4; ++i) bytes[i] = static_cast<uint8_t>(value >> (8 * i));
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
    // Heap allocated: the save data is a few KB, too big for the GBA's fast IWRAM .bss.
    static SaveManager* mgr = new SaveManager();
    if (!mgr->initialized_) {
        mgr->init();
    }
    return *mgr;
}

void SaveManager::init() {
    if (initialized_) return;
    initialized_ = true;
    reset_to_defaults();
}

void SaveManager::reset_to_defaults() {
    data_ = SaveData();

    // Initialize 3 default profiles
    for (size_t i = 0; i < PROFILE_COUNT; ++i) {
        data_.profiles[i].is_created = (i == 0); // Profile 1 active by default
        char def_name[16] = { 'P', 'r', 'o', 'f', 'i', 'l', ' ', static_cast<char>('1' + i), '\0' };
        std::memcpy(data_.profiles[i].name, def_name, 16);
    }

    data_.active_slot = 0;
}

void SaveManager::select_profile(uint8_t slot_index) {
    if (slot_index < PROFILE_COUNT) {
        data_.active_slot = slot_index;
        data_.profiles[data_.active_slot].is_created = true;
    }
}

SaveProfile& SaveManager::get_active_profile() {
    return data_.profiles[data_.active_slot];
}

bool SaveManager::has_active_run() const {
    return data_.profiles[data_.active_slot].active_run.has_active_run;
}

const SaveProfile& SaveManager::get_profile(uint8_t slot_index) const {
    if (slot_index >= PROFILE_COUNT) return data_.profiles[0];
    return data_.profiles[slot_index];
}

void SaveManager::create_profile(uint8_t slot_index, std::string_view name) {
    if (slot_index >= PROFILE_COUNT) return;
    data_.profiles[slot_index].is_created = true;
    size_t len = std::min<size_t>(name.size(), 15);
    std::memcpy(data_.profiles[slot_index].name, name.data(), len);
    data_.profiles[slot_index].name[len] = '\0';
    data_.profiles[slot_index].total_runs = 0;
    data_.profiles[slot_index].total_victories = 0;
    for (int c = 0; c < 5; ++c) {
        data_.profiles[slot_index].max_ascension[c] = 0;
    }
    data_.profiles[slot_index].active_run.has_active_run = false;
    write_u32(data_.profiles[slot_index].active_run.profile_xp, 0);
}

void SaveManager::delete_profile(uint8_t slot_index) {
    if (slot_index >= PROFILE_COUNT) return;
    data_.profiles[slot_index] = SaveProfile();
    char def_name[16] = { 'P', 'r', 'o', 'f', 'i', 'l', ' ', static_cast<char>('1' + slot_index), '\0' };
    std::memcpy(data_.profiles[slot_index].name, def_name, 16);
    data_.profiles[slot_index].is_created = false;
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
    r.map_seed = run.map_seed;
    r.rng_state = run.rng_state();
    r.current_node_index = static_cast<int8_t>(run.current_node_index);
    r.phase = static_cast<uint8_t>(run.phase);
    r.card_removals_count = run.card_removals_count;
    write_u32(r.play_frames, run.play_frames);
    r.combats_won = run.combats_won;
    r.elites_won = run.elites_won;
    r.bosses_won = run.bosses_won;
    for (size_t i = 0; i < sizeof(r.events_seen); ++i) r.events_seen[i] = static_cast<uint8_t>(run.events_seen >> (8 * i));

    // Ancient met at the start of the act, with its offer
    const bool ancient = run.phase == RunPhase::ANCIENT_BLESSING && run.current_ancient < AncientId::COUNT;
    r.ancient = ancient ? static_cast<uint8_t>(static_cast<uint8_t>(run.current_ancient) + 1) : 0;
    r.ancient_offer_count = ancient ? static_cast<uint8_t>(run.ancient_offers.size()) : 0;
    for (size_t i = 0; i < MAX_ANCIENT_OFFERS; ++i) {
        const bool used = i < r.ancient_offer_count;
        r.ancient_offers[i] = used ? static_cast<uint8_t>(run.ancient_offers[i].slot * SAVE_ANCIENT_SLOT_STRIDE +
                                                          run.ancient_offers[i].option)
                                   : 0;
    }

    // Deck
    r.deck_count = static_cast<uint8_t>(run.master_deck.size());
    for (size_t i = 0; i < run.master_deck.size() && i < 50; ++i) {
        const DeckCard& card = run.master_deck[i];
        const uint16_t amount = std::min<uint8_t>(card.enchant_amount, SAVE_ENCHANT_AMOUNT_MAX);
        r.deck[i].card_id = static_cast<uint16_t>(static_cast<uint16_t>(card.id) | (amount << SAVE_CARD_ID_BITS));
        r.deck[i].upgraded = static_cast<uint8_t>((card.upgraded ? SAVE_UPGRADED_BIT : 0) |
                                                  (static_cast<uint8_t>(card.enchantment) << SAVE_ENCHANTMENT_SHIFT));
        r.deck[i].bonus = card.bonus;

        // Auto-discover in compendium
        p.discover_card(static_cast<uint16_t>(card.id));
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
    r.potion_slots = run.potion_slots;
    for (size_t i = 0; i < run.potions.size() && i < MAX_POTION_SLOTS; ++i) {
        size_t len = std::min<size_t>(run.potions[i].size(), 23);
        std::memcpy(r.potions[i], run.potions[i].data(), len);
        r.potions[i][len] = '\0';

        discover_potion_by_id(run.potions[i]);
    }

    // Relic counters
    r.relic_counter_count = static_cast<uint8_t>(run.relic_counters.size());
    for (size_t i = 0; i < run.relic_counters.size() && i < MAX_RELIC_COUNTERS; ++i) {
        const RelicCounter& counter = run.relic_counters[i];
        size_t len = std::min<size_t>(counter.relic_id.size(), 23);
        std::memcpy(r.relic_counter_ids[i], counter.relic_id.data(), len);
        r.relic_counter_ids[i][len] = '\0';
        r.relic_counter_values[i] = counter.value;
    }

    commit_to_storage();
}

bool SaveManager::restore_run(RunManager& run) {
    const SaveProfile& p = get_active_profile();
    const auto& r = p.active_run;

    if (!r.has_active_run) return false;

    CharacterId character = static_cast<CharacterId>(r.character);
    run.start_run(character, r.ascension, static_cast<PaletteVariant>(r.palette), 12345);
    run.current_act = r.act;
    run.current_floor = r.floor;
    run.hp = r.hp;
    run.max_hp = r.max_hp;
    run.gold = r.gold;
    run.map_seed = r.map_seed;
    run.map.generate(r.act, r.map_seed, run.ascension_level);
    run.set_rng_state(r.rng_state);
    run.current_node_index = r.current_node_index;
    // Runs are saved between rooms, or before the relic of an Ancient is taken (restored below): the state of an
    // interrupted room is not stored.
    run.phase = RunPhase::MAP_NAVIGATION;
    run.card_removals_count = r.card_removals_count;
    run.play_frames = read_u32(r.play_frames);
    run.combats_won = r.combats_won;
    run.elites_won = r.elites_won;
    run.bosses_won = r.bosses_won;
    run.events_seen = 0;
    for (size_t i = 0; i < sizeof(r.events_seen); ++i) run.events_seen |= uint64_t{ r.events_seen[i] } << (8 * i);

    // Restore Deck
    run.master_deck.clear();
    for (uint8_t i = 0; i < r.deck_count && i < 50; ++i) {
        DeckCard card(static_cast<CardId>(r.deck[i].card_id & SAVE_CARD_ID_MASK), (r.deck[i].upgraded & SAVE_UPGRADED_BIT) != 0);
        card.bonus = r.deck[i].bonus;
        const uint8_t enchantment = (r.deck[i].upgraded >> SAVE_ENCHANTMENT_SHIFT) & SAVE_ENCHANTMENT_MASK;
        if (enchantment < static_cast<uint8_t>(Enchantment::COUNT)) {
            card.enchantment = static_cast<Enchantment>(enchantment);
            card.enchant_amount = static_cast<uint8_t>(r.deck[i].card_id >> SAVE_CARD_ID_BITS);
        }
        run.master_deck.push_back(card);
    }

    // Restore Relics
    run.relics.clear();
    for (uint8_t i = 0; i < r.relic_count && i < 25; ++i) {
        std::string_view id = find_static_relic_id(stored_id(r.relics[i]), character);
        if (!id.empty()) run.relics.push_back(id);
    }

    // Restore Potions
    run.potions.clear();
    run.potion_slots = std::clamp<uint8_t>(r.potion_slots, BASE_POTION_SLOTS, MAX_POTION_SLOTS);
    for (uint8_t i = 0; i < r.potion_count && i < run.potion_slots; ++i) {
        std::string_view id = find_static_potion_id(stored_id(r.potions[i]));
        if (!id.empty()) run.potions.push_back(id);
    }

    // Restore relic counters
    run.relic_counters.clear();
    for (uint8_t i = 0; i < r.relic_counter_count && i < MAX_RELIC_COUNTERS; ++i) {
        std::string_view id = find_static_relic_id(stored_id(r.relic_counter_ids[i]), character);
        if (!id.empty()) run.relic_counters.push_back(RelicCounter{ id, r.relic_counter_values[i] });
    }

    run.regenerate_map(); // relics are back: the Golden Compass changes the Act 2 map

    // Saved before the relic of the Ancient was taken: the same Ancient offers the same relics again.
    if (r.phase == static_cast<uint8_t>(RunPhase::ANCIENT_BLESSING) && r.ancient > 0 &&
        r.ancient <= static_cast<uint8_t>(AncientId::COUNT)) {
        const AncientId id = static_cast<AncientId>(r.ancient - 1);
        const Ancient& ancient = get_ancient(id);
        run.current_ancient = id;
        run.ancient_offers.clear();
        for (uint8_t i = 0; i < r.ancient_offer_count && i < MAX_ANCIENT_OFFERS; ++i) {
            const uint8_t slot = r.ancient_offers[i] / SAVE_ANCIENT_SLOT_STRIDE;
            const uint8_t option = r.ancient_offers[i] % SAVE_ANCIENT_SLOT_STRIDE;
            if (slot < ancient.slots.size() && option < ancient.slots[slot].options.size()) {
                run.ancient_offers.push_back(AncientOffer{ slot, option });
            }
        }
        run.phase = RunPhase::ANCIENT_BLESSING;
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

uint8_t SaveManager::unlocked_ascension(CharacterId char_id) {
    uint8_t cid = static_cast<uint8_t>(char_id);
    if (cid >= 5) return 0;
    uint8_t level = get_active_profile().max_ascension[cid];
    return level > 10 ? 10 : level;
}

void SaveManager::record_defeat() {
    SaveProfile& p = get_active_profile();
    p.total_runs++;
    p.active_run.has_active_run = false;
    commit_to_storage();
}

uint32_t SaveManager::profile_xp() const {
    return read_u32(data_.profiles[data_.active_slot].active_run.profile_xp);
}

void SaveManager::add_profile_xp(uint32_t amount) {
    const uint32_t xp = profile_xp();
    write_u32(get_active_profile().active_run.profile_xp, amount > UINT32_MAX - xp ? UINT32_MAX : xp + amount);
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

void SaveManager::set_storage(SaveStorage* storage) {
    storage_ = storage;
}

void SaveManager::commit_to_storage() {
    if (!storage_) return;

    ++sequence_;
    SaveHeader header = {};
    std::memcpy(header.magic, SAVE_MAGIC, sizeof(SAVE_MAGIC));
    header.version = SAVE_FORMAT_VERSION;
    header.payload_size = sizeof(SaveData);
    header.sequence = sequence_;
    header.checksum = checksum(&data_, sizeof(SaveData));

    // Odd sequences go to bank 0, even ones to bank 1: the previous save always survives.
    size_t offset = (sequence_ % 2) ? 0 : SAVE_BANK_SIZE;
    // Payload first, header last: a torn write leaves an invalid header, never a valid one on bad data.
    SaveHeader invalid = {};
    storage_->write(&invalid, sizeof(SaveHeader), offset);
    storage_->write(&data_, sizeof(SaveData), offset + sizeof(SaveHeader));
    storage_->write(&header, sizeof(SaveHeader), offset);
}

bool SaveManager::load_from_storage() {
    if (!storage_) return false;

    // Pick the newest bank whose header and checksum are valid.
    SaveHeader best = {};
    bool found = false;
    size_t best_offset = 0;

    for (size_t bank = 0; bank < 2; ++bank) {
        size_t offset = bank * SAVE_BANK_SIZE;
        SaveHeader header;
        storage_->read(&header, sizeof(SaveHeader), offset);
        if (!is_valid_header(header)) continue;
        if (found && header.sequence <= best.sequence) continue;

        storage_->read(&data_, sizeof(SaveData), offset + sizeof(SaveHeader));
        if (checksum(&data_, sizeof(SaveData)) != header.checksum) continue;

        best = header;
        best_offset = offset;
        found = true;
    }

    if (!found) {
        reset_to_defaults();
        sequence_ = 0;
        return false;
    }

    storage_->read(&data_, sizeof(SaveData), best_offset + sizeof(SaveHeader));
    sequence_ = best.sequence;
    if (data_.active_slot >= PROFILE_COUNT) data_.active_slot = 0;
    sanitize_settings(data_.settings);
    return true;
}

} // namespace engine
