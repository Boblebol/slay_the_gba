// Cards, relics and potions built on the statuses of #91, #92 and #93: one case each.
#include <algorithm>
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/potion.h"
#include "engine/relic.h"
#include "engine/run.h"

using namespace engine;

namespace {

// Jaw Worm opens with Chomp (11 damage, no block): a predictable punching bag.
CombatEngine make_combat(CharacterId hero, EnemyType enemy_type = EnemyType::JAW_WORM) {
    CombatEngine combat;
    combat.start_combat(enemy_type, hero);
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

void make_enemy_idle(Enemy& enemy) {
    enemy.current_intent = EnemyIntent{};
    enemy.current_intent.type = EnemyIntentType::DEFEND;
}

void end_turn_idle(CombatEngine& combat) {
    for (Enemy& foe : combat.enemies) make_enemy_idle(foe);
    combat.end_player_turn();
}

// Ends the turn with the first enemy attacking for `damage`.
void end_turn_attacked(CombatEngine& combat, int16_t damage) {
    for (Enemy& foe : combat.enemies) make_enemy_idle(foe);
    combat.enemies[0].current_intent.type = EnemyIntentType::ATTACK;
    combat.enemies[0].current_intent.base_damage = damage;
    combat.end_player_turn();
}

void pad_draw_pile(CombatEngine& combat, int count) {
    for (int i = 0; i < count; ++i) combat.draw_pile.push_back(CardId::DEFEND);
}

// Enters the first map room after forcing its type, like a player walking into it.
void enter_room_of_type(RunManager& run, RoomType type) {
    int index = type == RoomType::BOSS ? run.map.boss_node_index : 0;
    run.map.nodes[index].type = type;
    run.enter_room(index);
}

// A run of `hero` holding `relics`, standing in its first fight.
RunManager make_relic_run(CharacterId hero, std::initializer_list<std::string_view> relics) {
    RunManager run;
    run.start_run(hero, 0, PaletteVariant::CLASSIC, 777);
    for (std::string_view relic : relics) run.add_relic(relic);
    enter_room_of_type(run, RoomType::MONSTER);
    return run;
}

bool in_pool(CharacterId hero, CardId id) {
    std::span<const CardId> pool = RunManager::card_reward_pool(hero);
    return std::find(pool.begin(), pool.end(), id) != pool.end();
}

void check_def(CardId id, CardType type, uint8_t cost, uint8_t value, uint8_t upgraded_value, uint8_t secondary = 0,
               uint8_t upgraded_secondary = 0) {
    const CardDef& def = CardDef::get(id);
    assert(def.id == id && def.type == type && def.target == TargetType::SELF);
    assert(def.cost == cost && def.upgraded_cost == cost);
    assert(def.base_value == value && def.upgraded_base_value == upgraded_value);
    assert(def.secondary_value == secondary && def.upgraded_secondary_value == upgraded_secondary);
    assert(def.keywords == 0 && def.upgraded_keywords == 0);
}

void test_plating_cards() {
    check_def(CardId::STONE_ARMOR, CardType::POWER, 1, 4, 6);
    check_def(CardId::NEUTRON_AEGIS, CardType::POWER, 1, 8, 11);
    assert(in_pool(CharacterId::IRONCLAD, CardId::STONE_ARMOR) && in_pool(CharacterId::REGENT, CardId::NEUTRON_AEGIS));

    CombatEngine stone = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(stone, 20);
    play(stone, CardId::STONE_ARMOR);
    assert(stone.player.get_status(StatusType::PLATED_ARMOR) == 4 && stone.player.energy == 2);
    assert(stone.powers_in_play.size() == 1 && stone.discard_pile.empty());
    play(stone, DeckCard(CardId::STONE_ARMOR, true));
    assert(stone.player.get_status(StatusType::PLATED_ARMOR) == 10);
    int16_t hp = stone.player.hp;
    end_turn_attacked(stone, 12);
    assert(stone.player.hp == hp - 2 && stone.player.get_status(StatusType::PLATED_ARMOR) == 9);

    CombatEngine aegis = make_combat(CharacterId::REGENT);
    play(aegis, CardId::NEUTRON_AEGIS);
    assert(aegis.player.get_status(StatusType::PLATED_ARMOR) == 8);
    play(aegis, DeckCard(CardId::NEUTRON_AEGIS, true));
    assert(aegis.player.get_status(StatusType::PLATED_ARMOR) == 19);
    std::cout << "[PASS] Stone Armor, Neutron Aegis: Plating" << std::endl;
}

void test_flame_barrier() {
    check_def(CardId::FLAME_BARRIER, CardType::SKILL, 2, 12, 16, 4, 6);
    assert(in_pool(CharacterId::IRONCLAD, CardId::FLAME_BARRIER));

    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(combat, 20);
    play(combat, CardId::FLAME_BARRIER);
    assert(combat.player.block == 12 && combat.player.energy == 1);
    assert(combat.power_amount(PowerId::FLAME_BARRIER) == 4);
    int16_t hp = combat.player.hp;
    int16_t foe_hp = combat.enemies[0].hp;
    end_turn_attacked(combat, 5);
    assert(combat.player.hp == hp && combat.enemies[0].hp == foe_hp - 4);
    // Over at the start of the next turn: the next attack is not answered.
    assert(!combat.has_power(PowerId::FLAME_BARRIER));
    end_turn_attacked(combat, 5);
    assert(combat.enemies[0].hp == foe_hp - 4);

    // Upgraded, on top of Thorns; an enemy that does not attack takes nothing.
    play(combat, DeckCard(CardId::FLAME_BARRIER, true));
    assert(combat.player.block == 16);
    combat.player.add_status(StatusType::THORNS, 3);
    end_turn_idle(combat);
    assert(combat.enemies[0].hp == foe_hp - 4);
    play(combat, DeckCard(CardId::FLAME_BARRIER, true));
    end_turn_attacked(combat, 5);
    assert(combat.enemies[0].hp == foe_hp - 4 - 9);
    std::cout << "[PASS] Flame Barrier: Block, and damage back to the attacks until the next turn" << std::endl;
}

void test_reflect() {
    check_def(CardId::REFLECT, CardType::SKILL, 1, 15, 20);
    assert(in_pool(CharacterId::REGENT, CardId::REFLECT));

    CombatEngine combat = make_combat(CharacterId::REGENT);
    pad_draw_pile(combat, 20);
    combat.enemies[0].hp = 100;
    play(combat, CardId::REFLECT);
    assert(combat.player.block == 15 && combat.has_power(PowerId::REFLECT));
    int16_t hp = combat.player.hp;
    // 9 damage, all blocked: 9 sent back.
    end_turn_attacked(combat, 9);
    assert(combat.player.hp == hp && combat.enemies[0].hp == 91);
    assert(!combat.has_power(PowerId::REFLECT));

    // 25 damage on 20 Block: only the 20 blocked go back.
    play(combat, DeckCard(CardId::REFLECT, true));
    assert(combat.player.block == 20);
    end_turn_attacked(combat, 25);
    assert(combat.player.hp == hp - 5 && combat.enemies[0].hp == 71);

    // Without the card, Block sends nothing back.
    play(combat, CardId::DEFEND);
    end_turn_attacked(combat, 5);
    assert(combat.enemies[0].hp == 71);
    std::cout << "[PASS] Reflect: the attack damage absorbed by Block goes back to the attacker" << std::endl;
}

void test_buffer_card() {
    check_def(CardId::BUFFER, CardType::POWER, 2, 1, 2);
    assert(in_pool(CharacterId::DEFECT, CardId::BUFFER));

    CombatEngine combat = make_combat(CharacterId::DEFECT);
    pad_draw_pile(combat, 20);
    play(combat, CardId::BUFFER);
    assert(combat.player.get_status(StatusType::BUFFER) == 1 && combat.player.energy == 1);
    int16_t hp = combat.player.hp;
    end_turn_attacked(combat, 30);
    assert(combat.player.hp == hp && combat.player.get_status(StatusType::BUFFER) == 0);
    play(combat, DeckCard(CardId::BUFFER, true));
    assert(combat.player.get_status(StatusType::BUFFER) == 2);
    std::cout << "[PASS] Buffer: the next HP loss is cancelled" << std::endl;
}

void test_wraith_form() {
    check_def(CardId::WRAITH_FORM, CardType::POWER, 3, 2, 3, 1, 1);
    // Ancient rarity: never a combat reward.
    assert(!in_pool(CharacterId::SILENT, CardId::WRAITH_FORM));

    CombatEngine combat = make_combat(CharacterId::SILENT);
    pad_draw_pile(combat, 30);
    play(combat, CardId::WRAITH_FORM);
    assert(combat.player.get_status(StatusType::INTANGIBLE) == 2 && combat.player.energy == 0);
    assert(combat.power_amount(PowerId::WRAITH_FORM) == 1);
    int16_t hp = combat.player.hp;
    end_turn_attacked(combat, 30);
    assert(combat.player.hp == hp - 1);
    assert(combat.player.get_status(StatusType::INTANGIBLE) == 1);
    assert(combat.player.get_status(StatusType::DEXTERITY) == -1);
    end_turn_attacked(combat, 30);
    assert(combat.player.hp == hp - 2);
    assert(combat.player.get_status(StatusType::INTANGIBLE) == 0);
    assert(combat.player.get_status(StatusType::DEXTERITY) == -2);

    // Artifact cancels the Dexterity loss.
    CombatEngine kept = make_combat(CharacterId::SILENT);
    pad_draw_pile(kept, 30);
    play(kept, DeckCard(CardId::WRAITH_FORM, true));
    assert(kept.player.get_status(StatusType::INTANGIBLE) == 3);
    kept.player.add_status(StatusType::ARTIFACT, 1);
    end_turn_idle(kept);
    assert(kept.player.get_status(StatusType::DEXTERITY) == 0 && kept.player.get_status(StatusType::ARTIFACT) == 0);
    std::cout << "[PASS] Wraith Form: Intangible, 1 Dexterity lost each turn" << std::endl;
}

void test_vigor_cards() {
    check_def(CardId::PATTER, CardType::SKILL, 1, 8, 10, 2, 3);
    check_def(CardId::TERRAFORMING, CardType::SKILL, 1, 7, 10);
    assert(in_pool(CharacterId::REGENT, CardId::PATTER) && in_pool(CharacterId::REGENT, CardId::TERRAFORMING));

    CombatEngine combat = make_combat(CharacterId::REGENT);
    combat.enemies[0].hp = 100;
    play(combat, CardId::PATTER);
    assert(combat.player.block == 8 && combat.player.get_status(StatusType::VIGOR) == 2);
    play(combat, DeckCard(CardId::PATTER, true));
    assert(combat.player.block == 18 && combat.player.get_status(StatusType::VIGOR) == 5);
    play(combat, CardId::TERRAFORMING);
    assert(combat.player.get_status(StatusType::VIGOR) == 12);
    play(combat, DeckCard(CardId::TERRAFORMING, true));
    assert(combat.player.get_status(StatusType::VIGOR) == 22);
    assert(combat.discard_pile.size() == 4);
    play(combat, CardId::STRIKE);
    assert(combat.enemies[0].hp == 100 - 28 && combat.player.get_status(StatusType::VIGOR) == 0);
    std::cout << "[PASS] Patter, Terraforming: Vigor for the next Attack" << std::endl;
}

void test_relics() {
    for (std::string_view id : { "BRONZE_SCALES", "GORGET", "AKABEKO", "FAKE_SNECKO_EYE", "SNECKO_EYE" }) {
        assert(find_relic(id) != nullptr);
    }

    RunManager scales = make_relic_run(CharacterId::IRONCLAD, { "BRONZE_SCALES" });
    assert(scales.combat.player.get_status(StatusType::THORNS) == 3);

    // Gorget: the 4 Plating are whole when the first turn ends.
    RunManager gorget = make_relic_run(CharacterId::IRONCLAD, { "GORGET" });
    assert(gorget.combat.player.get_status(StatusType::PLATED_ARMOR) == 4);
    end_turn_idle(gorget.combat);
    assert(gorget.combat.player.get_status(StatusType::PLATED_ARMOR) == 3);

    RunManager akabeko = make_relic_run(CharacterId::IRONCLAD, { "AKABEKO" });
    CombatEngine& fight = akabeko.combat;
    assert(fight.player.get_status(StatusType::VIGOR) == 8);
    fight.enemies[0].hp = 100;
    fight.enemies[0].block = 0;
    play(fight, CardId::STRIKE);
    assert(fight.enemies[0].hp == 100 - 14 && fight.player.get_status(StatusType::VIGOR) == 0);

    // Confused from the first draw: the opening hand of Bludgeons (3 energy) does not all cost 3 on every seed.
    for (std::string_view eye : { "FAKE_SNECKO_EYE", "SNECKO_EYE" }) {
        RunManager run;
        run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
        run.master_deck.clear();
        for (int i = 0; i < 30; ++i) run.master_deck.push_back(DeckCard(CardId::BLUDGEON));
        run.add_relic(eye);
        enter_room_of_type(run, RoomType::MONSTER);
        assert(run.combat.player.get_status(StatusType::CONFUSED) == 1);
        const bool snecko = eye == "SNECKO_EYE";
        assert(run.combat.hand.size() == (snecko ? 7u : 5u));
        assert(run.combat.power_amount(PowerId::TURN_DRAW) == (snecko ? 2 : 0));
        bool changed = false;
        for (const DeckCard& card : run.combat.hand) {
            int cost = run.combat.effective_cost(card);
            assert(cost >= 0 && cost <= 3);
            changed = changed || cost != 3;
        }
        assert(changed);
        end_turn_idle(run.combat);
        assert(run.combat.hand.size() == (snecko ? 7u : 5u));
    }

    // A run without them starts with no status.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(plain.combat.player.statuses.empty());
    std::cout << "[PASS] Bronze Scales, Gorget, Akabeko, Fake Snecko Eye, Snecko Eye" << std::endl;
}

void test_potions() {
    struct Case {
        std::string_view id;
        StatusType status;
        int16_t amount;
    };
    constexpr Case CASES[] = {
        { "HEART_OF_IRON", StatusType::PLATED_ARMOR, 7 }, { "LIQUID_BRONZE", StatusType::THORNS, 3 },
        { "GHOST_IN_A_JAR", StatusType::INTANGIBLE, 1 },  { "LUCKY_TONIC", StatusType::BUFFER, 1 },
        { "REGEN_POTION", StatusType::REGEN, 5 },
    };
    for (const Case& potion : CASES) {
        assert(find_potion(potion.id) != nullptr);
        CombatEngine combat = make_combat(CharacterId::IRONCLAD);
        assert(combat.use_potion(potion.id));
        assert(combat.player.get_status(potion.status) == potion.amount && combat.player.statuses.size() == 1);
        assert(combat.state == CombatState::PLAYER_TURN && combat.player.energy == 3 && combat.hand.size() == 5);
        // Refused outside the player's turn.
        CombatEngine waiting = make_combat(CharacterId::IRONCLAD);
        waiting.state = CombatState::ENEMY_TURN;
        assert(!waiting.use_potion(potion.id) && waiting.player.statuses.empty());
    }

    // Ghost in a Jar covers the enemy turn that follows; the Regen Potion heals 5 + 4 + 3 + 2 + 1.
    CombatEngine ghost = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(ghost, 30);
    ghost.use_potion("GHOST_IN_A_JAR");
    int16_t hp = ghost.player.hp;
    end_turn_attacked(ghost, 40);
    assert(ghost.player.hp == hp - 1);

    CombatEngine regen = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(regen, 40);
    regen.player.hp = 20;
    regen.use_potion("REGEN_POTION");
    for (int turn = 0; turn < 6; ++turn) end_turn_idle(regen);
    assert(regen.player.hp == 35 && regen.player.statuses.empty());
    std::cout << "[PASS] Heart of Iron, Liquid Bronze, Ghost in a Jar, Lucky Tonic, Regen Potion" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Status Content ===" << std::endl;
    test_plating_cards();
    test_flame_barrier();
    test_reflect();
    test_buffer_card();
    test_wraith_form();
    test_vigor_cards();
    test_relics();
    test_potions();
    std::cout << "All status content tests passed." << std::endl;
    return 0;
}
