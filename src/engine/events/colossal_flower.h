#pragma once
// Fleur Colossale (COLOSSAL_FLOWER): the event the game had before the data (#253), kept as it was until its own
// issue gives it the choices of the reference.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect COLOSSAL_FLOWER_DRINK[] = {
    { .type = EventEffectType::HEAL, .amount = 20 },
};

inline constexpr EventEffect COLOSSAL_FLOWER_THORN[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 10 },
    { .type = EventEffectType::GAIN_RELIC, .ref = "VAJRA" },
};

inline constexpr EventOption COLOSSAL_FLOWER_START[] = {
    { .label = "Boire le nectar doré",
      .cost_benefit = "Récupérez 20 PV.",
      .result = "Le nectar restaure vos forces (+20 PV) !",
      .effects = COLOSSAL_FLOWER_DRINK },
    { .label = "Arracher l'Épine Ancestrale",
      .cost_benefit = "Subissez 10 dégâts, obtenez la Relique Vajra (+1 Force).",
      .result = "Vous coupez l'épine au prix d'une vive blessure. Vous obtenez Vajra !",
      .effects = COLOSSAL_FLOWER_THORN },
    { .label = "Contourner la plante",
      .cost_benefit = "Ne prenez aucun risque.",
      .result = "Vous contournez la fleur avec précaution." },
};

inline constexpr EventPage COLOSSAL_FLOWER_PAGES[] = {
    { .text = "Une fleur cyclopéenne exsude un parfum suave. Du nectar doré perle sur ses pétales tranchants comme des "
              "rasoirs.",
      .options = COLOSSAL_FLOWER_START },
};

inline constexpr EventDef COLOSSAL_FLOWER = {
    .id = "COLOSSAL_FLOWER",
    .title = "Fleur Colossale",
    .acts = EVENT_ALL_ACTS,
    .pages = COLOSSAL_FLOWER_PAGES,
};

} // namespace engine::events
