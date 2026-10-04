// Cards made during a combat (#77), percent modifiers of damage and Block (#89), transformed cards (#79), and the
// Ironclad, Silent and Defect cards built on them. One case per rule of each mechanic, then one case per card with
// the base and the upgraded numbers of bible/data/card_v0.111.0.json.
#include <cassert>
#include <algorithm>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

// A hero against `count` enemies with 200 HP each, empty piles, and the hero's pool for the random cards.
CombatEngine make_fight(CharacterId hero = CharacterId::IRONCLAD, int count = 1) {
    FixedVector<EnemyType, 3> pack;
    for (int i = 0; i < count; ++i) pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, hero);
    for (Enemy& foe : combat.enemies) {
        foe.max_hp = 200;
        foe.hp = 200;
        foe.block = 0;
    }
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.card_pool = RunManager::card_reward_pool(hero);
    return combat;
}

// Ends the turn with enemies that do not attack, or that all attack for `damage`.
void end_turn(CombatEngine& combat, int16_t damage = 0) {
    for (Enemy& foe : combat.enemies) {
        foe.current_intent = EnemyIntent();
        foe.current_intent.type = damage > 0 ? EnemyIntentType::ATTACK : EnemyIntentType::BUFF;
        foe.current_intent.base_damage = damage;
    }
    combat.end_player_turn();
}

// Adds `card` to the hand with full energy and plays it.
void play(CombatEngine& combat, DeckCard card, int target = 0) {
    combat.hand.push_back(card);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(static_cast<int>(combat.hand.size()) - 1, target);
    assert(played);
}

// HP the first enemy loses when `card` is played on it.
int16_t damage_of(CombatEngine& combat, DeckCard card) {
    int16_t before = combat.enemies[0].hp;
    play(combat, card);
    return static_cast<int16_t>(before - combat.enemies[0].hp);
}

// Block the player gains when `card` is played, starting from none.
int16_t block_of(CombatEngine& combat, DeckCard card) {
    combat.player.block = 0;
    play(combat, card);
    return combat.player.block;
}

template <size_t Capacity>
int count_of(const FixedVector<DeckCard, Capacity>& pile, CardId id) {
    int count = 0;
    for (const DeckCard& card : pile) count += card.id == id;
    return count;
}

bool in_pool(CharacterId hero, CardId id) {
    auto pool = RunManager::card_reward_pool(hero);
    return std::find(pool.begin(), pool.end(), id) != pool.end();
}

void expect_costs(CardId id, int base, int upgraded) {
    assert(CardDef::get_cost(id, false) == base);
    assert(CardDef::get_cost(id, true) == upgraded);
}

// --- #77: cards made during the combat -----------------------------------------------------------------------

void test_random_card_needs_a_pool() {
    CombatEngine combat = make_fight();
    combat.card_pool = {};
    assert(combat.random_card() == CardId::NONE);
    assert(!combat.create_random_card(CardFilter::ANY, Pile::HAND));
    assert(combat.hand.empty() && combat.counters.combat_cards_created == 0);
    // Infernal Blade without a pool makes nothing, and is still played.
    play(combat, CardId::INFERNAL_BLADE);
    assert(combat.hand.empty() && combat.exhaust_pile.size() == 1);
}

void test_random_card_follows_the_filter_and_the_pool() {
    for (CharacterId hero : { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT }) {
        CombatEngine combat = make_fight(hero);
        bool types_seen[3] = {};
        CardId first = CardId::NONE;
        bool varied = false;
        for (int draw = 0; draw < 200; ++draw) {
            CardId any = combat.random_card();
            assert(in_pool(hero, any));
            types_seen[static_cast<int>(CardDef::get(any).type)] = true;
            if (first == CardId::NONE) first = any;
            varied = varied || any != first;
            assert(CardDef::get(combat.random_card(CardFilter::ATTACK)).type == CardType::ATTACK);
            assert(CardDef::get(combat.random_card(CardFilter::SKILL)).type == CardType::SKILL);
            assert(CardDef::get(combat.random_card(CardFilter::POWER)).type == CardType::POWER);
            assert(in_pool(hero, combat.random_card(CardFilter::POWER)));
        }
        assert(varied && types_seen[0] && types_seen[1] && types_seen[2]);
    }
}

void test_random_card_can_leave_one_out() {
    CombatEngine combat = make_fight();
    static constexpr CardId TWO[] = { CardId::ANGER, CardId::INFLAME };
    combat.card_pool = TWO;
    for (int draw = 0; draw < 20; ++draw) {
        assert(combat.random_card(CardFilter::ANY, CardId::ANGER) == CardId::INFLAME);
        assert(combat.random_card(CardFilter::ATTACK) == CardId::ANGER);
    }
    assert(combat.random_card(CardFilter::ATTACK, CardId::ANGER) == CardId::NONE);
    assert(combat.random_card(CardFilter::SKILL) == CardId::NONE);
}

