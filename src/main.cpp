#include "bn_bg_palettes.h"
#include "bn_core.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/fx.h"
#include "gba/save.h"
#include "gba/scene.h"
#include "gba/text.h"

namespace
{

constexpr int FADE_IN_FRAMES = 14;
constexpr int FADE_OUT_FRAMES = 12;
constexpr int QUICK_FADE_FRAMES = 5;

void play_exit_transition(gba::Transition transition)
{
    if(transition == gba::Transition::BATTLE)
    {
        // Two white flashes, then a fast fade to black, like entering a wild battle.
        for(int flash = 0; flash < 2; ++flash)
        {
            for(int frame = 0; frame < 10; ++frame)
            {
                bn::fixed t = frame < 5 ? bn::fixed(frame) / 5 : bn::fixed(10 - frame) / 5;
                gba::set_screen_fade(bn::color(31, 31, 31), t * bn::fixed(0.85));
                gba::update_audio();
                bn::core::update();
            }
        }

        for(int frame = 1; frame <= 10; ++frame)
        {
            gba::set_screen_fade(bn::fixed(frame) / 10);
            gba::update_audio();
            bn::core::update();
        }

        return;
    }

    int frames = transition == gba::Transition::QUICK ? QUICK_FADE_FRAMES : FADE_OUT_FRAMES;

    for(int frame = 1; frame <= frames; ++frame)
    {
        gba::set_screen_fade(gba::ease_in_out(bn::fixed(frame) / frames));
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
    gba::SceneId next = gba::SceneId::TITLE;
    gba::set_screen_fade(1);

    while(true)
    {
        bool quick = next == gba::SceneId::DECK || next == gba::SceneId::DECK_CHOICE || context->combat_resume;
        int fade_frames = quick ? QUICK_FADE_FRAMES : FADE_IN_FRAMES;
        context->transition = gba::Transition::FADE;
        bn::unique_ptr<gba::Scene> scene = gba::create_scene(next, *context);
        bn::optional<gba::SceneId> result;
        int frame = 0;

        while(! result)
        {
            ++context->frame;

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
