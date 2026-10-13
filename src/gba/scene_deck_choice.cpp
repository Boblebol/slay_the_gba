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
#include "gba/widgets.h"

namespace gba
{

namespace
{

constexpr int LEAVE_FRAMES = 40;

// Cards of the deck to upgrade, remove or transform, or offered cards to add (Sea Glass), asked by the run (a relic
// just obtained). Same picker as the forge of the campfire.
class DeckChoiceScene final : public Scene
{
public:
    explicit DeckChoiceScene(GameContext& context) :
        _ctx(context),
        _bg(bn::regular_bg_items::bg_map.create_bg(BG_ORIGIN_X, 176 + (context.frame % 512))),
        _picker(CardPickerLayout{ 50, -74, -22, -30, -50, 142 })
    {
        _refresh();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        engine::RunManager& run = _ctx.run;
        _popups.update();
        _picker.update(_ctx.frame);

        if(! run.has_deck_choice())
        {
            // The last card is chosen: its popup plays, then the room the choice came from is back.
            if(++_leave_timer > LEAVE_FRAMES)
            {
                _ctx.transition = Transition::QUICK;
                return _ctx.deck_choice_return;
            }

            return bn::nullopt;
        }

        if(bn::keypad::a_pressed() && _picker.size() > 0)
        {
            engine::DeckChoiceKind kind = run.deck_choice.kind;

            if(run.resolve_deck_choice(_picker.selected_tag()))
            {
                _popups.spawn(-74, -60, _done_text(kind), TextColor::GREEN);
                play_sfx(kind == engine::DeckChoiceKind::UPGRADE ? Sfx::BUFF : Sfx::UI_CONFIRM);
                _refresh();
            }
            else
            {
                play_sfx(Sfx::ERROR);
            }
        }
        else if(bn::keypad::b_pressed())
        {
            if(run.skip_deck_choice())
            {
                play_sfx(Sfx::UI_CANCEL);
                _refresh();
            }
            else
            {
                play_sfx(Sfx::ERROR);
            }
        }

        return bn::nullopt;
    }

private:
    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    CardPicker _picker;
    Label _title;
    Label _hint;
    PopupPool _popups;
    int _leave_timer = 0;

    [[nodiscard]] static bn::string_view _done_text(engine::DeckChoiceKind kind)
    {
        switch(kind)
        {
        case engine::DeckChoiceKind::UPGRADE:
            return "Améliorée !";
        case engine::DeckChoiceKind::REMOVE:
            return "Retirée !";
        case engine::DeckChoiceKind::ENCHANT:
            return "Envoûtée !";
        case engine::DeckChoiceKind::DUPLICATE:
            return "Copiée !";
        case engine::DeckChoiceKind::ADD:
            return "Ajoutée !";
        default:
            return "Transformée !";
        }
    }

    // Cards the choice still allows, title and hint. Once the choice is over, the picker is emptied.
    void _refresh()
    {
        const engine::RunManager& run = _ctx.run;
        int selected = _picker.selected();
        _picker.clear();

        if(! run.has_deck_choice())
        {
            _hint.clear();
            return;
        }

        engine::DeckChoiceKind kind = run.deck_choice.kind;
        _picker.set_preview_upgraded(kind == engine::DeckChoiceKind::UPGRADE);

        // Sea Glass: the cards offered, not the deck's.
        bool offers = kind == engine::DeckChoiceKind::ADD;
        int count = offers ? int(run.deck_choice_offers.size()) : int(run.master_deck.size());

        for(int index = 0; index < count; ++index)
        {
            if(run.deck_choice_allows(index))
            {
                _picker.add(offers ? run.deck_choice_offers[index] : run.master_deck[index], index);
            }
        }

        _picker.set_selected(selected);

        int remaining = run.deck_choice.remaining;
        TextLine title;
        bn::ostringstream stream(title);
        stream << (kind == engine::DeckChoiceKind::UPGRADE ? "AMELIORER" :
                   kind == engine::DeckChoiceKind::REMOVE ? "RETIRER" :
                   kind == engine::DeckChoiceKind::ENCHANT ? "ENVOUTER" :
                   kind == engine::DeckChoiceKind::DUPLICATE ? "COPIER" :
                   kind == engine::DeckChoiceKind::ADD ? "AJOUTER" : "TRANSFORMER")
               << " - " << remaining << (remaining > 1 ? " cartes" : " carte");
        _title.set(0, -72, title, Align::CENTER, TextColor::GOLD);
        _hint.set(0, 73, run.deck_choice.optional ? "◄► Carte   A Choisir   B Terminer" : "◄► Carte   A Choisir",
                  Align::CENTER, TextColor::GRAY);
    }
};

}

bn::unique_ptr<Scene> make_deck_choice_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new DeckChoiceScene(context));
}

}
