#pragma once
// Répi, historien de la guerre (WAR_HISTORIAN_REPY), all acts. Choices: bible/data/event_choices/a.json.
// The scholar's single-use keys are the Lantern Key quest card: the event is only met with one in the deck, and
// either option spends it. The cage gives the History Course relic; the chest, 2 random potions and 2 random relics.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventCondition WAR_HISTORIAN_REPY_KEY = { .check = EventCheck::HAS_CARD, .card = CardId::LANTERN_KEY };

inline constexpr EventEffect WAR_HISTORIAN_REPY_CAGE[] = {
    { .type = EventEffectType::LOSE_CARD, .card = CardId::LANTERN_KEY },
    { .type = EventEffectType::GAIN_RELIC, .ref = "HISTORY_COURSE" },
};

inline constexpr EventEffect WAR_HISTORIAN_REPY_CHEST[] = {
    { .type = EventEffectType::LOSE_CARD, .card = CardId::LANTERN_KEY },
    { .type = EventEffectType::GAIN_POTION, .amount = 2 },
    { .type = EventEffectType::GAIN_RELIC },
    { .type = EventEffectType::GAIN_RELIC },
};

inline constexpr EventOption WAR_HISTORIAN_REPY_START[] = {
    { .label = "Ouvrir la cage",
      .cost_benefit = "Clé perdue, Manuel d'Histoire.",
      .result = "La cage s'ouvre. Répi vous offre son Manuel d'Histoire et file à ses archives.",
      .effects = WAR_HISTORIAN_REPY_CAGE,
      .condition = WAR_HISTORIAN_REPY_KEY },
    { .label = "Ouvrir le coffre",
      .cost_benefit = "Clé perdue, 2 potions, 2 reliques.",
      .result = "Le coffre s'ouvre sur 2 potions et 2 reliques. Répi proteste dans sa cage.",
      .effects = WAR_HISTORIAN_REPY_CHEST,
      .condition = WAR_HISTORIAN_REPY_KEY },
};

inline constexpr EventPage WAR_HISTORIAN_REPY_PAGES[] = {
    { .text = "Un érudit est prisonnier d'une cage suspendue, près d'un coffre. Il marmonne sur des « clés à usage "
              "unique » en griffonnant sur son parchemin. Vous pouvez ouvrir la cage ou le coffre.",
      .options = WAR_HISTORIAN_REPY_START },
};

inline constexpr EventDef WAR_HISTORIAN_REPY = {
    .id = "WAR_HISTORIAN_REPY",
    .title = "Répi, historien de la guerre",
    .acts = EVENT_ALL_ACTS,
    .pages = WAR_HISTORIAN_REPY_PAGES,
    .condition = WAR_HISTORIAN_REPY_KEY,
};

} // namespace engine::events
