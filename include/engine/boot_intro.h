#pragma once
// Boot sequence shown before the title screen (#1): a credits card, then a short intro where the Spire rises out of
// the dark as a pixelated silhouette, sharpens into colour and the logo appears letter by letter. START or A skips
// the whole sequence. Kept free of any Butano type so that the engine tests cover the timing.
#include <cstdint>

namespace engine::boot_intro {

enum class Phase : uint8_t
{
    CREDITS, // credits card
    INTRO,   // Spire silhouette, reveal and logo
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
inline constexpr int INTRO_FRAMES = 210;        // Spire intro on screen
inline constexpr int SILHOUETTE_FRAMES = 70;    // pure silhouette rising before the colours come in
inline constexpr int REVEAL_FRAMES = 60;        // silhouette to full colour, pixels sharpening
inline constexpr int RISE_PIXELS = 32;          // the key art starts that much lower and rises into place
inline constexpr int LOGO_START = SILHOUETTE_FRAMES + REVEAL_FRAMES; // first logo letter, intro frame
inline constexpr int LOGO_LETTER_DELAY = 4;     // frames between two logo letters
inline constexpr int SKIP_DELAY = 8;            // frames before a press can skip (ignores a key held at boot)
inline constexpr int MOSAIC_MAX = 6;            // coarsest mosaic step of the silhouette (0 = sharp, GBA max 15)
inline constexpr int TOTAL_FRAMES = CREDITS_FRAMES + INTRO_FRAMES;

[[nodiscard]] constexpr Phase phase_at(int frame)
{
    if(frame < CREDITS_FRAMES)
    {
        return Phase::CREDITS;
    }

    return frame < TOTAL_FRAMES ? Phase::INTRO : Phase::DONE;
}

// Frame counted from the start of the intro (negative during the credits).
[[nodiscard]] constexpr int intro_frame(int frame)
{
    return frame - CREDITS_FRAMES;
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

// How much of the key art is still a silhouette, in 1/256 (256 = flat dark shape, 0 = full colour).
[[nodiscard]] constexpr int silhouette_256(int intro)
{
    if(intro <= SILHOUETTE_FRAMES)
    {
        return 256;
    }

    int t = intro - SILHOUETTE_FRAMES;
    return t >= REVEAL_FRAMES ? 0 : 256 - t * 256 / REVEAL_FRAMES;
}

// Mosaic step of the key art: blocky while it is a silhouette, sharp once revealed.
[[nodiscard]] constexpr int mosaic_level(int intro)
{
    return (silhouette_256(intro) * MOSAIC_MAX + 255) / 256;
}

// Vertical offset (pixels, positive = lower) of the key art: it rises into place during the silhouette, easing out.
[[nodiscard]] constexpr int rise_offset(int intro)
{
    int span = SILHOUETTE_FRAMES + REVEAL_FRAMES / 2;

    if(intro <= 0)
    {
        return RISE_PIXELS;
    }

    if(intro >= span)
    {
        return 0;
    }

    int left = span - intro; // quadratic ease out
    return RISE_PIXELS * left * left / (span * span);
}

// Number of logo letters shown at intro frame `intro`, out of `letters`.
[[nodiscard]] constexpr int logo_letters_shown(int intro, int letters)
{
    if(intro < LOGO_START)
    {
        return 0;
    }

    int shown = (intro - LOGO_START) / LOGO_LETTER_DELAY + 1;
    return shown < letters ? shown : letters;
}

// Whether START or A (`skip_pressed`) or the end of the sequence leaves for the title screen at `frame`.
[[nodiscard]] constexpr bool should_leave(int frame, bool skip_pressed)
{
    return phase_at(frame) == Phase::DONE || (skip_pressed && frame >= SKIP_DELAY);
}

} // namespace engine::boot_intro
