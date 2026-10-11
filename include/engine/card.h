#pragma once

#include <cstdint>
#include <string_view>

namespace engine {

// 16 bits: the reference game has 569 cards.
enum class CardId : uint16_t {
    NONE = 0,
    STRIKE,
    DEFEND,
    BASH,
    IRON_WAVE,
    CLEAVE,
    ANGER,
    METALLICIZE,
    // Silent
    NEUTRALIZE,
    SURVIVOR,
    POISON_STAB,
    DEADLY_POISON,
    // Defect
    ZAP,
    DUALCAST,
    // Regent
    FALLING_STAR,
    VENERATE,
    // Necrobinder
    UNLEASH,
    BODYGUARD,
    // Ironclad commons (appended: saves store the numeric id)
    TWIN_STRIKE,
    POMMEL_STRIKE,
    SHRUG_IT_OFF,
    TAUNT,
    // Silent commons (appended: saves store the numeric id)
    SLICE,
    BACKFLIP,
    SUCKER_PUNCH,
    DAGGER_SPRAY,
    // Defect commons (appended: saves store the numeric id)
    BEAM_CELL,
    BALL_LIGHTNING,
    COLD_SNAP,
    LEAP,
    // Regent commons (appended: saves store the numeric id)
    SOLAR_STRIKE,
    GATHER_LIGHT,
    CELESTIAL_MIGHT,
    // Necrobinder commons (appended: saves store the numeric id)
    PULL_AGGRO,
    DRAIN_POWER,
    // More Ironclad commons (appended: saves store the numeric id)
    BODY_SLAM,
    // More Silent commons (appended: saves store the numeric id)
    DEFLECT,
    // More Defect commons (appended: saves store the numeric id)
    COOLHEADED,
    SWEEPING_BEAM,
    // Ironclad cards on existing mechanics (appended: saves store the numeric id)
    THUNDERCLAP,
    DISMANTLE,
    INFLAME,
    PERFECTED_STRIKE,
    BLUDGEON,
    BULLY,
    SWORD_BOOMERANG,
    BREAK,
    CONFLAGRATION,
    EXPECT_A_FIGHT,
    PILLAGE,
    UPPERCUT,
    FIGHT_ME,
    // Silent cards on existing mechanics (appended: saves store the numeric id)
    LEG_SWEEP,
    HAZE,
    PRECISE_CUT,
    BOUNCING_FLASK,
    FLECHETTES,
    FOOTWORK,
    ESCAPE_PLAN,
    DASH,
    BUBBLE_BUBBLE,
    OUTBREAK,
    // Defect cards on existing mechanics (appended: saves store the numeric id)
    SHADOW_SHIELD,
    METEOR_STRIKE,
    GLACIER,
    ICE_LANCE,
    SKIM,
    ALL_FOR_ONE,
    NULL_CARD,
    GO_FOR_THE_EYES,
    SCRAPE,
    // Necrobinder cards on existing mechanics (appended: saves store the numeric id)
    BURY,
    MISERY,
    UNDEATH,
    // Ironclad Exhaust cards (appended: saves store the numeric id)
    ASHEN_STRIKE,
    TREMBLE,
    EVIL_EYE,
    MOLTEN_FIST,
    CINDER,
    SECOND_WIND,
    TRUE_GRIT,
    DOMINATE,
    FIEND_FIRE,
    THRASH,
    IMPERVIOUS,
    PACTS_END,
    NOT_YET,
    // Silent Exhaust cards (appended: saves store the numeric id)
    ADRENALINE,
    CALCULATED_GAMBLE,
    MIRAGE,
    // Defect Exhaust cards (appended: saves store the numeric id)
    RAINBOW,
    CHILL,
    FUSION,
    REBOOT,
    SUPERCRITICAL,
    DOUBLE_ENERGY,
    // Necrobinder Exhaust cards (appended: saves store the numeric id)
    SHARED_FATE,
    WISP,
    PUTREFY,
    REANIMATE,
    AFTERLIFE,
    // Ironclad cards on keywords, X cost, delayed effects and counters (appended)
    BLOOD_WALL,
    BLOODLETTING,
    BREAKTHROUGH,
    FORGOTTEN_RITUAL,
    HEMOKINESIS,
    MANGLE,
    OFFERING,
    SETUP_STRIKE,
    SPITE,
    WHIRLWIND,
    // Silent cards on keywords, X cost, delayed effects and counters (appended)
    ANTICIPATE,
    ASSASSINATE,
    BACKSTAB,
    BLUR,
    DODGE_AND_ROLL,
    FINISHER,
    MALAISE,
    MEMENTO_MORI,
    MURDER,
    PIERCING_WAIL,
    PREDATOR,
    SIDESTEP,
    SKEWER,
    SUPPRESS,
    EXPERTISE,
    SNAKEBITE,
    // Defect cards on keywords, X cost, delayed effects and counters (appended)
    BOOT_SEQUENCE,
    CHARGE_BATTERY,
    FTL,
    HELIX_DRILL,
    LIGHTNING_ROD,
    TEMPEST,
    // Regent cards on keywords, X cost, delayed effects and counters (appended)
    CRUSH_UNDER,
    GLITTERSTREAM,
    HEAVENLY_DRILL,
    HEGEMONY,
    KNOW_THY_PLACE,
    LUNAR_BLAST,
    PROPHESIZE,
    SUPERMASSIVE,
    CONVERGENCE,
    // Necrobinder cards on keywords, X cost, delayed effects and counters (appended)
    DEATH_MARCH,
    DEFILE,
    DEFY,
    DELAY,
    ENFEEBLING_TOUCH,
    FEAR,
    INVOKE,
    PARSE,
    PULL_FROM_BELOW,
    REAP,
    SOW,
    ERADICATE,
    // Regent Star cards (appended: saves store the numeric id)
    ALIGNMENT,
    ASTRAL_PULSE,
    CLOAK_OF_STARS,
    COMET,
    CRESCENT_SPEAR,
    DEVASTATE,
    DYING_STAR,
    GAMMA_BLAST,
    GLOW,
    GUIDING_STAR,
    HIDDEN_CACHE,
    METEOR_SHOWER,
    PARTICLE_WALL,
    RADIATE,
    RESONANCE,
    ROYAL_GAMBLE,
    SEVEN_STARS,
    SHINING_STRIKE,
    STARDUST,
    // Fatal and play conditions (appended: saves store the numeric id)
    FEED,
    BATTLE_TRANCE,
    ECHOING_SLASH,
    GRAND_FINALE,
    SUNDER,
    KNOCKOUT_BLOW,
    // Ironclad powers and triggered cards (appended: saves store the numeric id)
    FEEL_NO_PAIN,
    DARK_EMBRACE,
    DRUM_OF_BATTLE,
    BARRICADE,
    INFERNO,
    CRIMSON_MANTLE,
    RUPTURE,
    AGGRESSION,
    PYRE,
    DEMON_FORM,
    JUGGERNAUT,
    RAGE,
    VICIOUS,
    // Silent powers and triggered cards (appended: saves store the numeric id)
    ACCELERANT,
    SPEEDSTER,
    ENVENOM,
    SERPENT_FORM,
    AFTERIMAGE,
    WELL_LAID_PLANS,
    STRANGLE,
    CORROSIVE_WAVE,
    NOXIOUS_FUMES,
    // Defect powers and triggered cards (appended: saves store the numeric id)
    MACHINE_LEARNING,
    STORM,
    FERAL,
    SUBROUTINE,
    // Regent powers and triggered cards (appended: saves store the numeric id)
    CHILD_OF_THE_STARS,
    GENESIS,
    THE_SEALED_THRONE,
    BLACK_HOLE,
    MAKE_IT_SO,
    PALE_BLUE_DOT,
    ARSENAL,
    MONOLOGUE,
    ORBIT,
    PILLAR_OF_CREATION,
    MONARCHS_GAZE,
    // Necrobinder powers and triggered cards (appended: saves store the numeric id)
    FRIENDSHIP,
    DANSE_MACABRE,
    DEMESNE,
    SPIRIT_OF_ASH,
    PAGESTORM,
    SLEIGHT_OF_FLESH,
    // Defect orbs, Focus and Doom (appended: saves store the numeric id)
    SHATTER,
    VOLTAIC,
    QUADCAST,
    BARRAGE,
    TESLA_COIL,
    CHAOS,
    COMPILE_DRIVER,
    DARKNESS,
    MULTI_CAST,
    LOOP,
    CONSUMING_SHADOW,
    HAILSTORM,
    THUNDER,
    COOLANT,
    BIASED_COGNITION,
    HOTFIX,
    DEFRAGMENT,
    FOCUSED_STRIKE,
    HYPERBEAM,
    // Necrobinder orbs, Focus and Doom (appended: saves store the numeric id)
    NEGATIVE_PULSE,
    END_OF_DAYS,
    BLIGHT_STRIKE,
    NO_ESCAPE,
    SCOURGE,
    DEATHBRINGER,
    DEATHS_DOOR,
    COUNTDOWN,
    REAPER_FORM,
    NEUROSURGE,
    TIMES_UP,
    OBLIVION,
    SHROUD,
    // Defect orb slots and Synchronize (appended: saves store the numeric id)
    CAPACITOR,
    BULK_UP,
    SYNCHRONIZE,
    // Ironclad replays, cost changes and evolving cards (appended: saves store the numeric id)
    HOWL_FROM_BEYOND,
    HELLRAISER,
    STAMPEDE,
    CASCADE,
    ONE_TWO_PUNCH,
    TEAR_ASUNDER,
    HAVOC,
    CORRUPTION,
    UNRELENTING,
    MIDNIGHT,
    STOMP,
    RAMPAGE,
    // Silent replays, cost changes and evolving cards (appended: saves store the numeric id)
    BURST,
    BULLET_TIME,
    PINPOINT,
    POUNCE,
    // Defect replays, cost changes and evolving cards (appended: saves store the numeric id)
    ECHO_FORM,
    SIGNAL_BOOST,
    UPROAR,
    ADAPTIVE_STRIKE,
    SYNTHESIS,
    GENETIC_ALGORITHM,
    MOMENTUM_STRIKE,
    CLAW,
    MODDED,
    // Regent replays, cost changes and evolving cards (appended: saves store the numeric id)
    BOMBARDMENT,
    I_AM_INVINCIBLE,
    VOID_FORM,
    KINGLY_KICK,
    KINGLY_PUNCH,
    // Necrobinder replays, cost changes and evolving cards (appended: saves store the numeric id)
    EIDOLON,
    BANSHEES_CRY,
    VEILPIERCER,
    BORROWED_TIME,
    THE_SCYTHE,
    MELANCHOLY,
    HANG,
    // Status cards and the token made from them (appended: saves store the numeric id)
    BURN,
    DAZED,
    DEBRIS,
    SLIMED,
    SOOT,
    VOID_CARD,
    WOUND,
    FUEL,
    // Defect Status cards (appended: saves store the numeric id)
    BOOST_AWAY,
    FLAK_CANNON,
    SMOKESTACK,
    COMPACT,
    GUNK_UP,
    ITERATION,
    FIGHT_THROUGH,
    OVERCLOCK,
    ROCKET_PUNCH,
    TRASH_TO_TREASURE,
    TURBO,
    // Regent Status cards (appended: saves store the numeric id)
    COLLISION_COURSE,
    CRASH_LANDING,
    // Shiv: the Silent's token (appended: saves store the numeric id)
    SHIV,
    // Silent Shivs (appended: saves store the numeric id)
    BLADE_DANCE,
    CLOAK_AND_DAGGER,
    LEADING_STRIKE,
    BLADE_OF_INK,
    INFINITE_BLADES,
    STORM_OF_STEEL,
    FAN_OF_KNIVES,
    PHANTOM_BLADES,
    ACCURACY,
    UP_MY_SLEEVE,
    KNIFE_TRAP,
    // Ironclad hand choices (appended: saves store the numeric id)
    ARMAMENTS,
    BRAND,
    BURNING_PACT,
    // Silent hand choices (appended: saves store the numeric id)
    ACROBATICS,
    DAGGER_THROW,
    PREPARED,
    HIDDEN_DAGGERS,
    // Defect hand choices (appended: saves store the numeric id)
    SCAVENGE,
    // Regent hand choices (appended: saves store the numeric id)
    PHOTON_CUT,
    GLIMMER,
    TYRANNY,
    // Necrobinder hand choices (appended: saves store the numeric id)
    SCULPTING_STRIKE,
    // Ironclad cards on the card-played event (appended: saves store the numeric id)
    JUGGLING,
    // Silent cards on existing mechanics, second batch (appended: saves store the numeric id)
    EXPOSE,
    // Ironclad pile choices (appended: saves store the numeric id)
    HEADBUTT,
    // Defect pile choices (appended: saves store the numeric id)
    HOLOGRAM,
    // Regent pile choices (appended: saves store the numeric id)
    COSMIC_INDIFFERENCE,
    FOREGONE_CONCLUSION,
    // Necrobinder pile choices (appended: saves store the numeric id)
    GRAVEBLAST,
    DREDGE,
    CLEANSE,
    // Plating, Thorns, Intangible, Buffer and Vigor, by hero (appended: saves store the numeric id)
    STONE_ARMOR,
    FLAME_BARRIER,
    WRAITH_FORM,
    BUFFER,
    NEUTRON_AEGIS,
    REFLECT,
    PATTER,
    TERRAFORMING,
    // Forge and the Sovereign Blade (Regent) (appended: saves store the numeric id)
    SOVEREIGN_BLADE,
    REFINE_BLADE,
    SPOILS_OF_BATTLE,
    WROUGHT_IN_WAR,
    BULWARK,
    CONQUEROR,
    FURNACE,
    PARRY,
    SUMMON_FORTH,
    BEAT_INTO_SHAPE,
    BIG_BANG,
    SEEKING_EDGE,
    SWORD_SAGE,
    THE_SMITH,
    // Osty's cards (Necrobinder) (appended: saves store the numeric id)
    POKE,
    SNAP,
    RATTLE,
    BONE_SHARDS,
    SQUEEZE,
    FLATTEN,
    HIGH_FIVE,
    FETCH,
    CALCIFY,
    RIGHT_HAND_HAND,
    NECRO_MASTERY,
    PROTECTOR,
    SACRIFICE,
    SPUR,
    SIC_EM,
    SENTRY_MODE,
    SWEEPING_GAZE,
    // Soul (Necrobinder): a token, made by other cards (appended: saves store the numeric id)
    SOUL,
    // Souls cards (Necrobinder) (appended: saves store the numeric id)
    REAVE,
    GRAVE_WARDEN,
    CAPTURE_SPIRIT,
    SEVERANCE,
    DIRGE,
    HAUNT,
    DEVOUR_LIFE,
    SOUL_STORM,
    GLIMPSE_BEYOND,
    MANIFEST_AUTHORITY,
    // Giant Rock: a token made by other cards (appended: saves store the numeric id)
    GIANT_ROCK,
    // Luminesce: a token made by other cards
    LUMINESCE,
    // Apparition: an event card
    APPARITION,
    // Maul: an event card; every copy grows together during the combat
    MAUL,
    // Apotheosis: an event card
    APOTHEOSIS,
    // Wish: an event card
    WISH,
    // Whistle: an event card that stuns the enemy
    WHISTLE,
    // Brightest Flame: an event card; costs 2 Max HP for the rest of the run
    BRIGHTEST_FLAME,
    // Folly (Hubris): a Curse, Eternal
    FOLLY,
    // Enthralled (Assuétude): a Curse, Eternal; must be played before the other cards in hand
    ENTHRALLED,
    // Flash of Steel: a colorless 0-cost attack that draws a card
    FLASH_OF_STEEL,
    // Peck: an event card that hits several times
    PECK,
    // Production: a colorless 0-cost skill that gives energy
    PRODUCTION,
    // Master of Strategy: a colorless 0-cost skill that draws cards
    MASTER_OF_STRATEGY,
    // Byrd Swoop: an event card
    BYRD_SWOOP,
    // Ultimate Strike: a colorless Attack
    ULTIMATE_STRIKE,
    // Ultimate Defend: a colorless Skill
    ULTIMATE_DEFEND,
    // Finesse: a colorless Skill
    FINESSE,
    // Prowess: a colorless Power
    PROWESS,
    // Squash: an event card
    SQUASH,
    // Exterminate: an event card
    EXTERMINATE,
    // Feeding Frenzy: an event skill that gives temporary Strength
    FEEDING_FRENZY,
    // Outmaneuver: an event skill that gives energy next turn
    OUTMANEUVER,
    // Toric Toughness: an event skill that gives block now and at the start of the next 2 turns
    TORIC_TOUGHNESS,
    // Entrench: an event skill that doubles the block
    ENTRENCH,
    // Stack: an event skill that gives block equal to the discard pile size
    STACK,
    // Rip and Tear: an event attack that hits random enemies twice
    RIP_AND_TEAR,
    // Scrawl: a colorless rare skill that draws until the hand is full
    SCRAWL,
    // Mind Blast: a colorless innate attack that deals damage equal to the draw pile size
    MIND_BLAST,
    // Batch F: colorless and event cards built on existing mechanics
    SHOCKWAVE,
    BOLAS,
    THRUMMING_HATCHET,
    IMPATIENCE,
    CLASH,
    VOLLEY,
    OMNISLICE,
    RESTLESSNESS,
    // Batch G: colorless and event cards, some with a new power or counter
    ETERNAL_ARMOR,
    FASTEN,
    REBOUND,
    PANIC_BUTTON,
    GOLD_AXE,
    ANOINTED,
    FISTICUFFS,
    REND,
    THE_BOMB,
    ROLLING_BOULDER,
    PANACHE,
    JACKPOT,
    // Batch H: cards that play cards, move cards between piles or hold the hand
    BEAT_DOWN,
    CATASTROPHE,
    NOSTALGIA,
    RELAX,
    NEOWS_FURY,
    METAMORPHOSIS,
    DISTRACTION,
    CALTROPS,
    PROLONG,
    SALVO,
    THINKING_AHEAD,
    JACK_OF_ALL_TRADES,
    // Batch I: Powers with turn and event triggers, and cards that pick from the draw pile
    HELLO_WORLD,
    DARK_SHACKLES,
    EQUILIBRIUM,
    PURITY,
    SECRET_TECHNIQUE,
    DRAMATIC_ENTRANCE,
    SECRET_WEAPON,
    MAYHEM,
    PREP_TIME,
    STRATAGEM,
    CALAMITY,
    AUTOMATION,
    // Batch J: cards that copy, replay or create cards, and Powers that add cards each turn
    ENLIGHTENMENT,
    DUAL_WIELD,
    CALL_OF_THE_VOID,
    HEIRLOOM_HAMMER,
    BUNDLE_OF_JOY,
    SPECTRUM_SHIFT,
    DECISIONS_DECISIONS,
    WHITE_NOISE,
    CREATIVE_AI,
    NIGHTMARE,
    TOOLS_OF_THE_TRADE,
    INFERNAL_BLADE,
    STOKE,
    // Curses on existing mechanics (Unplayable, Eternal, end-of-turn effects in hand)
    INJURY,
    GREED,
    CLUMSY,
    ASCENDERS_BANE,
    CURSE_OF_THE_BELL,
    POOR_SLEEP,
    WRITHE,
    NORMALITY,
    DOUBT,
    SHAME,
    DECAY,
    REGRET,
    BAD_LUCK,
    DEBT,
    SPORE_MIND,
    GUILTY,
    // Sly (Silent): played for free when discarded from the hand during the turn
    TACTICIAN,
    ABRASIVE,
    MASTER_PLANNER,
    HAND_TRICK,
    UNTOUCHABLE,
    RICOCHET,
    REFLEX,
    FLICK_FLACK,
    SHADOWMELD,
    UNMOVABLE,
    COLOSSUS,
    CRUELTY,
    TRACKING,
    LETHALITY,
    SHADOW_STEP,
    DEBILITATE,
    HAND_OF_GREED,
    THE_HUNT,
    ROYALTIES,
    FORBIDDEN_GRIMOIRE,
    ALCHEMIZE,
    THE_GAMBIT,
    // Choose 1 card among N offered (mechanic #72)
    SEEKER_STRIKE,
    // Transform a card (mechanic #79)
    PRIMAL_FORCE,
    SEANCE,
    ENTROPY,
    SPLASH,
    // Event card: a single attack (Mad Science)
    MAD_SCIENCE,
    // Regent: choose 1 of 3 random Colorless cards (Quasar)
    QUASAR,
    // Choose 1 of 3 random cards, free this turn (mechanic #72)
    DISCOVERY,
    // Event card: choose 1 of 3 upgraded Powers (Abundance)
    ABUNDANCE,
    // Regent minions (mechanic #103): tokens, and the cards that transform into them
    MINION_STRIKE,
    MINION_DIVE_BOMB,
    MINION_SACRIFICE,
    BEGONE,
    CHARGE,
    GUARDS,
    // Echo N on a copy of a card (mechanic #86)
    HIDDEN_GEM,
    TRANSFIGURE,
    COUNT
};

// Where a card created during a fight goes.
// The cards whose French name contains "Sbire" (Vitruvian Minion).
constexpr bool is_minion_card(CardId id) {
    return id == CardId::MINION_STRIKE || id == CardId::MINION_DIVE_BOMB || id == CardId::MINION_SACRIFICE;
}

enum class Pile : uint8_t {
    HAND,
    DRAW, // at a random place
    DISCARD,
    OFFERS // cards offered by a choice (CombatEngine::offers), only picked from: "Choose 1 of 3 cards"
};

constexpr uint8_t NOT_IN_DECK = 0xFF;

// What an enchantment adds to one copy of a card.
enum class Enchantment : uint8_t {
    NONE,
    INKY // applies INKY_WEAK Weak to the enemies the card hits
};

constexpr uint8_t INKY_WEAK = 1;

struct DeckCard {
    CardId id = CardId::NONE;
    bool upgraded = false;
    uint8_t bonus = 0; // added to this copy's base value: gained during the combat (Thrash, Rampage) or kept by the deck
    uint8_t added_keywords = 0; // CardKeyword bits (the low eight) given to this copy during the combat, on top of its CardDef
    int8_t cost_delta = 0;      // energy cost change of this copy for the combat (Momentum Strike, Modded)
    uint8_t bookmark_discount = 0; // energy off this copy until it is played (Bookmark)
    bool free_this_turn = false; // this copy costs 0 until the end of the turn (Bullet Time)
    bool cost_one_this_turn = false; // this copy costs at most 1 until the end of the turn (Enlightenment)
    uint8_t echo = 0;           // Echo N: the card is replayed N more times each time it is played (Hidden Gem, Transfigure)
    bool sly_combat = false;    // this copy has Sly for the rest of the combat (Master Planner)
    bool sly_this_turn = false; // this copy has Sly until the end of the turn (Hand Trick)
    uint8_t deck_index = NOT_IN_DECK; // position of the card in the run's deck, for the gains that outlive the combat
    Enchantment enchantment = Enchantment::NONE;

