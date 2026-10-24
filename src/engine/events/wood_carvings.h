#pragma once
// Gravures sur bois (WOOD_CARVINGS), Act 1 (A). Choices: bible/data/event_choices/b.json.
// Guessed: the Slither enchantment does not exist, the card gets Royally Approved (Innate and Retain) instead; the
// card transformed by the Bird and the Torus can be any card of the deck, not only a starter one; the fourth
// option is the usual "leave".
#include "engine/event_script.h"

namespace engine::events {

inline constexpr EventEffect WOOD_CARVINGS_BIRD[] = {
    { .type = EventEffectType::TRANSFORM_CARD, .amount = 1, .card = CardId::PECK },
};

inline constexpr EventEffect WOOD_CARVINGS_SNAKE[] = {
    { .type = EventEffectType::ENCHANT_CARD, .amount = 1, .enchantment = Enchantment::ROYALLY_APPROVED, .enchant_amount = 1,
      .only_type = static_cast<int8_t>(CardType::ATTACK) },
};

inline constexpr EventEffect WOOD_CARVINGS_TORUS[] = {
    { .type = EventEffectType::TRANSFORM_CARD, .amount = 1, .card = CardId::TORIC_TOUGHNESS },
};

inline constexpr EventOption WOOD_CARVINGS_START[] = {
    { .label = "L'oiseau",
      .cost_benefit = "Transformez une carte en Picorer.",
      .result = "L'oiseau s'emboîte parfaitement. Choisissez une carte à transformer en Picorer.",
      .effects = WOOD_CARVINGS_BIRD },
    { .label = "Le serpent",
      .cost_benefit = "Enchantez une Attaque avec Glissement.",
      .result = "Le serpent glisse dans son logement. Choisissez une Attaque à enchanter.",
      .effects = WOOD_CARVINGS_SNAKE,
      .condition = { .check = EventCheck::HAS_ATTACK } },
    { .label = "Le tore",
      .cost_benefit = "Transformez une carte en Robustesse torique.",
      .result = "Le tore tourne sur lui-même. Choisissez une carte à transformer en Robustesse torique.",
      .effects = WOOD_CARVINGS_TORUS },
    { .label = "Partir",
      .cost_benefit = "Ne rien placer.",
      .result = "Vous refermez la boîte sans toucher aux figurines." },
};

inline constexpr EventPage WOOD_CARVINGS_PAGES[] = {
    { .text = "Vous ouvrez une boîte poussiéreuse : 3 figurines en bois gravées. Un oiseau, un serpent et... un tore ? "
              "Un piédestal proche a des emplacements pour leur base. Laquelle placez-vous ?",
      .options = WOOD_CARVINGS_START },
};

inline constexpr EventDef WOOD_CARVINGS = {
    .id = "WOOD_CARVINGS",
    .title = "Gravures sur bois",
    .acts = EVENT_ACT_1A,
    .pages = WOOD_CARVINGS_PAGES,
};

} // namespace engine::events
