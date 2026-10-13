#pragma once

#include <cstddef>
#include <cstdint>

// End credits shown after a winning run (#22): the text, and the scrolling arithmetic the GBA scene follows.
namespace engine
{

enum class CreditStyle : uint8_t
{
    HEADING, // section title
    TEXT,    // name or mention
    GAP      // empty line
};

struct CreditLine
{
    CreditStyle style;
    const char* text;
};

inline constexpr CreditLine CREDIT_LINES[] = {
    { CreditStyle::HEADING, "SLAY THE GBA" },
    { CreditStyle::TEXT, "Un portage de Slay the Spire 2" },
    { CreditStyle::TEXT, "sur Game Boy Advance" },
    { CreditStyle::GAP, "" },
    { CreditStyle::HEADING, "CRÉATEUR" },
    { CreditStyle::TEXT, "Alexandre Enouf" },
    { CreditStyle::TEXT, "@Boblebol" },
    { CreditStyle::GAP, "" },
    { CreditStyle::HEADING, "HOMMAGE" },
    { CreditStyle::TEXT, "Mega Crit" },
    { CreditStyle::TEXT, "Slay the Spire 2" },
    { CreditStyle::GAP, "" },
    { CreditStyle::HEADING, "MOTEUR ET OUTILS" },
    { CreditStyle::TEXT, "Butano" },
    { CreditStyle::TEXT, "devkitPro / devkitARM" },
    { CreditStyle::GAP, "" },
    { CreditStyle::HEADING, "MERCI D'AVOIR JOUÉ" },
    { CreditStyle::GAP, "" },
};

inline constexpr int CREDIT_LINE_COUNT = static_cast<int>(sizeof(CREDIT_LINES) / sizeof(CREDIT_LINES[0]));

inline constexpr int CREDITS_SCREEN_HEIGHT = 160;
inline constexpr int CREDITS_LINE_HEIGHT = 14;
inline constexpr int CREDITS_SPEED_SLOW = 1;  // half pixels per frame
inline constexpr int CREDITS_SPEED_FAST = 4;  // pixels per frame while B or R is held
inline constexpr int CREDITS_SKIP_DELAY = 30; // frames before a press can leave the credits

// Total scroll distance (in half pixels) after which the last line has left the top of the screen.
[[nodiscard]] constexpr int credits_total_half_pixels()
{
    return (CREDITS_SCREEN_HEIGHT + CREDIT_LINE_COUNT * CREDITS_LINE_HEIGHT) * 2;
}

// Scroll position in half pixels after one more frame; `fast` accelerates the scroll.
[[nodiscard]] constexpr int credits_advance(int half_pixels, bool fast)
{
    return half_pixels + (fast ? CREDITS_SPEED_FAST * 2 : CREDITS_SPEED_SLOW);
}

// Whether the whole text has scrolled off the screen.
[[nodiscard]] constexpr bool credits_finished(int half_pixels)
{
    return half_pixels >= credits_total_half_pixels();
}

// Screen y (0 = top of the screen) of line `index` at the given scroll position.
[[nodiscard]] constexpr int credits_line_y(int index, int half_pixels)
{
    return CREDITS_SCREEN_HEIGHT + index * CREDITS_LINE_HEIGHT - half_pixels / 2;
}

// Whether line `index` is on screen at the given scroll position.
[[nodiscard]] constexpr bool credits_line_visible(int index, int half_pixels)
{
    int y = credits_line_y(index, half_pixels);
    return y > -CREDITS_LINE_HEIGHT && y < CREDITS_SCREEN_HEIGHT;
}

// A press can leave the credits only once the delay is over.
[[nodiscard]] constexpr bool credits_can_skip(int frame)
{
    return frame >= CREDITS_SKIP_DELAY;
}

} // namespace engine
