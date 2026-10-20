#pragma once
// Le Marchand ??? (FAKE_MERCHANT), all acts. Choices: bible/data/event_choices/{a,b,c}.json.
// - "Buy bootleg relics": the reference offers the 10 fake relics at unknown prices, so the event lists them over
//   four pages (page 1 to 4) and each costs the same guessed price.
// - "Throw Foul Potion": needs a Foul Potion on the belt, which is spent, and starts a fight. The reference gives
//   big rewards for it but not their content, and the Fake Merchant monster is not in the encounter tables yet:
//   the fight is a normal one of the act with its usual rewards.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t FAKE_MERCHANT_PRICE = 50; // guessed: the reference says "prix inconnus"

inline constexpr EventCondition FAKE_MERCHANT_AFFORD = { .check = EventCheck::MIN_GOLD, .amount = FAKE_MERCHANT_PRICE };

inline constexpr EventEffect FAKE_MERCHANT_SHOP[] = { { .type = EventEffectType::GOTO_PAGE, .amount = 1 } };
inline constexpr EventEffect FAKE_MERCHANT_NEXT_2[] = { { .type = EventEffectType::GOTO_PAGE, .amount = 2 } };
inline constexpr EventEffect FAKE_MERCHANT_NEXT_3[] = { { .type = EventEffectType::GOTO_PAGE, .amount = 3 } };
inline constexpr EventEffect FAKE_MERCHANT_NEXT_4[] = { { .type = EventEffectType::GOTO_PAGE, .amount = 4 } };

inline constexpr EventEffect FAKE_MERCHANT_THROW[] = {
    { .type = EventEffectType::LOSE_POTION, .ref = "FOUL_POTION" },
    { .type = EventEffectType::FIGHT },
};

#define FAKE_MERCHANT_BUY(NAME, RELIC)                                                                              \
    inline constexpr EventEffect FAKE_MERCHANT_##NAME[] = {                                                         \
        { .type = EventEffectType::LOSE_GOLD, .amount = FAKE_MERCHANT_PRICE },                                      \
        { .type = EventEffectType::GAIN_RELIC, .ref = RELIC },                                                      \
    }
FAKE_MERCHANT_BUY(ANCHOR, "FAKE_ANCHOR");
FAKE_MERCHANT_BUY(BLOOD_VIAL, "FAKE_BLOOD_VIAL");
FAKE_MERCHANT_BUY(HAPPY_FLOWER, "FAKE_HAPPY_FLOWER");
FAKE_MERCHANT_BUY(LEES_WAFFLE, "FAKE_LEES_WAFFLE");
FAKE_MERCHANT_BUY(MANGO, "FAKE_MANGO");
FAKE_MERCHANT_BUY(MERCHANTS_RUG, "FAKE_MERCHANTS_RUG");
FAKE_MERCHANT_BUY(ORICHALCUM, "FAKE_ORICHALCUM");
FAKE_MERCHANT_BUY(SNECKO_EYE, "FAKE_SNECKO_EYE");
FAKE_MERCHANT_BUY(STRIKE_DUMMY, "FAKE_STRIKE_DUMMY");
FAKE_MERCHANT_BUY(VENERABLE_TEA_SET, "FAKE_VENERABLE_TEA_SET");
#undef FAKE_MERCHANT_BUY

inline constexpr EventOption FAKE_MERCHANT_START[] = {
    { .label = "Reliques contrefaites",
      .cost_benefit = "Achetez une relique pour 50 Or.",
      .result = "Le marchand étale sa marchandise, tout sourire.",
      .effects = FAKE_MERCHANT_SHOP },
    { .label = "Lancer une Potion infecte",
      .cost_benefit = "Défiez le marchand au combat.",
      .result = "La fiole éclate sur le marchand. Son sourire disparaît : il se jette sur vous !",
      .effects = FAKE_MERCHANT_THROW,
      .condition = { .check = EventCheck::HAS_POTION, .ref = "FOUL_POTION" } },
    { .label = "Partir",
      .cost_benefit = "Ne rien acheter.",
      .result = "Vous passez votre chemin. Le marchand vous regarde partir sans un mot." },
};

inline constexpr EventOption FAKE_MERCHANT_PAGE_1[] = {
    { .label = "Ancre ???",
      .cost_benefit = "50 Or.",
      .result = "Vous payez 50 Or. Vous obtenez Ancre ???.",
      .effects = FAKE_MERCHANT_ANCHOR,
      .condition = FAKE_MERCHANT_AFFORD },
    { .label = "Fiole de sang ???",
      .cost_benefit = "50 Or.",
      .result = "Vous payez 50 Or. Vous obtenez Fiole de sang ???.",
      .effects = FAKE_MERCHANT_BLOOD_VIAL,
      .condition = FAKE_MERCHANT_AFFORD },
    { .label = "Fleur joyeuse ???",
      .cost_benefit = "50 Or.",
      .result = "Vous payez 50 Or. Vous obtenez Fleur joyeuse ???.",
      .effects = FAKE_MERCHANT_HAPPY_FLOWER,
      .condition = FAKE_MERCHANT_AFFORD },
    { .label = "Voir la suite",
      .cost_benefit = "Autres reliques.",
      .result = "Le marchand sort d'autres reliques de sa besace.",
      .effects = FAKE_MERCHANT_NEXT_2 },
};

