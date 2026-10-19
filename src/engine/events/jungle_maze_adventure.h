#pragma once
// Aventure du labyrinthe de la jungle (JUNGLE_MAZE_ADVENTURE), Act 1 (A). Choices: bible/data/event_choices/a.json.
// The gold is random in the reference (35-65 together, 135-165 alone): the events have no random amount, so the
// value is fixed, the one keengamer gives (42 and 140).
#include "engine/event_script.h"

namespace engine::events {

constexpr int16_t JUNGLE_MAZE_TOGETHER_GOLD = 42;
constexpr int16_t JUNGLE_MAZE_SOLO_GOLD = 140;
constexpr int16_t JUNGLE_MAZE_SOLO_HP = 18;

inline constexpr EventEffect JUNGLE_MAZE_ADVENTURE_TOGETHER[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = JUNGLE_MAZE_TOGETHER_GOLD },
};

inline constexpr EventEffect JUNGLE_MAZE_ADVENTURE_SOLO[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = JUNGLE_MAZE_SOLO_GOLD },
    { .type = EventEffectType::LOSE_HP, .amount = JUNGLE_MAZE_SOLO_HP },
};

inline constexpr EventOption JUNGLE_MAZE_ADVENTURE_START[] = {
    { .label = "Unir ses forces",
      .cost_benefit = "Gagnez 42 Or.",
      .result = "Vous explorez le labyrinthe en équipe et partagez le butin. Vous obtenez 42 Or.",
      .effects = JUNGLE_MAZE_ADVENTURE_TOGETHER },
    { .label = "Quête en solo",
      .cost_benefit = "Gagnez 140 Or, perdez 18 PV.",
      .result = "Seul contre les pièges, vous gardez tout le trésor. Vous obtenez 140 Or, mais y laissez 18 PV.",
      .effects = JUNGLE_MAZE_ADVENTURE_SOLO,
      .condition = { .check = EventCheck::MIN_HP, .amount = JUNGLE_MAZE_SOLO_HP } },
};

inline constexpr EventPage JUNGLE_MAZE_ADVENTURE_PAGES[] = {
    { .text = "Dans une clairière, une bande d'aventuriers hétéroclite, l'air démoralisée, vous indique un labyrinthe "
              "géant. Ils vous proposent de piller ses trésors ensemble, mais il faudra alors partager le butin.",
      .options = JUNGLE_MAZE_ADVENTURE_START },
};

inline constexpr EventDef JUNGLE_MAZE_ADVENTURE = {
    .id = "JUNGLE_MAZE_ADVENTURE",
    .title = "Aventure du labyrinthe de la jungle",
    .acts = EVENT_ACT_1A,
    .pages = JUNGLE_MAZE_ADVENTURE_PAGES,
};

} // namespace engine::events
