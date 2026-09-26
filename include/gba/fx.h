#pragma once

#include "bn_color.h"
#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_math.h"

namespace gba
{

// Moves `current` a fraction of the remaining distance towards `target`, snapping when close.
[[nodiscard]] constexpr bn::fixed approach(bn::fixed current, bn::fixed target, bn::fixed ratio = bn::fixed(0.25))
{
    bn::fixed delta = target - current;

    if(bn::abs(delta) < bn::fixed(0.5))
    {
        return target;
    }

    return current + delta * ratio;
}

// Integer counter (HP digits, gold) ticking towards `target` by at most `max_step` per frame.
[[nodiscard]] constexpr int step_towards(int current, int target, int max_step)
{
    if(current < target)
    {
        return bn::min(target, current + max_step);
    }

    return bn::max(target, current - max_step);
}

// Cubic ease-out on t in [0, 1].
[[nodiscard]] constexpr bn::fixed ease_out(bn::fixed t)
{
    bn::fixed inv = 1 - t;
    return 1 - inv * inv * inv;
}

// Smoothstep ease-in-out on t in [0, 1].
[[nodiscard]] constexpr bn::fixed ease_in_out(bn::fixed t)
{
    return t * t * (3 - 2 * t);
}

// Linear interpolation between two values with t in [0, 1].
[[nodiscard]] constexpr bn::fixed lerp(bn::fixed from, bn::fixed to, bn::fixed t)
{
    return from + (to - from) * t;
}

// Sine wave of the given period (frames) and amplitude, sampled at `frame`.
[[nodiscard]] bn::fixed wave(int frame, int period, bn::fixed amplitude);

// Parabolic hop: 0 at t=0 and t=1, `height` at t=0.5.
[[nodiscard]] constexpr bn::fixed hop(bn::fixed t, bn::fixed height)
{
    return 4 * height * t * (1 - t);
}

// Decaying random screen shake.
class Shake
{
public:
    void start(int frames, int power);
    void update();

    [[nodiscard]] bn::fixed_point offset() const
    {
        return _offset;
    }

private:
    int _frames = 0;
    int _total = 1;
    int _power = 0;
    bn::fixed_point _offset;
};

// Scene transition helpers: fade both BG and sprite palettes towards a colour.
void set_screen_fade(bn::color color, bn::fixed intensity);
void set_screen_fade(bn::fixed intensity);

}
