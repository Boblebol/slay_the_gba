#pragma once

#include <cstdint>
#include <span>
#include <string_view>
#include "engine/card.h"
#include "engine/entity.h"
#include "engine/fixed_vector.h"
#include "engine/character.h"
#include "engine/power.h"

namespace engine {

enum class CombatState : uint8_t {
    PLAYER_TURN,
    ENEMY_TURN,
    VICTORY,
    DEFEAT
};

// What one enemy did during the last enemy turn, so the presentation layer can replay it step by step.
struct EnemyActionRecord {
    int8_t enemy_index = -1;
    EnemyIntentType intent_type = EnemyIntentType::ATTACK;
    int16_t damage_to_player = 0; // HP actually lost by the player
    int16_t damage_blocked = 0;   // Absorbed by the player's block or minion
    int16_t block_gained = 0;
    int16_t player_hp_after = 0;
    // Statuses the move gave the player (NONE for one Artifact cancelled), and whether it tried to give any.
    IntentDebuff debuffs[MAX_INTENT_DEBUFFS];
    bool debuff_attempted = false;
    CardId cards_added = CardId::NONE; // card the move put in the player's piles, and how many of it
    uint8_t cards_added_count = 0;
    PowerId power = PowerId::NONE; // power the move put on the player, NONE when it gave none or Artifact cancelled it
    uint8_t hits = 1;     // hits of its attack
    int16_t healed = 0;   // HP the enemy gave itself back
    bool escaped = false; // it left the fight with this move
    uint8_t summoned = 0; // enemies the move brought into the fight, one bit each (index in CombatEngine::enemies)
};

// Which cards an effect may draw at random. A field left at its default does not filter.
struct CardFilter {
    CardPool pool = CardPool::NONE;       // NONE: the pool of the hero of the fight
    CardRarity rarity = CardRarity::NONE; // NONE: Common, Uncommon or Rare
    bool any_type = true;
    CardType type = CardType::ATTACK;     // read when `any_type` is false
    int16_t cost = -1;                    // energy cost the card must have, -1 for any
    bool cost_upgraded = false;           // the cost is read on the upgraded card

    constexpr CardFilter() = default;
    constexpr CardFilter(CardPool p) : pool(p) {}
    constexpr CardFilter(CardType t) : any_type(false), type(t) {}
    constexpr CardFilter(CardPool p, CardType t) : pool(p), any_type(false), type(t) {}
    constexpr CardFilter(CardRarity r) : rarity(r) {}
    constexpr CardFilter(CardPool p, CardRarity r) : pool(p), rarity(r) {}
};

// Tallies read by card effects. The turn ones restart with each player turn, the combat ones with the fight.
struct CombatCounters {
    // This turn
    uint8_t attacks_played = 0;
    uint8_t skills_played = 0;
    uint8_t powers_played = 0;
    uint8_t statuses_played = 0; // Status cards that can be played (Slimed, Debris)
    uint8_t shivs_played = 0;
    uint8_t cards_drawn = 0;     // drawn by effects, after the turn's opening draw
    uint8_t cards_discarded = 0; // discarded by effects, not by the end of the turn
    uint8_t energy_spent = 0;
    uint8_t hits_dealt = 0;      // attack hits on enemies
    uint8_t hits_on_enemy[MAX_ENEMIES] = {}; // the same, for each enemy of CombatEngine::enemies
    uint8_t stars_gained = 0;
    uint8_t stars_spent = 0;
    uint8_t osty_attacks = 0;    // attacks of Osty, one for an attack on every enemy
    uint8_t fetches_played = 0;  // times Fetch was played
    int16_t hp_lost = 0;
    // This combat
    uint8_t combat_cards_played = 0;
    uint8_t combat_cards_drawn = 0;
    uint8_t combat_cards_created = 0;
    uint8_t combat_ethereal_played = 0;
    uint8_t combat_hp_losses = 0; // number of times the player lost HP
    uint8_t combat_orbs_channeled = 0;
    uint8_t combat_lightning_channeled = 0;
    uint8_t doom_applied = 0;     // this turn: times Doom was applied to an enemy

