#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_sstream.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bg_event_mystic_altar.h"

#include "engine/ancient.h"
#include "engine/save_system.h"

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

// The Ancient stands in the light of the altar, its offer is listed on the left, what the relic under the cursor
// does fills the panel at the bottom.
constexpr int PORTRAIT_X = 84;
constexpr int PORTRAIT_Y = -42;
constexpr int KIND_Y = -2; // under the portrait: blessing or curse of Neow
constexpr int TEXT_LEFT = -114;
constexpr int ROW_Y = -26;
constexpr int ROW_STEP = 12;
constexpr int PANEL_TEXT_Y = 18;
constexpr int PANEL_LINE_HEIGHT = 11;
constexpr int PANEL_TEXT_WIDTH = 224;
constexpr int PANEL_LINES = 4;
constexpr int LEAVE_FRAMES = 50;
constexpr int MAX_ROWS = int(engine::MAX_ANCIENT_OFFERS);

// Neow greets the hero in the panel before showing the offer, at the start of the run.
constexpr bn::string_view NEOW_GREETING = "« Salutations, petite flamme... Atteins le sommet, et je te relèverai "
                                          "encore. Prends un présent pour la route. »";

class AncientScene final : public Scene
{
public:
    explicit AncientScene(GameContext& context) :
        _ctx(context),
        _ancient(engine::get_ancient(context.run.current_ancient < engine::AncientId::COUNT ?
                                         context.run.current_ancient : engine::AncientId::NEOW)),
        _bg(bn::regular_bg_items::bg_event_mystic_altar.create_bg(BG_ORIGIN_X, BG_ORIGIN_Y)),
        _portrait(ancient_portrait(_ancient.id).create_sprite(PORTRAIT_X, PORTRAIT_Y))
    {
        _portrait.set_z_order(Z_ACTOR);
        play_music(Music::MAP);

        // Checkpoint: "Continuer" on the title screen comes back to this Ancient and its offer.
        if(_ctx.run.phase == engine::RunPhase::ANCIENT_BLESSING)
        {
            engine::SaveManager::instance().autosave_run(_ctx.run);
        }

        _name.set(TEXT_LEFT, -70, bn::string_view(_ancient.name.data(), _ancient.name.size()), Align::LEFT,
                  TextColor::GOLD);
        _epithet.set(TEXT_LEFT, -58, bn::string_view(_ancient.epithet.data(), _ancient.epithet.size()), Align::LEFT,
                     TextColor::GRAY);
        _greeting = _ancient.id == engine::AncientId::NEOW && !_ctx.run.ancient_offers.empty();
        _refresh();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        engine::RunManager& run = _ctx.run;
        ++_timer;
        _popups.update();
        _portrait.set_y(PORTRAIT_Y + wave(_timer, 120, 2));

        if(_leave_timer > 0)
        {
            if(++_leave_timer > LEAVE_FRAMES)
            {
                return _leave();
            }

            return bn::nullopt;
        }

        if(_timer < 16)
        {
            return bn::nullopt;
        }

        int count = int(run.ancient_offers.size());

        if(_greeting)
        {
            if(bn::keypad::a_pressed())
            {
                play_sfx(Sfx::UI_CONFIRM);
                _greeting = false;
                _refresh();
            }

            return bn::nullopt;
        }

        if(count == 0)
        {
            // Nothing to offer (no room for one more relic): the act starts without it.
            if(bn::keypad::a_pressed())
            {
                play_sfx(Sfx::UI_CONFIRM);
                run.leave_ancient();
                return _leave();
            }

            return bn::nullopt;
        }

        if(bn::keypad::up_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _cursor = (_cursor + count - 1) % count;
            _refresh();
        }
        else if(bn::keypad::down_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            _cursor = (_cursor + 1) % count;
            _refresh();
        }
        else if(bn::keypad::a_pressed())
        {
            if(run.choose_ancient_relic(_cursor))
            {
                play_sfx(Sfx::RELIC);
                _popups.spawn(-40, -40, "Relique obtenue !", TextColor::GREEN);
                _leave_timer = 1;
                _hint.clear();
            }
            else
            {
                play_sfx(Sfx::ERROR);
            }
        }

        _bob();
        return bn::nullopt;
    }

private:
    GameContext& _ctx;
    const engine::Ancient& _ancient;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _portrait;
    Label _name;
    Label _epithet;
    Label _prompt;
    Label _hint;
    Label _kind;
    bn::vector<Label, MAX_ROWS> _rows;
    bn::vector<Label, PANEL_LINES> _desc;
    PopupPool _popups;
    int _timer = 0;
    int _cursor = 0;
    int _leave_timer = 0;
    bool _greeting = false;

