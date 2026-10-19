#pragma once
// Ranwid l'Aîné (RANWID_THE_ELDER), tous les actes. Choices: bible/data/event_choices/b.json.
// The reference lists 5 options, the last two being the locked versions (no potion, no relic) of the 2nd and 3rd:
// here those are the same options greyed out by their condition. The potion and the relic given are not chosen
// (no screen for it): they are the last potion of the belt and the last relic obtained (never the starter relic,
// so a relic must be owned besides it). The new relic(s) are drawn like a chest's, after the loss.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect RANWID_THE_ELDER_GOLD[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = 100 },
    { .type = EventEffectType::GAIN_RELIC },
};

inline constexpr EventEffect RANWID_THE_ELDER_POTION[] = {
    { .type = EventEffectType::LOSE_POTION },
    { .type = EventEffectType::GAIN_RELIC },
};

inline constexpr EventEffect RANWID_THE_ELDER_RELIC[] = {
    { .type = EventEffectType::LOSE_RELIC },
    { .type = EventEffectType::GAIN_RELIC },
    { .type = EventEffectType::GAIN_RELIC },
};

inline constexpr EventOption RANWID_THE_ELDER_START[] = {
    { .label = "Donner 100 Or",
      .cost_benefit = "-100 Or, gagnez une relique.",
      .result = "Ranwid empoche vos pièces avec un sourire édenté et vous tend une relique.",
      .effects = RANWID_THE_ELDER_GOLD,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = 100 } },
    { .label = "Donner une potion",
      .cost_benefit = "-1 potion, gagnez une relique.",
      .result = "Ranwid boit la potion d'un trait, puis vous tend une relique.",
      .effects = RANWID_THE_ELDER_POTION,
      .condition = { .check = EventCheck::HAS_POTION } },
    { .label = "Donner une relique",
      .cost_benefit = "-1 relique, gagnez-en 2.",
      .result = "Ranwid examine longuement votre relique, puis vous en rend deux autres.",
      .effects = RANWID_THE_ELDER_RELIC,
      .condition = { .check = EventCheck::MIN_RELICS, .amount = 2 } },
};

inline constexpr EventPage RANWID_THE_ELDER_PAGES[] = {
    { .text = "Vous êtes approché par la personne la plus âgée que vous ayez jamais vue. « Nous nous rencontrons à "
              "nouveau... c'est moi, Ranwid ! » Vous ne connaissez pas cet homme.",
      .options = RANWID_THE_ELDER_START },
};

inline constexpr EventDef RANWID_THE_ELDER = {
    .id = "RANWID_THE_ELDER",
    .title = "Ranwid l'Aîné",
    .acts = EVENT_ALL_ACTS,
    .pages = RANWID_THE_ELDER_PAGES,
};

} // namespace engine::events
