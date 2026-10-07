// Brightest Flame (#413): gain energy, draw cards, lose 2 Max HP. Values of bible/data/card_v0.111.0.json.
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
    combat.player.energy = 3;
    return combat;
}

void check(bool upgraded, int energy, int draws) {
    assert(CardDef::get_cost(CardId::BRIGHTEST_FLAME, upgraded) == 0);
    CombatEngine combat = make_fight();
    for (int i = 0; i < 5; ++i) combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::BRIGHTEST_FLAME, upgraded));
    const int16_t max_hp = combat.player.max_hp;
    assert(combat.play_card(0, 0));
    assert(combat.player.energy == 3 + energy);
    assert(static_cast<int>(combat.hand.size()) == draws);
    assert(combat.player.max_hp == max_hp - 2);
    assert(combat.player.hp <= combat.player.max_hp);
}

} // namespace

int main() {
    assert(CardDef::get(CardId::BRIGHTEST_FLAME).type == CardType::SKILL);
    check(false, 2, 2);
    check(true, 3, 3);
    std::cout << "[PASS] Brightest Flame: 2/3 energy, 2/3 cards, -2 Max HP" << std::endl;
    return 0;
}
