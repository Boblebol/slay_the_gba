#pragma once
// Mannequin de combat usé (BATTLEWORN_DUMMY), Act 3. Choices: bible/data/event_choices/a.json.
// Three settings, each a fight against the dummy that lasts 3 turns (it leaves after the third, unbeaten, and then
// gives nothing): 75 HP for a potion, 150 HP for 2 upgrades, 300 HP for a relic. Guessed: the dummy never attacks (the
// reference lists no move), the 2 upgrades are picked by the player (the reference draws them), and there is no
// "leave" option (the reference has none either).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect BATTLEWORN_DUMMY_SETTING_1[] = {
    { .type = EventEffectType::FIGHT, .amount = 1, .ref = EVENT_FIGHT_BATTLEWORN_DUMMY },
};

inline constexpr EventEffect BATTLEWORN_DUMMY_SETTING_2[] = {
    { .type = EventEffectType::FIGHT, .amount = 2, .ref = EVENT_FIGHT_BATTLEWORN_DUMMY },
};

inline constexpr EventEffect BATTLEWORN_DUMMY_SETTING_3[] = {
    { .type = EventEffectType::FIGHT, .amount = 3, .ref = EVENT_FIGHT_BATTLEWORN_DUMMY },
};

inline constexpr EventOption BATTLEWORN_DUMMY_START[] = {
    { .label = "Réglage 1",
      .cost_benefit = "75 PV, 3 tours. Victoire : une potion.",
      .result = "Le mannequin cliquette et se met en position. Le compte à rebours commence !",
      .effects = BATTLEWORN_DUMMY_SETTING_1 },
    { .label = "Réglage 2",
      .cost_benefit = "150 PV, 3 tours. Victoire : 2 améliorations.",
      .result = "Le mannequin cliquette et se met en position. Le compte à rebours commence !",
      .effects = BATTLEWORN_DUMMY_SETTING_2 },
    { .label = "Réglage 3",
      .cost_benefit = "300 PV, 3 tours. Victoire : une relique.",
      .result = "Le mannequin cliquette et se met en position. Le compte à rebours commence !",
      .effects = BATTLEWORN_DUMMY_SETTING_3 },
};

inline constexpr EventPage BATTLEWORN_DUMMY_PAGES[] = {
    { .text = "Le mannequin grésille, bourdonne, puis s'illumine : « BZZZT ! SÉQUENCE D'ENTRAÎNEMENT INITIÉE ! VOUS AVEZ "
              "3 TOURS POUR ME VAINCRE. RÉGLEZ LE PARAMÈTRE OU SUBISSEZ UNE HUMILIATION LÉTALE. »",
      .options = BATTLEWORN_DUMMY_START },
};

inline constexpr EventDef BATTLEWORN_DUMMY = {
    .id = "BATTLEWORN_DUMMY",
    .title = "Mannequin de combat usé",
    .acts = EVENT_ACT_3,
    .pages = BATTLEWORN_DUMMY_PAGES,
};

} // namespace engine::events
