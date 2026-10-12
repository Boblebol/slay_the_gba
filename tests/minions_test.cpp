// Regent minions (#103): the three Minion cards, the cards that transform into them, and Vitruvian Minion.
#include <cassert>
#include <iostream>
#include <span>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

bool in_pool(CardId id) {
    for (CardId pool_card : RunManager::card_reward_pool(CharacterId::REGENT)) {
        if (pool_card == id) return true;
    }
    return false;
}

CombatEngine fight() {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::REGENT);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 9;
    combat.player.block = 0;
    combat.enemies[0].hp = combat.enemies[0].max_hp = 500;
    return combat;
}

void test_minion_cards() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::MINION_STRIKE, up) == 0);
        assert(CardDef::has_keyword(CardId::MINION_STRIKE, up, KEYWORD_EXHAUST));
        CombatEngine combat = fight();
        combat.draw_pile.push_back(DeckCard(CardId::DEFEND, false));
        combat.hand.push_back(DeckCard(CardId::MINION_STRIKE, up));
        assert(combat.play_card(0, 0));
        assert(combat.enemies[0].hp == 500 - (up ? 9 : 6) && combat.hand.size() == 1);
        assert(combat.exhaust_pile.size() == 1);

        combat = fight();
        combat.hand.push_back(DeckCard(CardId::MINION_DIVE_BOMB, up));
        assert(combat.play_card(0, 0));
        assert(combat.enemies[0].hp == 500 - (up ? 16 : 13));

        combat = fight();
        combat.hand.push_back(DeckCard(CardId::MINION_SACRIFICE, up));
        assert(combat.play_card(0, 0));
        assert(combat.player.block == (up ? 10 : 7));
    }
    std::cout << "[PASS] Minion Strike (6 + draw 1), Minion Dive Bomb (13), Minion Sacrifice (7 Block): free and Exhaust" << std::endl;
}

void test_begone() {
    assert(in_pool(CardId::BEGONE));
    for (bool up : { false, true }) {
        CombatEngine combat = fight();
        combat.hand.push_back(DeckCard(CardId::BEGONE, up));
        combat.hand.push_back(DeckCard(CardId::DEFEND, false));
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        assert(combat.play_card(0, 0));
        assert(combat.choice_pending());
        const uint8_t pick[] = { 1 };
        assert(combat.resolve_choice(std::span<const uint8_t>(pick, 1)));
        assert(combat.hand[0].id == CardId::DEFEND);
        assert(combat.hand[1].id == CardId::MINION_STRIKE && combat.hand[1].upgraded == up);
    }
    std::cout << "[PASS] Begone!: a card of the hand becomes Minion Strike (upgraded if upgraded)" << std::endl;
}

void test_charge() {
    assert(in_pool(CardId::CHARGE));
    for (bool up : { false, true }) {
        CombatEngine combat = fight();
        for (int i = 0; i < 3; ++i) combat.draw_pile.push_back(DeckCard(CardId::DEFEND, false));
        combat.hand.push_back(DeckCard(CardId::CHARGE, up));
        assert(combat.play_card(0, 0));
        assert(combat.choice_pending());
        const uint8_t picks[] = { 0, 2 };
        assert(combat.resolve_choice(std::span<const uint8_t>(picks, 2)));
        int bombs = 0;
        for (size_t i = 0; i < combat.draw_pile.size(); ++i) {
            bombs += combat.draw_pile[i].id == CardId::MINION_DIVE_BOMB && combat.draw_pile[i].upgraded == up;
        }
        assert(bombs == 2 && combat.draw_pile.size() == 3);
    }
    std::cout << "[PASS] Charge!!: 2 cards of the draw pile become Minion Dive Bomb" << std::endl;
}

void test_guards() {
    assert(in_pool(CardId::GUARDS));
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::GUARDS, up) == 2);
        assert(CardDef::has_keyword(CardId::GUARDS, up, KEYWORD_EXHAUST));
        CombatEngine combat = fight();
        combat.hand.push_back(DeckCard(CardId::GUARDS, up));
        for (int i = 0; i < 3; ++i) combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        assert(combat.play_card(0, 0));
        assert(combat.choice_pending());
        const uint8_t picks[] = { 0, 1 };
        assert(combat.resolve_choice(std::span<const uint8_t>(picks, 2)));
        int sacrifices = 0;
        for (size_t i = 0; i < combat.hand.size(); ++i) sacrifices += combat.hand[i].id == CardId::MINION_SACRIFICE;
        assert(sacrifices == 2 && combat.hand.size() == 3);
    }
    std::cout << "[PASS] Guards!!!: any number of cards of the hand become Minion Sacrifice, Exhaust" << std::endl;
}

void test_vitruvian_minion() {
    // Without the relic.
    CombatEngine plain = fight();
    plain.hand.push_back(DeckCard(CardId::MINION_DIVE_BOMB, false));
    plain.hand.push_back(DeckCard(CardId::MINION_SACRIFICE, false));
    assert(plain.play_card(0, 0) && plain.play_card(0, 0));
    assert(plain.enemies[0].hp == 500 - 13 && plain.player.block == 7);
    // With the relic: only the Minion cards double.
    CombatEngine combat = fight();
    combat.add_power(PowerId::VITRUVIAN_MINION);
    combat.hand.push_back(DeckCard(CardId::MINION_DIVE_BOMB, false));
    combat.hand.push_back(DeckCard(CardId::MINION_SACRIFICE, false));
    combat.hand.push_back(DeckCard(CardId::STRIKE, false));
    combat.hand.push_back(DeckCard(CardId::DEFEND, false));
    assert(combat.play_card(0, 0) && combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 500 - 26 && combat.player.block == 14);
    assert(combat.play_card(0, 0) && combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 500 - 26 - 6 && combat.player.block == 14 + 5);
    std::cout << "[PASS] Vitruvian Minion: Minion cards deal double damage and give double Block, other cards do not" << std::endl;
}

} // namespace

int main() {
    test_minion_cards();
    test_begone();
    test_charge();
    test_guards();
    test_vitruvian_minion();
    std::cout << "All minion tests passed." << std::endl;
    return 0;
}
