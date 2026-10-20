#pragma once
// Bienvenue chez Wongo's (WELCOME_TO_WONGOS), all acts: a merchant sells relics for gold; leaving downgrades a random
// upgraded card. Choices: bible/data/event_choices/b.json.
// - Featured item: the reference draws a Rare relic when the event opens (Frozen Egg in the guide); it is fixed here
//   (WONGOS_FEATURED_RELIC, devinée).
// - Mystery box: the reference delivers the 3 relics after 5 combats; here they are given at once (simplified).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr std::string_view WONGOS_FEATURED_RELIC = "FROZEN_EGG"; // devinée

inline constexpr EventEffect WELCOME_TO_WONGOS_BIN[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = 100 },
    { .type = EventEffectType::GAIN_RELIC, .ref = "COMMON" },
};

inline constexpr EventEffect WELCOME_TO_WONGOS_FEATURED[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = 200 },
    { .type = EventEffectType::GAIN_RELIC, .ref = WONGOS_FEATURED_RELIC },
};

inline constexpr EventEffect WELCOME_TO_WONGOS_BOX[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = 300 },
    { .type = EventEffectType::GAIN_RELIC },
    { .type = EventEffectType::GAIN_RELIC },
    { .type = EventEffectType::GAIN_RELIC },
};

inline constexpr EventEffect WELCOME_TO_WONGOS_LEAVE[] = {
    { .type = EventEffectType::DOWNGRADE_CARD, .amount = 1 },
};

inline constexpr EventOption WELCOME_TO_WONGOS_START[] = {
    { .label = "Bac à soldes",
      .cost_benefit = "Payez 100 Or, 1 relique commune.",
      .result = "Vous fouillez le bac et en sortez une relique. La vendeuse encaisse sans lever la tête.",
      .effects = WELCOME_TO_WONGOS_BIN,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = 100 } },
    { .label = "Article vedette",
      .cost_benefit = "Payez 200 Or, gagnez Oeuf gelé.",
      .result = "« Wongtastique, » soupire la vendeuse. Vous obtenez Oeuf gelé !",
      .effects = WELCOME_TO_WONGOS_FEATURED,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = 200 } },
    { .label = "Boîte mystère",
      .cost_benefit = "Payez 300 Or, 3 reliques aléatoires.",
      .result = "La boîte s'ouvre sur trois reliques. « Merci d'avoir choisi Wongo's. »",
      .effects = WELCOME_TO_WONGOS_BOX,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = 300 } },
    { .label = "Partir",
      .cost_benefit = "Une carte améliorée est rétrogradée.",
      .result = "Vous partez les mains vides. Une de vos cartes perd son amélioration.",
      .effects = WELCOME_TO_WONGOS_LEAVE },
};

inline constexpr EventPage WELCOME_TO_WONGOS_PAGES[] = {
    { .text = "« Bienvenue chez Wongo's. Nous avons tout ce que vous désirez à prix Wongtastique, » annonce le "
              "vendeur le moins enthousiaste que vous ayez jamais rencontré, sans même lever la tête.",
      .options = WELCOME_TO_WONGOS_START },
};

inline constexpr EventDef WELCOME_TO_WONGOS = {
    .id = "WELCOME_TO_WONGOS",
    .title = "Bienvenue chez Wongo's",
    .acts = EVENT_ALL_ACTS,
    .pages = WELCOME_TO_WONGOS_PAGES,
};

} // namespace engine::events
