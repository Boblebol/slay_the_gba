#include "gba/scene_factories.h"

#include <span>

#include "bn_blending.h"
#include "bn_bg_palette_ptr.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sstream.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bg_title.h"
#include "bn_sprite_items_title_glow.h"
#include "bn_sprite_items_title_mist.h"
#include "common_variable_16x16_sprite_font.h"
#include "bn_sprite_items_title_logo.h"

#include "engine/run.h"
#include "engine/save_system.h"
#include "engine/title_anim.h"
#include "engine/title_edition.h"

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
constexpr int LOGO_PIECES = 5;      // 32x32 frames of title_logo, left to right (scripts/build_identity.py)
constexpr int LOGO_PIECE_W = 32;
constexpr int MENU_Y = 28;
constexpr int MENU_SPACING = 11;
constexpr int MENU_ENTRIES = 4;

// Animated backdrop (#7): two bands of clouds / mist scrolling at different speeds (parallax) over the key art, and a
// halo pulsing on the Spire. Positions are in screen pixels from the top left corner of the key art.
constexpr int CLOUD_BAND_Y = -34;
constexpr int MIST_BAND_Y = 30;
constexpr int CLOUD_SPEED = 9;  // 64ths of a pixel per frame
constexpr int MIST_SPEED = 26;
constexpr int GLOW_X = -13;
constexpr int GLOW_Y = -40;
constexpr bn::fixed MIST_ALPHA = bn::fixed(0.45);

#define SLAY_STRINGIFY_IMPL(token) #token
#define SLAY_STRINGIFY(token) SLAY_STRINGIFY_IMPL(token)

#ifdef SLAY_VERSION
constexpr const char* GAME_VERSION = SLAY_STRINGIFY(SLAY_VERSION);
#else
constexpr const char* GAME_VERSION = "";
#endif

[[nodiscard]] TextColor accent_color(engine::title_edition::Accent accent)
{
    using Accent = engine::title_edition::Accent;

    switch(accent)
    {
    case Accent::GREEN:
        return TextColor::GREEN;
    case Accent::BLUE:
        return TextColor::BLUE;
    case Accent::GOLD:
        return TextColor::GOLD;
    case Accent::PURPLE:
        return TextColor::PURPLE;
    default:
        return TextColor::RED;
    }
}

enum class MenuEntry : uint8_t
{
    CONTINUE,   // only when a run is saved
    NEW_RUN,
    COMPENDIUM,
    STATISTICS
};

enum class TitleStage : uint8_t
{
    PRESS_START,
    MENU,       // "Continuer" (when a run is saved), "Nouvelle partie", "Compendium", "Statistiques"
    PALETTE,
    ASCENSION   // level choice, once the tint is picked and a level above 0 is unlocked
};

