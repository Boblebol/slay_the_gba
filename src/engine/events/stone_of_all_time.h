#pragma once
// Pierre de tous les temps (STONE_OF_ALL_TIME), all acts. Choices: bible/data/event_choices/c.json.
// Drink and Lift: lose a random potion, +10 Max HP (needs a potion). Push: lose 6 HP, an Attack of the deck gets
// Vigorous 8 (needs an Attack). The reference lists two more "locked" options (marked unknown, supposed to reach 4
// options): they are replaced by the two options above being greyed out without a potion / an Attack, and by a
// "leave" option, guessed, so that the event can always be left.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect STONE_OF_ALL_TIME_DRINK[] = {
    { .type = EventEffectType::LOSE_POTION },
    { .type = EventEffectType::GAIN_MAX_HP, .amount = 10 },
};

inline constexpr EventEffect STONE_OF_ALL_TIME_PUSH[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 6 },
    { .type = EventEffectType::ENCHANT_CARD, .enchantment = Enchantment::VIGOROUS, .enchant_amount = 8 },
};

inline constexpr EventOption STONE_OF_ALL_TIME_START[] = {
    { .label = "Boire et soulever",
      .cost_benefit = "Perdez 1 potion, +10 PV Max.",
      .result = "Vous videz une potion et soulevez la Pierre à bout de bras. Vous gagnez 10 PV Max.",
      .effects = STONE_OF_ALL_TIME_DRINK,
      .condition = { .check = EventCheck::HAS_POTION } },
    { .label = "Pousser",
      .cost_benefit = "Perdez 6 PV, Attaque Vigoureuse 8.",
      .result = "Vous poussez la Pierre de toutes vos forces. Choisissez une Attaque à envoûter de Vigoureux 8.",
      .effects = STONE_OF_ALL_TIME_PUSH,
      .condition = { .check = EventCheck::HAS_ATTACK } },
    { .label = "Partir",
      .cost_benefit = "Ne touchez pas à la Pierre.",
      .result = "La Pierre est là depuis toujours. Elle y restera." },
};

inline constexpr EventPage STONE_OF_ALL_TIME_PAGES[] = {
    { .text = "Un énorme rocher trône au centre d'une cour abandonnée. Une plaque indique : « Pierre de tous les "
              "temps ». La cour fut construite autour de cette Pierre, qui n'a jamais quitté son emplacement.",
      .options = STONE_OF_ALL_TIME_START },
};

inline constexpr EventDef STONE_OF_ALL_TIME = {
    .id = "STONE_OF_ALL_TIME",
    .title = "Pierre de tous les temps",
    .acts = EVENT_ALL_ACTS,
    .pages = STONE_OF_ALL_TIME_PAGES,
};

} // namespace engine::events
