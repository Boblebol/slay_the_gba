#pragma once
// Pont glissant (SLIPPERY_BRIDGE), every act. Choices: bible/data/event_choices/b.json.
// "Se reposer" heals then leads to a second page whose only option is the fight (the Wrigglers group of the
// reference is a normal fight of the act here).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t SLIPPERY_BRIDGE_HP_COST = 8;
// Devinée: the heal amount of "Se reposer" is not in the reference (one source says 25).
inline constexpr int16_t SLIPPERY_BRIDGE_HEAL = 25;

inline constexpr EventEffect SLIPPERY_BRIDGE_TRUDGE[] = {
    { .type = EventEffectType::LOSE_HP, .amount = SLIPPERY_BRIDGE_HP_COST },
    { .type = EventEffectType::REMOVE_CARD, .amount = 1 },
};

inline constexpr EventEffect SLIPPERY_BRIDGE_REST[] = {
    { .type = EventEffectType::HEAL, .amount = SLIPPERY_BRIDGE_HEAL },
    { .type = EventEffectType::GOTO_PAGE, .amount = 1 },
};

inline constexpr EventEffect SLIPPERY_BRIDGE_FIGHT[] = {
    { .type = EventEffectType::FIGHT },
};

inline constexpr EventOption SLIPPERY_BRIDGE_START[] = {
    { .label = "Persévérer",
      .cost_benefit = "Perdez 8 PV, retirez une carte.",
      .result = "Vous avancez contre le vent, à pas lents. Choisissez une carte à retirer de votre deck.",
      .effects = SLIPPERY_BRIDGE_TRUDGE,
      .condition = { .check = EventCheck::MIN_HP, .amount = SLIPPERY_BRIDGE_HP_COST } },
    { .label = "Se reposer",
      .cost_benefit = "Soignez 25 PV, puis combattez.",
      .result = "Vous vous abritez et reprenez des forces. Mais quelque chose remue sous le pont...",
      .effects = SLIPPERY_BRIDGE_REST },
};

inline constexpr EventOption SLIPPERY_BRIDGE_FIGHT_OPTIONS[] = {
    { .label = "Combattre !",
      .cost_benefit = "Un combat commence.",
      .result = "Des larves surgissent de sous le pont !",
      .effects = SLIPPERY_BRIDGE_FIGHT },
};

inline constexpr EventPage SLIPPERY_BRIDGE_PAGES[] = {
    { .text = "Alors que vous traversez un pont de bois chancelant, un torrent de pluie se lève. De puissantes "
              "rafales de vent menacent de mettre fin à votre périple.",
      .options = SLIPPERY_BRIDGE_START },
    { .text = "Des créatures grouillantes montent à l'assaut du pont.", .options = SLIPPERY_BRIDGE_FIGHT_OPTIONS },
};

inline constexpr EventDef SLIPPERY_BRIDGE = {
    .id = "SLIPPERY_BRIDGE",
    .title = "Pont glissant",
    .acts = EVENT_ALL_ACTS,
    .pages = SLIPPERY_BRIDGE_PAGES,
};

} // namespace engine::events
