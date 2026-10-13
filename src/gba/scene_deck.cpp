#include "gba/scene_factories.h"

#include "bn_algorithm.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sstream.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bg_map.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/card_picker.h"
#include "gba/layers.h"
#include "gba/loot_text.h"
#include "gba/text.h"

namespace gba
{

namespace
{

class DeckScene final : public Scene
{
public:
    explicit DeckScene(GameContext& context) :
        _ctx(context),
        _bg(bn::regular_bg_items::bg_map.create_bg(BG_ORIGIN_X, 176 + (context.frame % 512))),
        _picker(CardPickerLayout{ 50, -74, -22, -30, -50, 142 })
    {
        // Opened on "Reliques" from the pause menu.
        _show(context.deck_relics && ! context.run.relics.empty() ? View::RELICS : View::DECK);
        context.deck_relics = false;
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        if(_view == View::RELICS)
        {
            int count = int(_ctx.run.relics.size());

            if(bn::keypad::down_pressed() && _relic_cursor + 1 < count)
            {
                ++_relic_cursor;
                play_sfx(Sfx::UI_CURSOR);
                _refresh_relics();
            }
            else if(bn::keypad::up_pressed() && _relic_cursor > 0)
            {
                --_relic_cursor;
                play_sfx(Sfx::UI_CURSOR);
                _refresh_relics();
            }
        }
        else
        {
            _picker.update(_ctx.frame);
        }

        if(_view_count() > 1 && (bn::keypad::l_pressed() || bn::keypad::r_pressed()))
        {
            play_sfx(Sfx::UI_CURSOR);
            _show(_next_view(bn::keypad::r_pressed() ? 1 : -1));
        }

        if(bn::keypad::b_pressed() || bn::keypad::select_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _ctx.transition = Transition::QUICK;
            return _ctx.deck_return;
        }

        return bn::nullopt;
    }

private:
    // Card lists the viewer can show. The last two only exist during a fight.
    enum class View : uint8_t
    {
        DECK,
        EXHAUSTED, // cards exhausted in the fight the viewer was opened from
        POWERS,    // Power cards in play in that fight
        RELICS     // relics of the run, as a list
    };

    static constexpr int VIEWS = 4;
    static constexpr int RELIC_ROWS = 6;
    static constexpr int RELIC_ROW_Y = -56;
    static constexpr int RELIC_ROW_STEP = 12;
    static constexpr int RELIC_TEXT_Y = 24;
    static constexpr int RELIC_TEXT_STEP = 10;
    static constexpr int RELIC_TEXT_WIDTH = 224;

    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    CardPicker _picker;
    Label _title;
    Label _hint;
    View _view = View::DECK;
    int _relic_cursor = 0;
    bn::vector<Label, RELIC_ROWS> _relic_rows;
    bn::vector<Label, 4> _relic_text;

    [[nodiscard]] bool _available(View view) const
    {
        if(view == View::DECK)
        {
            return true;
        }

        if(view == View::RELICS)
        {
            return ! _ctx.run.relics.empty();
        }

        // Opened from a fight, directly or through the pause menu.
        bool in_combat = _ctx.deck_return == SceneId::COMBAT ||
                         (_ctx.deck_return == SceneId::PAUSE && _ctx.pause_return == SceneId::COMBAT);

        if(! in_combat)
        {
            return false;
        }

        return view == View::EXHAUSTED ? ! _ctx.run.combat.exhaust_pile.empty()
                                       : ! _ctx.run.combat.powers_in_play.empty();
    }

    [[nodiscard]] int _view_count() const
    {
        return 1 + _available(View::EXHAUSTED) + _available(View::POWERS) + _available(View::RELICS);
    }

    // Next view with something to show, going forward (1) or backward (-1).
    [[nodiscard]] View _next_view(int step) const
    {
        int index = int(_view);

        for(int tries = 0; tries < VIEWS; ++tries)
        {
            index = (index + step + VIEWS) % VIEWS;

            if(_available(View(index)))
            {
                break;
            }
        }

        return View(index);
    }

    void _show(View view)
    {
        const engine::RunManager& run = _ctx.run;
        TextLine title;
        bn::ostringstream stream(title);

        _view = view;
        _picker.clear();
        _relic_rows.clear();
        _relic_text.clear();

        if(view == View::RELICS)
        {
            _relic_cursor = 0;
            int count = int(run.relics.size());
            stream << "RELIQUES - " << count << (count > 1 ? " possedees" : " possedee");
            _refresh_relics();
        }
        else if(view == View::EXHAUSTED)
        {
            int count = int(run.combat.exhaust_pile.size());

            for(int index = 0; index < count; ++index)
            {
                _picker.add(run.combat.exhaust_pile[index], index);
            }

            stream << "EPUISEES - " << count << (count > 1 ? " cartes" : " carte");
        }
        else if(view == View::POWERS)
        {
            int count = int(run.combat.powers_in_play.size());

            for(int index = 0; index < count; ++index)
            {
                _picker.add(run.combat.powers_in_play[index], index);
            }

            stream << "POUVOIRS - " << count << (count > 1 ? " cartes" : " carte");
        }
        else
        {
            for(int index = 0; index < int(run.master_deck.size()); ++index)
            {
                _picker.add(run.master_deck[index], index);
            }

            stream << "DECK - " << int(run.master_deck.size()) << " cartes - " << int(run.relics.size())
                   << (run.relics.size() > 1 ? " reliques" : " relique");
        }

        _title.set(0, -72, title, Align::CENTER, TextColor::GOLD);
        _hint.set(0, 73, _view_count() > 1 ? "◄► Parcourir  L/R Vue  B Retour" : "◄► Parcourir   B Retour", Align::CENTER,
                  TextColor::GRAY);

        if(view == View::RELICS)
        {
            _hint.set(0, 73, "▲▼ Parcourir  L/R Vue  B Retour", Align::CENTER, TextColor::GRAY);
        }
    }

    // Rows of the relic list around the cursor, then the description of the relic under it.
    void _refresh_relics()
    {
        const engine::RunManager& run = _ctx.run;
        int count = int(run.relics.size());
        int first = bn::clamp(_relic_cursor - RELIC_ROWS / 2, 0, bn::max(0, count - RELIC_ROWS));

        _relic_rows.clear();
        _relic_text.clear();

        for(int index = first; index < count && ! _relic_rows.full(); ++index)
        {
            bool focused = index == _relic_cursor;
            TextLine row;
            bn::ostringstream stream(row);
            stream << (focused ? "► " : "  ") << relic_name(run.relics[index]);
            _relic_rows.emplace_back().set(-112, RELIC_ROW_Y + (index - first) * RELIC_ROW_STEP, row, Align::LEFT,
                                           focused ? TextColor::GOLD : TextColor::WHITE);
        }

        if(_relic_cursor < count)
        {
            bn::vector<TextLine, 4> lines;
            wrap_text(relic_description(run.relics[_relic_cursor]), RELIC_TEXT_WIDTH, lines);

            for(int index = 0; index < lines.size() && ! _relic_text.full(); ++index)
            {
                _relic_text.emplace_back().set(-112, RELIC_TEXT_Y + index * RELIC_TEXT_STEP, lines[index], Align::LEFT,
                                               TextColor::WHITE);
            }
        }
    }
};

}

bn::unique_ptr<Scene> make_deck_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new DeckScene(context));
}

}
