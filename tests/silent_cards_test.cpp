// Silent cards added on top of the existing mechanics: each one is played in its base and upgraded versions,
// and must be offered in the hero's card pool.
#include <algorithm>
#include <cassert>
#include <iostream>
#include <span>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

// A fight against three enemies: two Lice (18 HP) and a Jaw Worm (42 HP).
CombatEngine make_pack(CharacterId hero) {
    CombatEngine combat;
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::JAW_WORM);
    combat.start_combat_multi(pack, hero);
    return combat;
}

// Puts `card` alone in hand with full energy and plays it.
void play(CombatEngine& combat, DeckCard card, int target = -1) {
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

int count_in(const auto& pile, CardId id) {
    int count = 0;
    for (const DeckCard& card : pile) count += card.id == id;
    return count;
}

void test_expose() {
    for (bool upgraded : { false, true }) {
        // Expose: Block and Artifact of the target are removed, then it is made Vulnerable. It costs nothing.
        CombatEngine expose = make_pack(CharacterId::SILENT);
        expose.enemies[1].block = 12;
        expose.enemies[1].add_status(StatusType::ARTIFACT, 2);
        expose.enemies[0].block = 7;
        expose.enemies[0].add_status(StatusType::ARTIFACT, 1);
        play(expose, DeckCard(CardId::EXPOSE, upgraded), 1);
        assert(expose.player.energy == expose.player.max_energy);
        assert(expose.enemies[1].block == 0);
        assert(expose.enemies[1].get_status(StatusType::ARTIFACT) == 0);
        assert(expose.enemies[1].get_status(StatusType::VULNERABLE) == (upgraded ? 3 : 2));
        assert(expose.enemies[1].hp == 18);

        // The other enemies are left alone.
        assert(expose.enemies[0].block == 7 && expose.enemies[0].get_status(StatusType::ARTIFACT) == 1);
        assert(expose.enemies[0].get_status(StatusType::VULNERABLE) == 0);
        assert(expose.enemies[2].get_status(StatusType::VULNERABLE) == 0);

        // It exhausts itself.
        assert(CardDef::has_keyword(CardId::EXPOSE, upgraded, KEYWORD_EXHAUST));
        assert(count_in(expose.exhaust_pile, CardId::EXPOSE) == 1);
        assert(count_in(expose.discard_pile, CardId::EXPOSE) == 0);

        // The Vulnerable it applies makes the next Attack hit harder: 6 base damage becomes 9.
        CombatEngine follow_up = make_pack(CharacterId::SILENT);
        play(follow_up, DeckCard(CardId::EXPOSE, upgraded), 0);
        play(follow_up, CardId::STRIKE, 0);
        assert(follow_up.enemies[0].hp == 18 - 9);

        // Nothing to remove: it only applies Vulnerable.
        CombatEngine bare = make_pack(CharacterId::SILENT);
        play(bare, DeckCard(CardId::EXPOSE, upgraded), 2);
        assert(bare.enemies[2].block == 0 && bare.enemies[2].get_status(StatusType::ARTIFACT) == 0);
        assert(bare.enemies[2].get_status(StatusType::VULNERABLE) == (upgraded ? 3 : 2));
    }

    // Base and upgraded definitions: 0 energy, 2 then 3 Vulnerable.
    assert(CardDef::get_cost(CardId::EXPOSE, false) == 0 && CardDef::get_cost(CardId::EXPOSE, true) == 0);
    assert(CardDef::get_secondary(CardId::EXPOSE, false) == 2 && CardDef::get_secondary(CardId::EXPOSE, true) == 3);
    assert(CardDef::get(CardId::EXPOSE).type == CardType::SKILL);
    assert(CardDef::get(CardId::EXPOSE).target == TargetType::SINGLE_ENEMY);

    assert_in_pool(CharacterId::SILENT, { CardId::EXPOSE });
    std::cout << "[PASS] Silent: Expose" << std::endl;
}

void test_untouchable() {
    for (bool upgraded : { false, true }) {
        // Untouchable: 2 energy, Gain 6 Block, or 9 once upgraded.
        CombatEngine combat = make_pack(CharacterId::SILENT);
        play(combat, DeckCard(CardId::UNTOUCHABLE, upgraded));
        assert(combat.player.block == (upgraded ? 9 : 6));
        assert(combat.player.energy == combat.player.max_energy - 2);
        assert(count_in(combat.discard_pile, CardId::UNTOUCHABLE) == 1);
    }

    // Base and upgraded definitions: 2 energy, 6 then 9 Block.
    assert(CardDef::get_cost(CardId::UNTOUCHABLE, false) == 2 && CardDef::get_cost(CardId::UNTOUCHABLE, true) == 2);
    assert(CardDef::get_value(CardId::UNTOUCHABLE, false) == 6 && CardDef::get_value(CardId::UNTOUCHABLE, true) == 9);
    assert(CardDef::get(CardId::UNTOUCHABLE).type == CardType::SKILL);

    assert_in_pool(CharacterId::SILENT, { CardId::UNTOUCHABLE });
    std::cout << "[PASS] Silent: Untouchable" << std::endl;
}

void test_ricochet() {
    for (bool upgraded : { false, true }) {
        // Ricochet: 3 damage to a random enemy, 4 times (5 when upgraded). It costs 2 energy.
        CombatEngine combat = make_pack(CharacterId::SILENT);
        for (Enemy& enemy : combat.enemies) {
            enemy.hp = enemy.max_hp = 200;
            enemy.block = 0;
        }
        int hp_before = 0;
        for (const Enemy& enemy : combat.enemies) hp_before += enemy.hp;
        play(combat, DeckCard(CardId::RICOCHET, upgraded), 0);
        assert(combat.player.energy == combat.player.max_energy - 2);

        int hp_after = 0;
        for (const Enemy& enemy : combat.enemies) hp_after += enemy.hp;
        assert(hp_before - hp_after == 3 * (upgraded ? 5 : 4));
        assert(count_in(combat.discard_pile, CardId::RICOCHET) == 1);
    }

    // Base and upgraded definitions: 2 energy, 3 damage, 4 then 5 hits, Sly.
    assert(CardDef::get_cost(CardId::RICOCHET, false) == 2 && CardDef::get_cost(CardId::RICOCHET, true) == 2);
    assert(CardDef::get_value(CardId::RICOCHET, false) == 3 && CardDef::get_value(CardId::RICOCHET, true) == 3);
    assert(CardDef::get_secondary(CardId::RICOCHET, false) == 4 && CardDef::get_secondary(CardId::RICOCHET, true) == 5);
    assert(CardDef::get(CardId::RICOCHET).type == CardType::ATTACK);
    assert(CardDef::has_keyword(CardId::RICOCHET, false, KEYWORD_SLY));
    assert(CardDef::has_keyword(CardId::RICOCHET, true, KEYWORD_SLY));

    assert_in_pool(CharacterId::SILENT, { CardId::RICOCHET });
    std::cout << "[PASS] Silent: Ricochet" << std::endl;
}

void test_reflex() {
    for (bool upgraded : { false, true }) {
        // Reflex: Sly, costs 3, draws 2 cards (3 when upgraded).
        assert(CardDef::get_cost(CardId::REFLEX, false) == 3 && CardDef::get_cost(CardId::REFLEX, true) == 3);
        assert(CardDef::get_secondary(CardId::REFLEX, false) == 0 && CardDef::get_secondary(CardId::REFLEX, true) == 0);
        assert(CardDef::get_value(CardId::REFLEX, false) == 2 && CardDef::get_value(CardId::REFLEX, true) == 3);
        assert(CardDef::get(CardId::REFLEX).type == CardType::SKILL);
        assert(CardDef::get(CardId::REFLEX).target == TargetType::SELF);

        CombatEngine reflex = make_pack(CharacterId::SILENT);
        reflex.draw_pile.clear();
        reflex.draw_pile.push_back(DeckCard(CardId::STRIKE, false));
        reflex.draw_pile.push_back(DeckCard(CardId::DEFEND, false));
        reflex.draw_pile.push_back(DeckCard(CardId::STRIKE, false));
        play(reflex, DeckCard(CardId::REFLEX, upgraded));
        assert(reflex.player.energy == reflex.player.max_energy - 3);
        assert(reflex.hand.size() == (upgraded ? 3u : 2u));
    }

    assert_in_pool(CharacterId::SILENT, { CardId::REFLEX });
    std::cout << "[PASS] Silent: Reflex" << std::endl;
}

void test_flick_flack() {
    for (bool upgraded : { false, true }) {
        // Flick-Flack: 7 damage to every enemy (9 upgraded). It costs 1 energy.
        CombatEngine flick = make_pack(CharacterId::SILENT);
        play(flick, DeckCard(CardId::FLICK_FLACK, upgraded));
        int damage = upgraded ? 9 : 7;
        assert(flick.player.energy == flick.player.max_energy - 1);
        assert(flick.enemies[0].hp == 18 - damage);
        assert(flick.enemies[1].hp == 18 - damage);
        assert(flick.enemies[2].hp == 42 - damage);
        assert(count_in(flick.discard_pile, CardId::FLICK_FLACK) == 1);
    }

    // Base and upgraded definitions: 1 energy, 7 then 9 damage, Attack against all enemies.
    assert(CardDef::get_cost(CardId::FLICK_FLACK, false) == 1 && CardDef::get_cost(CardId::FLICK_FLACK, true) == 1);
    assert(CardDef::get_value(CardId::FLICK_FLACK, false) == 7 && CardDef::get_value(CardId::FLICK_FLACK, true) == 9);
    assert(CardDef::get(CardId::FLICK_FLACK).type == CardType::ATTACK);
    assert(CardDef::get(CardId::FLICK_FLACK).target == TargetType::ALL_ENEMIES);

    assert_in_pool(CharacterId::SILENT, { CardId::FLICK_FLACK });
    std::cout << "[PASS] Silent: Flick-Flack" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Silent Cards ===" << std::endl;
    test_expose();
    test_untouchable();
    test_ricochet();
    test_reflex();
    test_flick_flack();
    std::cout << "=== All Silent Cards Passed! ===" << std::endl;
    return 0;
}