class TitleScene final : public Scene
{
public:
    explicit TitleScene(GameContext& context) :
        _ctx(context),
        _bg(bn::regular_bg_items::bg_title.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y)),
        _hero(hero_battler().create_sprite(0, 200)),
        _glow(bn::sprite_items::title_glow.create_sprite(GLOW_X, GLOW_Y))
    {
        bn::blending::set_transparency_alpha(MIST_ALPHA);
        _glow.set_z_order(Z_BACK);

        // Edition identity (#3): the key art and the halo fade towards the colour of the edition, its name sits under
        // the logo and the version tag in the corner.
        const engine::title_edition::Theme theme = engine::title_edition::theme(current_hero());
        const bn::color tint = bn::color(theme.tint_bgr555);
        bn::bg_palette_ptr bg_palette = _bg.palette();
        bg_palette.set_fade(tint, bn::fixed(theme.tint_strength) / 32);
        bn::sprite_palette_ptr glow_palette = _glow.palette();
        glow_palette.set_fade(tint, bn::fixed(0.5));

        for(int tile = 0; tile < engine::title_anim::MIST_TILES; ++tile)
        {
            _clouds.push_back(bn::sprite_items::title_mist.create_sprite(0, CLOUD_BAND_Y, 0));
            _mist.push_back(bn::sprite_items::title_mist.create_sprite(0, MIST_BAND_Y, 1));
        }

        for(bn::sprite_ptr& sprite : _clouds)
        {
            sprite.set_blending_enabled(true);
            sprite.set_z_order(Z_BACK);
        }

        for(bn::sprite_ptr& sprite : _mist)
        {
            sprite.set_blending_enabled(true);
            sprite.set_z_order(Z_BACK);
        }

        _animate_backdrop();
        for(int piece = 0; piece < LOGO_PIECES; ++piece)
        {
            int x = (piece - LOGO_PIECES / 2) * LOGO_PIECE_W;
            _logo.push_back(bn::sprite_items::title_logo.create_sprite(x, -120, piece));
            _logo.back().set_z_order(Z_TEXT);
            _logo_x.push_back(x);
        }

        // The edition name already carries the hero's name, so it replaces the separate hero name label: the OAM
        // (128 sprites) has no room left for both next to the zone, the prompt and the credits.
        _hero_name.set(0, -42, bn::string_view(theme.edition.data(), theme.edition.size()), Align::CENTER,
                       accent_color(theme.accent));
        char version[16];
        int version_length = engine::title_edition::format_version(GAME_VERSION, version, 15);
        _version.set(HALF_W - 3, 62, bn::string_view(version, version_length), Align::RIGHT, TextColor::GRAY);
        _hero_name.set_visible(false);
        _zone.set(0, -31, hero_zone_name(), Align::CENTER, TextColor::GRAY);
        _zone.set_visible(false);
        _press.set(0, 50, "Appuyez sur START", Align::CENTER, TextColor::WHITE);
        _press.set_visible(false);
        _options_hint.set(0, 62, "SELECT : options", Align::CENTER, TextColor::GRAY);
        _credits.set(0, 72, "Fan-game - Slay the Spire (c) Mega Crit", Align::CENTER, TextColor::GRAY);
        _hero.set_z_order(Z_ACTOR);

        _palettes = engine::get_character_palettes(current_hero());
        play_music(Music::TITLE);

        // Back from the compendium or the statistics: the logo is already in place and the menu is open.
        if(_ctx.title_menu)
        {
            _ctx.title_menu = false;
            _timer = 60;
            _hero_name.set_visible(true);
            _zone.set_visible(true);
            _press.clear();
            _options_hint.clear();
            _open_menu();
        }
    }

    ~TitleScene() final
    {
        bn::blending::set_transparency_alpha(1);
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_timer;
        _animate_backdrop();
        _animate_logo();

        switch(_stage)
        {
        case TitleStage::PRESS_START:
            return _update_press_start();
        case TitleStage::MENU:
            return _update_menu();
        case TitleStage::PALETTE:
            return _update_palette();
        default:
            return _update_ascension();
        }
    }

