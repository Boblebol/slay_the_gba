#include "gba/assets.h"

#include "gba/card_art.h"

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
#include "bn_sprite_items_mon_assassin_ruby_raider.h"
#include "bn_sprite_items_mon_axe_ruby_raider.h"
#include "bn_sprite_items_mon_brute_ruby_raider.h"
#include "bn_sprite_items_mon_calcified_cultist.h"
#include "bn_sprite_items_mon_crossbow_ruby_raider.h"
#include "bn_sprite_items_mon_cubex_construct.h"
#include "bn_sprite_items_mon_cultist.h"
#include "bn_sprite_items_mon_damp_cultist.h"
#include "bn_sprite_items_mon_eye_with_teeth.h"
#include "bn_sprite_items_mon_stabbot.h"
#include "bn_sprite_items_mon_corpse_slug.h"
#include "bn_sprite_items_mon_gas_bomb.h"
#include "bn_sprite_items_mon_spiny_toad.h"
#include "bn_sprite_items_mon_seapunk.h"
#include "bn_sprite_items_mon_flyconid.h"
#include "bn_sprite_items_mon_fossil_stalker.h"
#include "bn_sprite_items_mon_fuzzy_wurm_crawler.h"
#include "bn_sprite_items_mon_gremlin_merc.h"
#include "bn_sprite_items_mon_gremlin_nob.h"
#include "bn_sprite_items_mon_haunted_ship.h"
#include "bn_sprite_items_mon_hunter_killer.h"
#include "bn_sprite_items_mon_inklet.h"
#include "bn_sprite_items_mon_jaw_worm.h"
#include "bn_sprite_items_mon_noisebot.h"
#include "bn_sprite_items_mon_mawler.h"
#include "bn_sprite_items_mon_nibbit.h"
#include "bn_sprite_items_mon_punch_construct.h"
#include "bn_sprite_items_mon_toadpole.h"
#include "bn_sprite_items_mon_louse.h"
#include "bn_sprite_items_mon_fat_gremlin.h"
#include "bn_sprite_items_mon_living_shield.h"
#include "bn_sprite_items_mon_sewer_clam.h"
#include "bn_sprite_items_mon_sneaky_gremlin.h"
#include "bn_sprite_items_mon_sludge_spinner.h"
#include "bn_sprite_items_mon_tracker_ruby_raider.h"
#include "bn_sprite_items_mon_the_forgotten.h"
#include "bn_sprite_items_mon_vine_shambler.h"

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
    const bn::sprite_item* item = card_art::big(id);

    // A card of another hero has no art on this cartridge.
    return item ? *item : bn::sprite_items::HERO_ITEM(card_strike_);
}

int hand_card_frame(const engine::DeckCard& card)
{
    int index = static_cast<int>(card.id);

    // Unknown cards and the cards of another hero show the blank card of slot 0.
    int slot = (index >= 0 && index < static_cast<int>(engine::CardId::COUNT)) ? card_art::SHEET_SLOT[index] : 0;
    return slot * 2 + (card.upgraded ? 1 : 0);
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
    case engine::EnemyType::FUZZY_WURM_CRAWLER:
        return { &bn::sprite_items::mon_fuzzy_wurm_crawler, false };
    case engine::EnemyType::CROSSBOW_RUBY_RAIDER:
        return { &bn::sprite_items::mon_crossbow_ruby_raider, false };
    case engine::EnemyType::BRUTE_RUBY_RAIDER:
        return { &bn::sprite_items::mon_brute_ruby_raider, false };
    case engine::EnemyType::ASSASSIN_RUBY_RAIDER:
        return { &bn::sprite_items::mon_assassin_ruby_raider, false };
    case engine::EnemyType::AXE_RUBY_RAIDER:
        return { &bn::sprite_items::mon_axe_ruby_raider, false };
    case engine::EnemyType::TRACKER_RUBY_RAIDER:
        return { &bn::sprite_items::mon_tracker_ruby_raider, false };
    case engine::EnemyType::CALCIFIED_CULTIST:
        return { &bn::sprite_items::mon_calcified_cultist, false };
    case engine::EnemyType::DAMP_CULTIST:
        return { &bn::sprite_items::mon_damp_cultist, false };
    case engine::EnemyType::SEWER_CLAM:
        return { &bn::sprite_items::mon_sewer_clam, false };
    case engine::EnemyType::EYE_WITH_TEETH:
        return { &bn::sprite_items::mon_eye_with_teeth, false };
    case engine::EnemyType::SPINY_TOAD:
        return { &bn::sprite_items::mon_spiny_toad, false };
    case engine::EnemyType::SEAPUNK:
        return { &bn::sprite_items::mon_seapunk, false };
    case engine::EnemyType::STABBOT:
        return { &bn::sprite_items::mon_stabbot, false };
    case engine::EnemyType::NOISEBOT:
        return { &bn::sprite_items::mon_noisebot, false };
    case engine::EnemyType::CUBEX_CONSTRUCT:
        return { &bn::sprite_items::mon_cubex_construct, false };
    case engine::EnemyType::PUNCH_CONSTRUCT:
        return { &bn::sprite_items::mon_punch_construct, false };
    case engine::EnemyType::INKLET:
        return { &bn::sprite_items::mon_inklet, false };
    case engine::EnemyType::NIBBIT:
        return { &bn::sprite_items::mon_nibbit, false };
    case engine::EnemyType::GREMLIN_MERC:
        return { &bn::sprite_items::mon_gremlin_merc, false };
    case engine::EnemyType::HAUNTED_SHIP:
        return { &bn::sprite_items::mon_haunted_ship, false };
    case engine::EnemyType::SLUDGE_SPINNER:
        return { &bn::sprite_items::mon_sludge_spinner, false };
    case engine::EnemyType::FLYCONID:
        return { &bn::sprite_items::mon_flyconid, false };
    case engine::EnemyType::CORPSE_SLUG:
        return { &bn::sprite_items::mon_corpse_slug, false };
    case engine::EnemyType::GAS_BOMB:
        return { &bn::sprite_items::mon_gas_bomb, false };
    case engine::EnemyType::SNEAKY_GREMLIN:
        return { &bn::sprite_items::mon_sneaky_gremlin, false };
    case engine::EnemyType::TOADPOLE:
        return { &bn::sprite_items::mon_toadpole, false };
    case engine::EnemyType::HUNTER_KILLER:
        return { &bn::sprite_items::mon_hunter_killer, false };
    case engine::EnemyType::MAWLER:
        return { &bn::sprite_items::mon_mawler, false };
    case engine::EnemyType::FOSSIL_STALKER:
        return { &bn::sprite_items::mon_fossil_stalker, false };
    case engine::EnemyType::FAT_GREMLIN:
        return { &bn::sprite_items::mon_fat_gremlin, false };
    case engine::EnemyType::LIVING_SHIELD:
        return { &bn::sprite_items::mon_living_shield, false };
    case engine::EnemyType::VINE_SHAMBLER:
        return { &bn::sprite_items::mon_vine_shambler, false };
    case engine::EnemyType::THE_FORGOTTEN:
        return { &bn::sprite_items::mon_the_forgotten, false };
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
