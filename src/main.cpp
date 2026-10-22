#include "bn_bg_palettes.h"
#include "bn_core.h"

#include "engine/scene_transition.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/fx.h"
#include "gba/save.h"
#include "gba/scene.h"
#include "gba/text.h"

namespace
{

constexpr int FADE_IN_FRAMES = 14;
constexpr int QUICK_FADE_FRAMES = 5;

engine::TransitionStyle transition_style(gba::Transition transition)
{
    switch(transition)
    {
    case gba::Transition::BATTLE:
        return engine::TransitionStyle::BATTLE;
    case gba::Transition::QUICK:
        return engine::TransitionStyle::QUICK;
    case gba::Transition::ROOM:
        return engine::TransitionStyle::ROOM;
    default:
        return engine::TransitionStyle::FADE;
    }
}

// Plays the exit transition described by engine/scene_transition.h through the hardware fade
// registers: one palette write per frame, no scene loading in between, so it stays at 60 FPS.
void play_exit_transition(gba::Transition transition)
{
    engine::TransitionStyle style = transition_style(transition);
    int frames = engine::transition_frames(style);

    for(int frame = 1; frame <= frames; ++frame)
    {
        engine::TransitionStep step = engine::transition_step(style, frame);
        bn::fixed intensity = bn::fixed(step.level) / engine::TRANSITION_MAX_LEVEL;
        gba::set_screen_fade(step.white ? bn::color(31, 31, 31) : bn::color(0, 0, 0), intensity);
        gba::update_audio();
        bn::core::update();
    }
}

}

int main()
{
    bn::core::init();
    bn::bg_palettes::set_transparent_color(gba::hero_backdrop_color());
    gba::init_text();
    gba::init_save();

    auto* context = new gba::GameContext();
    gba::SceneId next = gba::SceneId::SPLASH;
    gba::set_screen_fade(1);

    while(true)
    {
        bool quick = next == gba::SceneId::DECK || next == gba::SceneId::DECK_CHOICE || next == gba::SceneId::PAUSE ||
                     context->combat_resume;
        int fade_frames = quick ? QUICK_FADE_FRAMES : FADE_IN_FRAMES;
        context->transition = gba::Transition::FADE;
        bn::unique_ptr<gba::Scene> scene = gba::create_scene(next, *context);
        bn::optional<gba::SceneId> result;
        int frame = 0;
        bool play_time = gba::scene_counts_play_time(next);

        while(! result)
        {
            ++context->frame;

            if(play_time)
            {
                context->run.count_play_frame();
            }

            if(frame <= fade_frames)
            {
                gba::set_screen_fade(1 - gba::ease_in_out(bn::fixed(frame) / fade_frames));
                ++frame;
            }

            result = scene->update();
            gba::update_audio();
            bn::core::update();
        }

        play_exit_transition(context->transition);
        scene.reset();
        next = *result;
    }
}
