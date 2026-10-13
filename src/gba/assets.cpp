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
#include "bn_sprite_items_boss_the_insatiable.h"
#include "bn_sprite_items_boss_lagavulin_matriarch.h"
#include "bn_sprite_items_boss_test_subject.h"
#include "bn_sprite_items_boss_vantom.h"
#include "bn_sprite_items_mon_assassin_ruby_raider.h"
#include "bn_sprite_items_mon_axe_ruby_raider.h"
#include "bn_sprite_items_mon_brute_ruby_raider.h"
#include "bn_sprite_items_mon_calcified_cultist.h"
#include "bn_sprite_items_mon_crossbow_ruby_raider.h"
#include "bn_sprite_items_mon_bowlbug_rock.h"
#include "bn_sprite_items_mon_chomper.h"
#include "bn_sprite_items_mon_cubex_construct.h"
#include "bn_sprite_items_mon_cultist.h"
#include "bn_sprite_items_mon_damp_cultist.h"
#include "bn_sprite_items_mon_eye_with_teeth.h"
#include "bn_sprite_items_mon_soul_nexus.h"
#include "bn_sprite_items_mon_stabbot.h"
#include "bn_sprite_items_mon_corpse_slug.h"
#include "bn_sprite_items_mon_gas_bomb.h"
#include "bn_sprite_items_mon_spiny_toad.h"
#include "bn_sprite_items_mon_seapunk.h"
#include "bn_sprite_items_mon_flail_knight.h"
#include "bn_sprite_items_mon_flyconid.h"
#include "bn_sprite_items_mon_fossil_stalker.h"
#include "bn_sprite_items_mon_fuzzy_wurm_crawler.h"
#include "bn_sprite_items_mon_gremlin_merc.h"
#include "bn_sprite_items_mon_gremlin_nob.h"
#include "bn_sprite_items_mon_haunted_ship.h"
#include "bn_sprite_items_mon_hunter_killer.h"
#include "bn_sprite_items_mon_entomancer.h"
#include "bn_sprite_items_mon_inklet.h"
#include "bn_sprite_items_mon_jaw_worm.h"
#include "bn_sprite_items_mon_noisebot.h"
#include "bn_sprite_items_mon_mawler.h"
#include "bn_sprite_items_mon_zapbot.h"
#include "bn_sprite_items_mon_nibbit.h"
#include "bn_sprite_items_mon_globe_head.h"
#include "bn_sprite_items_mon_punch_construct.h"
#include "bn_sprite_items_mon_bowlbug_nectar.h"
#include "bn_sprite_items_mon_devoted_sculptor.h"
#include "bn_sprite_items_mon_axebot.h"
#include "bn_sprite_items_mon_bygone_effigy.h"
#include "bn_sprite_items_mon_turret_operator.h"
#include "bn_sprite_items_mon_skulking_colony.h"
#include "bn_sprite_items_mon_terror_eel.h"
#include "bn_sprite_items_mon_infested_prism.h"
#include "bn_sprite_items_mon_magi_knight.h"
#include "bn_sprite_items_mon_toadpole.h"
#include "bn_sprite_items_mon_osty.h"
#include "bn_sprite_items_mon_parafright.h"
#include "bn_sprite_items_mon_slimed_berserker.h"
#include "bn_sprite_items_mon_frog_knight.h"
#include "bn_sprite_items_mon_guardbot.h"
#include "bn_sprite_items_mon_bowlbug_silk.h"
#include "bn_sprite_items_mon_bowlbug_egg.h"
#include "bn_sprite_items_mon_byrdonis.h"
#include "bn_sprite_items_mon_tough_egg.h"
#include "bn_sprite_items_mon_the_obscura.h"
#include "bn_sprite_items_mon_ovicopter.h"
#include "bn_sprite_items_mon_fabricator.h"
#include "bn_sprite_items_mon_shrinker_beetle.h"
#include "bn_sprite_items_mon_fogmog.h"
#include "bn_sprite_items_mon_snapping_jaxfruit.h"
#include "bn_sprite_items_mon_twig_slime_m.h"
#include "bn_sprite_items_mon_twig_slime_s.h"
#include "bn_sprite_items_mon_leaf_slime_m.h"
#include "bn_sprite_items_mon_leaf_slime_s.h"
#include "bn_sprite_items_mon_slithering_strangler.h"
#include "bn_sprite_items_mon_living_fog.h"
#include "bn_sprite_items_mon_two_tailed_rat.h"
#include "bn_sprite_items_mon_owl_magistrate.h"
#include "bn_sprite_items_mon_scroll_of_biting.h"
#include "bn_sprite_items_mon_phrog_parasite.h"
#include "bn_sprite_items_mon_wriggler.h"
#include "bn_sprite_items_mon_louse.h"
#include "bn_sprite_items_mon_louse_progenitor.h"
#include "bn_sprite_items_mon_fat_gremlin.h"
#include "bn_sprite_items_mon_living_shield.h"
#include "bn_sprite_items_mon_sewer_clam.h"
#include "bn_sprite_items_mon_spectral_knight.h"
#include "bn_sprite_items_mon_sneaky_gremlin.h"
#include "bn_sprite_items_mon_sludge_spinner.h"
#include "bn_sprite_items_mon_tracker_ruby_raider.h"
#include "bn_sprite_items_mon_slumbering_beetle.h"
#include "bn_sprite_items_mon_mecha_knight.h"
#include "bn_sprite_items_mon_the_forgotten.h"
#include "bn_sprite_items_mon_the_lost.h"
#include "bn_sprite_items_mon_tunneler.h"
#include "bn_sprite_items_mon_vine_shambler.h"

