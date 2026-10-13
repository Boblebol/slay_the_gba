#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"

#include "bn_regular_bg_items_bg_victory.h"

#include "engine/options.h"
#include "engine/save_system.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/text.h"

namespace gba
{

namespace
{

// Options reached from the title screen (SELECT) or the pause menu: two volumes, the button preset and the legend of the buttons.
// The settings are written to the cartridge when the screen is left.
constexpr int TITLE_Y = -68;
constexpr int ROW_Y = -48;
constexpr int ROW_STEP = 14;
constexpr int LABEL_X = -100;
constexpr int VALUE_X = 100;
constexpr int LEGEND_Y = 4;
constexpr int LEGEND_STEP = 11;
constexpr int HINT_Y = 70;

class OptionsScene final : public Scene
{
public:
    explicit OptionsScene(GameContext& context) :
        _ctx(context),
        _bg(bn::regular_bg_items::bg_victory.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y))
    {
        // From the pause menu the music of the fight or of the map keeps playing.
        if(context.options_return == SceneId::TITLE)
        {
            play_music(Music::TITLE);
        }

        engine::sanitize_settings(engine::SaveManager::instance().settings());
        _title.set(0, TITLE_Y, "OPTIONS", Align::CENTER, TextColor::GOLD);
        _hint.set(0, HINT_Y, "◄ ► : changer   B : enregistrer", Align::CENTER, TextColor::GRAY);
        refresh();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        engine::GameSettings& settings = engine::SaveManager::instance().settings();
        int step = 0;

        if(bn::keypad::up_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _row = (_row + engine::OPTION_ROW_COUNT - 1) % engine::OPTION_ROW_COUNT;
            refresh();
        }
        else if(bn::keypad::down_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _row = (_row + 1) % engine::OPTION_ROW_COUNT;
            refresh();
        }
        else if(bn::keypad::left_pressed())
        {
            step = -1;
        }
        else if(bn::keypad::right_pressed() || bn::keypad::a_pressed())
        {
            step = 1;
        }

        if(step != 0)
        {
            engine::adjust_option(settings, engine::OptionRow(_row), step);
            play_sfx(Sfx::UI_CURSOR); // heard at the new SFX volume
            refresh();
        }

        if(bn::keypad::b_pressed() || bn::keypad::start_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            engine::SaveManager::instance().commit_to_storage();

            if(_ctx.options_return != SceneId::TITLE)
            {
                _ctx.transition = Transition::QUICK;
            }

            return _ctx.options_return;
        }

        return bn::nullopt;
    }

private:
    void refresh()
    {
        const engine::GameSettings& settings = engine::SaveManager::instance().settings();

        for(int index = 0; index < engine::OPTION_ROW_COUNT; ++index)
        {
            engine::OptionRow row = engine::OptionRow(index);
            bool selected = index == _row;
            int y = ROW_Y + index * ROW_STEP;
            TextColor color = selected ? TextColor::GOLD : TextColor::WHITE;

            TextLine label;
            label.append(selected ? "► " : "  ");
            label.append(engine::option_label(row));
            _labels[index].set(LABEL_X, y, label, Align::LEFT, color);

            TextLine value;
            value.append("◄ ");

            if(row == engine::OptionRow::CONTROLS)
            {
                value.append(engine::preset_name(engine::control_preset(settings)));
            }
            else
            {
                int volume = row == engine::OptionRow::MUSIC ? settings.music_volume : settings.sfx_volume;

                if(volume == 0)
                {
                    value.append("Non");
                }
                else
                {
                    for(int cell = 0; cell < engine::VOLUME_MAX; ++cell)
                    {
                        value.append(cell < volume ? "I" : ".");
                    }
                }
            }

            value.append(" ►");
            _values[index].set(VALUE_X, y, value, Align::RIGHT, color);
        }

        engine::ControlPreset preset = engine::control_preset(settings);

        for(int index = 0; index < engine::LEGEND_LINE_COUNT; ++index)
        {
            _legend[index].set(0, LEGEND_Y + index * LEGEND_STEP, engine::legend_line(preset, index), Align::CENTER,
                               TextColor::WHITE);
        }
    }

    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    Label _title;
    Label _hint;
    Label _labels[engine::OPTION_ROW_COUNT];
    Label _values[engine::OPTION_ROW_COUNT];
    Label _legend[engine::LEGEND_LINE_COUNT];
    int _row = 0;
};

}

bn::unique_ptr<Scene> make_options_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new OptionsScene(context));
}

}
