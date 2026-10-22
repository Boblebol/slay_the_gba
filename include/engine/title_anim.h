#pragma once
// Pure timing of the animated title screen (#7): scrolling mist and clouds, pulsing glow of the Spire, blinking
// prompt. Kept free of any Butano type so that the engine tests cover it.
#include <cstdint>

namespace engine::title_anim {

constexpr int MIST_TILE_W = 64;       // width in pixels of one mist sprite (seamless tile)
constexpr int MIST_TILES = 5;         // sprites per band: covers 240 px plus one tile of wrap room
constexpr int GLOW_FRAMES = 4;        // intensity levels of the glow sprite sheet
constexpr int GLOW_PERIOD = 96;       // frames of a full pulse
constexpr int BLINK_PERIOD = 24;      // frames the prompt stays on, then off

// Scroll position (pixels, in [0, MIST_TILE_W)) of a band moving `speed_64th` 64ths of a pixel per frame.
constexpr int mist_scroll(int frame, int speed_64th) {
    int64_t total = static_cast<int64_t>(frame) * speed_64th;
    return static_cast<int>((total / 64) % MIST_TILE_W);
}

// Screen x (pixel offset from the left edge, may start left of the screen) of sprite `tile` of a band.
constexpr int mist_tile_x(int tile, int scroll) { return tile * MIST_TILE_W - scroll; }

// Glow sheet frame (0 dim .. GLOW_FRAMES-1 bright): rises then falls over GLOW_PERIOD frames.
constexpr int glow_frame(int frame) {
    int half = GLOW_PERIOD / 2;
    int phase = frame % GLOW_PERIOD;
    int rising = phase < half ? phase : GLOW_PERIOD - 1 - phase;
    return rising * GLOW_FRAMES / half;
}

// Blinking "press start" prompt.
constexpr bool prompt_visible(int frame) { return (frame / BLINK_PERIOD) % 2 == 0; }

}  // namespace engine::title_anim
