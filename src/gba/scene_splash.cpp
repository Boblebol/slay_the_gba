#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"

#include "bn_regular_bg_items_bg_splash.h"

#include "gba/assets.h"
#include "gba/text.h"

namespace gba
{

namespace
{

constexpr int SPLASH_FRAMES = 150;
constexpr int TEXT_DELAY_FRAMES = 20;

class SplashScene final : public Scene
{
public:
    explicit SplashScene(GameContext&) :
        _bg(bn::regular_bg_items::bg_splash.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y))
    {
        _credit.set(0, 38, "Portage par Boblebol", Align::CENTER, TextColor::GOLD);
        _site.set(0, 52, "alexandre-enouf.fr", Align::CENTER, TextColor::GRAY);
        _credit.set_visible(false);
        _site.set_visible(false);
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_timer;

        if(_timer > TEXT_DELAY_FRAMES)
        {
            _credit.set_visible(true);
            _site.set_visible(true);
        }

        bool skip = bn::keypad::start_pressed() || bn::keypad::a_pressed();

        if(_timer >= SPLASH_FRAMES || (_timer > TEXT_DELAY_FRAMES && skip))
        {
            return SceneId::TITLE;
        }

        return bn::nullopt;
    }

private:
    bn::regular_bg_ptr _bg;
    Label _credit;
    Label _site;
    int _timer = 0;
};

}

bn::unique_ptr<Scene> make_splash_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new SplashScene(context));
}

}
