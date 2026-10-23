// End of a run (#11 defeat, #12 victory): run statistics, cause of the end, score, experience of the profile,
// Ascension unlocked and the menu of the end screen.
#include <cassert>
#include <cstdint>
#include <iostream>
#include "engine/run.h"
#include "engine/run_end.h"
#include "engine/save_system.h"

using namespace engine;

namespace {

int find_room(const MapGraph& map, RoomType type) {
    for (size_t i = 0; i < map.nodes.size(); ++i) {
        if (map.nodes[i].type == type) return static_cast<int>(i);
    }
    return -1;
}

// A run standing in a room of `type` of `act`, its fight just started.
RunManager run_in_room(RoomType type, uint8_t act, uint8_t ascension = 0) {
    RunManager run;
    for (uint32_t seed = 1; seed < 200; ++seed) {
        run.start_run(CharacterId::IRONCLAD, ascension, PaletteVariant::CLASSIC, seed);
        run.current_act = act;
        run.map.generate(act, seed * 7919u, ascension);
        if (find_room(run.map, type) >= 0) break;
    }
    run.current_node_index = find_room(run.map, type);
    assert(run.current_node_index >= 0);
    run.current_floor = static_cast<uint8_t>(run.map.nodes[run.current_node_index].floor + 1);
    run.phase = RunPhase::IN_COMBAT;
    run.combat.player.hp = run.hp;
    run.combat.player.max_hp = run.max_hp;
    run.combat.state = CombatState::VICTORY;
    return run;
}

void fresh_profile() {
    SaveManager& save = SaveManager::instance();
    save.init();
    save.reset_to_defaults();
    save.select_profile(0);
}

void test_statistics_follow_the_run() {
    RunManager run = run_in_room(RoomType::ELITE, 1);
    assert(run.combats_won == 0 && run.elites_won == 0 && run.bosses_won == 0 && run.play_frames == 0);
    run.proceed_after_combat();
    assert(run.combats_won == 1 && run.elites_won == 1 && run.bosses_won == 0);

    RunManager boss = run_in_room(RoomType::BOSS, 3);
    boss.combats_won = 20;
    boss.elites_won = 3;
    boss.bosses_won = 2;
    boss.proceed_after_combat();
    assert(boss.phase == RunPhase::VICTORY_ACT);
    assert(boss.combats_won == 21 && boss.elites_won == 3 && boss.bosses_won == 3);

    // Play time: counted frame by frame, never wraps around, and a new run starts again from zero.
    run.count_play_frame();
    run.count_play_frame();
    assert(run.play_frames == 2);
    run.play_frames = UINT32_MAX;
    run.count_play_frame();
    assert(run.play_frames == UINT32_MAX);
    run.start_run(CharacterId::IRONCLAD);
    assert(run.play_frames == 0 && run.combats_won == 0 && run.elites_won == 0 && run.bosses_won == 0);
    std::cout << "  statistics: OK\n";
}

void test_cause_and_killer() {
    // Lost fight: the enemy still standing is named.
    RunManager run = run_in_room(RoomType::MONSTER, 1);
    run.combat.state = CombatState::DEFEAT;
    run.combat.player.hp = 0;
    run.combat.enemies.clear();
    Enemy dead;
    dead.name = "Premier";
    dead.hp = 0;
    Enemy alive;
    alive.name = "Second";
    alive.hp = 12;
    run.combat.enemies.push_back(dead);
    run.combat.enemies.push_back(alive);
    RunSummary summary = summarize_run(run, false);
    assert(summary.cause == RunEndCause::COMBAT);
    assert(summary.killer == "Second");

    // The last HP came from the hero's own card or effect: no enemy is blamed.
    run.combat.last_hp_loss_source = CombatEngine::SELF_INFLICTED_HP_LOSS;
    summary = summarize_run(run, false);
    assert(summary.cause == RunEndCause::COMBAT && summary.self_inflicted && summary.killer.empty());
    // The last HP came from the attack of the second enemy, even if the first still stands.
    run.combat.last_hp_loss_source = 1;
    summary = summarize_run(run, false);
    assert(!summary.self_inflicted && summary.killer == "Second");

    // Abandoned from the pause menu, even in the middle of a fight.
    run.phase = RunPhase::GAME_OVER;
    summary = summarize_run(run, false);
    assert(summary.cause == RunEndCause::ABANDON && summary.killer.empty());

    // Killed by the choice of an event.
    run.phase = RunPhase::MAP_NAVIGATION;
    run.hp = 0;
    run.current_event.title = "Bains abyssaux";
    summary = summarize_run(run, false);
    assert(summary.cause == RunEndCause::EVENT && summary.killer == "Bains abyssaux");

    // Victory: no killer.
    summary = summarize_run(run, true);
    assert(summary.cause == RunEndCause::VICTORY && summary.killer.empty());
    std::cout << "  cause and killer: OK\n";
}

void test_engine_records_who_took_the_last_hp() {
    CombatEngine combat;
    combat.start_combat(EnemyType::CULTIST, CharacterId::IRONCLAD);
    assert(combat.last_hp_loss_source == CombatEngine::NO_HP_LOSS_YET);
    // A card of the hero that costs HP (Hemokinesis at 2 HP).
    combat.player.hp = 2;
    combat.lose_hp(2);
    assert(combat.state == CombatState::DEFEAT);
    assert(combat.last_hp_loss_source == CombatEngine::SELF_INFLICTED_HP_LOSS);

    // An enemy attack that finishes the hero names that enemy (the Jaw Worm opens with Chomp).
    CombatEngine attacked;
    attacked.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    attacked.player.hp = 1;
    attacked.player.block = 0;
    attacked.end_player_turn();
    assert(attacked.state == CombatState::DEFEAT);
    assert(attacked.last_hp_loss_source == 0);
    std::cout << "  engine records the source: OK\n";
}

void test_summary_and_score() {
    RunManager run = run_in_room(RoomType::BOSS, 3, 4);
    run.master_deck.clear();
    run.master_deck.push_back(DeckCard(CardId::STRIKE, true));
    run.master_deck.push_back(DeckCard(CardId::STRIKE, false));
    run.master_deck.push_back(DeckCard(CardId::BASH, true));
    run.relics.push_back("ANCHOR");
    run.gold = 321;
    run.play_frames = 60 * 3725 + 59; // 1 h 2 min 5 s and a bit
    run.combats_won = 15;
    run.elites_won = 3;
    run.bosses_won = 3;

    RunSummary summary = summarize_run(run, true);
    assert(summary.act == 3 && summary.ascension == 4);
    assert(summary.floors_climbed == 2 * FLOORS_PER_ACT + run.current_floor);
    assert(summary.deck_size == 3 && summary.upgraded_cards == 2);
    assert(summary.relic_count == run.relics.size() && summary.gold == 321);
    assert(summary.play_seconds == 3725);
    const uint32_t raw = summary.floors_climbed * SCORE_PER_FLOOR + 9 * SCORE_PER_COMBAT + 3 * SCORE_PER_ELITE +
                         3 * SCORE_PER_BOSS + SCORE_VICTORY;
    assert(summary.score == raw * 120 / 100);
    assert(summary.score == run_score(summary));

    // A new act starts before its first room: the floor of that act is not climbed yet.
    run.current_node_index = -1;
    run.current_floor = 1;
    assert(summarize_run(run, false).floors_climbed == 2 * FLOORS_PER_ACT);

    // Defeat: no victory bonus; the counts never go below zero.
    RunSummary loss;
    loss.cause = RunEndCause::COMBAT;
    loss.floors_climbed = 4;
    loss.combats_won = 1;
    loss.elites_won = 1;
    loss.bosses_won = 1;
    assert(run_score(loss) == 4 * SCORE_PER_FLOOR + SCORE_PER_ELITE + SCORE_PER_BOSS);
    std::cout << "  summary and score: OK\n";
}

void test_record_gives_experience_and_ascension() {
    fresh_profile();
    SaveManager& save = SaveManager::instance();
    assert(save.profile_xp() == 0);

    RunManager run = run_in_room(RoomType::MONSTER, 1);
    run.combats_won = 2;
    save.autosave_run(run);
    assert(save.has_active_run());
    run.phase = RunPhase::GAME_OVER;
    RunRecord defeat = record_run_end(run, false);
    assert(!save.has_active_run());
    assert(defeat.xp_before == 0 && defeat.xp_after == defeat.summary.score && defeat.summary.score > 0);
    assert(defeat.ascension_before == 0 && defeat.ascension_after == 0);
    assert(save.get_active_profile().total_runs == 1 && save.get_active_profile().total_victories == 0);

    // The experience stays with the profile through the autosaves of the next run.
    RunManager next = run_in_room(RoomType::BOSS, 3);
    save.autosave_run(next);
    assert(save.profile_xp() == defeat.xp_after);
    RunRecord victory = record_run_end(next, true);
    assert(victory.xp_before == defeat.xp_after);
    assert(victory.xp_after == defeat.xp_after + victory.summary.score);
    assert(victory.ascension_before == 0 && victory.ascension_after == 1);
    assert(save.get_active_profile().total_runs == 2 && save.get_active_profile().total_victories == 1);

    // Levels.
    assert(profile_level(0) == 1 && profile_level(XP_PER_LEVEL - 1) == 1 && profile_level(XP_PER_LEVEL) == 2);
    assert(profile_level_progress(XP_PER_LEVEL + 7) == 7);

    // Never wraps around.
    save.add_profile_xp(UINT32_MAX);
    assert(save.profile_xp() == UINT32_MAX);

    // A new profile in the slot starts from zero.
    save.create_profile(0, "Nouveau");
    assert(save.profile_xp() == 0);
    std::cout << "  experience and Ascension: OK\n";
}

void test_statistics_saved_with_the_run() {
    fresh_profile();
    SaveManager& save = SaveManager::instance();
    save.add_profile_xp(1234);
    RunManager run;
    run.start_run(CharacterId::IRONCLAD);
    run.play_frames = 0x01020304u;
    run.combats_won = 7;
    run.elites_won = 2;
    run.bosses_won = 1;
    save.autosave_run(run);

    RunManager back;
    assert(save.restore_run(back));
    assert(back.play_frames == 0x01020304u);
    assert(back.combats_won == 7 && back.elites_won == 2 && back.bosses_won == 1);
    assert(save.profile_xp() == 1234);
    std::cout << "  save round trip: OK\n";
}

void test_end_menu() {
    EndMenu won = make_end_menu(true);
    assert(won.count == 3 && won.cursor == 0);
    assert(won.items[0] == EndItem::CREDITS && won.items[1] == EndItem::NEW_RUN && won.items[2] == EndItem::MAIN_MENU);
    move_end_cursor(won, -1);
    assert(won.cursor == 2);
    move_end_cursor(won, 1);
    assert(won.cursor == 0);

    EndMenu lost = make_end_menu(false);
    assert(lost.count == 2 && lost.items[0] == EndItem::NEW_RUN && lost.items[1] == EndItem::MAIN_MENU);
    move_end_cursor(lost, 1);
    move_end_cursor(lost, 1);
    assert(lost.cursor == 0);
    std::cout << "  end menu: OK\n";
}

} // namespace

int main() {
    std::cout << "Run end (#11, #12)\n";
    test_statistics_follow_the_run();
    test_cause_and_killer();
    test_engine_records_who_took_the_last_hp();
    test_summary_and_score();
    test_record_gives_experience_and_ascension();
    test_statistics_saved_with_the_run();
    test_end_menu();
    std::cout << "[PASS] run_end_test\n";
    return 0;
}
