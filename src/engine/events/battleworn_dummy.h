#pragma once
// Mannequin d'Entraînement (BATTLEWORN_DUMMY): the event the game had before the data (#253), kept as it was until
// its own issue gives it the choices of the reference.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect BATTLEWORN_DUMMY_FIGHT[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 6 },
    { .type = EventEffectType::GAIN_GOLD, .amount = 60 },
};

inline constexpr EventEffect BATTLEWORN_DUMMY_REPAIR[] = {
    { .type = EventEffectType::UPGRADE_CARD, .amount = 1 },
};

inline constexpr EventOption BATTLEWORN_DUMMY_START[] = {
    { .label = "Attaque acharnée",
      .cost_benefit = "Subissez 6 dégâts, gagnez 60 Or.",
      .result = "Vous frappez sans retenue ! Le mannequin explose en pièces d'or.",
      .effects = BATTLEWORN_DUMMY_FIGHT },
    { .label = "Réparer les rouages",
      .cost_benefit = "Améliorez une carte de votre deck.",
      .result = "Vous étudiez la mécanique interne. Une de vos cartes est améliorée (+) !",
      .effects = BATTLEWORN_DUMMY_REPAIR },
    { .label = "Prendre la fuite",
      .cost_benefit = "Passez votre chemin sans risque.",
      .result = "Vous quittez discrètement la salle." },
};

inline constexpr EventPage BATTLEWORN_DUMMY_PAGES[] = {
    { .text = "« BZZZT ! SÉQUENCE D'ENTRAÎNEMENT INITIÉE ! Réglez vos coups ou subissez une humiliation létale ! »",
      .options = BATTLEWORN_DUMMY_START },
};

inline constexpr EventDef BATTLEWORN_DUMMY = {
    .id = "BATTLEWORN_DUMMY",
    .title = "Mannequin d'Entraînement",
    .acts = EVENT_ALL_ACTS,
    .pages = BATTLEWORN_DUMMY_PAGES,
};

} // namespace engine::events
