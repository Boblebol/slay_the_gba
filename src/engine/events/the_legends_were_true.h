#pragma once
// La légende disait vrai (THE_LEGENDS_WERE_TRUE), all acts. Choices: bible/data/event_choices/c.json.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect THE_LEGENDS_WERE_TRUE_NAB[] = {
    { .type = EventEffectType::ADD_CARD, .card = CardId::SPOILS_MAP },
};

inline constexpr EventEffect THE_LEGENDS_WERE_TRUE_EXIT[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 8 },
    { .type = EventEffectType::GAIN_POTION, .amount = 1 },
};

inline constexpr EventOption THE_LEGENDS_WERE_TRUE_START[] = {
    { .label = "Attraper la carte",
      .cost_benefit = "Ajoute Carte au trésor.",
      .result = "Vous vous emparez de la carte. La porte se rouvre : vous voilà avec un plan au trésor !",
      .effects = THE_LEGENDS_WERE_TRUE_NAB },
    { .label = "Chercher une sortie",
      .cost_benefit = "Perdez 8 PV, gagnez 1 potion.",
      .result = "Vous tâtonnez dans le noir, vous cognant partout, et finissez par trouver une potion et la sortie.",
      .effects = THE_LEGENDS_WERE_TRUE_EXIT,
      .condition = { .check = EventCheck::MIN_HP, .amount = 8 } },
};

inline constexpr EventPage THE_LEGENDS_WERE_TRUE_PAGES[] = {
    { .text = "Dans une pièce plongée dans le noir, une lueur éclaire soudain un podium où repose une carte dessinée, "
              "et la porte claque derrière vous. Vous ressentez l'envie pressante de vous en emparer. Tout ceci est "
              "très suspect.",
      .options = THE_LEGENDS_WERE_TRUE_START },
};

inline constexpr EventDef THE_LEGENDS_WERE_TRUE = {
    .id = "THE_LEGENDS_WERE_TRUE",
    .title = "La légende disait vrai",
    .acts = EVENT_ALL_ACTS,
    .pages = THE_LEGENDS_WERE_TRUE_PAGES,
};

} // namespace engine::events
