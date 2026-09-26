#pragma once

#include "gba/scene.h"

namespace gba
{

[[nodiscard]] bn::unique_ptr<Scene> make_title_scene(GameContext& context);
[[nodiscard]] bn::unique_ptr<Scene> make_map_scene(GameContext& context);
[[nodiscard]] bn::unique_ptr<Scene> make_deck_scene(GameContext& context);
[[nodiscard]] bn::unique_ptr<Scene> make_combat_scene(GameContext& context);
[[nodiscard]] bn::unique_ptr<Scene> make_rewards_scene(GameContext& context);
[[nodiscard]] bn::unique_ptr<Scene> make_event_scene(GameContext& context);
[[nodiscard]] bn::unique_ptr<Scene> make_campfire_scene(GameContext& context);
[[nodiscard]] bn::unique_ptr<Scene> make_shop_scene(GameContext& context);
[[nodiscard]] bn::unique_ptr<Scene> make_end_scene(GameContext& context, bool victory);

}
