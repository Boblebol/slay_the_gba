#pragma once
#include <cstdint>
#include <span>
#include <string_view>
#include "engine/entity.h"
#include "engine/map.h"

namespace engine {

// Where the fights of an act come from. Act 1 has two variants in the reference (the Overgrowth, A, and the
// Underdocks, B): a run plays one of them.
enum class ActZone : uint8_t {
    ACT1A,
    ACT1B,
    ACT2,
    ACT3
};

// Weak encounters are the first fights of an act, normal ones the monster rooms after them.
enum class EncounterTier : uint8_t {
    WEAK,
    NORMAL,
    ELITE,
    BOSS
};

// One group of monsters met together, as listed in bible/data/monster_v0.111.0.json ("encounters").
struct Encounter {
    std::string_view id; // reference id ("SewerClamNormal"), or "Legacy..." for the groups kept from before
    uint8_t count;       // monsters in the fight
    EnemyType monsters[MAX_ENEMIES];
    // When not empty, the `count` monsters are drawn from this pool, all different, instead of `monsters`.
    std::span<const EnemyType> random_pool = {};
};

// The fights of the first `WEAK_ENCOUNTER_FLOORS` floors of an act are weak encounters.
constexpr uint8_t WEAK_ENCOUNTER_FLOORS = 3;

// The zone of `act`: for Act 1, the variant follows the act's map seed, so a seed always plays the same one
// and a resumed run finds it again without saving it.
[[nodiscard]] ActZone act_zone(uint8_t act, uint32_t map_seed);

// Tier of a fighting room on floor `floor` (1 to 16).
[[nodiscard]] EncounterTier encounter_tier(RoomType room, uint8_t floor);

// Encounters a room of `tier` draws from in `zone`. Never empty: when the reference has no encounter whose
// every monster exists in EnemyType, the groups the game had before the tables are used.
[[nodiscard]] std::span<const Encounter> encounter_table(ActZone zone, EncounterTier tier);

} // namespace engine
