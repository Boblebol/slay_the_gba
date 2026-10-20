#pragma once
// Phare englouti (DROWNING_BEACON), Act 1 (B). Choices: bible/data/event_choices/b.json.
// Bottle some of the liquid (a Glowwater Potion) or climb to the lens (Fresnel Lens, at the cost of 13 Max HP).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect DROWNING_BEACON_BOTTLE[] = {
    { .type = EventEffectType::GAIN_POTION, .ref = "GLOWWATER_POTION" },
};

inline constexpr EventEffect DROWNING_BEACON_CLIMB[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "FRESNEL_LENS" },
    { .type = EventEffectType::LOSE_MAX_HP, .amount = 13 },
};

inline constexpr EventOption DROWNING_BEACON_START[] = {
    { .label = "Mettre en bouteille",
      .cost_benefit = "Obtenez Potion luminescente.",
      .result = "Vous recueillez le liquide faiblement luminescent dans une fiole. Vous obtenez Potion luminescente !",
      .effects = DROWNING_BEACON_BOTTLE },
    { .label = "Grimper",
      .cost_benefit = "Lentille de Fresnel, -13 PV Max.",
      .result = "L'ascension vous épuise, mais vous arrachez la lentille. Vous obtenez Lentille de Fresnel !",
      .effects = DROWNING_BEACON_CLIMB },
};

inline constexpr EventPage DROWNING_BEACON_PAGES[] = {
    { .text = "Après un paysage onirique d'eaux calmes et de rochers, vous émergez face à un phare qui s'enfonce dans "
              "la boue et émet une lueur d'anti-lumière. Mettre en bouteille le liquide luminescent, ou grimper "
              "récupérer la lentille ?",
      .options = DROWNING_BEACON_START },
};

inline constexpr EventDef DROWNING_BEACON = {
    .id = "DROWNING_BEACON",
    .title = "Phare englouti",
    .acts = EVENT_ACT_1B,
    .pages = DROWNING_BEACON_PAGES,
};

} // namespace engine::events
