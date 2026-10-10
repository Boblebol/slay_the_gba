// Beat Down (#365), Catastrophe (#361), Nostalgia (#358), Relax (#416), Neow's Fury (#409), Metamorphosis (#406),
// Distraction (#405), Caltrops (#400), Prolong (#384), Salvo (#371), Thinking Ahead (#368), Jack of All Trades (#366).
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include "engine/card_meta.h"
#include "engine/combat.h"

using namespace engine;

namespace {

CombatEngine make_fight() {
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::CULTIST);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.exhaust_pile.clear();
    combat.returning_cards.clear();
    combat.player.energy = 9;
    combat.player.block = 0;
    combat.enemies[0].max_hp = 300;
    combat.enemies[0].hp = 300;
    combat.enemies[0].block = 0;
    for (int i = 0; i < 6; ++i) combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    return combat;
}

bool pick(CombatEngine& combat, std::initializer_list<uint8_t> indices) {
    return combat.resolve_choice(std::span<const uint8_t>(indices.begin(), indices.size()));
}

void test_beat_down(bool upgraded, int plays) {
    assert(CardDef::get_cost(CardId::BEAT_DOWN, upgraded) == 3);
    CombatEngine combat = make_fight();
    for (int i = 0; i < 5; ++i) combat.discard_pile.push_back(DeckCard(CardId::STRIKE));
    combat.discard_pile.push_back(DeckCard(CardId::DEFEND));
    combat.hand.push_back(DeckCard(CardId::BEAT_DOWN, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300 - 6 * plays); // only Attacks are played, each once
    assert(combat.player.block == 0);
    assert(combat.discard_pile.size() == 7);
}

void test_catastrophe(bool upgraded, int plays) {
    assert(CardDef::get_cost(CardId::CATASTROPHE, upgraded) == 2);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::CATASTROPHE, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.block == 5 * plays);
    assert(combat.draw_pile.size() == static_cast<size_t>(6 - plays));
    assert(combat.discard_pile.size() == static_cast<size_t>(plays + 1));
}

void test_nostalgia(bool upgraded) {
    assert(CardDef::get_cost(CardId::NOSTALGIA, upgraded) == (upgraded ? 0 : 1));
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::NOSTALGIA, upgraded));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::BASH));
    assert(combat.play_card(0, 0));
    assert(combat.has_power(PowerId::NOSTALGIA));
    assert(combat.play_card(0, 0)); // the first Attack of the turn goes on top of the draw pile
    assert(combat.draw_pile.back() == CardId::STRIKE && combat.draw_pile.size() == 7);
    assert(combat.discard_pile.empty()); // a Power stays in play and does not use up the first play
    assert(combat.play_card(0, 0)); // the second one goes to the discard pile as usual
    assert(combat.draw_pile.size() == 7 && combat.discard_pile.back() == CardId::BASH);
}

void test_relax(bool upgraded, int block, int next) {
    assert(CardDef::get_cost(CardId::RELAX, upgraded) == 3);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::RELAX, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.block == block);
    assert(combat.delayed_effects.size() == 2);
    assert(combat.delayed_effects[0].type == DelayedType::DRAW && combat.delayed_effects[0].amount == next);
    assert(combat.delayed_effects[1].type == DelayedType::ENERGY && combat.delayed_effects[1].amount == next);
    assert(combat.exhaust_pile.size() == 1 && combat.exhaust_pile[0] == CardId::RELAX);
}

void test_neows_fury(bool upgraded, int damage, int cards) {
    assert(CardDef::get_cost(CardId::NEOWS_FURY, upgraded) == 1);
    CombatEngine combat = make_fight();
    combat.discard_pile.push_back(DeckCard(CardId::STRIKE));
    combat.discard_pile.push_back(DeckCard(CardId::DEFEND));
    combat.discard_pile.push_back(DeckCard(CardId::BASH));
    combat.hand.push_back(DeckCard(CardId::NEOWS_FURY, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300 - damage);
    assert(combat.choice_pending());
    if (cards == 2) {
        assert(pick(combat, { 0, 1 }));
    } else {
        assert(pick(combat, { 0, 1, 2 }));
    }
    assert(combat.hand.size() == static_cast<size_t>(cards));
    assert(combat.discard_pile.size() == static_cast<size_t>(3 - cards));
    assert(combat.exhaust_pile.size() == 1);
}

void test_metamorphosis(bool upgraded, int added) {
    assert(CardDef::get_cost(CardId::METAMORPHOSIS, upgraded) == 2);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::METAMORPHOSIS, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.draw_pile.size() == static_cast<size_t>(6 + added));
    int attacks = 0;
    for (const DeckCard& card : combat.draw_pile) {
        if (CardDef::get(card.id).type != CardType::ATTACK) continue;
        ++attacks;
        assert(combat.effective_cost(card) == 0);
    }
    assert(attacks == added);
}

