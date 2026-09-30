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

struct DeckCardSave {
    uint8_t card_id = 0;
    bool upgraded = false;
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
    char potions[3][24];
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
constexpr uint16_t SAVE_FORMAT_VERSION = 3;
// Oldest layout still readable. Version 2 generated a different map from the same seed: its profiles and
// settings are kept but its in-progress runs are dropped.
constexpr uint16_t SAVE_MIN_FORMAT_VERSION = 2;

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
    void record_defeat();

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
