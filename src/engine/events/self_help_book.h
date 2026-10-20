#pragma once
// Développement personnel (SELF_HELP_BOOK), all acts. Choices: bible/data/event_choices/c.json.
// The reference lists 7 options: three readings, their three locked variants (no Power / Skill / Attack to
// enchant) and Move On. The locked variants are the same option greyed out (EventCheck::CAN_ENCHANT), which keeps
// the page within MAX_EVENT_OPTIONS.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect SELF_HELP_BOOK_BOOK[] = {
    { .type = EventEffectType::ENCHANT_CARD, .enchantment = Enchantment::SWIFT, .enchant_amount = 2,
      .only_type = static_cast<int8_t>(CardType::POWER) },
};

inline constexpr EventEffect SELF_HELP_BOOK_PASSAGE[] = {
    { .type = EventEffectType::ENCHANT_CARD, .enchantment = Enchantment::NIMBLE, .enchant_amount = 2,
      .only_type = static_cast<int8_t>(CardType::SKILL) },
};

inline constexpr EventEffect SELF_HELP_BOOK_BACK[] = {
    { .type = EventEffectType::ENCHANT_CARD, .enchantment = Enchantment::SHARP, .enchant_amount = 2,
      .only_type = static_cast<int8_t>(CardType::ATTACK) },
};

inline constexpr EventOption SELF_HELP_BOOK_START[] = {
    { .label = "Lire le livre en entier",
      .cost_benefit = "Pouvoir : Célérité 2.",
      .result = "Vous lisez tout le livre. Choisirez un Pouvoir à envoûter avec Célérité 2.",
      .effects = SELF_HELP_BOOK_BOOK,
      .condition = { .check = EventCheck::CAN_ENCHANT, .amount = static_cast<int16_t>(CardType::POWER) } },
    { .label = "Lire un passage au hasard",
      .cost_benefit = "Compétence : Agilité 2.",
      .result = "Vous lisez un passage au hasard. Choisirez une Compétence à envoûter avec Agilité 2.",
      .effects = SELF_HELP_BOOK_PASSAGE,
      .condition = { .check = EventCheck::CAN_ENCHANT, .amount = static_cast<int16_t>(CardType::SKILL) } },
    { .label = "Lire la 4e de couverture",
      .cost_benefit = "Attaque : Tranchant 2.",
      .result = "Vous lisez la quatrième de couverture. Choisirez une Attaque à envoûter avec Tranchant 2.",
      .effects = SELF_HELP_BOOK_BACK,
      .condition = { .check = EventCheck::CAN_ENCHANT, .amount = static_cast<int16_t>(CardType::ATTACK) } },
    { .label = "Passer son chemin",
      .cost_benefit = "Ne rien lire.",
      .result = "Ce livre ensanglanté est bien trop douteux. Vous passez votre chemin." },
};

inline constexpr EventPage SELF_HELP_BOOK_PAGES[] = {
    { .text = "À terre gît un tome intitulé « Les 3 méthodes secrètes pour terrasser la Tour ». Il est incrusté de "
              "sang : vous le jugez hautement douteux. Le lisez-vous ?",
      .options = SELF_HELP_BOOK_START },
};

inline constexpr EventDef SELF_HELP_BOOK = {
    .id = "SELF_HELP_BOOK",
    .title = "Développement personnel",
    .acts = EVENT_ALL_ACTS,
    .pages = SELF_HELP_BOOK_PAGES,
};

} // namespace engine::events
