#pragma once
// Philosophes multicolores (COLORFUL_PHILOSOPHERS), Act 2. Choices: bible/data/event_choices/b.json.
// Six options (one color per hero, and Equality), so two pages. Each color is a card reward (3 cards of varied
// rarities, 1 to keep) from that hero's pool. The reference does not offer the player's own color: the data cannot
// hide an option by hero, so all five are shown.
// Equality gives the Prismatic Shard, which is not a relic of the game yet: it is obtained under its id and has no
// effect until a relic issue implements it.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect COLORFUL_PHILOSOPHERS_BLUE[] = {
    { .type = EventEffectType::CARD_REWARD, .ref = "DEFECT" },
};
inline constexpr EventEffect COLORFUL_PHILOSOPHERS_RED[] = {
    { .type = EventEffectType::CARD_REWARD, .ref = "IRONCLAD" },
};
inline constexpr EventEffect COLORFUL_PHILOSOPHERS_PINK[] = {
    { .type = EventEffectType::CARD_REWARD, .ref = "NECROBINDER" },
};
inline constexpr EventEffect COLORFUL_PHILOSOPHERS_ORANGE[] = {
    { .type = EventEffectType::CARD_REWARD, .ref = "REGENT" },
};
inline constexpr EventEffect COLORFUL_PHILOSOPHERS_GREEN[] = {
    { .type = EventEffectType::CARD_REWARD, .ref = "SILENT" },
};
inline constexpr EventEffect COLORFUL_PHILOSOPHERS_EQUALITY[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "PRISMATIC_SHARD" },
};
inline constexpr EventEffect COLORFUL_PHILOSOPHERS_MORE[] = {
    { .type = EventEffectType::GOTO_PAGE, .amount = 1 },
};
inline constexpr EventEffect COLORFUL_PHILOSOPHERS_BACK[] = {
    { .type = EventEffectType::GOTO_PAGE, .amount = 0 },
};

inline constexpr EventOption COLORFUL_PHILOSOPHERS_FIRST[] = {
    { .label = "Bleu",
      .cost_benefit = "3 cartes Defect, une au choix.",
      .result = "Les statues bleues exultent. Vous choisirez l'une de leurs cartes.",
      .effects = COLORFUL_PHILOSOPHERS_BLUE },
    { .label = "Rouge",
      .cost_benefit = "3 cartes Ironclad, une au choix.",
      .result = "Les statues rouges rugissent de joie. Vous choisirez l'une de leurs cartes.",
      .effects = COLORFUL_PHILOSOPHERS_RED },
    { .label = "Rose",
      .cost_benefit = "3 cartes Necrobinder, une au choix.",
      .result = "Les statues roses sourient. Vous choisirez l'une de leurs cartes.",
      .effects = COLORFUL_PHILOSOPHERS_PINK },
    { .label = "Autres couleurs",
      .cost_benefit = "Orange, vert ou Égalité.",
      .result = "Vous reprenez la parole.",
      .effects = COLORFUL_PHILOSOPHERS_MORE },
};

inline constexpr EventOption COLORFUL_PHILOSOPHERS_SECOND[] = {
    { .label = "Orange",
      .cost_benefit = "3 cartes Regent, une au choix.",
      .result = "Les statues orange se rengorgent. Vous choisirez l'une de leurs cartes.",
      .effects = COLORFUL_PHILOSOPHERS_ORANGE },
    { .label = "Vert",
      .cost_benefit = "3 cartes Silent, une au choix.",
      .result = "Les statues vertes hochent la tête. Vous choisirez l'une de leurs cartes.",
      .effects = COLORFUL_PHILOSOPHERS_GREEN },
    { .label = "Égalité",
      .cost_benefit = "Gagnez l'Éclat prismatique.",
      .result = "Aucune couleur n'est meilleure qu'une autre ! Les statues vous remercient : vous obtenez l'Éclat prismatique.",
      .effects = COLORFUL_PHILOSOPHERS_EQUALITY },
    { .label = "Retour",
      .cost_benefit = "Bleu, rouge ou rose.",
      .result = "Vous reprenez la parole.",
      .effects = COLORFUL_PHILOSOPHERS_BACK },
};

inline constexpr EventPage COLORFUL_PHILOSOPHERS_PAGES[] = {
    { .text = "Perchées sur un dais, trois statues de couleurs différentes débattent avec ardeur : quelle couleur est "
              "LA MEILLEURE ? Vous intervenez pour donner votre avis.",
      .options = COLORFUL_PHILOSOPHERS_FIRST },
    { .text = "Les statues attendent votre avis avec impatience. Orange, vert, ou bien aucune couleur ne l'emporte ?",
      .options = COLORFUL_PHILOSOPHERS_SECOND },
};

inline constexpr EventDef COLORFUL_PHILOSOPHERS = {
    .id = "COLORFUL_PHILOSOPHERS",
    .title = "Philosophes multicolores",
    .acts = EVENT_ACT_2,
    .pages = COLORFUL_PHILOSOPHERS_PAGES,
};

} // namespace engine::events