void test_random_card_is_a_card_made() {
    CombatEngine combat = make_fight();
    combat.add_power(PowerId::ARSENAL, 1); // card created: 1 Strength
    assert(combat.create_random_card(CardFilter::ATTACK, Pile::HAND, true, true));
    assert(combat.hand.size() == 1 && combat.hand[0].upgraded && combat.hand[0].free_this_turn);
    assert(combat.hand[0].deck_index == NOT_IN_DECK);
    assert(combat.effective_cost(combat.hand[0]) == 0 || CardDef::is_x_cost(combat.hand[0].id, true));
    assert(combat.counters.combat_cards_created == 1);
    assert(combat.player.get_status(StatusType::STRENGTH) == 1);

    assert(combat.create_random_card(CardFilter::SKILL, Pile::DRAW));
    assert(combat.draw_pile.size() == 1 && !combat.draw_pile[0].upgraded && !combat.draw_pile[0].free_this_turn);
    assert(combat.create_random_card(CardFilter::POWER, Pile::DISCARD));
    assert(combat.discard_pile.size() == 1 && CardDef::get(combat.discard_pile[0].id).type == CardType::POWER);
    assert(combat.counters.combat_cards_created == 3);
}

void test_random_card_overflows_to_the_discard_pile() {
    CombatEngine combat = make_fight();
    for (int i = 0; i < 10; ++i) combat.hand.push_back(CardId::DEFEND);
    assert(combat.create_random_card(CardFilter::ATTACK, Pile::HAND, false, true));
    assert(combat.hand.size() == 10 && combat.discard_pile.size() == 1);
    // It never reached the hand: it keeps its cost for the day it is drawn.
    assert(!combat.discard_pile[0].free_this_turn);
}

void test_free_card_costs_again_next_turn() {
    CombatEngine combat = make_fight();
    static constexpr CardId ONE[] = { CardId::BLUDGEON };
    combat.card_pool = ONE;
    combat.retain_hand = true;
    assert(combat.create_random_card(CardFilter::ATTACK, Pile::HAND, false, true));
    assert(combat.effective_cost(combat.hand[0]) == 0);
    end_turn(combat);
    assert(combat.hand[0].id == CardId::BLUDGEON && combat.effective_cost(combat.hand[0]) == 3);
}

void test_run_gives_the_pool_of_the_hero() {
    for (CharacterId hero : { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT, CharacterId::REGENT,
                              CharacterId::NECROBINDER }) {
        RunManager run;
        run.start_run(hero, 0, PaletteVariant::CLASSIC, 777);
        run.map.nodes[0].type = RoomType::MONSTER;
        run.enter_room(0);
        auto pool = RunManager::card_reward_pool(hero);
        assert(run.combat.card_pool.data() == pool.data() && run.combat.card_pool.size() == pool.size());
        assert(in_pool(hero, run.combat.random_card()));
    }
}

// --- #89: percent modifiers ----------------------------------------------------------------------------------

void test_attack_damage_is_unchanged_without_modifiers() {
    CombatEntity attacker, target;
    assert(attack_damage(10, attacker, target) == 10);
    attacker.add_status(StatusType::WEAK, 1);
    assert(attack_damage(10, attacker, target) == 7);
    target.add_status(StatusType::VULNERABLE, 1);
    assert(attack_damage(10, attacker, target) == 11); // 10 x 0.75 x 1.5 = 11.25
    attacker.clear_status(StatusType::WEAK);
    assert(attack_damage(10, attacker, target) == 15);
    attacker.add_status(StatusType::STRENGTH, 3);
    assert(attack_damage(10, attacker, target) == 19); // 13 x 1.5 = 19.5
    target.add_status(StatusType::INTANGIBLE, 1);
    assert(attack_damage(10, attacker, target) == 1);
}

void test_damage_dealt_and_taken_percent() {
    CombatEntity attacker, target;
    attacker.modifiers.dealt = 200;
    assert(attack_damage(7, attacker, target) == 14);
    attacker.modifiers.dealt = 150;
    assert(attack_damage(7, attacker, target) == 10); // 10.5
    attacker.modifiers.dealt = 125;
    assert(attack_damage(7, attacker, target) == 8); // 8.75
    attacker.modifiers.dealt = 100;
    target.modifiers.taken = 50;
    assert(attack_damage(7, attacker, target) == 3); // 3.5
    target.modifiers.taken = 150;
    assert(attack_damage(7, attacker, target) == 10);
    // Strength is added before the percent.
    attacker.add_status(StatusType::STRENGTH, 3);
    attacker.modifiers.dealt = 200;
    target.modifiers.taken = 100;
    assert(attack_damage(7, attacker, target) == 20);
    // A percent of 0, or below, deals nothing.
    attacker.modifiers.dealt = 0;
    assert(attack_damage(7, attacker, target) == 0);
    attacker.modifiers.dealt = -50;
    assert(attack_damage(7, attacker, target) == 0);
}

void test_modifiers_multiply_first_and_round_down_once() {
    CombatEntity attacker, target;
    attacker.add_status(StatusType::WEAK, 1);
    target.add_status(StatusType::VULNERABLE, 1);
    attacker.modifiers.vulnerable = 175;
    // 7 x 0.75 x 1.75 = 9.19: rounding after each step would give 5 x 1.75 = 8.
    assert(attack_damage(7, attacker, target) == 9);
    attacker.modifiers.dealt = 200;
    // 7 x 0.75 x 1.75 x 2 = 18.4: doubling the rounded 9 would also give 18, doubling the rounded 8 gives 16.
    assert(attack_damage(7, attacker, target) == 18);
    // 3 x 0.75 x 1.75 x 2 = 7.9.
    assert(attack_damage(3, attacker, target) == 7);
    target.modifiers.taken = 50;
    assert(attack_damage(3, attacker, target) == 3); // 3.9
    // Intangible still caps what is left at 1.
    target.add_status(StatusType::INTANGIBLE, 1);
    assert(attack_damage(30, attacker, target) == 1);
}

