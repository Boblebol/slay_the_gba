#pragma once
// Bains abyssaux (ABYSSAL_BATHS), Act 1 (B). Choices: bible/data/event_choices/a.json.
// Immerse: +2 Max HP for 3 HP; Abstain: +10 HP (the crystalline salts). Linger repeats the bath on a page of its
// own, with 1 more HP lost each time. The reference does not say where the baths end (the page only notes that
// lingering too long kills): guessed, there are 6 baths at most (3 to 8 HP), and an option the HP cannot pay is
// greyed out, so that the baths never kill.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect ABYSSAL_BATHS_BATH_1[] = {
    { .type = EventEffectType::GAIN_MAX_HP, .amount = 2 },
    { .type = EventEffectType::LOSE_HP, .amount = 3 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 1 },
};

inline constexpr EventEffect ABYSSAL_BATHS_BATH_2[] = {
    { .type = EventEffectType::GAIN_MAX_HP, .amount = 2 },
    { .type = EventEffectType::LOSE_HP, .amount = 4 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 2 },
};

inline constexpr EventEffect ABYSSAL_BATHS_BATH_3[] = {
    { .type = EventEffectType::GAIN_MAX_HP, .amount = 2 },
    { .type = EventEffectType::LOSE_HP, .amount = 5 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 3 },
};

inline constexpr EventEffect ABYSSAL_BATHS_BATH_4[] = {
    { .type = EventEffectType::GAIN_MAX_HP, .amount = 2 },
    { .type = EventEffectType::LOSE_HP, .amount = 6 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 4 },
};

inline constexpr EventEffect ABYSSAL_BATHS_BATH_5[] = {
    { .type = EventEffectType::GAIN_MAX_HP, .amount = 2 },
    { .type = EventEffectType::LOSE_HP, .amount = 7 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 5 },
};

inline constexpr EventEffect ABYSSAL_BATHS_BATH_6[] = {
    { .type = EventEffectType::GAIN_MAX_HP, .amount = 2 },
    { .type = EventEffectType::LOSE_HP, .amount = 8 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 6 },
};

inline constexpr EventEffect ABYSSAL_BATHS_SALTS[] = {
    { .type = EventEffectType::HEAL, .amount = 10 },
};

inline constexpr EventOption ABYSSAL_BATHS_START[] = {
    { .label = "S'immerger",
      .cost_benefit = "+2 PV Max, perdez 3 PV.",
      .result = "Vous vous glissez dans le bassin. Le liquide vous brûle, mais vous vous sentez grandir (+2 PV Max, -3 PV).",
      .effects = ABYSSAL_BATHS_BATH_1,
      .condition = { .check = EventCheck::MIN_HP, .amount = 3 } },
    { .label = "S'abstenir",
      .cost_benefit = "Récupérez 10 PV.",
      .result = "Vous ramassez les sels cristallins qui bordent le bassin. Ils vous soignent (+10 PV).",
      .effects = ABYSSAL_BATHS_SALTS },
};

inline constexpr EventOption ABYSSAL_BATHS_PAGE_1[] = {
    { .label = "S'attarder",
      .cost_benefit = "+2 PV Max, perdez 4 PV.",
      .result = "Vous vous attardez. La brûlure s'aggrave, mais vous gagnez en robustesse (+2 PV Max, -4 PV).",
      .effects = ABYSSAL_BATHS_BATH_2,
      .condition = { .check = EventCheck::MIN_HP, .amount = 4 } },
    { .label = "Sortir du bain",
      .cost_benefit = "Quittez les bassins.",
      .result = "Vous sortez du bain, ruisselant et transformé." },
};

inline constexpr EventOption ABYSSAL_BATHS_PAGE_2[] = {
    { .label = "S'attarder",
      .cost_benefit = "+2 PV Max, perdez 5 PV.",
      .result = "Vous restez. La douleur est vive, votre corps s'endurcit encore (+2 PV Max, -5 PV).",
      .effects = ABYSSAL_BATHS_BATH_3,
      .condition = { .check = EventCheck::MIN_HP, .amount = 5 } },
    { .label = "Sortir du bain",
      .cost_benefit = "Quittez les bassins.",
      .result = "Vous sortez du bain, ruisselant et transformé." },
};

