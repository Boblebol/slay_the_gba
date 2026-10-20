#pragma once
// Arôme du chaos (AROMA_OF_CHAOS), Act 1 (A). Choices: bible/data/event_choices/a.json.
// Let Go transforms a card the player picks; Maintain Control upgrades one. Both open the deck screen.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect AROMA_OF_CHAOS_LET_GO[] = {
    { .type = EventEffectType::TRANSFORM_CARD, .amount = 1, .pick = EventPick::CHOOSE },
};

inline constexpr EventEffect AROMA_OF_CHAOS_CONTROL[] = {
    { .type = EventEffectType::UPGRADE_CARD, .amount = 1, .pick = EventPick::CHOOSE },
};

inline constexpr EventOption AROMA_OF_CHAOS_START[] = {
    { .label = "Lâcher prise",
      .cost_benefit = "Transformez une carte.",
      .result = "Le chaos vous emporte. Choisissez la carte qui se transformera.",
      .effects = AROMA_OF_CHAOS_LET_GO },
    { .label = "Garder le contrôle",
      .cost_benefit = "Améliorez une carte.",
      .result = "Vous résistez au tourbillon. Choisissez la carte à améliorer.",
      .effects = AROMA_OF_CHAOS_CONTROL },
};

inline constexpr EventPage AROMA_OF_CHAOS_PAGES[] = {
    { .text = "Dans une clairière, un mélange de fleurs, de pourriture et d'autre chose vous submerge. Le monde se "
              "tord autour de vous et vous perdez toute notion de vous-même.",
      .options = AROMA_OF_CHAOS_START },
};

inline constexpr EventDef AROMA_OF_CHAOS = {
    .id = "AROMA_OF_CHAOS",
    .title = "Arôme du chaos",
    .acts = EVENT_ACT_1A,
    .pages = AROMA_OF_CHAOS_PAGES,
};

} // namespace engine::events