    constexpr DeckCard() = default;
    constexpr DeckCard(CardId i, bool up = false) : id(i), upgraded(up) {}
    constexpr operator CardId() const { return id; }
    constexpr bool operator==(CardId other) const { return id == other; }
    constexpr bool operator==(const DeckCard& other) const { return id == other.id && upgraded == other.upgraded; }
};

// Card keywords, combined as bits in CardDef::keywords.
enum CardKeyword : uint16_t {
    KEYWORD_EXHAUST = 1,  // goes to the exhaust pile when played
    KEYWORD_RETAIN = 2,   // stays in hand at the end of the turn
    KEYWORD_ETHEREAL = 4, // exhausted when still in hand at the end of the turn
    KEYWORD_INNATE = 8,   // drawn in the opening hand
    KEYWORD_RETURN = 16,  // once played, comes back to the hand at the start of the next turn
    KEYWORD_RETAIN_ONCE = 32, // per-copy only: stays in hand at the end of this turn, then the mark is removed
    KEYWORD_PLAY_FIRST = 64,  // while in hand, it must be played before any other card
    KEYWORD_LIMIT_PLAYS = 128, // while in hand, no more than MAX_PLAYS_WHEN_LIMITED cards can be played each turn
    KEYWORD_UNPLAYABLE = 256, // cannot be played from the hand; played by another card, it does nothing
    KEYWORD_ETERNAL = 512,    // can never be removed from the deck nor transformed (shop, events, relics)
    KEYWORD_SLY = 1024        // discarded from the hand during the turn: played for free (Silent)
};

constexpr uint8_t MAX_PLAYS_WHEN_LIMITED = 3;

// What a card needs, besides its cost, to be played.
enum class PlayCondition : uint8_t {
    NONE,
    DRAW_PILE_EMPTY,
    HAND_ALL_ATTACKS, // every card in hand, itself included, is an Attack
    OSTY_ALIVE        // "Osty deals N damage": he cannot attack while he is dead
};

// CardDef::cost of an X card: it spends all the remaining energy and X is the amount spent.
constexpr uint8_t COST_X = 0xFF;

enum class CardType : uint8_t {
    ATTACK,
    SKILL,
    POWER,
    STATUS, // added to the piles during a combat, never part of the deck
    CURSE,  // a deck card that hinders the player, usually Unplayable
    QUEST   // a deck card tied to a goal of the run, usually Unplayable
};

enum class TargetType : uint8_t {
    SINGLE_ENEMY,
    SELF,
    ALL_ENEMIES,
    RANDOM_ENEMY // picked by the engine, no targeting
};

struct CardDef {
    CardId id;
    std::string_view name;
    uint8_t cost;
    CardType type;
    TargetType target;
    uint8_t base_value; // Damage or Block
    uint8_t secondary_value; // e.g. Vulnerable turns
    std::string_view description;