void test_weak_and_vulnerable_can_be_made_stronger_or_milder() {
    CombatEntity attacker, target;
    attacker.add_status(StatusType::WEAK, 1);
    attacker.modifiers.weak = 60;
    assert(attack_damage(10, attacker, target) == 6);
    attacker.clear_status(StatusType::WEAK);
    assert(attack_damage(10, attacker, target) == 10); // only while it is Weak
    target.add_status(StatusType::VULNERABLE, 1);
    attacker.modifiers.vulnerable = 125;
    assert(attack_damage(10, attacker, target) == 12);
    attacker.modifiers.vulnerable = 200;
    assert(attack_damage(10, attacker, target) == 20);
    target.clear_status(StatusType::VULNERABLE);
    assert(attack_damage(10, attacker, target) == 10); // only on a Vulnerable target
}

void test_modifiers_tied_to_a_status_of_the_other_side() {
    CombatEntity attacker, target;
    attacker.modifiers.dealt_to_weak = 150;
    assert(attack_damage(10, attacker, target) == 10);
    target.add_status(StatusType::WEAK, 1);
    assert(attack_damage(10, attacker, target) == 15);
    target.clear_status(StatusType::WEAK);

    target.modifiers.taken_from_vulnerable = 50;
    assert(attack_damage(10, attacker, target) == 10);
    attacker.add_status(StatusType::VULNERABLE, 1);
    assert(attack_damage(10, attacker, target) == 5);
}

void test_block_percent() {
    CombatEntity entity;
    entity.gain_block(5, 200);
    assert(entity.block == 10);
    entity.block = 0;
    entity.gain_block(5, 400);
    assert(entity.block == 20);
    // Dexterity first, then Frail and the percent together, rounded down once: 2 x 0.75 x 2 = 3, where
    // rounding 1.5 down first would give 2.
    entity.block = 0;
    entity.add_status(StatusType::DEXTERITY, 1);
    entity.add_status(StatusType::FRAIL, 1);
    entity.gain_block(1, 200);
    assert(entity.block == 3);
    // Without a percent, Frail still takes a quarter away.
    entity.block = 0;
    entity.gain_block(6);
    assert(entity.block == 5); // 7 x 0.75 = 5.25
    // A negative total gives nothing, whatever the percent.
    entity.block = 0;
    entity.add_status(StatusType::DEXTERITY, -9);
    entity.gain_block(5, 200);
    assert(entity.block == 0);
}

void test_powers_set_the_modifiers_and_take_them_back() {
    CombatEngine combat = make_fight(CharacterId::SILENT);
    assert(combat.player.modifiers.dealt == 100 && combat.player.modifiers.vulnerable == 150);
    combat.add_power(PowerId::DOUBLE_DAMAGE, 1, true);
    combat.add_power(PowerId::CRUELTY, 25);
    combat.add_power(PowerId::TRACKING, 50);
    combat.add_power(PowerId::COLOSSUS, 50);
    assert(combat.player.modifiers.dealt == 200 && combat.player.modifiers.vulnerable == 175);
    assert(combat.player.modifiers.dealt_to_weak == 150 && combat.player.modifiers.taken_from_vulnerable == 50);
    // The end of the turn takes the turn-only one, the next turn takes Colossus, the others stay.
    end_turn(combat);
    assert(combat.player.modifiers.dealt == 100 && combat.player.modifiers.taken_from_vulnerable == 100);
    assert(combat.player.modifiers.vulnerable == 175 && combat.player.modifiers.dealt_to_weak == 150);
    // A new combat starts clean.
    combat.reset_combat_state();
    assert(combat.player.modifiers.vulnerable == 150 && combat.player.modifiers.dealt_to_weak == 100);
}

// --- #79: transformed cards ----------------------------------------------------------------------------------

void test_transform_into_a_given_card() {
    CombatEngine combat = make_fight();
    combat.add_power(PowerId::ARSENAL, 1); // would react to a card made
    DeckCard strike(CardId::STRIKE, true);
    strike.deck_index = 4;
    strike.bonus = 3;
    strike.cost_delta = -1;
    strike.added_keywords = KEYWORD_RETAIN;
    strike.enchantment = Enchantment::INKY;
    combat.hand.push_back(strike);
    combat.transform_card(combat.hand[0], DeckCard(CardId::GIANT_ROCK));
    const DeckCard& rock = combat.hand[0];
    assert(rock.id == CardId::GIANT_ROCK && !rock.upgraded);
    // Nothing of the old card is kept, and the card of the deck is out of reach.
    assert(rock.bonus == 0 && rock.cost_delta == 0 && rock.added_keywords == 0);
    assert(rock.enchantment == Enchantment::NONE && rock.deck_index == NOT_IN_DECK);
    // A transformed card is not a card made.
    assert(combat.counters.combat_cards_created == 0 && combat.player.get_status(StatusType::STRENGTH) == 0);
    assert(combat.hand.size() == 1);

    // Wherever the card is, and into an upgraded card when asked.
    combat.draw_pile.push_back(CardId::DEFEND);
    combat.discard_pile.push_back(CardId::BASH);
    combat.transform_card(combat.draw_pile[0], DeckCard(CardId::FUEL, true));
    combat.transform_card(combat.discard_pile[0], DeckCard(CardId::SHIV));
    assert(combat.draw_pile[0].id == CardId::FUEL && combat.draw_pile[0].upgraded);
    assert(combat.discard_pile[0].id == CardId::SHIV);
}

