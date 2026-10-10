// Card effects that outlast the combat (#94): gold, potion, extra card reward, card removal, and a defeat on a
// condition. Values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
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

CombatEngine weak_fight(CharacterId hero, int16_t enemy_hp) {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, hero);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 9;
    combat.enemies[0].hp = enemy_hp;
    combat.enemies[0].block = 0;
    combat.enemy = combat.enemies[0];
    return combat;
}

void test_hand_of_greed() {
    for (bool up : { false, true }) {
        const int16_t value = up ? 25 : 20;
        assert(CardDef::get_cost(CardId::HAND_OF_GREED, up) == 2);
        // A hit that does not kill gives nothing.
        CombatEngine combat = weak_fight(CharacterId::IRONCLAD, 100);
        combat.hand.push_back(DeckCard(CardId::HAND_OF_GREED, up));
        assert(combat.play_card(0, 0));
        assert(combat.enemies[0].hp == 100 - value && combat.gold_gained == 0);
        // The killing blow does.
        combat = weak_fight(CharacterId::IRONCLAD, value);
        combat.hand.push_back(DeckCard(CardId::HAND_OF_GREED, up));
        assert(combat.play_card(0, 0));
        assert(combat.gold_gained == value);
    }
    std::cout << "[PASS] Hand of Greed: 20/25 damage, Fatal: 20/25 gold kept for the run" << std::endl;
}

void test_gold_reaches_the_run() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 1234);
    int16_t before = run.gold;
    run.combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    run.combat.gold_gained = 20;
    run.combat.add_power(PowerId::ROYALTIES, 30);
    run.combat.state = CombatState::VICTORY;
    run.phase = RunPhase::IN_COMBAT;
    run.proceed_after_combat();
    assert(run.gold >= before + 50);
    assert(run.combat.gold_gained == 0);
    std::cout << "[PASS] Gold of the combat and Royalties are paid by the run" << std::endl;
}

void test_royalties() {
    assert(in_pool(CharacterId::REGENT, CardId::ROYALTIES));
    for (bool up : { false, true }) {
        CombatEngine combat = weak_fight(CharacterId::REGENT, 100);
        combat.hand.push_back(DeckCard(CardId::ROYALTIES, up));
        assert(combat.play_card(0, 0));
        assert(combat.power_amount(PowerId::ROYALTIES) == (up ? 40 : 30));
    }
    std::cout << "[PASS] Royalties: 30/40 gold at the end of the combat" << std::endl;
}

void test_the_hunt() {
    assert(in_pool(CharacterId::SILENT, CardId::THE_HUNT));
    for (bool up : { false, true }) {
        const int16_t value = up ? 15 : 10;
        assert(CardDef::has_keyword(CardId::THE_HUNT, up, KEYWORD_EXHAUST));
        CombatEngine combat = weak_fight(CharacterId::SILENT, 100);
        combat.hand.push_back(DeckCard(CardId::THE_HUNT, up));
        assert(combat.play_card(0, 0));
        assert(combat.enemies[0].hp == 100 - value && combat.extra_card_rewards == 0);
        combat = weak_fight(CharacterId::SILENT, value);
        combat.hand.push_back(DeckCard(CardId::THE_HUNT, up));
        assert(combat.play_card(0, 0));
        assert(combat.extra_card_rewards == 1);
    }
    std::cout << "[PASS] The Hunt: 10/15 damage, Fatal: an extra card reward" << std::endl;
}

void test_extra_card_reward_screen() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 1234);
    run.combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    run.combat.extra_card_rewards = 1;
    run.combat.state = CombatState::VICTORY;
    run.phase = RunPhase::IN_COMBAT;
    run.proceed_after_combat();
    assert(run.phase == RunPhase::REWARD_SCREEN);
    size_t deck = run.master_deck.size();
    run.pick_card_reward(0);
    // The first pick leaves the screen open on a second set of cards.
    assert(run.phase == RunPhase::REWARD_SCREEN && run.master_deck.size() == deck + 1);
    assert(run.current_card_rewards.size() > 0);
    run.pick_card_reward(0);
    assert(run.phase != RunPhase::REWARD_SCREEN && run.master_deck.size() == deck + 2);
    std::cout << "[PASS] An extra card reward offers a second set of cards before the rewards end" << std::endl;
}

void test_forbidden_grimoire() {
    assert(in_pool(CharacterId::NECROBINDER, CardId::FORBIDDEN_GRIMOIRE));
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::FORBIDDEN_GRIMOIRE, up) == 2);
        assert(CardDef::has_keyword(CardId::FORBIDDEN_GRIMOIRE, up, KEYWORD_ETERNAL));
    }
    RunManager run;
    run.start_run(CharacterId::NECROBINDER, 1234);
    run.combat.start_combat(EnemyType::JAW_WORM, CharacterId::NECROBINDER);
    run.combat.hand.clear();
    run.combat.hand.push_back(DeckCard(CardId::FORBIDDEN_GRIMOIRE, false));
    run.combat.player.energy = 9;
    assert(run.combat.play_card(0, 0));
    run.combat.state = CombatState::VICTORY;
    run.phase = RunPhase::IN_COMBAT;
    run.proceed_after_combat();
    assert(run.has_deck_choice() && run.deck_choice.kind == DeckChoiceKind::REMOVE && run.deck_choice.optional);
    std::cout << "[PASS] Forbidden Grimoire: a card of the deck can be removed at the end of the combat" << std::endl;
}

void test_alchemize() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::ALCHEMIZE, up) == 1);
        assert(CardDef::has_keyword(CardId::ALCHEMIZE, up, KEYWORD_EXHAUST));
    }
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 1234);
    run.combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    run.combat.hand.clear();
    run.combat.hand.push_back(DeckCard(CardId::ALCHEMIZE, false));
    run.combat.player.energy = 9;
    size_t potions = run.potions.size();
    assert(run.combat.play_card(0, 0));
    assert(run.combat.potions_gained == 1);
    run.combat.state = CombatState::VICTORY;
    run.phase = RunPhase::IN_COMBAT;
    run.proceed_after_combat();
    assert(run.potions.size() == potions + 1);
    std::cout << "[PASS] Alchemize: a random potion reaches the belt after the combat" << std::endl;
}

void test_the_gambit() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::THE_GAMBIT, up) == 0);
        CombatEngine combat = weak_fight(CharacterId::IRONCLAD, 100);
        combat.player.block = 0;
        combat.hand.push_back(DeckCard(CardId::THE_GAMBIT, up));
        assert(combat.play_card(0, 0));
        assert(combat.player.block == (up ? 75 : 50));
        // Jaw Worm's Chomp is fully blocked: alive.
        combat.end_player_turn();
        assert(combat.player.is_alive() && combat.state != CombatState::DEFEAT);
        // Without Block the hit goes through: dead.
        combat.player.block = 0;
        combat.end_player_turn();
        assert(combat.state == CombatState::DEFEAT || !combat.player.is_alive());
    }
    std::cout << "[PASS] The Gambit: 50/75 Block, unblocked attack damage kills" << std::endl;
}

} // namespace

int main() {
    test_hand_of_greed();
    test_gold_reaches_the_run();
    test_royalties();
    test_the_hunt();
    test_extra_card_reward_screen();
    test_forbidden_grimoire();
    test_alchemize();
    test_the_gambit();
    std::cout << "All beyond-combat tests passed." << std::endl;
    return 0;
}
