#pragma once
// Mannequin de combat usé (BATTLEWORN_DUMMY), Act 3. Choices: bible/data/event_choices/a.json.
// Three settings, each a fight against the dummy that lasts 3 turns (it leaves after the third, unbeaten, and then
// gives nothing): 75 HP for a random potion, 150 HP for 2 random cards upgraded, 300 HP for a random relic. The dummy
// never attacks (its only move is Nothing) and there is no "leave" option, as in the reference.
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
      .cost_benefit = "150 PV, 3 tours. Victoire : 2 cartes améliorées au hasard.",
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
