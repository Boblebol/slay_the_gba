#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sstream.h"

#include "bn_regular_bg_items_bg_gameover.h"
#include "bn_regular_bg_items_bg_victory.h"
#include "bn_sprite_items_fx_burst.h"

#include "engine/run_end.h"

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

// End of a run (#11 defeat, #12 victory): the summary of the run on the left, the hero on the right, then the menu.
constexpr int TITLE_Y = -70;
constexpr int SUBTITLE_Y = -57;
constexpr int LINE_X = -110;
constexpr int LINE_Y = -44;
constexpr int LINE_STEP = 12;
constexpr int LINE_COUNT = 7;
constexpr int HERO_X = 80;
constexpr int HERO_Y = -4;
constexpr int ITEM_X = -66;
constexpr int ITEM_STEP = 12;
constexpr int ITEM_BOTTOM_Y = 68; // y of the last entry of the menu
constexpr int SUBTITLE_MAX_WIDTH = 228;
// Frames before the menu answers, so a button still held from the fight does not skip the screen.
constexpr int INPUT_DELAY = 40;

// h:mm:ss
void write_time(bn::ostringstream& stream, uint32_t seconds)
{
    int hours = int(seconds / 3600);
    int minutes = int(seconds / 60 % 60);
    int secs = int(seconds % 60);
    stream << hours << ':';

    if(minutes < 10)
    {
        stream << '0';
    }

    stream << minutes << ':';

    if(secs < 10)
    {
        stream << '0';
    }

    stream << secs;
}

class EndScene final : public Scene
{
public:
    EndScene(GameContext& context, bool victory) :
        _ctx(context),
        _bg((victory ? bn::regular_bg_items::bg_victory : bn::regular_bg_items::bg_gameover)
                    .create_bg(BG_ORIGIN_X, BG_ORIGIN_Y)),
        _hero(hero_battler().create_sprite(HERO_X, HERO_Y)),
        _menu(engine::make_end_menu(victory)),
        _victory(victory)
    {
        _hero.set_z_order(Z_ACTOR);
        bn::sprite_palette_ptr palette = _hero.palette();

        if(victory)
        {
            palette.set_fade(palette_variant_color(context.palette), palette_variant_intensity(context.palette));
        }
        else
        {
            palette.set_fade(bn::color(4, 2, 8), bn::fixed(0.6));
        }

        play_music(victory ? Music::VICTORY : Music::GAME_OVER);

        // The run is over: the profile gains its experience and statistics, the saved run is erased.
        const engine::RunRecord record = engine::record_run_end(context.run, victory);
        const engine::RunSummary& summary = record.summary;

        _title.set(0, TITLE_Y, victory ? "VICTOIRE TOTALE !" : "DÉFAITE DANS LA TOUR", Align::CENTER,
                   victory ? TextColor::GOLD : TextColor::RED);
        _set_subtitle(summary);
        _set_lines(record);
        _refresh_menu();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_timer;
        _fx.update();

        if(_victory)
        {
            // Victory hops, with sparks around the hero.
            bn::fixed t = bn::fixed(_timer % 40) / 40;
            _hero.set_y(HERO_Y - hop(t, 10));

            if(_timer % 30 == 0)
            {
                int spark = _timer / 30;
                _fx.spawn(bn::sprite_items::fx_burst, HERO_X - 24 + (spark * 37) % 48, HERO_Y - 28 + (spark * 23) % 40,
                          FX_BUFF, 4, 4);
            }
        }
        else
        {
            _hero.set_y(HERO_Y + 6 + wave(_timer, 120, 1));
        }

        if(_timer <= INPUT_DELAY)
        {
            return bn::nullopt;
        }

        if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            engine::move_end_cursor(_menu, bn::keypad::down_pressed() ? 1 : -1);
            _refresh_menu();
        }
        else if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            play_sfx(Sfx::UI_CONFIRM);
            return _choose(_menu.items[_menu.cursor]);
        }

        return bn::nullopt;
    }

