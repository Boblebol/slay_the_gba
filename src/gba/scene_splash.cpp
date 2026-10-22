#include "gba/scene_factories.h"

#include "bn_bg_palettes.h"
#include "bn_bgs_mosaic.h"
#include "bn_color.h"
#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bg_splash.h"
#include "bn_regular_bg_items_bg_title.h"
#include "common_variable_16x16_sprite_font.h"

#include "engine/boot_intro.h"

#include "gba/assets.h"
#include "gba/fx.h"
#include "gba/layers.h"
#include "gba/text.h"

namespace gba
{

namespace
{

namespace intro = engine::boot_intro;

constexpr int CREDIT_FIRST_Y = 6;   // first credit line, below the portrait of the bitmap
constexpr int CREDIT_HEADING_GAP = 11; // heading to its mention
constexpr int CREDIT_BLOCK_GAP = 15;   // mention to the next heading
constexpr int LOGO_Y = -62;         // same place as the logo of the title screen
constexpr int LOGO_LETTERS = 12;    // sprites of "SLAY THE GBA", as on the title screen
const bn::color SILHOUETTE_COLOR(5, 1, 2); // deep red-black the Spire stands out of

/**
 * Boot sequence (#1): a credits card (Mega Crit, Butano, the demake author), then the Spire rising out of the dark
 * as a blocky red silhouette that sharpens into the key art while the logo appears letter by letter. START or A
 * skips straight to the title screen, which the main loop reaches through its usual fade to black.
 */
class SplashScene final : public Scene
{
public:
    explicit SplashScene(GameContext&) :
        _bg(bn::regular_bg_items::bg_splash.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y))
    {
        int y = CREDIT_FIRST_Y;

        for(int index = 0; index < intro::BOOT_CREDIT_LINE_COUNT; ++index)
        {
            const intro::BootCreditLine& line = intro::BOOT_CREDIT_LINES[index];
            _credits.push_back(Label());
            _credits.back().set(0, y, line.text, Align::CENTER, line.heading ? TextColor::GOLD : TextColor::GRAY);
            _credits.back().set_visible(false);
            y += line.heading ? CREDIT_HEADING_GAP : CREDIT_BLOCK_GAP;
        }
    }

    ~SplashScene() final
    {
        // Palette effects and the mosaic are global: leave them clean for the title screen.
        bn::bg_palettes::set_grayscale_intensity(0);
        bn::bg_palettes::set_contrast(0);
        bn::bgs_mosaic::set_stretch(0);
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        bool skip = bn::keypad::start_pressed() || bn::keypad::a_pressed();

        if(intro::should_leave(_timer, skip))
        {
            return SceneId::TITLE;
        }

        if(intro::phase_at(_timer) == intro::Phase::CREDITS)
        {
            _update_credits();
        }
        else
        {
            _update_intro(intro::intro_frame(_timer));
        }

        ++_timer;
        return bn::nullopt;
    }

private:
    bn::optional<bn::regular_bg_ptr> _bg;
    bn::vector<Label, intro::BOOT_CREDIT_LINE_COUNT> _credits;
    bn::vector<bn::sprite_ptr, LOGO_LETTERS> _logo;
    int _timer = 0;

    void _update_credits()
    {
        int shown = intro::credit_lines_shown(_timer);

        for(int index = 0; index < _credits.size(); ++index)
        {
            _credits[index].set_visible(index < shown);
        }

        int fade = intro::credits_fade_256(_timer);

        // Before that the main loop is still fading the card in: leave the fade alone.
        if(fade > 0)
        {
            set_screen_fade(bn::fixed(fade) / 256);
        }
    }

    void _start_intro()
    {
        _credits.clear();
        _bg.reset();
        _bg = bn::regular_bg_items::bg_title.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y + intro::RISE_PIXELS);
        _bg->set_mosaic_enabled(true);

        bn::sprite_text_generator generator(common::variable_16x16_sprite_font);
        generator.set_center_alignment();
        generator.set_z_order(Z_TEXT);
        generator.generate(0, LOGO_Y, "SLAY THE GBA", _logo);

        for(bn::sprite_ptr& letter : _logo)
        {
            letter.set_visible(false);
        }

        set_screen_fade(0);
    }

    void _update_intro(int frame)
    {
        if(frame == 0)
        {
            _start_intro();
        }

        // The key art rises into place as a flat silhouette, then its colours and pixels come back.
        bn::fixed dark = bn::fixed(intro::silhouette_256(frame)) / 256;
        bn::bg_palettes::set_grayscale_intensity(dark);
        bn::bg_palettes::set_contrast(dark);
        bn::bg_palettes::set_fade(SILHOUETTE_COLOR, dark / 2);
        bn::bgs_mosaic::set_stretch(bn::fixed(intro::mosaic_level(frame)) / 16);
        _bg->set_position(BG_ORIGIN_X, BG_ORIGIN_Y + intro::rise_offset(frame));

        int letters = intro::logo_letters_shown(frame, _logo.size());

        for(int index = 0; index < _logo.size(); ++index)
        {
            _logo[index].set_visible(index < letters);
        }
    }
};

}

bn::unique_ptr<Scene> make_splash_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new SplashScene(context));
}

}
