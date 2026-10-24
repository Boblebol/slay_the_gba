#pragma once
// Tourbillon aquatique (SPIRALING_WHIRLPOOL), Act 1 (B). Choices: bible/data/event_choices/b.json.
// Guessed: the healing (the reference has no figure) is 20 HP; the Spiral enchantment does not exist, the card
// gets Adroit (3 Block) instead, on any card of the deck. "Reach In": a random relic and the Writhe curse.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t SPIRALING_WHIRLPOOL_HEAL = 20;

inline constexpr EventEffect SPIRALING_WHIRLPOOL_DRINK[] = {
    { .type = EventEffectType::HEAL, .amount = SPIRALING_WHIRLPOOL_HEAL },
};

inline constexpr EventEffect SPIRALING_WHIRLPOOL_OBSERVE[] = {
    { .type = EventEffectType::ENCHANT_CARD, .amount = 1, .enchantment = Enchantment::ADROIT, .enchant_amount = 3 },
};

inline constexpr EventEffect SPIRALING_WHIRLPOOL_REACH[] = {
    { .type = EventEffectType::GAIN_RELIC },
    { .type = EventEffectType::ADD_CURSE, .card = CardId::WRITHE },
};

inline constexpr EventOption SPIRALING_WHIRLPOOL_START[] = {
    { .label = "Boire",
      .cost_benefit = "Soignez 20 PV.",
      .result = "Vous buvez l'eau tourbillonnante. Elle est étonnamment douce : 20 PV rendus.",
      .effects = SPIRALING_WHIRLPOOL_DRINK },
    { .label = "Observer",
      .cost_benefit = "Enchantez une carte avec Spirale.",
      .result = "Les spirales vous hypnotisent. Choisissez une carte à enchanter.",
      .effects = SPIRALING_WHIRLPOOL_OBSERVE,
      .condition = { .check = EventCheck::CAN_ENCHANT, .amount = -1 } },
    { .label = "Plonger la main",
      .cost_benefit = "Relique aléatoire. Ajoutez Contorsion.",
      .result = "Votre bras s'enfonce dans le vortex et en ressort avec une relique, mais le corps tordu.",
      .effects = SPIRALING_WHIRLPOOL_REACH },
};

inline constexpr EventPage SPIRALING_WHIRLPOOL_PAGES[] = {
    { .text = "Vous tombez sur un énorme tourbillon aquatique. L'eau s'écrase sur les murs, ornés de motifs en "
              "spirale. Il tourne, tourne, tourne... quel spectacle ! Que faites-vous ?",
      .options = SPIRALING_WHIRLPOOL_START },
};

inline constexpr EventDef SPIRALING_WHIRLPOOL = {
    .id = "SPIRALING_WHIRLPOOL",
    .title = "Tourbillon aquatique",
    .acts = EVENT_ACT_1B,
    .pages = SPIRALING_WHIRLPOOL_PAGES,
};

} // namespace engine::events
