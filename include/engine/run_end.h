#pragma once

#include <cstdint>
#include <string_view>

#include "engine/run.h"

// End of a run (#11 defeat, #12 victory): what the end screen sums up, the score, the experience it gives to the
// profile and the menu that follows. The GBA scene only draws what is computed here.
namespace engine
{

// Why the run is over.
enum class RunEndCause : uint8_t
{
    VICTORY, // the boss of act 3 is beaten
    COMBAT,  // the hero fell in a fight
    EVENT,   // the hero fell to the choice of an event
    ABANDON  // given up from the pause menu
};

inline constexpr int FLOORS_PER_ACT = 16;
inline constexpr uint32_t PLAY_FRAMES_PER_SECOND = 60;

// Score (and experience) of a run: each floor climbed, each fight won, more for the elites and the bosses, a bonus
// for the victory, then 5 % more per Ascension level.
inline constexpr uint32_t SCORE_PER_FLOOR = 5;
inline constexpr uint32_t SCORE_PER_COMBAT = 2;
inline constexpr uint32_t SCORE_PER_ELITE = 10;
inline constexpr uint32_t SCORE_PER_BOSS = 50;
inline constexpr uint32_t SCORE_VICTORY = 250;
inline constexpr uint32_t SCORE_ASCENSION_PERCENT = 5;

// Experience the profile needs for each level.
inline constexpr uint32_t XP_PER_LEVEL = 500;

struct RunSummary
{
    RunEndCause cause = RunEndCause::ABANDON;
    uint8_t act = 1;
    uint8_t ascension = 0;
    uint16_t floors_climbed = 0; // over every act
    uint8_t combats_won = 0;
    uint8_t elites_won = 0;
    uint8_t bosses_won = 0;
    uint8_t deck_size = 0;
    uint8_t upgraded_cards = 0;
    uint8_t relic_count = 0;
    int16_t gold = 0;
    uint32_t play_seconds = 0;
    uint32_t score = 0;
    // COMBAT: name of the enemy that won the fight. EVENT: title of the event. Empty otherwise.
    std::string_view killer;
};

[[nodiscard]] constexpr uint32_t profile_level(uint32_t xp)
{
    return 1 + xp / XP_PER_LEVEL;
}

// Experience gathered towards the next level.
[[nodiscard]] constexpr uint32_t profile_level_progress(uint32_t xp)
{
    return xp % XP_PER_LEVEL;
}

// Why `run` is over: a victory when `victory` is set, otherwise read from the phase and the last fight.
[[nodiscard]] RunEndCause run_end_cause(const RunManager& run, bool victory);

// Score of the counts of `summary` (its `score` is ignored).
[[nodiscard]] uint32_t run_score(const RunSummary& summary);

[[nodiscard]] RunSummary summarize_run(const RunManager& run, bool victory);

// What ending the run changed in the profile.
struct RunRecord
{
    RunSummary summary;
    uint32_t xp_before = 0;
    uint32_t xp_after = 0;
    uint8_t ascension_before = 0; // highest Ascension the hero could start with
    uint8_t ascension_after = 0;
};

// Ends the run once: the profile gains the score as experience, counts the run (and the victory, which unlocks the
// next Ascension) and the saved run is erased.
RunRecord record_run_end(const RunManager& run, bool victory);

// Menu of the end screen.
enum class EndItem : uint8_t
{
    CREDITS,  // victory only: the end credits, then the title
    NEW_RUN,  // a new run with the same hero, colour and Ascension
    MAIN_MENU // the menu of the title screen
};

inline constexpr int END_MAX_ITEMS = 3;

struct EndMenu
{
    EndItem items[END_MAX_ITEMS] = {};
    int count = 0;
    int cursor = 0;
};

[[nodiscard]] constexpr EndMenu make_end_menu(bool victory)
{
    EndMenu menu;

    if(victory)
    {
        menu.items[menu.count++] = EndItem::CREDITS;
    }

    menu.items[menu.count++] = EndItem::NEW_RUN;
    menu.items[menu.count++] = EndItem::MAIN_MENU;
    return menu;
}

// Moves the cursor by `delta` entries, wrapping around.
constexpr void move_end_cursor(EndMenu& menu, int delta)
{
    if(menu.count <= 0)
    {
        return;
    }

    menu.cursor = ((menu.cursor + delta) % menu.count + menu.count) % menu.count;
}

[[nodiscard]] constexpr const char* end_label(EndItem item)
{
    switch(item)
    {
    case EndItem::CREDITS:
        return "Voir les crédits";
    case EndItem::NEW_RUN:
        return "Recommencer une partie";
    default:
        return "Menu principal";
    }
}

}
