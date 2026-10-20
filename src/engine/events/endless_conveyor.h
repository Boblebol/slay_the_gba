#pragma once
// Convoyeur sans fin (ENDLESS_CONVEYOR), Acte 1 (B). Choices: bible/data/event_choices/b.json.
// Page 0 is the only option of the event: observe the chef (a random upgrade). It opens the belt, a hub: each dish
// (40 gold, greyed out below) leads back to the page of the belt it was taken from, until "Partir". The 8 dishes do
// not fit in the 4 options of a page, so the belt is split in 3 pages, linked by "Plats suivants / précédents":
//   belt 1 (pages 1, 4): Caviar, Clam Roll, Fried Eel         belt 2 (pages 2, 5): Golden Fysh, Jelly Liver, Seapunk Salad
//   belt 3 (pages 3, 6): Spicy Snappy, Suspicious Condiment, back, leave
// The Golden Fysh is free and gives 75 gold: taking it moves to the copy of the belt (pages 4, 5, 6) without it, so
// it can only be taken once and the belt is no source of gold. Everything else can be bought again and again.
#include <array>
#include "engine/event_script.h"

namespace engine::events {

constexpr int16_t ENDLESS_CONVEYOR_PRICE = 40;

constexpr std::array<EventEffect, 1> endless_conveyor_go(int16_t page) {
    return { EventEffect{ .type = EventEffectType::GOTO_PAGE, .amount = page } };
}

// A paid dish: the price, its effect, then back to `page`.
constexpr std::array<EventEffect, 3> endless_conveyor_dish(EventEffect effect, int16_t page) {
    return { EventEffect{ .type = EventEffectType::LOSE_GOLD, .amount = ENDLESS_CONVEYOR_PRICE }, effect,
             EventEffect{ .type = EventEffectType::GOTO_PAGE, .amount = page } };
}

constexpr EventEffect ENDLESS_CONVEYOR_CAVIAR_FX = { .type = EventEffectType::GAIN_MAX_HP, .amount = 4 };
constexpr EventEffect ENDLESS_CONVEYOR_CLAM_FX = { .type = EventEffectType::HEAL, .amount = 10 };
constexpr EventEffect ENDLESS_CONVEYOR_EEL_FX = { .type = EventEffectType::ADD_CARD }; // a random Colorless card
constexpr EventEffect ENDLESS_CONVEYOR_JELLY_FX = { .type = EventEffectType::TRANSFORM_CARD };
constexpr EventEffect ENDLESS_CONVEYOR_SALAD_FX = { .type = EventEffectType::ADD_CARD, .card = CardId::FEEDING_FRENZY };
constexpr EventEffect ENDLESS_CONVEYOR_SNAPPY_FX = { .type = EventEffectType::UPGRADE_CARD, .pick = EventPick::RANDOM };
constexpr EventEffect ENDLESS_CONVEYOR_CONDIMENT_FX = { .type = EventEffectType::GAIN_POTION, .amount = 1 };

inline constexpr EventEffect ENDLESS_CONVEYOR_OBSERVE[] = {
    { .type = EventEffectType::UPGRADE_CARD, .pick = EventPick::RANDOM },
    { .type = EventEffectType::GOTO_PAGE, .amount = 1 },
};
inline constexpr EventEffect ENDLESS_CONVEYOR_GOLDEN[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = 75 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 5 },
};

