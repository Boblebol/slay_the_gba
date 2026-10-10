// Percentage modifiers (#89): Block and damage multipliers. The witness card Shadowmeld, then Unmovable, Colossus,
// Cruelty, Tracking, Shadow Step, Lethality and Debilitate. Values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
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
    combat.player.block = 0;
    combat.player.statuses.clear();
    combat.enemies[0].block = 0;
    combat.enemies[0].hp = combat.enemies[0].max_hp = 500;
    combat.enemies[0].statuses.clear();
    return combat;
}

bool in_pool(CharacterId hero, CardId id) {
    for (CardId pool_card : RunManager::card_reward_pool(hero)) {
        if (pool_card == id) return true;
    }
    return false;
}

void play(CombatEngine& combat, CardId id, bool up = false) {
    combat.hand.clear();
    combat.hand.push_back(DeckCard(id, up));
    assert(combat.play_card(0, 0));
}

int16_t damage_of(CombatEngine& combat, CardId id) {
    int16_t before = combat.enemies[0].hp;
    play(combat, id);
    return static_cast<int16_t>(before - combat.enemies[0].hp);
}

void test_shadowmeld() {
    for (bool up : { false, true }) {
        assert(CardDef::get(CardId::SHADOWMELD).type == CardType::SKILL);
        assert(CardDef::get_cost(CardId::SHADOWMELD, up) == (up ? 0 : 1));
        assert(in_pool(CharacterId::SILENT, CardId::SHADOWMELD));
        CombatEngine combat = fight(CharacterId::SILENT);
        play(combat, CardId::SHADOWMELD, up);
        assert(combat.player.block == 0);
        play(combat, CardId::DEFEND);
        assert(combat.player.block == 10);
        play(combat, CardId::DEFEND);
        assert(combat.player.block == 20);
        // The doubling ends with the turn.
        combat.end_player_turn();
        combat.player.block = 0;
        combat.player.energy = 3;
        play(combat, CardId::DEFEND);
        assert(combat.player.block == 5);
    }
    std::cout << "[PASS] Shadowmeld: 1 energy (0 upgraded), Block gained x2 this turn only, in the pool" << std::endl;
}

void test_unmovable() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::UNMOVABLE, up) == (up ? 1 : 2));
        assert(in_pool(CharacterId::IRONCLAD, CardId::UNMOVABLE));
        CombatEngine combat = fight(CharacterId::IRONCLAD);
        play(combat, CardId::UNMOVABLE, up);
        play(combat, CardId::DEFEND);
        assert(combat.player.block == 10);
        play(combat, CardId::DEFEND);
        assert(combat.player.block == 15);
    }
    std::cout << "[PASS] Unmovable: the first Block of each turn is doubled, the next ones are not" << std::endl;
}

void test_colossus() {
    for (bool up : { false, true }) {
        assert(in_pool(CharacterId::IRONCLAD, CardId::COLOSSUS));
        CombatEngine combat = fight(CharacterId::IRONCLAD);
        play(combat, CardId::COLOSSUS, up);
        assert(combat.player.block == (up ? 7 : 4));
        assert(combat.player.halve_vulnerable_damage);
        Enemy& foe = combat.enemies[0];
        foe.current_intent.type = EnemyIntentType::ATTACK;
        foe.current_intent.base_damage = 10;
        int16_t plain = foe.intent_damage(combat.player);
        foe.add_status(StatusType::VULNERABLE, 1);
        assert(foe.intent_damage(combat.player) == plain / 2);
    }
    std::cout << "[PASS] Colossus: 4/7 Block, 50% less damage from Vulnerable enemies" << std::endl;
}