void test_transform_into_a_random_card() {
    CombatEngine combat = make_fight();
    for (int round = 0; round < 100; ++round) {
        combat.hand.clear();
        DeckCard anger(CardId::ANGER, true);
        anger.deck_index = 2;
        combat.hand.push_back(anger);
        assert(combat.transform_card_random(combat.hand[0]));
        const DeckCard& made = combat.hand[0];
        assert(made.id != CardId::ANGER && in_pool(CharacterId::IRONCLAD, made.id));
        assert(!made.upgraded && made.deck_index == NOT_IN_DECK);
    }
    // No other card to become: the card stays as it is.
    static constexpr CardId ONE[] = { CardId::ANGER };
    combat.card_pool = ONE;
    combat.hand.clear();
    combat.hand.push_back(DeckCard(CardId::ANGER, true));
    assert(!combat.transform_card_random(combat.hand[0]));
    assert(combat.hand[0].id == CardId::ANGER && combat.hand[0].upgraded);
    combat.card_pool = {};
    assert(!combat.transform_card_random(combat.hand[0]));
}

void test_transformation_lasts_the_combat_and_spares_the_deck() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    const size_t deck_size = run.master_deck.size();
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    CombatEngine& combat = run.combat;
    for (Enemy& foe : combat.enemies) foe.hp = foe.max_hp = 200;
    int strikes = count_of(combat.hand, CardId::STRIKE);
    assert(strikes > 0);
    combat.hand.push_back(CardId::PRIMAL_FORCE);
    assert(combat.play_card(static_cast<int>(combat.hand.size()) - 1, 0));
    assert(count_of(combat.hand, CardId::STRIKE) == 0 && count_of(combat.hand, CardId::GIANT_ROCK) >= strikes);
    // The rocks go through the piles like any card: played, discarded, shuffled back.
    int rocks = count_of(combat.hand, CardId::GIANT_ROCK);
    for (int turn = 0; turn < 4; ++turn) {
        for (Enemy& foe : combat.enemies) {
            foe.current_intent = EnemyIntent();
            foe.current_intent.type = EnemyIntentType::BUFF;
        }
        combat.end_player_turn();
    }
    assert(count_of(combat.hand, CardId::GIANT_ROCK) + count_of(combat.draw_pile, CardId::GIANT_ROCK) +
           count_of(combat.discard_pile, CardId::GIANT_ROCK) == rocks);
    // The deck of the run is untouched.
    assert(run.master_deck.size() == deck_size);
    assert(count_of(run.master_deck, CardId::GIANT_ROCK) == 0 && count_of(run.master_deck, CardId::STRIKE) == 5);
}

void test_compact_transforms() {
    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_fight(CharacterId::DEFECT);
        DeckCard wound(CardId::WOUND);
        wound.deck_index = 1;
        combat.hand.push_back(wound);
        combat.hand.push_back(CardId::STRIKE);
        assert(block_of(combat, DeckCard(CardId::COMPACT, upgraded)) == (upgraded ? 7 : 6));
        assert(combat.hand[0].id == CardId::FUEL && combat.hand[0].upgraded == upgraded);
        assert(combat.hand[0].deck_index == NOT_IN_DECK && combat.hand[1].id == CardId::STRIKE);
        assert(combat.counters.combat_cards_created == 0);
    }
}

// --- Ironclad ------------------------------------------------------------------------------------------------

void test_infernal_blade() {
    expect_costs(CardId::INFERNAL_BLADE, 1, 0);
    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_fight();
        play(combat, DeckCard(CardId::INFERNAL_BLADE, upgraded));
        assert(combat.player.energy == combat.player.max_energy - (upgraded ? 0 : 1));
        assert(combat.hand.size() == 1 && CardDef::get(combat.hand[0].id).type == CardType::ATTACK);
        assert(in_pool(CharacterId::IRONCLAD, combat.hand[0].id) && !combat.hand[0].upgraded);
        assert(combat.hand[0].free_this_turn && combat.counters.combat_cards_created == 1);
        assert(count_of(combat.exhaust_pile, CardId::INFERNAL_BLADE) == 1);
    }
    // The Attack is free this turn only.
    CombatEngine combat = make_fight();
    static constexpr CardId ONE[] = { CardId::BLUDGEON, CardId::INFLAME };
    combat.card_pool = ONE;
    play(combat, CardId::INFERNAL_BLADE);
    assert(combat.hand[0].id == CardId::BLUDGEON && combat.effective_cost(combat.hand[0]) == 0);
    combat.player.energy = 0;
    assert(combat.play_card(0, 0) && combat.enemies[0].hp == 200 - 32);
    assert(!combat.discard_pile[0].free_this_turn);
}

void test_stoke() {
    expect_costs(CardId::STOKE, 1, 1);
    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_fight();
        combat.hand.push_back(CardId::STRIKE);
        combat.hand.push_back(CardId::DEFEND);
        combat.hand.push_back(CardId::BASH);
        play(combat, DeckCard(CardId::STOKE, upgraded));
        assert(combat.exhaust_pile.size() == 3 && count_of(combat.exhaust_pile, CardId::STOKE) == 0);
        assert(count_of(combat.discard_pile, CardId::STOKE) == 1);
        assert(combat.hand.size() == 3 && combat.counters.combat_cards_created == 3);
        for (const DeckCard& card : combat.hand) {
            assert(in_pool(CharacterId::IRONCLAD, card.id) && card.upgraded == upgraded && !card.free_this_turn);
        }
    }
    // An empty hand makes nothing.
    CombatEngine empty = make_fight();
    play(empty, CardId::STOKE);
    assert(empty.hand.empty() && empty.exhaust_pile.empty());
    // A card an exhaustion draws stays, on top of the cards made.
    CombatEngine embrace = make_fight();
    embrace.add_power(PowerId::DARK_EMBRACE, 1);
    embrace.draw_pile.push_back(CardId::DEFEND);
    embrace.hand.push_back(CardId::STRIKE);
    play(embrace, CardId::STOKE);
    assert(embrace.exhaust_pile.size() == 1 && embrace.hand.size() == 2 && embrace.hand[0].id == CardId::DEFEND);
}