    uint8_t cards_played() const {
        return static_cast<uint8_t>(attacks_played + skills_played + powers_played + statuses_played);
    }
    void start_turn() {
        attacks_played = skills_played = powers_played = statuses_played = shivs_played = 0;
        cards_drawn = cards_discarded = energy_spent = hits_dealt = 0;
        hits_on_enemy[0] = hits_on_enemy[1] = hits_on_enemy[2] = 0;
        stars_gained = stars_spent = 0;
        osty_attacks = 0;
        fetches_played = 0;
        doom_applied = 0;
        hp_lost = 0;
    }
};

// Effects scheduled for a later turn.
enum class DelayedType : uint8_t {
    ENERGY,            // start of turn
    BLOCK,             // start of turn, not modified by Dexterity
    DRAW,              // start of turn, with the opening draw
    STARS,             // start of turn
    SUMMON,            // start of turn
    CHANNEL_LIGHTNING, // start of turn
    DAMAGE_ALL,        // end of turn: damage to every enemy, not modified by Strength (countdown cards)
    PICK_FROM_DRAW,    // start of turn, once the hand is drawn: `amount` cards of the draw pile, picked, go to the hand
    COPIES_TO_HAND     // start of turn: NIGHTMARE_COPIES copies of a card to the hand; `amount` = card id * 2 + upgraded
};

constexpr uint8_t NIGHTMARE_COPIES = 3;
// Cards Vakuu plays at most during his turn (Whispering Earring): cards that come back to the hand, or that
// make others, cannot keep him playing forever.
constexpr uint8_t VAKUU_MAX_PLAYS = 50;

struct DelayedEffect {
    DelayedType type = DelayedType::ENERGY;
    int16_t amount = 0;
    uint8_t delay = 1;  // turns before it first triggers: 1 = the next start of turn, or this end of turn
    uint8_t repeat = 1; // consecutive turns it triggers
};

// Why a hand card cannot be played right now.
enum class PlayBlocker : uint8_t {
    NONE,
    NOT_PLAYABLE, // not the player's turn, or no such card
    ENERGY,
    STARS,
    CONDITION,    // the card's own PlayCondition
    PLAY_FIRST,   // another card in hand must be played first
    PLAY_LIMIT,   // a card in hand caps the number of cards played this turn
    CHOICE        // cards of the hand must be picked first (CombatEngine::pending_choices)
};

// What happens to the cards the player picks.
enum class ChoiceKind : uint8_t {
    DISCARD,
    EXHAUST,
    TOP_OF_DRAW,  // put on top of the draw pile
    TO_HAND,      // from the draw or the discard pile; a card stays where it is when the hand is full
    UPGRADE,      // for the rest of the combat
    MAKE_ETHEREAL, // the copy gains Ethereal
    MAKE_RETAIN,  // the copy gains Retain
    // The picked hand card stays; `amount` copies of it are made (CardChoice::amount):
    COPY_ATTACK_OR_POWER, // to the hand, only an Attack or a Power can be picked (Dual Wield)
    COPY_COLORLESS,       // to the hand, only a Colorless card can be picked (Heirloom Hammer)
    COPY_NEXT_TURN,       // to the hand at the start of the next turn (Nightmare)
    PLAY_REPEATED,        // the picked hand card is played `amount` times, then goes where it goes (Decisions, Decisions)
    MAKE_SLY,             // the picked hand card gains Sly until the end of the turn (Hand Trick)
    MAKE_FREE,            // the picked hand card costs 0 for the rest of the combat (Touch of Insanity)
    ADD_ECHO,             // the picked hand card gains Echo 1 and costs 1 more (Transfigure)
    TRANSFORM             // the picked card becomes `CardChoice::into` (a random card of the hero's pool when NONE) for the combat
};

// What follows once the cards are picked: the part of a card's text that comes after its choice.
enum class ChoiceThen : uint8_t {
    NONE,
    DRAW,          // draw `amount` cards
    DRAW_PER_CARD, // draw one card for each card picked
    SHIVS          // add `amount` Shivs to the hand, upgraded when `upgraded`
};

constexpr uint8_t WHOLE_PILE = 0xFF;
// Cards a choice about Pile::OFFERS offers at most (Choices Paradox offers 5).
constexpr uint8_t MAX_OFFERS = 5;
// Where an offered card comes from when it was made for the offer, not taken from the draw pile.
constexpr uint8_t NOT_IN_PILE = 0xFF;

// The Soul (Necrobinder) is a card, not a gauge: a token that costs 0, "Draw 2 cards. Exhaust." (3 cards once
// upgraded), which other cards add to the piles. This is its id.
constexpr CardId SOUL_CARD = CardId::SOUL;
constexpr bool is_soul(CardId id) { return id == SOUL_CARD; }

// Cards the player has to pick, in the hand or in a pile, before the combat goes on.
struct CardChoice {
    ChoiceKind kind = ChoiceKind::DISCARD;
    uint8_t count = 1;     // cards to pick; all the cards that can be picked when there are fewer
    bool optional = false; // fewer cards, or none, can be picked ("any number of cards")
    ChoiceThen then = ChoiceThen::NONE;
    uint8_t amount = 0;
    bool upgraded = false;    // TRANSFORM into a given card: the new card comes upgraded
    CardId into = CardId::NONE; // TRANSFORM: the card it becomes, NONE for a random one
    Pile source = Pile::HAND; // where the cards are picked
    // Cards of the source the choice is about, from its first one. Set by ask_choice for the discard pile:
    // the card that asks goes there once played, and cannot pick itself.
    uint8_t pool = WHOLE_PILE;
    // Only cards of this type can be picked, when `any_type` is false (Secret Technique, Secret Weapon).
    bool any_type = true;
    CardType type = CardType::ATTACK;
    // Pile::OFFERS: the `amount` cards offered are dealt when the choice comes first, from the draw pile when
    // `offer_from_draw`, otherwise made at random from `offer_filter` (upgraded when `upgraded`, costing 0 this
    // turn when `offer_free`). The picked cards go to the hand, the others stay where they were or vanish.
    bool offer_from_draw = true;
    bool offer_free = false;
    bool offer_retain = false;   // the cards made for the offer have Retain
    CardFilter offer_filter;
    bool free_this_turn = false; // TO_HAND: the cards picked cost 0 this turn (Liquid Memories)
};

// Where a card goes once it has been played.
enum class AfterPlay : uint8_t {
    DISCARD,
    HAND,
    DRAW_TOP,
    EXHAUST,
    NOWHERE // it stays where it was: played from the exhaust pile
};

// A gain that outlives the combat: RunManager adds it to the card of the deck.
struct PermanentGain {
    uint8_t deck_index = NOT_IN_DECK;
    uint8_t amount = 0;
    bool enchantment = false; // added to the card's enchantment amount (Goopy) instead of its bonus
};

class CombatEngine {
public:
    Player player;
    Enemy enemy; // Primary enemy (kept in sync with enemies[0])
    FixedVector<Enemy, MAX_ENEMIES> enemies; // Multi-enemy field: 1 to MAX_ENEMIES enemies
    int selected_target_index = 0;
    // Ascension of the room (set by the run), given to the enemies summoned during the fight: 100 = none.
    int16_t enemy_hp_percent = 100;
    int16_t enemy_damage_percent = 100;
    int16_t gold_lost = 0; // gold taken during the combat (curses in hand at the end of a turn), paid by the run after it
    int16_t gold_gained = 0; // gold earned during the combat (Hand of Greed), paid by the run after it
    uint8_t extra_card_rewards = 0; // extra card rewards earned during the combat (The Hunt)
    uint8_t potions_gained = 0;     // random potions earned during the combat (Alchemize)
    bool fill_potion_belt = false;  // the run fills its empty potion slots with random potions (Entropic Brew)
    bool gambit_active = false;     // taking unblocked attack damage ends the combat in defeat (The Gambit)
    int16_t gold = 0; // the run's gold, lent to the combat for the relics that spend it (Seal of Gold)
    CharacterId current_character = CharacterId::IRONCLAD;
    