#include "bn_sprite_items_ancient_darv.h"
#include "bn_sprite_items_ancient_neow.h"
#include "bn_sprite_items_ancient_nonupeipe.h"
#include "bn_sprite_items_ancient_orobas.h"
#include "bn_sprite_items_ancient_pael.h"
#include "bn_sprite_items_ancient_tanx.h"
#include "bn_sprite_items_ancient_tezcatara.h"
#include "bn_sprite_items_ancient_vakuu.h"

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
    case engine::EnemyType::THE_INSATIABLE:
        return { &bn::sprite_items::boss_the_insatiable, true };
    case engine::EnemyType::LAGAVULIN_MATRIARCH:
        return { &bn::sprite_items::boss_lagavulin_matriarch, true };
    case engine::EnemyType::TEST_SUBJECT:
        return { &bn::sprite_items::boss_test_subject, true };
    case engine::EnemyType::VANTOM:
        return { &bn::sprite_items::boss_vantom, true };
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
    case engine::EnemyType::MECHA_KNIGHT:
        return { &bn::sprite_items::mon_mecha_knight, false };
    case engine::EnemyType::CALCIFIED_CULTIST:
        return { &bn::sprite_items::mon_calcified_cultist, false };
    case engine::EnemyType::DAMP_CULTIST:
        return { &bn::sprite_items::mon_damp_cultist, false };
    case engine::EnemyType::SEWER_CLAM:
        return { &bn::sprite_items::mon_sewer_clam, false };
    case engine::EnemyType::EYE_WITH_TEETH:
        return { &bn::sprite_items::mon_eye_with_teeth, false };
    case engine::EnemyType::SPECTRAL_KNIGHT:
        return { &bn::sprite_items::mon_spectral_knight, false };
    case engine::EnemyType::SPINY_TOAD:
        return { &bn::sprite_items::mon_spiny_toad, false };
    case engine::EnemyType::FLAIL_KNIGHT:
        return { &bn::sprite_items::mon_flail_knight, false };
    case engine::EnemyType::SEAPUNK:
        return { &bn::sprite_items::mon_seapunk, false };
    case engine::EnemyType::STABBOT:
        return { &bn::sprite_items::mon_stabbot, false };
    case engine::EnemyType::ZAPBOT:
        return { &bn::sprite_items::mon_zapbot, false };
    case engine::EnemyType::NOISEBOT:
        return { &bn::sprite_items::mon_noisebot, false };
    case engine::EnemyType::CUBEX_CONSTRUCT:
        return { &bn::sprite_items::mon_cubex_construct, false };
    case engine::EnemyType::TERROR_EEL:
        return { &bn::sprite_items::mon_terror_eel, false };
    case engine::EnemyType::PUNCH_CONSTRUCT:
        return { &bn::sprite_items::mon_punch_construct, false };
    case engine::EnemyType::BOWLBUG_NECTAR:
        return { &bn::sprite_items::mon_bowlbug_nectar, false };
    case engine::EnemyType::AXEBOT:
        return { &bn::sprite_items::mon_axebot, false };
    case engine::EnemyType::TURRET_OPERATOR:
        return { &bn::sprite_items::mon_turret_operator, false };
    case engine::EnemyType::BOWLBUG_ROCK:
        return { &bn::sprite_items::mon_bowlbug_rock, false };
    case engine::EnemyType::SKULKING_COLONY:
        return { &bn::sprite_items::mon_skulking_colony, false };
    case engine::EnemyType::MAGI_KNIGHT:
        return { &bn::sprite_items::mon_magi_knight, false };
    case engine::EnemyType::BYGONE_EFFIGY:
        return { &bn::sprite_items::mon_bygone_effigy, false };
    case engine::EnemyType::INKLET:
        return { &bn::sprite_items::mon_inklet, false };
    case engine::EnemyType::NIBBIT:
        return { &bn::sprite_items::mon_nibbit, false };
    case engine::EnemyType::GREMLIN_MERC:
        return { &bn::sprite_items::mon_gremlin_merc, false };
    case engine::EnemyType::GLOBE_HEAD:
        return { &bn::sprite_items::mon_globe_head, false };
    case engine::EnemyType::LOUSE_PROGENITOR:
        return { &bn::sprite_items::mon_louse_progenitor, false };
    case engine::EnemyType::HAUNTED_SHIP:
        return { &bn::sprite_items::mon_haunted_ship, false };
    case engine::EnemyType::SLUDGE_SPINNER:
        return { &bn::sprite_items::mon_sludge_spinner, false };
    case engine::EnemyType::OSTY:
        return { &bn::sprite_items::mon_osty, false };
    case engine::EnemyType::SLUMBERING_BEETLE:
        return { &bn::sprite_items::mon_slumbering_beetle, false };
    case engine::EnemyType::INFESTED_PRISM:
        return { &bn::sprite_items::mon_infested_prism, false };
    case engine::EnemyType::FLYCONID:
        return { &bn::sprite_items::mon_flyconid, false };
    case engine::EnemyType::CORPSE_SLUG:
        return { &bn::sprite_items::mon_corpse_slug, false };
    case engine::EnemyType::GAS_BOMB:
        return { &bn::sprite_items::mon_gas_bomb, false };
    case engine::EnemyType::SNEAKY_GREMLIN:
        return { &bn::sprite_items::mon_sneaky_gremlin, false };
    case engine::EnemyType::GUARDBOT:
        return { &bn::sprite_items::mon_guardbot, false };
    case engine::EnemyType::DEVOTED_SCULPTOR:
        return { &bn::sprite_items::mon_devoted_sculptor, false };
    case engine::EnemyType::BOWLBUG_SILK:
        return { &bn::sprite_items::mon_bowlbug_silk, false };
    case engine::EnemyType::TOADPOLE:
        return { &bn::sprite_items::mon_toadpole, false };
    case engine::EnemyType::HUNTER_KILLER:
        return { &bn::sprite_items::mon_hunter_killer, false };
    case engine::EnemyType::ENTOMANCER:
        return { &bn::sprite_items::mon_entomancer, false };
    case engine::EnemyType::MAWLER:
        return { &bn::sprite_items::mon_mawler, false };
    case engine::EnemyType::SOUL_NEXUS:
        return { &bn::sprite_items::mon_soul_nexus, false };
    case engine::EnemyType::FOSSIL_STALKER:
        return { &bn::sprite_items::mon_fossil_stalker, false };
    case engine::EnemyType::CHOMPER:
        return { &bn::sprite_items::mon_chomper, false };
    case engine::EnemyType::PARAFRIGHT:
        return { &bn::sprite_items::mon_parafright, false };
    case engine::EnemyType::TUNNELER:
        return { &bn::sprite_items::mon_tunneler, false };
    case engine::EnemyType::SLIMED_BERSERKER:
        return { &bn::sprite_items::mon_slimed_berserker, false };
    case engine::EnemyType::FROG_KNIGHT:
        return { &bn::sprite_items::mon_frog_knight, false };
    case engine::EnemyType::BOWLBUG_EGG:
        return { &bn::sprite_items::mon_bowlbug_egg, false };
    case engine::EnemyType::BYRDONIS:
        return { &bn::sprite_items::mon_byrdonis, false };
    case engine::EnemyType::OVICOPTER:
        return { &bn::sprite_items::mon_ovicopter, false };
    case engine::EnemyType::TOUGH_EGG:
        return { &bn::sprite_items::mon_tough_egg, false };
    case engine::EnemyType::THE_OBSCURA:
        return { &bn::sprite_items::mon_the_obscura, false };
    case engine::EnemyType::FABRICATOR:
        return { &bn::sprite_items::mon_fabricator, false };
    case engine::EnemyType::SHRINKER_BEETLE:
        return { &bn::sprite_items::mon_shrinker_beetle, false };
    case engine::EnemyType::FOGMOG:
        return { &bn::sprite_items::mon_fogmog, false };
    case engine::EnemyType::SNAPPING_JAXFRUIT:
        return { &bn::sprite_items::mon_snapping_jaxfruit, false };
    case engine::EnemyType::TWIG_SLIME_M:
        return { &bn::sprite_items::mon_twig_slime_m, false };
    case engine::EnemyType::TWIG_SLIME_S:
        return { &bn::sprite_items::mon_twig_slime_s, false };
    case engine::EnemyType::LEAF_SLIME_M:
        return { &bn::sprite_items::mon_leaf_slime_m, false };
    case engine::EnemyType::LEAF_SLIME_S:
        return { &bn::sprite_items::mon_leaf_slime_s, false };
    case engine::EnemyType::SLITHERING_STRANGLER:
        return { &bn::sprite_items::mon_slithering_strangler, false };
    case engine::EnemyType::LIVING_FOG:
        return { &bn::sprite_items::mon_living_fog, false };
    case engine::EnemyType::TWO_TAILED_RAT:
        return { &bn::sprite_items::mon_two_tailed_rat, false };
    case engine::EnemyType::OWL_MAGISTRATE:
        return { &bn::sprite_items::mon_owl_magistrate, false };
    case engine::EnemyType::SCROLL_OF_BITING:
        return { &bn::sprite_items::mon_scroll_of_biting, false };
    case engine::EnemyType::PHROG_PARASITE:
        return { &bn::sprite_items::mon_phrog_parasite, false };
    case engine::EnemyType::WRIGGLER:
        return { &bn::sprite_items::mon_wriggler, false };
    case engine::EnemyType::FAT_GREMLIN:
        return { &bn::sprite_items::mon_fat_gremlin, false };
    case engine::EnemyType::LIVING_SHIELD:
        return { &bn::sprite_items::mon_living_shield, false };
    case engine::EnemyType::VINE_SHAMBLER:
        return { &bn::sprite_items::mon_vine_shambler, false };
    case engine::EnemyType::THE_FORGOTTEN:
        return { &bn::sprite_items::mon_the_forgotten, false };
    case engine::EnemyType::THE_LOST:
        return { &bn::sprite_items::mon_the_lost, false };
    default:
        return { &bn::sprite_items::mon_cultist, false };
    }
}

const bn::sprite_item& ancient_portrait(engine::AncientId id)
{
    switch(id)
    {
    case engine::AncientId::DARV:
        return bn::sprite_items::ancient_darv;
    case engine::AncientId::OROBAS:
        return bn::sprite_items::ancient_orobas;
    case engine::AncientId::PAEL:
        return bn::sprite_items::ancient_pael;
    case engine::AncientId::TEZCATARA:
        return bn::sprite_items::ancient_tezcatara;
    case engine::AncientId::NONUPEIPE:
        return bn::sprite_items::ancient_nonupeipe;
    case engine::AncientId::TANX:
        return bn::sprite_items::ancient_tanx;
    case engine::AncientId::VAKUU:
        return bn::sprite_items::ancient_vakuu;
    default:
        return bn::sprite_items::ancient_neow;
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
