// Encounter tables per act (#225): what the tables hold, and the fights a run draws from them.
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include "engine/encounter.h"
#include "engine/run.h"

using namespace engine;

namespace {

constexpr ActZone ZONES[] = { ActZone::ACT1A, ActZone::ACT1B, ActZone::ACT2, ActZone::ACT3 };
constexpr EncounterTier TIERS[] = { EncounterTier::WEAK, EncounterTier::NORMAL, EncounterTier::ELITE,
                                    EncounterTier::BOSS };
constexpr uint32_t SEEDS = 400;

// Identifier of each EnemyType, as written in the code; empty for a value outside the enum.
std::string_view enemy_identifier(EnemyType type) {
    switch (type) {
        case EnemyType::CULTIST: return "CULTIST";
        case EnemyType::JAW_WORM: return "JAW_WORM";
        case EnemyType::LOUSE: return "LOUSE";
        case EnemyType::GREMLIN_NOB: return "GREMLIN_NOB";
        case EnemyType::CEREMONIAL_BEAST: return "CEREMONIAL_BEAST";
        case EnemyType::FUZZY_WURM_CRAWLER: return "FUZZY_WURM_CRAWLER";
        case EnemyType::CROSSBOW_RUBY_RAIDER: return "CROSSBOW_RUBY_RAIDER";
        case EnemyType::BRUTE_RUBY_RAIDER: return "BRUTE_RUBY_RAIDER";
        case EnemyType::ASSASSIN_RUBY_RAIDER: return "ASSASSIN_RUBY_RAIDER";
        case EnemyType::AXE_RUBY_RAIDER: return "AXE_RUBY_RAIDER";
        case EnemyType::TRACKER_RUBY_RAIDER: return "TRACKER_RUBY_RAIDER";
        case EnemyType::CALCIFIED_CULTIST: return "CALCIFIED_CULTIST";
        case EnemyType::DAMP_CULTIST: return "DAMP_CULTIST";
        case EnemyType::SEWER_CLAM: return "SEWER_CLAM";
        case EnemyType::THE_FORGOTTEN: return "THE_FORGOTTEN";
        case EnemyType::LIVING_SHIELD: return "LIVING_SHIELD";
        case EnemyType::FAT_GREMLIN: return "FAT_GREMLIN";
        case EnemyType::VINE_SHAMBLER: return "VINE_SHAMBLER";
        case EnemyType::EYE_WITH_TEETH: return "EYE_WITH_TEETH";
        case EnemyType::STABBOT: return "STABBOT";
        case EnemyType::SPINY_TOAD: return "SPINY_TOAD";
        case EnemyType::NOISEBOT: return "NOISEBOT";
        case EnemyType::CUBEX_CONSTRUCT: return "CUBEX_CONSTRUCT";
        case EnemyType::PUNCH_CONSTRUCT: return "PUNCH_CONSTRUCT";
        case EnemyType::INKLET: return "INKLET";
        case EnemyType::NIBBIT: return "NIBBIT";
        case EnemyType::FLYCONID: return "FLYCONID";
        case EnemyType::CORPSE_SLUG: return "CORPSE_SLUG";
        case EnemyType::GAS_BOMB: return "GAS_BOMB";
        case EnemyType::GREMLIN_MERC: return "GREMLIN_MERC";
        case EnemyType::SNEAKY_GREMLIN: return "SNEAKY_GREMLIN";
        case EnemyType::HAUNTED_SHIP: return "HAUNTED_SHIP";
        case EnemyType::SLUDGE_SPINNER: return "SLUDGE_SPINNER";
        case EnemyType::TOADPOLE: return "TOADPOLE";
        case EnemyType::HUNTER_KILLER: return "HUNTER_KILLER";
        case EnemyType::MAWLER: return "MAWLER";
        case EnemyType::FOSSIL_STALKER: return "FOSSIL_STALKER";
        case EnemyType::SEAPUNK: return "SEAPUNK";
        case EnemyType::ENTOMANCER: return "ENTOMANCER";
    }
    return {};
}

std::string read_file(const char* path) {
    std::ifstream file(path);
    assert(file && "run the tests from the repository root");
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

// The GBA picks a sprite in enemy_art (src/gba/assets.cpp): a case of its own, or the Cultist's by default.
bool has_sprite(const std::string& assets, EnemyType type) {
    std::string_view name = enemy_identifier(type);
    if (name.empty()) return false;
    if (assets.find("case engine::EnemyType::" + std::string(name) + ":") != std::string::npos) return true;
    return type == EnemyType::CULTIST && assets.find("return { &bn::sprite_items::mon_cultist") != std::string::npos;
}

// Every monster an encounter can bring.
std::vector<EnemyType> possible_monsters(const Encounter& encounter) {
    if (!encounter.random_pool.empty()) {
        return { encounter.random_pool.begin(), encounter.random_pool.end() };
    }
    return { encounter.monsters, encounter.monsters + encounter.count };
}

bool contains(std::span<const Encounter> table, std::string_view id) {
    for (const Encounter& encounter : table) {
        if (encounter.id == id) return true;
    }
    return false;
}

const Encounter& find_encounter(std::span<const Encounter> table, std::string_view id) {
    for (const Encounter& encounter : table) {
        if (encounter.id == id) return encounter;
    }
    assert(false && "encounter not in its table");
    return table[0];
}

std::set<std::string_view> ids_of(std::span<const Encounter> table) {
    std::set<std::string_view> ids;
    for (const Encounter& encounter : table) ids.insert(encounter.id);
    return ids;
}

// The fight in progress holds the monsters of `encounter`, in its order, or different ones from its pool.
void check_fight_matches(const RunManager& run, const Encounter& encounter) {
    assert(run.phase == RunPhase::IN_COMBAT);
    assert(run.combat.enemies.size() == encounter.count);
    std::set<EnemyType> seen;
    for (size_t i = 0; i < run.combat.enemies.size(); ++i) {
        EnemyType type = run.combat.enemies[i].type;
        if (encounter.random_pool.empty()) {
            assert(type == encounter.monsters[i]);
        } else {
            std::vector<EnemyType> pool = possible_monsters(encounter);
            assert(std::find(pool.begin(), pool.end(), type) != pool.end());
            assert(seen.insert(type).second); // all different
        }
    }
}

// A run of `seed` in act `act`, before its first room.
RunManager make_run(uint32_t seed, uint8_t act) {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
    while (run.current_act < act) run.advance_act();
    return run;
}

// First node of the map that is a fighting room of `tier`, or -1.
int find_room(const RunManager& run, EncounterTier tier) {
    for (size_t i = 0; i < run.map.nodes.size(); ++i) {
        const MapNode& node = run.map.nodes[i];
        bool fights = node.type == RoomType::MONSTER || node.type == RoomType::ELITE || node.type == RoomType::BOSS;
        if (fights && encounter_tier(node.type, static_cast<uint8_t>(node.floor + 1)) == tier) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void test_tables_use_existing_monsters_with_a_sprite() {
    const std::string assets = read_file("src/gba/assets.cpp");
    const std::string reference = read_file("bible/data/monster_v0.111.0.json");
    for (ActZone zone : ZONES) {
        for (EncounterTier tier : TIERS) {
            std::span<const Encounter> table = encounter_table(zone, tier);
            assert(!table.empty());
            assert(ids_of(table).size() == table.size()); // no encounter listed twice
            for (const Encounter& encounter : table) {
                // The GBA shows up to three enemies (bn::vector<EnemyView, 3> in scene_combat.cpp).
                assert(encounter.count >= 1 && encounter.count <= 3);
                assert(encounter.random_pool.empty() || encounter.random_pool.size() >= encounter.count);
                for (EnemyType type : possible_monsters(encounter)) {
                    assert(!enemy_identifier(type).empty());
                    assert(has_sprite(assets, type));
                }
                // Encounters that are not legacy ones are named like in the reference.
                if (!encounter.id.starts_with("Legacy")) {
                    std::string key = "\"id\": \"" + std::string(encounter.id) + "\"";
                    assert(reference.find(key) != std::string::npos);
                }
            }
        }
    }
    std::cout << "[PASS] Encounter tables: existing monsters, 1 to 3 per fight, a sprite each" << std::endl;
}

void test_reference_encounters() {
    assert(ids_of(encounter_table(ActZone::ACT1A, EncounterTier::WEAK)) ==
           (std::set<std::string_view>{ "FuzzyWurmCrawlerWeak", "NibbitsWeak" }));
    assert(ids_of(encounter_table(ActZone::ACT1A, EncounterTier::NORMAL)) ==
           (std::set<std::string_view>{ "RubyRaidersNormal", "VineShamblerNormal", "CubexConstructNormal",
                                        "InkletsNormal", "NibbitsNormal", "FlyconidNormal", "MawlerNormal" }));
    assert(ids_of(encounter_table(ActZone::ACT1B, EncounterTier::NORMAL)) ==
           (std::set<std::string_view>{ "CultistsNormal", "SewerClamNormal", "CorpseSlugsNormal",
                                        "PunchConstructNormal", "GremlinMercNormal", "HauntedShipNormal",
                                        "FossilStalkerNormal", "SeapunkNormal" }));
    for (ActZone zone : ZONES) {
        assert(ids_of(encounter_table(zone, EncounterTier::BOSS)) ==
               std::set<std::string_view>{ "CeremonialBeastBoss" });
        assert(ids_of(encounter_table(zone, EncounterTier::ELITE)) ==
               std::set<std::string_view>{ zone == ActZone::ACT2 ? "EntomancerElite" : "LegacyGremlinNob" });
    }
    assert(ids_of(encounter_table(ActZone::ACT1B, EncounterTier::WEAK)) ==
           (std::set<std::string_view>{ "CorpseSlugsWeak", "SludgeSpinnerWeak", "ToadpolesWeak", "SeapunkWeak" }));
    // The groups the game had before, where the reference has nothing the engine can play yet.
    for (ActZone zone : { ActZone::ACT2, ActZone::ACT3 }) {
        assert(ids_of(encounter_table(zone, EncounterTier::WEAK)) == std::set<std::string_view>{ "LegacyCultist" });
    }
    assert(ids_of(encounter_table(ActZone::ACT2, EncounterTier::NORMAL)) ==
           (std::set<std::string_view>{ "HunterKillerNormal", "SpinyToadNormal" }));
    assert(ids_of(encounter_table(ActZone::ACT3, EncounterTier::NORMAL)) ==
           (std::set<std::string_view>{ "LegacyCultistAndLouse", "LegacyJawWorm", "LegacyLice" }));

    // The ruby raiders: three of the five raiders in the engine.
    const Encounter& raiders = find_encounter(encounter_table(ActZone::ACT1A, EncounterTier::NORMAL),
                                              "RubyRaidersNormal");
    assert(raiders.count == 3 && raiders.random_pool.size() == 5);
    const Encounter& inklets = find_encounter(encounter_table(ActZone::ACT1A, EncounterTier::NORMAL),
                                              "InkletsNormal");
    assert(inklets.count == 3 && inklets.monsters[0] == EnemyType::INKLET && inklets.monsters[2] == EnemyType::INKLET);

    // Every monster of the engine can be met.
    std::set<EnemyType> met;
    for (ActZone zone : ZONES) {
        for (EncounterTier tier : TIERS) {
            for (const Encounter& encounter : encounter_table(zone, tier)) {
                for (EnemyType type : possible_monsters(encounter)) met.insert(type);
            }
        }
    }
    // Except the ones whose only reference encounter needs a monster the engine does not have yet: the
    // Forgotten is only met with the Lost (TheLostAndForgottenNormal), the Living Shield with the Turret Operator
    // (TurretOperatorWeak), the Fat Gremlin with the Gremlin Merc (GremlinMercNormal), like the Sneaky Gremlin.
    // The Eye With Teeth is brought by the Fogmog (FogmogNormal).
    // The Stabbot and the Noisebot are brought by the Fabricator (FabricatorNormal).
    // The Gas Bomb is brought by the Living Fog (LivingFogNormal).
    const std::set<EnemyType> waiting_for_a_companion = { EnemyType::THE_FORGOTTEN, EnemyType::LIVING_SHIELD,
                                                          EnemyType::FAT_GREMLIN, EnemyType::EYE_WITH_TEETH,
                                                          EnemyType::STABBOT, EnemyType::NOISEBOT,
                                                          EnemyType::GAS_BOMB, EnemyType::SNEAKY_GREMLIN };
    for (int i = 0; !enemy_identifier(static_cast<EnemyType>(i)).empty(); ++i) {
        EnemyType type = static_cast<EnemyType>(i);
        assert(met.count(type) == (waiting_for_a_companion.count(type) ? 0 : 1));
    }
    std::cout << "[PASS] Encounter tables: the reference encounters, the legacy groups elsewhere" << std::endl;
}

void test_zone_and_tier() {
    std::set<ActZone> act1;
    for (uint32_t seed = 1; seed <= SEEDS; ++seed) {
        act1.insert(act_zone(1, seed * 2654435761u));
        assert(act_zone(2, seed) == ActZone::ACT2);
        assert(act_zone(3, seed) == ActZone::ACT3);
    }
    assert(act1 == (std::set<ActZone>{ ActZone::ACT1A, ActZone::ACT1B }));

    for (uint8_t floor = 1; floor <= WEAK_ENCOUNTER_FLOORS; ++floor) {
        assert(encounter_tier(RoomType::MONSTER, floor) == EncounterTier::WEAK);
    }
    assert(encounter_tier(RoomType::MONSTER, WEAK_ENCOUNTER_FLOORS + 1) == EncounterTier::NORMAL);
    assert(encounter_tier(RoomType::MONSTER, 15) == EncounterTier::NORMAL);
    assert(encounter_tier(RoomType::ELITE, 2) == EncounterTier::ELITE);
    assert(encounter_tier(RoomType::BOSS, 16) == EncounterTier::BOSS);
    std::cout << "[PASS] Encounter tables: Act 1 plays variant A or B, the first floors are weak" << std::endl;
}

// Over many seeds, every room tier of every act meets each encounter of its list, and nothing else.
void test_rooms_meet_every_encounter() {
    for (uint8_t act = 1; act <= 3; ++act) {
        std::set<ActZone> zones_met;
        std::set<std::string_view> met[4][4];
        std::set<std::set<EnemyType>> raider_groups;
        for (uint32_t seed = 1; seed <= SEEDS; ++seed) {
            for (EncounterTier tier : TIERS) {
                RunManager run = make_run(seed, act);
                int room = find_room(run, tier);
                if (room < 0) continue;
                ActZone zone = act_zone(run.current_act, run.map_seed);
                zones_met.insert(zone);
                run.enter_room(room);
                std::span<const Encounter> table = encounter_table(zone, tier);
                assert(contains(table, run.current_encounter));
                const Encounter& encounter = find_encounter(table, run.current_encounter);
                check_fight_matches(run, encounter);
                met[static_cast<int>(zone)][static_cast<int>(tier)].insert(encounter.id);
                if (!encounter.random_pool.empty()) {
                    std::set<EnemyType> group;
                    for (size_t i = 0; i < run.combat.enemies.size(); ++i) group.insert(run.combat.enemies[i].type);
                    raider_groups.insert(group);
                }
            }
        }
        for (ActZone zone : zones_met) {
            for (EncounterTier tier : TIERS) {
                assert(met[static_cast<int>(zone)][static_cast<int>(tier)] == ids_of(encounter_table(zone, tier)));
            }
        }
        if (act == 1) {
            assert(zones_met == (std::set<ActZone>{ ActZone::ACT1A, ActZone::ACT1B }));
            assert(raider_groups.size() >= 8); // nearly every one of the 10 trios of the five raiders
        } else {
            assert(zones_met.size() == 1);
        }
    }
    std::cout << "[PASS] Encounter tables: each act meets every encounter of its lists, elites and bosses included"
              << std::endl;
}

// The fights of a run, room after room along its first path, then the elite and the boss.
std::vector<std::string> fights_of(uint32_t seed) {
    RunManager run = make_run(seed, 1);
    std::vector<std::string> fights;
    auto record = [&] {
        std::string fight(run.current_encounter);
        for (size_t i = 0; i < run.combat.enemies.size(); ++i) {
            fight += ' ';
            fight += enemy_identifier(run.combat.enemies[i].type);
        }
        fights.push_back(fight);
    };
    for (int step = 0; step < 15; ++step) {
        FixedVector<int, 3> rooms = run.get_available_rooms();
        if (rooms.empty()) break;
        int room = rooms[0];
        run.map.nodes[room].type = RoomType::MONSTER; // a fight on every floor
        run.enter_room(room);
        record();
    }
    run.map.nodes[0].type = RoomType::ELITE;
    run.enter_room(0);
    record();
    run.enter_room(run.map.boss_node_index);
    record();
    return fights;
}

void test_same_seed_same_fights() {
    for (uint32_t seed : { 1u, 42u, 99u, 777u, 4242u }) {
        std::vector<std::string> first = fights_of(seed);
        assert(first.size() >= 10);
        assert(first == fights_of(seed));
    }
    // A run of Act 1 (A): weak fights on the first floors, then the ruby raiders or the Vine Shambler, both met.
    uint32_t seed = 1;
    assert(act_zone(1, make_run(seed, 1).map_seed) == ActZone::ACT1A);
    std::vector<std::string> fights = fights_of(seed);
    int raiders = 0;
    int shamblers = 0;
    int inklets = 0;
    for (size_t floor = 1; floor <= fights.size() - 2; ++floor) {
        std::string_view fight = fights[floor - 1];
        if (floor <= WEAK_ENCOUNTER_FLOORS) {
            assert(fight.starts_with("FuzzyWurmCrawlerWeak") || fight == "NibbitsWeak NIBBIT");
        } else if (fight.starts_with("RubyRaidersNormal")) {
            ++raiders;
        } else if (fight.starts_with("NibbitsNormal")) {
            assert(fight == "NibbitsNormal NIBBIT NIBBIT");
        } else {
            if (fight.starts_with("InkletsNormal")) {
                assert(fight == "InkletsNormal INKLET INKLET INKLET");
                ++inklets;
            } else {
                assert(fight == "VineShamblerNormal VINE_SHAMBLER" || fight == "CubexConstructNormal CUBEX_CONSTRUCT" ||
                       fight == "FlyconidNormal FLYCONID" || fight == "MawlerNormal MAWLER");
                if (fight.starts_with("VineShamblerNormal") || fight.starts_with("CubexConstructNormal")) ++shamblers;
            }
        }
    }
    assert(raiders > 0 && shamblers > 0);
    assert(fights[fights.size() - 1] == "CeremonialBeastBoss CEREMONIAL_BEAST");
    std::cout << "[PASS] Encounter tables: the same seed gives the same fights" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Encounters ===" << std::endl;
    test_tables_use_existing_monsters_with_a_sprite();
    test_reference_encounters();
    test_zone_and_tier();
    test_rooms_meet_every_encounter();
    test_same_seed_same_fights();
    std::cout << "=== All Encounters Passed! ===" << std::endl;
    return 0;
}
