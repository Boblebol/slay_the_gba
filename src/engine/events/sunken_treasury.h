#pragma once
// Trésor englouti (SUNKEN_TREASURY), Act 1 (B). Choices: bible/data/event_choices/b.json.
// The reference gives a range for each chest (52 to 67 gold, 303 to 363 gold): the effects hold a fixed amount, so
// the low end of each range is used.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t SUNKEN_TREASURY_FIRST_GOLD = 52;   // "52 à 67" in the reference: lowest value
inline constexpr int16_t SUNKEN_TREASURY_SECOND_GOLD = 303; // "303 à 363" in the reference: lowest value

inline constexpr EventEffect SUNKEN_TREASURY_FIRST[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = SUNKEN_TREASURY_FIRST_GOLD },
};

inline constexpr EventEffect SUNKEN_TREASURY_SECOND[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = SUNKEN_TREASURY_SECOND_GOLD },
    { .type = EventEffectType::ADD_CURSE, .amount = 1, .card = CardId::GREED },
};

inline constexpr EventOption SUNKEN_TREASURY_START[] = {
    { .label = "Premier coffre",
      .cost_benefit = "Gagnez 52 Or.",
      .result = "La clé tourne. Quelques pièces d'or tintent au fond du coffre : +52 Or.",
      .effects = SUNKEN_TREASURY_FIRST },
    { .label = "Deuxième coffre",
      .cost_benefit = "Gagnez 303 Or, Cupidité.",
      .result = "Une fortune déborde du coffre maudit : +303 Or. Vous obtenez la malédiction Cupidité.",
      .effects = SUNKEN_TREASURY_SECOND },
};

inline constexpr EventPage SUNKEN_TREASURY_PAGES[] = {
    { .text = "Une chambre forte à moitié engloutie renferme 2 coffres, mais une seule clé fragile. Le premier "
              "cliquette de quelques pièces. Le second, énorme et orné, est sans doute maudit.",
      .options = SUNKEN_TREASURY_START },
};

inline constexpr EventDef SUNKEN_TREASURY = {
    .id = "SUNKEN_TREASURY",
    .title = "Trésor englouti",
    .acts = EVENT_ACT_1B,
    .pages = SUNKEN_TREASURY_PAGES,
};

} // namespace engine::events