    FixedVector<DeckCard, 50> draw_pile;
    FixedVector<DeckCard, 10> hand;
    FixedVector<DeckCard, 50> discard_pile;
    FixedVector<DeckCard, 50> exhaust_pile;
    uint8_t exhausted_this_turn = 0; // cards exhausted since the start of the player's turn
    FixedVector<DeckCard, 10> returning_cards; // played cards coming back to the hand next turn (KEYWORD_RETURN)
    CombatCounters counters;
    FixedVector<DelayedEffect, 12> delayed_effects;
    uint8_t keep_block_turns = 0;  // starts of turn where the player's Block is not removed (Blur)
    bool retain_hand = false;      // the whole hand stays at the end of this turn
    bool no_draw_this_turn = false; // "you cannot draw any more cards this turn"
    uint8_t no_card_block_turns = 0; // player turns left (this one included) where cards give no Block (Panic Button)
    bool double_block_turn = false;   // the Block gained this turn is doubled (Shadowmeld)
    bool unmovable_used = false;      // Unmovable already doubled a Block this turn
    bool attack_double_next = false;  // Attacks deal double damage on the next turn (Shadow Step)
    uint8_t fairy_bottles = 0;        // Fairy in a Bottle potions ready to revive the player (automatic, set by the run)
    uint8_t fairy_used = 0;           // Fairy in a Bottle potions spent during this combat
    bool triple_next_attack = false;  // the next Attack played deals triple damage (Gigantification Potion)
    bool attack_double_now = false;   // Attacks deal double damage this turn
    bool pen_nib_now = false;         // the Attack being played deals double damage (Pen Nib)
    bool minion_played_now = false;   // the card being played is a Minion card (Vitruvian Minion)
    bool strike_played_now = false;   // the card being played contains "Strike" (Strike Dummy)
    bool upgraded_attack_now = false; // the card being played is an upgraded Attack (Miniature Cannon)
    // What the enchantment of the card being resolved adds (mechanic #107): damage per hit (Sharp, Tezcatara's
    // Ember, Momentum, Mystic Lighter), doubled damage (Instinct), Block (Nimble, Goopy), Weak on each enemy hit (Inky).
    int16_t enchant_damage_now = 0;
    bool enchant_double_now = false;
    bool enchant_corrupted_now = false; // Corrupted: +50% damage
    int16_t enchant_block_now = 0;
    bool inky_now = false;
    bool rebound_pending = false;    // the next card played this turn goes on top of the draw pile (Rebound)
    uint8_t fatal_kills = 0;       // enemies killed by the hits of the card being played (minions do not count)
    // Active powers, in the order they were gained: every relic a run can hold plus a combat's worth of
    // card powers (add_power drops a power silently once it is full).
    FixedVector<ActivePower, 64> powers;
    FixedVector<DeckCard, 24> powers_in_play; // Power cards played this combat: they stay in play
    FixedVector<PermanentGain, 16> permanent_gains; // bonuses to write back to the deck after the combat
    FixedVector<CardChoice, 4> pending_choices;      // asked in order, the first one first
    // Cards offered by the first choice when it is about Pile::OFFERS, and the place of each one in the draw
    // pile (NOT_IN_PILE for a card made for the offer). Empty when no offer is being asked.
    FixedVector<DeckCard, MAX_OFFERS> offers;
    FixedVector<uint8_t, MAX_OFFERS> offer_origins;
    bool resolving_card = false;     // a card's effect is being resolved (Paël's Legion doubles its Block)
    bool lamp_doubling = false;      // the card being resolved is the one Unsettling Lamp doubles
    bool extra_turn_pending = false;  // a potion (Ambergris) grants an extra turn: the next end of turn skips the enemies
    bool end_turn_requested = false; // a card ended the turn (Void Form): the caller ends it once the card is resolved
    int8_t temp_strength = 0;      // Strength and Dexterity gained until the end of the turn
    int8_t temp_dexterity = 0;
    int8_t temp_focus = 0;
    int16_t forged = 0;            // total Forged this combat: what the Sovereign Blade gained on top of its damage
    // Whispering Earring: Vakuu is playing this turn, the first of the combat, instead of the player. Set when
    // the first turn starts with PowerId::WHISPERING_EARRING registered, cleared when the turn ends.
    bool vakuu_turn_active = false;

