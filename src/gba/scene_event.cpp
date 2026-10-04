#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sstream.h"
#include "bn_vector.h"

#include "engine/event_manager.h"

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
// Choice list: three choices keep the spacing they always had, four close up to leave the detail its two lines.
constexpr int MAX_CHOICES = int(engine::EVENT_MAX_CHOICES);
constexpr int CHOICE_STEP = 13;
constexpr int CHOICE_STEP_TIGHT = 11;
constexpr int DETAIL_LINES = 2;
constexpr int DETAIL_TOP = 62;
// Result: the message on the first lines, what the choice did on the last two.
constexpr int RESULT_MESSAGE_LINES = 3;
constexpr int SUMMARY_LINES = MAX_LINES - RESULT_MESSAGE_LINES;
constexpr int RESULT_MESSAGE_BYTES = 192; // room for three full lines of text
constexpr bn::string_view NBSP = "\xC2\xA0"; // drawn as a space, never a place to break a line

// The event of the room; the first of the table if the scene is ever entered without one.
const engine::EventEncounter& current_event(const GameContext& context)
{
    const engine::EventEncounter* event = context.run.current_event;
    return event ? *event : engine::EventManager::all()[0];
}

bn::string_view view(std::string_view text)
{
    return bn::string_view(text.data(), int(text.size()));
}

// `name` cut to `max_bytes` at most, never in the middle of a character.
bn::string_view clipped(std::string_view name, int max_bytes)
{
    int size = int(name.size());

    if(size > max_bytes)
    {
        size = max_bytes;

        while(size > 0 && (static_cast<unsigned char>(name[size]) & 0xC0) == 0x80)
        {
            --size;
        }
    }

    return bn::string_view(name.data(), size);
}

// What a choice needs, told to the player who cannot take it.
void describe_condition(const engine::EventChoice& choice, TextLine& out)
{
    out.clear();
    bn::ostringstream stream(out);

    switch(choice.condition)
    {
    case engine::EventCondition::GOLD_AT_LEAST:
        stream << "Il faut " << int(choice.condition_value) << " Or.";
        break;
    case engine::EventCondition::HP_ABOVE:
        stream << "Il faut plus de " << int(choice.condition_value) << " PV.";
        break;
    case engine::EventCondition::MAX_HP_ABOVE:
        stream << "Il faut plus de " << int(choice.condition_value) << " PV Max.";
        break;
    case engine::EventCondition::DECK_AT_LEAST:
        stream << "Il faut " << int(choice.condition_value) << " cartes dans le deck.";
        break;
    case engine::EventCondition::UPGRADABLE_CARD:
        stream << "Aucune carte à améliorer.";
        break;
    case engine::EventCondition::FREE_POTION_SLOT:
        stream << "Ceinture de potions pleine.";
        break;
    default:
        stream << "Choix impossible.";
        break;
    }
}

// One effect of the choice, as a few words: "-6 PV", "Relique : Vajra".
void describe_effect(const engine::EventEffect& effect, bn::string<80>& out)
{
    out.clear();
    bn::ostringstream stream(out);
    int amount = effect.amount;
    bn::string_view name = clipped(effect.name, 44);

    switch(effect.type)
    {
    case engine::EventOutcomeType::GAIN_GOLD:
        stream << "+" << amount << NBSP << "Or";
        break;
    case engine::EventOutcomeType::LOSE_GOLD:
        stream << "-" << amount << NBSP << "Or";
        break;
    case engine::EventOutcomeType::HEAL:
        stream << "+" << amount << NBSP << "PV";
        break;
    case engine::EventOutcomeType::LOSE_HP:
        stream << "-" << amount << NBSP << "PV";
        break;
    case engine::EventOutcomeType::GAIN_MAX_HP:
        stream << "+" << amount << NBSP << "PV" << NBSP << "Max";
        break;
    case engine::EventOutcomeType::LOSE_MAX_HP:
        stream << "-" << amount << NBSP << "PV" << NBSP << "Max";
        break;
    case engine::EventOutcomeType::GAIN_RELIC:
        stream << "Relique" << NBSP << ":" << NBSP << name;
        break;
    case engine::EventOutcomeType::GAIN_POTION:
        stream << "Potion" << NBSP << ":" << NBSP << name;
        break;
    case engine::EventOutcomeType::GAIN_CARD:
        stream << "Carte" << NBSP << ":" << NBSP << name;
        break;
    case engine::EventOutcomeType::REMOVE_CARD:
        stream << "Retirée" << NBSP << ":" << NBSP << name;
        break;
    case engine::EventOutcomeType::UPGRADE_CARD:
        stream << "Améliorée" << NBSP << ":" << NBSP << name;
        break;
    case engine::EventOutcomeType::TRANSFORM_CARD:
        stream << "Devient" << NBSP << ":" << NBSP << name;
        break;
    case engine::EventOutcomeType::FIGHT:
        stream << "Combat" << NBSP << "!";
        break;
    default:
        break;
    }
}

