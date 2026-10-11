#include "engine/encounter.h"

namespace engine {

namespace {

using namespace std::string_view_literals;

// Static constexpr data only: the tables live in the cartridge's ROM, never in the GBA's small RAM.
// Source: bible/data/monster_v0.111.0.json ("encounters" of each monster) and bible/16_bestiaire_monstres.md.
// Only the encounters whose every monster exists in EnemyType are listed; the reference gives no weights,
// so every encounter of a list is as likely as the others.

// RubyRaidersNormal is three raiders drawn among the five Ruby Raiders; TRACKER_RUBY_RAIDER is not in the
// engine yet.
constexpr EnemyType RUBY_RAIDERS[] = {
    EnemyType::CROSSBOW_RUBY_RAIDER,
    EnemyType::BRUTE_RUBY_RAIDER,
    EnemyType::ASSASSIN_RUBY_RAIDER,
    EnemyType::AXE_RUBY_RAIDER,
};

// Act 1 (A): the Overgrowth.
constexpr Encounter ACT1A_WEAK[] = {
    { "FuzzyWurmCrawlerWeak"sv, 1, { EnemyType::FUZZY_WURM_CRAWLER } },
    { "NibbitsWeak"sv, 1, { EnemyType::NIBBIT } },
};
constexpr Encounter ACT1A_NORMAL[] = {
    { "RubyRaidersNormal"sv, 3, {}, RUBY_RAIDERS },
    { "VineShamblerNormal"sv, 1, { EnemyType::VINE_SHAMBLER } },
    { "CubexConstructNormal"sv, 1, { EnemyType::CUBEX_CONSTRUCT } },
    { "InkletsNormal"sv, 3, { EnemyType::INKLET, EnemyType::INKLET, EnemyType::INKLET } },
    { "NibbitsNormal"sv, 2, { EnemyType::NIBBIT, EnemyType::NIBBIT } },
    { "FlyconidNormal"sv, 1, { EnemyType::FLYCONID } },
};

// Act 1 (B): the Underdocks.
constexpr Encounter ACT1B_WEAK[] = {
    { "CorpseSlugsWeak"sv, 2, { EnemyType::CORPSE_SLUG, EnemyType::CORPSE_SLUG } },
    { "SludgeSpinnerWeak"sv, 1, { EnemyType::SLUDGE_SPINNER } },
    { "ToadpolesWeak"sv, 2, { EnemyType::TOADPOLE, EnemyType::TOADPOLE } },
};
constexpr Encounter ACT1B_NORMAL[] = {
    { "CultistsNormal"sv, 2, { EnemyType::CALCIFIED_CULTIST, EnemyType::DAMP_CULTIST } },
    { "SewerClamNormal"sv, 1, { EnemyType::SEWER_CLAM } },
    { "CorpseSlugsNormal"sv, 3, { EnemyType::CORPSE_SLUG, EnemyType::CORPSE_SLUG, EnemyType::CORPSE_SLUG } },
    { "PunchConstructNormal"sv, 1, { EnemyType::PUNCH_CONSTRUCT } },
    { "GremlinMercNormal"sv, 1, { EnemyType::GREMLIN_MERC } },
    { "HauntedShipNormal"sv, 1, { EnemyType::HAUNTED_SHIP } },
};

// Act 2: the legacy groups, and the reference encounters the engine can play.
constexpr Encounter ACT2_NORMAL[] = {
    { "LegacyCultistAndLouse"sv, 2, { EnemyType::CULTIST, EnemyType::LOUSE } },
    { "LegacyJawWorm"sv, 1, { EnemyType::JAW_WORM } },
    { "LegacyLice"sv, 2, { EnemyType::LOUSE, EnemyType::LOUSE } },
    { "LouseProgenitorNormal"sv, 1, { EnemyType::LOUSE_PROGENITOR } },
};

constexpr Encounter CEREMONIAL_BEAST_BOSS[] = {
    { "CeremonialBeastBoss"sv, 1, { EnemyType::CEREMONIAL_BEAST } },
};

// The groups the game had before the tables, for the lists with no reference encounter in the engine yet.
constexpr Encounter LEGACY_WEAK[] = {
    { "LegacyCultist"sv, 1, { EnemyType::CULTIST } },
};
constexpr Encounter LEGACY_NORMAL[] = {
    { "LegacyCultistAndLouse"sv, 2, { EnemyType::CULTIST, EnemyType::LOUSE } },
    { "LegacyJawWorm"sv, 1, { EnemyType::JAW_WORM } },
    { "LegacyLice"sv, 2, { EnemyType::LOUSE, EnemyType::LOUSE } },
};
constexpr Encounter LEGACY_ELITE[] = {
    { "LegacyGremlinNob"sv, 1, { EnemyType::GREMLIN_NOB } },
};

// [zone][tier], in the order of ActZone and EncounterTier.
constexpr std::span<const Encounter> TABLES[4][4] = {
    { ACT1A_WEAK, ACT1A_NORMAL, LEGACY_ELITE, CEREMONIAL_BEAST_BOSS },
    { ACT1B_WEAK, ACT1B_NORMAL, LEGACY_ELITE, CEREMONIAL_BEAST_BOSS },
    { LEGACY_WEAK, ACT2_NORMAL, LEGACY_ELITE, CEREMONIAL_BEAST_BOSS },
    { LEGACY_WEAK, LEGACY_NORMAL, LEGACY_ELITE, CEREMONIAL_BEAST_BOSS },
};

} // namespace

ActZone act_zone(uint8_t act, uint32_t map_seed) {
    if (act <= 1) {
        return ((map_seed >> 16) & 1) ? ActZone::ACT1B : ActZone::ACT1A;
    }
    return act == 2 ? ActZone::ACT2 : ActZone::ACT3;
}

EncounterTier encounter_tier(RoomType room, uint8_t floor) {
    if (room == RoomType::BOSS) return EncounterTier::BOSS;
    if (room == RoomType::ELITE) return EncounterTier::ELITE;
    return floor <= WEAK_ENCOUNTER_FLOORS ? EncounterTier::WEAK : EncounterTier::NORMAL;
}

std::span<const Encounter> encounter_table(ActZone zone, EncounterTier tier) {
    return TABLES[static_cast<uint8_t>(zone)][static_cast<uint8_t>(tier)];
}

} // namespace engine
