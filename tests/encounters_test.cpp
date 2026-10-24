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
        case EnemyType::TERROR_EEL: return "TERROR_EEL";
        case EnemyType::MECHA_KNIGHT: return "MECHA_KNIGHT";
        case EnemyType::CALCIFIED_CULTIST: return "CALCIFIED_CULTIST";
        case EnemyType::DAMP_CULTIST: return "DAMP_CULTIST";
        case EnemyType::SEWER_CLAM: return "SEWER_CLAM";
        case EnemyType::THE_FORGOTTEN: return "THE_FORGOTTEN";
        case EnemyType::LIVING_SHIELD: return "LIVING_SHIELD";
        case EnemyType::FAT_GREMLIN: return "FAT_GREMLIN";
        case EnemyType::VINE_SHAMBLER: return "VINE_SHAMBLER";
        case EnemyType::EYE_WITH_TEETH: return "EYE_WITH_TEETH";
        case EnemyType::STABBOT: return "STABBOT";
        case EnemyType::SPECTRAL_KNIGHT: return "SPECTRAL_KNIGHT";
        case EnemyType::SPINY_TOAD: return "SPINY_TOAD";
        case EnemyType::NOISEBOT: return "NOISEBOT";
        case EnemyType::PARAFRIGHT: return "PARAFRIGHT";
        case EnemyType::ZAPBOT: return "ZAPBOT";
        case EnemyType::CUBEX_CONSTRUCT: return "CUBEX_CONSTRUCT";
        case EnemyType::PUNCH_CONSTRUCT: return "PUNCH_CONSTRUCT";
        case EnemyType::BOWLBUG_NECTAR: return "BOWLBUG_NECTAR";
        case EnemyType::AXEBOT: return "AXEBOT";
        case EnemyType::BYGONE_EFFIGY: return "BYGONE_EFFIGY";
        case EnemyType::TURRET_OPERATOR: return "TURRET_OPERATOR";
        case EnemyType::BOWLBUG_ROCK: return "BOWLBUG_ROCK";
        case EnemyType::SKULKING_COLONY: return "SKULKING_COLONY";
        case EnemyType::MAGI_KNIGHT: return "MAGI_KNIGHT";
        case EnemyType::INKLET: return "INKLET";
        case EnemyType::NIBBIT: return "NIBBIT";
        case EnemyType::FLYCONID: return "FLYCONID";
        case EnemyType::CORPSE_SLUG: return "CORPSE_SLUG";
        case EnemyType::GAS_BOMB: return "GAS_BOMB";
        case EnemyType::GREMLIN_MERC: return "GREMLIN_MERC";
        case EnemyType::SNEAKY_GREMLIN: return "SNEAKY_GREMLIN";
        case EnemyType::HAUNTED_SHIP: return "HAUNTED_SHIP";
        case EnemyType::GLOBE_HEAD: return "GLOBE_HEAD";
        case EnemyType::LOUSE_PROGENITOR: return "LOUSE_PROGENITOR";
        case EnemyType::SLUDGE_SPINNER: return "SLUDGE_SPINNER";
        case EnemyType::TOADPOLE: return "TOADPOLE";
        case EnemyType::HUNTER_KILLER: return "HUNTER_KILLER";
        case EnemyType::MAWLER: return "MAWLER";
        case EnemyType::FOSSIL_STALKER: return "FOSSIL_STALKER";
        case EnemyType::SEAPUNK: return "SEAPUNK";
        case EnemyType::OSTY: return "OSTY";
        case EnemyType::CHOMPER: return "CHOMPER";
        case EnemyType::SLUMBERING_BEETLE: return "SLUMBERING_BEETLE";
        case EnemyType::TUNNELER: return "TUNNELER";
        case EnemyType::SLIMED_BERSERKER: return "SLIMED_BERSERKER";
        case EnemyType::FROG_KNIGHT: return "FROG_KNIGHT";
        case EnemyType::GUARDBOT: return "GUARDBOT";
        case EnemyType::DEVOTED_SCULPTOR: return "DEVOTED_SCULPTOR";
        case EnemyType::THE_LOST: return "THE_LOST";
        case EnemyType::BOWLBUG_SILK: return "BOWLBUG_SILK";
        case EnemyType::BOWLBUG_EGG: return "BOWLBUG_EGG";
        case EnemyType::ENTOMANCER: return "ENTOMANCER";
        case EnemyType::INFESTED_PRISM: return "INFESTED_PRISM";
        case EnemyType::FLAIL_KNIGHT: return "FLAIL_KNIGHT";
        case EnemyType::SOUL_NEXUS: return "SOUL_NEXUS";
        case EnemyType::BYRDONIS: return "BYRDONIS";
        case EnemyType::PHROG_PARASITE: return "PHROG_PARASITE";
        case EnemyType::WRIGGLER: return "WRIGGLER";
        case EnemyType::THE_INSATIABLE: return "THE_INSATIABLE";
        case EnemyType::TOUGH_EGG: return "TOUGH_EGG";
        case EnemyType::THE_OBSCURA: return "THE_OBSCURA";
        case EnemyType::OVICOPTER: return "OVICOPTER";
        case EnemyType::FABRICATOR: return "FABRICATOR";
        case EnemyType::SHRINKER_BEETLE: return "SHRINKER_BEETLE";
        case EnemyType::OWL_MAGISTRATE: return "OWL_MAGISTRATE";
        case EnemyType::SCROLL_OF_BITING: return "SCROLL_OF_BITING";
        case EnemyType::FOGMOG: return "FOGMOG";
        case EnemyType::SNAPPING_JAXFRUIT: return "SNAPPING_JAXFRUIT";
        case EnemyType::TWIG_SLIME_M: return "TWIG_SLIME_M";
        case EnemyType::TWIG_SLIME_S: return "TWIG_SLIME_S";
        case EnemyType::LEAF_SLIME_M: return "LEAF_SLIME_M";
        case EnemyType::LEAF_SLIME_S: return "LEAF_SLIME_S";
        case EnemyType::SLITHERING_STRANGLER: return "SLITHERING_STRANGLER";
        case EnemyType::LIVING_FOG: return "LIVING_FOG";
        case EnemyType::TWO_TAILED_RAT: return "TWO_TAILED_RAT";
        case EnemyType::LAGAVULIN_MATRIARCH: return "LAGAVULIN_MATRIARCH";
        case EnemyType::KNOWLEDGE_DEMON: return "KNOWLEDGE_DEMON";
        case EnemyType::TEST_SUBJECT: return "TEST_SUBJECT";
        case EnemyType::WATERFALL_GIANT: return "WATERFALL_GIANT";
        case EnemyType::KIN_FOLLOWER: return "KIN_FOLLOWER";
        case EnemyType::KIN_PRIEST: return "KIN_PRIEST";
        case EnemyType::VANTOM: return "VANTOM";
        case EnemyType::SOUL_FYSH: return "SOUL_FYSH";
        case EnemyType::AEONGLASS: return "AEONGLASS";
        case EnemyType::QUEEN: return "QUEEN";
        case EnemyType::TORCH_HEAD_AMALGAM: return "TORCH_HEAD_AMALGAM";
        case EnemyType::CRUSHER: return "CRUSHER";
        case EnemyType::ROCKET: return "ROCKET";
        case EnemyType::PHANTASMAL_GARDENER: return "PHANTASMAL_GARDENER";
        case EnemyType::EXOSKELETON: return "EXOSKELETON";
        case EnemyType::DECIMILLIPEDE_SEGMENT_BACK: return "DECIMILLIPEDE_SEGMENT_BACK";
        case EnemyType::DECIMILLIPEDE_SEGMENT_FRONT: return "DECIMILLIPEDE_SEGMENT_FRONT";
        case EnemyType::DECIMILLIPEDE_SEGMENT_MIDDLE: return "DECIMILLIPEDE_SEGMENT_MIDDLE";
        case EnemyType::MYTE: return "MYTE";
        case EnemyType::THIEVING_HOPPER: return "THIEVING_HOPPER";
        case EnemyType::BATTLEWORN_DUMMY: return "BATTLEWORN_DUMMY";
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
                // A fight holds up to MAX_ENEMIES enemies (CombatEngine::enemies, and their places on the GBA).
                assert(encounter.count >= 1 && encounter.count <= MAX_ENEMIES);
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
    std::cout << "[PASS] Encounter tables: existing monsters, 1 to 6 per fight, a sprite each" << std::endl;
}

