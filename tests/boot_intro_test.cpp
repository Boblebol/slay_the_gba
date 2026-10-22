// Boot credits and intro (#1): the credits text, the phases, the Spire reveal, the logo and the skip.
#include <cassert>
#include <cstring>
#include <iostream>
#include "engine/boot_intro.h"

using namespace engine::boot_intro;

namespace {

bool credited(const char* needle) {
    for (const BootCreditLine& line : BOOT_CREDIT_LINES) {
        if (std::strstr(line.text, needle) != nullptr) return true;
    }
    return false;
}

void test_credits_card_mentions() {
    assert(credited("Mega Crit"));
    assert(credited("Slay the Spire"));
    assert(credited("GValiente"));
    assert(credited("Butano"));
    assert(credited("Alexandre Enouf"));
    assert(credited("@Boblebol"));
    std::cout << "  credits mentions: OK\n";
}

void test_credits_then_intro_then_title() {
    assert(phase_at(0) == Phase::CREDITS);
    assert(phase_at(CREDITS_FRAMES - 1) == Phase::CREDITS);
    assert(phase_at(CREDITS_FRAMES) == Phase::INTRO);
    assert(phase_at(TOTAL_FRAMES - 1) == Phase::INTRO);
    assert(phase_at(TOTAL_FRAMES) == Phase::DONE);
    assert(! should_leave(TOTAL_FRAMES - 1, false));
    assert(should_leave(TOTAL_FRAMES, false));
    std::cout << "  phases: OK\n";
}

void test_credit_lines_appear_and_fade() {
    assert(credit_lines_shown(0) == 1);
    int previous = 0;
    for (int frame = 0; frame < CREDITS_FRAMES; ++frame) {
        int shown = credit_lines_shown(frame);
        assert(shown >= previous && shown <= BOOT_CREDIT_LINE_COUNT);
        previous = shown;
    }
    // Every line is readable for at least a second before the fade starts.
    assert(credit_lines_shown(CREDITS_FRAMES - CREDITS_FADE_FRAMES - 60) == BOOT_CREDIT_LINE_COUNT);
    assert(credits_fade_256(0) == 0);
    assert(credits_fade_256(CREDITS_FRAMES - CREDITS_FADE_FRAMES) == 0);
    assert(credits_fade_256(CREDITS_FRAMES) == 256); // fade to black into the intro
    std::cout << "  credit lines and fade: OK\n";
}

void test_spire_reveal() {
    // Starts as a low, blocky, dark silhouette...
    assert(silhouette_256(0) == 256);
    assert(mosaic_level(0) == MOSAIC_MAX);
    assert(rise_offset(0) == RISE_PIXELS);
    // ...rises and sharpens smoothly into the full colour key art.
    int previous_dark = 256, previous_rise = RISE_PIXELS, previous_mosaic = MOSAIC_MAX;
    for (int intro = 0; intro < INTRO_FRAMES; ++intro) {
        int dark = silhouette_256(intro), rise = rise_offset(intro), mosaic = mosaic_level(intro);
        assert(dark <= previous_dark && rise <= previous_rise && mosaic <= previous_mosaic);
        assert(previous_rise - rise <= 1); // never more than a pixel per frame
        previous_dark = dark; previous_rise = rise; previous_mosaic = mosaic;
    }
    assert(silhouette_256(LOGO_START) == 0);
    assert(mosaic_level(LOGO_START) == 0);
    assert(rise_offset(LOGO_START) == 0);
    std::cout << "  spire reveal: OK\n";
}

void test_logo_appears_before_the_title() {
    constexpr int letters = 12; // "SLAY THE GBA"
    assert(logo_letters_shown(LOGO_START - 1, letters) == 0);
    assert(logo_letters_shown(LOGO_START, letters) == 1);
    assert(logo_letters_shown(INTRO_FRAMES - 30, letters) == letters); // whole logo held half a second
    std::cout << "  logo: OK\n";
}

void test_start_or_a_skips() {
    assert(! should_leave(0, true)); // a key held at power on does not skip
    assert(should_leave(SKIP_DELAY, true)); // during the credits
    assert(should_leave(CREDITS_FRAMES + 10, true)); // during the intro
    assert(! should_leave(CREDITS_FRAMES + 10, false));
    std::cout << "  skip: OK\n";
}

} // namespace

int main() {
    std::cout << "Boot intro tests\n";
    test_credits_card_mentions();
    test_credits_then_intro_then_title();
    test_credit_lines_appear_and_fade();
    test_spire_reveal();
    test_logo_appears_before_the_title();
    test_start_or_a_skips();
    std::cout << "All boot intro tests passed\n";
    return 0;
}
