// Boot credits (#1): the credits text, the phases and the skip. The credits card leads straight to the title.
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

void test_credits_then_title() {
    assert(phase_at(0) == Phase::CREDITS);
    assert(phase_at(CREDITS_FRAMES - 1) == Phase::CREDITS);
    assert(phase_at(CREDITS_FRAMES) == Phase::DONE); // no Spire/logo splash in between
    assert(! should_leave(CREDITS_FRAMES - 1, false));
    assert(should_leave(CREDITS_FRAMES, false));
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
    assert(credits_fade_256(CREDITS_FRAMES) == 256); // fade to black into the title
    std::cout << "  credit lines and fade: OK\n";
}

void test_start_or_a_skips() {
    assert(! should_leave(0, true)); // a key held at power on does not skip
    assert(should_leave(SKIP_DELAY, true)); // during the credits
    assert(! should_leave(SKIP_DELAY, false));
    std::cout << "  skip: OK\n";
}

} // namespace

int main() {
    std::cout << "Boot intro tests\n";
    test_credits_card_mentions();
    test_credits_then_title();
    test_credit_lines_appear_and_fade();
    test_start_or_a_skips();
    std::cout << "All boot intro tests passed\n";
    return 0;
}
