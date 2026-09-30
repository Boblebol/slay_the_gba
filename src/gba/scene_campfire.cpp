#include "gba/scene_factories.h"

#include "bn_bg_palettes.h"
#include "bn_keypad.h"
#include "bn_random.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sstream.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bg_campfire.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/card_picker.h"
#include "gba/fx.h"
#include "gba/layers.h"
#include "gba/text.h"
#include "gba/widgets.h"

namespace gba
{

namespace
{

constexpr int OPTION_Y = 34;

class CampfireScene final : public Scene
{
public:
    explicit CampfireScene(GameContext& context) :
        _ctx(context),
        _bg(bn::regular_bg_items::bg_campfire.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y)),
        _heart(create_icon(Icon::HEART, -104, -58)),
        _picker(CardPickerLayout{ 60, -74, -30, -30, -58, 142 })
    {
        _title.set(0, -72, "FEU DE CAMP", Align::CENTER, TextColor::GOLD);
        _refresh_hp();
        _refresh_options();
    }

    ~CampfireScene() final
    {
        bn::bg_palettes::set_fade(bn::color(), 0);
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_timer;
        _popups.update();
        _flicker();

        switch(_stage)
        {
        case Stage::MENU:
            if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
            {
                play_sfx(Sfx::UI_CURSOR);
                _cursor = 1 - _cursor;
                _refresh_options();
            }
            else if(bn::keypad::a_pressed())
            {
                if(_cursor == 0)
                {
                    int before = _ctx.run.hp;
                    _ctx.run.rest_at_campfire();
                    bn::string<16> text;
                    bn::ostringstream stream(text);
                    stream << '+' << (_ctx.run.hp - before) << " PV";
                    _popups.spawn(0, -20, text, TextColor::GREEN);
                    play_sfx(Sfx::HEAL);
                    _refresh_hp();
                    _stage = Stage::DONE;
                    _done_timer = 0;
                    _clear_options();
                }
                else if(_ctx.run.can_smith())
                {
                    play_sfx(Sfx::UI_OPEN);
                    _open_forge();
                }
                else
                {
                    _popups.spawn(0, 10, "Tout est déjà amélioré", TextColor::GRAY);
                    play_sfx(Sfx::ERROR);
                }
            }
            break;

        case Stage::FORGE:
            _picker.update(_ctx.frame);

            if(bn::keypad::b_pressed())
            {
                play_sfx(Sfx::UI_CANCEL);
                _picker.clear();
                _stage = Stage::MENU;
                _refresh_options();
            }
            else if(bn::keypad::a_pressed() && _picker.size() > 0)
            {
                _ctx.run.smith_at_campfire(_picker.selected_tag());
                _popups.spawn(-74, -60, "Amélioré !", TextColor::GREEN);
                play_sfx(Sfx::BUFF);
                _stage = Stage::DONE;
                _done_timer = 0;
                _hint.clear();
            }
            break;

        case Stage::DONE:
            ++_done_timer;

            if(_done_timer == 1 && _picker.preview())
            {
                _picker.preview()->set_scale(bn::fixed(1.2));
            }

            if(_done_timer > 50)
            {
                _ctx.run.phase = engine::RunPhase::MAP_NAVIGATION;
                return SceneId::MAP;
            }
            break;
        }

        return bn::nullopt;
    }

private:
    enum class Stage : uint8_t
    {
        MENU,
        FORGE,
        DONE
    };

    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _heart;
    CardPicker _picker;
    bn::random _random;
    Label _title;
    Label _hp;
    Label _hint;
    bn::vector<Label, 2> _options;
    PopupPool _popups;
    Stage _stage = Stage::MENU;
    int _timer = 0;
    int _cursor = 0;
    int _done_timer = 0;
    bn::fixed _glow;

    void _flicker()
    {
        // Warm light that breathes with the fire; skipped while the scene fades in.
        if(_timer < 16)
        {
            return;
        }

        bn::fixed target = bn::fixed(_random.get_int(12)) / 100;
        _glow = approach(_glow, target, bn::fixed(0.3));
        bn::bg_palettes::set_fade(bn::color(31, 18, 6), _glow);
    }

    void _refresh_hp()
    {
        bn::string<16> text;
        bn::ostringstream stream(text);
        stream << _ctx.run.hp << '/' << _ctx.run.max_hp;
        _hp.set(-94, -58, text, Align::LEFT, TextColor::WHITE);
    }

    void _clear_options()
    {
        _options.clear();
        _hint.clear();
    }

    void _refresh_options()
    {
        _options.clear();
        int heal = _ctx.run.max_hp * 3 / 10;
        bn::string<48> rest;
        bn::ostringstream stream(rest);
        stream << (_cursor == 0 ? "► " : "  ") << "Se reposer : +" << heal << " PV";
        _options.emplace_back().set(-100, OPTION_Y, rest, Align::LEFT, _cursor == 0 ? TextColor::GOLD : TextColor::WHITE);

        TextLine forge;
        forge.append(_cursor == 1 ? "► " : "  ");
        forge.append(hero_campfire_upgrade());
        forge.append(" : améliorer une carte");
        _options.emplace_back().set(-100, OPTION_Y + 16, forge, Align::LEFT,
                                    _cursor == 1 ? TextColor::GOLD : TextColor::WHITE);
        _hint.set(0, 72, "▲▼ Choisir   A Valider", Align::CENTER, TextColor::GRAY);
    }

    void _open_forge()
    {
        _stage = Stage::FORGE;
        _options.clear();
        _picker.clear();
        _picker.set_preview_upgraded(true);
        const auto& deck = _ctx.run.master_deck;

        for(int index = 0; index < int(deck.size()); ++index)
        {
            if(! deck[index].upgraded)
            {
                _picker.add(deck[index], index);
            }
        }

        _hint.set(0, 30, "◄► Carte   A Forger   B Retour", Align::CENTER, TextColor::GRAY);
    }
};

}

bn::unique_ptr<Scene> make_campfire_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new CampfireScene(context));
}

}
