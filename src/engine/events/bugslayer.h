#pragma once
// Tueur d'insectes (BUGSLAYER), Act 2: a warrior teaches one of two techniques, each an Event card added to the deck.
// Choices: bible/data/event_choices/c.json.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect BUGSLAYER_EXTERMINATE[] = {
    { .type = EventEffectType::ADD_CARD, .amount = 1, .card = CardId::EXTERMINATE },
};

inline constexpr EventEffect BUGSLAYER_SQUASH[] = {
    { .type = EventEffectType::ADD_CARD, .amount = 1, .card = CardId::SQUASH },
};

inline constexpr EventOption BUGSLAYER_START[] = {
    { .label = "Extermination",
      .cost_benefit = "Ajoutez Extermination au deck.",
      .result = "Le guerrier vous montre comment balayer tout un essaim. Vous obtenez Extermination !",
      .effects = BUGSLAYER_EXTERMINATE },
    { .label = "Aplatissement",
      .cost_benefit = "Ajoutez Aplatissement au deck.",
      .result = "Le guerrier vous apprend à écraser un nuisible d'un seul coup. Vous obtenez Aplatissement !",
      .effects = BUGSLAYER_SQUASH },
};

inline constexpr EventPage BUGSLAYER_PAGES[] = {
    { .text = "Un essaim d'insectes s'éparpille : un puissant guerrier combattait à vos côtés depuis le début ! "
              "« Veux-tu quelques conseils pour exterminer ces nuisibles ? » Vous acceptez d'un hochement de tête.",
      .options = BUGSLAYER_START },
};

inline constexpr EventDef BUGSLAYER = {
    .id = "BUGSLAYER",
    .title = "Tueur d'insectes",
    .acts = EVENT_ACT_2,
    .pages = BUGSLAYER_PAGES,
};

} // namespace engine::events
