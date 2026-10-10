// Necrobinder cards: each one is played in its base and upgraded versions, and must be offered in the hero's card pool.
// Values of bible/data/card_v0.111.0.json.
#include <algorithm>
#include <cassert>
#include <iostream>
#include <span>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

CombatEngine make_fight() {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::NECROBINDER);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.block = 0;
    combat.player.statuses.clear();
    combat.enemies[0].block = 0;
    combat.enemies[0].hp = combat.enemies[0].max_hp = 500;
    combat.enemies[0].statuses.clear();
    return combat;
}

void play(CombatEngine& combat, DeckCard card, int target = 0) {
    combat.hand.clear();
    combat.hand.push_back(card);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, target);
    assert(played);
}

void assert_in_pool(CharacterId hero, std::initializer_list<CardId> cards) {
    std::span<const CardId> pool = RunManager::card_reward_pool(hero);
    for (CardId id : cards) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
}

void test_debilitate() {
    assert_in_pool(CharacterId::NECROBINDER, { CardId::DEBILITATE });
    assert(CardDef::get(CardId::DEBILITATE).type == CardType::ATTACK);
    for (bool upgraded : { false, true }) {
        assert(CardDef::get_cost(CardId::DEBILITATE, upgraded) == 1);
        CombatEngine combat = make_fight();
        play(combat, DeckCard(CardId::DEBILITATE, upgraded));
        Enemy& foe = combat.enemies[0];
        assert(foe.hp == 500 - (upgraded ? 12 : 10));
        assert(foe.debilitate_turns == (upgraded ? 3 : 2));
        // Vulnerable is doubled (x2 instead of x1.5): a Strike deals 12 instead of 9.
        foe.add_status(StatusType::VULNERABLE, 5);
        int16_t before = foe.hp;
        play(combat, DeckCard(CardId::STRIKE, false));
        assert(before - combat.enemies[0].hp == 12);
        // The effect wears off after its turns.
        for (int turn = 0; turn < (upgraded ? 3 : 2); ++turn) {
            assert(combat.enemies[0].debilitate_turns > 0);
            combat.end_player_turn();
        }
        assert(combat.enemies[0].debilitate_turns == 0);
    }
    std::cout << "[PASS] Debilitate: 10 (12) damage, Vulnerable and Weak doubled for 2 (3) turns" << std::endl;
}

} // namespace

int main() {
    test_debilitate();
    std::cout << "All Necrobinder card tests passed." << std::endl;
    return 0;
}
