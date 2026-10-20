#pragma once
// L'avenir des potions ? (THE_FUTURE_OF_POTIONS), all acts. Choices: bible/data/event_choices/c.json.
// The reference lets the player pick the potion to insert; there is no potion choice screen yet, so the first potion
// of the belt is the one traded (TRADE_POTION). The option needs at least one potion.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect THE_FUTURE_OF_POTIONS_TRADE[] = {
    { .type = EventEffectType::TRADE_POTION },
};

inline constexpr EventOption THE_FUTURE_OF_POTIONS_START[] = {
    { .label = "Insérer une potion",
      .cost_benefit = "Potion contre carte améliorée.",
      .result = "La machine avale votre potion et recrache un comprimé : une carte améliorée de même rareté rejoint votre deck.",
      .effects = THE_FUTURE_OF_POTIONS_TRADE,
      .condition = { .check = EventCheck::HAS_POTION } },
    { .label = "Partir",
      .cost_benefit = "Ne rien insérer.",
      .result = "Vous laissez l'appareil à ses expériences et poursuivez votre chemin." },
};

inline constexpr EventPage THE_FUTURE_OF_POTIONS_PAGES[] = {
    { .text = "Un énorme appareil pivote sur son axe en bourdonnant. Ses emplacements semblent transformer les "
              "liquides en petits comprimés faciles à digérer. Et si vous essayiez quelque chose de nouveau ?",
      .options = THE_FUTURE_OF_POTIONS_START },
};

inline constexpr EventDef THE_FUTURE_OF_POTIONS = {
    .id = "THE_FUTURE_OF_POTIONS",
    .title = "L'avenir des potions ?",
    .acts = EVENT_ALL_ACTS,
    .pages = THE_FUTURE_OF_POTIONS_PAGES,
};

} // namespace engine::events
