// Whistle (event card): values of bible/data/card_v0.111.0.json.
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
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 3;
    combat.enemies[0].max_hp = 200;
    combat.enemies[0].hp = 200;
    combat.enemies[0].block = 0;
    return combat;
}

void test_whistle() {
    assert(CardDef::get(CardId::WHISTLE).type == CardType::ATTACK);
    assert(CardDef::get_cost(CardId::WHISTLE, false) == 2);
    assert(CardDef::get_cost(CardId::WHISTLE, true) == 2);
    assert(CardDef::has_keyword(CardId::WHISTLE, false, KEYWORD_EXHAUST));
    assert(CardDef::has_keyword(CardId::WHISTLE, true, KEYWORD_EXHAUST));

    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::WHISTLE));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 200 - 33);
    assert(combat.player.energy == 1);
    assert(combat.enemies[0].current_intent.type == EnemyIntentType::STUN);
    assert(combat.exhaust_pile.size() == 1);

    CombatEngine up = make_fight();
    up.hand.push_back(DeckCard(CardId::WHISTLE, true));
    assert(up.play_card(0, 0));
    assert(up.enemies[0].hp == 200 - 44);
    assert(up.enemies[0].current_intent.type == EnemyIntentType::STUN);
    assert(up.exhaust_pile.size() == 1);
    std::cout << "[PASS] Whistle: 2 energy, 33/44 damage, stun, Exhaust" << std::endl;
}

} // namespace

int main() {
    test_whistle();
    return 0;
}