inline constexpr EventOption FAKE_MERCHANT_PAGE_2[] = {
    { .label = "Gaufre de Lee ???",
      .cost_benefit = "50 Or.",
      .result = "Vous payez 50 Or. Vous obtenez Gaufre de Lee ???.",
      .effects = FAKE_MERCHANT_LEES_WAFFLE,
      .condition = FAKE_MERCHANT_AFFORD },
    { .label = "Mangue ???",
      .cost_benefit = "50 Or.",
      .result = "Vous payez 50 Or. Vous obtenez Mangue ???.",
      .effects = FAKE_MERCHANT_MANGO,
      .condition = FAKE_MERCHANT_AFFORD },
    { .label = "Tapis du Marchand ???",
      .cost_benefit = "50 Or.",
      .result = "Vous payez 50 Or. Vous obtenez Le tapis du Marchand ???.",
      .effects = FAKE_MERCHANT_MERCHANTS_RUG,
      .condition = FAKE_MERCHANT_AFFORD },
    { .label = "Voir la suite",
      .cost_benefit = "Autres reliques.",
      .result = "Le marchand fouille encore sa besace.",
      .effects = FAKE_MERCHANT_NEXT_3 },
};

inline constexpr EventOption FAKE_MERCHANT_PAGE_3[] = {
    { .label = "Orichalque ???",
      .cost_benefit = "50 Or.",
      .result = "Vous payez 50 Or. Vous obtenez Orichalque ???.",
      .effects = FAKE_MERCHANT_ORICHALCUM,
      .condition = FAKE_MERCHANT_AFFORD },
    { .label = "Œil de geckobra ???",
      .cost_benefit = "50 Or.",
      .result = "Vous payez 50 Or. Vous obtenez Œil de geckobra ???.",
      .effects = FAKE_MERCHANT_SNECKO_EYE,
      .condition = FAKE_MERCHANT_AFFORD },
    { .label = "Mannequin ???",
      .cost_benefit = "50 Or.",
      .result = "Vous payez 50 Or. Vous obtenez Mannequin d'entraînement ???.",
      .effects = FAKE_MERCHANT_STRIKE_DUMMY,
      .condition = FAKE_MERCHANT_AFFORD },
    { .label = "Voir la suite",
      .cost_benefit = "Autres reliques.",
      .result = "Le marchand sort enfin le fond de sa besace.",
      .effects = FAKE_MERCHANT_NEXT_4 },
};

inline constexpr EventOption FAKE_MERCHANT_PAGE_4[] = {
    { .label = "Service à thé ???",
      .cost_benefit = "50 Or.",
      .result = "Vous payez 50 Or. Vous obtenez Service à thé vénérable ???.",
      .effects = FAKE_MERCHANT_VENERABLE_TEA_SET,
      .condition = FAKE_MERCHANT_AFFORD },
    { .label = "Partir",
      .cost_benefit = "Ne rien acheter.",
      .result = "Vous quittez l'étal. Le marchand vous regarde partir sans un mot." },
};

inline constexpr EventPage FAKE_MERCHANT_PAGES[] = {
    { .text = "Un marchand au sourire un peu trop large vous interpelle : « Des reliques rares, rien que pour vous ! » "
              "Quelque chose cloche chez lui.",
      .options = FAKE_MERCHANT_START },
    { .text = "« Ces reliques sont authentiques, je le jure ! » Chacune coûte 50 Or.", .options = FAKE_MERCHANT_PAGE_1 },
    { .text = "« Encore quelques merveilles, ami ! » Chaque relique coûte 50 Or.", .options = FAKE_MERCHANT_PAGE_2 },
    { .text = "« Les dernières, et pas les moins belles ! » Chaque relique coûte 50 Or.", .options = FAKE_MERCHANT_PAGE_3 },
    { .text = "« Le tout dernier, mon préféré ! » Il coûte 50 Or.", .options = FAKE_MERCHANT_PAGE_4 },
};

inline constexpr EventDef FAKE_MERCHANT = {
    .id = "FAKE_MERCHANT",
    .title = "Le Marchand ???",
    .acts = EVENT_ALL_ACTS,
    .pages = FAKE_MERCHANT_PAGES,
};

} // namespace engine::events
