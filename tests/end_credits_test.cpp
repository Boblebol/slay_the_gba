// End credits (#22): the text, the scroll, the fast forward, the way out and the return to the title.
#include <cassert>
#include <cstring>
#include <iostream>
#include "engine/credits.h"

using namespace engine;

namespace {

bool mentions(const char* needle) {
    for (const CreditLine& line : CREDIT_LINES) {
        if (std::strstr(line.text, needle) != nullptr) return true;
    }
    return false;
}

void test_mentions() {
    assert(mentions("Alexandre Enouf"));
    assert(mentions("@Boblebol"));
    assert(mentions("Mega Crit"));
    assert(mentions("Butano"));
    assert(mentions("devkitPro"));
    std::cout << "  mentions: OK\n";
}

void test_scroll_is_smooth_and_upward() {
    int scroll = 0;
    int previous = credits_line_y(0, scroll);
    assert(previous == CREDITS_SCREEN_HEIGHT); // starts just below the screen
    assert(! credits_line_visible(0, scroll));
    while (! credits_finished(scroll)) {
        scroll = credits_advance(scroll, false);
        int y = credits_line_y(0, scroll);
        assert(y <= previous && previous - y <= 1); // never more than a pixel per frame
        previous = y;
    }
    assert(! credits_line_visible(CREDIT_LINE_COUNT - 1, scroll));
    std::cout << "  scroll: OK\n";
}

void test_every_line_crosses_the_screen() {
    for (int index = 0; index < CREDIT_LINE_COUNT; ++index) {
        bool seen = false;
        for (int scroll = 0; ! credits_finished(scroll); scroll = credits_advance(scroll, false)) {
            seen = seen || credits_line_visible(index, scroll);
        }
        assert(seen);
    }
    std::cout << "  lines shown: OK\n";
}

void test_fast_forward_and_ending() {
    int slow = 0, fast = 0, slow_frames = 0, fast_frames = 0;
    assert(! credits_finished(0));
    while (! credits_finished(slow)) { slow = credits_advance(slow, false); ++slow_frames; }
    while (! credits_finished(fast)) { fast = credits_advance(fast, true); ++fast_frames; }
    assert(fast_frames * 4 < slow_frames);
    std::cout << "  fast forward and ending: OK\n";
}

void test_skip_delay() {
    assert(! credits_can_skip(0));
    assert(! credits_can_skip(CREDITS_SKIP_DELAY - 1));
    assert(credits_can_skip(CREDITS_SKIP_DELAY));
    std::cout << "  skip delay: OK\n";
}

} // namespace

int main() {
    std::cout << "end credits test\n";
    test_mentions();
    test_scroll_is_smooth_and_upward();
    test_every_line_crosses_the_screen();
    test_fast_forward_and_ending();
    test_skip_delay();
    std::cout << "ALL OK\n";
    return 0;
}
