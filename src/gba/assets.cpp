#include "gba/assets.h"

#include "bn_regular_bg_items_bg_battle_defect.h"
#include "bn_regular_bg_items_bg_battle_ironclad.h"
#include "bn_regular_bg_items_bg_battle_necrobinder.h"
#include "bn_regular_bg_items_bg_battle_regent.h"
#include "bn_regular_bg_items_bg_battle_silent.h"
#include "bn_regular_bg_items_bg_event_abyssal_baths.h"
#include "bn_regular_bg_items_bg_event_battleworn_dummy.h"
#include "bn_regular_bg_items_bg_event_colossal_flower.h"
#include "bn_regular_bg_items_bg_event_mystic_altar.h"

#include "bn_sprite_items_boss_ceremonial_beast.h"
#include "bn_sprite_items_mon_cultist.h"
#include "bn_sprite_items_mon_gremlin_nob.h"
#include "bn_sprite_items_mon_jaw_worm.h"
#include "bn_sprite_items_mon_louse.h"

#include "bn_sprite_items_card_anger.h"
#include "bn_sprite_items_card_backflip.h"
#include "bn_sprite_items_card_ball_lightning.h"
#include "bn_sprite_items_card_bash.h"
#include "bn_sprite_items_card_beam_cell.h"
#include "bn_sprite_items_card_bodyguard.h"
#include "bn_sprite_items_card_celestial_might.h"
#include "bn_sprite_items_card_cleave.h"
#include "bn_sprite_items_card_cold_snap.h"
#include "bn_sprite_items_card_dagger_spray.h"
#include "bn_sprite_items_card_deadly_poison.h"
#include "bn_sprite_items_card_defend_defect.h"
#include "bn_sprite_items_card_defend_ironclad.h"
#include "bn_sprite_items_card_defend_necrobinder.h"
#include "bn_sprite_items_card_defend_regent.h"
#include "bn_sprite_items_card_defend_silent.h"
#include "bn_sprite_items_card_dualcast.h"
#include "bn_sprite_items_card_falling_star.h"
#include "bn_sprite_items_card_gather_light.h"
#include "bn_sprite_items_card_iron_wave.h"
#include "bn_sprite_items_card_leap.h"
#include "bn_sprite_items_card_metallicize.h"
#include "bn_sprite_items_card_neutralize.h"
#include "bn_sprite_items_card_poison_stab.h"
#include "bn_sprite_items_card_pommel_strike.h"
#include "bn_sprite_items_card_shrug_it_off.h"
#include "bn_sprite_items_card_slice.h"
#include "bn_sprite_items_card_solar_strike.h"
#include "bn_sprite_items_card_strike_defect.h"
#include "bn_sprite_items_card_strike_ironclad.h"
#include "bn_sprite_items_card_strike_necrobinder.h"
#include "bn_sprite_items_card_strike_regent.h"
#include "bn_sprite_items_card_strike_silent.h"
#include "bn_sprite_items_card_sucker_punch.h"
#include "bn_sprite_items_card_survivor.h"
#include "bn_sprite_items_card_taunt.h"
#include "bn_sprite_items_card_twin_strike.h"
#include "bn_sprite_items_card_unleash.h"
#include "bn_sprite_items_card_venerate.h"
#include "bn_sprite_items_card_zap.h"

#include "bn_sprite_items_hand_cards_defect.h"
#include "bn_sprite_items_hand_cards_ironclad.h"
#include "bn_sprite_items_hand_cards_necrobinder.h"
#include "bn_sprite_items_hand_cards_regent.h"
#include "bn_sprite_items_hand_cards_silent.h"
#include "bn_sprite_items_ui_energy_defect.h"
#include "bn_sprite_items_ui_energy_ironclad.h"
#include "bn_sprite_items_ui_energy_necrobinder.h"
#include "bn_sprite_items_ui_energy_regent.h"
#include "bn_sprite_items_ui_energy_silent.h"

#include "bn_sprite_items_map_cursor_defect.h"
#include "bn_sprite_items_map_cursor_ironclad.h"
#include "bn_sprite_items_map_cursor_necrobinder.h"
#include "bn_sprite_items_map_cursor_regent.h"
#include "bn_sprite_items_map_cursor_silent.h"
#include "bn_sprite_items_map_pawn_defect.h"
#include "bn_sprite_items_map_pawn_ironclad.h"
#include "bn_sprite_items_map_pawn_necrobinder.h"
#include "bn_sprite_items_map_pawn_regent.h"
#include "bn_sprite_items_map_pawn_silent.h"
#include "bn_sprite_items_hero_defect.h"
#include "bn_sprite_items_hero_ironclad.h"
#include "bn_sprite_items_hero_necrobinder.h"
#include "bn_sprite_items_hero_regent.h"
#include "bn_sprite_items_hero_silent.h"

