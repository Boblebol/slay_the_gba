#pragma once
// La clé de la lanterne (THE_LANTERN_KEY): the special event the Lantern Key quest card brings to the first ? room
// of the act after the one it was found in (RunManager::trigger_event). Never drawn at random (`acts` 0). Kept as
// #446 wrote it until its own issue gives it the choices of the reference.
#include "engine/event_script.h"
#include "engine/run.h" // LANTERN_KEY_GOLD

namespace engine::events {

inline constexpr EventEffect THE_LANTERN_KEY_GIVE_BACK[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = LANTERN_KEY_GOLD },
};

inline constexpr EventEffect THE_LANTERN_KEY_KEEP[] = {
    { .type = EventEffectType::GAIN_POTION, .ref = "FIRE_POTION" },
};

inline constexpr EventOption THE_LANTERN_KEY_START[] = {
    { .label = "Rendre la clé",
      .cost_benefit = "Gagnez 100 Or.",
      .result = "L'inconnu vous remercie avec une bourse bien garnie.",
      .effects = THE_LANTERN_KEY_GIVE_BACK },
    { .label = "Garder la clé",
      .cost_benefit = "Gagnez une potion.",
      .result = "Vous refusez. L'inconnu vous lance une fiole avant de disparaître.",
      .effects = THE_LANTERN_KEY_KEEP },
};

inline constexpr EventPage THE_LANTERN_KEY_PAGES[] = {
    { .text = "La clé luminescente vibre dans votre sac. Un inconnu surgit : « La voilà, je la cherchais partout ! »",
      .options = THE_LANTERN_KEY_START },
};

inline constexpr EventDef THE_LANTERN_KEY = {
    .id = "THE_LANTERN_KEY",
    .title = "La clé de la lanterne",
    .acts = 0,
    .pages = THE_LANTERN_KEY_PAGES,
};

} // namespace engine::events