// Reveals wrapped text one character at a time, Pokemon dialogue style.
class Typewriter
{
public:
    // `max_lines` leaves the last lines of the panel to something else.
    void start(const bn::string_view& text, int max_lines = MAX_LINES)
    {
        wrap_text(text, TEXT_WIDTH, _lines);

        while(_lines.size() > max_lines)
        {
            _lines.pop_back();
        }

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
        _event(current_event(context)),
        _bg(event_bg(_event.id).create_bg(BG_ORIGIN_X, BG_ORIGIN_Y))
    {
        _title.set(0, -72, view(_event.title), Align::CENTER, TextColor::GOLD);
        _writer.start(view(_event.dialogue));
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
            _prompt.set_visible(false); // the second line of the detail ends where the prompt blinks

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
                char message[RESULT_MESSAGE_BYTES] = {};
                engine::EventResult result;

                if(_ctx.run.choose_event_choice(_cursor, message, sizeof(message), &result))
                {
                    _choices.clear();
                    _clear_detail();
                    _writer.start(bn::string_view(message), RESULT_MESSAGE_LINES);
                    _stage = Stage::RESULT;
                    _show_summary(result);
                    _spawn_outcome_popups();
                }
                else
                {
                    // A choice whose condition is not met: it stays grey and the reason stays on screen.
                    play_sfx(Sfx::ERROR);
                }
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
                else if(_ctx.run.hp <= 0)
                {
                    return SceneId::GAME_OVER;
                }
                else
                {
                    // The map, or the fight the choice started.
                    return _ctx.run.phase == engine::RunPhase::IN_COMBAT ? scene_for_run_phase(_ctx) : SceneId::MAP;
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
    Label _detail[DETAIL_LINES];
    Label _summary[SUMMARY_LINES];
    bn::vector<Label, MAX_CHOICES> _choices;
    PopupPool _popups;
    Stage _stage = Stage::DIALOGUE;
    int _timer = 0;
    int _cursor = 0;
    int _hp_before = 0;
    int _gold_before = 0;

    [[nodiscard]] int _choice_y(int index) const
    {
        int step = int(_event.choices.size()) > 3 ? CHOICE_STEP_TIGHT : CHOICE_STEP;
        return PANEL_TOP + index * step;
    }

    void _clear_detail()
    {
        for(Label& line : _detail)
        {
            line.clear();
        }
    }

    void _refresh_choices()
    {
        _choices.clear();

        for(int index = 0; index < int(_event.choices.size()); ++index)
        {
            const engine::EventChoice& choice = _event.choices[index];
            bool available = engine::EventManager::choice_available(_ctx.run, choice);
            TextLine line;
            line.append(index == _cursor ? "► " : "  ");
            line.append(view(choice.label));
            _choices.emplace_back().set(TEXT_LEFT, _choice_y(index), line, Align::LEFT,
                                        ! available ? TextColor::GRAY
                                                    : index == _cursor ? TextColor::GOLD : TextColor::WHITE);
        }

        // Under the list: what the selected choice costs and gives, on two lines at most. A choice that
        // cannot be taken gives its reason first.
        const engine::EventChoice& selected = _event.choices[_cursor];
        bn::vector<TextLine, DETAIL_LINES> lines;
        wrap_text(view(selected.cost_benefit), TEXT_WIDTH, lines);
        _clear_detail();

        if(engine::EventManager::choice_available(_ctx.run, selected))
        {
            for(int index = 0; index < lines.size(); ++index)
            {
                _detail[index].set(TEXT_LEFT, DETAIL_TOP + index * LINE_HEIGHT, lines[index], Align::LEFT,
                                   TextColor::BLUE);
            }
        }
        else
        {
            TextLine reason;
            describe_condition(selected, reason);
            _detail[0].set(TEXT_LEFT, DETAIL_TOP, reason, Align::LEFT, TextColor::RED);
            _detail[1].set(TEXT_LEFT, DETAIL_TOP + LINE_HEIGHT, lines.empty() ? bn::string_view() : bn::string_view(lines[0]),
                           Align::LEFT, TextColor::GRAY);
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

    // What the choice did, under the message: "-10 PV, Relique : Vajra".
    void _show_summary(const engine::EventResult& result)
    {
        bn::string<160> text;
        bn::string<80> piece;

        for(const engine::EventEffect& effect : result.effects)
        {
            // An effect that changed nothing (a heal at full HP) is not told.
            if(effect.amount == 0)
            {
                continue;
            }

            describe_effect(effect, piece);

            if(piece.empty() || text.size() + piece.size() + 2 > text.max_size())
            {
                continue;
            }

            if(! text.empty())
            {
                text.append(", ");
            }

            text.append(piece);
        }

        bn::vector<TextLine, SUMMARY_LINES> lines;
        wrap_text(text, TEXT_WIDTH, lines);

        for(int index = 0; index < SUMMARY_LINES; ++index)
        {
            if(index < lines.size())
            {
                _summary[index].set(TEXT_LEFT, PANEL_TOP + (RESULT_MESSAGE_LINES + index) * LINE_HEIGHT, lines[index],
                                    Align::LEFT, TextColor::GOLD);
            }
            else
            {
                _summary[index].clear();
            }
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
