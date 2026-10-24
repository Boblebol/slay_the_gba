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
#include "gba/loot_text.h"
#include "gba/text.h"
#include "gba/widgets.h"

namespace gba
{

namespace
{

constexpr int CARD_Y = 0;
constexpr int CARD_X[] = { -72, 0, 72 };
// Four cards (Lasting Candy adds a Power to the reward): closer together, the focused one drawn on top.
constexpr int CARD_X_FOUR[] = { -87, -29, 29, 87 };
// Loot step, before the cards: one row per relic or potion, then what the row under the cursor does.
constexpr int LOOT_ROW_Y = -22;
constexpr int LOOT_ROW_STEP = 12;
constexpr int LOOT_KIND_Y = 24;
constexpr int LOOT_TEXT_Y = 36;
constexpr int LOOT_TEXT_STEP = 10;
constexpr int LOOT_TEXT_WIDTH = 224;

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

        _create_cards();

        _title.set(0, -72, "VICTOIRE !", Align::CENTER, TextColor::GOLD);
        bn::string<24> gold;
        bn::ostringstream stream(gold);
        stream << '+' << run.gold_reward << " Or";
        _gold.set(-20, -56, gold, Align::LEFT, TextColor::GOLD);

        // The relic and the potion of the fight are taken or left before the cards are dealt.
        if(! run.relic_reward.empty())
        {
            _loot.push_back(Loot::RELIC);
        }

        if(! run.potion_reward.empty())
        {
            _loot.push_back(Loot::POTION);
        }

        if(_loot.empty())
        {
            _open_cards();
        }
        else
        {
            _step = Step::LOOT;
            _refresh_loot();
        }
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        // The relic just taken asks for cards of the deck: the rewards come back once they are chosen.
        if(_ctx.run.has_deck_choice())
        {
            return scene_after_deck_choice(_ctx, SceneId::REWARDS);
        }

        if(_step != Step::CARDS)
        {
            // The cards wait above the screen until the loot is dealt with.
            _popups.update();
            _update_loot();
            return bn::nullopt;
        }

        if(_chosen < 0)
        {
            _popups.update(); // lets the popups of the loot step finish
        }

        ++_timer;
        int count = _cards.size();

        if(_sacrificed)
        {
            return _update_sacrifice();
        }

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
            else if(bn::keypad::select_pressed() && _ctx.run.reroll_card_rewards())
            {
                // Driftwood: the new cards fall from above the screen, as the first ones did.
                play_sfx(Sfx::UI_CONFIRM);
                _create_cards();
                _timer = 0;
                _selected = 0;
                _refresh_hint();
                _refresh_text();
                return bn::nullopt;
            }
            else if(bn::keypad::b_pressed() && _ctx.run.can_sacrifice_card_rewards())
            {
                // Paël's Wing: the cards go up in smoke; every second sacrifice gives a relic.
                engine::RunManager& run = _ctx.run;
                size_t owned = run.relics.size();
                bool relic = run.sacrifice_card_rewards();
                play_sfx(relic ? Sfx::BUFF : Sfx::UI_CONFIRM);

                if(relic && run.relics.size() > owned)
                {
                    TextLine text;
                    bn::ostringstream stream(text);
                    stream << "+ " << relic_name(run.relics[owned]);
                    _popups.spawn(0, 20, text, TextColor::GOLD);
                }
                else
                {
                    _popups.spawn(0, 20, "Sacrifié à Paël", TextColor::PURPLE);
                }

                _sacrificed = true;
                _exit_timer = 0;
                _hint.clear();
                _reroll_hint.clear();
                return bn::nullopt;
            }
            else if(bn::keypad::start_pressed())
            {
                _ctx.run.skip_card_rewards();
                play_sfx(Sfx::GOLD);
                return _leave();
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
                return _leave();
            }
        }

        return bn::nullopt;
    }

