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
#include "gba/loot_text.h"
#include "gba/text.h"
#include "gba/widgets.h"

namespace gba
{

namespace
{

constexpr int CARD_Y = -36;
constexpr int PRICE_Y = 0;
constexpr int CARD_X[] = { -72, 0, 72 };
constexpr int CARD_SLOTS = 3;
constexpr int MAX_CARDS = 7;
constexpr int SALE_Y = 10;
// The merchant greets the hero with a menu of his counters; A opens one, B comes back to the menu. Relics and
// potions are lists of names and prices where the cards stand, with the text of the one under the cursor in the
// panel below.
constexpr int PAGE_HOME = 0;
constexpr int PAGE_CARDS = 1;
constexpr int PAGE_RELICS = 2;
constexpr int PAGE_POTIONS = 3;
// Entries of the menu, in the order shown.
constexpr int HOME_CARDS = 0;
constexpr int HOME_REMOVE = 1;
constexpr int HOME_POTIONS = 2;
constexpr int HOME_RELICS = 3;
constexpr int HOME_LEAVE = 4;
constexpr int HOME_COUNT = 5;
constexpr int HOME_Y = -48;
constexpr int HOME_STEP = 14;
constexpr int ROW_Y = -50;
constexpr int ROW_STEP = 12;
constexpr int ROW_TEXT_Y = 28;
constexpr int ROW_TEXT_STEP = 10;
constexpr int ROW_TEXT_WIDTH = 224;
constexpr int HINT_Y = 68;

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
        for(int index = 0; index < int(_shop.cards.size()) && index < MAX_CARDS; ++index)
        {
            bn::sprite_ptr sprite = big_card_item(_shop.cards[index].card.id).create_sprite(CARD_X[0], CARD_Y);
            sprite.set_z_order(Z_CARD);
            sprite.set_visible(false);
            _cards.push_back(bn::move(sprite));
            _card_y[index] = CARD_Y;
        }

        _refresh_all();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_timer;
        _popups.update();

        // The relic just bought asks for cards of the deck: the shop comes back once they are chosen.
        if(_ctx.run.has_deck_choice())
        {
            return scene_after_deck_choice(_ctx, SceneId::SHOP);
        }

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
                    _popups.spawn(0, _home_y(HOME_REMOVE) - 10, "Carte retirée", TextColor::GREEN);
                    play_sfx(Sfx::GOLD);
                }

