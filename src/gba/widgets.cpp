#include "gba/widgets.h"

#include "bn_optional.h"
#include "bn_sprite_items_ui_hp_bar.h"
#include "bn_sprite_items_ui_icons.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "gba/fx.h"
#include "gba/layers.h"

namespace gba
{

namespace
{

constexpr int BAR_LEVELS = 30;

}

HpBar::HpBar(int x, int y) :
    _left(bn::sprite_items::ui_hp_bar.create_sprite(x - 16, y, 0)),
    _right(bn::sprite_items::ui_hp_bar.create_sprite(x + 16, y, 1)),
    _x(x),
    _y(y)
{
    _left.set_z_order(Z_HUD);
    _right.set_z_order(Z_HUD);
}

void HpBar::set(int hp, int max_hp)
{
    _target = bn::max(hp, 0);
    _max = bn::max(max_hp, 1);
}

void HpBar::snap()
{
    _shown = _target;
    _refresh();
}

void HpBar::update()
{
    if(_shown != _target)
    {
        // Ease the drain but never slower than a quarter HP per frame, so it always finishes.
        bn::fixed delta = bn::fixed(_target) - _shown;
        bn::fixed step = delta * bn::fixed(0.12);

        if(bn::abs(step) < bn::fixed(0.25))
        {
            step = delta > 0 ? bn::fixed(0.25) : bn::fixed(-0.25);
        }

        if(bn::abs(step) >= bn::abs(delta))
        {
            _shown = _target;
        }
        else
        {
            _shown += step;
        }
    }

    _refresh();
}

void HpBar::set_position(int x, int y)
{
    _x = x;
    _y = y;
    _left.set_position(x - _half_width, y);
    _right.set_position(x + _half_width, y);
    _label.move_to(x, y);
}

void HpBar::set_visible(bool visible)
{
    _visible = visible;
    _left.set_visible(visible);
    _right.set_visible(visible);
    _label.set_visible(visible);
}

void HpBar::set_horizontal_scale(bn::fixed scale)
{
    _left.set_horizontal_scale(scale);
    _right.set_horizontal_scale(scale);
    _half_width = (16 * scale).round_integer();
    set_position(_x, _y);
}

void HpBar::set_compact(bool compact)
{
    if(compact != _compact)
    {
        _compact = compact;
        _label_value = -1;
        _refresh();
    }
}

void HpBar::_refresh()
{
    int shown = _shown.round_integer();
    int level = (shown * BAR_LEVELS + _max - 1) / _max;
    level = bn::clamp(level, shown > 0 ? 1 : 0, BAR_LEVELS);

    int color = 0;

    if(shown * 2 <= _max)
    {
        color = shown * 5 <= _max ? 2 : 1;
    }

    int frame = color * (BAR_LEVELS + 1) + level;

    if(frame != _frame)
    {
        _frame = frame;
        _left.set_tiles(bn::sprite_items::ui_hp_bar.tiles_item(), frame * 2);
        _right.set_tiles(bn::sprite_items::ui_hp_bar.tiles_item(), frame * 2 + 1);
    }

    if(shown != _label_value)
    {
        _label_value = shown;
        bn::string<16> text;
        bn::ostringstream stream(text);
        stream << shown;

        if(! _compact)
        {
            stream << '/' << _max;
        }

        _label.set(_x, _y, text);
        _label.set_visible(_visible);
    }
}

void PopupPool::spawn(int x, int y, const bn::string_view& text, TextColor color, int delay)
{
    if(_popups.full())
    {
        _popups.erase(_popups.begin());
    }

    Popup& popup = _popups.emplace_back();
    popup.x = x;
    popup.y = y;
    popup.timer = 0;
    popup.delay = delay;
    popup.color = color;
    sanitize(text, popup.text);
}

void PopupPool::update()
{
    for(auto it = _popups.begin(); it != _popups.end();)
    {
        Popup& popup = *it;

        if(popup.delay > 0)
        {
            --popup.delay;
            ++it;
            continue;
        }

        ++popup.timer;

        // Quick pop upwards, then a slow drift.
        bn::fixed rise = popup.timer < 8 ? bn::fixed(1.5) : bn::fixed(0.35);
        popup.y -= rise;

        if(popup.timer == 1)
        {
            popup.label.set(popup.x.round_integer(), popup.y.round_integer(), popup.text, Align::CENTER,
                            popup.color);
        }
        else
        {
            popup.label.move_to(popup.x.round_integer(), popup.y.round_integer());
        }

        // Blink out at the end.
        if(popup.timer > 34)
        {
            popup.label.set_visible((popup.timer / 2) % 2 == 0);
        }

        if(popup.timer > 44)
        {
            it = _popups.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void PopupPool::clear()
{
    _popups.clear();
}

void FxPool::spawn(const bn::sprite_item& item, int x, int y, int first_frame, int frames, int ticks, bool flip)
{
    if(_effects.full())
    {
        _effects.erase(_effects.begin());
    }

    // An effect is decoration: when every sprite palette is taken (three monsters and all the text colours),
    // it is skipped instead of stopping the game on Butano's "no more available colors" error.
    bn::optional<bn::sprite_ptr> sprite = item.create_sprite_optional(x, y, first_frame);

    if(! sprite)
    {
        return;
    }

    sprite->set_z_order(Z_FX);
    sprite->set_horizontal_flip(flip);
    _effects.push_back(Fx{ bn::move(*sprite), &item, first_frame, frames, ticks, 0 });
}

void FxPool::update()
{
    for(auto it = _effects.begin(); it != _effects.end();)
    {
        Fx& fx = *it;
        ++fx.age;
        int frame = fx.age / fx.ticks;

        if(frame >= fx.frames)
        {
            it = _effects.erase(it);
            continue;
        }

        fx.sprite.set_tiles(fx.item->tiles_item(), fx.first_frame + frame);
        ++it;
    }
}

void FxPool::clear()
{
    _effects.clear();
}

bn::sprite_ptr create_icon(Icon icon, int x, int y)
{
    bn::sprite_ptr sprite = bn::sprite_items::ui_icons.create_sprite(x, y, static_cast<int>(icon));
    sprite.set_z_order(Z_HUD);
    return sprite;
}

void set_icon(bn::sprite_ptr& sprite, Icon icon)
{
    sprite.set_tiles(bn::sprite_items::ui_icons.tiles_item(), static_cast<int>(icon));
}

}
