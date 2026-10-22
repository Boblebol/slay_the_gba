#include "gba/dialogue_box.h"

#include "gba/audio.h"

namespace gba
{

namespace
{

int measure(std::string_view text)
{
    return text_width(bn::string_view(text.data(), int(text.size())));
}

}

void DialogueBox::start(const bn::string_view& text, const DialogueStyle& style)
{
    _style = style;

    if(_style.lines > DIALOGUE_PAGE_LINES)
    {
        _style.lines = DIALOGUE_PAGE_LINES;
    }

    _text.clear();

    if(text.size() < _text.max_size())
    {
        _text.append(text);
    }
    else
    {
        // Cut on a character boundary, never inside an accent.
        int size = _text.max_size() - 1;

        while(size > 0 && (static_cast<unsigned char>(text[size]) & 0xC0) == 0x80)
        {
            --size;
        }

        _text.append(bn::string_view(text.data(), size));
    }

    _total = engine::wrap_dialogue(std::string_view(_text.data(), _text.size()), style.width, measure, _spans,
                                   engine::DIALOGUE_MAX_LINES);
    _first = 0;
    _open_page();
}

void DialogueBox::clear()
{
    _labels.clear();
    _total = 0;
    _first = 0;
    _reveal.start(std::string_view(), _spans, 0);
}

void DialogueBox::update()
{
    if(_reveal.done())
    {
        return;
    }

    int line = _reveal.current_line();

    if(_reveal.update())
    {
        play_sfx(Sfx::TEXT_BLIP);
    }

    // The letter that just appeared is on `line`, or was the last one of it.
    _show_line(line);
}

void DialogueBox::finish()
{
    _reveal.skip();

    for(int index = 0; index < _reveal.line_count(); ++index)
    {
        _show_line(index);
    }
}

void DialogueBox::next_page()
{
    if(has_next_page())
    {
        _first += _reveal.line_count();
        _open_page();
    }
}

void DialogueBox::_open_page()
{
    int count = _total - _first;

    if(count > _style.lines)
    {
        count = _style.lines;
    }

    _labels.clear();

    for(int index = 0; index < count; ++index)
    {
        _labels.emplace_back();
    }

    _reveal.start(std::string_view(_text.data(), _text.size()), _spans + _first, count);
}

void DialogueBox::_show_line(int index)
{
    std::string_view shown = _reveal.visible_line(index);
    _labels[index].set(_line_x(index), _style.top + index * _style.line_height,
                       bn::string_view(shown.data(), int(shown.size())), Align::LEFT, _style.color);
}

int DialogueBox::_line_x(int index) const
{
    if(_style.align != Align::CENTER)
    {
        return _style.x;
    }

    std::string_view full = _reveal.line(index);
    return _style.x - measure(full) / 2;
}

}
