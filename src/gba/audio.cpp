#include "gba/audio.h"

#include "bn_fixed.h"
#include "bn_music.h"
#include "bn_music_item.h"
#include "bn_sound_item.h"

#include "bn_music_items.h"
#include "bn_sound_items.h"

#include "engine/save_system.h"

namespace gba
{

namespace
{

const bn::sound_item& sfx_item(Sfx sfx)
{
    switch(sfx)
    {
    case Sfx::UI_CURSOR:
        return bn::sound_items::sfx_ui_cursor;
    case Sfx::UI_CONFIRM:
        return bn::sound_items::sfx_ui_confirm;
    case Sfx::UI_CANCEL:
        return bn::sound_items::sfx_ui_cancel;
    case Sfx::UI_OPEN:
        return bn::sound_items::sfx_ui_open;
    case Sfx::CARD_PLAY:
        return bn::sound_items::sfx_card_play;
    case Sfx::HIT_SLASH:
        return bn::sound_items::sfx_hit_slash;
    case Sfx::HIT_HEAVY:
        return bn::sound_items::sfx_hit_heavy;
    case Sfx::HIT_BLUNT:
        return bn::sound_items::sfx_hit_blunt;
    case Sfx::BLOCK:
        return bn::sound_items::sfx_block;
    case Sfx::GOLD:
        return bn::sound_items::sfx_gold;
    case Sfx::RELIC:
        return bn::sound_items::sfx_relic;
    case Sfx::HEAL:
        return bn::sound_items::sfx_heal;
    case Sfx::DISCARD:
        return bn::sound_items::sfx_discard;
    case Sfx::ENEMY_DEATH:
        return bn::sound_items::sfx_enemy_death;
    case Sfx::BUFF:
        return bn::sound_items::sfx_buff;
    case Sfx::DEBUFF:
        return bn::sound_items::sfx_debuff;
    case Sfx::TURN_START:
        return bn::sound_items::sfx_turn_start;
    default:
        return bn::sound_items::sfx_error;
    }
}

const bn::music_item& music_item(Music music)
{
    switch(music)
    {
    case Music::MAP:
        return bn::music_items::bgm_map;
    case Music::COMBAT:
        return bn::music_items::bgm_combat;
    case Music::ELITE:
        return bn::music_items::bgm_elite;
    case Music::BOSS:
        return bn::music_items::bgm_boss;
    case Music::VICTORY:
        return bn::music_items::bgm_victory;
    case Music::GAME_OVER:
        return bn::music_items::bgm_game_over;
    default:
        return bn::music_items::bgm_title;
    }
}

// Music sits below the sound effects so impacts stay readable.
constexpr bn::fixed MUSIC_GAIN = bn::fixed(0.55);
constexpr bn::fixed FADE_STEP = bn::fixed(1) / 24;

Music current_music = Music::NONE;
Music requested_music = Music::NONE;
bn::fixed fade_level = 0;   // 0 silent -> 1 full volume

void apply_music_volume()
{
    int level = engine::SaveManager::instance().settings().music_volume;
    bn::music::set_volume(fade_level * MUSIC_GAIN * level / 10);
}

}

void play_music(Music music)
{
    requested_music = music;
}

void update_audio()
{
    if(requested_music != current_music)
    {
        if(current_music != Music::NONE && fade_level > 0)
        {
            fade_level = bn::max(bn::fixed(0), fade_level - FADE_STEP);
            apply_music_volume();
            return;
        }

        if(current_music != Music::NONE && bn::music::playing())
        {
            bn::music::stop();
        }

        current_music = requested_music;
        fade_level = 0;

        if(current_music != Music::NONE)
        {
            music_item(current_music).play(0, true);
        }
    }

    if(current_music != Music::NONE && bn::music::playing())
    {
        fade_level = bn::min(bn::fixed(1), fade_level + FADE_STEP);
        apply_music_volume();
    }
}

void play_sfx(Sfx sfx)
{
    int level = engine::SaveManager::instance().settings().sfx_volume;

    if(level > 0)
    {
        sfx_item(sfx).play(bn::fixed(level) / 10);
    }
}

}
