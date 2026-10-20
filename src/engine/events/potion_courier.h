#pragma once
// Coursier de potions (POTION_COURIER), all acts. Choices: bible/data/event_choices/a.json.
// The reference gives "1 random Uncommon potion" for the second option; the potion draw has no rarity filter, so any
// random potion is given (flagged in the PR). Potions beyond the belt size are lost, as for any potion.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect POTION_COURIER_GRAB[] = {
    { .type = EventEffectType::GAIN_POTION, .ref = "FOUL_POTION" },
    { .type = EventEffectType::GAIN_POTION, .ref = "FOUL_POTION" },
    { .type = EventEffectType::GAIN_POTION, .ref = "FOUL_POTION" },
};

inline constexpr EventEffect POTION_COURIER_RANSACK[] = {
    { .type = EventEffectType::GAIN_POTION, .amount = 1 },
};

inline constexpr EventOption POTION_COURIER_START[] = {
    { .label = "Prendre les potions",
      .cost_benefit = "Gagnez 3 Potions infectes.",
      .result = "Vous emportez le lot, le nez pincé. Vous obtenez 3 Potions infectes !",
      .effects = POTION_COURIER_GRAB },
    { .label = "Fouiller",
      .cost_benefit = "Gagnez 1 potion au hasard.",
      .result = "Vous fouillez ses affaires et dénichez une potion intacte.",
      .effects = POTION_COURIER_RANSACK },
};

inline constexpr EventPage POTION_COURIER_PAGES[] = {
    { .text = "Une odeur âcre flotte dans l'air. Un Coursier de potions gît à terre, sans un souffle. Ses affaires "
              "ont été fouillées, mais un lot de potions nauséabondes reste, marqué : « Destinataire : Le Marchand ».",
      .options = POTION_COURIER_START },
};

inline constexpr EventDef POTION_COURIER = {
    .id = "POTION_COURIER",
    .title = "Coursier de potions",
    .acts = EVENT_ALL_ACTS,
    .pages = POTION_COURIER_PAGES,
};

} // namespace engine::events