                _close_picker();
            }

            return bn::nullopt;
        }

        if(_replacing)
        {
            _update_replacing();
            return bn::nullopt;
        }

        if(_page == PAGE_HOME)
        {
            return _update_home();
        }

        if(_page == PAGE_CARDS)
        {
            int card_count = _cards.size();

            if(bn::keypad::left_pressed())
            {
                play_sfx(Sfx::UI_CURSOR);
                _card_cursor = (_card_cursor + card_count - 1) % card_count;
                _refresh_all();
            }
            else if(bn::keypad::right_pressed())
            {
                play_sfx(Sfx::UI_CURSOR);
                _card_cursor = (_card_cursor + 1) % card_count;
                _refresh_all();
            }
        }
        else if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
        {
            int count = _page_size(_page);
            play_sfx(Sfx::UI_CURSOR);
            _cursor = (_cursor + (bn::keypad::down_pressed() ? 1 : count - 1)) % count;
            _refresh_all();
        }

        if(bn::keypad::b_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _open_page(PAGE_HOME);
        }
        else if(bn::keypad::a_pressed())
        {
            if(_page == PAGE_CARDS)
            {
                _confirm_card();
            }
            else
            {
                _confirm_row();
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
    bn::vector<bn::sprite_ptr, MAX_CARDS> _cards;
    bn::fixed _card_y[MAX_CARDS] = {};
    int _sold_timer[MAX_CARDS] = {};
    int _card_cursor = 0;
    Label _title;
    Label _gold;
    bn::vector<Label, 2 * MAX_CARDS> _prices;
    Label _info;
    Label _desc;
    Label _hint;
    PopupPool _popups;
    int _timer = 0;
    // Row of the relic or potion list.
    int _cursor = 0;
    int _home_cursor = HOME_CARDS;
    bool _picking = false;
    bn::vector<Label, HOME_COUNT> _rows;
    bn::vector<Label, 4> _row_text;
    int _page = PAGE_HOME;
    // Full belt: the potion bought takes the place of one of the belt, chosen in the list.
    bool _replacing = false;
    int _belt_cursor = 0;

    [[nodiscard]] static int _home_y(int entry)
    {
        return HOME_Y + entry * HOME_STEP;
    }

    // First card shown: the three of the screen follow the cursor along the seven of the shelf.
    [[nodiscard]] int _first_card() const
    {
        return bn::clamp(_card_cursor - 1, 0, bn::max(0, int(_cards.size()) - CARD_SLOTS));
    }

    [[nodiscard]] bool _card_shown(int index) const
    {
        int slot = index - _first_card();
        return slot >= 0 && slot < CARD_SLOTS;
    }

    [[nodiscard]] int _card_x(int index) const
    {
        return CARD_X[bn::clamp(index - _first_card(), 0, CARD_SLOTS - 1)];
    }

    [[nodiscard]] int _card_x_or_zero(int index) const
    {
        return _card_shown(index) ? _card_x(index) : CARD_X[0];
    }

    [[nodiscard]] int _page_size(int page) const
    {
        return page == PAGE_CARDS ? _cards.size()
                                  : int(page == PAGE_RELICS ? _shop.relics.size() : _shop.potions.size());
    }

    // Items of the counter still for sale.
    [[nodiscard]] int _left_for_sale(int page) const
    {
        int count = 0;

        for(int index = 0; index < _page_size(page); ++index)
        {
            bool purchased = page == PAGE_CARDS    ? _shop.cards[index].purchased
                             : page == PAGE_RELICS ? _shop.relics[index].purchased
                                                   : _shop.potions[index].purchased;
            count += ! purchased;
        }

        return count;
    }

    void _open_page(int page)
    {
        // A card still flying away is gone for good.
        for(int index = 0; index < _cards.size(); ++index)
        {
            _sold_timer[index] = 0;
        }

        _page = page;
        _cursor = 0;
        _refresh_all();
    }

    bn::optional<SceneId> _update_home()
    {
        if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _home_cursor = (_home_cursor + (bn::keypad::down_pressed() ? 1 : HOME_COUNT - 1)) % HOME_COUNT;
            _refresh_all();
        }

        if(bn::keypad::b_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _ctx.run.leave_shop();
            return SceneId::MAP;
        }

        if(! bn::keypad::a_pressed())
        {
            return bn::nullopt;
        }

        int y = _home_y(_home_cursor) - 10;

        switch(_home_cursor)
        {
        case HOME_REMOVE:
            if(_shop.card_removal_used)
            {
                _popups.spawn(0, y, "Déjà utilisé", TextColor::GRAY);
                play_sfx(Sfx::ERROR);
            }
            else if(_ctx.run.gold < _shop.card_removal_cost)
            {
                _popups.spawn(0, y, "Pas assez d'or", TextColor::RED);
                play_sfx(Sfx::ERROR);
            }
            else
            {
                play_sfx(Sfx::UI_OPEN);
                _open_picker();
            }

            return bn::nullopt;

        case HOME_LEAVE:
            _ctx.run.leave_shop();
            return SceneId::MAP;

        default:
            break;
        }

        int page = _home_cursor == HOME_CARDS ? PAGE_CARDS : (_home_cursor == HOME_RELICS ? PAGE_RELICS : PAGE_POTIONS);

        if(_page_size(page) == 0)
        {
            _popups.spawn(0, y, "Rayon vide", TextColor::GRAY);
            play_sfx(Sfx::ERROR);
        }
        else
        {
            play_sfx(Sfx::UI_OPEN);
            _open_page(page);
        }

        return bn::nullopt;
    }

    void _update_replacing()
    {
        engine::RunManager& run = _ctx.run;
        int count = int(run.potions.size());

        if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _belt_cursor = (_belt_cursor + (bn::keypad::down_pressed() ? 1 : count - 1)) % count;
            _refresh_all();
        }
        else if(bn::keypad::b_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _replacing = false;
            _refresh_all();
        }
        else if(bn::keypad::a_pressed())
        {
            int cost = _shop.potions[_cursor].cost;

            if(run.buy_shop_potion(_cursor, _belt_cursor))
            {
                _spawn_price(0, ROW_Y + _cursor * ROW_STEP - 10, cost);
                play_sfx(Sfx::GOLD);
            }

            _replacing = false;
            _refresh_all();
        }
    }

    void _spawn_price(int x, int y, int cost)
    {
        bn::string<16> text;
        bn::ostringstream stream(text);
        stream << '-' << cost << " Or";
        _popups.spawn(x, y, text, TextColor::GOLD);
    }

    // Buys the card under the cursor.
    void _confirm_card()
    {
        engine::ShopCard& item = _shop.cards[_card_cursor];

        if(item.purchased)
        {
            return;
        }

        if(_ctx.run.buy_shop_card(_card_cursor))
        {
            _spawn_price(_card_x(_card_cursor), PRICE_Y - 10, item.cost);
            play_sfx(Sfx::GOLD);
            _sold_timer[_card_cursor] = 1;
        }
        else
        {
            _popups.spawn(_card_x(_card_cursor), PRICE_Y - 10, "Pas assez d'or", TextColor::RED);
            play_sfx(Sfx::ERROR);
        }

        _refresh_all();
    }

    // Buys the relic or the potion under the cursor.
    void _confirm_row()
    {
        engine::RunManager& run = _ctx.run;
        int y = ROW_Y + _cursor * ROW_STEP - 10;

        if(_page == PAGE_RELICS)
        {
            const engine::ShopRelic& item = _shop.relics[_cursor];

            if(item.purchased)
            {
                return;
            }

            if(run.buy_shop_relic(_cursor))
            {
                _spawn_price(0, y, item.cost);
                play_sfx(Sfx::RELIC);
            }
            else
            {
                _popups.spawn(0, y, run.gold < item.cost ? "Pas assez d'or" : "Plus de place", TextColor::RED);
                play_sfx(Sfx::ERROR);
            }
        }
        else
        {
            const engine::ShopPotion& item = _shop.potions[_cursor];

            if(item.purchased)
            {
                return;
            }

            if(run.gold < item.cost)
            {
                _popups.spawn(0, y, "Pas assez d'or", TextColor::RED);
                play_sfx(Sfx::ERROR);
            }
            else if(run.buy_shop_potion(_cursor))
            {
                _spawn_price(0, y, item.cost);
                play_sfx(Sfx::GOLD);
            }
            else
            {
                // Full belt: one potion has to go.
                play_sfx(Sfx::UI_OPEN);
                _replacing = true;
                _belt_cursor = 0;
            }
        }

        _refresh_all();
    }

    // The counters of the merchant, with what is left on each, and the text of the one under the cursor.
    void _refresh_home()
    {
        const engine::RunManager& run = _ctx.run;
        constexpr bn::string_view NAMES[] = { "Cartes", "Retirer une carte", "Potions", "Reliques",
                                              "Quitter la boutique" };

        for(int entry = 0; entry < HOME_COUNT; ++entry)
        {
            bool focused = entry == _home_cursor;
            bn::string<24> status;
            bn::ostringstream stream(status);
            TextColor status_color = TextColor::WHITE;
            bool closed = false;

            if(entry == HOME_REMOVE)
            {
                closed = _shop.card_removal_used;

                if(closed)
                {
                    stream << "Déjà utilisé";
                }
                else
                {
                    stream << _shop.card_removal_cost << " Or";
                    status_color = run.gold >= _shop.card_removal_cost ? TextColor::GOLD : TextColor::RED;
                }
            }
            else if(entry != HOME_LEAVE)
            {
                int page = entry == HOME_CARDS ? PAGE_CARDS : (entry == HOME_RELICS ? PAGE_RELICS : PAGE_POTIONS);
                int left = _left_for_sale(page);
                closed = left == 0;

                if(_page_size(page) == 0)
                {
                    stream << "Vide";
                }
                else if(closed)
                {
                    stream << "Tout vendu";
                }
                else
                {
                    stream << left << " en vente";
                }
            }

            TextLine row;
            bn::ostringstream row_stream(row);
            row_stream << (focused ? "► " : "  ") << NAMES[entry];
            _rows.emplace_back().set(-112, _home_y(entry), row, Align::LEFT,
                                     focused ? TextColor::GOLD : (closed ? TextColor::GRAY : TextColor::WHITE));

            if(! status.empty())
            {
                _prices.emplace_back().set(112, _home_y(entry), status, Align::RIGHT,
                                           closed ? TextColor::GRAY : status_color);
            }
        }

        constexpr bn::string_view TEXTS[] = {
            "Des cartes à ajouter au deck.",
            "Retire définitivement une carte du deck.",
            "Des potions pour la ceinture.",
            "Des reliques rares et puissantes.",
            "Reprendre l'ascension.",
        };
        _info.set(-112, 28, NAMES[_home_cursor], Align::LEFT, TextColor::GOLD);
        _desc.set(-112, 39, TEXTS[_home_cursor], Align::LEFT, TextColor::WHITE);
        _hint.set(-112, HINT_Y, "A Choisir   B Quitter", Align::LEFT, TextColor::GRAY);
    }

    // Names and prices of the shelf, or the belt when a potion has to make room, and the text of the row under
    // the cursor.
    void _refresh_rows()
    {
        const engine::RunManager& run = _ctx.run;
        bn::string_view description;

        if(_replacing)
        {
            // Three rows at most: the list follows the cursor along a longer belt.
            constexpr int SHOWN = 3;
            int count = int(run.potions.size());
            int first = bn::clamp(_belt_cursor - 1, 0, bn::max(0, count - SHOWN));

            for(int index = first; index < count && index < first + SHOWN; ++index)
            {
                bool focused = index == _belt_cursor;
                TextLine row;
                bn::ostringstream stream(row);
                stream << (focused ? "► " : "  ") << potion_name(run.potions[index]);
                _rows.emplace_back().set(-112, ROW_Y + (index - first) * ROW_STEP, row, Align::LEFT,
                                         focused ? TextColor::GOLD : TextColor::WHITE);
            }

            description = potion_description(run.potions[_belt_cursor]);
        }
        else
        {
            for(int index = 0; index < _page_size(_page); ++index)
            {
                bool relic = _page == PAGE_RELICS;
                std::string_view id = relic ? _shop.relics[index].relic_id : _shop.potions[index].potion_id;
                int cost = relic ? _shop.relics[index].cost : _shop.potions[index].cost;
                bool purchased = relic ? _shop.relics[index].purchased : _shop.potions[index].purchased;
                bool focused = index == _cursor;
                TextLine row;
                bn::ostringstream stream(row);
                stream << (focused ? "► " : "  ") << (relic ? relic_name(id) : potion_name(id));
                _rows.emplace_back().set(-112, ROW_Y + index * ROW_STEP, row, Align::LEFT,
                                         purchased ? TextColor::GRAY
                                                   : (focused ? TextColor::GOLD : TextColor::WHITE));
                bn::string<16> price;
                bn::ostringstream price_stream(price);

                if(purchased)
                {
                    price_stream << "Vendu";
                }
                else
                {
                    price_stream << cost << " Or";
                }

                _prices.emplace_back().set(112, ROW_Y + index * ROW_STEP, price, Align::RIGHT,
                                           purchased ? TextColor::GRAY
                                                     : (run.gold >= cost ? TextColor::GOLD : TextColor::RED));

                if(focused)
                {
                    description = relic ? relic_description(id) : potion_description(id);
                }
            }
        }

        bn::vector<TextLine, 4> lines;
        wrap_text(description, ROW_TEXT_WIDTH, lines);

        for(int index = 0; index < lines.size(); ++index)
        {
            _row_text.emplace_back().set(-112, ROW_TEXT_Y + index * ROW_TEXT_STEP, lines[index], Align::LEFT,
                                         TextColor::WHITE);
        }
    }

    void _animate_cards()
    {
        for(int index = 0; index < _cards.size() && _page == PAGE_CARDS; ++index)
        {
            bn::sprite_ptr& card = _cards[index];

            if(! _card_shown(index))
            {
                continue;
            }

            card.set_x(_card_x(index));

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

            bool focused = index == _card_cursor;
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
        bn::string<32> cards_title;
        bn::ostringstream cards_stream(cards_title);
        cards_stream << "CARTES " << (_card_cursor + 1) << '/' << int(_cards.size());
        _title.set(0, -72,
                   _replacing           ? "Jeter quelle potion ?"
                   : _page == PAGE_HOME ? "MARCHAND"
                   : _page == PAGE_CARDS
                       ? bn::string_view(cards_title)
                       : (_page == PAGE_RELICS ? "RELIQUES" : "POTIONS"),
                   Align::CENTER, TextColor::GOLD);

        _prices.clear();
        _rows.clear();
        _row_text.clear();
        _info.clear();
        _desc.clear();

        for(int index = 0; index < _cards.size(); ++index)
        {
            // The cards only stand on their own shelf; a card just bought finishes flying away.
            _cards[index].set_visible(_page == PAGE_CARDS && _card_shown(index) &&
                                      (! _shop.cards[index].purchased || _sold_timer[index] > 0));
            _cards[index].set_x(_card_x_or_zero(index));
        }

        if(_page == PAGE_HOME)
        {
            _refresh_home();
            return;
        }

        if(_page != PAGE_CARDS)
        {
            _refresh_rows();
            _hint.set(-112, HINT_Y, _replacing ? "A Remplacer   B Retour" : "A Acheter   B Retour", Align::LEFT,
                      TextColor::GRAY);
            return;
        }

        for(int index = 0; index < _cards.size(); ++index)
        {
            if(! _card_shown(index))
            {
                continue;
            }

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
                    : (_ctx.run.gold >= item.cost ? (item.on_sale ? TextColor::GREEN : TextColor::GOLD)
                                                  : TextColor::RED);
            _prices.emplace_back().set(_card_x(index), PRICE_Y, price, Align::CENTER, color);

            if(item.on_sale && ! item.purchased)
            {
                _prices.emplace_back().set(_card_x(index), PRICE_Y + SALE_Y, "PROMO -50%", Align::CENTER,
                                           TextColor::GREEN);
            }
        }

        const engine::DeckCard& card = _shop.cards[_card_cursor].card;
        TextLine text;
        card_name(card, text);
        _info.set(-112, 28, text, Align::LEFT, card.upgraded ? TextColor::GREEN : TextColor::GOLD);
        card_description(card, text);
        _desc.set(-112, 39, text, Align::LEFT, TextColor::WHITE);
        _hint.set(-112, HINT_Y, "◄ ► Choisir   A Acheter   B Retour", Align::LEFT, TextColor::GRAY);
    }

    void _set_menu_visible(bool visible)
    {
        for(Label& label : _rows)
        {
            label.set_visible(visible);
        }

        for(Label& label : _row_text)
        {
            label.set_visible(visible);
        }

        for(Label& label : _prices)
        {
            label.set_visible(visible);
        }

        _title.set_visible(visible);
        _info.set_visible(visible);
        _desc.set_visible(visible);
        _hint.set_visible(visible);
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