    void _refresh()
    {
        const engine::RunManager& run = _ctx.run;
        int count = int(run.ancient_offers.size());
        _rows.clear();
        _desc.clear();

        if(_greeting)
        {
            _prompt.clear();
            _kind.clear();
            bn::vector<TextLine, PANEL_LINES> lines;
            wrap_text(NEOW_GREETING, PANEL_TEXT_WIDTH, lines);

            for(int index = 0; index < lines.size(); ++index)
            {
                _desc.emplace_back().set(0, PANEL_TEXT_Y + index * PANEL_LINE_HEIGHT, lines[index], Align::CENTER,
                                         TextColor::WHITE);
            }

            _hint.set(0, 68, "A Continuer", Align::CENTER, TextColor::GRAY);
            return;
        }

        if(count == 0)
        {
            _prompt.set(TEXT_LEFT, -40, "n'a rien à vous offrir.", Align::LEFT, TextColor::WHITE);
            _kind.clear();
            _hint.set(0, 68, "A Continuer", Align::CENTER, TextColor::GRAY);
            return;
        }

        _prompt.set(TEXT_LEFT, -40, "Choisissez une relique", Align::LEFT, TextColor::WHITE);
        if(_cursor >= count)
        {
            _cursor = count - 1;
        }

        for(int index = 0; index < count; ++index)
        {
            const engine::AncientRelic& relic = run.ancient_offer(index);
            bool focused = index == _cursor;
            TextLine row;
            row.append(focused ? "► " : "  ");
            row.append(bn::string_view(relic.name.data(), relic.name.size()));
            _rows.emplace_back().set(TEXT_LEFT, ROW_Y + index * ROW_STEP, row, Align::LEFT,
                                     focused ? TextColor::GOLD : TextColor::WHITE);
        }

        // Neow's slots say whether the relic is a blessing or comes with a price.
        const engine::AncientSlot& slot = _ancient.slots[run.ancient_offers[_cursor].slot];
        if(slot.group_name == "blessing")
        {
            _kind.set(PORTRAIT_X, KIND_Y, "Bénédiction", Align::CENTER, TextColor::BLUE);
        }
        else if(slot.group_name == "curse")
        {
            _kind.set(PORTRAIT_X, KIND_Y, "Avec un prix", Align::CENTER, TextColor::RED);
        }
        else
        {
            _kind.clear();
        }

        const engine::AncientRelic& selected = run.ancient_offer(_cursor);
        bn::vector<TextLine, PANEL_LINES> lines;
        wrap_text(bn::string_view(selected.description.data(), selected.description.size()), PANEL_TEXT_WIDTH, lines);

        for(int index = 0; index < lines.size(); ++index)
        {
            _desc.emplace_back().set(0, PANEL_TEXT_Y + index * PANEL_LINE_HEIGHT, lines[index], Align::CENTER,
                                     TextColor::WHITE);
        }

        _hint.set(0, 68, "▲▼ Choisir  A Prendre", Align::CENTER, TextColor::GRAY);
    }

    void _bob()
    {
        if(_cursor < _rows.size())
        {
            int offset = wave(_timer, 30, 1).round_integer();
            _rows[_cursor].move_to(TEXT_LEFT + 2 + offset, ROW_Y + _cursor * ROW_STEP);
        }
    }

    // The act starts on its map, unless the relic offers cards first or asks for cards of the deck.
    [[nodiscard]] SceneId _leave()
    {
        engine::RunManager& run = _ctx.run;
        _ctx.nav.reset_to_player_floor();
        _ctx.map_toast.clear();
        if(run.current_act <= 1)
        {
            // Neow at the start of the run: the map opens on the hero's zone.
            _ctx.map_toast.append(hero_zone_name());
        }
        else
        {
            bn::ostringstream stream(_ctx.map_toast);
            stream << "ACTE " << int(run.current_act) << " : l'ascension continue !";
        }

        if(run.phase == engine::RunPhase::REWARD_SCREEN)
        {
            return scene_after_deck_choice(_ctx, SceneId::REWARDS);
        }

        return scene_after_deck_choice(_ctx, SceneId::MAP);
    }
};

}

bn::unique_ptr<Scene> make_ancient_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new AncientScene(context));
}

}
