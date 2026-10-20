#pragma once
// Tisseur zen (ZEN_WEAVER), Act 2. Choices: bible/data/event_choices/c.json.
// The reference lists a "Verrouillé" (locked) option for when the gold is short: here each paid option is greyed out
// instead (MIN_GOLD), and the fourth option leaves.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t ZEN_WEAVER_BREATHING_COST = 50;
inline constexpr int16_t ZEN_WEAVER_EMOTIONAL_COST = 125;
inline constexpr int16_t ZEN_WEAVER_ACUPUNCTURE_COST = 250;

inline constexpr EventEffect ZEN_WEAVER_BREATHING[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = ZEN_WEAVER_BREATHING_COST },
    { .type = EventEffectType::ADD_CARD, .amount = 2, .card = CardId::ENLIGHTENMENT },
};

inline constexpr EventEffect ZEN_WEAVER_EMOTIONAL[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = ZEN_WEAVER_EMOTIONAL_COST },
    { .type = EventEffectType::REMOVE_CARD, .amount = 1 },
};

inline constexpr EventEffect ZEN_WEAVER_ACUPUNCTURE[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = ZEN_WEAVER_ACUPUNCTURE_COST },
    { .type = EventEffectType::REMOVE_CARD, .amount = 2 },
};

inline constexpr EventOption ZEN_WEAVER_START[] = {
    { .label = "Techniques de respiration",
      .cost_benefit = "50 Or : ajoutez 2 Illumination.",
      .result = "Vous respirez lentement, au rythme de l'araignée. Deux Illumination rejoignent votre deck.",
      .effects = ZEN_WEAVER_BREATHING,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = ZEN_WEAVER_BREATHING_COST } },
    { .label = "Conscience émotionnelle",
      .cost_benefit = "Payez 125 Or, retirez une carte.",
      .result = "L'araignée vous aide à lâcher prise. Choisissez une carte à retirer de votre deck.",
      .effects = ZEN_WEAVER_EMOTIONAL,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = ZEN_WEAVER_EMOTIONAL_COST } },
    { .label = "Acupuncture arachnéenne",
      .cost_benefit = "Payez 250 Or, retirez 2 cartes.",
      .result = "Les pattes de l'araignée piquent avec précision. Choisissez 2 cartes à retirer de votre deck.",
      .effects = ZEN_WEAVER_ACUPUNCTURE,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = ZEN_WEAVER_ACUPUNCTURE_COST } },
    { .label = "Partir",
      .cost_benefit = "Ne rien payer.",
      .result = "Vous remerciez la petite araignée et poursuivez votre route." },
};

inline constexpr EventPage ZEN_WEAVER_PAGES[] = {
    { .text = "Vous traversez par inadvertance une toile d'araignée. PANIQUE ! Mais une vague de calme vous gagne. "
              "Une minuscule araignée descend : contre quelques pièces d'or, elle vous apprendra des techniques.",
      .options = ZEN_WEAVER_START },
};

inline constexpr EventDef ZEN_WEAVER = {
    .id = "ZEN_WEAVER",
    .title = "Tisseur zen",
    .acts = EVENT_ACT_2,
    .pages = ZEN_WEAVER_PAGES,
};

} // namespace engine::events
