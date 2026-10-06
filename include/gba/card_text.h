#pragma once

#include "bn_string.h"
#include "bn_string_view.h"

#include "engine/card.h"
#include "engine/combat.h"
#include "engine/entity.h"

namespace gba
{

// French display name (with a trailing "+" once upgraded).
void card_name(const engine::DeckCard& card, bn::istring& out);

// French rules text computed from the engine's values, so it always matches the actual effect.
void card_description(const engine::DeckCard& card, bn::istring& out);

// Energy cost as printed on the card: a digit, or "X" for a card that spends all the energy.
[[nodiscard]] bn::string_view card_cost_label(const engine::DeckCard& card);

// Label of an energy cost worked out during a combat: "0" to "12", "X".
[[nodiscard]] bn::string_view cost_label(int cost);

// Star cost (Regent) in the same form, empty for the cards that cost none.
[[nodiscard]] bn::string_view card_star_cost_label(const engine::DeckCard& card);

// "Attaque", "Compétence" or "Pouvoir".
[[nodiscard]] bn::string_view card_type_name(engine::CardType type);

// Compact status line such as "Force+2 Vuln2 Poison5".
// `max_width`: pixels the line may take; the statuses beyond it are replaced by a "+" mark.
void status_line(const engine::CombatEntity& entity, bn::istring& out, int max_width);

// Short name of a power of the enemies, as shown in a status line; empty for the other powers.
[[nodiscard]] bn::string_view enemy_power_label(engine::PowerId id);
// What that power does, on one line of the text box.
[[nodiscard]] bn::string_view enemy_power_description(engine::PowerId id);
// Adds to a status line the powers of the enemies the player holds (`holder` -1) or an enemy holds, while
// they fit in `max_width`.
void append_enemy_powers(const engine::CombatEngine& combat, int holder, bn::istring& out, int max_width);

}
