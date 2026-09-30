#include "gba/scene_factories.h"

#include <span>

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_text_generator.h"
#include "bn_sstream.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bg_title.h"
#include "common_variable_16x16_sprite_font.h"

#include "engine/save_system.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/fx.h"
#include "gba/layers.h"
#include "gba/text.h"

namespace gba
{

namespace
{

constexpr int LOGO_Y = -62;
constexpr int MENU_Y = 44;
constexpr int MENU_SPACING = 12;

enum class TitleStage : uint8_t
{
    PRESS_START,
    MENU,       // "Continuer" / "Nouvelle partie", only when a run is saved
    PALETTE
};

class TitleScene final : public Scene
{
public:
    explicit TitleScene(GameContext& context) :
        _ctx(context),
        _bg(bn::regular_bg_items::bg_title.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y)),
        _hero(hero_battler().create_sprite(0, 200))
    {
        bn::sprite_text_generator logo_generator(common::variable_16x16_sprite_font);
        logo_generator.set_center_alignment();
        logo_generator.set_z_order(Z_TEXT);
        logo_generator.generate(0, LOGO_Y, "SLAY THE GBA", _logo);

        for(bn::sprite_ptr& sprite : _logo)
        {
            _logo_x.push_back(sprite.x().round_integer());
            sprite.set_y(-120);
        }

        const engine::CharacterDef& hero = engine::get_character(current_hero());
        _hero_name.set(0, -42, bn::string_view(hero.name.data(), hero.name.size()), Align::CENTER, TextColor::GOLD);
        _hero_name.set_visible(false);
        _zone.set(0, -31, hero_zone_name(), Align::CENTER, TextColor::GRAY);
        _zone.set_visible(false);
        _press.set(0, 50, "Appuyez sur START", Align::CENTER, TextColor::WHITE);
        _press.set_visible(false);
        _credits.set(0, 72, "Fan-game - Slay the Spire (c) Mega Crit", Align::CENTER, TextColor::GRAY);
        _hero.set_z_order(Z_ACTOR);

        _palettes = engine::get_character_palettes(current_hero());
        play_music(Music::TITLE);
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_timer;
        _animate_logo();

        switch(_stage)
        {
        case TitleStage::PRESS_START:
            return _update_press_start();
        case TitleStage::MENU:
            return _update_menu();
        default:
            return _update_palette();
        }
    }

private:
    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _hero;
    bn::vector<bn::sprite_ptr, 12> _logo;
    bn::vector<int, 12> _logo_x;
    Label _hero_name;
    Label _zone;
    Label _press;
    Label _credits;
    Label _variant;
    Label _description;
    Label _hint;
    std::span<const engine::PaletteDef> _palettes;
    int _timer = 0;
    int _choice_timer = 0;
    int _selected = 0;
    int _menu_index = 0;
    TitleStage _stage = TitleStage::PRESS_START;
    Label _menu[2];

    [[nodiscard]] bn::optional<SceneId> _update_press_start()
    {
        if(_timer > 30)
        {
            _hero_name.set_visible(true);
            _zone.set_visible(true);
            _press.set_visible((_timer / 24) % 2 == 0);
        }

        if(_timer > 20 && (bn::keypad::start_pressed() || bn::keypad::a_pressed()))
        {
            play_sfx(Sfx::UI_CONFIRM);
            _press.clear();

            if(engine::SaveManager::instance().has_active_run())
            {
                _stage = TitleStage::MENU;
                _menu_index = 0;
                _refresh_menu();
            }
            else
            {
                _open_palette();
            }
        }

        return bn::nullopt;
    }

    [[nodiscard]] bn::optional<SceneId> _update_menu()
    {
        if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _menu_index = 1 - _menu_index;
            _refresh_menu();
        }
        else if(bn::keypad::b_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _close_menu();
            _stage = TitleStage::PRESS_START;
        }
        else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            play_sfx(Sfx::UI_CONFIRM);
            _close_menu();

            if(_menu_index == 0)
            {
                return _continue_run();
            }

            _open_palette();
        }

