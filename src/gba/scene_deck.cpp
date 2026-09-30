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
        const engine::RunManager& run = context.run;

        for(int index = 0; index < int(run.master_deck.size()); ++index)
        {
            _picker.add(run.master_deck[index], index);
        }

        TextLine title;
        bn::ostringstream stream(title);
        stream << "DECK - " << int(run.master_deck.size()) << " cartes - " << int(run.relics.size())
               << (run.relics.size() > 1 ? " reliques" : " relique");
        _title.set(0, -72, title, Align::CENTER, TextColor::GOLD);
        _hint.set(0, 73, "◄► Parcourir   B Retour", Align::CENTER, TextColor::GRAY);
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        _picker.update(_ctx.frame);

        if(bn::keypad::b_pressed() || bn::keypad::select_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _ctx.transition = Transition::QUICK;
            return _ctx.deck_return;
        }

        return bn::nullopt;
    }

private:
    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    CardPicker _picker;
    Label _title;
    Label _hint;
};

}

bn::unique_ptr<Scene> make_deck_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new DeckScene(context));
}

}
