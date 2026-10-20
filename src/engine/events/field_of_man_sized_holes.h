#pragma once
// Champ de trous à taille corporelle (FIELD_OF_MAN_SIZED_HOLES), Act 2. Choices: bible/data/event_choices/b.json.
// Guessed: the effect of the Perfect Fit enchantment is not in the reference; here, the card goes on top of the draw
// pile each time the pile is shuffled (see CombatEngine::shuffle_draw_pile).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect FIELD_OF_MAN_SIZED_HOLES_ENTER[] = {
    { .type = EventEffectType::ENCHANT_CARD, .amount = 1, .enchantment = Enchantment::PERFECT_FIT },
};

inline constexpr EventEffect FIELD_OF_MAN_SIZED_HOLES_RESIST[] = {
    { .type = EventEffectType::REMOVE_CARD, .amount = 2 },
    { .type = EventEffectType::ADD_CURSE, .card = CardId::NORMALITY },
};

inline constexpr EventOption FIELD_OF_MAN_SIZED_HOLES_START[] = {
    { .label = "Entrer dans votre trou",
      .cost_benefit = "Enchantez une carte au choix.",
      .result = "Vous vous glissez dans le trou, il vous va comme un gant. Vous allez choisir une carte à enchanter.",
      .effects = FIELD_OF_MAN_SIZED_HOLES_ENTER },
    { .label = "Résister",
      .cost_benefit = "Retirez 2 cartes, ajoutez Normalité.",
      .result = "Vous résistez à l'appel du trou. Vous allez retirer 2 cartes, et la Normalité s'installe.",
      .effects = FIELD_OF_MAN_SIZED_HOLES_RESIST },
};

inline constexpr EventPage FIELD_OF_MAN_SIZED_HOLES_PAGES[] = {
    { .text = "Vous vous arrêtez devant un énorme champ dans lequel sont taillées de larges silhouettes de corps. "
              "L'une de ces silhouettes correspond à la vôtre. La correspondance est parfaite...",
      .options = FIELD_OF_MAN_SIZED_HOLES_START },
};

inline constexpr EventDef FIELD_OF_MAN_SIZED_HOLES = {
    .id = "FIELD_OF_MAN_SIZED_HOLES",
    .title = "Champ de trous à taille corporelle",
    .acts = EVENT_ACT_2,
    .pages = FIELD_OF_MAN_SIZED_HOLES_PAGES,
};

} // namespace engine::events