void test_primal_force_and_giant_rock() {
    expect_costs(CardId::PRIMAL_FORCE, 0, 0);
    expect_costs(CardId::GIANT_ROCK, 1, 1);
    assert(!in_pool(CharacterId::IRONCLAD, CardId::GIANT_ROCK)); // a token
    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_fight();
        combat.hand.push_back(DeckCard(CardId::STRIKE, true));
        combat.hand.push_back(CardId::DEFEND);
        combat.hand.push_back(CardId::BASH);
        combat.hand.push_back(CardId::INFLAME);
        play(combat, DeckCard(CardId::PRIMAL_FORCE, upgraded));
        assert(combat.hand.size() == 4 && count_of(combat.hand, CardId::GIANT_ROCK) == 2);
        assert(combat.hand[0] == DeckCard(CardId::GIANT_ROCK, upgraded));
        assert(combat.hand[2] == DeckCard(CardId::GIANT_ROCK, upgraded));
        assert(combat.hand[1].id == CardId::DEFEND && combat.hand[3].id == CardId::INFLAME);
        assert(combat.counters.combat_cards_created == 0);
        assert(count_of(combat.discard_pile, CardId::PRIMAL_FORCE) == 1);
    }
    CombatEngine combat = make_fight();
    assert(damage_of(combat, DeckCard(CardId::GIANT_ROCK)) == 20);
    assert(damage_of(combat, DeckCard(CardId::GIANT_ROCK, true)) == 25);
    assert(combat.player.energy == combat.player.max_energy - 1);
    assert(count_of(combat.discard_pile, CardId::GIANT_ROCK) == 2);
}

void test_colossus() {
    expect_costs(CardId::COLOSSUS, 1, 1);
    CombatEngine combat = make_fight(CharacterId::IRONCLAD, 2);
    assert(block_of(combat, DeckCard(CardId::COLOSSUS)) == 4);
    assert(block_of(combat, DeckCard(CardId::COLOSSUS, true)) == 7);
    // A second Colossus adds nothing: still half.
    assert(combat.player.modifiers.taken_from_vulnerable == 50);
    combat.player.block = 0;
    combat.enemies[0].add_status(StatusType::VULNERABLE, 2);
    for (Enemy& foe : combat.enemies) foe.current_intent.base_damage = 11;
    // The intent shown already counts it: half of 11, rounded down, from the Vulnerable enemy only.
    assert(combat.enemies[0].intent_damage(combat.player) == 5);
    assert(combat.enemies[1].intent_damage(combat.player) == 11);
    int16_t hp = combat.player.hp;
    end_turn(combat, 11);
    assert(combat.player.hp == hp - 5 - 11);
    // Over at the start of the next turn.
    assert(!combat.has_power(PowerId::COLOSSUS));
    for (Enemy& foe : combat.enemies) foe.current_intent.base_damage = 11;
    assert(combat.enemies[0].get_status(StatusType::VULNERABLE) == 1);
    assert(combat.enemies[0].intent_damage(combat.player) == 11);
}

void test_cruelty() {
    expect_costs(CardId::CRUELTY, 1, 1);
    CombatEngine combat = make_fight();
    play(combat, CardId::CRUELTY);
    assert(damage_of(combat, CardId::STRIKE) == 6); // not Vulnerable: nothing more
    combat.enemies[0].add_status(StatusType::VULNERABLE, 3);
    assert(damage_of(combat, CardId::STRIKE) == 10); // 6 x 1.75 = 10.5
    CombatEngine upgraded = make_fight();
    play(upgraded, DeckCard(CardId::CRUELTY, true));
    upgraded.enemies[0].add_status(StatusType::VULNERABLE, 3);
    assert(damage_of(upgraded, CardId::STRIKE) == 12); // 6 x 2
    // Two of them add up: 150 + 25 + 50.
    play(upgraded, CardId::CRUELTY);
    assert(damage_of(upgraded, CardId::STRIKE) == 13); // 6 x 2.25 = 13.5
    // Weak on the Ironclad and Cruelty are multiplied before the single rounding: 6 x 0.75 x 1.75 = 7.875.
    combat.player.add_status(StatusType::WEAK, 1);
    assert(damage_of(combat, CardId::STRIKE) == 7);
    // The enemy's own attacks on a Vulnerable player are not changed.
    combat.player.add_status(StatusType::VULNERABLE, 1);
    combat.enemies[0].current_intent.base_damage = 10;
    assert(combat.enemies[0].intent_damage(combat.player) == 15);
}

