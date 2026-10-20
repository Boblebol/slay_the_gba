#pragma once
// Maître de thé (TEA_MASTER), all acts. Choices: bible/data/event_choices/c.json.
// The bible lists two more "locked" options: they are the two paid teas shown greyed out while the gold is short,
// which the condition of each option does. The relics do their own work (Bone Tea: upgraded starting hand of the
// next fight; Ember Tea: +2 Strength for 5 fights; Tea of Discourtesy: 2 Dazed shuffled in at the next fight).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t TEA_MASTER_BONE_PRICE = 50;
inline constexpr int16_t TEA_MASTER_EMBER_PRICE = 150;

inline constexpr EventEffect TEA_MASTER_BONE[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = TEA_MASTER_BONE_PRICE },
    { .type = EventEffectType::GAIN_RELIC, .ref = "BONE_TEA" },
};

inline constexpr EventEffect TEA_MASTER_EMBER[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = TEA_MASTER_EMBER_PRICE },
    { .type = EventEffectType::GAIN_RELIC, .ref = "EMBER_TEA" },
};

inline constexpr EventEffect TEA_MASTER_DISCOURTESY[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "TEA_OF_DISCOURTESY" },
};

inline constexpr EventOption TEA_MASTER_START[] = {
    { .label = "Thé d'os",
      .cost_benefit = "50 Or. Main de départ améliorée.",
      .result = "Le thé d'os vous réchauffe les os. Vous obtenez Thé d'os !",
      .effects = TEA_MASTER_BONE,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = TEA_MASTER_BONE_PRICE } },
    { .label = "Thé de braise",
      .cost_benefit = "150 Or. +2 Force, 5 combats.",
      .result = "Le thé de braise vous brûle la gorge. Vous obtenez Thé de braise !",
      .effects = TEA_MASTER_EMBER,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = TEA_MASTER_EMBER_PRICE } },
    { .label = "Thé de l'impolitesse",
      .cost_benefit = "Gratuit. +2 Hébété au combat.",
      .result = "Le maître de thé vous toise. Vous obtenez Thé de l'impolitesse !",
      .effects = TEA_MASTER_DISCOURTESY },
};

inline constexpr EventPage TEA_MASTER_PAGES[] = {
    { .text = "Vous arrivez par hasard dans une petite cahute mal éclairée ; l'atmosphère y est calme et "
              "contemplative, et des variétés de thé dont l'exhaustivité frôle l'obsession sont exposées. L'homme "
              "à l'intérieur vous indique une sélection de thés, chacun étiqueté d'un prix élevé.",
      .options = TEA_MASTER_START },
};

inline constexpr EventDef TEA_MASTER = {
    .id = "TEA_MASTER",
    .title = "Maître de thé",
    .acts = EVENT_ALL_ACTS,
    .pages = TEA_MASTER_PAGES,
};

} // namespace engine::events
