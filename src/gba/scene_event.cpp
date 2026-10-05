#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sstream.h"
#include "bn_vector.h"

#include "gba/assets.h"
#include "gba/audio.h"
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

// Reveals wrapped text one character at a time, Pokemon dialogue style.
class Typewriter
{
public:
    void start(const bn::string_view& text)
    {
        wrap_text(text, TEXT_WIDTH, _lines);
        _labels.clear();

        for(int index = 0; index < _lines.size(); ++index)
        {
            _labels.emplace_back();
        }

        _line = 0;
        _bytes = 0;
        _tick = 0;
    }

    void clear()
    {
        _lines.clear();
        _labels.clear();
    }

    [[nodiscard]] bool done() const
    {
        return _line >= _lines.size();
    }

    void finish()
    {
        for(int index = _line; index < _lines.size(); ++index)
        {
            _labels[index].set(TEXT_LEFT, PANEL_TOP + index * LINE_HEIGHT, _lines[index], Align::LEFT);
        }

        _line = _lines.size();
    }

    void update()
    {
        if(done())
        {
            return;
        }

        if(++_tick < 2)
        {
            return;
        }

        _tick = 0;
        const TextLine& line = _lines[_line];

        // Advance by one UTF-8 character.
        ++_bytes;

        while(_bytes < line.size() && (static_cast<unsigned char>(line[_bytes]) & 0xC0) == 0x80)
        {
            ++_bytes;
        }

        _labels[_line].set(TEXT_LEFT, PANEL_TOP + _line * LINE_HEIGHT, bn::string_view(line.data(), _bytes),
                           Align::LEFT);

        if(_bytes >= line.size())
        {
            ++_line;
            _bytes = 0;
        }
    }

private:
    bn::vector<TextLine, MAX_LINES> _lines;
    bn::vector<Label, MAX_LINES> _labels;
    int _line = 0;
    int _bytes = 0;
    int _tick = 0;
};

class EventScene final : public Scene
{
public:
    explicit EventScene(GameContext& context) :
        _ctx(context),
        _event(context.run.current_event),
        _bg(event_bg(_event.id).create_bg(BG_ORIGIN_X, BG_ORIGIN_Y))
    {
        _title.set(0, -72, bn::string_view(_event.title.data(), _event.title.size()), Align::CENTER, TextColor::GOLD);
        _writer.start(bn::string_view(_event.dialogue.data(), _event.dialogue.size()));
        _hp_before = context.run.hp;
        _gold_before = context.run.gold;
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
            else if(bn::keypad::a_pressed())
            {
                char message[96] = {};
                _ctx.run.choose_event_choice(_cursor, message, sizeof(message));
                _choices.clear();
                _detail.clear();
                _writer.start(bn::string_view(message));
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
                else
                {
                    return _ctx.run.hp <= 0 ? SceneId::GAME_OVER : scene_after_deck_choice(_ctx, SceneId::MAP);
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
    Typewriter _writer;
    Label _title;
    Label _prompt;
    Label _detail;
    bn::vector<Label, 3> _choices;
    PopupPool _popups;
    Stage _stage = Stage::DIALOGUE;
    int _timer = 0;
    int _cursor = 0;
    int _hp_before = 0;
    int _gold_before = 0;

    void _refresh_choices()
    {
        _choices.clear();

        for(int index = 0; index < int(_event.choices.size()); ++index)
        {
            const engine::EventChoice& choice = _event.choices[index];
            TextLine line;
            line.append(index == _cursor ? "► " : "  ");
            line.append(bn::string_view(choice.label.data(), choice.label.size()));
            _choices.emplace_back().set(TEXT_LEFT, PANEL_TOP + index * 13, line, Align::LEFT,
                                        index == _cursor ? TextColor::GOLD : TextColor::WHITE);
        }

        const engine::EventChoice& selected = _event.choices[_cursor];
        bn::vector<TextLine, 1> lines;
        wrap_text(bn::string_view(selected.cost_benefit.data(), selected.cost_benefit.size()), TEXT_WIDTH, lines);
        _detail.set(TEXT_LEFT, 64, lines.empty() ? bn::string_view() : bn::string_view(lines[0]), Align::LEFT,
                    TextColor::BLUE);
    }

    void _choices_bob()
    {
        if(_cursor < _choices.size())
        {
            int offset = wave(_timer, 30, 1).round_integer();
            _choices[_cursor].move_to(TEXT_LEFT + 2 + offset, PANEL_TOP + _cursor * 13);
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