#if defined(HERO_SILENT)
    #define HERO_SUFFIX silent
    #define HERO_ID engine::CharacterId::SILENT
    #define HERO_BACKDROP bn::color(1, 5, 2)
    #define HERO_ZONE "MARAIS DU VENIN"
    #define HERO_UPGRADE "Affûtage des dagues"
#elif defined(HERO_DEFECT)
    #define HERO_SUFFIX defect
    #define HERO_ID engine::CharacterId::DEFECT
    #define HERO_BACKDROP bn::color(1, 2, 7)
    #define HERO_ZONE "COMPLEXE ASTRAL"
    #define HERO_UPGRADE "Reprogrammation"
#elif defined(HERO_REGENT)
    #define HERO_SUFFIX regent
    #define HERO_ID engine::CharacterId::REGENT
    #define HERO_BACKDROP bn::color(4, 1, 5)
    #define HERO_ZONE "PALAIS SOUVERAIN"
    #define HERO_UPGRADE "Bénédiction royale"
#elif defined(HERO_NECROBINDER)
    #define HERO_SUFFIX necrobinder
    #define HERO_ID engine::CharacterId::NECROBINDER
    #define HERO_BACKDROP bn::color(3, 1, 4)
    #define HERO_ZONE "CRYPTE D'OUTRE-TOMBE"
    #define HERO_UPGRADE "Rituel de résurrection"
#else
    #define HERO_SUFFIX ironclad
    #define HERO_ID engine::CharacterId::IRONCLAD
    #define HERO_BACKDROP bn::color(6, 1, 2)
    #define HERO_ZONE "FORGES DU CARNAGE"
    #define HERO_UPGRADE "Forge infernale"
#endif

#define GBA_CONCAT_IMPL(a, b) a##b
#define GBA_CONCAT(a, b) GBA_CONCAT_IMPL(a, b)
#define HERO_ITEM(prefix) GBA_CONCAT(prefix, HERO_SUFFIX)

