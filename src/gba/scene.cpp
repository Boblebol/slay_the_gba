#include "gba/scene.h"

#include "bn_sstream.h"

#include "engine/relic.h"

#include "gba/audio.h"
#include "gba/scene_factories.h"

namespace gba
{

bn::unique_ptr<Scene> create_scene(SceneId id, GameContext& context)
{
    switch(id)
    {
    case SceneId::SPLASH:
        return make_splash_scene(context);
    case SceneId::MAP:
        return make_map_scene(context);
    case SceneId::DECK:
        return make_deck_scene(context);
    case SceneId::DECK_CHOICE:
        return make_deck_choice_scene(context);
    case SceneId::COMBAT:
        return make_combat_scene(context);
    case SceneId::REWARDS:
        return make_rewards_scene(context);
    case SceneId::EVENT:
        return make_event_scene(context);
    case SceneId::CAMPFIRE:
        return make_campfire_scene(context);
    case SceneId::SHOP:
        return make_shop_scene(context);
    case SceneId::ANCIENT:
        return make_ancient_scene(context);
    case SceneId::CREDITS:
        return make_credits_scene(context);
    case SceneId::VICTORY:
        return make_end_scene(context, true);
    case SceneId::GAME_OVER:
        return make_end_scene(context, false);
    default:
        return make_title_scene(context);
    }
}

SceneId scene_after_deck_choice(GameContext& context, SceneId back)
{
    if(! context.run.has_deck_choice())
    {
        return back;
    }

    context.deck_choice_return = back;
    context.transition = Transition::QUICK;
    return SceneId::DECK_CHOICE;
}

SceneId scene_for_run_phase(GameContext& context)
{
    engine::RunManager& run = context.run;

    switch(run.phase)
    {
    case engine::RunPhase::IN_COMBAT:
        context.transition = Transition::BATTLE;
        return SceneId::COMBAT;
    case engine::RunPhase::EVENT:
        return SceneId::EVENT;
    case engine::RunPhase::CAMPFIRE:
        return SceneId::CAMPFIRE;
    case engine::RunPhase::SHOP:
        return SceneId::SHOP;
    case engine::RunPhase::ANCIENT_BLESSING:
        return SceneId::ANCIENT;
    case engine::RunPhase::TREASURE:
    {
        // The chest is resolved by the engine on entry; announce it on the map.
        run.phase = engine::RunPhase::MAP_NAVIGATION;
        play_sfx(Sfx::RELIC);
        context.map_toast.clear();
        bn::ostringstream stream(context.map_toast);
        const engine::RelicData* relic = engine::find_relic(run.last_treasure_relic);

        if(relic)
        {
            stream << "+50 Or, " << bn::string_view(relic->name.data(), relic->name.size());
        }
        else
        {
            stream << "Coffre : +50 Or";
        }
        return scene_after_deck_choice(context, SceneId::MAP);
    }
    default:
        return SceneId::MAP;
    }
}

}
