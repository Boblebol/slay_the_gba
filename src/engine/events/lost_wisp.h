#pragma once
// Feu follet égaré (LOST_WISP), Act 2. Choices: bible/data/event_choices/a.json.
// Guessed: the search yields 60 Gold (the reference gives 45-75, drawn at random); the third option of the
// reference (locked by a Max HP threshold) is not documented, it is the usual "leave" here.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t LOST_WISP_SEARCH_GOLD = 60;

inline constexpr EventEffect LOST_WISP_CAPTURE[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "LOST_WISP" },
    { .type = EventEffectType::ADD_CURSE, .card = CardId::DECAY },
};

inline constexpr EventEffect LOST_WISP_SEARCH[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = LOST_WISP_SEARCH_GOLD },
};

inline constexpr EventOption LOST_WISP_START[] = {
    { .label = "Capturer le feu follet",
      .cost_benefit = "Obtenez Feu follet égaré. Ajoutez Pourriture.",
      .result = "Vous refermez la main sur le granule brûlant. Il vous suit désormais, mais un mal rampant vous gagne.",
      .effects = LOST_WISP_CAPTURE,
      .condition = { .check = EventCheck::NO_RELIC, .ref = "LOST_WISP" } },
    { .label = "Fouiller les environs",
      .cost_benefit = "Gagnez 60 Or.",
      .result = "Parmi les carcasses d'insectes, vous trouvez de quoi remplir votre bourse : 60 Or.",
      .effects = LOST_WISP_SEARCH },
    { .label = "Partir",
      .cost_benefit = "Ne rien faire.",
      .result = "Les flammes vous inquiètent. Vous rebroussez chemin." },
};

inline constexpr EventPage LOST_WISP_PAGES[] = {
    { .text = "Au loin se dresse une scène étrange. Des montagnes d'insectes morts entourent un petit granule "
              "lumineux. Il attire toutes sortes de bestioles, et des flammes en émanent. Vous vous approchez.",
      .options = LOST_WISP_START },
};

inline constexpr EventDef LOST_WISP = {
    .id = "LOST_WISP",
    .title = "Feu follet égaré",
    .acts = EVENT_ACT_2,
    .pages = LOST_WISP_PAGES,
};

} // namespace engine::events
