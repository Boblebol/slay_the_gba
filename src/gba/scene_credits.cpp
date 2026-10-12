#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"

#include "bn_regular_bg_items_bg_victory.h"

#include "engine/credits.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/text.h"

namespace gba
{

namespace
{

// Screen y of the top edge is 0 for the credits arithmetic; labels are centred on the screen.
constexpr int SCREEN_CENTER_Y = 80;

class CreditsScene final : public Scene
{
public:
    explicit CreditsScene(GameContext&) :
        _bg(bn::regular_bg_items::bg_victory.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y))
    {
        // Same track as the victory screen: no dedicated credits music asset yet.
        play_music(Music::VICTORY);
        refresh();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_frame;

        bool fast = bn::keypad::b_held() || bn::keypad::r_held();
        _scroll = engine::credits_advance(_scroll, fast);
        refresh();

        bool skip = bn::keypad::a_pressed() || bn::keypad::start_pressed();

        if(engine::credits_finished(_scroll) || (skip && engine::credits_can_skip(_frame)))
        {
            return SceneId::TITLE;
        }

        return bn::nullopt;
    }

private:
    void refresh()
    {
        for(int index = 0; index < engine::CREDIT_LINE_COUNT; ++index)
        {
            const engine::CreditLine& line = engine::CREDIT_LINES[index];

            if(line.style == engine::CreditStyle::GAP)
            {
                continue;
            }

            Label& label = _labels[index];

            if(! engine::credits_line_visible(index, _scroll))
            {
                label.clear();
                continue;
            }

            int y = engine::credits_line_y(index, _scroll) - SCREEN_CENTER_Y;

            if(label.empty())
            {
                label.set(0, y, line.text, Align::CENTER,
                          line.style == engine::CreditStyle::HEADING ? TextColor::GOLD : TextColor::WHITE);
            }
            else
            {
                label.move_to(0, y);
            }
        }
    }

    bn::regular_bg_ptr _bg;
    Label _labels[engine::CREDIT_LINE_COUNT];
    int _scroll = 0;
    int _frame = 0;
};

}

bn::unique_ptr<Scene> make_credits_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new CreditsScene(context));
}

}
