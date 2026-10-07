// Cards that are only created in combat (tokens). One test per card, with the values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

// An Ironclad against one Jaw Worm with 200 HP, empty piles and 3 energy.
CombatEngine make_fight() {
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.enemies[0].max_hp = 200;
    combat.enemies[0].hp = 200;
    combat.enemies[0].block = 0;
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 3;
    return combat;
}

void test_giant_rock() {
    CombatEngine combat = make_fight();
    assert(CardDef::get_cost(CardId::GIANT_ROCK, false) == 1);
    assert(CardDef::get(CardId::GIANT_ROCK).type == CardType::ATTACK);
    combat.hand.push_back(DeckCard(CardId::GIANT_ROCK));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 180);
    assert(combat.player.energy == 2);
    assert(combat.hand.empty() && combat.discard_pile.size() == 1);
    std::cout << "[PASS] Giant Rock: 1 energy, 20 damage" << std::endl;
}

void test_luminesce() {
    CombatEngine combat = make_fight();
    assert(CardDef::get_cost(CardId::LUMINESCE, false) == 0);
    assert(CardDef::get(CardId::LUMINESCE).type == CardType::SKILL);
    combat.hand.push_back(DeckCard(CardId::LUMINESCE));
    assert(combat.play_card(0, 0));
    assert(combat.player.energy == 5);
    assert(combat.hand.empty() && combat.exhaust_pile.size() == 1);
    assert(CardDef::get(CardId::LUMINESCE).keywords & KEYWORD_RETAIN);
    std::cout << "[PASS] Luminesce: 0 energy, +2 energy, Retain, Exhaust" << std::endl;
}

} // namespace

int main() {
    test_giant_rock();
    test_luminesce();
    return 0;
}
