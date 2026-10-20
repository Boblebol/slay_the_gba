#pragma once
// Automate infesté (INFESTED_AUTOMATON), Act 2. Choices: bible/data/event_choices/a.json.
// Both rewards are a random card of the hero's pool (ADD_RANDOM_CARD): a Power, or a card costing 0.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect INFESTED_AUTOMATON_STUDY[] = {
    { .type = EventEffectType::ADD_RANDOM_CARD, .amount = 1, .ref = "POWER" },
};

inline constexpr EventEffect INFESTED_AUTOMATON_TOUCH[] = {
    { .type = EventEffectType::ADD_RANDOM_CARD, .amount = 1, .ref = "ZERO_COST" },
};

inline constexpr EventOption INFESTED_AUTOMATON_START[] = {
    { .label = "Étudier",
      .cost_benefit = "Ajoute un Pouvoir aléatoire.",
      .result = "Vous étudiez les circuits de l'automate et en tirez un savoir-faire inattendu.",
      .effects = INFESTED_AUTOMATON_STUDY },
    { .label = "Toucher le coeur",
      .cost_benefit = "Ajoute une carte aléatoire à 0.",
      .result = "Votre main effleure le coeur luisant. Une carte vous revient en mémoire.",
      .effects = INFESTED_AUTOMATON_TOUCH },
};

inline constexpr EventPage INFESTED_AUTOMATON_PAGES[] = {
    { .text = "Vous arrivez dans une salle remplie de robots morts. Un seul automate paraît encore en service, mais "
              "son coeur faiblement lumineux est englouti par une excroissance organique grotesque, et il ne réagit "
              "plus.",
      .options = INFESTED_AUTOMATON_START },
};

inline constexpr EventDef INFESTED_AUTOMATON = {
    .id = "INFESTED_AUTOMATON",
    .title = "Automate infesté",
    .acts = EVENT_ACT_2,
    .pages = INFESTED_AUTOMATON_PAGES,
};

} // namespace engine::events