// Belt with the Golden Fysh: dishes lead back to pages 1, 2, 3. Without it: 4, 5, 6.
inline constexpr auto ENDLESS_CONVEYOR_CAVIAR_1 = endless_conveyor_dish(ENDLESS_CONVEYOR_CAVIAR_FX, 1);
inline constexpr auto ENDLESS_CONVEYOR_CLAM_1 = endless_conveyor_dish(ENDLESS_CONVEYOR_CLAM_FX, 1);
inline constexpr auto ENDLESS_CONVEYOR_EEL_1 = endless_conveyor_dish(ENDLESS_CONVEYOR_EEL_FX, 1);
inline constexpr auto ENDLESS_CONVEYOR_JELLY_2 = endless_conveyor_dish(ENDLESS_CONVEYOR_JELLY_FX, 2);
inline constexpr auto ENDLESS_CONVEYOR_SALAD_2 = endless_conveyor_dish(ENDLESS_CONVEYOR_SALAD_FX, 2);
inline constexpr auto ENDLESS_CONVEYOR_SNAPPY_3 = endless_conveyor_dish(ENDLESS_CONVEYOR_SNAPPY_FX, 3);
inline constexpr auto ENDLESS_CONVEYOR_CONDIMENT_3 = endless_conveyor_dish(ENDLESS_CONVEYOR_CONDIMENT_FX, 3);
inline constexpr auto ENDLESS_CONVEYOR_CAVIAR_4 = endless_conveyor_dish(ENDLESS_CONVEYOR_CAVIAR_FX, 4);
inline constexpr auto ENDLESS_CONVEYOR_CLAM_4 = endless_conveyor_dish(ENDLESS_CONVEYOR_CLAM_FX, 4);
inline constexpr auto ENDLESS_CONVEYOR_EEL_4 = endless_conveyor_dish(ENDLESS_CONVEYOR_EEL_FX, 4);
inline constexpr auto ENDLESS_CONVEYOR_JELLY_5 = endless_conveyor_dish(ENDLESS_CONVEYOR_JELLY_FX, 5);
inline constexpr auto ENDLESS_CONVEYOR_SALAD_5 = endless_conveyor_dish(ENDLESS_CONVEYOR_SALAD_FX, 5);
inline constexpr auto ENDLESS_CONVEYOR_SNAPPY_6 = endless_conveyor_dish(ENDLESS_CONVEYOR_SNAPPY_FX, 6);
inline constexpr auto ENDLESS_CONVEYOR_CONDIMENT_6 = endless_conveyor_dish(ENDLESS_CONVEYOR_CONDIMENT_FX, 6);
inline constexpr auto ENDLESS_CONVEYOR_GO_1 = endless_conveyor_go(1);
inline constexpr auto ENDLESS_CONVEYOR_GO_2 = endless_conveyor_go(2);
inline constexpr auto ENDLESS_CONVEYOR_GO_3 = endless_conveyor_go(3);
inline constexpr auto ENDLESS_CONVEYOR_GO_4 = endless_conveyor_go(4);
inline constexpr auto ENDLESS_CONVEYOR_GO_5 = endless_conveyor_go(5);
inline constexpr auto ENDLESS_CONVEYOR_GO_6 = endless_conveyor_go(6);

constexpr EventCondition ENDLESS_CONVEYOR_PAYS = { .check = EventCheck::MIN_GOLD, .amount = ENDLESS_CONVEYOR_PRICE };

constexpr EventOption endless_conveyor_dish_option(std::string_view label, std::string_view cost, std::string_view result,
                                                   std::span<const EventEffect> effects) {
    return { .label = label, .cost_benefit = cost, .result = result, .effects = effects, .condition = ENDLESS_CONVEYOR_PAYS };
}

inline constexpr std::string_view ENDLESS_CONVEYOR_CAVIAR_RESULT = "Des perles salées éclatent sous la dent. Vous gagnez 4 PV Max.";
inline constexpr std::string_view ENDLESS_CONVEYOR_CLAM_RESULT = "Un roulé tiède et réconfortant. Vous récupérez 10 PV.";
inline constexpr std::string_view ENDLESS_CONVEYOR_EEL_RESULT = "Une anguille croustillante, étrangement nourrissante. Une carte incolore rejoint votre deck.";
inline constexpr std::string_view ENDLESS_CONVEYOR_JELLY_RESULT = "Le foie en gelée tremble, puis vous transforme de l'intérieur. Choisissez la carte à transformer.";
inline constexpr std::string_view ENDLESS_CONVEYOR_SALAD_RESULT = "Une salade frénétique ! Repas frénétique rejoint votre deck.";
inline constexpr std::string_view ENDLESS_CONVEYOR_SNAPPY_RESULT = "Le vivaneau est si épicé qu'il vous aiguise l'esprit. Une carte aléatoire sera améliorée.";
inline constexpr std::string_view ENDLESS_CONVEYOR_CONDIMENT_RESULT = "Le condiment sent le soufre, mais le chef hoche la tête. Vous obtenez une potion.";