void test_reference_encounters() {
    assert(ids_of(encounter_table(ActZone::ACT1A, EncounterTier::WEAK)) ==
           (std::set<std::string_view>{ "FuzzyWurmCrawlerWeak", "NibbitsWeak", "ShrinkerBeetleWeak", "SlimesWeak" }));
    assert(ids_of(encounter_table(ActZone::ACT1A, EncounterTier::NORMAL)) ==
           (std::set<std::string_view>{ "RubyRaidersNormal", "VineShamblerNormal", "CubexConstructNormal",
                                        "InkletsNormal", "NibbitsNormal", "FlyconidNormal", "MawlerNormal",
                                        "OvergrowthCrawlers", "FogmogNormal", "SnappingJaxfruitNormal", "SlimesNormal",
                                        "SlitheringStranglerNormal" }));
    assert(ids_of(encounter_table(ActZone::ACT1B, EncounterTier::NORMAL)) ==
           (std::set<std::string_view>{ "CultistsNormal", "SewerClamNormal", "CorpseSlugsNormal",
                                        "PunchConstructNormal", "GremlinMercNormal", "HauntedShipNormal",
                                        "FossilStalkerNormal", "SeapunkNormal", "LivingFogNormal",
                                        "TwoTailedRatsNormal" }));
    // Act 1 (A) draws its boss between the Ceremonial Beast, the Vantom and the Kin, Act 1 (B) between the Lagavulin
    // Matriarch, the Soul Fysh and the Waterfall Giant, Act 2 between the Insatiable, the Knowledge Demon and the
    // Crusher with the Rocket, Act 3 between the Test Subject, the Aeonglass and the Queen.
    for (ActZone zone : ZONES) {
        std::set<std::string_view> bosses{ "TestSubjectBoss", "AeonglassBoss", "QueenBoss" };
        if (zone == ActZone::ACT1A) bosses = { "CeremonialBeastBoss", "VantomBoss", "TheKinBoss" };
        if (zone == ActZone::ACT2) bosses = { "TheInsatiableBoss", "KnowledgeDemonBoss", "KaiserCrabBoss" };
        if (zone == ActZone::ACT1B) bosses = { "LagavulinMatriarchBoss", "SoulFyshBoss", "WaterfallGiantBoss" };
        assert(ids_of(encounter_table(zone, EncounterTier::BOSS)) == bosses);
    }
    assert(ids_of(encounter_table(ActZone::ACT2, EncounterTier::BOSS)) ==
           (std::set<std::string_view>{ "TheInsatiableBoss", "KnowledgeDemonBoss", "KaiserCrabBoss" }));
    const Encounter& demon = find_encounter(encounter_table(ActZone::ACT2, EncounterTier::BOSS), "KnowledgeDemonBoss");
    assert(demon.count == 1 && demon.monsters[0] == EnemyType::KNOWLEDGE_DEMON);
    const Encounter& crab = find_encounter(encounter_table(ActZone::ACT2, EncounterTier::BOSS), "KaiserCrabBoss");
    assert(crab.count == 2 && crab.monsters[0] == EnemyType::CRUSHER && crab.monsters[1] == EnemyType::ROCKET);
    const Encounter& vantom = find_encounter(encounter_table(ActZone::ACT1A, EncounterTier::BOSS), "VantomBoss");
    assert(vantom.count == 1 && vantom.monsters[0] == EnemyType::VANTOM);
    const Encounter& kin = find_encounter(encounter_table(ActZone::ACT1A, EncounterTier::BOSS), "TheKinBoss");
    assert(kin.count == 3 && kin.monsters[0] == EnemyType::KIN_FOLLOWER && kin.monsters[1] == EnemyType::KIN_FOLLOWER &&
           kin.monsters[2] == EnemyType::KIN_PRIEST);
    const Encounter& matriarch = find_encounter(encounter_table(ActZone::ACT1B, EncounterTier::BOSS),
                                                "LagavulinMatriarchBoss");
    assert(matriarch.count == 1 && matriarch.monsters[0] == EnemyType::LAGAVULIN_MATRIARCH);
    const Encounter& fysh = find_encounter(encounter_table(ActZone::ACT1B, EncounterTier::BOSS), "SoulFyshBoss");
    assert(fysh.count == 1 && fysh.monsters[0] == EnemyType::SOUL_FYSH);
    const Encounter& giant = find_encounter(encounter_table(ActZone::ACT1B, EncounterTier::BOSS), "WaterfallGiantBoss");
    assert(giant.count == 1 && giant.monsters[0] == EnemyType::WATERFALL_GIANT);
    const Encounter& insatiable = find_encounter(encounter_table(ActZone::ACT2, EncounterTier::BOSS),
                                                 "TheInsatiableBoss");
    assert(insatiable.count == 1 && insatiable.monsters[0] == EnemyType::THE_INSATIABLE);
    const Encounter& subject = find_encounter(encounter_table(ActZone::ACT3, EncounterTier::BOSS), "TestSubjectBoss");
    assert(subject.count == 1 && subject.monsters[0] == EnemyType::TEST_SUBJECT);
    const Encounter& aeonglass = find_encounter(encounter_table(ActZone::ACT3, EncounterTier::BOSS), "AeonglassBoss");
    assert(aeonglass.count == 1 && aeonglass.monsters[0] == EnemyType::AEONGLASS);
    const Encounter& queen = find_encounter(encounter_table(ActZone::ACT3, EncounterTier::BOSS), "QueenBoss");
    assert(queen.count == 2 && queen.monsters[0] == EnemyType::QUEEN &&
           queen.monsters[1] == EnemyType::TORCH_HEAD_AMALGAM);
    assert(ids_of(encounter_table(ActZone::ACT1A, EncounterTier::ELITE)) ==
           (std::set<std::string_view>{ "BygoneEffigyElite", "ByrdonisElite", "PhrogParasiteElite",
                                        "LegacyGremlinNob" }));
    assert(ids_of(encounter_table(ActZone::ACT1B, EncounterTier::ELITE)) ==
           (std::set<std::string_view>{ "SkulkingColonyElite", "TerrorEelElite", "PhantasmalGardenersElite" }));
    assert(ids_of(encounter_table(ActZone::ACT2, EncounterTier::ELITE)) ==
           (std::set<std::string_view>{ "EntomancerElite", "InfestedPrismsElite", "DecimillipedeElite" }));
    assert(ids_of(encounter_table(ActZone::ACT3, EncounterTier::ELITE)) ==
           (std::set<std::string_view>{ "SoulNexusElite", "MechaKnightElite", "KnightsElite" }));
    assert(ids_of(encounter_table(ActZone::ACT1B, EncounterTier::WEAK)) ==
           (std::set<std::string_view>{ "CorpseSlugsWeak", "SludgeSpinnerWeak", "ToadpolesWeak", "SeapunkWeak" }));
    // Acts 2 and 3: the reference encounters the engine can play, with the groups the game had before in Act 3.
    assert(ids_of(encounter_table(ActZone::ACT2, EncounterTier::WEAK)) ==
           (std::set<std::string_view>{ "TunnelerWeak", "BowlbugsWeak", "ExoskeletonsWeak",
                                        "ThievingHopperWeak" }));
    assert(ids_of(encounter_table(ActZone::ACT3, EncounterTier::WEAK)) ==
           (std::set<std::string_view>{ "DevotedSculptorWeak", "TurretOperatorWeak", "ScrollsOfBitingWeak" }));
    assert(ids_of(encounter_table(ActZone::ACT2, EncounterTier::NORMAL)) ==
           (std::set<std::string_view>{ "HunterKillerNormal", "SpinyToadNormal", "ChompersNormal", "TunnelerNormal",
                                        "LouseProgenitorNormal", "TheObscuraNormal", "OvicopterNormal", "ExoskeletonsNormal",
                                        "MytesNormal" }));
    assert(ids_of(encounter_table(ActZone::ACT3, EncounterTier::NORMAL)) ==
           (std::set<std::string_view>{ "SlimedBerserkerNormal", "FrogKnightNormal", "GlobeHeadNormal",
                                        "TheLostAndForgottenNormal", "AxebotsNormal", "FabricatorNormal",
                                        "OwlMagistrateNormal", "ScrollsOfBitingNormal", "LegacyCultistAndLouse",
                                        "LegacyJawWorm", "LegacyLice" }));
    // Four Scrolls of Biting: the first encounter with more than three monsters.
    const Encounter& scrolls = find_encounter(encounter_table(ActZone::ACT3, EncounterTier::NORMAL),
                                              "ScrollsOfBitingNormal");
    assert(scrolls.count == 4);
    for (uint8_t i = 0; i < scrolls.count; ++i) assert(scrolls.monsters[i] == EnemyType::SCROLL_OF_BITING);
    const Encounter& weak_scrolls = find_encounter(encounter_table(ActZone::ACT3, EncounterTier::WEAK),
                                                   "ScrollsOfBitingWeak");
    assert(weak_scrolls.count == 3 && weak_scrolls.monsters[2] == EnemyType::SCROLL_OF_BITING);
    const Encounter& chompers = find_encounter(encounter_table(ActZone::ACT2, EncounterTier::NORMAL),
                                               "ChompersNormal");
    assert(chompers.count == 2 && chompers.monsters[0] == EnemyType::CHOMPER && chompers.monsters[1] == EnemyType::CHOMPER);
    const Encounter& turret = find_encounter(encounter_table(ActZone::ACT3, EncounterTier::WEAK), "TurretOperatorWeak");
    assert(turret.count == 2 && turret.monsters[0] == EnemyType::LIVING_SHIELD &&
           turret.monsters[1] == EnemyType::TURRET_OPERATOR);

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
    // Except the ones whose only reference encounter needs a monster the engine does not have yet: the Fat
    // Gremlin and the Sneaky Gremlin wait for their full Gremlin Merc group (GremlinMercNormal).
    // The Eye With Teeth is brought by the Fogmog (FogmogNormal).
    // The Stabbot, the Noisebot, the Zapbot and the Guardbot are summoned by the Fabricator (FabricatorNormal).
    // The Gas Bomb is brought by the Living Fog (LivingFogNormal), the Tough Egg laid by the Ovicopter
    // (OvicopterNormal).
    // Titos is summoned by the Entomancer, the Parafright brought by the Obscura: neither is a monster of the
    // engine yet. The Slumbering Beetle and the Nectar, Silk and Egg Bowlbugs wait for the random bugs of
    // BowlbugsNormal and SlumberingBeetleNormal.
    // The Wriggler is the companion of the Phrog Parasite, which fights alone for now.
    // The Battleworn Dummy is only met through its event (BattlewornDummyEvent), never drawn in a room.
    const std::set<EnemyType> waiting_for_a_companion = { EnemyType::FAT_GREMLIN, EnemyType::EYE_WITH_TEETH,
                                                          EnemyType::STABBOT, EnemyType::NOISEBOT,
                                                          EnemyType::GAS_BOMB, EnemyType::SNEAKY_GREMLIN,
                                                          EnemyType::OSTY, EnemyType::SLUMBERING_BEETLE,
                                                          EnemyType::PARAFRIGHT, EnemyType::ZAPBOT,
                                                          EnemyType::GUARDBOT, EnemyType::BOWLBUG_NECTAR,
                                                          EnemyType::BOWLBUG_SILK, EnemyType::BOWLBUG_EGG,
                                                          EnemyType::WRIGGLER, EnemyType::TOUGH_EGG,
                                                          EnemyType::BATTLEWORN_DUMMY };
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
    uint32_t seed = 2;
    assert(act_zone(1, make_run(seed, 1).map_seed) == ActZone::ACT1A);
    std::vector<std::string> fights = fights_of(seed);
    int raiders = 0;
    int shamblers = 0;
    int inklets = 0;
    for (size_t floor = 1; floor <= fights.size() - 2; ++floor) {
        std::string_view fight = fights[floor - 1];
        if (floor <= WEAK_ENCOUNTER_FLOORS) {
            assert(fight.starts_with("FuzzyWurmCrawlerWeak") || fight == "NibbitsWeak NIBBIT" ||
                   fight == "ShrinkerBeetleWeak SHRINKER_BEETLE" ||
                   fight == "SlimesWeak LEAF_SLIME_M LEAF_SLIME_S TWIG_SLIME_M TWIG_SLIME_S");
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
                       fight == "FlyconidNormal FLYCONID" || fight == "MawlerNormal MAWLER" ||
                       fight == "OvergrowthCrawlers FUZZY_WURM_CRAWLER SHRINKER_BEETLE" ||
                       fight == "FogmogNormal FOGMOG" || fight == "SnappingJaxfruitNormal FLYCONID SNAPPING_JAXFRUIT" ||
                       fight == "SlimesNormal LEAF_SLIME_M TWIG_SLIME_M" ||
                       fight == "SlitheringStranglerNormal LEAF_SLIME_M LEAF_SLIME_S SLITHERING_STRANGLER "
                                "SNAPPING_JAXFRUIT TWIG_SLIME_M TWIG_SLIME_S");
                if (fight.starts_with("VineShamblerNormal") || fight.starts_with("CubexConstructNormal")) ++shamblers;
            }
        }
    }
    assert(raiders > 0 && shamblers > 0);
    assert(fights[fights.size() - 1] == "CeremonialBeastBoss CEREMONIAL_BEAST" ||
           fights[fights.size() - 1] == "VantomBoss VANTOM" ||
           fights[fights.size() - 1] == "TheKinBoss KIN_FOLLOWER KIN_FOLLOWER KIN_PRIEST");
    std::cout << "[PASS] Encounter tables: the same seed gives the same fights" << std::endl;
}

// A room never draws the encounter of the previous fight, in any act, on any seed.
void test_no_same_encounter_twice_in_a_row() {
    int fights_checked = 0;
    for (uint8_t act = 1; act <= 3; ++act) {
        for (uint32_t seed = 1; seed <= 60; ++seed) {
            RunManager run = make_run(seed, act);
            std::string previous;
            for (int step = 0; step < 40; ++step) {
                int room = find_room(run, EncounterTier::NORMAL);
                if (room < 0) break;
                run.enter_room(room);
                assert(run.current_encounter != previous);
                previous = std::string(run.current_encounter);
                ++fights_checked;
            }
        }
    }
    assert(fights_checked > 1000);
    std::cout << "[PASS] Encounter tables: never the same encounter twice in a row" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Encounters ===" << std::endl;
    test_tables_use_existing_monsters_with_a_sprite();
    test_reference_encounters();
    test_zone_and_tier();
    test_rooms_meet_every_encounter();
    test_same_seed_same_fights();
    test_no_same_encounter_twice_in_a_row();
    std::cout << "=== All Encounters Passed! ===" << std::endl;
    return 0;
}
