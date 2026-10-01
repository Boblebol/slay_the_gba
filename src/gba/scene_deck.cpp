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

        if(_has_exhausted() && (bn::keypad::l_pressed() || bn::keypad::r_pressed()))
        {
            play_sfx(Sfx::UI_CURSOR);
            _show(_view == View::DECK ? View::EXHAUSTED : View::DECK);
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
    enum class View : uint8_t
    {
        DECK,
        EXHAUSTED   // cards exhausted in the fight the viewer was opened from
    };

    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    CardPicker _picker;
    Label _title;
    Label _hint;
    View _view = View::DECK;

    [[nodiscard]] bool _has_exhausted() const
    {
        return _ctx.deck_return == SceneId::COMBAT && ! _ctx.run.combat.exhaust_pile.empty();
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

        if(! _has_exhausted())
        {
            _hint.set(0, 73, "◄► Parcourir   B Retour", Align::CENTER, TextColor::GRAY);
        }
        else if(view == View::EXHAUSTED)
        {
            _hint.set(0, 73, "◄► Parcourir  L/R Deck  B Retour", Align::CENTER, TextColor::GRAY);
        }
        else
        {
            _hint.set(0, 73, "◄► Parcourir  L/R Épuisées  B Retour", Align::CENTER, TextColor::GRAY);
        }
    }
};

}

bn::unique_ptr<Scene> make_deck_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new DeckScene(context));
}

}
