#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sstream.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bg_shop.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/card_picker.h"
#include "gba/card_text.h"
#include "gba/fx.h"
#include "gba/layers.h"
#include "gba/text.h"
#include "gba/widgets.h"

namespace gba
{

namespace
{

constexpr int CARD_Y = -36;
constexpr int PRICE_Y = 0;
constexpr int CARD_X[] = { -72, 0, 72 };
constexpr int REMOVE_ITEM = 3;
constexpr int LEAVE_ITEM = 4;

class ShopScene final : public Scene
{
public:
    explicit ShopScene(GameContext& context) :
        _ctx(context),
        _shop(context.run.current_shop),
        _bg(bn::regular_bg_items::bg_shop.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y)),
        _gold_icon(create_icon(Icon::GOLD, -106, -72)),
        _picker(CardPickerLayout{ 60, -74, -30, -30, -58, 142 })
    {
        for(int index = 0; index < int(_shop.cards.size()) && index < 3; ++index)
        {
            bn::sprite_ptr sprite = big_card_item(_shop.cards[index].card.id).create_sprite(CARD_X[index], CARD_Y);
            sprite.set_z_order(Z_CARD);
            sprite.set_visible(! _shop.cards[index].purchased);
            _cards.push_back(bn::move(sprite));
            _card_y[index] = CARD_Y;
        }

        _title.set(0, -72, "MARCHAND", Align::CENTER, TextColor::GOLD);
        _refresh_all();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_timer;
        _popups.update();

        if(_picking)
        {
            _picker.update(_ctx.frame);

            if(bn::keypad::b_pressed())
            {
                play_sfx(Sfx::UI_CANCEL);
                _close_picker();
            }
            else if(bn::keypad::a_pressed() && _picker.size() > 0)
            {
                if(_ctx.run.remove_card_at_shop(_picker.selected_tag()))
                {
                    _popups.spawn(0, 10, "Carte retirée", TextColor::GREEN);
                    play_sfx(Sfx::GOLD);
                }

                _close_picker();
            }

            return bn::nullopt;
        }

        int card_count = _cards.size();

        if(_cursor < REMOVE_ITEM)
        {
            if(bn::keypad::left_pressed())
            {
                play_sfx(Sfx::UI_CURSOR);
                _cursor = (_cursor + card_count - 1) % card_count;
                _refresh_all();
            }
            else if(bn::keypad::right_pressed())
            {
                play_sfx(Sfx::UI_CURSOR);
                _cursor = (_cursor + 1) % card_count;
                _refresh_all();
            }
            else if(bn::keypad::down_pressed())
            {
                play_sfx(Sfx::UI_CURSOR);
                _cursor = REMOVE_ITEM;
                _refresh_all();
            }
        }
        else if(bn::keypad::up_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _cursor = _cursor == LEAVE_ITEM ? REMOVE_ITEM : 1;
            _refresh_all();
        }
        else if(bn::keypad::down_pressed() && _cursor == REMOVE_ITEM)
        {
            play_sfx(Sfx::UI_CURSOR);
            _cursor = LEAVE_ITEM;
            _refresh_all();
        }

        if(bn::keypad::b_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _ctx.run.leave_shop();
            return SceneId::MAP;
        }

        if(bn::keypad::a_pressed())
        {
            if(bn::optional<SceneId> next = _confirm())
            {
                return next;
            }
        }

        _animate_cards();
        return bn::nullopt;
    }

private:
    GameContext& _ctx;
    engine::ShopInventory& _shop;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _gold_icon;
    CardPicker _picker;
    bn::vector<bn::sprite_ptr, 3> _cards;
    bn::fixed _card_y[3] = {};
    int _sold_timer[3] = {};
    Label _title;
    Label _gold;
    bn::vector<Label, 3> _prices;
    Label _info;
    Label _desc;
    Label _remove;
    Label _leave;
    PopupPool _popups;
    int _timer = 0;
    int _cursor = 0;
    bool _picking = false;

    bn::optional<SceneId> _confirm()
    {
        if(_cursor < REMOVE_ITEM)
        {
            engine::ShopCard& item = _shop.cards[_cursor];

            if(item.purchased)
            {
                return bn::nullopt;
            }

            if(_ctx.run.buy_shop_card(_cursor))
            {
                bn::string<16> text;
                bn::ostringstream stream(text);
                stream << '-' << item.cost << " Or";
                _popups.spawn(CARD_X[_cursor], PRICE_Y - 10, text, TextColor::GOLD);
                play_sfx(Sfx::GOLD);
                _sold_timer[_cursor] = 1;
            }
            else
            {
                _popups.spawn(CARD_X[_cursor], PRICE_Y - 10, "Pas assez d'or", TextColor::RED);
                play_sfx(Sfx::ERROR);
            }

            _refresh_all();
            return bn::nullopt;
        }

        if(_cursor == REMOVE_ITEM)
        {
            if(_shop.card_removal_used)
            {
                _popups.spawn(0, 20, "Déjà utilisé", TextColor::GRAY);
                play_sfx(Sfx::ERROR);
            }
            else if(_ctx.run.gold < _shop.card_removal_cost)
            {
                _popups.spawn(0, 20, "Pas assez d'or", TextColor::RED);
                play_sfx(Sfx::ERROR);
            }
            else
            {
                play_sfx(Sfx::UI_OPEN);
                _open_picker();
            }

            return bn::nullopt;
        }

        _ctx.run.leave_shop();
        return SceneId::MAP;
    }

