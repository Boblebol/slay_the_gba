#pragma once
// Autel de la Tour (MYSTIC_ALTAR): an event of the game before the data (#253), not in the reference.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect MYSTIC_ALTAR_BLOOD[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 7 },
    { .type = EventEffectType::GAIN_GOLD, .amount = 90 },
};

inline constexpr EventEffect MYSTIC_ALTAR_PRAYER[] = {
    { .type = EventEffectType::UPGRADE_CARD, .amount = 1 },
};

inline constexpr EventOption MYSTIC_ALTAR_START[] = {
    { .label = "Offrande de sang",
      .cost_benefit = "Sacrifiez 7 PV, gagnez 90 Or.",
      .result = "L'autel absorbe votre sang et fait jaillir 90 Pièces d'or !",
      .effects = MYSTIC_ALTAR_BLOOD },
    { .label = "Prière méditative",
      .cost_benefit = "Améliorez une carte de votre deck.",
      .result = "Une bénédiction mystique forge l'une de vos cartes en version (+) !",
      .effects = MYSTIC_ALTAR_PRAYER },
    { .label = "S'incliner et partir",
      .cost_benefit = "Ne rien sacrifier.",
      .result = "Vous saluez l'autel et poursuivez votre chemin." },
};

inline constexpr EventPage MYSTIC_ALTAR_PAGES[] = {
    { .text = "Un monolithe d'obsidienne pulse au rythme d'une énergie ancienne. Une cavité d'offrande attend votre "
              "tribut.",
      .options = MYSTIC_ALTAR_START },
};

inline constexpr EventDef MYSTIC_ALTAR = {
    .id = "MYSTIC_ALTAR",
    .title = "Autel de la Tour",
    .acts = EVENT_ALL_ACTS,
    .pages = MYSTIC_ALTAR_PAGES,
};

} // namespace engine::events
