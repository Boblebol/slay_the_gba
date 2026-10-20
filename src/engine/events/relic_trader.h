#pragma once
// Trafiquant de reliques (RELIC_TRADER), Acts 1 to 3. Choices: bible/data/event_choices/b.json.
// Guessed: in the reference, the relic given up and the one received are drawn when the event opens and differ from
// one option to the next; the screen cannot show them here (static texts), so each of the three offers trades a
// random relic of the run for a random relic it did not own (TRADE_RELIC).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect RELIC_TRADER_TRADE[] = {
    { .type = EventEffectType::TRADE_RELIC },
};

inline constexpr EventOption RELIC_TRADER_START[] = {
    { .label = "Prendre celui du haut",
      .cost_benefit = "Échangez une relique au hasard.",
      .result = "Le trafiquant empoche l'une de vos reliques et vous glisse celle du haut.",
      .effects = RELIC_TRADER_TRADE },
    { .label = "Prendre celui du milieu",
      .cost_benefit = "Échangez une relique au hasard.",
      .result = "Le trafiquant empoche l'une de vos reliques et vous glisse celle du milieu.",
      .effects = RELIC_TRADER_TRADE },
    { .label = "Prendre celui du bas",
      .cost_benefit = "Échangez une relique au hasard.",
      .result = "Le trafiquant empoche l'une de vos reliques et vous glisse celle du bas.",
      .effects = RELIC_TRADER_TRADE },
};

inline constexpr EventPage RELIC_TRADER_PAGES[] = {
    { .text = "Vous tournez un coin lorsque vous vous retrouvez nez-à-nez avec une figure voilée. « Bienvenue ! "
              "T'as quelque chose à troquer ? » Elle ouvre grand son manteau, révélant des marchandises plus "
              "suspectes les unes que les autres...",
      .options = RELIC_TRADER_START },
};

inline constexpr EventDef RELIC_TRADER = {
    .id = "RELIC_TRADER",
    .title = "Trafiquant de reliques",
    .acts = EVENT_ALL_ACTS,
    .pages = RELIC_TRADER_PAGES,
};

} // namespace engine::events
