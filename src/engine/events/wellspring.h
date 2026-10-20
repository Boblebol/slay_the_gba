#pragma once
// Source mystique (WELLSPRING), Act 1A. Choices: bible/data/event_choices/a.json.
// The Guilty curse is the temporary one of the reference (removed from the deck after 5 combats, see RunManager).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect WELLSPRING_BATHE[] = {
    { .type = EventEffectType::REMOVE_CARD, .amount = 1, .pick = EventPick::CHOOSE },
    { .type = EventEffectType::ADD_CURSE, .amount = 1, .card = CardId::GUILTY },
};

inline constexpr EventEffect WELLSPRING_BOTTLE[] = {
    { .type = EventEffectType::GAIN_POTION, .amount = 1 },
};

inline constexpr EventOption WELLSPRING_START[] = {
    { .label = "Se baigner",
      .cost_benefit = "Retirez une carte, +Culpabilité.",
      .result = "L'eau vous lave de vos fardeaux, mais vous en sortez coupable. Choisissez une carte à retirer.",
      .effects = WELLSPRING_BATHE },
    { .label = "Mettre en bouteille",
      .cost_benefit = "Gagnez une potion aléatoire.",
      .result = "Vous remplissez une fiole d'eau scintillante. Vous obtenez une potion !",
      .effects = WELLSPRING_BOTTLE },
};

inline constexpr EventPage WELLSPRING_PAGES[] = {
    { .text = "Vous suivez un bruit de clapotis jusqu'à sa source et vous trouvez captivé par un bassin paisible. "
              "À la surface de l'eau vert émeraude flottent des granules lumineux. Vous avez envie de vous baigner. "
              "Quel est le pire qui puisse arriver ?",
      .options = WELLSPRING_START },
};

inline constexpr EventDef WELLSPRING = {
    .id = "WELLSPRING",
    .title = "Source mystique",
    .acts = EVENT_ACT_1A,
    .pages = WELLSPRING_PAGES,
};

} // namespace engine::events
