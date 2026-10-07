// Prowess (colorless), Squash and Exterminate (event): base and upgraded values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

// An Ironclad against `count` Jaw Worms with 200 HP each, empty piles and 3 energy.
CombatEngine make_fight(int count) {
    FixedVector<EnemyType, 3> pack;
    for (int i = 0; i < count; ++i) pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 3;
    for (size_t i = 0; i < combat.enemies.size(); ++i) {
        combat.enemies[i].max_hp = 200;
        combat.enemies[i].hp = 200;
        combat.enemies[i].block = 0;
    }
    return combat;
}

void test_prowess() {
    assert(CardDef::get_cost(CardId::PROWESS, false) == 1);
    assert(CardDef::get_cost(CardId::PROWESS, true) == 1);
    assert(CardDef::get(CardId::PROWESS).type == CardType::POWER);
    for (int up = 0; up < 2; ++up) {
        CombatEngine combat = make_fight(1);
        combat.hand.push_back(DeckCard(CardId::PROWESS, up == 1));
        const int strength = combat.player.get_status(StatusType::STRENGTH);
        const int dexterity = combat.player.get_status(StatusType::DEXTERITY);
        assert(combat.play_card(0, 0));
        assert(combat.player.energy == 2);
        assert(combat.player.get_status(StatusType::STRENGTH) == strength + 1 + up);
        assert(combat.player.get_status(StatusType::DEXTERITY) == dexterity + 1 + up);
    }
    std::cout << "[PASS] Prowess: 1 energy, +1/+2 Strength and +1/+2 Dexterity" << std::endl;
}

void test_squash() {
    assert(CardDef::get(CardId::SQUASH).type == CardType::ATTACK);
    for (int up = 0; up < 2; ++up) {
        CombatEngine combat = make_fight(1);
        combat.hand.push_back(DeckCard(CardId::SQUASH, up == 1));
        assert(CardDef::get_cost(CardId::SQUASH, up == 1) == 1);
        assert(combat.play_card(0, 0));
        assert(combat.player.energy == 2);
        assert(combat.enemies[0].hp == 200 - (10 + 2 * up));
        assert(combat.enemies[0].get_status(StatusType::VULNERABLE) == 2 + up);
    }
    std::cout << "[PASS] Squash: 10/12 damage, Vulnerable 2/3" << std::endl;
}

void test_exterminate() {
    assert(CardDef::get(CardId::EXTERMINATE).type == CardType::ATTACK);
    for (int up = 0; up < 2; ++up) {
        CombatEngine combat = make_fight(2);
        combat.hand.push_back(DeckCard(CardId::EXTERMINATE, up == 1));
        assert(CardDef::get_cost(CardId::EXTERMINATE, up == 1) == 1);
        assert(combat.play_card(0, 0));
        assert(combat.player.energy == 2);
        for (size_t i = 0; i < combat.enemies.size(); ++i) {
            assert(combat.enemies[i].hp == 200 - 4 * (3 + up));
        }
    }
    std::cout << "[PASS] Exterminate: 3/4 damage to ALL enemies 4 times" << std::endl;
}

} // namespace

int main() {
    test_prowess();
    test_squash();
    test_exterminate();
    return 0;
}
