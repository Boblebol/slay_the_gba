// Echo N (#86): a copy of a card that is replayed N more times each time it is played. Hidden Gem and Transfigure.
#include <cassert>
#include <iostream>
#include <span>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

CombatEngine fight(CharacterId hero) {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, hero);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 9;
    combat.enemies[0].hp = combat.enemies[0].max_hp = 500;
    return combat;
}

bool in_pool(CharacterId hero, CardId id) {
    for (CardId pool_card : RunManager::card_reward_pool(hero)) {
        if (pool_card == id) return true;
    }
    return false;
}

void test_echo_replays_the_card() {
    for (uint8_t echo : { 1, 2 }) {
        CombatEngine combat = fight(CharacterId::IRONCLAD);
        DeckCard strike(CardId::STRIKE, false);
        strike.echo = echo;
        combat.hand.push_back(strike);
        assert(combat.play_card(0, 0));
        assert(combat.enemies[0].hp == 500 - 6 * (1 + echo));
    }
    // The next play of the same copy is replayed again.
    CombatEngine combat = fight(CharacterId::IRONCLAD);
    DeckCard defend(CardId::DEFEND, false);
    defend.echo = 1;
    combat.hand.push_back(defend);
    assert(combat.play_card(0, 0));
    assert(combat.player.block == 10);
    std::cout << "[PASS] Echo N: the card is played N more times" << std::endl;
}

void test_hidden_gem() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::HIDDEN_GEM, up) == 1);
        CombatEngine combat = fight(CharacterId::IRONCLAD);
        for (int i = 0; i < 4; ++i) combat.draw_pile.push_back(DeckCard(CardId::STRIKE, false));
        combat.hand.push_back(DeckCard(CardId::HIDDEN_GEM, up));
        assert(combat.play_card(0, 0));
        int gems = 0;
        for (size_t i = 0; i < combat.draw_pile.size(); ++i) {
            if (combat.draw_pile[i].echo > 0) {
                assert(combat.draw_pile[i].echo == (up ? 3 : 2));
                ++gems;
            }
        }
        assert(gems == 1);
    }
    std::cout << "[PASS] Hidden Gem: 1 energy, one random card of the draw pile gains Echo 2 (3 upgraded)" << std::endl;
}

void test_transfigure() {
    assert(in_pool(CharacterId::NECROBINDER, CardId::TRANSFIGURE));
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::TRANSFIGURE, up) == 1);
        assert(CardDef::has_keyword(CardId::TRANSFIGURE, false, KEYWORD_EXHAUST));
        assert(!CardDef::has_keyword(CardId::TRANSFIGURE, true, KEYWORD_EXHAUST));
        CombatEngine combat = fight(CharacterId::NECROBINDER);
        combat.hand.push_back(DeckCard(CardId::TRANSFIGURE, up));
        combat.hand.push_back(DeckCard(CardId::DEFEND, false));
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        assert(combat.play_card(0, 0));
        assert(combat.choice_pending());
        const uint8_t pick[] = { 1 };
        assert(combat.resolve_choice(std::span<const uint8_t>(pick, 1)));
        assert(combat.hand[1].id == CardId::STRIKE && combat.hand[1].echo == 1);
        assert(combat.effective_cost(combat.hand[1]) == 2 && combat.hand[0].echo == 0);
        assert(combat.play_card(1, 0));
        assert(combat.enemies[0].hp == 500 - 12);
    }
    std::cout << "[PASS] Transfigure: a card of the hand gains Echo and costs 1 more" << std::endl;
}

} // namespace

int main() {
    test_echo_replays_the_card();
    test_hidden_gem();
    test_transfigure();
    std::cout << "All echo tests passed." << std::endl;
    return 0;
}