void test_cruelty() {
    assert(in_pool(CharacterId::IRONCLAD, CardId::CRUELTY));
    for (bool up : { false, true }) {
        CombatEngine combat = fight(CharacterId::IRONCLAD);
        combat.enemies[0].add_status(StatusType::VULNERABLE, 5);
        play(combat, CardId::CRUELTY, up);
        combat.hand.clear();
        // Strike 6: x1.5 Vulnerable = 9, then +25% (+50% upgraded).
        int16_t before = combat.enemies[0].hp;
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        assert(combat.play_card(0, 0));
        assert(before - combat.enemies[0].hp == (up ? 13 : 11));
    }
    std::cout << "[PASS] Cruelty: Vulnerable enemies take 25% (50% upgraded) more" << std::endl;
}

void test_tracking() {
    assert(in_pool(CharacterId::SILENT, CardId::TRACKING));
    CombatEngine combat = fight(CharacterId::SILENT);
    play(combat, CardId::TRACKING);
    combat.enemies[0].add_status(StatusType::WEAK, 3);
    int16_t before = combat.enemies[0].hp;
    combat.hand.clear();
    combat.hand.push_back(DeckCard(CardId::STRIKE, false));
    assert(combat.play_card(0, 0));
    assert(before - combat.enemies[0].hp == 9);
    std::cout << "[PASS] Tracking: Weak enemies take 50% more damage" << std::endl;
}

void test_shadow_step() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::SHADOW_STEP, up) == (up ? 0 : 1));
        assert(in_pool(CharacterId::SILENT, CardId::SHADOW_STEP));
        CombatEngine combat = fight(CharacterId::SILENT);
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.hand.push_back(DeckCard(CardId::SHADOW_STEP, up));
        assert(combat.play_card(1, 0));
        assert(combat.hand.empty());
        // Still normal damage this turn, doubled on the next one.
        assert(damage_of(combat, CardId::STRIKE) == 6);
        combat.end_player_turn();
        combat.hand.clear();
        assert(damage_of(combat, CardId::STRIKE) == 12);
    }
    std::cout << "[PASS] Shadow Step: discards the hand, Attacks deal double damage next turn only" << std::endl;
}

void test_lethality() {
    assert(in_pool(CharacterId::NECROBINDER, CardId::LETHALITY));
    assert(CardDef::has_keyword(CardId::LETHALITY, false, KEYWORD_ETHEREAL));
    for (bool up : { false, true }) {
        CombatEngine combat = fight(CharacterId::NECROBINDER);
        play(combat, CardId::LETHALITY, up);
        int16_t first = damage_of(combat, CardId::STRIKE);
        int16_t second = damage_of(combat, CardId::STRIKE);
        assert(second == 6);
        assert(first == (up ? 10 : 9));
    }
    std::cout << "[PASS] Lethality: the first Attack of the turn deals 50% (75% upgraded) more" << std::endl;
}

void test_debilitate() {
    assert(in_pool(CharacterId::NECROBINDER, CardId::DEBILITATE));
    for (bool up : { false, true }) {
        CombatEngine combat = fight(CharacterId::NECROBINDER);
        play(combat, CardId::DEBILITATE, up);
        Enemy& foe = combat.enemies[0];
        assert(foe.hp == 500 - (up ? 12 : 10));
        assert(foe.debilitate_turns == (up ? 3 : 2));
        // Vulnerable is x2 instead of x1.5 on this enemy.
        foe.add_status(StatusType::VULNERABLE, 3);
        assert(damage_of(combat, CardId::STRIKE) == 12);
        // The effect wears off after its turns.
        combat.end_player_turn();
        combat.end_player_turn();
        combat.end_player_turn();
        assert(combat.enemies[0].debilitate_turns == 0);
    }
    std::cout << "[PASS] Debilitate: Vulnerable doubled on the enemy for 2 (3) turns" << std::endl;
}

} // namespace

int main() {
    test_shadowmeld();
    test_unmovable();
    test_colossus();
    test_cruelty();
    test_tracking();
    test_shadow_step();
    test_lethality();
    test_debilitate();
    std::cout << "All percentage modifier tests passed." << std::endl;
    return 0;
}
