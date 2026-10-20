#pragma once
// Symbiote (SYMBIOTE), Acts 1 to 3. Choices: bible/data/event_choices/a.json.
// The reference lists a third, "locked Approach" option whose effects are unknown ("3e option du JSON non documentee"):
// not drawn here, Approach simply finds no Attack to enchant when the deck holds none.
// Guessed: the numbers of the Corrupted enchantment (CORRUPTED_HP_LOSS, +50% damage), see include/engine/card.h.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect SYMBIOTE_APPROACH[] = {
    { .type = EventEffectType::ENCHANT_CARD, .amount = 1, .enchantment = Enchantment::CORRUPTED },
};

inline constexpr EventEffect SYMBIOTE_BURN[] = {
    { .type = EventEffectType::TRANSFORM_CARD, .amount = 1 },
};

inline constexpr EventOption SYMBIOTE_START[] = {
    { .label = "Approcher",
      .cost_benefit = "Corrompez une Attaque au choix.",
      .result = "Le polype vous enveloppe. Vous allez choisir l'Attaque qu'il corrompra.",
      .effects = SYMBIOTE_APPROACH },
    { .label = "Tuer par le feu",
      .cost_benefit = "Transformez une carte au choix.",
      .result = "Les flammes dévorent le polype. Vous allez choisir une carte à transformer.",
      .effects = SYMBIOTE_BURN },
};

inline constexpr EventPage SYMBIOTE_PAGES[] = {
    { .text = "Au cours de votre périple, vous tombez sur un polype sombre et amorphe. Vous avez le pressentiment "
              "qu'il est très ancien, et très maléfique. Approchez...",
      .options = SYMBIOTE_START },
};

inline constexpr EventDef SYMBIOTE = {
    .id = "SYMBIOTE",
    .title = "Symbiote",
    .acts = EVENT_ALL_ACTS,
    .pages = SYMBIOTE_PAGES,
};

} // namespace engine::events
