// Transform a card for the combat (#79): Primal Force, Seance and Entropy. Values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include <span>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

bool in_pool(CharacterId hero, CardId id) {
    for (CardId pool_card : RunManager::card_reward_pool(hero)) {
        if (pool_card == id) return true;
    }
    return false;
}

CombatEngine fight(CharacterId hero) {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, hero);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 9;
    return combat;
}

void test_primal_force() {
    assert(in_pool(CharacterId::IRONCLAD, CardId::PRIMAL_FORCE));
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::PRIMAL_FORCE, up) == 0);
        CombatEngine combat = fight(CharacterId::IRONCLAD);
        combat.hand.push_back(DeckCard(CardId::PRIMAL_FORCE, up));
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.hand.push_back(DeckCard(CardId::DEFEND, false));
        combat.hand.push_back(DeckCard(CardId::BASH, false));
        assert(combat.play_card(0, 0));
        // The Attacks become Giant Rock (upgraded when Primal Force is), the Skill stays.
        assert(combat.hand.size() == 3);
        assert(combat.hand[0].id == CardId::GIANT_ROCK && combat.hand[0].upgraded == up);
        assert(combat.hand[1].id == CardId::DEFEND);
        assert(combat.hand[2].id == CardId::GIANT_ROCK && combat.hand[2].upgraded == up);
    }
    std::cout << "[PASS] Primal Force: 0 energy, every Attack of the hand becomes Giant Rock (upgraded if upgraded)" << std::endl;
}

void test_seance() {
    assert(in_pool(CharacterId::NECROBINDER, CardId::SEANCE));
    assert(CardDef::has_keyword(CardId::SEANCE, false, KEYWORD_ETHEREAL));
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::SEANCE, up) == (up ? 0 : 1));
        CombatEngine combat = fight(CharacterId::NECROBINDER);
        combat.draw_pile.push_back(DeckCard(CardId::STRIKE, false));
        combat.draw_pile.push_back(DeckCard(CardId::DEFEND, false));
        combat.draw_pile.push_back(DeckCard(CardId::BASH, false));
        combat.hand.push_back(DeckCard(CardId::SEANCE, up));
        assert(combat.play_card(0, 0));
        assert(combat.choice_pending());
        const uint8_t pick[] = { 1 };
        assert(combat.resolve_choice(std::span<const uint8_t>(pick, 1)));
        int souls = 0;
        for (size_t i = 0; i < combat.draw_pile.size(); ++i) souls += combat.draw_pile[i].id == SOUL_CARD;
        assert(combat.draw_pile.size() == 3 && souls == 1);
    }
    std::cout << "[PASS] Seance: Ethereal, 1 energy (0 upgraded), one card of the draw pile becomes Soul" << std::endl;
}

void test_entropy() {
    assert(CardDef::get_cost(CardId::ENTROPY, false) == 1);
    assert(CardDef::has_keyword(CardId::ENTROPY, true, KEYWORD_INNATE));
    assert(!CardDef::has_keyword(CardId::ENTROPY, false, KEYWORD_INNATE));
    for (bool up : { false, true }) {
        CombatEngine combat = fight(CharacterId::IRONCLAD);
        combat.hand.push_back(DeckCard(CardId::ENTROPY, up));
        assert(combat.play_card(0, 0));
        assert(combat.power_amount(PowerId::ENTROPY) == 1);
        // The next turn begins with a choice of one card of the hand to transform.
        for (int i = 0; i < 12; ++i) combat.draw_pile.push_back(DeckCard(CardId::DEFEND, false));
        combat.end_player_turn();
        assert(combat.choice_pending());
        const uint8_t pick[] = { 0 };
        assert(combat.resolve_choice(std::span<const uint8_t>(pick, 1)));
        assert(combat.hand.size() > 0 && !combat.choice_pending());
    }
    std::cout << "[PASS] Entropy: at the start of the turn one card of the hand is transformed, Innate once upgraded" << std::endl;
}

} // namespace

int main() {
    test_primal_force();
    test_seance();
    test_entropy();
    std::cout << "All transform tests passed." << std::endl;
    return 0;
}
