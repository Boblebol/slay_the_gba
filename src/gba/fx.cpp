#include "gba/fx.h"

#include "bn_bg_palettes.h"
#include "bn_random.h"
#include "bn_sprite_palettes.h"

namespace gba
{

namespace
{

bn::random shake_random;

}

bn::fixed wave(int frame, int period, bn::fixed amplitude)
{
    int phase = frame % period;
    bn::fixed degrees = bn::fixed(phase * 360) / period;
    return bn::degrees_lut_sin(degrees) * amplitude;
}

void Shake::start(int frames, int power)
{
    if(frames * power >= _frames * _power)
    {
        _frames = frames;
        _total = frames;
        _power = power;
    }
}

void Shake::update()
{
    if(_frames <= 0)
    {
        _offset = bn::fixed_point();
        return;
    }

    // Amplitude decays linearly so the shake settles instead of stopping dead.
    int amplitude = (_power * _frames + _total - 1) / _total;
    int range = amplitude * 2 + 1;
    _offset = bn::fixed_point(shake_random.get_int(range) - amplitude, shake_random.get_int(range) - amplitude);
    --_frames;
}

void set_screen_fade(bn::color color, bn::fixed intensity)
{
    bn::bg_palettes::set_fade(color, intensity);
    bn::sprite_palettes::set_fade(color, intensity);
}

void set_screen_fade(bn::fixed intensity)
{
    set_screen_fade(bn::color(0, 0, 0), intensity);
}

}