namespace gba
{

engine::CharacterId current_hero()
{
    return HERO_ID;
}

const bn::sprite_item& hero_battler()
{
    return bn::sprite_items::HERO_ITEM(hero_);
}

const bn::sprite_item& hero_map_pawn()
{
    return bn::sprite_items::HERO_ITEM(map_pawn_);
}

const bn::sprite_item& hero_map_cursor()
{
    return bn::sprite_items::HERO_ITEM(map_cursor_);
}

const bn::sprite_item& hero_hand_cards()
{
    return bn::sprite_items::HERO_ITEM(hand_cards_);
}

const bn::sprite_item& hero_energy_orb()
{
    return bn::sprite_items::HERO_ITEM(ui_energy_);
}

const bn::regular_bg_item& hero_battle_bg()
{
    return bn::regular_bg_items::HERO_ITEM(bg_battle_);
}

bn::color hero_backdrop_color()
{
    return HERO_BACKDROP;
}

bn::string_view hero_zone_name()
{
    return HERO_ZONE;
}

bn::string_view hero_campfire_upgrade()
{
    return HERO_UPGRADE;
}

bn::color palette_variant_color(engine::PaletteVariant variant)
{
    auto palettes = engine::get_character_palettes(current_hero());

    for(const engine::PaletteDef& def : palettes)
    {
        if(def.variant == variant)
        {
            return bn::color(def.primary_color_bgr555);
        }
    }

    return bn::color(31, 31, 31);
}

bn::fixed palette_variant_intensity(engine::PaletteVariant variant)
{
    switch(variant)
    {
    case engine::PaletteVariant::SHINY:
        return bn::fixed(0.45);
    case engine::PaletteVariant::SHADOW:
        return bn::fixed(0.55);
    case engine::PaletteVariant::GOLDEN:
        return bn::fixed(0.4);
    default:
        return 0;
    }
}

const bn::sprite_item& big_card_item(engine::CardId id)
{
    switch(id)
    {
    case engine::CardId::STRIKE:
        return bn::sprite_items::HERO_ITEM(card_strike_);
    case engine::CardId::DEFEND:
        return bn::sprite_items::HERO_ITEM(card_defend_);
    case engine::CardId::BASH:
        return bn::sprite_items::card_bash;
    case engine::CardId::IRON_WAVE:
        return bn::sprite_items::card_iron_wave;
    case engine::CardId::CLEAVE:
        return bn::sprite_items::card_cleave;
    case engine::CardId::ANGER:
        return bn::sprite_items::card_anger;
    case engine::CardId::METALLICIZE:
        return bn::sprite_items::card_metallicize;
    case engine::CardId::NEUTRALIZE:
        return bn::sprite_items::card_neutralize;
    case engine::CardId::SURVIVOR:
        return bn::sprite_items::card_survivor;
    case engine::CardId::POISON_STAB:
        return bn::sprite_items::card_poison_stab;
    case engine::CardId::DEADLY_POISON:
        return bn::sprite_items::card_deadly_poison;
    case engine::CardId::ZAP:
        return bn::sprite_items::card_zap;
    case engine::CardId::DUALCAST:
        return bn::sprite_items::card_dualcast;
    case engine::CardId::FALLING_STAR:
        return bn::sprite_items::card_falling_star;
    case engine::CardId::VENERATE:
        return bn::sprite_items::card_venerate;
    case engine::CardId::UNLEASH:
        return bn::sprite_items::card_unleash;
    case engine::CardId::BODYGUARD:
        return bn::sprite_items::card_bodyguard;
    case engine::CardId::TWIN_STRIKE:
        return bn::sprite_items::card_twin_strike;
    case engine::CardId::POMMEL_STRIKE:
        return bn::sprite_items::card_pommel_strike;
    case engine::CardId::SHRUG_IT_OFF:
        return bn::sprite_items::card_shrug_it_off;
    case engine::CardId::TAUNT:
        return bn::sprite_items::card_taunt;
    case engine::CardId::SLICE:
        return bn::sprite_items::card_slice;
    case engine::CardId::BACKFLIP:
        return bn::sprite_items::card_backflip;
    case engine::CardId::SUCKER_PUNCH:
        return bn::sprite_items::card_sucker_punch;
    case engine::CardId::DAGGER_SPRAY:
        return bn::sprite_items::card_dagger_spray;
    case engine::CardId::BEAM_CELL:
        return bn::sprite_items::card_beam_cell;
    case engine::CardId::BALL_LIGHTNING:
        return bn::sprite_items::card_ball_lightning;
    case engine::CardId::COLD_SNAP:
        return bn::sprite_items::card_cold_snap;
    case engine::CardId::LEAP:
        return bn::sprite_items::card_leap;
    case engine::CardId::SOLAR_STRIKE:
        return bn::sprite_items::card_solar_strike;
    case engine::CardId::GATHER_LIGHT:
        return bn::sprite_items::card_gather_light;
    case engine::CardId::CELESTIAL_MIGHT:
        return bn::sprite_items::card_celestial_might;
    default:
        return bn::sprite_items::HERO_ITEM(card_strike_);
    }
}

int hand_card_frame(const engine::DeckCard& card)
{
    int index = static_cast<int>(card.id);

    if(index < 0 || index >= static_cast<int>(engine::CardId::COUNT))
    {
        index = 0;
    }

    return index * 2 + (card.upgraded ? 1 : 0);
}

EnemyArt enemy_art(engine::EnemyType type)
{
    switch(type)
    {
    case engine::EnemyType::JAW_WORM:
        return { &bn::sprite_items::mon_jaw_worm, false };
    case engine::EnemyType::LOUSE:
        return { &bn::sprite_items::mon_louse, false };
    case engine::EnemyType::GREMLIN_NOB:
        return { &bn::sprite_items::mon_gremlin_nob, false };
    case engine::EnemyType::CEREMONIAL_BEAST:
        return { &bn::sprite_items::boss_ceremonial_beast, true };
    default:
        return { &bn::sprite_items::mon_cultist, false };
    }
}

const bn::regular_bg_item& event_bg(std::string_view event_id)
{
    if(event_id == "COLOSSAL_FLOWER")
    {
        return bn::regular_bg_items::bg_event_colossal_flower;
    }

    if(event_id == "ABYSSAL_BATHS")
    {
        return bn::regular_bg_items::bg_event_abyssal_baths;
    }

    if(event_id == "MYSTIC_ALTAR")
    {
        return bn::regular_bg_items::bg_event_mystic_altar;
    }

    return bn::regular_bg_items::bg_event_battleworn_dummy;
}

}