        return bn::nullopt;
    }

    [[nodiscard]] bn::optional<SceneId> _update_palette()
    {
        ++_choice_timer;

        // Hero rises from the bottom of the screen, then idles.
        bn::fixed t = ease_out(bn::min(bn::fixed(_choice_timer) / 20, bn::fixed(1)));
        _hero.set_position(0, lerp(120, 8, t) + wave(_timer, 60, 2));

        int count = int(_palettes.size());

        if(bn::keypad::left_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _selected = (_selected + count - 1) % count;
            _refresh_choice();
        }
        else if(bn::keypad::right_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _selected = (_selected + 1) % count;
            _refresh_choice();
        }
        else if(bn::keypad::b_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _stage = TitleStage::PRESS_START;
            _hero.set_y(200);
            _variant.clear();
            _description.clear();
            _hint.clear();
        }
        else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            play_sfx(Sfx::UI_CONFIRM);
            engine::PaletteVariant variant = _palettes[_selected].variant;
            _ctx.palette = variant;
            _ctx.run.start_run(current_hero(), 0, variant, 12345u + unsigned(_ctx.frame) * 2654435761u);
            _ctx.nav.reset_to_player_floor();
            _ctx.map_toast.clear();
            _ctx.map_toast.append(hero_zone_name());
            return SceneId::MAP;
        }

        return bn::nullopt;
    }

    void _open_palette()
    {
        _stage = TitleStage::PALETTE;
        _choice_timer = 0;
        _refresh_choice();
    }

    [[nodiscard]] SceneId _continue_run()
    {
        engine::RunManager& run = _ctx.run;
        engine::SaveManager::instance().restore_run(run);
        _ctx.palette = run.palette;
        _ctx.nav.reset_to_player_floor();
        _ctx.map_toast.clear();
        bn::ostringstream stream(_ctx.map_toast);
        stream << "Reprise : Acte " << int(run.current_act) << " - Étage " << int(run.current_floor);
        return SceneId::MAP;
    }

    void _refresh_menu()
    {
        constexpr bn::string_view entries[] = { "Continuer", "Nouvelle partie" };

        for(int index = 0; index < 2; ++index)
        {
            bool selected = index == _menu_index;
            TextLine line;
            line.append(selected ? "► " : "  ");
            line.append(entries[index]);
            _menu[index].set(0, MENU_Y + index * MENU_SPACING, line, Align::CENTER,
                             selected ? TextColor::GOLD : TextColor::WHITE);
        }
    }

    void _close_menu()
    {
        _menu[0].clear();
        _menu[1].clear();
    }

    void _animate_logo()
    {
        // Letters drop in one after the other with a small bounce, then float gently.
        for(int index = 0; index < _logo.size(); ++index)
        {
            int local = _timer - index * 3;
            bn::fixed y;

            if(local <= 0)
            {
                y = -120;
            }
            else if(local < 18)
            {
                bn::fixed t = bn::fixed(local) / 18;
                y = lerp(-120, LOGO_Y + 4, ease_out(t));
            }
            else if(local < 26)
            {
                y = LOGO_Y + 4 - bn::fixed(local - 18) / 2;
            }
            else
            {
                y = LOGO_Y + wave(_timer + index * 5, 90, 1);
            }

            _logo[index].set_position(_logo_x[index], y);
        }
    }

    void _refresh_choice()
    {
        const engine::PaletteDef& def = _palettes[_selected];
        TextLine line;
        line.append("◄ ");
        line.append(bn::string_view(def.name.data(), def.name.size()));
        line.append(" ►");
        _variant.set(0, 46, line, Align::CENTER, TextColor::GOLD);

        bn::vector<TextLine, 2> lines;
        wrap_text(bn::string_view(def.description.data(), def.description.size()), 228, lines);
        _description.set(0, 58, lines.empty() ? bn::string_view() : bn::string_view(lines[0]), Align::CENTER,
                         TextColor::WHITE);
        _hint.set(0, 72, "A : commencer   B : retour", Align::CENTER, TextColor::GRAY);
        _credits.clear();

        bn::sprite_palette_ptr palette = _hero.palette();
        bn::fixed intensity = palette_variant_intensity(def.variant);
        palette.set_fade(palette_variant_color(def.variant), intensity);
    }
};

}

bn::unique_ptr<Scene> make_title_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new TitleScene(context));
}

}
