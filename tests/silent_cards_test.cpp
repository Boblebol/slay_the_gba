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

void test_abrasive() {
    for (bool upgraded : { false, true }) {
        // Abrasive: Sly, +1 Dexterity and 4 (6) Thorns. It costs 3 energy.
        CombatEngine combat = make_pack(CharacterId::SILENT);
        combat.player.energy = combat.player.max_energy;
        play(combat, DeckCard(CardId::ABRASIVE, upgraded));
        assert(combat.player.energy == combat.player.max_energy - 3);
        assert(combat.player.get_status(StatusType::DEXTERITY) == 1);
        assert(combat.player.get_status(StatusType::THORNS) == (upgraded ? 6 : 4));
    }

    assert(CardDef::get_cost(CardId::ABRASIVE, false) == 3 && CardDef::get_cost(CardId::ABRASIVE, true) == 3);
    assert(CardDef::get(CardId::ABRASIVE).type == CardType::POWER);
    assert(CardDef::has_keyword(CardId::ABRASIVE, false, KEYWORD_SLY));

    assert_in_pool(CharacterId::SILENT, { CardId::ABRASIVE });
    std::cout << "[PASS] Silent: Abrasive" << std::endl;
}

void test_master_planner() {
    // Power, rare: 2 energy, 1 once upgraded. While in play, every Skill played gains Sly (Attacks do not).
    assert(CardDef::get(CardId::MASTER_PLANNER).type == CardType::POWER);
    assert(CardDef::get_cost(CardId::MASTER_PLANNER, false) == 2 && CardDef::get_cost(CardId::MASTER_PLANNER, true) == 1);
    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_pack(CharacterId::SILENT);
        combat.hand.clear();
        combat.hand.push_back(DeckCard(CardId::MASTER_PLANNER, upgraded));
        combat.hand.push_back(DeckCard(CardId::DEFEND, false));
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.player.energy = 5;
        assert(combat.play_card(0, -1));
        assert(combat.player.energy == (upgraded ? 4 : 3));
        assert(combat.has_power(PowerId::MASTER_PLANNER));
        assert(combat.play_card(0, -1)); // Defend, a Skill: it gains Sly
        assert(combat.play_card(0, 0));  // Strike, an Attack: it does not
        bool defend_sly = false, strike_sly = false;
        for (size_t i = 0; i < combat.discard_pile.size(); ++i) {
            if (combat.discard_pile[i].id == CardId::DEFEND) defend_sly = CardDef::is_sly(combat.discard_pile[i]);
            if (combat.discard_pile[i].id == CardId::STRIKE) strike_sly = CardDef::is_sly(combat.discard_pile[i]);
        }
        assert(defend_sly && !strike_sly);
    }
    assert_in_pool(CharacterId::SILENT, { CardId::MASTER_PLANNER });
    std::cout << "[PASS] Silent: Master Planner" << std::endl;
}

void test_hand_trick() {
    for (bool upgraded : { false, true }) {
        assert(CardDef::get_cost(CardId::HAND_TRICK, upgraded) == 1);
        assert(CardDef::get(CardId::HAND_TRICK).type == CardType::SKILL);
        CombatEngine combat = make_pack(CharacterId::SILENT);
        combat.hand.clear();
        combat.hand.push_back(DeckCard(CardId::HAND_TRICK, upgraded));
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.hand.push_back(DeckCard(CardId::DEFEND, false));
        combat.player.energy = combat.player.max_energy;
        combat.player.block = 0;
        assert(combat.play_card(0, 0));
        assert(combat.player.block == (upgraded ? 10 : 7));
        // Defend is the only Skill left in hand: it gains Sly until the end of the turn (picked by the player if asked).
        if (combat.choice_pending()) {
            for (size_t i = 0; i < combat.hand.size(); ++i) {
                if (combat.hand[i].id == CardId::STRIKE) assert(!combat.choice_allows(i));
            }
            for (size_t i = 0; i < combat.hand.size(); ++i) {
                if (combat.hand[i].id == CardId::DEFEND) {
                    const uint8_t pick[] = { static_cast<uint8_t>(i) };
                    assert(combat.resolve_choice(std::span<const uint8_t>(pick, 1)));
                    break;
                }
            }
        }
        bool defend_sly = false;
        bool strike_sly = false;
        for (size_t i = 0; i < combat.hand.size(); ++i) {
            if (combat.hand[i].id == CardId::DEFEND) defend_sly = CardDef::is_sly(combat.hand[i]);
            if (combat.hand[i].id == CardId::STRIKE) strike_sly = CardDef::is_sly(combat.hand[i]);
        }
        assert(defend_sly && !strike_sly);
    }
    assert_in_pool(CharacterId::SILENT, { CardId::HAND_TRICK });
    std::cout << "[PASS] Silent: Hand Trick" << std::endl;
}

