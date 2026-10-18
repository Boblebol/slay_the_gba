#pragma once
#include <cstdint>
#include <string_view>
#include <span>
#include "engine/character.h"
#include "engine/run.h"

namespace engine {

constexpr size_t PROFILE_COUNT = 3;
constexpr size_t CARD_BITSET_BYTES = 72;     // 569 / 8 = 71.125 -> 72 bytes
constexpr size_t RELIC_BITSET_BYTES = 38;    // 298 / 8 = 37.25 -> 38 bytes
constexpr size_t MONSTER_BITSET_BYTES = 14;  // 112 / 8 = 14 bytes
constexpr size_t POTION_BITSET_BYTES = 8;    // 63 / 8 = 7.875 -> 8 bytes
constexpr size_t ANCIENT_BITSET_BYTES = 1;   // 8 / 8 = 1 byte

// The enchantment of a card (mechanic #107) is packed into bits the older saves leave at 0, so they still load:
// its kind above the upgraded bit, its amount above the CardId.
constexpr uint8_t SAVE_UPGRADED_BIT = 0x01;
constexpr uint8_t SAVE_ENCHANTMENT_SHIFT = 1;  // DeckCardSave::upgraded, bits 1 to 4
constexpr uint8_t SAVE_ENCHANTMENT_MASK = 0x0F;
constexpr uint8_t SAVE_CARD_ID_BITS = 10;      // DeckCardSave::card_id, bits 0 to 9
constexpr uint16_t SAVE_CARD_ID_MASK = (1u << SAVE_CARD_ID_BITS) - 1;
constexpr uint8_t SAVE_ENCHANT_AMOUNT_MAX = 0x3F; // bits 10 to 15
static_assert(static_cast<uint16_t>(CardId::COUNT) <= SAVE_CARD_ID_MASK, "CardId no longer fits its saved bits");
static_assert(static_cast<uint8_t>(Enchantment::COUNT) <= SAVE_ENCHANTMENT_MASK + 1, "Enchantment no longer fits its saved bits");

// An option of an Ancient's slot is saved on one byte with its slot (see InRunSaveData::ancient_offers).
constexpr uint8_t SAVE_ANCIENT_SLOT_STRIDE = 32;

struct DeckCardSave {
    uint16_t card_id = 0; // CardId, and the enchantment amount in the high bits
    uint8_t upgraded = 0; // upgraded bit, and the enchantment above it
    uint8_t bonus = 0; // permanent bonus of the card (DeckCard::bonus); 0 in the saves written before it existed
};

struct InRunSaveData {
    bool has_active_run = false;
    uint8_t character = 0; // CharacterId
    uint8_t ascension = 0; // 0 to 10
    uint8_t palette = 0;   // PaletteVariant
    uint8_t act = 1;
    uint8_t floor = 1;
    int16_t hp = 80;
    int16_t max_hp = 80;
    int16_t gold = 99;
    uint32_t map_seed = 0;
    uint32_t rng_state = 0;
    int8_t current_node_index = -1;
    uint8_t phase = 0;
    uint8_t card_removals_count = 0;

    uint8_t deck_count = 0;
    DeckCardSave deck[50];

    uint8_t relic_count = 0;
    char relics[25][24];

    uint8_t potion_count = 0;
    uint8_t potion_slots = BASE_POTION_SLOTS;
    char potions[MAX_POTION_SLOTS][24];

    uint8_t relic_counter_count = 0;
    uint8_t relic_counter_values[MAX_RELIC_COUNTERS] = { 0 };
    char relic_counter_ids[MAX_RELIC_COUNTERS][24];
    // The Ancient met at the start of the act, while its relic is not taken: AncientId + 1 (0 when there is none,
    // as in the saves written before it), then each relic offered as slot * SAVE_ANCIENT_SLOT_STRIDE + option.
    uint8_t ancient = 0;
    uint8_t ancient_offer_count = 0;
    uint8_t ancient_offers[MAX_ANCIENT_OFFERS] = { 0 };
    // Statistics of the end screen (#11, #12), taken from the reserved bytes (0 in the saves written before them):
    // the play time in frames (little endian), then the fights, elites and bosses won.
    uint8_t play_frames[4] = { 0 };
    uint8_t combats_won = 0;
    uint8_t elites_won = 0;
    uint8_t bosses_won = 0;
    // Experience of the profile (#11), little endian. It belongs to the profile, not to the run: it sits here
    // because these are the only spare bytes of a profile. The autosave and the end of a run never clear it.
    uint8_t profile_xp[4] = { 0 };
    // Room for what the run will have to remember next, so a new field does not move the ones after it.
    uint8_t reserved[32 - 2 - MAX_ANCIENT_OFFERS - 11] = { 0 };
};

struct SaveProfile {
    char name[16] = { 'P', 'r', 'o', 'f', 'i', 'l', ' ', '1', '\0' };
    bool is_created = false;
    uint32_t playtime_seconds = 0;
    uint16_t total_runs = 0;
    uint16_t total_victories = 0;
    uint8_t max_ascension[5] = { 0, 0, 0, 0, 0 }; // Per character (0..10)

