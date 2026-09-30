#include "gba/audio.h"

#include "bn_fixed.h"
#include "bn_sound_item.h"

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