void test_unmovable() {
    expect_costs(CardId::UNMOVABLE, 2, 1);
    CombatEngine combat = make_fight();
    play(combat, CardId::UNMOVABLE);
    assert(block_of(combat, CardId::DEFEND) == 10);
    assert(block_of(combat, CardId::DEFEND) == 5); // the first time only
    // Block that does not come from a card is neither doubled nor counted.
    end_turn(combat);
    combat.player.block = 0;
    combat.add_block(4);
    assert(combat.player.block == 4);
    assert(block_of(combat, DeckCard(CardId::DEFEND, true)) == 16);
    assert(block_of(combat, CardId::DEFEND) == 5);
    // Dexterity and Frail come first, one rounding: (5 + 2) x 0.75 x 2 = 10.5.
    end_turn(combat);
    combat.player.add_status(StatusType::DEXTERITY, 2);
    combat.player.add_status(StatusType::FRAIL, 2);
    assert(block_of(combat, CardId::DEFEND) == 10);
    // The upgrade only makes it cheaper. Two of them double the first two gains of each turn.
    CombatEngine twice = make_fight();
    play(twice, DeckCard(CardId::UNMOVABLE, true));
    play(twice, CardId::UNMOVABLE);
    assert(block_of(twice, CardId::DEFEND) == 10);
    assert(block_of(twice, CardId::DEFEND) == 10);
    assert(block_of(twice, CardId::DEFEND) == 5);
    // A card that gives no Block does not use it up.
    CombatEngine none = make_fight();
    play(none, CardId::UNMOVABLE);
    none.player.add_status(StatusType::DEXTERITY, -5);
    assert(block_of(none, CardId::DEFEND) == 0);
    none.player.clear_status(StatusType::DEXTERITY);
    assert(block_of(none, CardId::DEFEND) == 10);
}

// --- Silent --------------------------------------------------------------------------------------------------

void test_nightmare() {
    expect_costs(CardId::NIGHTMARE, 3, 2);
    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_fight(CharacterId::SILENT);
        DeckCard slice(CardId::SLICE, true);
        slice.deck_index = 3;
        slice.bonus = 2;
        combat.hand.push_back(CardId::DEFEND);
        combat.hand.push_back(slice);
        play(combat, DeckCard(CardId::NIGHTMARE, upgraded));
        assert(combat.player.energy == combat.player.max_energy - (upgraded ? 2 : 3));
        assert(count_of(combat.exhaust_pile, CardId::NIGHTMARE) == 1);
        // The card is picked in the hand, and stays there.
        assert(combat.choice_pending() && combat.pending_choices[0].kind == ChoiceKind::COPY_NEXT_TURN);
        assert(combat.choice_size() == 1 && combat.choice_allows(0) && combat.choice_allows(1));
        const uint8_t pick[] = { 1 };
        assert(combat.resolve_choice(pick));
        assert(!combat.choice_pending() && combat.hand.size() == 2 && combat.counters.combat_cards_created == 0);
        combat.retain_hand = true;
        end_turn(combat);
        // Three copies next turn, as they were, and out of reach of the deck.
        assert(combat.hand.size() == 5 && count_of(combat.hand, CardId::SLICE) == 4);
        assert(combat.counters.combat_cards_created == 3);
        for (size_t i = 2; i < 5; ++i) {
            assert(combat.hand[i].id == CardId::SLICE && combat.hand[i].upgraded && combat.hand[i].bonus == 2);
            assert(combat.hand[i].deck_index == NOT_IN_DECK);
        }
        assert(combat.hand[1].deck_index == 3);
        // Once only.
        end_turn(combat);
        assert(count_of(combat.hand, CardId::SLICE) + count_of(combat.discard_pile, CardId::SLICE) == 4);
    }
    // A single card in hand is picked without asking, an empty hand copies nothing.
    CombatEngine single = make_fight(CharacterId::SILENT);
    single.hand.push_back(CardId::STRIKE);
    play(single, CardId::NIGHTMARE);
    assert(!single.choice_pending() && single.pending_copies.size() == 1);
    end_turn(single);
    assert(count_of(single.hand, CardId::STRIKE) + count_of(single.discard_pile, CardId::STRIKE) == 4);
    CombatEngine empty = make_fight(CharacterId::SILENT);
    play(empty, CardId::NIGHTMARE);
    assert(!empty.choice_pending() && empty.pending_copies.empty());
    // The copies come before the draw: a full hand sends the rest to the discard pile.
    CombatEngine full = make_fight(CharacterId::SILENT);
    for (int i = 0; i < 8; ++i) full.hand.push_back(CardId::DEFEND);
    full.hand.push_back(CardId::STRIKE);
    full.hand.push_back(CardId::NIGHTMARE);
    full.player.energy = 3;
    assert(full.play_card(9, 0));
    const uint8_t strike[] = { 8 };
    assert(full.resolve_choice(strike));
    full.retain_hand = true;
    end_turn(full);
    assert(full.hand.size() == 10 && count_of(full.hand, CardId::STRIKE) == 2);
    assert(count_of(full.discard_pile, CardId::STRIKE) == 2);
}

