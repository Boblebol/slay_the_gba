#pragma once
// À coups de poing (PUNCH_OFF), Act 1 (B). Choices: bible/data/event_choices/b.json.
// Nab the treasure (Injury curse, a random relic) or fight the two Punch Constructs for better rewards.
// The rewards of the fight are not documented in the reference (marked unknown): the fight gives a relic like an
// Elite's, on top of the usual card, gold and potion rewards (guessed, see RunManager::generate_rewards).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect PUNCH_OFF_NAB[] = {
    { .type = EventEffectType::ADD_CURSE, .card = CardId::INJURY },
    { .type = EventEffectType::GAIN_RELIC },
};

inline constexpr EventEffect PUNCH_OFF_FIGHT[] = {
    { .type = EventEffectType::FIGHT, .ref = EVENT_FIGHT_PUNCH_CONSTRUCTS },
};

inline constexpr EventOption PUNCH_OFF_START[] = {
    { .label = "Chiper",
      .cost_benefit = "Relique aléatoire, +Blessure.",
      .result = "Vous filez le trésor sous le nez des combattants. Un coup au passage vous laisse une Blessure.",
      .effects = PUNCH_OFF_NAB },
    { .label = "Je peux les prendre",
      .cost_benefit = "Combat : relique et récompenses.",
      .result = "Vous vous jetez dans la mêlée ! Les deux Assemblages se tournent vers vous.",
      .effects = PUNCH_OFF_FIGHT },
};

inline constexpr EventPage PUNCH_OFF_PAGES[] = {
    { .text = "Deux Assemblages de pugilat sont en pleine bagarre, et vous apercevez entre eux un trésor... "
              "Devriez-vous essayer de l'attraper ?",
      .options = PUNCH_OFF_START },
};

inline constexpr EventDef PUNCH_OFF = {
    .id = "PUNCH_OFF",
    .title = "À coups de poing",
    .acts = EVENT_ACT_1B,
    .pages = PUNCH_OFF_PAGES,
};

} // namespace engine::events
