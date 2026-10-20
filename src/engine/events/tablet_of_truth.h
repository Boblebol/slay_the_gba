#pragma once
// Stèle de la Vérité (TABLET_OF_TRUTH), Act 1 (A): decipher the tablet page after page (3 Max HP and a random upgrade
// each time, the last page upgrades every card) or smash it for 20 HP. Choices: bible/data/event_choices/b.json.
// The Max HP lost on the 4th and 5th reading is uncertain in the reference ("unknown"): it is guessed equal to the
// first three, TABLET_OF_TRUTH_LATE_COST (devinée).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t TABLET_OF_TRUTH_COST = 3;
inline constexpr int16_t TABLET_OF_TRUTH_LATE_COST = 3; // devinée

inline constexpr EventEffect TABLET_OF_TRUTH_READ_1[] = {
    { .type = EventEffectType::LOSE_MAX_HP, .amount = TABLET_OF_TRUTH_COST },
    { .type = EventEffectType::UPGRADE_CARD, .amount = 1, .pick = EventPick::RANDOM },
    { .type = EventEffectType::GOTO_PAGE, .amount = 1 },
};
inline constexpr EventEffect TABLET_OF_TRUTH_READ_2[] = {
    { .type = EventEffectType::LOSE_MAX_HP, .amount = TABLET_OF_TRUTH_COST },
    { .type = EventEffectType::UPGRADE_CARD, .amount = 1, .pick = EventPick::RANDOM },
    { .type = EventEffectType::GOTO_PAGE, .amount = 2 },
};
inline constexpr EventEffect TABLET_OF_TRUTH_READ_3[] = {
    { .type = EventEffectType::LOSE_MAX_HP, .amount = TABLET_OF_TRUTH_COST },
    { .type = EventEffectType::UPGRADE_CARD, .amount = 1, .pick = EventPick::RANDOM },
    { .type = EventEffectType::GOTO_PAGE, .amount = 3 },
};
inline constexpr EventEffect TABLET_OF_TRUTH_READ_4[] = {
    { .type = EventEffectType::LOSE_MAX_HP, .amount = TABLET_OF_TRUTH_LATE_COST },
    { .type = EventEffectType::UPGRADE_CARD, .amount = 1, .pick = EventPick::RANDOM },
    { .type = EventEffectType::GOTO_PAGE, .amount = 4 },
};
inline constexpr EventEffect TABLET_OF_TRUTH_READ_ALL[] = {
    { .type = EventEffectType::LOSE_MAX_HP, .amount = TABLET_OF_TRUTH_LATE_COST },
    { .type = EventEffectType::UPGRADE_CARD, .amount = 99, .pick = EventPick::RANDOM },
};
inline constexpr EventEffect TABLET_OF_TRUTH_SMASH[] = {
    { .type = EventEffectType::HEAL, .amount = 20 },
};

inline constexpr EventOption TABLET_OF_TRUTH_START[] = {
    { .label = "Déchiffrer",
      .cost_benefit = "-3 PV Max, améliore 1 carte au hasard.",
      .result = "Les symboles s'éclairent peu à peu. Une de vos cartes est améliorée (+) !",
      .effects = TABLET_OF_TRUTH_READ_1 },
    { .label = "Briser",
      .cost_benefit = "Soignez 20 PV.",
      .result = "Vous fracassez la stèle. Son énergie apaisante vous envahit.",
      .effects = TABLET_OF_TRUTH_SMASH },
};
inline constexpr EventOption TABLET_OF_TRUTH_PAGE_2[] = {
    { .label = "Continuer",
      .cost_benefit = "-3 PV Max, améliore 1 carte au hasard.",
      .result = "Le sens vous apparaît, mais votre tête vous fait mal. Une de vos cartes est améliorée (+) !",
      .effects = TABLET_OF_TRUTH_READ_2 },
    { .label = "Abandonner", .cost_benefit = "Quittez la stèle.", .result = "Vous détournez les yeux de la stèle." },
};
inline constexpr EventOption TABLET_OF_TRUTH_PAGE_3[] = {
    { .label = "Continuer encore",
      .cost_benefit = "-3 PV Max, améliore 1 carte au hasard.",
      .result = "Vous poursuivez, le front en sueur. Une de vos cartes est améliorée (+) !",
      .effects = TABLET_OF_TRUTH_READ_3 },
    { .label = "Abandonner", .cost_benefit = "Quittez la stèle.", .result = "Vous détournez les yeux de la stèle." },
};
inline constexpr EventOption TABLET_OF_TRUTH_PAGE_4[] = {
    { .label = "CONTINUER",
      .cost_benefit = "-3 PV Max, améliore 1 carte au hasard.",
      .result = "Les mots résonnent dans votre crâne. Une de vos cartes est améliorée (+) !",
      .effects = TABLET_OF_TRUTH_READ_4 },
    { .label = "Abandonner", .cost_benefit = "Quittez la stèle.", .result = "Vous détournez les yeux de la stèle." },
};
inline constexpr EventOption TABLET_OF_TRUTH_PAGE_5[] = {
    { .label = "Tout perdre",
      .cost_benefit = "-3 PV Max, améliore TOUTES les cartes.",
      .result = "La vérité vous submerge. Toutes vos cartes sont améliorées (+) !",
      .effects = TABLET_OF_TRUTH_READ_ALL },
    { .label = "Abandonner", .cost_benefit = "Quittez la stèle.", .result = "Vous détournez les yeux de la stèle." },
};

inline constexpr EventPage TABLET_OF_TRUTH_PAGES[] = {
    { .text = "Dans un caveau, une stèle couverte d'inscriptions familières. Vous pouvez la déchiffrer, mais cela "
              "exigera toute votre concentration, ou la détruire pour libérer son pouvoir guérisseur.",
      .options = TABLET_OF_TRUTH_START },
    { .text = "Vous continuez la traduction. Votre esprit se fatigue, mais la stèle en révèle davantage.",
      .options = TABLET_OF_TRUTH_PAGE_2 },
    { .text = "La stèle livre un nouveau passage. Votre concentration vacille.", .options = TABLET_OF_TRUTH_PAGE_3 },
    { .text = "Chaque mot pèse davantage. Il reste peu à lire, mais le prix s'alourdit.",
      .options = TABLET_OF_TRUTH_PAGE_4 },
    { .text = "Le dernier passage est là. Le lire vous coûtera cher, mais votre deck n'aura plus de secrets.",
      .options = TABLET_OF_TRUTH_PAGE_5 },
};

inline constexpr EventDef TABLET_OF_TRUTH = {
    .id = "TABLET_OF_TRUTH",
    .title = "Stèle de la Vérité",
    .acts = EVENT_ACT_1A,
    .pages = TABLET_OF_TRUTH_PAGES,
};

} // namespace engine::events
