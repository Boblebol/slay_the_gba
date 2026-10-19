#pragma once
// Salle du Fromage (ROOM_FULL_OF_CHEESE), all acts: gorge on cheese (2 random Common cards of the hero) or search the
// room (14 HP for the Chosen Cheese relic, +1 Max HP at the end of each combat). Choices: bible/data/event_choices/c.json.
// The reference offers "2 out of 8 random Common cards"; here the 2 cards are drawn at random, with no choice screen.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect ROOM_FULL_OF_CHEESE_GORGE[] = {
    { .type = EventEffectType::ADD_CARD, .amount = 2 }, // card NONE: random Common cards of the hero
};

inline constexpr EventEffect ROOM_FULL_OF_CHEESE_SEARCH[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 14 },
    { .type = EventEffectType::GAIN_RELIC, .ref = "CHOSEN_CHEESE" },
};

inline constexpr EventOption ROOM_FULL_OF_CHEESE_START[] = {
    { .label = "Se gaver",
      .cost_benefit = "+2 cartes communes aléatoires.",
      .result = "Vous vous empiffrez jusqu'à l'écoeurement. Deux cartes rejoignent votre deck.",
      .effects = ROOM_FULL_OF_CHEESE_GORGE },
    { .label = "Fouiller",
      .cost_benefit = "Perdez 14 PV, relique Fromage.",
      .result = "Vous fouillez dans le fromage et vous en prenez plein la figure. Vous obtenez Fromage d'élection !",
      .effects = ROOM_FULL_OF_CHEESE_SEARCH,
      .condition = { .check = EventCheck::MIN_HP, .amount = 14 } },
};

inline constexpr EventPage ROOM_FULL_OF_CHEESE_PAGES[] = {
    { .text = "Cette salle est remplie de fromage ! Les bougies, le mobilier, et même la porte qui vient de se "
              "refermer derrière vous sont en fromage.",
      .options = ROOM_FULL_OF_CHEESE_START },
};

inline constexpr EventDef ROOM_FULL_OF_CHEESE = {
    .id = "ROOM_FULL_OF_CHEESE",
    .title = "Salle du Fromage",
    .acts = EVENT_ALL_ACTS,
    .pages = ROOM_FULL_OF_CHEESE_PAGES,
};

} // namespace engine::events
