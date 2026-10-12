#pragma once

#include "bn_optional.h"
#include "bn_string.h"
#include "bn_unique_ptr.h"

#include "engine/navigation.h"
#include "engine/run.h"

namespace gba
{

enum class SceneId : uint8_t
{
    SPLASH, // boot credits, shown once before the title
    TITLE,
    MAP,
    DECK,
    DECK_CHOICE, // cards of the deck the run asks for (see engine::DeckChoice)
    COMBAT,
    REWARDS,
    EVENT,
    CAMPFIRE,
    SHOP,
    ANCIENT, // the Ancient met at the start of an act, who offers its relics
    CREDITS, // end credits scrolling after a winning run
    VICTORY,
    GAME_OVER
};

// How the screen changes when leaving a scene.
enum class Transition : uint8_t
{
    FADE,     // fade to black
    BATTLE,   // Pokemon-like white flashes before the fight
    QUICK     // short fade, used for overlays such as the deck viewer
};

// State shared by every scene for the whole session.
struct GameContext
{
    engine::RunManager run;
    engine::MapNavigator nav{ run };
    engine::PaletteVariant palette = engine::PaletteVariant::CLASSIC;
    SceneId deck_return = SceneId::MAP;
    SceneId deck_choice_return = SceneId::MAP; // scene to come back to once the deck choice is made
    Transition transition = Transition::FADE;
    bool combat_resume = false;     // returning from the deck viewer: skip the combat intro
    bn::string<64> map_toast;       // message shown when the map scene opens (treasure, act change...)
    int frame = 0;                  // global frame counter
};

class Scene
{
public:
    virtual ~Scene() = default;

    // Advances one frame. Returns the next scene once this one is finished.
    [[nodiscard]] virtual bn::optional<SceneId> update() = 0;
};

[[nodiscard]] bn::unique_ptr<Scene> create_scene(SceneId id, GameContext& context);

// Scene to open next when the run asks for cards of the deck: the choice first, then `back`.
[[nodiscard]] SceneId scene_after_deck_choice(GameContext& context, SceneId back);

// Scene to open for the room the run just entered (combat, event, shop...).
[[nodiscard]] SceneId scene_for_run_phase(GameContext& context);

}
