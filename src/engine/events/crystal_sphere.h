#pragma once
// Sphère de cristal (CRYSTAL_SPHERE), all acts. Choices: bible/data/event_choices/a.json.
// The tile mini-game of the divinations (3 or 6 tiles to uncover) is not modelled: each divination is paid out as
// a fixed reward, a card to pick and random potions.
#include "engine/event_script.h"

namespace engine::events {

// Guessed (the reference gives 51 to 99 depending on the source, keengamer: 56): the price of the 3 divinations.
inline constexpr int16_t CRYSTAL_SPHERE_PRICE = 56;

inline constexpr EventEffect CRYSTAL_SPHERE_UNCOVER[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = CRYSTAL_SPHERE_PRICE },
    // Guessed: what 3 divinations give.
    { .type = EventEffectType::GAIN_POTION, .amount = 1 },
    { .type = EventEffectType::CARD_REWARD },
};

inline constexpr EventEffect CRYSTAL_SPHERE_PLAN[] = {
    { .type = EventEffectType::ADD_CURSE, .card = CardId::DEBT },
    // Guessed: what 6 divinations give.
    { .type = EventEffectType::GAIN_POTION, .amount = 2 },
    { .type = EventEffectType::CARD_REWARD },
};

inline constexpr EventOption CRYSTAL_SPHERE_START[] = {
    { .label = "Dévoiler l'avenir",
      .cost_benefit = "Payez 56 Or, 3 divinations.",
      .result = "Vous payez la voyante. La sphère s'illumine et dévoile vos fortunes !",
      .effects = CRYSTAL_SPHERE_UNCOVER,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = CRYSTAL_SPHERE_PRICE } },
    { .label = "Plan de paiement",
      .cost_benefit = "Ajoute Dette, 6 divinations.",
      .result = "Vous signez la décharge. La sphère révèle six fortunes, et une dette s'ajoute à votre deck.",
      .effects = CRYSTAL_SPHERE_PLAN },
};

inline constexpr EventPage CRYSTAL_SPHERE_PAGES[] = {
    { .text = "« J'avais prédit votre venue ! » lance une voix rauque dans la hutte mystique. « Votre destin vous "
              "amène. Voici les prestations de la Sphère de cristal. N'oubliez pas de signer cette décharge. »",
      .options = CRYSTAL_SPHERE_START },
};

inline constexpr EventDef CRYSTAL_SPHERE = {
    .id = "CRYSTAL_SPHERE",
    .title = "Sphère de cristal",
    .acts = EVENT_ALL_ACTS,
    .pages = CRYSTAL_SPHERE_PAGES,
};

} // namespace engine::events