    FixedVector<EnemyActionRecord, MAX_ENEMIES> last_enemy_actions;
    // HP of each enemy once the end of the player's turn is resolved (orb passives, Doom...), before the enemies
    // act: the scene shows that damage on its own. Set by end_player_turn, which empties it when it stops earlier.
    FixedVector<int16_t, MAX_ENEMIES> turn_end_enemy_hp;

    CombatState state = CombatState::PLAYER_TURN;
    int turn_number = 1;
    int selected_card_index = 0;
    
    // Simple notification message
    char combat_log[48] = {0};

    void init_starter_deck(CharacterId character = CharacterId::IRONCLAD);
    void start_combat(EnemyType enemy_type, CharacterId character = CharacterId::IRONCLAD);
    void start_combat_multi(std::span<const EnemyType> enemy_types, CharacterId character = CharacterId::IRONCLAD);
    // Opens the fight once the draw pile is ready: clears the combat state, then starts the first turn.
    void open_first_turn();
    // The two halves of open_first_turn, for whoever registers powers (relics) between them.
    // Plans the next move of every living enemy, with what each one sees of the fight. On the first turn the
    // patterns start again.
    void plan_enemies();
    // Brings an enemy into the fight, at the end of CombatEngine::enemies or in the place of a dead one; a
    // Minion of `leader` when one is given. It acts from the next enemy turn on, and its move is planned with the
    // others'. Its index, or -1 when the MAX_ENEMIES places are taken.
    int summon_enemy(EnemyType type, int leader = -1);
    // Gives a power to the enemy at `enemy_index`: it holds it until it dies or leaves the fight.
    void add_enemy_power(int enemy_index, PowerId id, int16_t amount = 1);
    // Amount of a power that enemy holds, 0 when it does not.
    [[nodiscard]] int16_t enemy_power_amount(int enemy_index, PowerId id) const;
    // Puts a harmful power on the player, as an enemy move does. False when Artifact cancelled it.
    bool afflict_player(PowerId id, int16_t amount, bool turn_only);
    // True when every enemy of the fight ran away: nothing was killed, so nothing is won.
    [[nodiscard]] bool every_enemy_escaped() const;
    void reset_combat_state();
    // Puts the Innate cards on top of the draw pile and starts the first turn. Once the hand is drawn, the
    // Imbued cards of the draw pile are played for free.
    void begin_first_turn();
    void start_player_turn();
    // Draws one card, reshuffling the discard pile when needed. False when nothing was drawn (full hand, empty piles).
    bool draw_card();
    void draw_cards(int count);
    void shuffle_discard_into_draw();
    // Removes the card from the combat: it goes to the exhaust pile.
    void exhaust_card(DeckCard card);
    // Exhausts the hand card at `index` and returns it.
    DeckCard exhaust_from_hand(size_t index);
    // Discards the hand card at `index` because of an effect (counted, unlike the end of turn discard).
    void discard_from_hand(size_t index);
    // Adds a card made during the combat to a pile: at a random place of the draw pile, and in the discard
    // pile when the hand is full.
    // Whether `id` is a card `filter` lets an effect draw: of the pool and rarity asked, never a Basic, Ancient
    // or Event card, a Curse, a Status card or a token. Every card of the engine has its effect, so every card
    // that fits can be drawn.
    [[nodiscard]] bool card_fits(CardId id, const CardFilter& filter) const;
    // A card drawn at random among the ones that fit, with the fight's random generator. NONE when none fits.
    CardId random_card(const CardFilter& filter = {});
    // Creates such a card in `pile`. False when no card fits.
    bool create_random_card(const CardFilter& filter, Pile pile, bool upgraded = false);
    // `by_player` is false for a card an enemy puts in the player's piles: nothing of the player reacts to it
    // as to a card they created.
    void create_card(DeckCard card, Pile pile, bool by_player = true);
    void create_card_in_discard(DeckCard card) { create_card(card, Pile::DISCARD); }
    // Adds Shivs to the hand (Silent).
    void add_shivs(int count, bool upgraded = false, Enchantment enchantment = Enchantment::NONE);
    // Forge (Regent): the first Forge of the combat adds Sovereign Blade to the hand, and every Forge adds
    // `amount` to the damage of every Sovereign Blade, wherever it is, for the rest of the combat.
    void forge(int amount);

