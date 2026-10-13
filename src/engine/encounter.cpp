#include "engine/encounter.h"

namespace engine {

namespace {

using namespace std::string_view_literals;

// Static constexpr data only: the tables live in the cartridge's ROM, never in the GBA's small RAM.
// Source: bible/data/monster_v0.111.0.json ("encounters" of each monster) and bible/16_bestiaire_monstres.md.
// Only the encounters whose every monster exists in EnemyType are listed; the reference gives no weights,
// so every encounter of a list is as likely as the others.

// RubyRaidersNormal is three raiders drawn among the five Ruby Raiders.
constexpr EnemyType RUBY_RAIDERS[] = {
    EnemyType::CROSSBOW_RUBY_RAIDER,
    EnemyType::BRUTE_RUBY_RAIDER,
    EnemyType::ASSASSIN_RUBY_RAIDER,
    EnemyType::AXE_RUBY_RAIDER,
    EnemyType::TRACKER_RUBY_RAIDER,
};

// Act 1 (A): the Overgrowth.
constexpr Encounter ACT1A_WEAK[] = {
    { "FuzzyWurmCrawlerWeak"sv, 1, { EnemyType::FUZZY_WURM_CRAWLER } },
    { "NibbitsWeak"sv, 1, { EnemyType::NIBBIT } },
    { "ShrinkerBeetleWeak"sv, 1, { EnemyType::SHRINKER_BEETLE } },
    { "SlimesWeak"sv, 4, { EnemyType::LEAF_SLIME_M, EnemyType::LEAF_SLIME_S, EnemyType::TWIG_SLIME_M, EnemyType::TWIG_SLIME_S } },
};
constexpr Encounter ACT1A_NORMAL[] = {
    { "RubyRaidersNormal"sv, 3, {}, RUBY_RAIDERS },
    { "VineShamblerNormal"sv, 1, { EnemyType::VINE_SHAMBLER } },
    { "CubexConstructNormal"sv, 1, { EnemyType::CUBEX_CONSTRUCT } },
    { "InkletsNormal"sv, 3, { EnemyType::INKLET, EnemyType::INKLET, EnemyType::INKLET } },
    { "NibbitsNormal"sv, 2, { EnemyType::NIBBIT, EnemyType::NIBBIT } },
    { "FlyconidNormal"sv, 1, { EnemyType::FLYCONID } },
    { "MawlerNormal"sv, 1, { EnemyType::MAWLER } },
    { "OvergrowthCrawlers"sv, 2, { EnemyType::FUZZY_WURM_CRAWLER, EnemyType::SHRINKER_BEETLE } },
    { "FogmogNormal"sv, 1, { EnemyType::FOGMOG } },
    { "SnappingJaxfruitNormal"sv, 2, { EnemyType::FLYCONID, EnemyType::SNAPPING_JAXFRUIT } },
    { "SlimesNormal"sv, 2, { EnemyType::LEAF_SLIME_M, EnemyType::TWIG_SLIME_M } },
    { "SlitheringStranglerNormal"sv, 6,
      { EnemyType::LEAF_SLIME_M, EnemyType::LEAF_SLIME_S, EnemyType::SLITHERING_STRANGLER, EnemyType::SNAPPING_JAXFRUIT,
        EnemyType::TWIG_SLIME_M, EnemyType::TWIG_SLIME_S } },
};

// Act 1 (B): the Underdocks.
constexpr Encounter ACT1B_WEAK[] = {
    { "CorpseSlugsWeak"sv, 2, { EnemyType::CORPSE_SLUG, EnemyType::CORPSE_SLUG } },
    { "SludgeSpinnerWeak"sv, 1, { EnemyType::SLUDGE_SPINNER } },
    { "ToadpolesWeak"sv, 2, { EnemyType::TOADPOLE, EnemyType::TOADPOLE } },
    { "SeapunkWeak"sv, 1, { EnemyType::SEAPUNK } },
};
constexpr Encounter ACT1B_NORMAL[] = {
    { "CultistsNormal"sv, 2, { EnemyType::CALCIFIED_CULTIST, EnemyType::DAMP_CULTIST } },
    { "SewerClamNormal"sv, 1, { EnemyType::SEWER_CLAM } },
    { "CorpseSlugsNormal"sv, 3, { EnemyType::CORPSE_SLUG, EnemyType::CORPSE_SLUG, EnemyType::CORPSE_SLUG } },
    { "PunchConstructNormal"sv, 1, { EnemyType::PUNCH_CONSTRUCT } },
    { "GremlinMercNormal"sv, 1, { EnemyType::GREMLIN_MERC } },
    { "HauntedShipNormal"sv, 1, { EnemyType::HAUNTED_SHIP } },
    { "FossilStalkerNormal"sv, 1, { EnemyType::FOSSIL_STALKER } },
    { "SeapunkNormal"sv, 2, { EnemyType::CALCIFIED_CULTIST, EnemyType::SEAPUNK } },
    { "LivingFogNormal"sv, 1, { EnemyType::LIVING_FOG } },
    { "TwoTailedRatsNormal"sv, 3, { EnemyType::TWO_TAILED_RAT, EnemyType::TWO_TAILED_RAT, EnemyType::TWO_TAILED_RAT } },
};

// Act 2: the Hive.
constexpr Encounter ACT2_WEAK[] = {
    { "TunnelerWeak"sv, 1, { EnemyType::TUNNELER } },
    { "BowlbugsWeak"sv, 1, { EnemyType::BOWLBUG_ROCK } },
    { "ExoskeletonsWeak"sv, 3, { EnemyType::EXOSKELETON, EnemyType::EXOSKELETON, EnemyType::EXOSKELETON } },
    { "ThievingHopperWeak"sv, 1, { EnemyType::THIEVING_HOPPER } },
};
constexpr Encounter ACT2_NORMAL[] = {
    { "HunterKillerNormal"sv, 1, { EnemyType::HUNTER_KILLER } },
    { "SpinyToadNormal"sv, 1, { EnemyType::SPINY_TOAD } },
    { "ChompersNormal"sv, 2, { EnemyType::CHOMPER, EnemyType::CHOMPER } },
    { "TunnelerNormal"sv, 2, { EnemyType::CHOMPER, EnemyType::TUNNELER } },
    { "LouseProgenitorNormal"sv, 1, { EnemyType::LOUSE_PROGENITOR } },
    { "TheObscuraNormal"sv, 1, { EnemyType::THE_OBSCURA } },
    { "OvicopterNormal"sv, 1, { EnemyType::OVICOPTER } },
    { "ExoskeletonsNormal"sv, 4, { EnemyType::EXOSKELETON, EnemyType::EXOSKELETON, EnemyType::EXOSKELETON, EnemyType::EXOSKELETON } },
    { "MytesNormal"sv, 2, { EnemyType::MYTE, EnemyType::MYTE } },
};

// Act 3: the Glory. The groups the game had before stay beside them: the Louse and the Jaw Worm are met there.
constexpr Encounter ACT3_WEAK[] = {
    { "DevotedSculptorWeak"sv, 1, { EnemyType::DEVOTED_SCULPTOR } },
    { "TurretOperatorWeak"sv, 2, { EnemyType::LIVING_SHIELD, EnemyType::TURRET_OPERATOR } },
    { "ScrollsOfBitingWeak"sv, 3, { EnemyType::SCROLL_OF_BITING, EnemyType::SCROLL_OF_BITING, EnemyType::SCROLL_OF_BITING } },
};
constexpr Encounter ACT3_NORMAL[] = {
    { "SlimedBerserkerNormal"sv, 1, { EnemyType::SLIMED_BERSERKER } },
    { "FrogKnightNormal"sv, 1, { EnemyType::FROG_KNIGHT } },
    { "GlobeHeadNormal"sv, 1, { EnemyType::GLOBE_HEAD } },
    { "TheLostAndForgottenNormal"sv, 2, { EnemyType::THE_FORGOTTEN, EnemyType::THE_LOST } },
    { "AxebotsNormal"sv, 1, { EnemyType::AXEBOT } },
    { "FabricatorNormal"sv, 1, { EnemyType::FABRICATOR } },
    { "OwlMagistrateNormal"sv, 1, { EnemyType::OWL_MAGISTRATE } },
    { "ScrollsOfBitingNormal"sv, 4, { EnemyType::SCROLL_OF_BITING, EnemyType::SCROLL_OF_BITING, EnemyType::SCROLL_OF_BITING, EnemyType::SCROLL_OF_BITING } },
    { "LegacyCultistAndLouse"sv, 2, { EnemyType::CULTIST, EnemyType::LOUSE } },
    { "LegacyJawWorm"sv, 1, { EnemyType::JAW_WORM } },
    { "LegacyLice"sv, 2, { EnemyType::LOUSE, EnemyType::LOUSE } },
};

// Act 1 (A) elites: the reference ones the engine can play, and the Gremlin Nob the game had before.
constexpr Encounter ACT1A_ELITE[] = {
    { "BygoneEffigyElite"sv, 1, { EnemyType::BYGONE_EFFIGY } },
    { "ByrdonisElite"sv, 1, { EnemyType::BYRDONIS } },
    { "PhrogParasiteElite"sv, 1, { EnemyType::PHROG_PARASITE } },
    { "LegacyGremlinNob"sv, 1, { EnemyType::GREMLIN_NOB } },
};
constexpr Encounter ACT1B_ELITE[] = {
    { "SkulkingColonyElite"sv, 1, { EnemyType::SKULKING_COLONY } },
    { "TerrorEelElite"sv, 1, { EnemyType::TERROR_EEL } },
    { "PhantasmalGardenersElite"sv, 4, { EnemyType::PHANTASMAL_GARDENER, EnemyType::PHANTASMAL_GARDENER, EnemyType::PHANTASMAL_GARDENER, EnemyType::PHANTASMAL_GARDENER } },
};
constexpr Encounter ACT2_ELITE[] = {
    { "EntomancerElite"sv, 1, { EnemyType::ENTOMANCER } },
    { "InfestedPrismsElite"sv, 1, { EnemyType::INFESTED_PRISM } },
    { "DecimillipedeElite"sv, 3, { EnemyType::DECIMILLIPEDE_SEGMENT_BACK, EnemyType::DECIMILLIPEDE_SEGMENT_FRONT,
                                   EnemyType::DECIMILLIPEDE_SEGMENT_MIDDLE } },
};
constexpr Encounter ACT3_ELITE[] = {
    { "SoulNexusElite"sv, 1, { EnemyType::SOUL_NEXUS } },
    { "MechaKnightElite"sv, 1, { EnemyType::MECHA_KNIGHT } },
    { "KnightsElite"sv, 3, { EnemyType::FLAIL_KNIGHT, EnemyType::MAGI_KNIGHT, EnemyType::SPECTRAL_KNIGHT } },
};

constexpr Encounter ACT1B_BOSS[] = {
    { "LagavulinMatriarchBoss"sv, 1, { EnemyType::LAGAVULIN_MATRIARCH } },
    { "SoulFyshBoss"sv, 1, { EnemyType::SOUL_FYSH } },
    { "WaterfallGiantBoss"sv, 1, { EnemyType::WATERFALL_GIANT } },
};
constexpr Encounter ACT3_BOSS[] = {
    { "TestSubjectBoss"sv, 1, { EnemyType::TEST_SUBJECT } },
    { "AeonglassBoss"sv, 1, { EnemyType::AEONGLASS } },
    { "QueenBoss"sv, 2, { EnemyType::QUEEN, EnemyType::TORCH_HEAD_AMALGAM } },
};
constexpr Encounter ACT1A_BOSS[] = {
    { "CeremonialBeastBoss"sv, 1, { EnemyType::CEREMONIAL_BEAST } },
    { "VantomBoss"sv, 1, { EnemyType::VANTOM } },
    { "TheKinBoss"sv, 3, { EnemyType::KIN_FOLLOWER, EnemyType::KIN_FOLLOWER, EnemyType::KIN_PRIEST } },
};
constexpr Encounter ACT2_BOSS[] = {
    { "TheInsatiableBoss"sv, 1, { EnemyType::THE_INSATIABLE } },
    { "KnowledgeDemonBoss"sv, 1, { EnemyType::KNOWLEDGE_DEMON } },
    { "KaiserCrabBoss"sv, 2, { EnemyType::CRUSHER, EnemyType::ROCKET } },
};

// [zone][tier], in the order of ActZone and EncounterTier.
constexpr std::span<const Encounter> TABLES[4][4] = {
    { ACT1A_WEAK, ACT1A_NORMAL, ACT1A_ELITE, ACT1A_BOSS },
    { ACT1B_WEAK, ACT1B_NORMAL, ACT1B_ELITE, ACT1B_BOSS },
    { ACT2_WEAK, ACT2_NORMAL, ACT2_ELITE, ACT2_BOSS },
    { ACT3_WEAK, ACT3_NORMAL, ACT3_ELITE, ACT3_BOSS },
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
