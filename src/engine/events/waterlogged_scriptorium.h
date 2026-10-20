#pragma once
// Scriptorium englouti (WATERLOGGED_SCRIPTORIUM), Act 1 (B). Choices: bible/data/event_choices/a.json.
// The reference lists 5 options: the 3 goods and a locked variant of the two paid ones. Here the paid options are
// shown greyed out instead (condition MIN_GOLD), which is the same screen with 3 lines.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t WATERLOGGED_SCRIPTORIUM_INK_MAX_HP = 6;
inline constexpr int16_t WATERLOGGED_SCRIPTORIUM_SPONGE_PRICE = 99;
inline constexpr int16_t WATERLOGGED_SCRIPTORIUM_SPONGE_CARDS = 2;
inline constexpr int16_t WATERLOGGED_SCRIPTORIUM_QUILL_PRICE = 55;
inline constexpr int16_t WATERLOGGED_SCRIPTORIUM_QUILL_CARDS = 1;

inline constexpr EventEffect WATERLOGGED_SCRIPTORIUM_INK[] = {
    { .type = EventEffectType::GAIN_MAX_HP, .amount = WATERLOGGED_SCRIPTORIUM_INK_MAX_HP },
};

inline constexpr EventEffect WATERLOGGED_SCRIPTORIUM_SPONGE[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = WATERLOGGED_SCRIPTORIUM_SPONGE_PRICE },
    { .type = EventEffectType::ENCHANT_CARD, .amount = WATERLOGGED_SCRIPTORIUM_SPONGE_CARDS,
      .enchantment = Enchantment::STEADY },
};

inline constexpr EventEffect WATERLOGGED_SCRIPTORIUM_QUILL[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = WATERLOGGED_SCRIPTORIUM_QUILL_PRICE },
    { .type = EventEffectType::ENCHANT_CARD, .amount = WATERLOGGED_SCRIPTORIUM_QUILL_CARDS,
      .enchantment = Enchantment::STEADY },
};

inline constexpr EventOption WATERLOGGED_SCRIPTORIUM_START[] = {
    { .label = "Encre sanglante",
      .cost_benefit = "Gagnez 6 PV Max.",
      .result = "Vous trempez la plume dans l'encre sanglante. Vous vous sentez plus robuste : +6 PV Max !",
      .effects = WATERLOGGED_SCRIPTORIUM_INK },
    { .label = "Éponge piquante",
      .cost_benefit = "Payez 99 Or, 2 cartes Stables.",
      .result = "Le scribe vous tend l'éponge contre 99 Or. Vous allez choisir 2 cartes à rendre Stables.",
      .effects = WATERLOGGED_SCRIPTORIUM_SPONGE,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = WATERLOGGED_SCRIPTORIUM_SPONGE_PRICE } },
    { .label = "Plume tentaculaire",
      .cost_benefit = "Payez 55 Or, 1 carte Stable.",
      .result = "Le scribe vous cède la plume contre 55 Or. Vous allez choisir 1 carte à rendre Stable.",
      .effects = WATERLOGGED_SCRIPTORIUM_QUILL,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = WATERLOGGED_SCRIPTORIUM_QUILL_PRICE } },
};

inline constexpr EventPage WATERLOGGED_SCRIPTORIUM_PAGES[] = {
    { .text = "Dans les passages embourbés, vous tombez sur une silhouette émaciée qui tient une petite boutique, "
              "où s'entassent des liasses de vélins et de parchemins humides. Le scribe-commerçant accourt et vous "
              "indique le matériel exposé sur le bureau.",
      .options = WATERLOGGED_SCRIPTORIUM_START },
};

inline constexpr EventDef WATERLOGGED_SCRIPTORIUM = {
    .id = "WATERLOGGED_SCRIPTORIUM",
    .title = "Scriptorium englouti",
    .acts = EVENT_ACT_1B,
    .pages = WATERLOGGED_SCRIPTORIUM_PAGES,
};

} // namespace engine::events