    // Compendium bitsets
    uint8_t discovered_cards[CARD_BITSET_BYTES] = { 0 };
    uint8_t discovered_relics[RELIC_BITSET_BYTES] = { 0 };
    uint8_t discovered_monsters[MONSTER_BITSET_BYTES] = { 0 };
    uint8_t discovered_potions[POTION_BITSET_BYTES] = { 0 };
    uint8_t discovered_ancients[ANCIENT_BITSET_BYTES] = { 0 };

    InRunSaveData active_run;

    // Helper discovery methods
    void discover_card(uint16_t card_index);
    bool is_card_discovered(uint16_t card_index) const;
    uint16_t count_discovered_cards() const;

    void discover_relic(uint16_t relic_index);
    bool is_relic_discovered(uint16_t relic_index) const;
    uint16_t count_discovered_relics() const;

    void discover_monster(uint16_t monster_index);
    bool is_monster_discovered(uint16_t monster_index) const;
    uint16_t count_discovered_monsters() const;

    void discover_potion(uint16_t potion_index);
    bool is_potion_discovered(uint16_t potion_index) const;
    uint16_t count_discovered_potions() const;

    void discover_ancient(uint8_t ancient_index);
    bool is_ancient_discovered(uint8_t ancient_index) const;
    uint8_t count_discovered_ancients() const;
};

// Player preferences, stored next to the profiles.
struct GameSettings {
    uint8_t music_volume = 8;   // 0 (off) to 10
    uint8_t sfx_volume = 8;     // 0 (off) to 10
    uint8_t control_preset = 0; // 0: classic, 1: quick
    uint8_t reserved = 0;
};

// Everything written to the cartridge.
struct SaveData {
    SaveProfile profiles[PROFILE_COUNT];
    GameSettings settings;
    uint8_t active_slot = 0;
};

// Raw byte storage backing the save: battery SRAM on GBA, a memory buffer or a file on host.
class SaveStorage {
public:
    virtual ~SaveStorage() = default;
    virtual void read(void* destination, size_t size, size_t offset) = 0;
    virtual void write(const void* source, size_t size, size_t offset) = 0;
};

// The save is written alternately into two banks so a power loss during a write never destroys the
// previous good copy. Each bank starts with a header holding a checksum of its payload.
constexpr size_t SAVE_BANK_SIZE = 16 * 1024;
// Version 4 stores card ids on 16 bits, which moves every field after the deck. Version 5 stores the size of
// the potion belt and up to MAX_POTION_SLOTS potions, which makes every profile longer. Version 6 adds the
// counters of the relics and reserved bytes.
constexpr uint16_t SAVE_FORMAT_VERSION = 6;
// Oldest layout still readable. A cartridge saved with an older one starts again from the defaults.
constexpr uint16_t SAVE_MIN_FORMAT_VERSION = 6;

class SaveManager {
public:
    static SaveManager& instance();

    void init();
    // Resets every profile and setting to its default value (in memory only).
    void reset_to_defaults();
    void select_profile(uint8_t slot_index); // 0, 1, 2
    uint8_t get_active_profile_index() const { return data_.active_slot; }
    SaveProfile& get_active_profile();
    GameSettings& settings() { return data_.settings; }
    [[nodiscard]] bool has_active_run() const;
    const SaveProfile& get_profile(uint8_t slot_index) const;

    void create_profile(uint8_t slot_index, std::string_view name);
    void delete_profile(uint8_t slot_index);

    // Autosave in-progress run
    void autosave_run(const RunManager& run);
    bool restore_run(RunManager& run);
    void clear_active_run();

    // End run bookkeeping
    void record_victory(CharacterId char_id, uint8_t ascension);
    // Highest Ascension level the active profile may start with this hero (0..10).
    uint8_t unlocked_ascension(CharacterId char_id);
    void record_defeat();
    // Experience the active profile has gathered over its runs (#11).
    [[nodiscard]] uint32_t profile_xp() const;
    // Adds to the experience of the active profile (stops at the largest count). Written with the next commit.
    void add_profile_xp(uint32_t amount);

    // Compendium discovery helpers
    void discover_card_by_id(std::string_view card_id);
    void discover_relic_by_id(std::string_view relic_id);
    void discover_monster_by_id(std::string_view monster_id);
    void discover_potion_by_id(std::string_view potion_id);
    void discover_ancient_by_id(uint8_t ancient_index);

    // Persistence (SRAM / Storage). Without a storage backend both calls only touch memory.
    void set_storage(SaveStorage* storage);
    void commit_to_storage();
    // Loads the newest valid bank. Returns false (and keeps defaults) when no valid save exists.
    bool load_from_storage();

private:
    SaveManager() = default;
    SaveData data_;
    SaveStorage* storage_ = nullptr;
    uint32_t sequence_ = 0; // incremented on every commit, picks the newest bank on load
    bool initialized_ = false;
};

} // namespace engine