void test_tracking() {
    assert(CardDef::get_cost(CardId::TRACKING, false) == 2);
    assert(CardDef::get_cost(CardId::TRACKING, true) == 1);
    assert(CardDef::get(CardId::TRACKING).type == CardType::POWER);
    assert_in_pool(CharacterId::SILENT, { CardId::TRACKING });
    for (bool upgraded : { false, true }) {
        // Weak enemies take 50% more damage from Attacks (Strike 6 -> 9); the upgrade keeps the same 50%.
        CombatEngine combat = make_pack(CharacterId::SILENT);
        play(combat, DeckCard(CardId::TRACKING, upgraded));
        assert(combat.player.energy == combat.player.max_energy - (upgraded ? 1 : 2));
        combat.enemies[0].add_status(StatusType::WEAK, 3);
        int16_t weak_before = combat.enemies[0].hp;
        int16_t other_before = combat.enemies[1].hp;
        combat.hand.clear();
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.player.energy = combat.player.max_energy;
        assert(combat.play_card(0, 0));
        assert(weak_before - combat.enemies[0].hp == 9);
        combat.hand.clear();
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.player.energy = combat.player.max_energy;
        assert(combat.play_card(0, 1));
        assert(other_before - combat.enemies[1].hp == 6);
    }
    std::cout << "[PASS] Silent: Tracking" << std::endl;
}

void test_the_hunt() {
    for (bool upgraded : { false, true }) {
        const int16_t damage = upgraded ? 15 : 10;
        assert(CardDef::get_cost(CardId::THE_HUNT, upgraded) == 1);
        assert(CardDef::get(CardId::THE_HUNT).type == CardType::ATTACK);
        assert(CardDef::has_keyword(CardId::THE_HUNT, upgraded, KEYWORD_EXHAUST));

        // A survivor takes the damage and earns no extra reward; the card is exhausted.
        CombatEngine combat = make_pack(CharacterId::SILENT);
        int16_t hp = combat.enemies[2].hp;
        play(combat, DeckCard(CardId::THE_HUNT, upgraded), 2);
        assert(combat.enemies[2].hp == hp - damage);
        assert(combat.extra_card_rewards == 0);
        assert(combat.exhaust_pile.size() == 1);

        // A kill (Fatal) earns one extra card reward.
        CombatEngine kill = make_pack(CharacterId::SILENT);
        kill.enemies[0].hp = damage;
        play(kill, DeckCard(CardId::THE_HUNT, upgraded), 0);
        assert(!kill.enemies[0].is_alive());
        assert(kill.extra_card_rewards == 1);
    }
    assert_in_pool(CharacterId::SILENT, { CardId::THE_HUNT });
    std::cout << "[PASS] Silent: The Hunt" << std::endl;
}

void test_shadow_step() {
    for (bool upgraded : { false, true }) {
        // Shadow Step: 1 energy, free once upgraded. It discards the whole hand.
        assert(CardDef::get_cost(CardId::SHADOW_STEP, false) == 1 && CardDef::get_cost(CardId::SHADOW_STEP, true) == 0);
        assert(CardDef::get(CardId::SHADOW_STEP).type == CardType::SKILL);
        assert(CardDef::get(CardId::SHADOW_STEP).target == TargetType::SELF);

        // One sturdy Jaw Worm, with nothing to soften or deflect the hits.
        CombatEngine combat;
        combat.start_combat(EnemyType::JAW_WORM, CharacterId::SILENT);
        combat.enemies[0].hp = combat.enemies[0].max_hp = 500;
        combat.enemies[0].block = 0;
        combat.enemies[0].statuses.clear();
        combat.player.statuses.clear();
        combat.hand.clear();
        combat.draw_pile.clear();
        combat.discard_pile.clear();
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.hand.push_back(DeckCard(CardId::DEFEND, false));
        combat.hand.push_back(DeckCard(CardId::SHADOW_STEP, upgraded));
        combat.player.energy = combat.player.max_energy;
        assert(combat.play_card(2, -1));
        assert(combat.hand.empty());
        assert(combat.player.energy == combat.player.max_energy - (upgraded ? 0 : 1));

        // This turn, Attacks still deal normal damage.
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.player.energy = combat.player.max_energy;
        int16_t hp = combat.enemies[0].hp;
        assert(combat.play_card(0, 0));
        assert(hp - combat.enemies[0].hp == 6);

        // Next turn, Attacks deal double damage.
        combat.end_player_turn();
        combat.enemies[0].block = 0;
        combat.hand.clear();
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.player.energy = combat.player.max_energy;
        hp = combat.enemies[0].hp;
        assert(combat.play_card(0, 0));
        assert(hp - combat.enemies[0].hp == 12);

        // The turn after, damage is back to normal.
        combat.end_player_turn();
        combat.enemies[0].block = 0;
        combat.hand.clear();
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.player.energy = combat.player.max_energy;
        hp = combat.enemies[0].hp;
        assert(combat.play_card(0, 0));
        assert(hp - combat.enemies[0].hp == 6);
    }

    assert_in_pool(CharacterId::SILENT, { CardId::SHADOW_STEP });
    std::cout << "[PASS] Silent: Shadow Step" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Silent Cards ===" << std::endl;
    test_expose();
    test_abrasive();
    test_master_planner();
    test_hand_trick();
    test_tracking();
    test_the_hunt();
    test_shadow_step();
    std::cout << "=== All Silent Cards Passed! ===" << std::endl;
    return 0;
}
