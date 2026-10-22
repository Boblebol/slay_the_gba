#pragma once

// Pure description of the screen transitions played when a scene is left (#18). The GBA side
// (src/main.cpp) only turns each step into a hardware fade; the timing lives here so it is tested.

namespace engine
{

enum class TransitionStyle
{
    FADE,   // fade to black
    BATTLE, // two white flashes then a fast fade to black, before a fight
    QUICK,  // short fade, for overlays such as the deck viewer
    ROOM    // slow curtain to black (ease-in), from the map to an event, a shop or a campfire
};

constexpr int TRANSITION_MAX_LEVEL = 64; // fade intensity is `level / TRANSITION_MAX_LEVEL`

struct TransitionStep
{
    bool white = false; // fade towards white instead of black
    int level = 0;      // 0 = scene fully visible, TRANSITION_MAX_LEVEL = fully faded
};

constexpr int BATTLE_FLASH_FRAMES = 10;
constexpr int BATTLE_FLASH_COUNT = 2;
constexpr int BATTLE_FADE_FRAMES = 10;

// Number of frames of the exit transition (one `transition_step` per frame, from 1 to this value).
[[nodiscard]] constexpr int transition_frames(TransitionStyle style)
{
    switch(style)
    {
    case TransitionStyle::BATTLE:
        return BATTLE_FLASH_FRAMES * BATTLE_FLASH_COUNT + BATTLE_FADE_FRAMES;
    case TransitionStyle::QUICK:
        return 5;
    case TransitionStyle::ROOM:
        return 18;
    default:
        return 12;
    }
}

// Step displayed on `frame` (1-based) of the exit transition. The last frame is always fully black.
[[nodiscard]] constexpr TransitionStep transition_step(TransitionStyle style, int frame)
{
    const int total = transition_frames(style);

    if(frame >= total)
    {
        return { false, TRANSITION_MAX_LEVEL };
    }

    if(frame < 0)
    {
        frame = 0;
    }

    if(style == TransitionStyle::BATTLE)
    {
        const int flash_end = BATTLE_FLASH_FRAMES * BATTLE_FLASH_COUNT;

        if(frame <= flash_end)
        {
            // Triangle wave 0 -> peak -> 0 over each flash, peak at 85% of the white.
            const int half = BATTLE_FLASH_FRAMES / 2;
            const int phase = (frame - 1) % BATTLE_FLASH_FRAMES;
            const int rise = phase < half ? phase + 1 : BATTLE_FLASH_FRAMES - phase;
            return { true, rise * TRANSITION_MAX_LEVEL * 85 / (half * 100) };
        }

        return { false, (frame - flash_end) * TRANSITION_MAX_LEVEL / BATTLE_FADE_FRAMES };
    }

    const int t = frame * TRANSITION_MAX_LEVEL / total;

    if(style == TransitionStyle::ROOM)
    {
        // Quadratic ease-in: starts gently, then the curtain drops.
        return { false, t * t / TRANSITION_MAX_LEVEL };
    }

    // Smoothstep ease-in-out.
    return { false, t * t * (3 * TRANSITION_MAX_LEVEL - 2 * t) / (TRANSITION_MAX_LEVEL * TRANSITION_MAX_LEVEL) };
}

}
