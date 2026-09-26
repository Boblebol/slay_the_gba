#pragma once

#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_string_view.h"
#include "bn_vector.h"

namespace bn
{
class sprite_text_generator;
}

namespace gba
{

enum class TextColor : uint8_t
{
    WHITE,
    RED,
    GREEN,
    GOLD,
    BLUE,
    GRAY,
    PURPLE
};

enum class Align : uint8_t
{
    LEFT,
    CENTER,
    RIGHT
};

using TextLine = bn::string<64>;

// Must be called once after bn::core::init().
void init_text();

// Shared generator using the French-capable slay font.
[[nodiscard]] bn::sprite_text_generator& text_generator();

// Replaces characters missing from the font so a string can never trigger a Butano error.
void sanitize(const bn::string_view& text, bn::istring& out);

// Width in pixels of `text` once sanitised.
[[nodiscard]] int text_width(const bn::string_view& text);

// Greedy word wrap into lines no wider than `max_width` pixels. Returns the number of lines.
int wrap_text(const bn::string_view& text, int max_width, bn::ivector<TextLine>& lines);

/**
 * Sprite text label that only regenerates its sprites when its content or style change.
 * Moving it shifts the existing sprites, so animated labels (bobbing, floating numbers) stay cheap.
 */
class Label
{
public:
    void set(int x, int y, const bn::string_view& text, Align align = Align::CENTER,
             TextColor color = TextColor::WHITE);
    void set_text(const bn::string_view& text);
    void set_color(TextColor color);
    void move_to(int x, int y);
    void set_visible(bool visible);
    void clear();

    [[nodiscard]] int x() const
    {
        return _x;
    }

    [[nodiscard]] int y() const
    {
        return _y;
    }

    [[nodiscard]] bool empty() const
    {
        return _text.empty();
    }

private:
    TextLine _text;
    bn::vector<bn::sprite_ptr, 12> _sprites;
    int _x = 0;
    int _y = 0;
    Align _align = Align::CENTER;
    TextColor _color = TextColor::WHITE;
    bool _visible = true;

    void _regenerate();
};

}
