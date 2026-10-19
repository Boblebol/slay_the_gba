#pragma once
// Nid d'oizodon (BYRDONIS_NEST), Act 1A. Choices: bible/data/event_choices/a.json.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect BYRDONIS_NEST_EAT[] = {
    { .type = EventEffectType::GAIN_MAX_HP, .amount = 7 },
};

inline constexpr EventEffect BYRDONIS_NEST_TAKE[] = {
    { .type = EventEffectType::ADD_CARD, .amount = 1, .card = CardId::BYRDONIS_EGG },
};

inline constexpr EventOption BYRDONIS_NEST_START[] = {
    { .label = "Manger l'oeuf",
      .cost_benefit = "Gagnez 7 PV Max.",
      .result = "Vous gobez l'oeuf d'un trait. Vous vous sentez plus robuste !",
      .effects = BYRDONIS_NEST_EAT },
    { .label = "Prendre l'oeuf",
      .cost_benefit = "Ajoute un Oeuf d'oizodon.",
      .result = "Vous glissez l'oeuf dans votre sac. Il éclora peut-être à un feu de camp.",
      .effects = BYRDONIS_NEST_TAKE },
};

inline constexpr EventPage BYRDONIS_NEST_PAGES[] = {
    { .text = "Vous apercevez une Énorme Bête se traînant pour chasser un Oizo Vert blessé. Dans l'alcôve d'où "
              "l'oizo a pris la fuite, un oeuf solitaire gît, abandonné et sans surveillance. Votre estomac "
              "gargouille...",
      .options = BYRDONIS_NEST_START },
};

inline constexpr EventDef BYRDONIS_NEST = {
    .id = "BYRDONIS_NEST",
    .title = "Nid d'oizodon",
    .acts = EVENT_ACT_1A,
    .pages = BYRDONIS_NEST_PAGES,
};

} // namespace engine::events