    void _animate_cards()
    {
        for(int index = 0; index < _cards.size(); ++index)
        {
            bn::sprite_ptr& card = _cards[index];

            if(_sold_timer[index] > 0)
            {
                // Bought: the card lifts off and shrinks away.
                int age = _sold_timer[index]++;
                bn::fixed t = bn::min(bn::fixed(age) / 20, bn::fixed(1));
                card.set_scale(lerp(1, bn::fixed(0.2), t));
                card.set_y(CARD_Y - ease_out(t) * 40);

                if(age >= 20)
                {
                    card.set_visible(false);
                    _sold_timer[index] = 0;
                }

                continue;
            }

            bool focused = index == _cursor;
            _card_y[index] = approach(_card_y[index], CARD_Y - (focused ? 6 : 0), bn::fixed(0.3));
            card.set_y(_card_y[index] + (focused ? wave(_timer, 80, 1) : bn::fixed(0)));
            card.set_z_order(focused ? Z_CARD_FOCUS : Z_CARD);
        }
    }

    void _refresh_all()
    {
        bn::string<16> gold;
        bn::ostringstream gold_stream(gold);
        gold_stream << _ctx.run.gold;
        _gold.set(-96, -72, gold, Align::LEFT, TextColor::GOLD);

        _prices.clear();

        for(int index = 0; index < _cards.size(); ++index)
        {
            const engine::ShopCard& item = _shop.cards[index];
            bn::string<16> price;
            bn::ostringstream stream(price);

            if(item.purchased)
            {
                stream << "Vendu";
            }
            else
            {
                stream << item.cost << " Or";
            }

            TextColor color = item.purchased ? TextColor::GRAY
                    : (_ctx.run.gold >= item.cost ? TextColor::GOLD : TextColor::RED);
            _prices.emplace_back().set(CARD_X[index], PRICE_Y, price, Align::CENTER, color);
        }

        if(_cursor < REMOVE_ITEM && _cursor < _cards.size())
        {
            const engine::DeckCard& card = _shop.cards[_cursor].card;
            TextLine text;
            card_name(card, text);
            _info.set(-112, 28, text, Align::LEFT, card.upgraded ? TextColor::GREEN : TextColor::GOLD);
            card_description(card, text);
            _desc.set(-112, 39, text, Align::LEFT, TextColor::WHITE);
        }
        else if(_cursor == REMOVE_ITEM)
        {
            _info.set(-112, 28, "Service de purge", Align::LEFT, TextColor::GOLD);
            _desc.set(-112, 39, "Retire définitivement une carte du deck.", Align::LEFT, TextColor::WHITE);
        }
        else
        {
            _info.set(-112, 28, "Quitter", Align::LEFT, TextColor::GOLD);
            _desc.set(-112, 39, "Reprendre l'ascension.", Align::LEFT, TextColor::WHITE);
        }

        bn::string<48> remove;
        bn::ostringstream remove_stream(remove);
        remove_stream << (_cursor == REMOVE_ITEM ? "► " : "  ") << "Retirer une carte - " << _shop.card_removal_cost
                      << " Or";
        _remove.set(-112, 56, remove, Align::LEFT,
                    _shop.card_removal_used ? TextColor::GRAY
                                            : (_cursor == REMOVE_ITEM ? TextColor::GOLD : TextColor::WHITE));
        _leave.set(-112, 68, _cursor == LEAVE_ITEM ? "► Quitter la boutique" : "  Quitter la boutique", Align::LEFT,
                   _cursor == LEAVE_ITEM ? TextColor::GOLD : TextColor::WHITE);
    }

    void _set_menu_visible(bool visible)
    {
        for(int index = 0; index < _cards.size(); ++index)
        {
            _cards[index].set_visible(visible && ! _shop.cards[index].purchased);
        }

        for(Label& label : _prices)
        {
            label.set_visible(visible);
        }

        _info.set_visible(visible);
        _desc.set_visible(visible);
        _remove.set_visible(visible);
        _leave.set_visible(visible);
    }

    void _open_picker()
    {
        _picking = true;
        _set_menu_visible(false);
        _picker.clear();
        const auto& deck = _ctx.run.master_deck;

        for(int index = 0; index < int(deck.size()); ++index)
        {
            _picker.add(deck[index], index);
        }
    }

    void _close_picker()
    {
        _picking = false;
        _picker.clear();
        _set_menu_visible(true);
        _refresh_all();
    }
};

}

bn::unique_ptr<Scene> make_shop_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new ShopScene(context));
}

}
