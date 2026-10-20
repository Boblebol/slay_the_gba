#pragma once
// Reflets stelfeR (REFLECTIONS), Act 3. Choices: bible/data/event_choices/c.json.
// Touch a Mirror: 4 random cards upgraded then 2 random upgraded cards downgraded (the order of the reference).
// Shatter: the whole deck is duplicated, and the Bad Luck curse joins it.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect REFLECTIONS_TOUCH[] = {
    { .type = EventEffectType::UPGRADE_CARD, .amount = 4, .pick = EventPick::RANDOM },
    { .type = EventEffectType::DOWNGRADE_CARD, .amount = 2 },
};

inline constexpr EventEffect REFLECTIONS_SHATTER[] = {
    { .type = EventEffectType::DUPLICATE_DECK },
    { .type = EventEffectType::ADD_CURSE, .amount = 1, .card = CardId::BAD_LUCK },
};

inline constexpr EventOption REFLECTIONS_START[] = {
    { .label = "Toucher un miroir",
      .cost_benefit = "Améliore 4 cartes, rétrograde 2.",
      .result = "Le miroir ondule. Quatre cartes brillent plus fort, deux autres se ternissent.",
      .effects = REFLECTIONS_TOUCH },
    { .label = "Briser",
      .cost_benefit = "Dupliquez le deck, Malchance.",
      .result = "Le miroir éclate : chaque carte se retrouve en double. Vous obtenez Malchance.",
      .effects = REFLECTIONS_SHATTER },
};

inline constexpr EventPage REFLECTIONS_PAGES[] = {
    { .text = "Que se passe-t-il ? Quelque chose ici ne tourne pas rond...\n...dnor sap enruot ici esohc euqleuQ",
      .options = REFLECTIONS_START },
};

inline constexpr EventDef REFLECTIONS = {
    .id = "REFLECTIONS",
    .title = "Reflets stelfeR",
    .acts = EVENT_ACT_3,
    .pages = REFLECTIONS_PAGES,
};

} // namespace engine::events
