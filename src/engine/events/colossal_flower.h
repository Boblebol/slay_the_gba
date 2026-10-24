#pragma once
// Fleur colossale (COLOSSAL_FLOWER), Act 2. Choices: bible/data/event_choices/c.json.
// Each time the arm goes deeper the nectar is richer (35, 75, then 135 Gold) and the petals cut more (5, 6, then 7
// HP); at the heart of the flower, the Pollinous Core. Guessed: a deeper reach that the HP cannot pay (it would
// kill) is greyed out, the reference does not say what happens then.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect COLOSSAL_FLOWER_NECTAR_1[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = 35 },
};

inline constexpr EventEffect COLOSSAL_FLOWER_NECTAR_2[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = 75 },
};

inline constexpr EventEffect COLOSSAL_FLOWER_NECTAR_3[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = 135 },
};

inline constexpr EventEffect COLOSSAL_FLOWER_DEEPER_1[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 5 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 1 },
};

inline constexpr EventEffect COLOSSAL_FLOWER_DEEPER_2[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 6 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 2 },
};

inline constexpr EventEffect COLOSSAL_FLOWER_CENTER[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 7 },
    { .type = EventEffectType::GAIN_RELIC, .ref = "POLLINOUS_CORE" },
};

inline constexpr EventOption COLOSSAL_FLOWER_START[] = {
    { .label = "Extraire le nectar",
      .cost_benefit = "Gagnez 35 Or.",
      .result = "Vous recueillez le nectar doré du bout des pétales, sans vous blesser (+35 Or).",
      .effects = COLOSSAL_FLOWER_NECTAR_1 },
    { .label = "Aller plus profond",
      .cost_benefit = "Perdez 5 PV.",
      .result = "Vous glissez le bras entre les pétales. Ils vous entaillent (-5 PV).",
      .effects = COLOSSAL_FLOWER_DEEPER_1,
      .condition = { .check = EventCheck::MIN_HP, .amount = 5 } },
};

inline constexpr EventOption COLOSSAL_FLOWER_DEEP_1[] = {
    { .label = "Extraire le nectar",
      .cost_benefit = "Gagnez 75 Or.",
      .result = "Le nectar est plus épais ici. Vous en remplissez vos poches (+75 Or).",
      .effects = COLOSSAL_FLOWER_NECTAR_2 },
    { .label = "Aller encore plus profond",
      .cost_benefit = "Perdez 6 PV.",
      .result = "Vous poussez plus loin. Les pétales se referment sur votre bras (-6 PV).",
      .effects = COLOSSAL_FLOWER_DEEPER_2,
      .condition = { .check = EventCheck::MIN_HP, .amount = 6 } },
};

inline constexpr EventOption COLOSSAL_FLOWER_DEEP_2[] = {
    { .label = "Extraire le nectar",
      .cost_benefit = "Gagnez 135 Or.",
      .result = "Un nectar presque solide : une fortune dorée (+135 Or).",
      .effects = COLOSSAL_FLOWER_NECTAR_3 },
    { .label = "Entrer au centre",
      .cost_benefit = "Perdez 7 PV, obtenez le Noyau pollinique.",
      .result = "Vous plongez au cœur de la fleur, lacéré, et en ressortez le Noyau pollinique (-7 PV).",
      .effects = COLOSSAL_FLOWER_CENTER,
      .condition = { .check = EventCheck::MIN_HP, .amount = 7 } },
};

inline constexpr EventPage COLOSSAL_FLOWER_PAGES[] = {
    { .text = "Une fleur colossale pousse au sommet d'un monticule d'ossements. Ses pétales changeants, tranchants comme "
              "des rasoirs, pulsent autour d'un amas de pollen. Un peu de nectar doré suffirait... mais le trésor "
              "du centre vous tente.",
      .options = COLOSSAL_FLOWER_START },
    { .text = "Les pétales ondoient, imprévisibles. Plus profond, le nectar est plus riche, mais le centre reste hors "
              "d'atteinte. Jusqu'où irez-vous ?",
      .options = COLOSSAL_FLOWER_DEEP_1 },
    { .text = "Le pollen luit tout près, entre des pétales qui se resserrent. Encore un effort, ou prenez ce nectar "
              "épais et partez.",
      .options = COLOSSAL_FLOWER_DEEP_2 },
};

inline constexpr EventDef COLOSSAL_FLOWER = {
    .id = "COLOSSAL_FLOWER",
    .title = "Fleur Colossale",
    .acts = EVENT_ACT_2,
    .pages = COLOSSAL_FLOWER_PAGES,
};

} // namespace engine::events
