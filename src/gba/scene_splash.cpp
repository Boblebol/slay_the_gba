#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bg_splash.h"

#include "engine/boot_intro.h"

#include "gba/assets.h"
#include "gba/fx.h"
#include "gba/text.h"

namespace gba
{

namespace
{

namespace intro = engine::boot_intro;

constexpr int CREDIT_FIRST_Y = 6;   // first credit line, below the portrait of the bitmap
constexpr int CREDIT_HEADING_GAP = 11; // heading to its mention
constexpr int CREDIT_BLOCK_GAP = 15;   // mention to the next heading

/**
 * Boot sequence (#1): a credits card (Mega Crit, Butano, the demake author) that fades to black into the title
 * screen. START or A skips straight to the title screen, which the main loop reaches through its usual fade.
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

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        bool skip = bn::keypad::start_pressed() || bn::keypad::a_pressed();

        if(intro::should_leave(_timer, skip))
        {
            return SceneId::TITLE;
        }

        _update_credits();

        ++_timer;
        return bn::nullopt;
    }

private:
    bn::regular_bg_ptr _bg;
    bn::vector<Label, intro::BOOT_CREDIT_LINE_COUNT> _credits;
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
};

}

bn::unique_ptr<Scene> make_splash_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new SplashScene(context));
}

}