void test_distraction(bool upgraded) {
    assert(CardDef::get_cost(CardId::DISTRACTION, upgraded) == (upgraded ? 0 : 1));
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::DISTRACTION, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.hand.size() == 1);
    assert(CardDef::get(combat.hand[0].id).type == CardType::SKILL);
    // The free copy costs nothing, except an X-cost Skill, which stays X.
    assert(combat.effective_cost(combat.hand[0]) == 0 || combat.effective_cost(combat.hand[0]) == COST_X);
    assert(combat.exhaust_pile.size() == 1);
}

void test_caltrops(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::CALTROPS, upgraded) == 1);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::CALTROPS, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.get_status(StatusType::THORNS) == damage);
}

void test_prolong(bool upgraded) {
    assert(CardDef::get_cost(CardId::PROLONG, upgraded) == 0);
    CombatEngine combat = make_fight();
    combat.player.block = 10;
    combat.hand.push_back(DeckCard(CardId::PROLONG, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.delayed_effects.size() == 1);
    assert(combat.delayed_effects[0].type == DelayedType::BLOCK && combat.delayed_effects[0].amount == 10);
    assert(combat.exhaust_pile.size() == (upgraded ? 0u : 1u));
}

void test_salvo(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::SALVO, upgraded) == 1);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::SALVO, upgraded));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    assert(!combat.retain_hand);
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300 - damage);
    assert(combat.retain_hand);
}

void test_thinking_ahead(bool upgraded) {
    assert(CardDef::get_cost(CardId::THINKING_AHEAD, upgraded) == 0);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::THINKING_AHEAD, upgraded));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    assert(combat.play_card(0, 0));
    assert(combat.hand.size() == 3 && combat.choice_pending());
    assert(pick(combat, { 0 })); // the Strike
    assert(combat.hand.size() == 2);
    assert(combat.draw_pile.size() == 5 && combat.draw_pile.back() == CardId::STRIKE);
    assert(combat.exhaust_pile.size() == (upgraded ? 0u : 1u));
}

void test_jack_of_all_trades(bool upgraded, int added) {
    assert(CardDef::get_cost(CardId::JACK_OF_ALL_TRADES, upgraded) == 0);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::JACK_OF_ALL_TRADES, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.hand.size() == static_cast<size_t>(added));
    for (const DeckCard& card : combat.hand) assert(card_meta(card.id).pool == CardPool::COLORLESS);
    assert(combat.exhaust_pile.size() == 1);
}

} // namespace

int main() {
    for (bool up : { false, true }) {
        test_beat_down(up, up ? 4 : 3);
        test_catastrophe(up, up ? 3 : 2);
        test_nostalgia(up);
        test_relax(up, up ? 18 : 16, up ? 3 : 2);
        test_neows_fury(up, up ? 14 : 10, up ? 3 : 2);
        test_metamorphosis(up, up ? 5 : 3);
        test_distraction(up);
        test_caltrops(up, up ? 5 : 3);
        test_prolong(up);
        test_salvo(up, up ? 16 : 12);
        test_thinking_ahead(up);
        test_jack_of_all_trades(up, up ? 2 : 1);
    }
    std::cout << "[PASS] Cards batch H: Beat Down, Catastrophe, Nostalgia, Relax, Neow's Fury, Metamorphosis, "
                 "Distraction, Caltrops, Prolong, Salvo, Thinking Ahead, Jack of All Trades"
              << std::endl;
    return 0;
}
