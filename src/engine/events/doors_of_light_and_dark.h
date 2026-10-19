#pragma once
// Portes de l'ombre et de la lumière (DOORS_OF_LIGHT_AND_DARK), Act 1B. Choices: bible/data/event_choices/a.json.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect DOORS_OF_LIGHT_AND_DARK_LIGHT[] = {
    { .type = EventEffectType::UPGRADE_CARD, .amount = 2, .pick = EventPick::RANDOM },
};

inline constexpr EventEffect DOORS_OF_LIGHT_AND_DARK_DARK[] = {
    { .type = EventEffectType::REMOVE_CARD, .amount = 1 },
};

inline constexpr EventOption DOORS_OF_LIGHT_AND_DARK_START[] = {
    { .label = "Porte de lumière",
      .cost_benefit = "Améliorez 2 cartes au hasard.",
      .result = "La lumière vous enveloppe. Deux de vos cartes sont améliorées (+) !",
      .effects = DOORS_OF_LIGHT_AND_DARK_LIGHT },
    { .label = "Porte d'ombre",
      .cost_benefit = "Retirez une carte de votre deck.",
      .result = "L'ombre vous avale. Choisissez la carte qu'elle emportera.",
      .effects = DOORS_OF_LIGHT_AND_DARK_DARK },
};

inline constexpr EventPage DOORS_OF_LIGHT_AND_DARK_PAGES[] = {
    { .text = "Une porte existe là où il n'y en avait pas. Dedans, deux portes scintillantes et un portier élégant : "
              "« Un visiteur, enfin ! Choisissez une porte, n'importe laquelle ! »",
      .options = DOORS_OF_LIGHT_AND_DARK_START },
};

inline constexpr EventDef DOORS_OF_LIGHT_AND_DARK = {
    .id = "DOORS_OF_LIGHT_AND_DARK",
    .title = "Portes de l'ombre et de la lumière",
    .acts = EVENT_ACT_1B,
    .pages = DOORS_OF_LIGHT_AND_DARK_PAGES,
};

} // namespace engine::events
