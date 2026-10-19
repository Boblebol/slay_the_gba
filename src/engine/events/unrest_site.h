#pragma once
// Site sans repos (UNREST_SITE), Acte 1 (A). Choices: bible/data/event_choices/a.json.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect UNREST_SITE_REST[] = {
    { .type = EventEffectType::HEAL, .amount = 9999 }, // up to the max HP
    { .type = EventEffectType::ADD_CURSE, .card = CardId::POOR_SLEEP },
};

inline constexpr EventEffect UNREST_SITE_TREES[] = {
    { .type = EventEffectType::LOSE_MAX_HP, .amount = 8 },
    { .type = EventEffectType::GAIN_RELIC },
};

inline constexpr EventOption UNREST_SITE_START[] = {
    { .label = "Dormir quand même",
      .cost_benefit = "Soignez tous vos PV, Mauvais sommeil.",
      .result = "Vous dormez d'un sommeil agité. Vos PV sont restaurés, mais Mauvais sommeil rejoint votre deck.",
      .effects = UNREST_SITE_REST },
    { .label = "Abattre les arbres",
      .cost_benefit = "Perdez 8 PV Max, gagnez une relique.",
      .result = "Vous abattez les arbres dégoulinants de sève. Une relique roule à vos pieds, mais vous perdez 8 PV Max.",
      .effects = UNREST_SITE_TREES },
};

inline constexpr EventPage UNREST_SITE_PAGES[] = {
    { .text = "Vous découvrez un Site de repos isolé et allumez un feu pour vous reposer. Ou du moins, c'est ce que "
              "vous pensiez. Le feu croît de tous côtés et s'étend vers un bosquet d'arbres dégoulinant de sève. "
              "Essayez-vous de trouver le sommeil malgré tout ?",
      .options = UNREST_SITE_START },
};

inline constexpr EventDef UNREST_SITE = {
    .id = "UNREST_SITE",
    .title = "Site sans repos",
    .acts = EVENT_ACT_1A,
    .pages = UNREST_SITE_PAGES,
};

} // namespace engine::events
