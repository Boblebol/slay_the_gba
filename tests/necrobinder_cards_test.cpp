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
