#pragma once
// La statue engloutie (SUNKEN_STATUE), Acte 1 (A et B). Choices: bible/data/event_choices/a.json.
// The Sword of Stone does what the reference lists for it as a relic, so the option only gives it.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect SUNKEN_STATUE_DIVE[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = 111 },
    { .type = EventEffectType::LOSE_HP, .amount = 7 },
};

inline constexpr EventEffect SUNKEN_STATUE_SWORD[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "SWORD_OF_STONE" },
};

inline constexpr EventOption SUNKEN_STATUE_START[] = {
    { .label = "Plonger dans l'eau",
      .cost_benefit = "Gagnez 111 Or, perdez 7 PV.",
      .result = "Vous plongez et remontez des poignées de pièces : +111 Or, -7 PV.",
      .effects = SUNKEN_STATUE_DIVE,
      .condition = { .check = EventCheck::MIN_HP, .amount = 7 } },
    { .label = "Prendre l'épée",
      .cost_benefit = "Obtenez l'Épée de pierre.",
      .result = "Vous arrachez l'épée aux mains de la statue. Vous obtenez l'Épée de pierre !",
      .effects = SUNKEN_STATUE_SWORD },
};

inline constexpr EventPage SUNKEN_STATUE_PAGES[] = {
    { .text = "Une statue érodée, à moitié immergée dans un bassin, agrippe une épée de pierre. Quelque chose "
              "scintille au fond du lac : des offrandes votives ? Un peu d'argent ne serait pas de refus...",
      .options = SUNKEN_STATUE_START },
};

inline constexpr EventDef SUNKEN_STATUE = {
    .id = "SUNKEN_STATUE",
    .title = "La statue engloutie",
    .acts = EVENT_ACT_1A | EVENT_ACT_1B,
    .pages = SUNKEN_STATUE_PAGES,
};

} // namespace engine::events