    // One attack hit on an enemy: Strength, Weak and Vulnerable applied. Returns the HP lost by the enemy.
    int16_t hit(Enemy& target, int16_t base);
    // Gains a power, or adds to the one already active with the same scope and target.
    void add_power(PowerId id, int16_t amount = 1, bool turn_only = false, int8_t target = -1);
    // Total amount of a power, 0 when it is not active.
    int16_t power_amount(PowerId id) const;
    bool has_power(PowerId id) const { return power_amount(id) != 0; }
    // The power's own tally (see ActivePower::counter), 0 when it is not active.
    uint8_t power_counter(PowerId id) const;

    // Block from a card: Dexterity applies.
    void gain_block(int16_t amount);
    // Block from anything else: the exact amount.
    void add_block(int16_t amount);
    // Applies a status to an enemy.
    void afflict(Enemy& target, StatusType type, int16_t amount);
    // Damage that is not an attack: no Strength, no Vulnerable, the enemy's Block still absorbs it.
    void damage_enemy(Enemy& target, int16_t amount);
    void damage_all_enemies(int16_t amount);

    // Orbs (Defect). Slot 0 is the oldest orb, the "rightmost" of the card texts.
    void channel_orb(OrbType type);
    // Triggers the orb's evoke effect `times` times, then removes it from its slot.
    void evoke_orb(size_t slot, int times = 1);
    // Triggers the orb's passive effect. Lightning hits `target` when given, a random enemy otherwise.
    void trigger_orb_passive(size_t slot, Enemy* target = nullptr);
    // Gains or loses orb slots. An orb left without a slot is removed, the newest first, without being evoked.
    void change_orb_slots(int delta);
    // Number of different orb types in the slots.
    int unique_orbs() const;
    // Kills the enemy when its Doom has reached its HP.
    void reap(Enemy& target);