private:
    enum class Step : uint8_t
    {
        LOOT,     // relic and potion of the fight
        REPLACE,  // full belt: which potion makes room for the new one
        CARDS
    };

    enum class Loot : uint8_t
    {
        RELIC,
        POTION
    };

    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _gold_icon;
    bn::vector<bn::sprite_ptr, 4> _cards;
    Label _title;
    Label _gold;
    Label _hint;
    Label _reroll_hint;
    Label _prompt;
    Label _name;
    bn::vector<Label, 2> _desc;
    PopupPool _popups;
    int _timer = 0;
    int _selected = 1;
    int _chosen = -1;
    bool _sacrificed = false;
    int _exit_timer = 0;
    Step _step = Step::CARDS;
    bn::vector<Loot, 2> _loot;
    bn::vector<Label, 3> _rows;
    Label _loot_kind;
    bn::vector<Label, 4> _loot_text;
    int _loot_cursor = 0;
    int _belt_cursor = 0;

    // Leaving the screen after a boss opens the next act: its Ancient comes first.
    [[nodiscard]] SceneId _leave()
    {
        engine::RunManager& run = _ctx.run;

        if(run.phase == engine::RunPhase::ANCIENT_BLESSING)
        {
            return SceneId::ANCIENT;
        }

        // The Hunt: more cards are offered.
        if(run.phase == engine::RunPhase::REWARD_SCREEN)
        {
            return SceneId::REWARDS;
        }

        return SceneId::MAP;
    }

    void _open_cards()
    {
        _step = Step::CARDS;
        _timer = 0;
        _rows.clear();
        _loot_kind.clear();
        _loot_text.clear();
        _hint.set(116, -40, "START Passer", Align::RIGHT, TextColor::GRAY);
        // The Battleworn Dummy gives no card: only its potion, relic or upgrades.
        _prompt.set(-116, -40, _ctx.run.current_card_rewards.empty() ? "Rien d'autre à prendre" : "Choisissez une carte",
                    Align::LEFT, TextColor::WHITE);
        _refresh_hint();
        _refresh_text();
    }

    // One sprite per card of the reward, above the screen until they are dealt.
    void _create_cards()
    {
        const engine::RunManager& run = _ctx.run;
        _cards.clear();

        for(int index = 0; index < int(run.current_card_rewards.size()); ++index)
        {
            int x = run.current_card_rewards.size() > 3 ? CARD_X_FOUR[index] : CARD_X[index];
            bn::sprite_ptr sprite = big_card_item(run.current_card_rewards[index].id).create_sprite(x, -130);
            sprite.set_z_order(Z_CARD);
            _cards.push_back(bn::move(sprite));
        }
    }

    // Paël's Wing: the sacrificed cards rise and fade out, then the rewards go on.
    [[nodiscard]] bn::optional<SceneId> _update_sacrifice()
    {
        ++_exit_timer;
        _popups.update();

        for(bn::sprite_ptr& card : _cards)
        {
            card.set_y(card.y() - 3);
        }

        if(_exit_timer > 40)
        {
            return _leave();
        }

        return bn::nullopt;
    }

    // Driftwood: the free reroll of the reward, while it has not been used.
    void _refresh_hint()
    {
        TextLine text;
        bn::ostringstream stream(text);

        if(_ctx.run.can_sacrifice_card_rewards())
        {
            stream << "B Sacrifier";
        }

        if(_ctx.run.can_reroll_card_rewards())
        {
            stream << (text.empty() ? "" : "  ") << "SELECT Relancer";
        }

        if(text.empty())
        {
            _reroll_hint.clear();
        }
        else
        {
            _reroll_hint.set(116, -56, text, Align::RIGHT, TextColor::GRAY);
        }
    }

    void _update_loot()
    {
        engine::RunManager& run = _ctx.run;

        if(_step == Step::REPLACE)
        {
            int count = int(run.potions.size());

            if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
            {
                play_sfx(Sfx::UI_CURSOR);
                _belt_cursor = (_belt_cursor + (bn::keypad::down_pressed() ? 1 : count - 1)) % count;
                _refresh_loot();
            }
            else if(bn::keypad::b_pressed())
            {
                play_sfx(Sfx::UI_CANCEL);
                _step = Step::LOOT;
                _refresh_loot();
            }
            else if(bn::keypad::a_pressed() && run.take_potion_reward(_belt_cursor))
            {
                play_sfx(Sfx::UI_CONFIRM);
                _popups.spawn(0, 12, "Potion remplacée", TextColor::GREEN);
                _loot_taken();
            }

            return;
        }

        int count = _loot.size();

        if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
        {
            if(count > 1)
            {
                play_sfx(Sfx::UI_CURSOR);
                _loot_cursor = (_loot_cursor + 1) % count;
                _refresh_loot();
            }
        }
        else if(bn::keypad::a_pressed())
        {
            if(_loot[_loot_cursor] == Loot::RELIC)
            {
                if(run.take_relic_reward())
                {
                    play_sfx(Sfx::RELIC);
                    _popups.spawn(0, 12, "Relique obtenue !", TextColor::GREEN);
                    _loot_taken();
                }
                else
                {
                    play_sfx(Sfx::ERROR);
                }
            }
            else if(run.take_potion_reward())
            {
                play_sfx(Sfx::UI_CONFIRM);
                _popups.spawn(0, 12, "Potion rangée", TextColor::GREEN);
                _loot_taken();
            }
            else
            {
                // Full belt: one potion has to go.
                play_sfx(Sfx::UI_OPEN);
                _step = Step::REPLACE;
                _belt_cursor = 0;
                _refresh_loot();
            }
        }
        else if(bn::keypad::start_pressed())
        {
            // What is not taken is left behind.
            play_sfx(Sfx::UI_CONFIRM);
            _open_cards();
        }
    }

    void _loot_taken()
    {
        // Black Star: once a relic is taken, the next one takes its place.
        if(_loot[_loot_cursor] != Loot::RELIC || _ctx.run.relic_reward.empty())
        {
            _loot.erase(_loot.begin() + _loot_cursor);
        }

        _loot_cursor = 0;

        if(_loot.empty())
        {
            _open_cards();
        }
        else
        {
            _step = Step::LOOT;
            _refresh_loot();
        }
    }

    void _refresh_loot()
    {
        const engine::RunManager& run = _ctx.run;
        bn::string_view kind;
        bn::string_view description;
        _rows.clear();

        if(_step == Step::REPLACE)
        {
            _prompt.set(-116, -40, "Jeter quelle potion ?", Align::LEFT, TextColor::WHITE);
            _hint.set(116, -40, "B Retour", Align::RIGHT, TextColor::GRAY);

            // Three rows at most: the list follows the cursor along a longer belt.
            int count = int(run.potions.size());
            int first = bn::clamp(_belt_cursor - 1, 0, bn::max(0, count - _rows.max_size()));

            for(int index = first; index < count && ! _rows.full(); ++index)
            {
                bool focused = index == _belt_cursor;
                TextLine row;
                bn::ostringstream stream(row);
                stream << (focused ? "► " : "  ") << potion_name(run.potions[index]);
                _rows.emplace_back().set(-112, LOOT_ROW_Y + (index - first) * LOOT_ROW_STEP, row, Align::LEFT,
                                         focused ? TextColor::GOLD : TextColor::WHITE);
            }

            kind = "Dans la ceinture";
            description = potion_description(run.potions[_belt_cursor]);
        }
        else
        {
            _prompt.set(-116, -40, "A Prendre", Align::LEFT, TextColor::WHITE);
            _hint.set(116, -40, "START Continuer", Align::RIGHT, TextColor::GRAY);

            for(int index = 0; index < _loot.size(); ++index)
            {
                bool focused = index == _loot_cursor;
                bool relic = _loot[index] == Loot::RELIC;
                TextLine row;
                bn::ostringstream stream(row);
                stream << (focused ? "► " : "  ") << (relic ? "Relique : " : "Potion : ")
                       << (relic ? relic_name(run.relic_reward) : potion_name(run.potion_reward));
                _rows.emplace_back().set(-112, LOOT_ROW_Y + index * LOOT_ROW_STEP, row, Align::LEFT,
                                         focused ? TextColor::GOLD : TextColor::WHITE);
            }

            if(_loot[_loot_cursor] == Loot::RELIC)
            {
                kind = relic_kind(run.relic_reward);
                description = relic_description(run.relic_reward);
            }
            else
            {
                kind = potion_kind(run.potion_reward);
                description = potion_description(run.potion_reward);
            }
        }

        _loot_kind.set(0, LOOT_KIND_Y, kind, Align::CENTER, TextColor::GOLD);
        bn::vector<TextLine, 4> lines;
        wrap_text(description, LOOT_TEXT_WIDTH, lines);
        _loot_text.clear();

        for(int index = 0; index < lines.size(); ++index)
        {
            _loot_text.emplace_back().set(0, LOOT_TEXT_Y + index * LOOT_TEXT_STEP, lines[index], Align::CENTER,
                                          TextColor::WHITE);
        }
    }

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
               << card_cost_label(card) << " ●";

        if(bn::string_view stars = card_star_cost_label(card); ! stars.empty())
        {
            stream << " " << stars << " ★";
        }

        stream << ")";
        _name.set(0, 46, header, Align::CENTER, is_enchanted(card) ? TextColor::PURPLE : TextColor::GOLD);

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