void test_shadow_step() {
    expect_costs(CardId::SHADOW_STEP, 1, 0);
    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_fight(CharacterId::SILENT);
        combat.hand.push_back(CardId::DEFEND);
        combat.hand.push_back(CardId::SLICE);
        play(combat, DeckCard(CardId::SHADOW_STEP, upgraded));
        assert(combat.player.energy == combat.player.max_energy - (upgraded ? 0 : 1));
        assert(combat.hand.empty() && combat.discard_pile.size() == 3 && combat.counters.cards_discarded == 2);
        // Not this turn.
        assert(damage_of(combat, CardId::STRIKE) == 6);
        end_turn(combat);
        assert(combat.has_power(PowerId::DOUBLE_DAMAGE));
        assert(damage_of(combat, CardId::STRIKE) == 12);
        assert(damage_of(combat, DeckCard(CardId::STRIKE, true)) == 18);
        // Strength is doubled with the rest, and Weak is multiplied in before the single rounding.
        combat.player.add_status(StatusType::STRENGTH, 1);
        combat.player.add_status(StatusType::WEAK, 2);
        assert(damage_of(combat, CardId::STRIKE) == 10); // 7 x 0.75 x 2 = 10.5
        combat.player.clear_status(StatusType::STRENGTH);
        combat.player.clear_status(StatusType::WEAK);
        // That turn only.
        end_turn(combat);
        assert(!combat.has_power(PowerId::DOUBLE_DAMAGE));
        assert(damage_of(combat, CardId::STRIKE) == 6);
    }
    // Two Shadow Steps in a turn: the next two turns, never four times the damage.
    CombatEngine twice = make_fight(CharacterId::SILENT);
    play(twice, CardId::SHADOW_STEP);
    play(twice, CardId::SHADOW_STEP);
    end_turn(twice);
    assert(damage_of(twice, CardId::STRIKE) == 12);
    end_turn(twice);
    assert(damage_of(twice, CardId::STRIKE) == 12);
    end_turn(twice);
    assert(damage_of(twice, CardId::STRIKE) == 6);
}

void test_shadowmeld() {
    expect_costs(CardId::SHADOWMELD, 1, 0);
    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_fight(CharacterId::SILENT);
        play(combat, DeckCard(CardId::SHADOWMELD, upgraded));
        assert(combat.player.energy == combat.player.max_energy - (upgraded ? 0 : 1));
        // Every Block gained this turn, from a card or not.
        assert(block_of(combat, CardId::DEFEND) == 10);
        assert(block_of(combat, DeckCard(CardId::DEFEND, true)) == 16);
        combat.add_block(3);
        assert(combat.player.block == 22);
        // Dexterity and Frail first, one rounding: (5 + 2) x 0.75 x 2 = 10.5.
        combat.player.add_status(StatusType::DEXTERITY, 2);
        combat.player.add_status(StatusType::FRAIL, 2);
        assert(block_of(combat, CardId::DEFEND) == 10);
        combat.player.clear_status(StatusType::DEXTERITY);
        combat.player.clear_status(StatusType::FRAIL);
        // This turn only.
        end_turn(combat);
        assert(!combat.has_power(PowerId::SHADOWMELD));
        assert(block_of(combat, CardId::DEFEND) == 5);
    }
    // Two of them: four times.
    CombatEngine twice = make_fight(CharacterId::SILENT);
    play(twice, CardId::SHADOWMELD);
    play(twice, CardId::SHADOWMELD);
    assert(block_of(twice, CardId::DEFEND) == 20);
    // What the reactions hear is the Block really gained (Juggernaut does not care, the event amount does).
    CombatEngine heard = make_fight(CharacterId::SILENT);
    play(heard, CardId::SHADOWMELD);
    heard.add_power(PowerId::JUGGERNAUT, 5);
    int16_t hp = heard.enemies[0].hp;
    play(heard, CardId::DEFEND);
    assert(heard.enemies[0].hp == hp - 5);
}

void test_tracking() {
    expect_costs(CardId::TRACKING, 2, 1);
    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_fight(CharacterId::SILENT);
        play(combat, DeckCard(CardId::TRACKING, upgraded));
        assert(combat.player.energy == combat.player.max_energy - (upgraded ? 1 : 2));
        assert(damage_of(combat, CardId::STRIKE) == 6); // not Weak: nothing more
        combat.enemies[0].add_status(StatusType::WEAK, 3);
        assert(damage_of(combat, CardId::STRIKE) == 9);
        // With Vulnerable: 7 x 1.5 x 1.5 = 15.75, one rounding (10 x 1.5 = 15 after a first rounding too, so
        // use 5: 5 x 2.25 = 11.25, where rounding 7.5 down first gives 7 x 1.5 = 10).
        combat.enemies[0].add_status(StatusType::VULNERABLE, 3);
        combat.player.add_status(StatusType::STRENGTH, -1);
        assert(damage_of(combat, CardId::STRIKE) == 11);
        combat.player.clear_status(StatusType::STRENGTH);
        // Only Attacks: Poison is not changed.
        combat.enemies[0].add_status(StatusType::POISON, 4);
        int16_t hp = combat.enemies[0].hp;
        end_turn(combat);
        assert(combat.enemies[0].hp == hp - 4);
    }
    // Two of them add up: 100 + 50 + 50.
    CombatEngine twice = make_fight(CharacterId::SILENT);
    play(twice, CardId::TRACKING);
    play(twice, CardId::TRACKING);
    twice.enemies[0].add_status(StatusType::WEAK, 1);
    assert(damage_of(twice, CardId::STRIKE) == 12);
}

// --- Defect --------------------------------------------------------------------------------------------------

void test_white_noise() {
    expect_costs(CardId::WHITE_NOISE, 1, 0);
    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_fight(CharacterId::DEFECT);
        play(combat, DeckCard(CardId::WHITE_NOISE, upgraded));
        assert(combat.player.energy == combat.player.max_energy - (upgraded ? 0 : 1));
        assert(combat.hand.size() == 1 && CardDef::get(combat.hand[0].id).type == CardType::POWER);
        assert(in_pool(CharacterId::DEFECT, combat.hand[0].id) && !combat.hand[0].upgraded);
        assert(combat.effective_cost(combat.hand[0]) == 0 && combat.counters.combat_cards_created == 1);
        assert(count_of(combat.exhaust_pile, CardId::WHITE_NOISE) == 1);
        // Played for nothing, it stays in play.
        combat.player.energy = 0;
        assert(combat.play_card(0, 0) && combat.powers_in_play.size() == 1);
    }
}

