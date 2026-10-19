#pragma once
// Vallon des Murmures (WHISPERING_HOLLOW), Act 1 (A). Choices: bible/data/event_choices/a.json.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect WHISPERING_HOLLOW_EXCHANGE[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = 50 },
    { .type = EventEffectType::GAIN_POTION, .amount = 2 }, // no ref: two random potions
};

inline constexpr EventEffect WHISPERING_HOLLOW_HUG[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 9 },
    { .type = EventEffectType::TRANSFORM_CARD, .amount = 1, .pick = EventPick::CHOOSE },
};

inline constexpr EventOption WHISPERING_HOLLOW_START[] = {
    { .label = "Échanger de l'or",
      .cost_benefit = "Payez 50 Or, gagnez 2 potions.",
      .result = "Vous glissez vos pièces dans la cage de branches. Elle vous rend 2 potions.",
      .effects = WHISPERING_HOLLOW_EXCHANGE,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = 50 } },
    { .label = "Étreindre l'arbre",
      .cost_benefit = "Perdez 9 PV, transformez 1 carte.",
      .result = "Les branches vous serrent à vous briser les côtes. Vous perdez 9 PV et une carte va se transformer.",
      .effects = WHISPERING_HOLLOW_HUG,
      .condition = { .check = EventCheck::MIN_HP, .amount = 9 } },
};

inline constexpr EventPage WHISPERING_HOLLOW_PAGES[] = {
    { .text = "Au milieu d'un vallon d'arbres morts se dresse un arbre blanc comme l'os. Des boules d'argile pendent "
              "de ses branches, courbées comme une cage thoracique. Il murmure : « ...faire échange... »",
      .options = WHISPERING_HOLLOW_START },
};

inline constexpr EventDef WHISPERING_HOLLOW = {
    .id = "WHISPERING_HOLLOW",
    .title = "Vallon des Murmures",
    .acts = EVENT_ACT_1A,
    .pages = WHISPERING_HOLLOW_PAGES,
};

} // namespace engine::events
