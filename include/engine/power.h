#pragma once

#include <cstdint>
#include "engine/card.h"
#include "engine/status.h"

namespace engine {

// Lasting effects on the player: Power cards, "this turn" effects of other cards, and the relics that act
// during a fight. The powers of the enemies are in the same registry: the ones a monster holds itself
// (ActivePower::holder), and the ones its moves put on the player.
// One id per behaviour: two cards with the same effect share it (Pyre and Friendship both give TURN_ENERGY).
enum class PowerId : uint8_t {
    NONE = 0,
    // Start of the player's turn
    TURN_ENERGY,     // gain `amount` energy
    TURN_DRAW,       // draw `amount` more cards
    TURN_STRENGTH,   // gain `amount` Strength
    TURN_STARS,      // gain `amount` Stars
    TURN_POISON_ALL, // apply `amount` Poison to every enemy
    CRIMSON_MANTLE,  // lose 1 HP per stack, gain `amount` Block
    INFERNO,         // lose 1 HP per stack; losing HP during the turn deals `amount` damage to every enemy
    AGGRESSION,      // a random Attack of the discard pile comes back to the hand, upgraded
    // Rules read by the engine
    BARRICADE,       // Block is kept from one turn to the next
    KEEP_HAND,       // the hand is not discarded at the end of the turn
    ACCELERANT,      // enemy Poison triggers `amount` more times
    FERAL,           // the first `amount` 0-cost Attacks of each turn return to the hand
    // Reactions to what happens during the combat
    FEEL_NO_PAIN,       // card exhausted: `amount` Block
    DARK_EMBRACE,       // card exhausted: draw `amount`
    RUPTURE,            // HP lost during the turn: `amount` Strength
    JUGGERNAUT,         // Block gained: `amount` damage to a random enemy
    RAGE,               // Attack played: `amount` Block
    VICIOUS,            // Vulnerable applied: draw `amount`
    SPEEDSTER,          // card drawn during the turn: `amount` damage to every enemy
    ENVENOM,            // unblocked attack damage: `amount` Poison
    SERPENT_FORM,       // card played: `amount` damage to a random enemy
    AFTERIMAGE,         // card played: `amount` Block
    STRANGLE,           // card played: the enemy it is bound to loses `amount` HP
    CORROSIVE_WAVE,     // card drawn: `amount` Poison to every enemy
    STORM,              // Power played: channel `amount` Lightning
    SUBROUTINE,         // Power played: `amount` energy
    CHILD_OF_THE_STARS, // Stars spent: `amount` Block for each
    SEALED_THRONE,      // card played: `amount` Stars
    BLACK_HOLE,         // Stars gained or spent: `amount` damage to every enemy
    PALE_BLUE_DOT,      // end of a turn with PALE_BLUE_DOT_CARDS cards played: draw `amount` next turn
    ARSENAL,            // card created: `amount` Strength
    MONOLOGUE,          // card played: `amount` Strength until the end of the turn
    ORBIT,              // every ORBIT_ENERGY energy spent: `amount` energy
    PILLAR_OF_CREATION, // card created: `amount` Block
    MONARCHS_GAZE,      // attack hit: the enemy loses `amount` Strength until the end of its turn
    DANSE_MACABRE,      // card costing DANSE_MACABRE_COST energy or more played: `amount` Block
    SPIRIT_OF_ASH,      // Ethereal card played: `amount` Block
    PAGESTORM,          // Ethereal card drawn: draw `amount`
    SLEIGHT_OF_FLESH,   // debuff applied to an enemy: it takes `amount` damage
    // Orbs and Focus (Defect)
    LOOP,               // start of turn: the oldest orb's passive triggers `amount` times
    COOLANT,            // start of turn: `amount` Block for each different orb type
    BIASED_COGNITION,   // start of turn: lose 1 Focus per stack
    CONSUMING_SHADOW,   // end of turn: evoke the newest orb, once per stack
    HAILSTORM,          // end of turn with a Frost orb: `amount` damage to every enemy
    THUNDER,            // Lightning orb evoked: `amount` more damage to the enemy it hits
    // Cards played again or for free
    REPLAY_ATTACK,      // the next `amount` Attacks are played once more
    REPLAY_SKILL,       // the next `amount` Skills are played once more
    REPLAY_POWER,       // the next `amount` Powers are played once more
    ECHO_FORM,          // the first `amount` cards of each turn are played once more
    FREE_ATTACK,        // the next `amount` Attacks cost 0
    FREE_SKILL,         // the next `amount` Skills cost 0
    FREE_POWER,         // the next `amount` Powers cost 0
    FREE_ETHEREAL,      // the next `amount` Ethereal cards cost 0
    VOID_FORM,          // the first `amount` cards of each turn cost 0
    CORRUPTION,         // Skills cost 0 and are exhausted when played
    SURCHARGE,          // cards cost `amount` more
    HELLRAISER,         // a "Strike" card drawn is played at once on a random enemy
    STAMPEDE,           // end of turn: a random Attack of the hand is played on a random enemy, once per stack
    // Shivs (Silent)
    ACCURACY,           // Shivs deal `amount` more damage
    PHANTOM_BLADES,     // Shivs are kept in hand; the first one played each turn deals `amount` more damage
    FAN_OF_KNIVES,      // Shivs hit every enemy
    INFINITE_BLADES,    // start of turn: `amount` Shivs in hand
    TYRANNY,            // start of turn: draw `amount` more, then exhaust `amount` cards of the hand, picked by the player
    // Status cards
    SMOKESTACK,         // Status card created: `amount` damage to every enemy
    TRASH_TO_TREASURE,  // Status card created: channel a random orb, once per stack
    ITERATION,          // first Status card drawn each turn: draw `amount`
    // Doom (Necrobinder)
    COUNTDOWN,          // start of turn: `amount` Doom to a random enemy
    NEUROSURGE,         // start of turn: `amount` Doom to the player
    REAPER_FORM,        // unblocked attack damage: as much Doom
    OBLIVION,           // card played: `amount` Doom to the enemy it is bound to
    SHROUD,             // Doom applied: `amount` Block
    // Souls (Necrobinder)
    HAUNT,              // Soul played: a random enemy loses `amount` HP
    DEVOUR_LIFE,        // Soul played: summon Osty for `amount`
    // Attacks played
    JUGGLING,           // the JUGGLING_ATTACK-th Attack played each turn: a copy of it comes to the hand
    // Relics, registered by RunManager when the fight starts. `turns` is then the number of the turn.
    CAPTAINS_WHEEL,    // start of turn 3: `amount` Block
    HORN_CLEAT,        // start of turn 2: `amount` Block
    CANDELABRA,        // start of turn 2: `amount` energy
    CHANDELIER,        // start of turn 3: `amount` energy
    SPARKLING_ROUGE,   // start of turn 3: `amount` Strength and Dexterity
    HAPPY_FLOWER,      // every 3 turns: `amount` energy
    PENDULUM,          // every 3 turns: draw `amount` more
    VERY_HOT_COCOA,    // turn 1: `amount` more energy
    BREAD,             // turn 1: lose BREAD_ENERGY_LOST energy; later turns: `amount` energy
    FIDDLE,            // marker: no card is drawn after the opening draw of the turn (the bonus is a TURN_DRAW)
    MERCURY_HOURGLASS, // start of turn: `amount` damage to every enemy
    CLOAK_CLASP,       // end of turn: `amount` Block for each card in hand
    POCKETWATCH,       // end of a turn with POCKETWATCH_CARDS cards played or fewer: draw `amount` more next turn
    PARRYING_SHIELD,   // end of turn with PARRYING_SHIELD_BLOCK Block or more: `amount` damage to a random enemy
    STONE_CALENDAR,    // end of turn 7: `amount` damage to every enemy
    ART_OF_WAR,        // end of a turn without an Attack: `amount` energy next turn
    ORICHALCUM,        // end of turn without Block: `amount` Block
    SCREAMING_FLAGON,  // end of turn with an empty hand: `amount` damage to every enemy
    ICE_CREAM,         // unspent energy is kept
    STURDY_CLAMP,      // up to `amount` Block is kept
    GREMLIN_HORN,      // enemy killed: `amount` energy and 1 card
    RAZOR_TOOTH,       // an Attack or a Skill played is upgraded for the rest of the combat
    THE_ABACUS,        // draw pile shuffled: `amount` Block
    PERMAFROST,        // first Power played: `amount` Block
    GAME_PIECE,        // Power played: draw `amount`
    CENTENNIAL_PUZZLE, // first HP loss: draw `amount`
    LIZARD_TAIL,       // HP reduced to 0: back to `amount` % of Max HP, once (counter = 1 when spent)
    UNCEASING_TOP,     // hand empty during the turn: draw 1
    TOUGH_BANDAGES,    // card discarded during the turn: `amount` Block
    TINGSHA,           // card discarded during the turn: `amount` damage to a random enemy
    TURN_SUMMON,       // start of turn: summon Osty for `amount` (Bound Phylactery)
    BONE_FLUTE,        // Osty attacks: `amount` Block
    CALCIFY,           // Osty's attacks deal `amount` more damage
    NECRO_MASTERY,     // Osty loses HP: every enemy loses as much
    GOLD_PLATED_CABLES, // the oldest orb's passive triggers once more
    METRONOME,         // METRONOME_ORBS orbs channeled in the combat: `amount` damage to every enemy, once
    EMOTION_CHIP,      // start of a turn that follows a HP loss: every orb's passive triggers
    BOOK_REPAIR_KNIFE, // enemy killed by Doom (minions excepted): heal `amount`
    HELICAL_DART,      // Shiv played: `amount` Dexterity until the end of the turn
    JOSS_PAPER,        // every JOSS_PAPER_CARDS cards exhausted: draw `amount`
    LETTER_OPENER,     // every LETTER_OPENER_SKILLS Skills played in a turn: `amount` damage to every enemy
    TUNING_FORK,       // every TUNING_FORK_SKILLS Skills played: `amount` Block
    KUNAI,             // every KUNAI_ATTACKS Attacks played in a turn: `amount` Dexterity
    KUSARIGAMA,        // every KUSARIGAMA_ATTACKS Attacks played in a turn: `amount` damage to a random enemy
    MUMMIFIED_HAND,    // Power played: a random card of the hand costs 0 until the end of the turn
    NUNCHAKU,          // every NUNCHAKU_ATTACKS Attacks played: `amount` energy
    RAINBOW_RING,      // an Attack, a Skill and a Power played in the turn: `amount` Strength and Dexterity, once a turn
    RIPPLE_BASIN,      // end of a turn without an Attack: `amount` Block
    BURNING_STICKS,    // first Skill exhausted: a copy of it comes to the hand
    SHURIKEN,          // every SHURIKEN_ATTACKS Attacks played in a turn: `amount` Strength
    ORNAMENTAL_FAN,    // every ORNAMENTAL_FAN_ATTACKS Attacks played in a turn: `amount` Block
    SELF_FORMING_CLAY, // HP lost: `amount` Block at the start of the next turn
    CHARONS_ASHES,     // card exhausted: `amount` damage to every enemy
    RED_SKULL,         // HP at RED_SKULL_HP_PERCENT % of Max HP or less: `amount` more Strength (counter = Strength given now)
    DEMON_TONGUE,      // first HP loss of each turn: heal as much (counter = turn of the last heal)
    BRIMSTONE,         // start of turn: `amount` Strength, and 1 Strength per stack to every enemy
    SNECKO_SKULL,      // Poison applied: `amount` more
    MINI_REGENT,       // first Stars spent each turn: `amount` Strength
    GALACTIC_DUST,     // every GALACTIC_DUST_STARS Stars spent: `amount` Block
    LUNAR_PASTRY,      // end of turn: `amount` Stars
    REGALITE,          // first card created each turn: `amount` Block
    IVORY_TILE,        // card costing IVORY_TILE_COST energy or more played: `amount` energy
    SEAL_OF_GOLD,      // start of turn: spend 3 gold (CombatEngine::gold) to gain `amount` energy
    SAI,               // start of turn: `amount` Block
    PAELS_LEGION,      // Block from a card is doubled, then the relic sleeps `amount` turns (counter = turn starts left asleep + 1)
    // Damage sent back and statuses lost each turn (appended)
    FLAME_BARRIER,     // until the next turn: each enemy attack deals `amount` damage back to the attacker
    REFLECT,           // until the next turn: the attack damage the player's Block absorbs goes back to the attacker
    WRAITH_FORM,       // start of turn: lose `amount` Dexterity
    // Forge and the Sovereign Blade (Regent) (appended)
    FURNACE,           // start of turn: Forge `amount`
    PARRY,             // Sovereign Blade played: `amount` Block
    SEEKING_EDGE,      // Sovereign Blade hits every enemy
    SWORD_SAGE,        // Sovereign Blade is played `amount` more times
    CONQUEROR,         // this turn: Sovereign Blade deals double damage to the enemy it is bound to
    // Osty's cards (Necrobinder) (appended)
    SIC_EM,            // this turn: Osty hits the enemy it is bound to: summon Osty for `amount`
    SENTRY_MODE,       // start of turn: `amount` Sweeping Gaze to the hand
    // Potions (appended)
    DEMISE,            // end of enemy turn: the enemy it is bound to loses `amount` HP
    DUPLICATION,       // this turn: the next `amount` cards are played once more
    // Powers of the enemies (appended). On the player, given by an enemy move:
    TANGLED,           // this turn: Attacks cost `amount` more energy
    TENDER,            // card played: lose `amount` Strength and Dexterity until the end of the turn
    // Held by an enemy:
    PERSONAL_HIVE,     // it takes unblocked attack damage: `amount` Dazed into the player's draw pile
    // Relics of the Ancients (appended)
    PAELS_TEARS,       // end of a turn with unspent energy: `amount` energy next turn
    // Colorless and event cards (appended)
    FASTEN,            // Defend cards give `amount` more Block
    ROLLING_BOULDER,   // start of turn: `amount` damage to every enemy, then 5 more (counter = the growth so far)
    PANACHE,           // every PANACHE_CARDS cards played in a turn: `amount` damage to every enemy
    NOSTALGIA,         // the first Attack or Skill played each turn goes on top of the draw pile (counter = used this turn)
    HELLO_WORLD,       // start of turn: `amount` random Common cards to the hand
    MAYHEM,            // start of turn: play the top card of the draw pile, `amount` times
    PREP_TIME,         // start of turn: `amount` Vigor
    STRATAGEM,         // draw pile shuffled: pick `amount` cards of it for the hand
    CALAMITY,          // Attack played: `amount` random Attacks to the hand
    AUTOMATION         // every AUTOMATION_CARDS cards drawn: `amount` energy (counter = cards drawn so far)
};

// Whether the power is a harm an enemy move puts on the player: Artifact cancels it.
constexpr bool is_player_debuff_power(PowerId id) {
    return id == PowerId::TANGLED || id == PowerId::TENDER;
}

constexpr uint8_t PALE_BLUE_DOT_CARDS = 5;
constexpr uint8_t PANACHE_CARDS = 5;
constexpr uint8_t AUTOMATION_CARDS = 10;
constexpr uint8_t ROLLING_BOULDER_GROWTH = 5;
constexpr uint8_t ORBIT_ENERGY = 4;
constexpr uint8_t DANSE_MACABRE_COST = 2;
constexpr uint8_t BREAD_ENERGY_LOST = 2;
constexpr uint8_t POCKETWATCH_CARDS = 3;
constexpr uint8_t PARRYING_SHIELD_BLOCK = 10;
constexpr uint8_t STONE_CALENDAR_TURN = 7;
constexpr uint8_t METRONOME_ORBS = 7;
constexpr uint8_t JUGGLING_ATTACK = 3;
constexpr uint8_t JOSS_PAPER_CARDS = 5;
constexpr uint8_t LETTER_OPENER_SKILLS = 3;
constexpr uint8_t TUNING_FORK_SKILLS = 10;
constexpr uint8_t KUNAI_ATTACKS = 3;
constexpr uint8_t KUSARIGAMA_ATTACKS = 3;
constexpr uint8_t NUNCHAKU_ATTACKS = 10;
constexpr uint8_t SHURIKEN_ATTACKS = 3;
constexpr uint8_t ORNAMENTAL_FAN_ATTACKS = 3;
constexpr uint8_t RED_SKULL_HP_PERCENT = 50;
constexpr uint8_t GALACTIC_DUST_STARS = 10;
constexpr uint8_t IVORY_TILE_COST = 3;

struct ActivePower {
    PowerId id = PowerId::NONE;
    int16_t amount = 0;     // adds up when the same power is gained again
    uint8_t stacks = 0;     // number of times it was gained
    uint8_t counter = 0;    // the power's own tally: uses this turn, energy spent so far...
    uint8_t turns = 0;      // player turns started since it was gained
    int8_t target = -1;     // enemy it is bound to, -1 for none
    // Enemy that holds the power, -1 when the player does. A power held by an enemy only hears the fight while
    // that enemy is alive, counts its holder's turns in `turns`, and is removed when it dies or leaves.
    int8_t holder = -1;
    bool turn_only = false; // removed at the end of the player's turn
};

// What the powers can react to. Each event is raised from a single place in CombatEngine.
enum class CombatEvent : uint8_t {
    ENEMY_TURN_START,       // an enemy is about to act; `enemy`
    TURN_START,             // player's turn, after Block and energy are reset and before the draw
    TURN_END,               // player's turn, before the hand is discarded
    CARD_PLAYED,            // costs paid, before the card's effect; `card`, `cost` (energy paid)
    CARD_DRAWN,             // `card`, `opening_draw`
    CARD_DISCARDED,         // by an effect, not by the end of the turn; `card`
    CARD_EXHAUSTED,         // `card`
    CARD_CREATED,           // `card`
    BLOCK_GAINED,           // `amount`
    HP_LOST,                // by the player; `amount`
    ATTACK_HIT,             // one attack hit on an enemy; `enemy`
    UNBLOCKED_DAMAGE_DEALT, // that hit went through the enemy's Block; `enemy`, `amount`
    DEBUFF_APPLIED,         // to an enemy; `enemy`, `status`, `amount`
    ENEMY_KILLED,           // whatever killed it; `enemy`
    OSTY_ATTACKED,          // once per attack of Osty, however many enemies it hits
    OSTY_HP_LOST,           // to an enemy's attack or because he was made to die; `amount`
    DRAW_PILE_SHUFFLED,
    STARS_GAINED,           // `amount`
    STARS_SPENT             // `amount`
};

struct EventInfo {
    int16_t amount = 0;
    int8_t enemy = -1; // index in CombatEngine::enemies
    DeckCard card;
    StatusType status = StatusType::NONE;
    uint8_t cost = 0;
    bool opening_draw = false; // drawn as part of the turn's opening hand
};

} // namespace engine