    // Osty (Necrobinder). He is summoned with Player::summon_minion; what he does and what happens to him goes
    // through these, so that the powers hear about it.
    // "Osty deals `base` damage": one attack hit with Strength, Weak and Vulnerable applied, like the player's.
    // Nothing happens when he is dead. Returns the HP lost by the enemy. With `hits`, the attack hits that many
    // times, as long as the enemy is alive: it is still one attack, and the HP lost are those of the last hit.
    int16_t osty_attack(Enemy& target, int16_t base, int hits = 1);
    // The same on every living enemy: it counts as one attack.
    void osty_attack_all(int16_t base);
    // Osty recovers HP, up to his Max HP. Nothing happens when he is dead.
    void heal_osty(int16_t amount);
    // Osty dies ("Osty dies", or the hit that brings his HP to 0). The HP he had left count as lost.
    void kill_osty();

    // Souls (Necrobinder). Adds `count` Souls to a pile, as cards made during the combat: at random places of
    // the draw pile, and in the discard pile when the hand is full.
    void add_souls(int count, Pile pile = Pile::DRAW, bool upgraded = false);
    // "For each Soul in your Exhaust Pile": a Soul is exhausted when it is played.
    int exhausted_souls() const;

    // Stars gained or spent by the Regent: every change goes through these two.
    void gain_stars(int amount);
    void spend_stars(int amount);
    // Damage to the player that is not an attack (Burn): Block absorbs it.
    void damage_player(int16_t amount);
    // The player loses HP: Block does not absorb it.
    void lose_hp(int16_t amount);
    // Takes gold from the run (paid once the combat is over).
    void lose_gold(int16_t amount) { if (amount > 0) gold_lost = static_cast<int16_t>(gold_lost + amount); }
    // Effect of a card still in hand at the end of the player's turn (Burn, curses): damage, HP loss, gold loss
    // or a status. Cards without such an effect do nothing.
    void trigger_end_of_turn_in_hand(const DeckCard& card);
    // Sly cards discarded from the hand during the turn wait here until the action that discarded them is over.
    FixedVector<DeckCard, 12> sly_pending;
    void play_sly_pending();
    // The card becomes `into` for the rest of the combat (a random card of the hero's pool when NONE).
    void transform_card(DeckCard& card, CardId into, bool upgraded);
    // Strength or Dexterity until the end of the turn.
    void gain_temporary(StatusType type, int8_t amount);
    // Strength gained by the player; returns the amount applied (Ruined Helmet doubles the first gain of the combat).
    int16_t gain_strength(int16_t amount);
    // The enemy loses Strength until the end of its turn.
    void sap_strength(Enemy& target, int16_t amount);
    // Schedules an effect: see DelayedEffect.
    void schedule(DelayedType type, int16_t amount, uint8_t delay = 1, uint8_t repeat = 1);
    
