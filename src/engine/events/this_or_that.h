#pragma once
// Ceci ou Cela ? (THIS_OR_THAT), every act. Choices: bible/data/event_choices/b.json.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t THIS_OR_THAT_HP_COST = 6;
// Devinée: the reference gives a range of 41 to 68 gold (66 for one source); the event pays its middle, rounded.
inline constexpr int16_t THIS_OR_THAT_GOLD = 55;

inline constexpr EventEffect THIS_OR_THAT_THIS[] = {
    { .type = EventEffectType::LOSE_HP, .amount = THIS_OR_THAT_HP_COST },
    { .type = EventEffectType::GAIN_GOLD, .amount = THIS_OR_THAT_GOLD },
};

inline constexpr EventEffect THIS_OR_THAT_THAT[] = {
    { .type = EventEffectType::ADD_CURSE, .card = CardId::CLUMSY },
    { .type = EventEffectType::GAIN_RELIC },
};

inline constexpr EventOption THIS_OR_THAT_START[] = {
    { .label = "Ceci",
      .cost_benefit = "Perdez 6 PV, gagnez 55 Or.",
      .result = "Le sac vous mord la main, mais il est bien rempli. Vous gagnez 55 Or !",
      .effects = THIS_OR_THAT_THIS,
      .condition = { .check = EventCheck::MIN_HP, .amount = THIS_OR_THAT_HP_COST } },
    { .label = "Cela",
      .cost_benefit = "Ajoutez Maladresse, gagnez une relique.",
      .result = "Vous saisissez la relique maudite. Maladresse rejoint votre deck.",
      .effects = THIS_OR_THAT_THAT },
};

inline constexpr EventPage THIS_OR_THAT_PAGES[] = {
    { .text = "Des bras jaillissent d'un trou dans le sol, tenant un sac suspect rempli de richesses et une relique "
              "clairement maudite. « Ceci... ou Cela ? » chuchote une voix rauque d'en-dessous.",
      .options = THIS_OR_THAT_START },
};

inline constexpr EventDef THIS_OR_THAT = {
    .id = "THIS_OR_THAT",
    .title = "Ceci ou Cela ?",
    .acts = EVENT_ALL_ACTS,
    .pages = THIS_OR_THAT_PAGES,
};

} // namespace engine::events
