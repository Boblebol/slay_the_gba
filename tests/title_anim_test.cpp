// Timing of the animated title screen (#7): mist scroll, Spire glow, blinking prompt.
#include <cassert>
#include <iostream>
#include "engine/title_anim.h"

using namespace engine::title_anim;

// Mist band: always inside one tile, advances with its speed, and the far band is slower (parallax).
void test_mist_scroll_wraps_and_parallax() {
    for (int frame = 0; frame < 5000; ++frame) {
        assert(mist_scroll(frame, 24) >= 0 && mist_scroll(frame, 24) < MIST_TILE_W);
        assert(mist_scroll(frame, 10) >= 0 && mist_scroll(frame, 10) < MIST_TILE_W);
    }
    assert(mist_scroll(0, 24) == 0);
    assert(mist_scroll(1, 64) == 1);
    assert(mist_scroll(64, 64) == 0);  // 1 px/frame: back to the start after one tile
    assert(mist_scroll(32, 64) == 32);
    assert(mist_scroll(100, 32) > mist_scroll(100, 8));
    // Seamless: the row of sprites always covers the 240 px screen.
    for (int scroll = 0; scroll < MIST_TILE_W; ++scroll) {
        assert(mist_tile_x(0, scroll) <= 0);
        assert(mist_tile_x(MIST_TILES - 1, scroll) + MIST_TILE_W >= 240);
    }
}

// Glow: stays within the sheet, reaches both the dimmest and the brightest frame, and loops.
void test_glow_pulses() {
    bool low = false, high = false;
    for (int frame = 0; frame < GLOW_PERIOD * 3; ++frame) {
        int g = glow_frame(frame);
        assert(g >= 0 && g < GLOW_FRAMES);
        low |= g == 0;
        high |= g == GLOW_FRAMES - 1;
        assert(g == glow_frame(frame + GLOW_PERIOD));
    }
    assert(low && high);
}

// Prompt: blinks on and off with a fixed period.
void test_prompt_blinks() {
    assert(prompt_visible(0));
    assert(prompt_visible(BLINK_PERIOD - 1));
    assert(!prompt_visible(BLINK_PERIOD));
    assert(prompt_visible(BLINK_PERIOD * 2));
}

int main() {
    test_mist_scroll_wraps_and_parallax();
    test_glow_pulses();
    test_prompt_blinks();
    std::cout << "title_anim tests passed\n";
    return 0;
}