inline constexpr EventOption ENDLESS_CONVEYOR_START[] = {
    { .label = "Observer le chef",
      .cost_benefit = "Améliore une carte aléatoire.",
      .result = "Le chef ondule au-dessus de ses plats. Vous retenez ses gestes : une carte aléatoire sera améliorée.",
      .effects = ENDLESS_CONVEYOR_OBSERVE },
};

inline constexpr EventOption ENDLESS_CONVEYOR_BELT_1[] = {
    endless_conveyor_dish_option("Caviar", "-40 Or, +4 PV Max.", ENDLESS_CONVEYOR_CAVIAR_RESULT, ENDLESS_CONVEYOR_CAVIAR_1),
    endless_conveyor_dish_option("Roulé de palourde", "-40 Or, soigne 10 PV.", ENDLESS_CONVEYOR_CLAM_RESULT, ENDLESS_CONVEYOR_CLAM_1),
    endless_conveyor_dish_option("Anguille frite", "-40 Or, une carte incolore.", ENDLESS_CONVEYOR_EEL_RESULT, ENDLESS_CONVEYOR_EEL_1),
    { .label = "Plats suivants",
      .cost_benefit = "Le tapis continue d'avancer.",
      .result = "Le tapis roule encore.",
      .effects = ENDLESS_CONVEYOR_GO_2 },
};
inline constexpr EventOption ENDLESS_CONVEYOR_BELT_2[] = {
    { .label = "Poisson d'or",
      .cost_benefit = "Gratuit : gagnez 75 Or.",
      .result = "GAGNANT CHANCEUX ! Le chef vous glisse 75 Pièces d'or avec le poisson.",
      .effects = ENDLESS_CONVEYOR_GOLDEN },
    endless_conveyor_dish_option("Foie en gelée", "-40 Or, transforme une carte.", ENDLESS_CONVEYOR_JELLY_RESULT, ENDLESS_CONVEYOR_JELLY_2),
    endless_conveyor_dish_option("Salade seapunk", "-40 Or, ajoute Repas frénétique.", ENDLESS_CONVEYOR_SALAD_RESULT, ENDLESS_CONVEYOR_SALAD_2),
    { .label = "Plats suivants",
      .cost_benefit = "Le tapis continue d'avancer.",
      .result = "Le tapis roule encore.",
      .effects = ENDLESS_CONVEYOR_GO_3 },
};
inline constexpr EventOption ENDLESS_CONVEYOR_BELT_3[] = {
    endless_conveyor_dish_option("Vivaneau épicé", "-40 Or, améliore une carte.", ENDLESS_CONVEYOR_SNAPPY_RESULT, ENDLESS_CONVEYOR_SNAPPY_3),
    endless_conveyor_dish_option("Condiment suspect", "-40 Or, une potion aléatoire.", ENDLESS_CONVEYOR_CONDIMENT_RESULT, ENDLESS_CONVEYOR_CONDIMENT_3),
    { .label = "Plats précédents",
      .cost_benefit = "Revenir au début du tapis.",
      .result = "Le tapis ramène les premiers plats.",
      .effects = ENDLESS_CONVEYOR_GO_1 },
    { .label = "Partir",
      .cost_benefit = "Quitter la baraque.",
      .result = "Vous quittez la baraque, le ventre plein, le chef agitant six bras d'adieu." },
};

