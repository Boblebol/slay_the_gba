#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sstream.h"
#include "bn_vector.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/dialogue_box.h"
#include "gba/fx.h"
#include "gba/layers.h"
#include "gba/text.h"
#include "gba/widgets.h"

namespace gba
{

namespace
{

constexpr int PANEL_TOP = 16;
constexpr int LINE_HEIGHT = 11;
constexpr int TEXT_LEFT = -112;
constexpr int TEXT_WIDTH = 224;
constexpr int MAX_LINES = 5;

constexpr DialogueStyle PANEL_STYLE = { TEXT_LEFT, PANEL_TOP, LINE_HEIGHT, TEXT_WIDTH, MAX_LINES, Align::LEFT,
                                        TextColor::WHITE };

class EventScene final : public Scene
{
public:
    explicit EventScene(GameContext& context) :
        _ctx(context),
        _event(context.run.current_event),
        _bg(event_bg(_event.id).create_bg(BG_ORIGIN_X, BG_ORIGIN_Y))
    {
        _title.set(0, -72, bn::string_view(_event.title.data(), _event.title.size()), Align::CENTER, TextColor::GOLD);
        _open_page();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_timer;
        _writer.update();
        _popups.update();

        switch(_stage)
        {
        case Stage::DIALOGUE:
            if(bn::keypad::a_pressed())
            {
                if(! _writer.done())
                {
                    _writer.finish();
                }
                else if(_writer.has_next_page())
                {
                    play_sfx(Sfx::UI_CURSOR);
                    _writer.next_page();
                }
                else
                {
                    play_sfx(Sfx::UI_CONFIRM);
                    _stage = Stage::CHOICE;
                    _writer.clear();
                    _refresh_choices();
                }
            }

            _prompt.set(108, 72, "▼", Align::CENTER, TextColor::GOLD);
            _prompt.set_visible(_writer.done() && (_timer / 12) % 2 == 0);
            break;

        case Stage::CHOICE:
        {
            int count = int(_event.choices.size());

            if(bn::keypad::up_pressed())
            {
                play_sfx(Sfx::UI_CURSOR);
                _cursor = (_cursor + count - 1) % count;
                _refresh_choices();
            }
            else if(bn::keypad::down_pressed())
            {
                play_sfx(Sfx::UI_CURSOR);
                _cursor = (_cursor + 1) % count;
                _refresh_choices();
            }
            else if(bn::keypad::a_pressed() && ! _event.choices[_cursor].enabled)
            {
                play_sfx(Sfx::ERROR);
            }
            else if(bn::keypad::a_pressed())
            {
                char message[256] = {};
                _ctx.run.choose_event_choice(_cursor, message, sizeof(message));
                _choices.clear();
                _detail.clear();
                _writer.start(bn::string_view(message), PANEL_STYLE);
                _stage = Stage::RESULT;
                _spawn_outcome_popups();
            }

            _choices_bob();
            break;
        }

        case Stage::RESULT:
            _prompt.set(108, 72, "▼", Align::CENTER, TextColor::GOLD);
            _prompt.set_visible(_writer.done() && (_timer / 12) % 2 == 0);

            if(bn::keypad::a_pressed())
            {
                if(! _writer.done())
                {
                    _writer.finish();
                }
                else if(_writer.has_next_page())
                {
                    play_sfx(Sfx::UI_CURSOR);
                    _writer.next_page();
                }
                else
                {
                    return _leave_result();
                }
            }
            break;
        }

        return bn::nullopt;
    }

private:
    enum class Stage : uint8_t
    {
        DIALOGUE,
        CHOICE,
        RESULT
    };

    GameContext& _ctx;
    const engine::EventEncounter& _event;
    bn::regular_bg_ptr _bg;
    DialogueBox _writer;
    Label _title;
    Label _prompt;
    bn::vector<Label, 2> _detail;
    bn::vector<Label, engine::MAX_EVENT_OPTIONS> _choices;
    PopupPool _popups;
    Stage _stage = Stage::DIALOGUE;
    int _timer = 0;
    int _cursor = 0;
    int _hp_before = 0;
    int _gold_before = 0;

