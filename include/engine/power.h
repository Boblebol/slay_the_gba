#pragma once

#include <cstdint>
#include "engine/card.h"
#include "engine/status.h"

namespace engine {

// Lasting effects on the player: Power cards and "this turn" effects of other cards.
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
    SLEIGHT_OF_FLESH    // debuff applied to an enemy: it takes `amount` damage
};

constexpr uint8_t PALE_BLUE_DOT_CARDS = 5;
constexpr uint8_t ORBIT_ENERGY = 4;
constexpr uint8_t DANSE_MACABRE_COST = 2;

struct ActivePower {
    PowerId id = PowerId::NONE;
    int16_t amount = 0;     // adds up when the same power is gained again
    uint8_t stacks = 0;     // number of times it was gained
    uint8_t counter = 0;    // the power's own tally: uses this turn, energy spent so far...
    uint8_t turns = 0;      // player turns started since it was gained
    int8_t target = -1;     // enemy it is bound to, -1 for none
    bool turn_only = false; // removed at the end of the player's turn
};

// What the powers can react to. Each event is raised from a single place in CombatEngine.
enum class CombatEvent : uint8_t {
    TURN_START,             // player's turn, after Block and energy are reset and before the draw
    TURN_END,               // player's turn, before the hand is discarded
    CARD_PLAYED,            // costs paid, before the card's effect; `card`, `cost` (energy paid)
    CARD_DRAWN,             // `card`, `opening_draw`
    CARD_EXHAUSTED,         // `card`
    CARD_CREATED,           // `card`
    BLOCK_GAINED,           // `amount`
    HP_LOST,                // by the player; `amount`
    ATTACK_HIT,             // one attack hit on an enemy; `enemy`
    UNBLOCKED_DAMAGE_DEALT, // that hit went through the enemy's Block; `enemy`, `amount`
    DEBUFF_APPLIED,         // to an enemy; `enemy`, `status`, `amount`
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