    // Asks the player to pick cards of the hand or of a pile. Nothing is asked when there is nothing to decide: every card
    // that can be picked is, at once. Otherwise the choice waits in `pending_choices`, and no card can be
    // played until it is resolved.
    void ask_choice(const CardChoice& choice);
    // Same, for cards picked in the draw or the discard pile.
    void ask_pile_choice(Pile source, ChoiceKind kind, uint8_t count = 1);
    // "Choose 1 of `count` cards in your Draw Pile to add into your Hand" (Seeker Strike): `count` different
    // cards of the draw pile, at random, are offered; the picked one goes to the hand, the others stay.
    void offer_from_draw(uint8_t count);
    // "Choose 1 of `count` random cards to add to your Hand" (Discovery): `count` different random cards of
    // `filter` are made and offered; the picked one goes to the hand, the others vanish.
    void offer_random_cards(const CardFilter& filter, uint8_t count, bool upgraded = false, bool free_this_turn = false, bool retain = false);
    // Cards of a pile, as the choices index them.
    size_t pile_size(Pile pile) const;
    const DeckCard& pile_card(Pile pile, size_t index) const;
    bool choice_pending() const { return !pending_choices.empty(); }
    // Whether the card at `index` of the choice's source can be picked for the choice being asked.
    bool choice_allows(size_t index) const;
    // First position, from `from` (included) stepping by `step` (+1 or -1), whose card can be picked. Positions
    // go through `order` (indices of the choice's source, as a sorted pile view shows them), or through the
    // indices 0 to the pool size when `order` is empty. With `wrap` the search goes around the ends. -1 when no
    // card can be picked.
    int next_pickable_choice(std::span<const uint8_t> order, int from, int step, bool wrap) const;
    // Number of cards the choice being asked expects: fewer than its count when the hand is short.
    int choice_size() const;
    // Number of cards of its source the choice being asked is about: indices 0 to this number can be shown.
    int choice_pool_size() const;
    // Applies the choice being asked to the picked cards of its source. False, and nothing happens, when the picks
    // do not answer it (wrong number, same card twice, card that cannot be picked).
    bool resolve_choice(std::span<const uint8_t> indices);
    // Answers the choice being asked without the player: the first cards that can be picked, none when
    // the choice is optional.
    void resolve_choice_default();

    PlayBlocker play_blocker(int index) const;
    bool can_play_card(int index) const { return play_blocker(index) == PlayBlocker::NONE; }
    bool play_condition_met(PlayCondition condition) const;
    bool play_card(int index, int target_index = -1);
    // Plays a card for free that does not come from the hand: the caller has already taken it from where it
    // was. A single-target card goes to `target_index`, or to a random enemy when it is negative.
    void autoplay(DeckCard card, int target_index = -1, AfterPlay destination = AfterPlay::DISCARD);
    // Vakuu's turn (Whispering Earring). He plays the cards of the hand from left to right, each time the
    // leftmost one he can play, so the cards drawn or made on the way are played too. He plays a card while it
    // is playable and paid for (the cards that cost 0 still go once the energy is spent), aims the single-target
    // ones at the leftmost living enemy, and leaves in hand the Unplayable cards, the ones whose condition
    // fails, the ones that would ask the player to pick cards (opens_choice) and the X cards with nothing to
    // spend. He stops after VAKUU_MAX_PLAYS cards.
    // Whether playing the card asks the player to pick cards of the hand or of a pile.
    [[nodiscard]] static bool opens_choice(const DeckCard& card);
    // Index in the hand of the card Vakuu plays next, -1 when his turn must end (or is not his). A choice
    // asked by something else than his own plays (a power, a relic) is answered for the player first.
    [[nodiscard]] int vakuu_next_card();
    // Plays that card. False when there is none: the caller ends the turn (end_player_turn).
    bool vakuu_step();
    // Vakuu's whole turn, its end included. Returns the number of cards he played, 0 when it is not his turn.
    int vakuu_play_turn();
    // Energy the card costs right now, COST_X for an X card.
    int effective_cost(const DeckCard& card) const;
    void end_player_turn();
    void execute_enemy_turn();

