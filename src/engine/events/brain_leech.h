#pragma once
// Sangsue cérébrale (BRAIN_LEECH), Acts 1 to 3. Choices: bible/data/event_choices/b.json.
#include "engine/event_script.h"

namespace engine::events {

// Guessed: the pool of the cards of "Partager la connaissance" is unknown in the reference; here, 5 distinct cards
// of the hero's pool (basic ones excluded).
inline constexpr int16_t BRAIN_LEECH_SHARED_CARDS = 5;
inline constexpr int16_t BRAIN_LEECH_RIP_HP = 5;

inline constexpr EventEffect BRAIN_LEECH_SHARE[] = {
    { .type = EventEffectType::CARD_CHOICE, .amount = BRAIN_LEECH_SHARED_CARDS },
};

inline constexpr EventEffect BRAIN_LEECH_RIP[] = {
    { .type = EventEffectType::LOSE_HP, .amount = BRAIN_LEECH_RIP_HP },
    { .type = EventEffectType::CARD_REWARD, .ref = "COLORLESS" },
};

inline constexpr EventOption BRAIN_LEECH_START[] = {
    { .label = "Partager la connaissance",
      .cost_benefit = "Choisissez 1 carte parmi 5.",
      .result = "La sangsue vous souffle cinq savoirs. Vous allez en choisir un pour votre deck.",
      .effects = BRAIN_LEECH_SHARE },
    { .label = "Arracher la sangsue",
      .cost_benefit = "Perdez 5 PV, carte incolore.",
      .result = "Vous arrachez la sangsue d'un coup sec. Vous allez choisir une carte incolore.",
      .effects = BRAIN_LEECH_RIP },
};

inline constexpr EventPage BRAIN_LEECH_PAGES[] = {
    { .text = "*poum* Vous ressentez une douleur aiguë sur le dessus du crâne tandis qu'une pensée poignarde votre "
              "esprit. « PARTAGER CONNAISSANCE ??? » Vous hésitez sur la marche à suivre...",
      .options = BRAIN_LEECH_START },
};

inline constexpr EventDef BRAIN_LEECH = {
    .id = "BRAIN_LEECH",
    .title = "Sangsue cérébrale",
    .acts = EVENT_ALL_ACTS,
    .pages = BRAIN_LEECH_PAGES,
};

} // namespace engine::events
