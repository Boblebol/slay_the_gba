#pragma once

#include "bn_string.h"
#include "bn_string_view.h"
#include "bn_vector.h"

#include "engine/dialogue.h"
#include "gba/text.h"

namespace gba
{

// Where a dialogue box writes its text and how it is dressed.
struct DialogueStyle
{
    int x = -112;
    int top = 16;
    int line_height = 11;
    int width = 224;    // pixels
    int lines = 5;      // lines per page: a longer text continues on the next page
    Align align = Align::LEFT;
    TextColor color = TextColor::WHITE;
};

constexpr int DIALOGUE_PAGE_LINES = 6;

// Text panel of the event, Ancient and shop scenes (#19): wraps the text to the screen, types it letter by letter
// with a voice blip, and cuts a text too long for the panel into pages.
class DialogueBox
{
public:
    void start(const bn::string_view& text, const DialogueStyle& style);
    void clear();

    // Types the next letter (once per frame); plays the voice blip.
    void update();

    // The page on screen is fully typed.
    [[nodiscard]] bool done() const
    {
        return _reveal.done();
    }

    // A page follows the one on screen.
    [[nodiscard]] bool has_next_page() const
    {
        return _first + _reveal.line_count() < _total;
    }

    // Shows the whole page at once.
    void finish();

    // Starts typing the next page.
    void next_page();

    [[nodiscard]] bool empty() const
    {
        return _total == 0;
    }

private:
    bn::string<320> _text;
    engine::DialogueSpan _spans[engine::DIALOGUE_MAX_LINES];
    engine::DialogueReveal _reveal;
    bn::vector<Label, DIALOGUE_PAGE_LINES> _labels;
    DialogueStyle _style;
    int _total = 0; // wrapped lines of the whole text
    int _first = 0; // first line of the page on screen

    void _open_page();
    void _show_line(int index);
    [[nodiscard]] int _line_x(int index) const;
};

}