// The same belt once the Golden Fysh is gone.
inline constexpr EventOption ENDLESS_CONVEYOR_BELT_1B[] = {
    endless_conveyor_dish_option("Caviar", "-40 Or, +4 PV Max.", ENDLESS_CONVEYOR_CAVIAR_RESULT, ENDLESS_CONVEYOR_CAVIAR_4),
    endless_conveyor_dish_option("Roulé de palourde", "-40 Or, soigne 10 PV.", ENDLESS_CONVEYOR_CLAM_RESULT, ENDLESS_CONVEYOR_CLAM_4),
    endless_conveyor_dish_option("Anguille frite", "-40 Or, une carte incolore.", ENDLESS_CONVEYOR_EEL_RESULT, ENDLESS_CONVEYOR_EEL_4),
    { .label = "Plats suivants",
      .cost_benefit = "Le tapis continue d'avancer.",
      .result = "Le tapis roule encore.",
      .effects = ENDLESS_CONVEYOR_GO_5 },
};
inline constexpr EventOption ENDLESS_CONVEYOR_BELT_2B[] = {
    endless_conveyor_dish_option("Foie en gelée", "-40 Or, transforme une carte.", ENDLESS_CONVEYOR_JELLY_RESULT, ENDLESS_CONVEYOR_JELLY_5),
    endless_conveyor_dish_option("Salade seapunk", "-40 Or, ajoute Repas frénétique.", ENDLESS_CONVEYOR_SALAD_RESULT, ENDLESS_CONVEYOR_SALAD_5),
    { .label = "Plats suivants",
      .cost_benefit = "Le tapis continue d'avancer.",
      .result = "Le tapis roule encore.",
      .effects = ENDLESS_CONVEYOR_GO_6 },
};
inline constexpr EventOption ENDLESS_CONVEYOR_BELT_3B[] = {
    endless_conveyor_dish_option("Vivaneau épicé", "-40 Or, améliore une carte.", ENDLESS_CONVEYOR_SNAPPY_RESULT, ENDLESS_CONVEYOR_SNAPPY_6),
    endless_conveyor_dish_option("Condiment suspect", "-40 Or, une potion aléatoire.", ENDLESS_CONVEYOR_CONDIMENT_RESULT, ENDLESS_CONVEYOR_CONDIMENT_6),
    { .label = "Plats précédents",
      .cost_benefit = "Revenir au début du tapis.",
      .result = "Le tapis ramène les premiers plats.",
      .effects = ENDLESS_CONVEYOR_GO_4 },
    { .label = "Partir",
      .cost_benefit = "Quitter la baraque.",
      .result = "Vous quittez la baraque, le ventre plein, le chef agitant six bras d'adieu." },
};

inline constexpr std::string_view ENDLESS_CONVEYOR_BELT_TEXT = "Le convoyeur de chitine serpente devant vous, chargé de plats. 40 Pièces d'or par assiette.";

inline constexpr EventPage ENDLESS_CONVEYOR_PAGES[] = {
    { .text = "Vous entrez dans une baraque à l'enseigne de travers : « FESTIN À VOLONTÉ - PAYEZ SELON VOTRE FAIM ! » "
              "Un chef svelte à nombreux bras pose des bouchées sur un convoyeur de chitine. Un panneau : 40 Pièces d'or "
              "par assiette.",
      .options = ENDLESS_CONVEYOR_START },
    { .text = ENDLESS_CONVEYOR_BELT_TEXT, .options = ENDLESS_CONVEYOR_BELT_1 },
    { .text = ENDLESS_CONVEYOR_BELT_TEXT, .options = ENDLESS_CONVEYOR_BELT_2 },
    { .text = ENDLESS_CONVEYOR_BELT_TEXT, .options = ENDLESS_CONVEYOR_BELT_3 },
    { .text = ENDLESS_CONVEYOR_BELT_TEXT, .options = ENDLESS_CONVEYOR_BELT_1B },
    { .text = ENDLESS_CONVEYOR_BELT_TEXT, .options = ENDLESS_CONVEYOR_BELT_2B },
    { .text = ENDLESS_CONVEYOR_BELT_TEXT, .options = ENDLESS_CONVEYOR_BELT_3B },
};

inline constexpr EventDef ENDLESS_CONVEYOR = {
    .id = "ENDLESS_CONVEYOR",
    .title = "Convoyeur sans fin",
    .acts = EVENT_ACT_1B,
    .pages = ENDLESS_CONVEYOR_PAGES,
};

} // namespace engine::events
