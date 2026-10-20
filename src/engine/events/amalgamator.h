#pragma once
// Amalgamateur (AMALGAMATOR), Acte 2. Choices: bible/data/event_choices/b.json.
// Two Defends (or two Strikes) are taken out of the deck, whichever they are, and the Ultimate card comes in. An
// option is greyed out while the deck holds fewer than the 2 cards it needs.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect AMALGAMATOR_DEFENDS[] = {
    { .type = EventEffectType::REMOVE_CARD, .amount = 2, .card = CardId::DEFEND },
    { .type = EventEffectType::ADD_CARD, .card = CardId::ULTIMATE_DEFEND },
};

inline constexpr EventEffect AMALGAMATOR_STRIKES[] = {
    { .type = EventEffectType::REMOVE_CARD, .amount = 2, .card = CardId::STRIKE },
    { .type = EventEffectType::ADD_CARD, .card = CardId::ULTIMATE_STRIKE },
};

inline constexpr EventOption AMALGAMATOR_START[] = {
    { .label = "Fusionner les Défenses",
      .cost_benefit = "-2 Défenses, +Défense ultime.",
      .result = "Les six bras martèlent vos Défenses jusqu'à n'en faire qu'une. Vous obtenez Défense ultime !",
      .effects = AMALGAMATOR_DEFENDS,
      .condition = { .check = EventCheck::HAS_CARD, .amount = 2, .card = CardId::DEFEND } },
    { .label = "Fusionner les Frappes",
      .cost_benefit = "-2 Frappes, +Frappe ultime.",
      .result = "Les six bras forgent vos Frappes en un seul coup. Vous obtenez Frappe ultime !",
      .effects = AMALGAMATOR_STRIKES,
      .condition = { .check = EventCheck::HAS_CARD, .amount = 2, .card = CardId::STRIKE } },
};

inline constexpr EventPage AMALGAMATOR_PAGES[] = {
    { .text = "BANG ! BANG !!! Un énorme être à six bras, au visage de glyphes lumineux, surgit d'un mur et "
              "s'écrie : « Un être doté d'un Esprit Ascendant a trouvé mon atelier ? Réalisons quelque ALLIAGE ! »",
      .options = AMALGAMATOR_START },
};

inline constexpr EventDef AMALGAMATOR = {
    .id = "AMALGAMATOR",
    .title = "Amalgamateur",
    .acts = EVENT_ACT_2,
    .pages = AMALGAMATOR_PAGES,
};

} // namespace engine::events
