#pragma once
// Tombe du guerrier oublié (GRAVE_OF_THE_FORGOTTEN), Act 3: take the Forgotten Soul along (relic), or confront it with
// the truth (Decay curse and an Exhaust card of the deck enchanted with Soul's Power, which removes its Exhaust).
// Choices: bible/data/event_choices/c.json. The third option of the reference is the locked version of the second
// one: here the second option is greyed out while no card of the deck has Exhaust (CAN_SOULS_POWER).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect GRAVE_OF_THE_FORGOTTEN_ACCEPT[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "FORGOTTEN_SOUL" },
};

inline constexpr EventEffect GRAVE_OF_THE_FORGOTTEN_CONFRONT[] = {
    { .type = EventEffectType::ADD_CURSE, .amount = 1, .card = CardId::DECAY },
    { .type = EventEffectType::ENCHANT_CARD, .amount = 1, .enchantment = Enchantment::SOULS_POWER },
};

inline constexpr EventOption GRAVE_OF_THE_FORGOTTEN_START[] = {
    { .label = "Accepter l'âme",
      .cost_benefit = "Gagnez Âme oubliée.",
      .result = "La flamme bleue se glisse en vous. Vous obtenez Âme oubliée !",
      .effects = GRAVE_OF_THE_FORGOTTEN_ACCEPT },
    { .label = "Confronter",
      .cost_benefit = "Gagnez Déclin, enchantez une carte.",
      .result = "L'âme s'éteint, mais sa peine vous marque. Vous obtenez Déclin ! Choisissez une carte qui perdra Épuisement.",
      .effects = GRAVE_OF_THE_FORGOTTEN_CONFRONT,
      .condition = { .check = EventCheck::CAN_SOULS_POWER } },
};

inline constexpr EventPage GRAVE_OF_THE_FORGOTTEN_PAGES[] = {
    { .text = "Une tombe solitaire, surmontée d'une flamme bleue. L'âme d'un guerrier orgueilleux y implore de "
              "voyager à vos côtés. Peut-être vaudrait-il mieux qu'elle affronte la réalité ?",
      .options = GRAVE_OF_THE_FORGOTTEN_START },
};

inline constexpr EventDef GRAVE_OF_THE_FORGOTTEN = {
    .id = "GRAVE_OF_THE_FORGOTTEN",
    .title = "Tombe du guerrier oublié",
    .acts = EVENT_ACT_3,
    .pages = GRAVE_OF_THE_FORGOTTEN_PAGES,
};

} // namespace engine::events
