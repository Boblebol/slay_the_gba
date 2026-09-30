#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sstream.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bg_rewards.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/card_text.h"
#include "gba/fx.h"
#include "gba/layers.h"
#include "gba/text.h"
#include "gba/widgets.h"

namespace gba
{

namespace
{

constexpr int CARD_Y = 0;
constexpr int CARD_X[] = { -72, 0, 72 };

class RewardsScene final : public Scene
{
public:
    explicit RewardsScene(GameContext& context) :
        _ctx(context),
        _bg(bn::regular_bg_items::bg_rewards.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y)),
        _gold_icon(create_icon(Icon::GOLD, -30, -56))
    {
        const engine::RunManager& run = context.run;
        play_music(Music::MAP);

        for(int index = 0; index < int(run.current_card_rewards.size()); ++index)
        {
            bn::sprite_ptr sprite = big_card_item(run.current_card_rewards[index].id).create_sprite(CARD_X[index], -130);
            sprite.set_z_order(Z_CARD);
            _cards.push_back(bn::move(sprite));
        }

        _title.set(0, -72, "VICTOIRE !", Align::CENTER, TextColor::GOLD);
        bn::string<24> gold;
        bn::ostringstream stream(gold);
        stream << '+' << run.gold_reward << " Or";
        _gold.set(-20, -56, gold, Align::LEFT, TextColor::GOLD);
        _hint.set(116, -40, "START Passer", Align::RIGHT, TextColor::GRAY);
        _prompt.set(-116, -40, "Choisissez une carte", Align::LEFT, TextColor::WHITE);
        _refresh_text();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_timer;
        int count = _cards.size();

        if(_chosen < 0 && _timer > 20 && count > 0)
        {
            if(bn::keypad::left_pressed())
            {
                play_sfx(Sfx::UI_CURSOR);
                _selected = (_selected + count - 1) % count;
                _refresh_text();
            }
            else if(bn::keypad::right_pressed())
            {
                play_sfx(Sfx::UI_CURSOR);
                _selected = (_selected + 1) % count;
                _refresh_text();
            }
            else if(bn::keypad::a_pressed())
            {
                _chosen = _selected;
                _exit_timer = 0;
                _ctx.run.pick_card_reward(_chosen);
                play_sfx(Sfx::UI_CONFIRM);
            }
            else if(bn::keypad::start_pressed())
            {
                _ctx.run.skip_card_rewards();
                play_sfx(Sfx::GOLD);
                return SceneId::MAP;
            }
        }

        for(int index = 0; index < count; ++index)
        {
            bn::sprite_ptr& card = _cards[index];
            int local = _timer - index * 5;
            bn::fixed y = -130;

            if(local > 0)
            {
                bn::fixed t = bn::min(bn::fixed(local) / 18, bn::fixed(1));
                y = lerp(-130, CARD_Y, ease_out(t));
            }

            bool focused = index == _selected;

            if(local > 18)
            {
                y = CARD_Y + (focused ? -6 : 0) + wave(_timer + index * 20, 90, focused ? 2 : 1);
            }

            if(_chosen >= 0)
            {
                if(index == _chosen)
                {
                    // Chosen card grows, then flies into the deck (bottom of the screen).
                    bn::fixed t = bn::min(bn::fixed(_exit_timer) / 24, bn::fixed(1));
                    card.set_scale(t < bn::fixed(0.4) ? lerp(1, bn::fixed(1.3), t / bn::fixed(0.4))
                                                      : lerp(bn::fixed(1.3), bn::fixed(0.3), (t - bn::fixed(0.4)) / bn::fixed(0.6)));
                    y = t < bn::fixed(0.4) ? bn::fixed(CARD_Y - 6) : lerp(CARD_Y - 6, 90, ease_in_out((t - bn::fixed(0.4)) / bn::fixed(0.6)));
                }
                else
                {
                    y = card.y() - 4;
                }
            }

            card.set_y(y);
            card.set_z_order(focused ? Z_CARD_FOCUS : Z_CARD);
        }

        if(_chosen >= 0)
        {
            ++_exit_timer;

            if(_exit_timer == 1)
            {
                _popups.spawn(0, 20, "Ajoutée au deck !", TextColor::GREEN);
                play_sfx(Sfx::GOLD);
            }

            _popups.update();

            if(_exit_timer > 34)
            {
                return SceneId::MAP;
            }
        }

        return bn::nullopt;
    }

private:
    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _gold_icon;
    bn::vector<bn::sprite_ptr, 3> _cards;
    Label _title;
    Label _gold;
    Label _hint;
    Label _prompt;
    Label _name;
    bn::vector<Label, 2> _desc;
    PopupPool _popups;
    int _timer = 0;
    int _selected = 1;
    int _chosen = -1;
    int _exit_timer = 0;

    void _refresh_text()
    {
        const engine::RunManager& run = _ctx.run;

        if(run.current_card_rewards.empty())
        {
            return;
        }

        _selected = bn::clamp(_selected, 0, int(run.current_card_rewards.size()) - 1);
        const engine::DeckCard& card = run.current_card_rewards[_selected];
        TextLine text;
        card_name(card, text);
        TextLine header;
        bn::ostringstream stream(header);
        stream << text << "  (" << card_type_name(engine::CardDef::get(card.id).type) << ", "
               << int(engine::CardDef::get_cost(card.id, card.upgraded)) << " ●)";
        _name.set(0, 46, header, Align::CENTER, TextColor::GOLD);

        card_description(card, text);
        bn::vector<TextLine, 2> lines;
        wrap_text(text, 224, lines);
        _desc.clear();

        for(int index = 0; index < lines.size(); ++index)
        {
            _desc.emplace_back().set(0, 58 + index * 10, lines[index], Align::CENTER, TextColor::WHITE);
        }
    }
};

}

bn::unique_ptr<Scene> make_rewards_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new RewardsScene(context));
}

}