private:
    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _hero;
    engine::EndMenu _menu;
    FxPool _fx;
    Label _title;
    Label _subtitle;
    Label _lines[LINE_COUNT];
    Label _items[engine::END_MAX_ITEMS];
    int _timer = 0;
    bool _victory;

    void _set_subtitle(const engine::RunSummary& summary)
    {
        TextLine text;
        bn::ostringstream stream(text);
        // An empty std::string_view has a null pointer, which bn::string_view refuses.
        bn::string_view killer = summary.killer.empty() ? bn::string_view("") : bn::string_view(summary.killer.data(), summary.killer.size());

        switch(summary.cause)
        {
        case engine::RunEndCause::VICTORY:
            stream << "La Tour s'incline devant vous.";
            break;
        case engine::RunEndCause::COMBAT:
            if(summary.self_inflicted)
            {
                stream << "Achevé par vos propres effets";
            }
            else
            {
                stream << "Vaincu par " << killer;
            }
            break;
        case engine::RunEndCause::EVENT:
            stream << "Emporté par : " << killer;
            break;
        default:
            stream << "Vous avez abandonné l'ascension.";
            break;
        }

        // A name too long for the screen gives way to a plain sentence.
        if(text_width(text) > SUBTITLE_MAX_WIDTH)
        {
            text.clear();
            text.append("Votre ascension s'arrête ici...");
        }

        _subtitle.set(0, SUBTITLE_Y, text, Align::CENTER, TextColor::WHITE);
    }

    void _set_lines(const engine::RunRecord& record)
    {
        const engine::RunSummary& summary = record.summary;
        int index = 0;

        {
            TextLine text;
            bn::ostringstream stream(text);
            stream << "Étage " << int(summary.floors_climbed) << " (Acte " << int(summary.act) << ')';
            _lines[index++].set(LINE_X, LINE_Y, text, Align::LEFT, TextColor::WHITE);
        }
        {
            TextLine text;
            bn::ostringstream stream(text);
            stream << int(summary.deck_size) << " cartes, " << int(summary.upgraded_cards) << " améliorées";
            _lines[index++].set(LINE_X, LINE_Y + LINE_STEP, text, Align::LEFT, TextColor::WHITE);
        }
        {
            TextLine text;
            bn::ostringstream stream(text);
            stream << int(summary.relic_count) << " reliques  " << int(summary.gold) << " Or";
            _lines[index++].set(LINE_X, LINE_Y + 2 * LINE_STEP, text, Align::LEFT, TextColor::WHITE);
        }
        {
            TextLine text;
            bn::ostringstream stream(text);
            stream << "Temps ";
            write_time(stream, summary.play_seconds);
            _lines[index++].set(LINE_X, LINE_Y + 3 * LINE_STEP, text, Align::LEFT, TextColor::GRAY);
        }
        {
            TextLine text;
            bn::ostringstream stream(text);
            stream << "Score " << int(summary.score);
            _lines[index++].set(LINE_X, LINE_Y + 4 * LINE_STEP, text, Align::LEFT, TextColor::GOLD);
        }
        {
            TextLine text;
            bn::ostringstream stream(text);
            uint32_t gained = record.xp_after - record.xp_before;
            stream << '+' << int(gained) << " XP  Niveau " << int(engine::profile_level(record.xp_after));

            if(engine::profile_level(record.xp_after) > engine::profile_level(record.xp_before))
            {
                stream << " !";
            }

            _lines[index++].set(LINE_X, LINE_Y + 5 * LINE_STEP, text, Align::LEFT, TextColor::GREEN);
        }

        if(record.ascension_after > record.ascension_before)
        {
            TextLine text;
            bn::ostringstream stream(text);
            stream << "Ascension " << int(record.ascension_after) << " débloquée !";
            _lines[index++].set(LINE_X, LINE_Y + 6 * LINE_STEP, text, Align::LEFT, TextColor::GOLD);
        }
        else
        {
            TextLine text;
            bn::ostringstream stream(text);
            stream << int(engine::profile_level_progress(record.xp_after)) << '/' << int(engine::XP_PER_LEVEL)
                   << " XP vers le niveau " << int(engine::profile_level(record.xp_after) + 1);
            _lines[index++].set(LINE_X, LINE_Y + 6 * LINE_STEP, text, Align::LEFT, TextColor::GRAY);
        }
    }

    void _refresh_menu()
    {
        int first_y = ITEM_BOTTOM_Y - (_menu.count - 1) * ITEM_STEP;

        for(int index = 0; index < engine::END_MAX_ITEMS; ++index)
        {
            if(index >= _menu.count)
            {
                _items[index].clear();
                continue;
            }

            bool focused = index == _menu.cursor;
            TextLine text;
            text.append(focused ? "► " : "  ");
            text.append(engine::end_label(_menu.items[index]));
            _items[index].set(ITEM_X, first_y + index * ITEM_STEP, text, Align::LEFT,
                              focused ? TextColor::GOLD : TextColor::WHITE);
        }
    }

    [[nodiscard]] SceneId _choose(engine::EndItem item)
    {
        switch(item)
        {
        case engine::EndItem::CREDITS:
            return SceneId::CREDITS;
        case engine::EndItem::NEW_RUN:
        {
            // Same hero, colour and Ascension; a new seed. The map writes the first save of the run.
            engine::RunManager& run = _ctx.run;
            uint8_t ascension = run.ascension_level;
            _ctx.combat_resume = false;
            _ctx.pause_end_turn = false;
            run.start_run(current_hero(), ascension, _ctx.palette, 12345u + unsigned(_ctx.frame) * 2654435761u);
            _ctx.nav.reset_to_player_floor();
            _ctx.map_toast.clear();
            _ctx.map_toast.append(hero_zone_name());
            return SceneId::MAP;
        }
        default:
            _ctx.title_menu = true;
            return SceneId::TITLE;
        }
    }
};

}

bn::unique_ptr<Scene> make_end_scene(GameContext& context, bool victory)
{
    return bn::unique_ptr<Scene>(new EndScene(context, victory));
}

}
