// Feeding Frenzy (#417), Outmaneuver (#411), Toric Toughness (#420), Entrench (#423).
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

CombatEngine make_fight() {
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.exhaust_pile.clear();
    combat.delayed_effects.clear();
    combat.player.energy = 3;
    combat.player.block = 0;
    for (int i = 0; i < 6; ++i) combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    return combat;
}

void test_feeding_frenzy(bool upgraded, int strength) {
    assert(CardDef::get_cost(CardId::FEEDING_FRENZY, upgraded) == 0);
    CombatEngine combat = make_fight();
    const int before = combat.player.get_status(StatusType::STRENGTH);
    combat.hand.push_back(DeckCard(CardId::FEEDING_FRENZY, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.get_status(StatusType::STRENGTH) == before + strength);
    assert(combat.player.energy == 3);
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::STRENGTH) == before);
}

void test_outmaneuver(bool upgraded, int energy) {
    assert(CardDef::get_cost(CardId::OUTMANEUVER, upgraded) == 1);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::OUTMANEUVER, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.energy == 2);
    assert(combat.delayed_effects.size() == 1);
    assert(combat.delayed_effects[0].type == DelayedType::ENERGY);
    assert(combat.delayed_effects[0].amount == energy);
}

void test_toric_toughness(bool upgraded, int block) {
    assert(CardDef::get_cost(CardId::TORIC_TOUGHNESS, upgraded) == 2);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::TORIC_TOUGHNESS, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.block == block);
    assert(combat.delayed_effects.size() == 1);
    assert(combat.delayed_effects[0].type == DelayedType::BLOCK);
    assert(combat.delayed_effects[0].amount == block);
    assert(combat.delayed_effects[0].repeat == 2);
}

void test_entrench(bool upgraded) {
    assert(CardDef::get_cost(CardId::ENTRENCH, upgraded) == 2);
    CombatEngine combat = make_fight();
    combat.player.block = 7;
    combat.hand.push_back(DeckCard(CardId::ENTRENCH, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.block == 14);
    assert(combat.player.energy == 1);
}

} // namespace

int main() {
    test_feeding_frenzy(false, 5);
    test_feeding_frenzy(true, 7);
    test_outmaneuver(false, 2);
    test_outmaneuver(true, 3);
    test_toric_toughness(false, 5);
    test_toric_toughness(true, 7);
    test_entrench(false);
    test_entrench(true);
    std::cout << "[PASS] Cards batch D: Feeding Frenzy, Outmaneuver, Toric Toughness, Entrench" << std::endl;
    return 0;
}
