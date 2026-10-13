#include "engine/run_end.h"

#include <algorithm>

#include "engine/save_system.h"

namespace engine {

RunEndCause run_end_cause(const RunManager& run, bool victory) {
    if (victory) return RunEndCause::VICTORY;
    // The pause menu marks an abandoned run; the phase of a fight lost is still IN_COMBAT.
    if (run.phase == RunPhase::GAME_OVER) return RunEndCause::ABANDON;
    if (run.phase == RunPhase::IN_COMBAT && run.combat.state == CombatState::DEFEAT) return RunEndCause::COMBAT;
    return run.hp <= 0 ? RunEndCause::EVENT : RunEndCause::ABANDON;
}

uint32_t run_score(const RunSummary& summary) {
    const uint32_t normal_fights =
        summary.combats_won - std::min<uint32_t>(summary.combats_won, summary.elites_won + summary.bosses_won);
    uint32_t score = summary.floors_climbed * SCORE_PER_FLOOR + normal_fights * SCORE_PER_COMBAT +
                     summary.elites_won * SCORE_PER_ELITE + summary.bosses_won * SCORE_PER_BOSS;
    if (summary.cause == RunEndCause::VICTORY) score += SCORE_VICTORY;
    return score * (100 + summary.ascension * SCORE_ASCENSION_PERCENT) / 100;
}

RunSummary summarize_run(const RunManager& run, bool victory) {
    RunSummary summary;
    summary.cause = run_end_cause(run, victory);
    summary.act = run.current_act;
    summary.ascension = run.ascension_level;
    // The floor of the act counts once its room is entered: a new act starts before its first room.
    const int act_floor = run.current_node_index >= 0 ? run.current_floor : 0;
    summary.floors_climbed = static_cast<uint16_t>(std::max(0, run.current_act - 1) * FLOORS_PER_ACT + act_floor);
    summary.combats_won = run.combats_won;
    summary.elites_won = run.elites_won;
    summary.bosses_won = run.bosses_won;
    summary.deck_size = static_cast<uint8_t>(run.master_deck.size());
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].upgraded) ++summary.upgraded_cards;
    }
    summary.relic_count = static_cast<uint8_t>(run.relics.size());
    summary.gold = run.gold;
    summary.play_seconds = run.play_frames / PLAY_FRAMES_PER_SECOND;

    if (summary.cause == RunEndCause::COMBAT && !run.combat.enemies.empty()) {
        // The enemy still standing; the first one when every enemy fell with the hero.
        summary.killer = run.combat.enemies[0].name;
        for (size_t i = 0; i < run.combat.enemies.size(); ++i) {
            if (run.combat.enemies[i].is_alive()) {
                summary.killer = run.combat.enemies[i].name;
                break;
            }
        }
    } else if (summary.cause == RunEndCause::EVENT) {
        summary.killer = run.current_event.title;
    }

    summary.score = run_score(summary);
    return summary;
}

RunRecord record_run_end(const RunManager& run, bool victory) {
    SaveManager& save = SaveManager::instance();
    RunRecord record;
    record.summary = summarize_run(run, victory);
    record.xp_before = save.profile_xp();
    record.ascension_before = save.unlocked_ascension(run.character);
    save.add_profile_xp(record.summary.score);
    // Both write the profile to the cartridge, the experience with it.
    if (victory) {
        save.record_victory(run.character, run.ascension_level);
    } else {
        save.record_defeat();
    }
    record.xp_after = save.profile_xp();
    record.ascension_after = save.unlocked_ascension(run.character);
    return record;
}

} // namespace engine
