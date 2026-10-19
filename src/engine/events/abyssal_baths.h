#pragma once
// Bains Abyssaux (ABYSSAL_BATHS): the event the game had before the data (#253), kept until its own issue gives it
// the choices of the reference. The bath now takes the 35 gold it announces, and needs them.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect ABYSSAL_BATHS_BATH[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = 35 },
    { .type = EventEffectType::HEAL, .amount = 25 },
};

inline constexpr EventEffect ABYSSAL_BATHS_DIVE[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 8 },
    { .type = EventEffectType::GAIN_GOLD, .amount = 80 },
};

inline constexpr EventOption ABYSSAL_BATHS_START[] = {
    { .label = "Se prélasser dans le bain",
      .cost_benefit = "Récupérez 25 PV, coûte 35 Or.",
      .result = "Les vapeurs chaudes dissolvent vos courbatures (+25 PV).",
      .effects = ABYSSAL_BATHS_BATH,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = 35 } },
    { .label = "Plonger dans les profondeurs",
      .cost_benefit = "Gagnez 80 Or, subissez 8 dégâts de brûlure.",
      .result = "L'eau bouillante vous brûle mais vous repêchez 80 Pièces d'or !",
      .effects = ABYSSAL_BATHS_DIVE },
    { .label = "Repartir",
      .cost_benefit = "Ignorer les bains.",
      .result = "Vous reprenez votre ascension sans vous attarder." },
};

inline constexpr EventPage ABYSSAL_BATHS_PAGES[] = {
    { .text = "Des bassins fumants émettent des vapeurs minérales revigorantes. Au fond d'une vasque, des pièces d'or "
              "scintillent.",
      .options = ABYSSAL_BATHS_START },
};

inline constexpr EventDef ABYSSAL_BATHS = {
    .id = "ABYSSAL_BATHS",
    .title = "Bains Abyssaux",
    .acts = EVENT_ALL_ACTS,
    .pages = ABYSSAL_BATHS_PAGES,
};

} // namespace engine::events