    // Shows the page of the event the run is on: its text, then its options.
    void _open_page()
    {
        _writer.start(bn::string_view(_event.dialogue.data(), _event.dialogue.size()), PANEL_STYLE);
        _stage = Stage::DIALOGUE;
        _cursor = 0;
        _hp_before = _ctx.run.hp;
        _gold_before = _ctx.run.gold;
    }

    // Once the result is read: the next page of the event, or the scene the option leads to.
    [[nodiscard]] bn::optional<SceneId> _leave_result()
    {
        engine::RunManager& run = _ctx.run;

        if(run.hp <= 0)
        {
            return SceneId::GAME_OVER;
        }

        if(run.phase == engine::RunPhase::EVENT)
        {
            if(run.has_deck_choice())
            {
                return scene_after_deck_choice(_ctx, SceneId::EVENT);
            }

            _open_page();
            return bn::nullopt;
        }

        if(run.phase == engine::RunPhase::IN_COMBAT)
        {
            return scene_for_run_phase(_ctx);
        }

        return scene_after_deck_choice(_ctx, run.phase == engine::RunPhase::REWARD_SCREEN ? SceneId::REWARDS : SceneId::MAP);
    }

    [[nodiscard]] int _choice_y(int index) const
    {
        return PANEL_TOP + index * (_event.choices.size() > 3 ? 12 : 13);
    }

    void _refresh_choices()
    {
        _choices.clear();

        for(int index = 0; index < int(_event.choices.size()); ++index)
        {
            const engine::EventChoice& choice = _event.choices[index];
            TextLine line;
            line.append(index == _cursor ? "► " : "  ");
            line.append(bn::string_view(choice.label.data(), choice.label.size()));
            // An option the run cannot take (not enough gold...) stays visible, greyed out.
            TextColor color = ! choice.enabled ? TextColor::GRAY : index == _cursor ? TextColor::GOLD : TextColor::WHITE;
            _choices.emplace_back().set(TEXT_LEFT, _choice_y(index), line, Align::LEFT, color);
        }

        const engine::EventChoice& selected = _event.choices[_cursor];
        // The preview of the consequences may need two lines.
        bn::vector<TextLine, 2> lines;
        wrap_text(bn::string_view(selected.cost_benefit.data(), selected.cost_benefit.size()), TEXT_WIDTH, lines);
        _detail.clear();

        for(int index = 0; index < lines.size(); ++index)
        {
            _detail.emplace_back().set(TEXT_LEFT, 60 + index * LINE_HEIGHT, lines[index], Align::LEFT, TextColor::BLUE);
        }
    }

    void _choices_bob()
    {
        if(_cursor < _choices.size())
        {
            int offset = wave(_timer, 30, 1).round_integer();
            _choices[_cursor].move_to(TEXT_LEFT + 2 + offset, _choice_y(_cursor));
        }
    }

    void _spawn_outcome_popups()
    {
        bn::string<16> text;
        bn::ostringstream stream(text);
        int hp_delta = _ctx.run.hp - _hp_before;
        int gold_delta = _ctx.run.gold - _gold_before;

        if(hp_delta != 0)
        {
            stream << (hp_delta > 0 ? "+" : "") << hp_delta << " PV";
            _popups.spawn(-40, -10, text, hp_delta > 0 ? TextColor::GREEN : TextColor::RED);
            play_sfx(hp_delta > 0 ? Sfx::HEAL : Sfx::HIT_SLASH);
        }

        if(gold_delta != 0)
        {
            text.clear();
            stream << (gold_delta > 0 ? "+" : "") << gold_delta << " Or";
            _popups.spawn(40, -10, text, TextColor::GOLD, 8);

            if(hp_delta == 0)
            {
                play_sfx(Sfx::GOLD);
            }
        }
    }
};

}

bn::unique_ptr<Scene> make_event_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new EventScene(context));
}

}
