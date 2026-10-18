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
constexpr int OPTION_STEP = 16;
// With the actions relics add, the frame shows three closer rows that follow the cursor.
constexpr int LONG_OPTION_Y = 30;
constexpr int LONG_OPTION_STEP = 12;
constexpr int LONG_OPTION_ROWS = 3;

class CampfireScene final : public Scene
{
public:
    explicit CampfireScene(GameContext& context) :
        _ctx(context),
        _bg(bn::regular_bg_items::bg_campfire.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y)),
        _heart(create_icon(Icon::HEART, -104, -58)),
        _picker(CardPickerLayout{ 60, -74, -30, -30, -58, 142 }),
        _actions(context.run.campfire_actions())
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
                int count = int(_actions.size());
                _cursor = (_cursor + (bn::keypad::down_pressed() ? 1 : count - 1)) % count;
                _refresh_options();
            }
            else if(bn::keypad::a_pressed())
            {
                engine::CampfireAction action = _actions[_cursor];

                if(action == engine::CampfireAction::REST)
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
                else if(action == engine::CampfireAction::COOK)
                {
                    // Meat Cleaver: the run asks for 2 cards to remove, then the map is back (see DeckChoiceScene).
                    play_sfx(Sfx::UI_OPEN);
                    _ctx.run.cook_at_campfire();
                    return scene_after_deck_choice(_ctx, _ctx.run.phase == engine::RunPhase::CAMPFIRE ? SceneId::CAMPFIRE
                                                                                                   : SceneId::MAP);
                }
                else if(action == engine::CampfireAction::LIFT)
                {
                    // Girya: +1 Strength for the rest of the run, then the rest site is used up.
                    if(_ctx.run.lift_at_campfire())
                    {
                        _popups.spawn(0, -20, "+1 Force", TextColor::GREEN);
                        play_sfx(Sfx::BUFF);
                        _stage = Stage::DONE;
                        _done_timer = 0;
                        _clear_options();
                    }
                    else
                    {
                        _popups.spawn(0, 10, "Maximum atteint", TextColor::GRAY);
                        play_sfx(Sfx::ERROR);
                    }
                }
                else if(action == engine::CampfireAction::DIG)
                {
                    if(_ctx.run.dig_at_campfire())
                    {
                        _popups.spawn(0, -20, "Relique trouvée !", TextColor::GREEN);
                        play_sfx(Sfx::BUFF);
                        _stage = Stage::DONE;
                        _done_timer = 0;
                        _clear_options();
                    }
                    else
                    {
                        _popups.spawn(0, 10, "Rien à fouiller", TextColor::GRAY);
                        play_sfx(Sfx::ERROR);
                    }
                }
                else if(action == engine::CampfireAction::LEAVE)
                {
                    // Miniature Tent: the player is done with the rest site.
                    play_sfx(Sfx::UI_CONFIRM);
                    _ctx.run.leave_campfire();
                    return SceneId::MAP;
                }
                else if(action == engine::CampfireAction::CLONE)
                {
                    // Clone: a card enchanted with it is duplicated, then the rest site is used up.
                    play_sfx(Sfx::UI_OPEN);
                    _open_clone();
                }
                else if(action == engine::CampfireAction::HATCH)
                {
                    // Byrdonis Egg: the egg leaves the deck for the Byrdpip, then the rest site is used up.
                    if(_ctx.run.hatch_at_campfire())
                    {
                        _popups.spawn(0, -20, "Oizillon obtenu !", TextColor::GREEN);
                        play_sfx(Sfx::BUFF);
                        _stage = Stage::DONE;
                        _done_timer = 0;
                        _clear_options();
                    }
                }
                else if(action != engine::CampfireAction::SMITH)
                {
                    // An action the engine does not resolve yet.
                    _popups.spawn(0, 10, "Indisponible", TextColor::GRAY);
                    play_sfx(Sfx::ERROR);
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

        case Stage::CLONE:
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
                if(_ctx.run.clone_at_campfire(_picker.selected_tag()))
                {
                    _popups.spawn(-74, -60, "Clonée !", TextColor::PURPLE);
                    play_sfx(Sfx::BUFF);
                    _stage = Stage::DONE;
                    _done_timer = 0;
                    _hint.clear();
                }
                else
                {
                    _popups.spawn(0, 10, "Deck plein", TextColor::GRAY);
                    play_sfx(Sfx::ERROR);
                }
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
                // Dream Catcher: the Rest opened the card reward screen, which comes next.
                if(_ctx.run.phase == engine::RunPhase::REWARD_SCREEN)
                {
                    return SceneId::REWARDS;
                }

                // Miniature Tent: the actions not done yet are offered again.
                if(_ctx.run.phase == engine::RunPhase::CAMPFIRE)
                {
                    _actions = _ctx.run.campfire_actions();
                    _cursor = 0;
                    _picker.clear();
                    _stage = Stage::MENU;
                    _refresh_hp();
                    _refresh_options();
                    break;
                }

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
        CLONE,
        DONE
    };

    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _heart;
    CardPicker _picker;
    engine::FixedVector<engine::CampfireAction, engine::MAX_CAMPFIRE_ACTIONS> _actions;
    bn::random _random;
    Label _title;
    Label _hp;
    Label _hint;
    bn::vector<Label, LONG_OPTION_ROWS> _options;
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
        int count = int(_actions.size());
        bool is_long = count > 2;
        int step = is_long ? LONG_OPTION_STEP : OPTION_STEP;
        int top = is_long ? LONG_OPTION_Y : OPTION_Y;
        int first = is_long ? bn::clamp(_cursor - 1, 0, count - LONG_OPTION_ROWS) : 0;

        for(int index = first; index < count && ! _options.full(); ++index)
        {
            TextLine line;
            bn::ostringstream stream(line);
            stream << (_cursor == index ? "► " : "  ");

            switch(_actions[index])
            {
            case engine::CampfireAction::REST:
                stream << "Se reposer : +" << (_ctx.run.max_hp * 3 / 10) << " PV";
                break;
            case engine::CampfireAction::SMITH:
                stream << hero_campfire_upgrade() << " : améliorer une carte";
                break;
            case engine::CampfireAction::DIG:
                stream << "Fouiller : obtenir une relique";
                break;
            case engine::CampfireAction::LIFT:
                stream << "Soulever : gagner de la Force";
                break;
            case engine::CampfireAction::CLONE:
                stream << "Cloner : dupliquer une carte";
                break;
            case engine::CampfireAction::LEAVE:
                stream << "Partir";
                break;
            case engine::CampfireAction::HATCH:
                stream << "Faire éclore : l'Œuf d'oizodon";
                break;
            default:
                stream << "Cuisiner : -2 cartes, +5 PV Max";
                break;
            }

            _options.emplace_back().set(-100, top + (index - first) * step, line, Align::LEFT,
                                        _cursor == index ? TextColor::GOLD : TextColor::WHITE);
        }

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

    void _open_clone()
    {
        _stage = Stage::CLONE;
        _options.clear();
        _picker.clear();
        _picker.set_preview_upgraded(false);
        const auto& deck = _ctx.run.master_deck;

        for(int index = 0; index < int(deck.size()); ++index)
        {
            if(deck[index].enchantment == engine::Enchantment::CLONE)
            {
                _picker.add(deck[index], index);
            }
        }

        _hint.set(0, 30, "◄► Carte   A Cloner   B Retour", Align::CENTER, TextColor::GRAY);
    }
};

}

bn::unique_ptr<Scene> make_campfire_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new CampfireScene(context));
}

}
