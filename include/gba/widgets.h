#pragma once

#include "bn_fixed.h"
#include "bn_optional.h"
#include "bn_sprite_item.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "gba/text.h"

namespace gba
{

// Pokemon-style HP bar: green / yellow / red fill that drains smoothly, with "hp/max" printed on it.
class HpBar
{
public:
    HpBar(int x, int y);

    void set(int hp, int max_hp);   // animate towards these values
    void snap();                    // jump straight to the current target
    void update();
    void set_position(int x, int y);
    void set_visible(bool visible);
    void set_horizontal_scale(bn::fixed scale);
    // Writes only the HP left, without "/max", where a full label would not fit (a crowded row of enemies).
    void set_compact(bool compact);

    [[nodiscard]] bool animating() const
    {
        return _shown != _target;
    }

private:
    bn::sprite_ptr _left;
    bn::sprite_ptr _right;
    Label _label;
    bn::fixed _shown;
    int _target = 0;
    int _max = 1;
    int _x;
    int _y;
    int _label_value = -1;
    int _frame = 0;
    int _half_width = 16;
    bool _visible = true;
    bool _compact = false;

    void _refresh();
};

// Floating combat numbers ("-12", "Bloqué", "+5 Armure") that rise and vanish.
class PopupPool
{
public:
    void spawn(int x, int y, const bn::string_view& text, TextColor color, int delay = 0);
    void update();
    void clear();

private:
    struct Popup
    {
        Label label;
        TextLine text;
        bn::fixed x;
        bn::fixed y;
        int timer = 0;
        int delay = 0;
        TextColor color = TextColor::WHITE;
    };

    bn::vector<Popup, 4> _popups;
};

// One-shot animated effects (slashes, sparks, block rings).
class FxPool
{
public:
    // Plays `frames` frames of `item` starting at `first_frame`, each shown `ticks` frames.
    void spawn(const bn::sprite_item& item, int x, int y, int first_frame, int frames, int ticks = 3,
               bool flip = false);
    void update();
    void clear();

private:
    struct Fx
    {
        bn::sprite_ptr sprite;
        const bn::sprite_item* item;
        int first_frame;
        int frames;
        int ticks;
        int age;
    };

    bn::vector<Fx, 5> _effects;
};

// Frame indexes inside ui_icons.bmp.
enum class Icon : uint8_t
{
    ATTACK,
    DEFEND,
    BUFF,
    DEBUFF,
    UNKNOWN,
    TARGET,
    HEART,
    GOLD,
    STUN,
    HEAL,
    ESCAPE,
    STATUS
};

[[nodiscard]] bn::sprite_ptr create_icon(Icon icon, int x, int y);
void set_icon(bn::sprite_ptr& sprite, Icon icon);

// Frame indexes inside fx_burst.bmp.
constexpr int FX_HIT = 0;
constexpr int FX_BLOCK = 4;
constexpr int FX_BUFF = 8;

}
