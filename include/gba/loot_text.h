#pragma once

#include <string_view>

#include "bn_string_view.h"

namespace gba
{

// Texts of the relics and potions a run can obtain, for the screens that offer them: the rewards of a fight and
// the merchant. Names and potion texts come from the engine databases; an unknown id is shown as it is.

[[nodiscard]] bn::string_view relic_name(std::string_view relic_id);
// "Relique rare", "Relique du marchand"...
[[nodiscard]] bn::string_view relic_kind(std::string_view relic_id);
// What a relic does, in French. Empty for a relic without a text here: only the relics a run can obtain
// (engine::RunManager::active_relics()) have one.
[[nodiscard]] bn::string_view relic_description(std::string_view relic_id);

[[nodiscard]] bn::string_view potion_name(std::string_view potion_id);
// "Potion commune", "Potion rare"...
[[nodiscard]] bn::string_view potion_kind(std::string_view potion_id);
[[nodiscard]] bn::string_view potion_description(std::string_view potion_id);

}
