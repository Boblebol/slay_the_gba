#include "engine/scene_transition.h"

#include <cstdio>
#include <cstdlib>

using namespace engine;

static void check(bool ok, const char* what)
{
    if(!ok)
    {
        std::printf("FAIL: %s\n", what);
        std::exit(1);
    }
}

int main()
{
    const TransitionStyle styles[] = { TransitionStyle::FADE, TransitionStyle::BATTLE, TransitionStyle::QUICK,
                                       TransitionStyle::ROOM };

    // Fondu au noir : chaque transition finit entierement noire, jamais blanche, avec un niveau borne.
    for(TransitionStyle style : styles)
    {
        const int total = transition_frames(style);
        check(total > 0 && total <= 40, "duree bornee");
        check(transition_step(style, total).level == TRANSITION_MAX_LEVEL, "derniere image noire");
        check(!transition_step(style, total).white, "derniere image non blanche");

        int previous_black = 0;

        for(int frame = 1; frame <= total; ++frame)
        {
            TransitionStep step = transition_step(style, frame);
            check(step.level >= 0 && step.level <= TRANSITION_MAX_LEVEL, "niveau borne");

            if(!step.white)
            {
                check(step.level >= previous_black, "fondu au noir monotone");
                previous_black = step.level;
            }
        }
    }

    // Entree en combat : deux flashs blancs puis fondu rapide au noir.
    int peaks = 0;
    int last = 0;
    bool rising = false;

    for(int frame = 1; frame <= BATTLE_FLASH_FRAMES * BATTLE_FLASH_COUNT; ++frame)
    {
        TransitionStep step = transition_step(TransitionStyle::BATTLE, frame);
        check(step.white, "flash blanc pendant l'intro");
        check(step.level <= TRANSITION_MAX_LEVEL * 85 / 100, "flash sous 85 pour cent");

        if(step.level < last && rising)
        {
            ++peaks;
        }

        if(step.level != last)
        {
            rising = step.level > last;
        }

        last = step.level;
    }

    check(peaks == BATTLE_FLASH_COUNT, "deux flashs");
    check(!transition_step(TransitionStyle::BATTLE, BATTLE_FLASH_FRAMES * BATTLE_FLASH_COUNT + 1).white,
          "puis fondu noir");

    // Carte vers salle (evenement, marchand, repos) : rideau plus lent que le fondu simple, qui demarre doucement.
    check(transition_frames(TransitionStyle::ROOM) > transition_frames(TransitionStyle::FADE), "rideau plus long");
    check(transition_step(TransitionStyle::ROOM, 4).level < transition_step(TransitionStyle::FADE, 4).level,
          "rideau demarre doucement");
    check(transition_frames(TransitionStyle::QUICK) < transition_frames(TransitionStyle::FADE), "quick plus court");

    std::printf("scene_transition_test OK\n");
    return 0;
}
