#pragma once
// Jungle dense (DENSE_VEGETATION), Act 1 (A). Choices: bible/data/event_choices/a.json.
// The reference rerolls the card shown for removal on each "Hold On"; the game has no preview, so "Surmonter" removes
// a random card and "S'accrocher" only costs HP: 3, then 4, then 5 HP for every further hold (pages 1 and 2).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect DENSE_VEGETATION_HOLD_1[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 3 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 1 },
};

inline constexpr EventEffect DENSE_VEGETATION_HOLD_2[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 4 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 2 },
};

inline constexpr EventEffect DENSE_VEGETATION_HOLD_3[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 5 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 2 },
};

inline constexpr EventEffect DENSE_VEGETATION_OVERCOME[] = {
    { .type = EventEffectType::REMOVE_CARD, .amount = 1, .pick = EventPick::RANDOM },
};

inline constexpr EventOption DENSE_VEGETATION_START[] = {
    { .label = "S'accrocher",
      .cost_benefit = "Perdez 3 PV, relancez la carte.",
      .result = "Vous vous accrochez aux lianes et reprenez votre souffle.",
      .effects = DENSE_VEGETATION_HOLD_1 },
    { .label = "Surmonter",
      .cost_benefit = "Une carte au hasard est retirée.",
      .result = "Vous vous frayez un chemin dans la jungle. Une carte au hasard de votre deck est retirée.",
      .effects = DENSE_VEGETATION_OVERCOME },
};

inline constexpr EventOption DENSE_VEGETATION_HOLD_A[] = {
    { .label = "S'accrocher",
      .cost_benefit = "Perdez 4 PV, relancez la carte.",
      .result = "Vous vous accrochez encore, à bout de forces.",
      .effects = DENSE_VEGETATION_HOLD_2 },
    { .label = "Surmonter",
      .cost_benefit = "Une carte au hasard est retirée.",
      .result = "Vous vous frayez un chemin dans la jungle. Une carte au hasard de votre deck est retirée.",
      .effects = DENSE_VEGETATION_OVERCOME },
};

inline constexpr EventOption DENSE_VEGETATION_HOLD_B[] = {
    { .label = "S'accrocher",
      .cost_benefit = "Perdez 5 PV, relancez la carte.",
      .result = "Vous vous accrochez encore, à bout de forces.",
      .effects = DENSE_VEGETATION_HOLD_3 },
    { .label = "Surmonter",
      .cost_benefit = "Une carte au hasard est retirée.",
      .result = "Vous vous frayez un chemin dans la jungle. Une carte au hasard de votre deck est retirée.",
      .effects = DENSE_VEGETATION_OVERCOME },
};

inline constexpr EventPage DENSE_VEGETATION_PAGES[] = {
    { .text = "Après un long détour par le mauvais chemin, vous voilà dans une jungle de fougères, de buissons et de "
              "lianes. Surtout de lianes. Épuisé, vous pensez : « Tu es perdu, et la mort approche. » Que faites-vous ?",
      .options = DENSE_VEGETATION_START },
    { .text = "Les lianes vous enserrent toujours. L'épuisement vous gagne. Que faites-vous ?",
      .options = DENSE_VEGETATION_HOLD_A },
    { .text = "Les lianes vous enserrent toujours. Vous tenez à peine debout. Que faites-vous ?",
      .options = DENSE_VEGETATION_HOLD_B },
};

inline constexpr EventDef DENSE_VEGETATION = {
    .id = "DENSE_VEGETATION",
    .title = "Jungle dense",
    .acts = EVENT_ACT_1A,
    .pages = DENSE_VEGETATION_PAGES,
};

} // namespace engine::events