    // Upgraded stats
    std::string_view upgraded_name;
    uint8_t upgraded_cost;
    uint8_t upgraded_base_value;
    uint8_t upgraded_secondary_value;
    std::string_view upgraded_description;

    // CardKeyword bits of the base and of the upgraded card: an upgrade can add or remove a keyword.
    uint16_t keywords = 0;
    uint16_t upgraded_keywords = 0;
    // Stars spent to play the card, on top of its energy cost (Regent). COST_X: all the Stars left.
    uint8_t star_cost = 0;
    uint8_t upgraded_star_cost = 0;
    PlayCondition condition = PlayCondition::NONE;

    static const CardDef& get(CardId id);
    static std::string_view get_display_name(CardId id, bool upgraded) {
        const auto& c = get(id);
        return upgraded ? c.upgraded_name : c.name;
    }
    static uint8_t get_cost(CardId id, bool upgraded) {
        const auto& c = get(id);
        return upgraded ? c.upgraded_cost : c.cost;
    }
    static uint8_t get_value(CardId id, bool upgraded) {
        const auto& c = get(id);
        return upgraded ? c.upgraded_base_value : c.base_value;
    }
    static uint8_t get_secondary(CardId id, bool upgraded) {
        const auto& c = get(id);
        return upgraded ? c.upgraded_secondary_value : c.secondary_value;
    }
    static bool has_keyword(CardId id, bool upgraded, CardKeyword keyword) {
        const auto& c = get(id);
        return ((upgraded ? c.upgraded_keywords : c.keywords) & keyword) != 0;
    }
    // Keyword of one copy: from its definition or given to that copy during the combat.
    static bool has_keyword(const DeckCard& card, CardKeyword keyword) {
        return (card.added_keywords & keyword) != 0 || has_keyword(card.id, card.upgraded, keyword);
    }
    // Sly: from its definition, or given to that copy during the combat.
    static bool is_sly(const DeckCard& card) { return card.sly_combat || card.sly_this_turn || has_keyword(card, KEYWORD_SLY); }
    // An Eternal card stays in the deck: it is never removed nor transformed.
    static bool is_eternal(const DeckCard& card) { return has_keyword(card.id, card.upgraded, KEYWORD_ETERNAL); }
    static bool is_x_cost(CardId id, bool upgraded) {
        return get_cost(id, upgraded) == COST_X;
    }
    static uint8_t get_star_cost(CardId id, bool upgraded) {
        const auto& c = get(id);
        return upgraded ? c.upgraded_star_cost : c.star_cost;
    }
    static std::string_view get_desc(CardId id, bool upgraded) {
        const auto& c = get(id);
        return upgraded ? c.upgraded_description : c.description;
    }
};

// Rarity and pool of a card in the reference (bible/data), for the effects that draw a card at random.
enum class CardRarity : uint8_t {
    NONE, // not a card of the reference: Status cards, tokens (Shiv, Soul, Sovereign Blade)
    BASIC,
    COMMON,
    UNCOMMON,
    RARE,
    ANCIENT,
    EVENT,
    CURSE,
    QUEST
};

enum class CardPool : uint8_t {
    NONE,
    IRONCLAD,
    SILENT,
    DEFECT,
    REGENT,
    NECROBINDER,
    COLORLESS,
    EVENT,
    CURSE,
    QUEST
};

struct CardMeta {
    CardRarity rarity = CardRarity::NONE;
    CardPool pool = CardPool::NONE;
};

// Generated table (include/engine/card_meta.h, by scripts/build_card_meta.py): one entry per CardId.
const CardMeta& card_meta(CardId id);

} // namespace engine
