#pragma once
// Boot sequence shown before the title screen (#1): a credits card that fades to black, then the title screen.
// START or A skips it. Kept free of any Butano type so that the engine tests cover the timing.
#include <cstdint>

namespace engine::boot_intro {

enum class Phase : uint8_t
{
    CREDITS, // credits card
    DONE     // leave for the title screen
};

struct BootCreditLine
{
    bool heading; // gold heading or plain gray mention
    const char* text;
};

inline constexpr BootCreditLine BOOT_CREDIT_LINES[] = {
    { true, "Hommage à Mega Crit" },
    { false, "Slay the Spire" },
    { true, "GBA engine by GValiente" },
    { false, "Butano" },
    { true, "Demake par Alexandre Enouf" },
    { false, "@Boblebol" },
};

inline constexpr int BOOT_CREDIT_LINE_COUNT =
    static_cast<int>(sizeof(BOOT_CREDIT_LINES) / sizeof(BOOT_CREDIT_LINES[0]));

inline constexpr int CREDITS_FRAMES = 180;      // credits card on screen
inline constexpr int CREDITS_LINE_DELAY = 12;   // frames between two credit lines showing up
inline constexpr int CREDITS_FADE_FRAMES = 16;  // fade to black at the end of the card
inline constexpr int SKIP_DELAY = 8;            // frames before a press can skip (ignores a key held at boot)

[[nodiscard]] constexpr Phase phase_at(int frame)
{
    return frame < CREDITS_FRAMES ? Phase::CREDITS : Phase::DONE;
}

// Number of credit lines visible on the card at `frame`.
[[nodiscard]] constexpr int credit_lines_shown(int frame)
{
    if(frame < 0)
    {
        return 0;
    }

    int shown = frame / CREDITS_LINE_DELAY + 1;
    return shown < BOOT_CREDIT_LINE_COUNT ? shown : BOOT_CREDIT_LINE_COUNT;
}

// Darkness of the credits card in 1/256 (0 = visible, 256 = black): it fades to black at its end.
[[nodiscard]] constexpr int credits_fade_256(int frame)
{
    int start = CREDITS_FRAMES - CREDITS_FADE_FRAMES;

    if(frame <= start)
    {
        return 0;
    }

    int t = frame - start;
    return t >= CREDITS_FADE_FRAMES ? 256 : t * 256 / CREDITS_FADE_FRAMES;
}

// Whether START or A (`skip_pressed`) or the end of the sequence leaves for the title screen at `frame`.
[[nodiscard]] constexpr bool should_leave(int frame, bool skip_pressed)
{
    return phase_at(frame) == Phase::DONE || (skip_pressed && frame >= SKIP_DELAY);
}

} // namespace engine::boot_intro