private:
    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _hero;
    bn::sprite_ptr _glow;
    bn::vector<bn::sprite_ptr, engine::title_anim::MIST_TILES> _clouds;
    bn::vector<bn::sprite_ptr, engine::title_anim::MIST_TILES> _mist;
    int _glow_frame = -1;
    bn::vector<bn::sprite_ptr, 12> _logo;
    bn::vector<int, 12> _logo_x;
    Label _version;
    Label _hero_name;
    Label _zone;
    Label _press;
    Label _credits;
    Label _options_hint;
    Label _variant;
    Label _description;
    Label _description_lines[3];
    Label _hint;
    std::span<const engine::PaletteDef> _palettes;
    int _timer = 0;
    int _choice_timer = 0;
    int _selected = 0;
    int _menu_index = 0;
    int _ascension = 0;
    int _max_ascension = 0;
    TitleStage _stage = TitleStage::PRESS_START;
    bn::vector<MenuEntry, MENU_ENTRIES> _entries;
    Label _menu[MENU_ENTRIES];

    [[nodiscard]] bn::optional<SceneId> _update_press_start()
    {
        if(_timer > 30)
        {
            _hero_name.set_visible(true);
            _zone.set_visible(true);
            _press.set_visible(engine::title_anim::prompt_visible(_timer));
        }

        if(_timer > 20 && bn::keypad::select_pressed())
        {
            play_sfx(Sfx::UI_OPEN);
            _ctx.options_return = SceneId::TITLE;
            return SceneId::OPTIONS;
        }

        if(_timer > 20 && (bn::keypad::start_pressed() || bn::keypad::a_pressed()))
        {
            play_sfx(Sfx::UI_CONFIRM);
            _press.clear();
            _options_hint.clear();
            _open_menu();
        }

        return bn::nullopt;
    }

    [[nodiscard]] bn::optional<SceneId> _update_menu()
    {
        int count = _entries.size();

        if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _menu_index = (_menu_index + (bn::keypad::down_pressed() ? 1 : count - 1)) % count;
            _refresh_menu();
        }
        else if(bn::keypad::b_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _close_menu();
            _back_to_press_start();
        }
        else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            play_sfx(Sfx::UI_CONFIRM);
            MenuEntry entry = _entries[_menu_index];

            switch(entry)
            {
            case MenuEntry::CONTINUE:
                _close_menu();
                return _continue_run();
            case MenuEntry::COMPENDIUM:
                return SceneId::COMPENDIUM;
            case MenuEntry::STATISTICS:
                return SceneId::STATISTICS;
            default:
                _close_menu();
                _open_palette();
                break;
            }
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
            _hero.set_y(200);
            _variant.clear();
            _description.clear();
            _hint.clear();
            _open_menu();
        }
        else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            play_sfx(Sfx::UI_CONFIRM);
            _max_ascension = engine::SaveManager::instance().unlocked_ascension(current_hero());

            if(_max_ascension == 0)
            {
                return _start_run(0);
            }

            _stage = TitleStage::ASCENSION;
            _ascension = _max_ascension; // most recent unlocked level by default
            _refresh_ascension();
        }

        return bn::nullopt;
    }

    [[nodiscard]] bn::optional<SceneId> _update_ascension()
    {
        int count = _max_ascension + 1;

        if(bn::keypad::left_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _ascension = (_ascension + count - 1) % count;
            _refresh_ascension();
        }
        else if(bn::keypad::right_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _ascension = (_ascension + 1) % count;
            _refresh_ascension();
        }
        else if(bn::keypad::b_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _clear_ascension_lines();
            _choice_timer = 0;
            _stage = TitleStage::PALETTE;
            _refresh_choice();
        }
        else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            play_sfx(Sfx::UI_CONFIRM);
            return _start_run(_ascension);
        }

        return bn::nullopt;
    }

    [[nodiscard]] SceneId _start_run(int ascension)
    {
        engine::PaletteVariant variant = _palettes[_selected].variant;
        _ctx.palette = variant;
        _ctx.run.start_run(current_hero(), uint8_t(ascension), variant, 12345u + unsigned(_ctx.frame) * 2654435761u);
        // Every run opens on Neow, whose relic is taken before the first map (the Ancient scene then opens the map
        // on the hero's zone).
        _ctx.run.meet_ancient(engine::AncientId::NEOW);
        return SceneId::ANCIENT;
    }

    void _refresh_ascension()
    {
        std::span<const engine::AscensionModifier> modifiers = engine::RunManager::get_ascension_modifiers();
        const engine::AscensionModifier& modifier = modifiers[_ascension];

        _hero.set_y(200); // frees the room for the description

        TextLine line;
        bn::ostringstream stream(line);
        stream << "◄ A" << _ascension << " - ";
        line.append(bn::string_view(modifier.title.data(), modifier.title.size()));
        line.append(" ►");
        _variant.set(0, 30, line, Align::CENTER, TextColor::GOLD);

        bn::vector<TextLine, 3> lines;
        wrap_text(bn::string_view(modifier.description.data(), modifier.description.size()), 228, lines);

        for(int index = 0; index < 3; ++index)
        {
            if(index < lines.size())
            {
                _description_lines[index].set(0, 44 + index * 10, bn::string_view(lines[index]), Align::CENTER,
                                              TextColor::WHITE);
            }
            else
            {
                _description_lines[index].clear();
            }
        }

        _hint.set(0, 74, "A : commencer   B : retour", Align::CENTER, TextColor::GRAY);
    }

    void _clear_ascension_lines()
    {
        for(Label& label : _description_lines)
        {
            label.clear();
        }
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

        // Saved in front of an Ancient whose relic was not taken yet: the Ancient is met again.
        if(run.phase == engine::RunPhase::ANCIENT_BLESSING)
        {
            return SceneId::ANCIENT;
        }

        bn::ostringstream stream(_ctx.map_toast);
        stream << "Reprise : Acte " << int(run.current_act) << " - Étage " << int(run.current_floor);
        return SceneId::MAP;
    }

    void _open_menu()
    {
        _close_menu();
        _entries.clear();

        if(engine::SaveManager::instance().has_active_run())
        {
            _entries.push_back(MenuEntry::CONTINUE);
        }

        _entries.push_back(MenuEntry::NEW_RUN);
        _entries.push_back(MenuEntry::COMPENDIUM);
        _entries.push_back(MenuEntry::STATISTICS);
        _stage = TitleStage::MENU;
        _menu_index = 0;
        _credits.set(0, 72, "Fan-game - Slay the Spire (c) Mega Crit", Align::CENTER, TextColor::GRAY);
        _refresh_menu();
    }

    void _back_to_press_start()
    {
        _stage = TitleStage::PRESS_START;
        _press.set(0, 50, "Appuyez sur START", Align::CENTER, TextColor::WHITE);
    }

    void _refresh_menu()
    {
        for(int index = 0; index < _entries.size(); ++index)
        {
            bool selected = index == _menu_index;
            TextLine line;
            line.append(selected ? "► " : "  ");

            switch(_entries[index])
            {
            case MenuEntry::CONTINUE:
                line.append("Continuer");
                break;
            case MenuEntry::NEW_RUN:
                line.append("Nouvelle partie");
                break;
            case MenuEntry::COMPENDIUM:
                line.append("Compendium");
                break;
            default:
                line.append("Statistiques");
                break;
            }

            _menu[index].set(0, MENU_Y + index * MENU_SPACING, line, Align::CENTER,
                             selected ? TextColor::GOLD : TextColor::WHITE);
        }
    }

    void _close_menu()
    {
        for(Label& label : _menu)
        {
            label.clear();
        }
    }

    void _animate_backdrop()
    {
        namespace anim = engine::title_anim;
        _scroll_band(_clouds, anim::mist_scroll(_timer, CLOUD_SPEED), CLOUD_BAND_Y);
        _scroll_band(_mist, anim::mist_scroll(_timer, MIST_SPEED), MIST_BAND_Y);

        int glow = anim::glow_frame(_timer);

        if(glow != _glow_frame)
        {
            _glow_frame = glow;
            _glow.set_item(bn::sprite_items::title_glow, glow);
        }
    }

    static void _scroll_band(bn::vector<bn::sprite_ptr, engine::title_anim::MIST_TILES>& band, int scroll, int y)
    {
        for(int tile = 0; tile < band.size(); ++tile)
        {
            band[tile].set_position(engine::title_anim::mist_tile_x(tile, scroll) - HALF_W + engine::title_anim::MIST_TILE_W / 2, y);
        }
    }

    void _animate_logo()
    {
        // Logo pieces drop in one after the other with a small bounce, then float gently.
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
                y = LOGO_Y + wave(_timer, 90, 1); // the pieces float together: a letter never splits
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
