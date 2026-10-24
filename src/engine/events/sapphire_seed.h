#pragma once
// Graine de saphir (SAPPHIRE_SEED), Act 1 (A). Choices: bible/data/event_choices/b.json.
// Guessed: the Sown enchantment does not exist in the game; the card gets Swift (draw 1 card the first time it is
// played in a combat) instead.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t SAPPHIRE_SEED_HEAL = 9;

inline constexpr EventEffect SAPPHIRE_SEED_CONSUME[] = {
    { .type = EventEffectType::HEAL, .amount = SAPPHIRE_SEED_HEAL },
    { .type = EventEffectType::UPGRADE_CARD, .amount = 1 },
};

inline constexpr EventEffect SAPPHIRE_SEED_PLANT[] = {
    { .type = EventEffectType::ENCHANT_CARD, .amount = 1, .enchantment = Enchantment::SWIFT, .enchant_amount = 1 },
};

inline constexpr EventOption SAPPHIRE_SEED_START[] = {
    { .label = "Consommer",
      .cost_benefit = "Soignez 9 PV. Améliorez une carte.",
      .result = "Vous croquez la graine. Une chaleur vous envahit : 9 PV rendus. Choisissez une carte à améliorer.",
      .effects = SAPPHIRE_SEED_CONSUME },
    { .label = "Planter et nourrir",
      .cost_benefit = "Enchantez une carte avec Semé.",
      .result = "Vous enterrez la graine. Choisissez une carte à enchanter.",
      .effects = SAPPHIRE_SEED_PLANT,
      .condition = { .check = EventCheck::CAN_ENCHANT, .amount = -1 } },
};

inline constexpr EventPage SAPPHIRE_SEED_PAGES[] = {
    { .text = "Vous manquez de peu de marcher sur un éclat de... graine ? Sa couleur ne laisse aucun doute : "
              "c'est une Graine de saphir ! La légende dit qu'elle amplifie l'endurance. Ou qu'on peut la planter.",
      .options = SAPPHIRE_SEED_START },
};

inline constexpr EventDef SAPPHIRE_SEED = {
    .id = "SAPPHIRE_SEED",
    .title = "Graine de saphir",
    .acts = EVENT_ACT_1A,
    .pages = SAPPHIRE_SEED_PAGES,
};

} // namespace engine::events
