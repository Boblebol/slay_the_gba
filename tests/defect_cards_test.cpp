// Defect cards: each card of the hero is played in its base and upgraded versions, and must be offered in the
// hero's card pool. Expected values come from the CardDef rows of src/engine/card.cpp, which match
// bible/data/card_v0.111.0.json.
#include <algorithm>
#include <cassert>
#include <iostream>
#include <span>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

// A fight against a single Jaw Worm (42 HP), with the orb pile and block cleared for exact numbers.
CombatEngine make_fight() {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::DEFECT);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.orbs.clear();
    combat.player.energy = combat.player.max_energy;
    combat.player.block = 0;
    return combat;
}

void assert_in_pool(CharacterId hero, std::initializer_list<CardId> cards) {
    std::span<const CardId> pool = RunManager::card_reward_pool(hero);
    for (CardId id : cards) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
}

void test_refract() {
    assert_in_pool(CharacterId::DEFECT, { CardId::REFRACT });
    for (bool upgraded : { false, true }) {
        assert(CardDef::get_cost(CardId::REFRACT, upgraded) == 3);
        assert(CardDef::get(CardId::REFRACT).type == CardType::ATTACK);

        CombatEngine combat = make_fight();
        combat.hand.push_back(DeckCard(CardId::REFRACT, upgraded));
        bool played = combat.play_card(0, 0);
        assert(played);
        // Two hits of 10 (13 when upgraded) on the same target.
        assert(combat.enemies[0].hp == 42 - 2 * (upgraded ? 13 : 10));
        // Channels 2 Glass orbs.
        assert(combat.player.orbs.size() == 2);
        assert(combat.player.orbs[0] == OrbType::GLASS && combat.player.orbs[1] == OrbType::GLASS);
    }
    std::cout << "[PASS] Refract: 10 (13) damage twice and 2 Glass orbs, in the Defect pool" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Defect Cards ===" << std::endl;
    test_refract();
    std::cout << "=== All Defect Cards Passed! ===" << std::endl;
    return 0;
}
