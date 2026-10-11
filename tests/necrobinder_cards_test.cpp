// Necrobinder cards: each one is played in its base and upgraded versions, and must be offered in the hero's pool.
#include <algorithm>
#include <cassert>
#include <iostream>
#include <span>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

void assert_in_pool(CharacterId hero, std::initializer_list<CardId> cards) {
    std::span<const CardId> pool = RunManager::card_reward_pool(hero);
    for (CardId id : cards) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
}

void test_forbidden_grimoire() {
    assert_in_pool(CharacterId::NECROBINDER, { CardId::FORBIDDEN_GRIMOIRE });
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::FORBIDDEN_GRIMOIRE, up) == 2);
        assert(CardDef::get(CardId::FORBIDDEN_GRIMOIRE).type == CardType::POWER);
        assert(CardDef::has_keyword(CardId::FORBIDDEN_GRIMOIRE, up, KEYWORD_ETERNAL));

        RunManager run;
        run.start_run(CharacterId::NECROBINDER, 12);
        run.combat.start_combat(EnemyType::JAW_WORM, CharacterId::NECROBINDER);
        run.combat.hand.clear();
        run.combat.hand.push_back(DeckCard(CardId::FORBIDDEN_GRIMOIRE, up));
        run.combat.player.energy = 2;
        assert(run.combat.play_card(0, 0));
        assert(run.combat.player.energy == 0);
        assert(run.combat.power_amount(PowerId::FORBIDDEN_GRIMOIRE) == 1);
        run.combat.state = CombatState::VICTORY;
        run.phase = RunPhase::IN_COMBAT;
        run.proceed_after_combat();
        assert(run.has_deck_choice() && run.deck_choice.kind == DeckChoiceKind::REMOVE && run.deck_choice.optional);
    }
    std::cout << "[PASS] Necrobinder: Forbidden Grimoire" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Necrobinder Cards ===" << std::endl;
    test_forbidden_grimoire();
    std::cout << "=== All Necrobinder Cards Passed! ===" << std::endl;
    return 0;
}