    // Applies a potion's effect during the player's turn. False when it can't be drunk (unknown potion, enemy turn).
    bool use_potion(std::string_view potion_id, int target_index = -1);

    bool has_alive_enemies() const;
    int count_alive_enemies() const;
    int get_first_alive_enemy_index() const;

    void shuffle_draw_pile();
    void set_log(std::string_view msg);

private:
    uint32_t rng_state_ = 123456789;
    uint32_t next_random();
    // Index of a living enemy picked at random; the first enemy when none is alive.
    int random_alive_enemy_index();
    void note_player_hp_lost(int16_t amount);
    // One hit of an attack of Osty: Calcify, and Sic 'Em on the enemy it is bound to.
    int16_t osty_hit(Enemy& target, int16_t base);
    void note_osty_attack();
    void note_osty_hp_lost(int16_t amount);
    // Tells every active power about an event, in the order the powers were gained.
    void emit(CombatEvent event, const EventInfo& info = EventInfo());
    void react(ActivePower& power, CombatEvent event, const EventInfo& info);
    ActivePower* find_power(PowerId id);
    int8_t index_of(const Enemy& target) const;
    // Moves every copy of `id` from the draw and discard piles to the hand, as long as there is room.
    void recall_to_hand(CardId id);
    // One play of a card and the extra plays it is entitled to: event, effect, tallies. The card is not moved.
    void run_card(DeckCard& card, int target_index, bool random_target, int x_value, uint8_t energy_paid, AfterPlay& after_play);
    // Sets the enchant_..._now modifiers for the card about to be resolved.
    void set_enchant_modifiers(const DeckCard& card);
    // The card's own effect.
    void resolve_effect(DeckCard& played_card, int target_idx, int x_value, AfterPlay& after_play);
    void place_played_card(const DeckCard& card, AfterPlay after_play);
    // What follows any card: deaths, defeat, victory.
    void finish_play();
    bool costs_nothing(const DeckCard& card) const;
    // Uses up what made the card free or played twice. Returns the number of extra plays.
    int consume_play_modifiers(const DeckCard& card);
    // Adds `amount` to the bonus of every copy of `id` in the piles.
    void boost_copies(CardId id, int amount);
    bool can_be_picked(const DeckCard& card, ChoiceKind kind) const;
    bool can_be_picked(const DeckCard& card, const CardChoice& choice) const;
    void apply_choice(const CardChoice& choice, std::span<const uint8_t> indices);
    // Number of cards of its source the choice is about.
    size_t choice_pool(const CardChoice& choice) const;
    // Answers, without asking, the choices that leave nothing to decide.
    void settle_choices();
    // Fills `offers` for a choice about Pile::OFFERS.
    void deal_offers(const CardChoice& choice);
    void clear_offers();
    void change_cost_of_copies(CardId id, int delta);
    uint8_t autoplay_depth_ = 0;
    uint8_t vakuu_plays_ = 0;     // cards Vakuu has played this turn
    // Orb value once Focus is applied, never negative.
    int16_t with_focus(int16_t base) const;
    // Raises ENEMY_KILLED for every enemy that died since the last call, whatever killed it.
    void settle_deaths();
    uint8_t alive_mask_ = 0;      // enemies known to be alive, one bit each
    uint8_t event_depth_ = 0;     // reactions triggering reactions
    int16_t turn_draw_bonus_ = 0; // cards added to the opening draw by the start-of-turn powers
    uint8_t turn_picks_from_draw_ = 0; // cards to pick in the draw pile once the hand is drawn (Foregone Conclusion)
    bool opening_draw_ = false;   // the turn's opening hand is being drawn
    bool lost_hp_last_turn_ = false; // HP lost between the previous turn start and this one
    bool blade_forged_ = false;   // the combat's first Forge has made the Sovereign Blade
    // Orb passives of the start (Plasma) or of the end (the others) of the turn.
    void trigger_turn_passives(bool turn_start);
    // Triggers the scheduled effects of the start (or the end) of the turn. Returns the extra cards to draw.
    int run_delayed_effects(bool end_of_turn);
};

} // namespace engine
