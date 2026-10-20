#pragma once
// Le thé de la table ronde (ROUND_TEA_PARTY), Act 3. Choices: bible/data/event_choices/c.json.
// Guessed (the reference does not document it): the "At least pay the entrance fee" screen that follows the brawl
// is a page with a single "Continuer" option which costs nothing more (the 11 HP and the relic come with the brawl).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t ROUND_TEA_PARTY_BRAWL_HP = 11;
inline constexpr int16_t ROUND_TEA_PARTY_FULL_HEAL = 999; // up to the max HP
inline constexpr int16_t ROUND_TEA_PARTY_FEE_PAGE = 1;

inline constexpr EventEffect ROUND_TEA_PARTY_TEA[] = {
    { .type = EventEffectType::HEAL, .amount = ROUND_TEA_PARTY_FULL_HEAL },
    { .type = EventEffectType::GAIN_RELIC, .ref = "ROYAL_POISON" },
};

inline constexpr EventEffect ROUND_TEA_PARTY_BRAWL[] = {
    { .type = EventEffectType::LOSE_HP, .amount = ROUND_TEA_PARTY_BRAWL_HP },
    { .type = EventEffectType::GAIN_RELIC },
    { .type = EventEffectType::GOTO_PAGE, .amount = ROUND_TEA_PARTY_FEE_PAGE },
};

inline constexpr EventOption ROUND_TEA_PARTY_START[] = {
    { .label = "Savourer le thé",
      .cost_benefit = "Soin complet. Poison royal.",
      .result = "Le thé est délicieux et vous revigore, mais son arrière-goût est étrangement amer. Vous obtenez "
                "Poison royal !",
      .effects = ROUND_TEA_PARTY_TEA },
    { .label = "Chercher la bagarre",
      .cost_benefit = "Perdez 11 PV, gagnez une relique.",
      .result = "Vous renversez la table. Les coups pleuvent, mais vous repartez avec un butin !",
      .effects = ROUND_TEA_PARTY_BRAWL,
      .condition = { .check = EventCheck::MIN_HP, .amount = ROUND_TEA_PARTY_BRAWL_HP } },
};

inline constexpr EventOption ROUND_TEA_PARTY_FEE[] = {
    { .label = "Continuer",
      .cost_benefit = "Rien de plus à payer.",
      .result = "Le chevalier à la couronne vous congédie d'un geste. Au moins, vous avez payé l'entrée." },
};

inline constexpr EventPage ROUND_TEA_PARTY_PAGES[] = {
    { .text = "Vous trouvez une invitation à un goûter destiné à un certain « Monseigneur Galalot » et vous vous y "
              "rendez à sa place. Dans une rotonde, des commandants et des mercenaires prennent le thé. Un géant "
              "couronné d'or s'écrie : « C'est à cette heure que vous vous manifestez ?! »",
      .options = ROUND_TEA_PARTY_START },
    { .text = "Le géant couronné vous toise : « Vous allez au moins payer le droit d'entrée ! »",
      .options = ROUND_TEA_PARTY_FEE },
};

inline constexpr EventDef ROUND_TEA_PARTY = {
    .id = "ROUND_TEA_PARTY",
    .title = "Le thé de la table ronde",
    .acts = EVENT_ACT_3,
    .pages = ROUND_TEA_PARTY_PAGES,
};

} // namespace engine::events
