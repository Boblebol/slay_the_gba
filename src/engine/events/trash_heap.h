#pragma once
// Tas de déchets (TRASH_HEAP), Act 1B. Choices: bible/data/event_choices/c.json. Both options draw one element at
// random in the list of the reference (relic_pool / card_pool of the effect, see include/engine/event_script.h).
#include "engine/event_script.h"

namespace engine::events {

inline constexpr std::string_view TRASH_HEAP_RELICS[] = {
    "DARKSTONE_PERIAPT", "DREAM_CATCHER", "HAND_DRILL", "MAW_BANK", "THE_BOOT",
};

inline constexpr CardId TRASH_HEAP_CARDS[] = {
    CardId::CALTROPS, CardId::CLASH,     CardId::DISTRACTION, CardId::DUAL_WIELD,   CardId::ENTRENCH,
    CardId::HELLO_WORLD, CardId::OUTMANEUVER, CardId::REBOUND, CardId::RIP_AND_TEAR, CardId::STACK,
};

inline constexpr EventEffect TRASH_HEAP_DIVE[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 8 },
    { .type = EventEffectType::GAIN_RELIC, .relic_pool = TRASH_HEAP_RELICS },
};

inline constexpr EventEffect TRASH_HEAP_JUNK[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = 100 },
    { .type = EventEffectType::ADD_CARD, .card_pool = TRASH_HEAP_CARDS },
};

inline constexpr EventOption TRASH_HEAP_START[] = {
    { .label = "Plonger dedans",
      .cost_benefit = "Perdez 8 PV, gagnez une relique.",
      .result = "Vous fouillez en profondeur, au prix de quelques coupures. Vous trouvez une relique !",
      .effects = TRASH_HEAP_DIVE },
    { .label = "Bric-à-brac au hasard",
      .cost_benefit = "Gagnez 100 Or et une carte.",
      .result = "Vous ramassez ce qui traîne en surface : de l'or, et une carte oubliée.",
      .effects = TRASH_HEAP_JUNK },
};

inline constexpr EventPage TRASH_HEAP_PAGES[] = {
    { .text = "Un gigantesque tas d'armes et de breloques abandonnées vibre, comme s'il grandissait de l'intérieur. "
              "En surface, vous trouverez sans doute des objets de valeur ; en fouillant plus profond, des trésors "
              "exotiques.",
      .options = TRASH_HEAP_START },
};

inline constexpr EventDef TRASH_HEAP = {
    .id = "TRASH_HEAP",
    .title = "Tas de déchets",
    .acts = EVENT_ACT_1B,
    .pages = TRASH_HEAP_PAGES,
};

} // namespace engine::events
