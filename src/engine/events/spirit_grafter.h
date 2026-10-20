#pragma once
// Esprit greffeur (SPIRIT_GRAFTER), Act 2. Choices: bible/data/event_choices/c.json.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect SPIRIT_GRAFTER_LET_IN[] = {
    { .type = EventEffectType::HEAL, .amount = 25 },
    { .type = EventEffectType::ADD_CARD, .card = CardId::METAMORPHOSIS },
};

inline constexpr EventEffect SPIRIT_GRAFTER_REJECT[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 10 },
    { .type = EventEffectType::UPGRADE_CARD, .amount = 1 },
};

inline constexpr EventOption SPIRIT_GRAFTER_START[] = {
    { .label = "Le laisser entrer",
      .cost_benefit = "Soignez 25 PV, gagnez Métamorphose.",
      .result = "L'esprit fusionne avec vous. Vous vous sentez mieux. Vous obtenez Métamorphose !",
      .effects = SPIRIT_GRAFTER_LET_IN },
    { .label = "Rejet",
      .cost_benefit = "Perdez 10 PV, améliorez une carte.",
      .result = "Vous repoussez l'esprit, non sans mal. Choisissez la carte à améliorer.",
      .effects = SPIRIT_GRAFTER_REJECT },
};

inline constexpr EventPage SPIRIT_GRAFTER_PAGES[] = {
    { .text = "Un cocon pend au-dessus de vous, gigotant. Soudain il prend feu et, dans une déflagration de flammes "
              "et de lumière, un Esprit greffeur vous fonce dessus, menaçant de fusionner avec votre être.",
      .options = SPIRIT_GRAFTER_START },
};

inline constexpr EventDef SPIRIT_GRAFTER = {
    .id = "SPIRIT_GRAFTER",
    .title = "Esprit greffeur",
    .acts = EVENT_ACT_2,
    .pages = SPIRIT_GRAFTER_PAGES,
};

} // namespace engine::events
