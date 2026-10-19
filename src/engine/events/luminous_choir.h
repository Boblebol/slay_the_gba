#pragma once
// Chœur lumineux (LUMINOUS_CHOIR), Act 1 (A). Choices: bible/data/event_choices/a.json.
// The reference found only two options out of three: the third is a "Partir" (leave), flagged as a guess there.
// The tribute is 149 gold (the amount the reference requires). The two cards of "Plonger la main" are chosen by the
// player once the page is left, after Spore Mind has joined the deck.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t LUMINOUS_CHOIR_TRIBUTE = 149;

inline constexpr EventEffect LUMINOUS_CHOIR_TRIBUTE_FX[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = LUMINOUS_CHOIR_TRIBUTE },
    { .type = EventEffectType::GAIN_RELIC },
};

inline constexpr EventEffect LUMINOUS_CHOIR_FLESH[] = {
    { .type = EventEffectType::REMOVE_CARD, .amount = 2 },
    { .type = EventEffectType::ADD_CURSE, .card = CardId::SPORE_MIND },
};

inline constexpr EventOption LUMINOUS_CHOIR_START[] = {
    { .label = "Offrir un tribut",
      .cost_benefit = "Payez 149 Or, gagnez une relique.",
      .result = "Le chœur s'apaise et le champignon libère une relique.",
      .effects = LUMINOUS_CHOIR_TRIBUTE_FX,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = LUMINOUS_CHOIR_TRIBUTE } },
    { .label = "Plonger la main",
      .cost_benefit = "Retirez 2 cartes, ajoutez Esprit de spores.",
      .result = "Vous plongez la main dans la chair du champignon. Choisissez 2 cartes à retirer ; Esprit de spores rejoint votre deck.",
      .effects = LUMINOUS_CHOIR_FLESH },
    { .label = "Partir",
      .cost_benefit = "Ne rien faire.",
      .result = "Vous quittez la clairière, la mélodie résonnant encore dans votre poitrine." },
};

inline constexpr EventPage LUMINOUS_CHOIR_PAGES[] = {
    { .text = "Dans une clairière baignée d'une lueur bleue, des champignons géants chantent des mélodies envoûtantes. "
              "Quelque chose de brillant pulse dans la chair du plus grand d'entre eux.",
      .options = LUMINOUS_CHOIR_START },
};

inline constexpr EventDef LUMINOUS_CHOIR = {
    .id = "LUMINOUS_CHOIR",
    .title = "Chœur lumineux",
    .acts = EVENT_ACT_1A,
    .pages = LUMINOUS_CHOIR_PAGES,
};

} // namespace engine::events
