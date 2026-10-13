#include "gba/scene_factories.h"

#include "bn_bg_palettes.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sstream.h"

#include "bn_regular_bg_items_bg_map.h"

#include "engine/pause_menu.h"
#include "engine/save_system.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/text.h"

namespace gba
{

namespace
{

// Pause menu (#10), opened with START in combat and on the map. The fight or the map scene is left while the menu
// is open, so its animations and timers stop; the fight comes back as it was (GameContext::combat_resume).
constexpr int TITLE_Y = -66;
constexpr int SUMMARY_Y = -52;
constexpr int ITEM_Y = -34;
constexpr int ITEM_STEP = 13;
constexpr int ITEM_X = -60;
constexpr int QUESTION_Y = -12;
constexpr int ANSWER_Y = 6;
constexpr int WARNING_Y = 24;
constexpr int HINT_Y = 70;
// The map behind the menu is darkened.
constexpr bn::fixed DIM = 0.6;

[[nodiscard]] engine::Button pressed_button()
{
    if(bn::keypad::up_pressed()) return engine::Button::UP;
    if(bn::keypad::down_pressed()) return engine::Button::DOWN;
    if(bn::keypad::left_pressed()) return engine::Button::LEFT;
    if(bn::keypad::right_pressed()) return engine::Button::RIGHT;
    if(bn::keypad::a_pressed()) return engine::Button::A;
    if(bn::keypad::b_pressed()) return engine::Button::B;
    if(bn::keypad::start_pressed()) return engine::Button::START;
    return engine::Button::SELECT; // no action in the menu
}

class PauseScene final : public Scene
{
public:
    explicit PauseScene(GameContext& context) :
        _ctx(context),
        _bg(bn::regular_bg_items::bg_map.create_bg(BG_ORIGIN_X, 176 + (context.frame % 512)))
    {
        const engine::RunManager& run = context.run;
        bool combat = context.pause_return == SceneId::COMBAT;
        _menu = engine::make_pause_menu(combat ? engine::PauseOrigin::COMBAT : engine::PauseOrigin::MAP,
                                        ! run.relics.empty(), run.phase == engine::RunPhase::MAP_NAVIGATION);
        _menu.cursor = bn::clamp(context.pause_cursor, 0, _menu.count - 1);

        _title.set(0, TITLE_Y, "PAUSE", Align::CENTER, TextColor::GOLD);

        TextLine summary;
        bn::ostringstream stream(summary);
        int hp = combat ? run.combat.player.hp : run.hp;
        int max_hp = combat ? run.combat.player.max_hp : run.max_hp;
        stream << "Acte " << int(run.current_act) << "  Étage " << int(run.current_floor) << "  PV " << hp << '/'
               << max_hp << "  " << run.gold << " Or";
        _summary.set(0, SUMMARY_Y, summary, Align::CENTER, TextColor::GRAY);
        _refresh();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        // Darken the background once the fade-in of the main loop has passed it.
        if(bn::bg_palettes::fade_intensity() < DIM)
        {
            bn::bg_palettes::set_fade(bn::color(0, 0, 0), DIM);
        }

        engine::Button button = pressed_button();

        if(button == engine::Button::SELECT)
        {
            return bn::nullopt;
        }

        bool was_confirming = _menu.confirming;
        int cursor = _menu.cursor;
        bool yes = _menu.confirm_yes;
        engine::PauseAction action = engine::press_pause(_menu, button);
        _ctx.pause_cursor = _menu.cursor;

        switch(action)
        {
        case engine::PauseAction::NONE:
            if(was_confirming != _menu.confirming)
            {
                play_sfx(_menu.confirming ? Sfx::UI_OPEN : Sfx::UI_CANCEL);
                _refresh();
            }
            else if(cursor != _menu.cursor || yes != _menu.confirm_yes)
            {
                play_sfx(Sfx::UI_CURSOR);
                _refresh();
            }
            return bn::nullopt;
        case engine::PauseAction::RESUME:
            play_sfx(Sfx::UI_CANCEL);
            return _back();
        case engine::PauseAction::END_TURN:
            play_sfx(Sfx::UI_CONFIRM);
            _ctx.pause_end_turn = true;
            return _back();
        case engine::PauseAction::DECK:
        case engine::PauseAction::RELICS:
            play_sfx(Sfx::UI_OPEN);
            _ctx.deck_return = SceneId::PAUSE;
            _ctx.deck_relics = action == engine::PauseAction::RELICS;
            _ctx.transition = Transition::QUICK;
            return SceneId::DECK;
        case engine::PauseAction::OPTIONS:
            play_sfx(Sfx::UI_OPEN);
            _ctx.options_return = SceneId::PAUSE;
            _ctx.transition = Transition::QUICK;
            return SceneId::OPTIONS;
        case engine::PauseAction::SAVE_QUIT:
            // Same checkpoint as the one written when the map opens: "Continuer" resumes from it.
            play_sfx(Sfx::UI_CONFIRM);
            engine::SaveManager::instance().autosave_run(_ctx.run);
            _ctx.title_menu = false;
            return SceneId::TITLE;
        default:
            // Abandoned: the run ends as a defeat, the end scene records it and erases the saved run.
            play_sfx(Sfx::UI_CONFIRM);
            _ctx.combat_resume = false;
            _ctx.pause_end_turn = false;
            _ctx.run.phase = engine::RunPhase::GAME_OVER;
            return SceneId::GAME_OVER;
        }
    }

private:
    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    engine::PauseMenu _menu;
    Label _title;
    Label _summary;
    Label _items[engine::PAUSE_MAX_ITEMS];
    Label _question;
    Label _answers;
    Label _warning;
    Label _hint;

    [[nodiscard]] SceneId _back()
    {
        _ctx.transition = Transition::QUICK;
        return _ctx.pause_return;
    }

    void _refresh()
    {
        if(_menu.confirming)
        {
            for(Label& item : _items)
            {
                item.clear();
            }

            _question.set(0, QUESTION_Y, "Abandonner la partie ?", Align::CENTER, TextColor::RED);
            if(_menu.confirm_yes)
            {
                _answers.set(0, ANSWER_Y, "  Non      ► Oui", Align::CENTER, TextColor::WHITE);
            }
            else
            {
                _answers.set(0, ANSWER_Y, "► Non        Oui", Align::CENTER, TextColor::WHITE);
            }
            _warning.set(0, WARNING_Y, "La partie comptera comme une défaite.", Align::CENTER, TextColor::GRAY);
            _hint.set(0, HINT_Y, "◄► Choisir  A Valider  B Annuler", Align::CENTER, TextColor::GRAY);
            return;
        }

        _question.clear();
        _answers.clear();
        _warning.clear();

        for(int index = 0; index < engine::PAUSE_MAX_ITEMS; ++index)
        {
            if(index >= _menu.count)
            {
                _items[index].clear();
                continue;
            }

            bool focused = index == _menu.cursor;
            engine::PauseItem item = _menu.items[index];
            TextLine text;
            text.append(focused ? "► " : "  ");
            text.append(engine::pause_label(item));
            TextColor color = focused ? TextColor::GOLD : TextColor::WHITE;

            if(item == engine::PauseItem::ABANDON && ! focused)
            {
                color = TextColor::RED;
            }

            _items[index].set(ITEM_X, ITEM_Y + index * ITEM_STEP, text, Align::LEFT, color);
        }

        _hint.set(0, HINT_Y, "▲▼ Choisir  A Valider  B Reprendre", Align::CENTER, TextColor::GRAY);
    }
};

}

bn::unique_ptr<Scene> make_pause_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new PauseScene(context));
}

}
