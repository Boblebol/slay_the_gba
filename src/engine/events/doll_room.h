#pragma once
// Salle aux poupées (DOLL_ROOM), all acts. Choices: bible/data/event_choices/c.json. Three dolls (relics): Daughter of
// the Wind, Mr. Struggles, Bing Bong. "Pick at Random" gives one of them (GAIN_RELIC with a list); "Take Some Time"
// costs 5 HP and offers 2 of the 3 (guessed: always the first two, the bible does not say which are drawn);
// "Examine Each" costs 15 HP and offers all 3.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t DOLL_ROOM_TIME_HP = 5;
inline constexpr int16_t DOLL_ROOM_EXAMINE_HP = 15;

inline constexpr EventEffect DOLL_ROOM_RANDOM[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "DAUGHTER_OF_THE_WIND,MR_STRUGGLES,BING_BONG" },
};
inline constexpr EventEffect DOLL_ROOM_TIME[] = {
    { .type = EventEffectType::LOSE_HP, .amount = DOLL_ROOM_TIME_HP },
    { .type = EventEffectType::GOTO_PAGE, .amount = 1 },
};
inline constexpr EventEffect DOLL_ROOM_EXAMINE[] = {
    { .type = EventEffectType::LOSE_HP, .amount = DOLL_ROOM_EXAMINE_HP },
    { .type = EventEffectType::GOTO_PAGE, .amount = 2 },
};
inline constexpr EventEffect DOLL_ROOM_DAUGHTER[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "DAUGHTER_OF_THE_WIND" },
};
inline constexpr EventEffect DOLL_ROOM_STRUGGLES[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "MR_STRUGGLES" },
};
inline constexpr EventEffect DOLL_ROOM_BING_BONG[] = {
    { .type = EventEffectType::GAIN_RELIC, .ref = "BING_BONG" },
};

inline constexpr EventOption DOLL_ROOM_START[] = {
    { .label = "Choisir au hasard",
      .cost_benefit = "Une poupée au hasard.",
      .result = "Vous attrapez une poupée sans regarder et filez. Vous obtenez une relique !",
      .effects = DOLL_ROOM_RANDOM },
    { .label = "Prendre son temps",
      .cost_benefit = "Perdez 5 PV, 1 poupée sur 2.",
      .result = "Vous prenez votre temps, malgré les cris. Perdez 5 PV.",
      .effects = DOLL_ROOM_TIME },
    { .label = "Examiner chacune",
      .cost_benefit = "Perdez 15 PV, 1 poupée sur 3.",
      .result = "Vous examinez chaque poupée, les oreilles en sang. Perdez 15 PV.",
      .effects = DOLL_ROOM_EXAMINE },
};

inline constexpr EventOption DOLL_ROOM_TWO[] = {
    { .label = "Fille des vents",
      .cost_benefit = "Chaque Attaque donne 1 Armure.",
      .result = "Vous emportez la Fille des vents.",
      .effects = DOLL_ROOM_DAUGHTER },
    { .label = "Monsieur Carnage",
      .cost_benefit = "Dégâts = tour, à tous, chaque tour.",
      .result = "Vous emportez Monsieur Carnage.",
      .effects = DOLL_ROOM_STRUGGLES },
};

inline constexpr EventOption DOLL_ROOM_THREE[] = {
    { .label = "Fille des vents",
      .cost_benefit = "Chaque Attaque donne 1 Armure.",
      .result = "Vous emportez la Fille des vents.",
      .effects = DOLL_ROOM_DAUGHTER },
    { .label = "Monsieur Carnage",
      .cost_benefit = "Dégâts = tour, à tous, chaque tour.",
      .result = "Vous emportez Monsieur Carnage.",
      .effects = DOLL_ROOM_STRUGGLES },
    { .label = "Bing bong",
      .cost_benefit = "Chaque carte ajoutée est copiée.",
      .result = "Vous emportez Bing bong.",
      .effects = DOLL_ROOM_BING_BONG },
};

inline constexpr EventPage DOLL_ROOM_PAGES[] = {
    { .text = "Une pièce dissimulée, remplie de poupées toutes uniques. Leurs murmures enflent jusqu'aux hurlements: "
              "elles veulent que vous les choisissiez. Prenez-en une et partez vite.",
      .options = DOLL_ROOM_START },
    { .text = "Deux poupées se détachent des autres. Laquelle emportez-vous ?", .options = DOLL_ROOM_TWO },
    { .text = "Après un examen minutieux, trois poupées se distinguent. Laquelle emportez-vous ?",
      .options = DOLL_ROOM_THREE },
};

inline constexpr EventDef DOLL_ROOM = {
    .id = "DOLL_ROOM",
    .title = "Salle aux poupées",
    .acts = EVENT_ALL_ACTS,
    .pages = DOLL_ROOM_PAGES,
};

} // namespace engine::events
