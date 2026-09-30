#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sstream.h"

#include "bn_regular_bg_items_bg_gameover.h"
#include "bn_regular_bg_items_bg_victory.h"

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

class EndScene final : public Scene
{
public:
    EndScene(GameContext& context, bool victory) :
        _ctx(context),
        _bg((victory ? bn::regular_bg_items::bg_victory : bn::regular_bg_items::bg_gameover)
                    .create_bg(BG_ORIGIN_X, BG_ORIGIN_Y)),
        _hero(hero_battler().create_sprite(0, 12)),
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

        _title.set(0, -64, victory ? "VICTOIRE TOTALE !" : "DÉFAITE DANS LA TOUR", Align::CENTER,
                   victory ? TextColor::GOLD : TextColor::RED);
        _subtitle.set(0, -50, victory ? "La Tour s'incline devant vous." : "Votre ascension s'arrête ici...",
                      Align::CENTER, TextColor::WHITE);

        const engine::RunManager& run = context.run;
        engine::SaveManager& save = engine::SaveManager::instance();

        // The run is over: remove it from the cartridge and update the profile statistics.
        if(victory)
        {
            save.record_victory(run.character, run.ascension_level);
        }
        else
        {
            save.record_defeat();
        }

        TextLine stats;
        bn::ostringstream stream(stats);
        stream << "Acte " << int(run.current_act) << "  Étage " << int(run.current_floor) << "  "
               << int(run.master_deck.size()) << " cartes  " << run.gold << " Or";
        _stats.set(0, 50, stats, Align::CENTER, TextColor::GRAY);
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_timer;

        if(_victory)
        {
            // Victory hops.
            bn::fixed t = bn::fixed(_timer % 40) / 40;
            _hero.set_y(12 - hop(t, 10));
        }
        else
        {
            _hero.set_y(18 + wave(_timer, 120, 1));
        }

        _press.set(0, 66, "START : nouvelle partie", Align::CENTER, TextColor::WHITE);
        _press.set_visible(_timer > 40 && (_timer / 24) % 2 == 0);

        if(_timer > 40 && bn::keypad::start_pressed())
        {
            play_sfx(Sfx::UI_CONFIRM);
            return SceneId::TITLE;
        }

        return bn::nullopt;
    }

private:
    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _hero;
    Label _title;
    Label _subtitle;
    Label _stats;
    Label _press;
    int _timer = 0;
    bool _victory;
};

}

bn::unique_ptr<Scene> make_end_scene(GameContext& context, bool victory)
{
    return bn::unique_ptr<Scene>(new EndScene(context, victory));
}

}
