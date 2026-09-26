#include "gba/text.h"

#include "bn_optional.h"
#include "bn_sprite_text_generator.h"

#include "gba/layers.h"
#include "gba/slay_font.h"

namespace gba
{

namespace
{

bn::optional<bn::sprite_text_generator> generator;

const bn::sprite_palette_item& palette_for(TextColor color)
{
    switch(color)
    {
    case TextColor::RED:
        return slay_font_red;
    case TextColor::GREEN:
        return slay_font_green;
    case TextColor::GOLD:
        return slay_font_gold;
    case TextColor::BLUE:
        return slay_font_blue;
    case TextColor::GRAY:
        return slay_font_gray;
    case TextColor::PURPLE:
        return slay_font_purple;
    default:
        return slay_font_white;
    }
}

bool is_supported(const bn::utf8_character& character)
{
    for(const bn::utf8_character& supported : slay_font_utf8_characters)
    {
        if(supported.data() == character.data())
        {
            return true;
        }
    }

    return false;
}

int utf8_length(unsigned char lead)
{
    if((lead & 0xE0) == 0xC0)
    {
        return 2;
    }

    if((lead & 0xF0) == 0xE0)
    {
        return 3;
    }

    if((lead & 0xF8) == 0xF0)
    {
        return 4;
    }

    return 1;
}

}

void init_text()
{
    generator.emplace(slay_font);
    generator->set_z_order(Z_TEXT);
}

bn::sprite_text_generator& text_generator()
{
    return *generator;
}

void sanitize(const bn::string_view& text, bn::istring& out)
{
    out.clear();

    const char* data = text.data();
    int size = text.size();
    int index = 0;

    while(index < size && out.size() + 4 < out.max_size())
    {
        auto lead = static_cast<unsigned char>(data[index]);

        if(lead < 0x80)
        {
            out.push_back(lead >= 32 && lead < 127 ? char(lead) : ' ');
            ++index;
            continue;
        }

        int length = utf8_length(lead);

        if(length == 1 || index + length > size)
        {
            out.push_back('?');
            ++index;
            continue;
        }

        bn::string_view sequence(data + index, length);
        bn::utf8_character character(data + index);

        if(is_supported(character))
        {
            out.append(sequence);
        }
        else if(sequence == "\xC2\xA0" || sequence == "\xE2\x80\xAF")
        {
            out.push_back(' '); // non-breaking spaces
        }
        else if(sequence == "\xE2\x80\x9C" || sequence == "\xE2\x80\x9D")
        {
            out.push_back('"'); // curly double quotes
        }
        else
        {
            out.push_back('?');
        }

        index += length;
    }
}

int text_width(const bn::string_view& text)
{
    TextLine clean;
    sanitize(text, clean);
    return generator->width(clean);
}

int wrap_text(const bn::string_view& text, int max_width, bn::ivector<TextLine>& lines)
{
    lines.clear();

    TextLine current;
    int start = 0;
    int size = text.size();

    while(start < size && ! lines.full())
    {
        int end = start;

        while(end < size && text[end] != ' ')
        {
            ++end;
        }

        bn::string_view word = text.substr(start, end - start);
        TextLine candidate = current;

        if(! candidate.empty())
        {
            candidate.push_back(' ');
        }

        if(candidate.size() + word.size() < candidate.max_size())
        {
            candidate.append(word);
        }

        if(! current.empty() && text_width(candidate) > max_width)
        {
            lines.push_back(current);
            current.clear();

            if(word.size() < current.max_size())
            {
                current.append(word);
            }
        }
        else
        {
            current = candidate;
        }

        start = end + 1;
    }

    if(! current.empty() && ! lines.full())
    {
        lines.push_back(current);
    }

    return lines.size();
}

void Label::set(int x, int y, const bn::string_view& text, Align align, TextColor color)
{
    TextLine clean;
    sanitize(text, clean);

    if(clean == _text && align == _align && color == _color)
    {
        move_to(x, y);
        return;
    }

    _text = clean;
    _x = x;
    _y = y;
    _align = align;
    _color = color;
    _regenerate();
}

void Label::set_text(const bn::string_view& text)
{
    set(_x, _y, text, _align, _color);
}

void Label::set_color(TextColor color)
{
    if(color != _color)
    {
        _color = color;
        _regenerate();
    }
}

void Label::move_to(int x, int y)
{
    int dx = x - _x;
    int dy = y - _y;

    if(dx || dy)
    {
        for(bn::sprite_ptr& sprite : _sprites)
        {
            sprite.set_position(sprite.x() + dx, sprite.y() + dy);
        }

        _x = x;
        _y = y;
    }
}

void Label::set_visible(bool visible)
{
    if(visible != _visible)
    {
        _visible = visible;

        for(bn::sprite_ptr& sprite : _sprites)
        {
            sprite.set_visible(visible);
        }
    }
}

void Label::clear()
{
    _text.clear();
    _sprites.clear();
}

void Label::_regenerate()
{
    _sprites.clear();

    if(_text.empty())
    {
        return;
    }

    bn::sprite_text_generator& gen = *generator;
    gen.set_palette_item(palette_for(_color));

    switch(_align)
    {
    case Align::LEFT:
        gen.set_left_alignment();
        break;
    case Align::RIGHT:
        gen.set_right_alignment();
        break;
    default:
        gen.set_center_alignment();
        break;
    }

    gen.generate(_x, _y, _text, _sprites);

    if(! _visible)
    {
        for(bn::sprite_ptr& sprite : _sprites)
        {
            sprite.set_visible(false);
        }
    }
}

}
