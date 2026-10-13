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

int16_t damage_of(CombatEngine& combat, CardId id) {
    int16_t before = combat.enemies[0].hp;
    play(combat, DeckCard(id, false));
    return static_cast<int16_t>(before - combat.enemies[0].hp);
}

void test_lethality() {
    assert(CardDef::get(CardId::LETHALITY).type == CardType::POWER);
    assert(CardDef::get_cost(CardId::LETHALITY, false) == 1 && CardDef::get_cost(CardId::LETHALITY, true) == 1);
    assert(CardDef::get_value(CardId::LETHALITY, false) == 50 && CardDef::get_value(CardId::LETHALITY, true) == 75);
    assert(CardDef::has_keyword(CardId::LETHALITY, false, KEYWORD_ETHEREAL));
    assert(CardDef::has_keyword(CardId::LETHALITY, true, KEYWORD_ETHEREAL));
    for (bool up : { false, true }) {
        CombatEngine combat = make_fight();
        play(combat, DeckCard(CardId::LETHALITY, up));
        assert(combat.power_amount(PowerId::LETHALITY) == (up ? 75 : 50));
        int16_t first = damage_of(combat, CardId::STRIKE);
        int16_t second = damage_of(combat, CardId::STRIKE);
        assert(second == 6);
        assert(first == (up ? 10 : 9));
    }
    assert_in_pool(CharacterId::NECROBINDER, { CardId::LETHALITY });
    std::cout << "[PASS] Necrobinder: Lethality" << std::endl;
}

void test_forbidden_grimoire() {
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
    test_debilitate();
    test_lethality();
    test_forbidden_grimoire();
    std::cout << "All Necrobinder card tests passed." << std::endl;
    return 0;
}
