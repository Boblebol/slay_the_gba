#pragma once
// Bosquet des Morphiques (MORPHIC_GROVE), Act 1 (A). Choices: bible/data/event_choices/c.json.
// The group takes all the gold (the bible keeps "all the gold" over the 100 of one source: LOSE_GOLD never goes
// below 0, so MORPHIC_GROVE_ALL_GOLD is simply larger than any purse) and transforms 2 chosen cards; the loner
// gives 5 Max HP.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t MORPHIC_GROVE_ALL_GOLD = 32767;
inline constexpr int16_t MORPHIC_GROVE_TRANSFORMS = 2;
inline constexpr int16_t MORPHIC_GROVE_MAX_HP = 5;

inline constexpr EventEffect MORPHIC_GROVE_GROUP[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = MORPHIC_GROVE_ALL_GOLD },
    { .type = EventEffectType::TRANSFORM_CARD, .amount = MORPHIC_GROVE_TRANSFORMS },
};

inline constexpr EventEffect MORPHIC_GROVE_LONER[] = {
    { .type = EventEffectType::GAIN_MAX_HP, .amount = MORPHIC_GROVE_MAX_HP },
};

inline constexpr EventOption MORPHIC_GROVE_START[] = {
    { .label = "Le groupe",
      .cost_benefit = "Perdez tout l'Or, transformez 2 cartes.",
      .result = "Les Morphiques réclament tout votre Or et remodèlent votre deck. Choisissez 2 cartes à transformer.",
      .effects = MORPHIC_GROVE_GROUP },
    { .label = "Le solitaire",
      .cost_benefit = "Gagnez 5 PV Max.",
      .result = "Le Morphique solitaire vous gratifie d'un regard timide. Vous gagnez 5 PV Max.",
      .effects = MORPHIC_GROVE_LONER },
};

inline constexpr EventPage MORPHIC_GROVE_PAGES[] = {
    { .text = "Un bosquet d'arbres cristallins vacille avec entrain. Des Morphiques gazouillants vous saluent en "
              "choeur, sauf un, seul dans son coin. Vous approchez-vous du groupe ou du solitaire ?",
      .options = MORPHIC_GROVE_START },
};

inline constexpr EventDef MORPHIC_GROVE = {
    .id = "MORPHIC_GROVE",
    .title = "Bosquet des Morphiques",
    .acts = EVENT_ACT_1A,
    .pages = MORPHIC_GROVE_PAGES,
};

} // namespace engine::events
