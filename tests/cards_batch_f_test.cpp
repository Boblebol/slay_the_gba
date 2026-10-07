// Shockwave (#388), Bolas (#378), Thrumming Hatchet (#381), Impatience (#395), Clash (#412), Volley (#356),
// Omnislice (#396), Restlessness (#372).
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

CombatEngine make_fight(int enemy_count = 1) {
    FixedVector<EnemyType, 3> pack;
    for (int i = 0; i < enemy_count; ++i) pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.exhaust_pile.clear();
    combat.returning_cards.clear();
    combat.player.energy = 3;
    combat.player.block = 0;
    for (size_t i = 0; i < combat.enemies.size(); ++i) {
        combat.enemies[i].max_hp = 200;
        combat.enemies[i].hp = 200;
        combat.enemies[i].block = 0;
    }
    for (int i = 0; i < 6; ++i) combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    return combat;
}

void test_shockwave(bool upgraded, int amount) {
    assert(CardDef::get_cost(CardId::SHOCKWAVE, upgraded) == 2);
    CombatEngine combat = make_fight(2);
    combat.hand.push_back(DeckCard(CardId::SHOCKWAVE, upgraded));
    assert(combat.play_card(0, 0));
    for (size_t i = 0; i < 2; ++i) {
        assert(combat.enemies[i].get_status(StatusType::WEAK) == amount);
        assert(combat.enemies[i].get_status(StatusType::VULNERABLE) == amount);
    }
    assert(combat.exhaust_pile.size() == 1);
    assert(combat.player.energy == 1);
}

void test_returning_attack(CardId id, int cost, bool upgraded, int damage) {
    assert(CardDef::get_cost(id, upgraded) == cost);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(id, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 200 - damage);
    assert(combat.returning_cards.size() == 1);
    assert(combat.discard_pile.empty());
}

void test_impatience(bool upgraded, int drawn) {
    assert(CardDef::get_cost(CardId::IMPATIENCE, upgraded) == 0);
    CombatEngine with_attack = make_fight();
    with_attack.hand.push_back(DeckCard(CardId::IMPATIENCE, upgraded));
    with_attack.hand.push_back(DeckCard(CardId::STRIKE));
    assert(with_attack.play_card(0, 0));
    assert(with_attack.hand.size() == 1);

    CombatEngine without = make_fight();
    without.hand.push_back(DeckCard(CardId::IMPATIENCE, upgraded));
    without.hand.push_back(DeckCard(CardId::DEFEND));
    assert(without.play_card(0, 0));
    assert(static_cast<int>(without.hand.size()) == 1 + drawn);
}

void test_clash(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::CLASH, upgraded) == 0);
    CombatEngine blocked = make_fight();
    blocked.hand.push_back(DeckCard(CardId::CLASH, upgraded));
    blocked.hand.push_back(DeckCard(CardId::DEFEND));
    assert(!blocked.can_play_card(0));

    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::CLASH, upgraded));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    assert(combat.can_play_card(0));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 200 - damage);
}

void test_volley(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::VOLLEY, upgraded) == COST_X);
    CombatEngine combat = make_fight(2);
    combat.hand.push_back(DeckCard(CardId::VOLLEY, upgraded));
    assert(combat.play_card(0, 0));
    int total = 0;
    for (size_t i = 0; i < 2; ++i) total += 200 - combat.enemies[i].hp;
    assert(total == 3 * damage);
    assert(combat.player.energy == 0);
}

void test_omnislice(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::OMNISLICE, upgraded) == 0);
    CombatEngine combat = make_fight(3);
    combat.hand.push_back(DeckCard(CardId::OMNISLICE, upgraded));
    assert(combat.play_card(0, 1));
    for (size_t i = 0; i < 3; ++i) assert(combat.enemies[i].hp == 200 - damage);
    assert(combat.player.energy == 3);
}

void test_restlessness(bool upgraded, int amount) {
    assert(CardDef::get_cost(CardId::RESTLESSNESS, upgraded) == 0);
    assert(CardDef::has_keyword(DeckCard(CardId::RESTLESSNESS, upgraded), KEYWORD_RETAIN));
    CombatEngine empty = make_fight();
    empty.hand.push_back(DeckCard(CardId::RESTLESSNESS, upgraded));
    assert(empty.play_card(0, 0));
    assert(static_cast<int>(empty.hand.size()) == amount);
    assert(empty.player.energy == 3 + amount);

    CombatEngine busy = make_fight();
    busy.hand.push_back(DeckCard(CardId::RESTLESSNESS, upgraded));
    busy.hand.push_back(DeckCard(CardId::STRIKE));
    assert(busy.play_card(0, 0));
    assert(busy.hand.size() == 1);
    assert(busy.player.energy == 3);
}

} // namespace

int main() {
    for (bool up : { false, true }) {
        test_shockwave(up, up ? 5 : 3);
        test_returning_attack(CardId::BOLAS, 0, up, up ? 4 : 3);
        test_returning_attack(CardId::THRUMMING_HATCHET, 1, up, up ? 14 : 11);
        test_impatience(up, up ? 3 : 2);
        test_clash(up, up ? 18 : 14);
        test_volley(up, up ? 14 : 10);
        test_omnislice(up, up ? 11 : 8);
        test_restlessness(up, up ? 3 : 2);
    }
    std::cout << "[PASS] Cards batch F: Shockwave, Bolas, Thrumming Hatchet, Impatience, Clash, Volley, Omnislice, Restlessness"
              << std::endl;
    return 0;
}
