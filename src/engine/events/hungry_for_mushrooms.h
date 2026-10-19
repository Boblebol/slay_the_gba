#pragma once
// Affamé de champis (HUNGRY_FOR_MUSHROOMS), Act 3: the model of an event file (see include/engine/event_script.h).
// Choices: bible/data/event_choices/a.json. Each relic does what the reference lists for the option when it is
// obtained (Big Mushroom: +20 Max HP; Fragrant Mushroom: -15 HP and 2 random upgrades), so the options only give it.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect HUNGRY_FOR_MUSHROOMS_BIG[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "BIG_MUSHROOM" },
};

inline constexpr EventEffect HUNGRY_FOR_MUSHROOMS_FRAGRANT[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "FRAGRANT_MUSHROOM" },
};

inline constexpr EventOption HUNGRY_FOR_MUSHROOMS_START[] = {
    { .label = "Gros champignon",
      .cost_benefit = "+20 PV Max, 2 cartes de moins au combat",
      .result = "Vous dévorez le gros champignon. Vous vous sentez... plus grand. Vous obtenez Gros champi !",
      .effects = HUNGRY_FOR_MUSHROOMS_BIG },
    { .label = "Champignon parfumé",
      .cost_benefit = "Perdez 15 PV, améliorez 2 cartes",
      .result = "Un parfum enivrant vous monte à la tête. Vous obtenez Champi aromatique !",
      .effects = HUNGRY_FOR_MUSHROOMS_FRAGRANT },
};

inline constexpr EventPage HUNGRY_FOR_MUSHROOMS_PAGES[] = {
    { .text = "Une odeur incroyable vous mène à un campement où cuisent des champignons appétissants. Vous avez si "
              "faim que vous ne remarquez même pas l'aventurier mort qui gît à terre.",
      .options = HUNGRY_FOR_MUSHROOMS_START },
};

inline constexpr EventDef HUNGRY_FOR_MUSHROOMS = {
    .id = "HUNGRY_FOR_MUSHROOMS",
    .title = "Affamé de champis",
    .acts = EVENT_ACT_3,
    .pages = HUNGRY_FOR_MUSHROOMS_PAGES,
};

} // namespace engine::events
