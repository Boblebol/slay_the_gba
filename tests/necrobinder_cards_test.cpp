// Necrobinder cards added on top of the existing mechanics: each one is played in its base and upgraded versions,
// and must be offered in the hero's card pool.
#include <algorithm>
#include <cassert>
#include <iostream>
#include <span>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

CombatEngine fight() {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::NECROBINDER);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 9;
    combat.player.block = 0;
    combat.player.statuses.clear();
    combat.enemies[0].block = 0;
    combat.enemies[0].hp = combat.enemies[0].max_hp = 500;
    combat.enemies[0].statuses.clear();
    return combat;
}

void play(CombatEngine& combat, CardId id, bool up = false) {
    combat.hand.clear();
    combat.hand.push_back(DeckCard(id, up));
    bool played = combat.play_card(0, 0);
    assert(played);
}

int16_t damage_of(CombatEngine& combat, CardId id) {
    int16_t before = combat.enemies[0].hp;
    play(combat, id);
    return static_cast<int16_t>(before - combat.enemies[0].hp);
}

void assert_in_pool(CharacterId hero, std::initializer_list<CardId> cards) {
    std::span<const CardId> pool = RunManager::card_reward_pool(hero);
    for (CardId id : cards) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
}

void test_lethality() {
    assert(CardDef::get(CardId::LETHALITY).type == CardType::POWER);
    assert(CardDef::get_cost(CardId::LETHALITY, false) == 1 && CardDef::get_cost(CardId::LETHALITY, true) == 1);
    assert(CardDef::get_value(CardId::LETHALITY, false) == 50 && CardDef::get_value(CardId::LETHALITY, true) == 75);
    assert(CardDef::has_keyword(CardId::LETHALITY, false, KEYWORD_ETHEREAL));
    assert(CardDef::has_keyword(CardId::LETHALITY, true, KEYWORD_ETHEREAL));
    for (bool up : { false, true }) {
        CombatEngine combat = fight();
        play(combat, CardId::LETHALITY, up);
        assert(combat.power_amount(PowerId::LETHALITY) == (up ? 75 : 50));
        int16_t first = damage_of(combat, CardId::STRIKE);
        int16_t second = damage_of(combat, CardId::STRIKE);
        assert(second == 6);
        assert(first == (up ? 10 : 9));
    }
    assert_in_pool(CharacterId::NECROBINDER, { CardId::LETHALITY });
    std::cout << "[PASS] Necrobinder: Lethality" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Necrobinder Cards ===" << std::endl;
    test_lethality();
    std::cout << "=== All Necrobinder Cards Passed! ===" << std::endl;
    return 0;
}