inline constexpr EventOption ABYSSAL_BATHS_PAGE_3[] = {
    { .label = "S'attarder",
      .cost_benefit = "+2 PV Max, perdez 6 PV.",
      .result = "Vous tenez bon. L'eau vous mord, vous gagnez encore en vigueur (+2 PV Max, -6 PV).",
      .effects = ABYSSAL_BATHS_BATH_4,
      .condition = { .check = EventCheck::MIN_HP, .amount = 6 } },
    { .label = "Sortir du bain",
      .cost_benefit = "Quittez les bassins.",
      .result = "Vous sortez du bain, ruisselant et transformé." },
};

inline constexpr EventOption ABYSSAL_BATHS_PAGE_4[] = {
    { .label = "S'attarder",
      .cost_benefit = "+2 PV Max, perdez 7 PV.",
      .result = "Vous retenez un cri. Le bain vous façonne à ses dépens (+2 PV Max, -7 PV).",
      .effects = ABYSSAL_BATHS_BATH_5,
      .condition = { .check = EventCheck::MIN_HP, .amount = 7 } },
    { .label = "Sortir du bain",
      .cost_benefit = "Quittez les bassins.",
      .result = "Vous sortez du bain, ruisselant et transformé." },
};

inline constexpr EventOption ABYSSAL_BATHS_PAGE_5[] = {
    { .label = "S'attarder",
      .cost_benefit = "+2 PV Max, perdez 8 PV.",
      .result = "Un dernier bain. Le liquide vous dévore et vous façonne à la fois (+2 PV Max, -8 PV).",
      .effects = ABYSSAL_BATHS_BATH_6,
      .condition = { .check = EventCheck::MIN_HP, .amount = 8 } },
    { .label = "Sortir du bain",
      .cost_benefit = "Quittez les bassins.",
      .result = "Vous sortez du bain, ruisselant et transformé." },
};

inline constexpr EventOption ABYSSAL_BATHS_PAGE_6[] = {
    { .label = "Sortir du bain",
      .cost_benefit = "Quittez les bassins.",
      .result = "Vous sortez du bain, ruisselant et transformé." },
};

inline constexpr EventPage ABYSSAL_BATHS_PAGES[] = {
    { .text = "Dans une salle solitaire, des bassins fumants aux couleurs hypnotiques bouillonnent sous des excroissances "
              "coquillées. L'air est lourd de sel. Le plus grand bassin ondule, comme s'il pressentait votre venue.",
      .options = ABYSSAL_BATHS_START },
    { .text = "Le liquide brûlant vous enveloppe. Votre corps picote, votre esprit s'éclaircit. Le bassin vous invite à rester.",
      .options = ABYSSAL_BATHS_PAGE_1 },
    { .text = "La chaleur devient mordante. Le bassin pulse autour de vous comme un cœur. Quelque chose en vous grandit, et cela fait mal.",
      .options = ABYSSAL_BATHS_PAGE_2 },
    { .text = "Vos oreilles bourdonnent. La substance visqueuse s'infiltre sous votre peau. Partir semble de plus en plus difficile.",
      .options = ABYSSAL_BATHS_PAGE_3 },
    { .text = "Le bain bouillonne avec rage. Chaque seconde vous brûle, chaque seconde vous renforce. Où est la limite ?",
      .options = ABYSSAL_BATHS_PAGE_4 },
    { .text = "Vous ne distinguez plus la douleur de la chaleur. Le bassin chante. Un dernier bain, peut-être ?",
      .options = ABYSSAL_BATHS_PAGE_5 },
    { .text = "Vos forces vous quittent : le bassin vous recrache, à bout de souffle et plus robuste. Il est temps de sortir.",
      .options = ABYSSAL_BATHS_PAGE_6 },
};

inline constexpr EventDef ABYSSAL_BATHS = {
    .id = "ABYSSAL_BATHS",
    .title = "Bains Abyssaux",
    .acts = EVENT_ACT_1B,
    .pages = ABYSSAL_BATHS_PAGES,
};

} // namespace engine::events