void test_creative_ai() {
    expect_costs(CardId::CREATIVE_AI, 3, 2);
    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_fight(CharacterId::DEFECT);
        play(combat, DeckCard(CardId::CREATIVE_AI, upgraded));
        assert(combat.player.energy == combat.player.max_energy - (upgraded ? 2 : 3));
        assert(combat.hand.empty()); // not when it is played
        end_turn(combat);
        assert(combat.hand.size() == 1 && CardDef::get(combat.hand[0].id).type == CardType::POWER);
        assert(in_pool(CharacterId::DEFECT, combat.hand[0].id));
        // It keeps its cost.
        assert(!combat.hand[0].free_this_turn && combat.counters.combat_cards_created == 1);
        // One more each turn (an Ethereal Power left in hand is exhausted).
        combat.retain_hand = true;
        end_turn(combat);
        assert(combat.hand.size() + combat.exhaust_pile.size() == 2 && combat.counters.combat_cards_created == 2);
        assert(CardDef::get(combat.hand.back().id).type == CardType::POWER);
    }
    // Two of them: two Powers each turn.
    CombatEngine twice = make_fight(CharacterId::DEFECT);
    play(twice, CardId::CREATIVE_AI);
    play(twice, CardId::CREATIVE_AI);
    end_turn(twice);
    assert(twice.hand.size() == 2 && twice.counters.combat_cards_created == 2);
}

// --- Pools and definitions -----------------------------------------------------------------------------------

void test_pools() {
    for (CardId id : { CardId::INFERNAL_BLADE, CardId::STOKE, CardId::PRIMAL_FORCE, CardId::COLOSSUS, CardId::CRUELTY,
                       CardId::UNMOVABLE }) {
        assert(in_pool(CharacterId::IRONCLAD, id));
        assert(!in_pool(CharacterId::SILENT, id) && !in_pool(CharacterId::DEFECT, id));
    }
    for (CardId id : { CardId::NIGHTMARE, CardId::SHADOW_STEP, CardId::SHADOWMELD, CardId::TRACKING }) {
        assert(in_pool(CharacterId::SILENT, id));
        assert(!in_pool(CharacterId::IRONCLAD, id) && !in_pool(CharacterId::DEFECT, id));
    }
    for (CardId id : { CardId::WHITE_NOISE, CardId::CREATIVE_AI }) {
        assert(in_pool(CharacterId::DEFECT, id));
        assert(!in_pool(CharacterId::IRONCLAD, id) && !in_pool(CharacterId::SILENT, id));
    }
    for (CharacterId hero : { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT, CharacterId::REGENT,
                              CharacterId::NECROBINDER }) {
        assert(!in_pool(hero, CardId::GIANT_ROCK));
    }
    // Keywords and types of bible/data.
    for (bool upgraded : { false, true }) {
        assert(CardDef::has_keyword(CardId::INFERNAL_BLADE, upgraded, KEYWORD_EXHAUST));
        assert(CardDef::has_keyword(CardId::NIGHTMARE, upgraded, KEYWORD_EXHAUST));
        assert(CardDef::has_keyword(CardId::WHITE_NOISE, upgraded, KEYWORD_EXHAUST));
        assert(!CardDef::has_keyword(CardId::STOKE, upgraded, KEYWORD_EXHAUST));
        assert(!CardDef::has_keyword(CardId::GIANT_ROCK, upgraded, KEYWORD_EXHAUST));
    }
    assert(CardDef::get(CardId::CRUELTY).type == CardType::POWER);
    assert(CardDef::get(CardId::UNMOVABLE).type == CardType::POWER);
    assert(CardDef::get(CardId::TRACKING).type == CardType::POWER);
    assert(CardDef::get(CardId::CREATIVE_AI).type == CardType::POWER);
    assert(CardDef::get(CardId::GIANT_ROCK).type == CardType::ATTACK);
    assert(CardDef::get(CardId::COLOSSUS).type == CardType::SKILL);
}

} // namespace

int main() {
    test_random_card_needs_a_pool();
    test_random_card_follows_the_filter_and_the_pool();
    test_random_card_can_leave_one_out();
    test_random_card_is_a_card_made();
    test_random_card_overflows_to_the_discard_pile();
    test_free_card_costs_again_next_turn();
    test_run_gives_the_pool_of_the_hero();
    test_attack_damage_is_unchanged_without_modifiers();
    test_damage_dealt_and_taken_percent();
    test_modifiers_multiply_first_and_round_down_once();
    test_weak_and_vulnerable_can_be_made_stronger_or_milder();
    test_modifiers_tied_to_a_status_of_the_other_side();
    test_block_percent();
    test_powers_set_the_modifiers_and_take_them_back();
    test_transform_into_a_given_card();
    test_transform_into_a_random_card();
    test_transformation_lasts_the_combat_and_spares_the_deck();
    test_compact_transforms();
    test_infernal_blade();
    test_stoke();
    test_primal_force_and_giant_rock();
    test_colossus();
    test_cruelty();
    test_unmovable();
    test_nightmare();
    test_shadow_step();
    test_shadowmeld();
    test_tracking();
    test_white_noise();
    test_creative_ai();
    test_pools();
    std::cout << "card_creation_test: all tests passed" << std::endl;
    return 0;
}
