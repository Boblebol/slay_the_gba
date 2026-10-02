#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sstream.h"

#include "bn_regular_bg_items_bg_map.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/card_picker.h"
#include "gba/layers.h"
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
        _show(View::DECK);
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        _picker.update(_ctx.frame);

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
        POWERS     // Power cards in play in that fight
    };

    static constexpr int VIEWS = 3;

    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    CardPicker _picker;
    Label _title;
    Label _hint;
    View _view = View::DECK;

    [[nodiscard]] bool _available(View view) const
    {
        if(view == View::DECK)
        {
            return true;
        }

        if(_ctx.deck_return != SceneId::COMBAT)
        {
            return false;
        }

        return view == View::EXHAUSTED ? ! _ctx.run.combat.exhaust_pile.empty()
                                       : ! _ctx.run.combat.powers_in_play.empty();
    }

    [[nodiscard]] int _view_count() const
    {
        return 1 + _available(View::EXHAUSTED) + _available(View::POWERS);
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

        if(view == View::EXHAUSTED)
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
    }
};

}

bn::unique_ptr<Scene> make_deck_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new DeckScene(context));
}

}
