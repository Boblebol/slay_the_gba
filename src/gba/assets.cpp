#include "gba/assets.h"

#include "gba/card_art.h"

#include "bn_regular_bg_items_bg_battle_defect.h"
#include "bn_regular_bg_items_bg_battle_ironclad.h"
#include "bn_regular_bg_items_bg_battle_necrobinder.h"
#include "bn_regular_bg_items_bg_battle_regent.h"
#include "bn_regular_bg_items_bg_battle_silent.h"
#include "bn_regular_bg_items_bg_event_battleworn_dummy.h"

#include "bn_sprite_items_boss_ceremonial_beast.h"
#include "bn_sprite_items_boss_the_insatiable.h"
#include "bn_sprite_items_boss_lagavulin_matriarch.h"
#include "bn_sprite_items_boss_knowledge_demon.h"
#include "bn_sprite_items_boss_test_subject.h"
#include "bn_sprite_items_boss_aeonglass.h"
#include "bn_sprite_items_boss_vantom.h"
#include "bn_sprite_items_mon_queen.h"
#include "bn_sprite_items_mon_torch_head_amalgam.h"
#include "bn_sprite_items_boss_soul_fysh.h"
#include "bn_sprite_items_boss_waterfall_giant.h"
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
#include "bn_sprite_items_mon_kin_follower.h"
#include "bn_sprite_items_mon_kin_priest.h"
#include "bn_sprite_items_mon_snapping_jaxfruit.h"
#include "bn_sprite_items_mon_twig_slime_m.h"
#include "bn_sprite_items_mon_twig_slime_s.h"
#include "bn_sprite_items_mon_leaf_slime_m.h"
#include "bn_sprite_items_mon_leaf_slime_s.h"
#include "bn_sprite_items_mon_slithering_strangler.h"
#include "bn_sprite_items_mon_living_fog.h"
#include "bn_sprite_items_mon_phantasmal_gardener.h"
#include "bn_sprite_items_mon_exoskeleton.h"
#include "bn_sprite_items_mon_decimillipede_back.h"
#include "bn_sprite_items_mon_decimillipede_front.h"
#include "bn_sprite_items_mon_decimillipede_middle.h"
#include "bn_sprite_items_mon_myte.h"
#include "bn_sprite_items_mon_thieving_hopper.h"
#include "bn_sprite_items_mon_two_tailed_rat.h"
#include "bn_sprite_items_mon_crusher.h"
#include "bn_sprite_items_mon_rocket.h"
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

// Backgrounds of the events: graphics/bg_event_<id>.bmp, picked up when it exists (see include/engine/event_script.h).
#if __has_include("bn_regular_bg_items_bg_event_abyssal_baths.h")
#include "bn_regular_bg_items_bg_event_abyssal_baths.h"
#define HAS_BG_EVENT_ABYSSAL_BATHS 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_amalgamator.h")
#include "bn_regular_bg_items_bg_event_amalgamator.h"
#define HAS_BG_EVENT_AMALGAMATOR 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_aroma_of_chaos.h")
#include "bn_regular_bg_items_bg_event_aroma_of_chaos.h"
#define HAS_BG_EVENT_AROMA_OF_CHAOS 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_brain_leech.h")
#include "bn_regular_bg_items_bg_event_brain_leech.h"
#define HAS_BG_EVENT_BRAIN_LEECH 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_bugslayer.h")
#include "bn_regular_bg_items_bg_event_bugslayer.h"
#define HAS_BG_EVENT_BUGSLAYER 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_byrdonis_nest.h")
#include "bn_regular_bg_items_bg_event_byrdonis_nest.h"
#define HAS_BG_EVENT_BYRDONIS_NEST 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_colorful_philosophers.h")
#include "bn_regular_bg_items_bg_event_colorful_philosophers.h"
#define HAS_BG_EVENT_COLORFUL_PHILOSOPHERS 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_colossal_flower.h")
#include "bn_regular_bg_items_bg_event_colossal_flower.h"
#define HAS_BG_EVENT_COLOSSAL_FLOWER 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_crystal_sphere.h")
#include "bn_regular_bg_items_bg_event_crystal_sphere.h"
#define HAS_BG_EVENT_CRYSTAL_SPHERE 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_dense_vegetation.h")
#include "bn_regular_bg_items_bg_event_dense_vegetation.h"
#define HAS_BG_EVENT_DENSE_VEGETATION 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_doll_room.h")
#include "bn_regular_bg_items_bg_event_doll_room.h"
#define HAS_BG_EVENT_DOLL_ROOM 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_doors_of_light_and_dark.h")
#include "bn_regular_bg_items_bg_event_doors_of_light_and_dark.h"
#define HAS_BG_EVENT_DOORS_OF_LIGHT_AND_DARK 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_drowning_beacon.h")
#include "bn_regular_bg_items_bg_event_drowning_beacon.h"
#define HAS_BG_EVENT_DROWNING_BEACON 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_endless_conveyor.h")
#include "bn_regular_bg_items_bg_event_endless_conveyor.h"
#define HAS_BG_EVENT_ENDLESS_CONVEYOR 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_fake_merchant.h")
#include "bn_regular_bg_items_bg_event_fake_merchant.h"
#define HAS_BG_EVENT_FAKE_MERCHANT 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_field_of_man_sized_holes.h")
#include "bn_regular_bg_items_bg_event_field_of_man_sized_holes.h"
#define HAS_BG_EVENT_FIELD_OF_MAN_SIZED_HOLES 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_grave_of_the_forgotten.h")
#include "bn_regular_bg_items_bg_event_grave_of_the_forgotten.h"
#define HAS_BG_EVENT_GRAVE_OF_THE_FORGOTTEN 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_hungry_for_mushrooms.h")
#include "bn_regular_bg_items_bg_event_hungry_for_mushrooms.h"
#define HAS_BG_EVENT_HUNGRY_FOR_MUSHROOMS 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_infested_automaton.h")
#include "bn_regular_bg_items_bg_event_infested_automaton.h"
#define HAS_BG_EVENT_INFESTED_AUTOMATON 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_jungle_maze_adventure.h")
#include "bn_regular_bg_items_bg_event_jungle_maze_adventure.h"
#define HAS_BG_EVENT_JUNGLE_MAZE_ADVENTURE 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_lost_wisp.h")
#include "bn_regular_bg_items_bg_event_lost_wisp.h"
#define HAS_BG_EVENT_LOST_WISP 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_luminous_choir.h")
#include "bn_regular_bg_items_bg_event_luminous_choir.h"
#define HAS_BG_EVENT_LUMINOUS_CHOIR 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_morphic_grove.h")
#include "bn_regular_bg_items_bg_event_morphic_grove.h"
#define HAS_BG_EVENT_MORPHIC_GROVE 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_potion_courier.h")
#include "bn_regular_bg_items_bg_event_potion_courier.h"
#define HAS_BG_EVENT_POTION_COURIER 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_punch_off.h")
#include "bn_regular_bg_items_bg_event_punch_off.h"
#define HAS_BG_EVENT_PUNCH_OFF 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_ranwid_the_elder.h")
#include "bn_regular_bg_items_bg_event_ranwid_the_elder.h"
#define HAS_BG_EVENT_RANWID_THE_ELDER 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_reflections.h")
#include "bn_regular_bg_items_bg_event_reflections.h"
#define HAS_BG_EVENT_REFLECTIONS 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_relic_trader.h")
#include "bn_regular_bg_items_bg_event_relic_trader.h"
#define HAS_BG_EVENT_RELIC_TRADER 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_room_full_of_cheese.h")
#include "bn_regular_bg_items_bg_event_room_full_of_cheese.h"
#define HAS_BG_EVENT_ROOM_FULL_OF_CHEESE 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_round_tea_party.h")
#include "bn_regular_bg_items_bg_event_round_tea_party.h"
#define HAS_BG_EVENT_ROUND_TEA_PARTY 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_sapphire_seed.h")
#include "bn_regular_bg_items_bg_event_sapphire_seed.h"
#define HAS_BG_EVENT_SAPPHIRE_SEED 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_self_help_book.h")
#include "bn_regular_bg_items_bg_event_self_help_book.h"
#define HAS_BG_EVENT_SELF_HELP_BOOK 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_slippery_bridge.h")
#include "bn_regular_bg_items_bg_event_slippery_bridge.h"
#define HAS_BG_EVENT_SLIPPERY_BRIDGE 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_spiraling_whirlpool.h")
#include "bn_regular_bg_items_bg_event_spiraling_whirlpool.h"
#define HAS_BG_EVENT_SPIRALING_WHIRLPOOL 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_spirit_grafter.h")
#include "bn_regular_bg_items_bg_event_spirit_grafter.h"
#define HAS_BG_EVENT_SPIRIT_GRAFTER 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_stone_of_all_time.h")
#include "bn_regular_bg_items_bg_event_stone_of_all_time.h"
#define HAS_BG_EVENT_STONE_OF_ALL_TIME 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_sunken_statue.h")
#include "bn_regular_bg_items_bg_event_sunken_statue.h"
#define HAS_BG_EVENT_SUNKEN_STATUE 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_sunken_treasury.h")
#include "bn_regular_bg_items_bg_event_sunken_treasury.h"
#define HAS_BG_EVENT_SUNKEN_TREASURY 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_symbiote.h")
#include "bn_regular_bg_items_bg_event_symbiote.h"
#define HAS_BG_EVENT_SYMBIOTE 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_tablet_of_truth.h")
#include "bn_regular_bg_items_bg_event_tablet_of_truth.h"
#define HAS_BG_EVENT_TABLET_OF_TRUTH 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_tea_master.h")
#include "bn_regular_bg_items_bg_event_tea_master.h"
#define HAS_BG_EVENT_TEA_MASTER 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_the_future_of_potions.h")
#include "bn_regular_bg_items_bg_event_the_future_of_potions.h"
#define HAS_BG_EVENT_THE_FUTURE_OF_POTIONS 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_the_lantern_key.h")
#include "bn_regular_bg_items_bg_event_the_lantern_key.h"
#define HAS_BG_EVENT_THE_LANTERN_KEY 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_the_legends_were_true.h")
#include "bn_regular_bg_items_bg_event_the_legends_were_true.h"
#define HAS_BG_EVENT_THE_LEGENDS_WERE_TRUE 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_this_or_that.h")
#include "bn_regular_bg_items_bg_event_this_or_that.h"
#define HAS_BG_EVENT_THIS_OR_THAT 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_tinker_time.h")
#include "bn_regular_bg_items_bg_event_tinker_time.h"
#define HAS_BG_EVENT_TINKER_TIME 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_trash_heap.h")
#include "bn_regular_bg_items_bg_event_trash_heap.h"
#define HAS_BG_EVENT_TRASH_HEAP 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_trial.h")
#include "bn_regular_bg_items_bg_event_trial.h"
#define HAS_BG_EVENT_TRIAL 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_unrest_site.h")
#include "bn_regular_bg_items_bg_event_unrest_site.h"
#define HAS_BG_EVENT_UNREST_SITE 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_war_historian_repy.h")
#include "bn_regular_bg_items_bg_event_war_historian_repy.h"
#define HAS_BG_EVENT_WAR_HISTORIAN_REPY 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_waterlogged_scriptorium.h")
#include "bn_regular_bg_items_bg_event_waterlogged_scriptorium.h"
#define HAS_BG_EVENT_WATERLOGGED_SCRIPTORIUM 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_welcome_to_wongos.h")
#include "bn_regular_bg_items_bg_event_welcome_to_wongos.h"
#define HAS_BG_EVENT_WELCOME_TO_WONGOS 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_wellspring.h")
#include "bn_regular_bg_items_bg_event_wellspring.h"
#define HAS_BG_EVENT_WELLSPRING 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_whispering_hollow.h")
#include "bn_regular_bg_items_bg_event_whispering_hollow.h"
#define HAS_BG_EVENT_WHISPERING_HOLLOW 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_wood_carvings.h")
#include "bn_regular_bg_items_bg_event_wood_carvings.h"
#define HAS_BG_EVENT_WOOD_CARVINGS 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_zen_weaver.h")
#include "bn_regular_bg_items_bg_event_zen_weaver.h"
#define HAS_BG_EVENT_ZEN_WEAVER 1
#endif
#if __has_include("bn_regular_bg_items_bg_event_mystic_altar.h")
#include "bn_regular_bg_items_bg_event_mystic_altar.h"
#define HAS_BG_EVENT_MYSTIC_ALTAR 1
#endif

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
    case engine::EnemyType::KNOWLEDGE_DEMON:
        return { &bn::sprite_items::boss_knowledge_demon, true };
    case engine::EnemyType::TEST_SUBJECT:
        return { &bn::sprite_items::boss_test_subject, true };
    case engine::EnemyType::AEONGLASS:
        return { &bn::sprite_items::boss_aeonglass, true };
    case engine::EnemyType::CRUSHER:
        return { &bn::sprite_items::mon_crusher, false };
    case engine::EnemyType::ROCKET:
        return { &bn::sprite_items::mon_rocket, false };
    case engine::EnemyType::VANTOM:
        return { &bn::sprite_items::boss_vantom, true };
    case engine::EnemyType::SOUL_FYSH:
        return { &bn::sprite_items::boss_soul_fysh, true };
    case engine::EnemyType::WATERFALL_GIANT:
        return { &bn::sprite_items::boss_waterfall_giant, true };
    case engine::EnemyType::KIN_FOLLOWER:
        return { &bn::sprite_items::mon_kin_follower, false };
    case engine::EnemyType::KIN_PRIEST:
        return { &bn::sprite_items::mon_kin_priest, false };
    // The Queen and the Torch Head Amalgam share their boss fight: monster-sized, side by side.
    case engine::EnemyType::QUEEN:
        return { &bn::sprite_items::mon_queen, false };
    case engine::EnemyType::TORCH_HEAD_AMALGAM:
        return { &bn::sprite_items::mon_torch_head_amalgam, false };
    case engine::EnemyType::PHANTASMAL_GARDENER:
        return { &bn::sprite_items::mon_phantasmal_gardener, false };
    case engine::EnemyType::EXOSKELETON:
        return { &bn::sprite_items::mon_exoskeleton, false };
    case engine::EnemyType::DECIMILLIPEDE_SEGMENT_BACK:
        return { &bn::sprite_items::mon_decimillipede_back, false };
    case engine::EnemyType::DECIMILLIPEDE_SEGMENT_FRONT:
        return { &bn::sprite_items::mon_decimillipede_front, false };
    case engine::EnemyType::DECIMILLIPEDE_SEGMENT_MIDDLE:
        return { &bn::sprite_items::mon_decimillipede_middle, false };
    case engine::EnemyType::MYTE:
        return { &bn::sprite_items::mon_myte, false };
    case engine::EnemyType::THIEVING_HOPPER:
        return { &bn::sprite_items::mon_thieving_hopper, false };
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
#ifdef HAS_BG_EVENT_ABYSSAL_BATHS
    if(event_id == "ABYSSAL_BATHS")
    {
        return bn::regular_bg_items::bg_event_abyssal_baths;
    }
#endif

#ifdef HAS_BG_EVENT_AMALGAMATOR
    if(event_id == "AMALGAMATOR")
    {
        return bn::regular_bg_items::bg_event_amalgamator;
    }
#endif

#ifdef HAS_BG_EVENT_AROMA_OF_CHAOS
    if(event_id == "AROMA_OF_CHAOS")
    {
        return bn::regular_bg_items::bg_event_aroma_of_chaos;
    }
#endif

#ifdef HAS_BG_EVENT_BRAIN_LEECH
    if(event_id == "BRAIN_LEECH")
    {
        return bn::regular_bg_items::bg_event_brain_leech;
    }
#endif

#ifdef HAS_BG_EVENT_BUGSLAYER
    if(event_id == "BUGSLAYER")
    {
        return bn::regular_bg_items::bg_event_bugslayer;
    }
#endif

#ifdef HAS_BG_EVENT_BYRDONIS_NEST
    if(event_id == "BYRDONIS_NEST")
    {
        return bn::regular_bg_items::bg_event_byrdonis_nest;
    }
#endif

#ifdef HAS_BG_EVENT_COLORFUL_PHILOSOPHERS
    if(event_id == "COLORFUL_PHILOSOPHERS")
    {
        return bn::regular_bg_items::bg_event_colorful_philosophers;
    }
#endif

#ifdef HAS_BG_EVENT_COLOSSAL_FLOWER
    if(event_id == "COLOSSAL_FLOWER")
    {
        return bn::regular_bg_items::bg_event_colossal_flower;
    }
#endif

#ifdef HAS_BG_EVENT_CRYSTAL_SPHERE
    if(event_id == "CRYSTAL_SPHERE")
    {
        return bn::regular_bg_items::bg_event_crystal_sphere;
    }
#endif

#ifdef HAS_BG_EVENT_DENSE_VEGETATION
    if(event_id == "DENSE_VEGETATION")
    {
        return bn::regular_bg_items::bg_event_dense_vegetation;
    }
#endif

#ifdef HAS_BG_EVENT_DOLL_ROOM
    if(event_id == "DOLL_ROOM")
    {
        return bn::regular_bg_items::bg_event_doll_room;
    }
#endif

#ifdef HAS_BG_EVENT_DOORS_OF_LIGHT_AND_DARK
    if(event_id == "DOORS_OF_LIGHT_AND_DARK")
    {
        return bn::regular_bg_items::bg_event_doors_of_light_and_dark;
    }
#endif

#ifdef HAS_BG_EVENT_DROWNING_BEACON
    if(event_id == "DROWNING_BEACON")
    {
        return bn::regular_bg_items::bg_event_drowning_beacon;
    }
#endif

#ifdef HAS_BG_EVENT_ENDLESS_CONVEYOR
    if(event_id == "ENDLESS_CONVEYOR")
    {
        return bn::regular_bg_items::bg_event_endless_conveyor;
    }
#endif

#ifdef HAS_BG_EVENT_FAKE_MERCHANT
    if(event_id == "FAKE_MERCHANT")
    {
        return bn::regular_bg_items::bg_event_fake_merchant;
    }
#endif

#ifdef HAS_BG_EVENT_FIELD_OF_MAN_SIZED_HOLES
    if(event_id == "FIELD_OF_MAN_SIZED_HOLES")
    {
        return bn::regular_bg_items::bg_event_field_of_man_sized_holes;
    }
#endif

#ifdef HAS_BG_EVENT_GRAVE_OF_THE_FORGOTTEN
    if(event_id == "GRAVE_OF_THE_FORGOTTEN")
    {
        return bn::regular_bg_items::bg_event_grave_of_the_forgotten;
    }
#endif

#ifdef HAS_BG_EVENT_HUNGRY_FOR_MUSHROOMS
    if(event_id == "HUNGRY_FOR_MUSHROOMS")
    {
        return bn::regular_bg_items::bg_event_hungry_for_mushrooms;
    }
#endif

#ifdef HAS_BG_EVENT_INFESTED_AUTOMATON
    if(event_id == "INFESTED_AUTOMATON")
    {
        return bn::regular_bg_items::bg_event_infested_automaton;
    }
#endif

#ifdef HAS_BG_EVENT_JUNGLE_MAZE_ADVENTURE
    if(event_id == "JUNGLE_MAZE_ADVENTURE")
    {
        return bn::regular_bg_items::bg_event_jungle_maze_adventure;
    }
#endif

#ifdef HAS_BG_EVENT_LOST_WISP
    if(event_id == "LOST_WISP")
    {
        return bn::regular_bg_items::bg_event_lost_wisp;
    }
#endif

#ifdef HAS_BG_EVENT_LUMINOUS_CHOIR
    if(event_id == "LUMINOUS_CHOIR")
    {
        return bn::regular_bg_items::bg_event_luminous_choir;
    }
#endif

#ifdef HAS_BG_EVENT_MORPHIC_GROVE
    if(event_id == "MORPHIC_GROVE")
    {
        return bn::regular_bg_items::bg_event_morphic_grove;
    }
#endif

#ifdef HAS_BG_EVENT_POTION_COURIER
    if(event_id == "POTION_COURIER")
    {
        return bn::regular_bg_items::bg_event_potion_courier;
    }
#endif

#ifdef HAS_BG_EVENT_PUNCH_OFF
    if(event_id == "PUNCH_OFF")
    {
        return bn::regular_bg_items::bg_event_punch_off;
    }
#endif

#ifdef HAS_BG_EVENT_RANWID_THE_ELDER
    if(event_id == "RANWID_THE_ELDER")
    {
        return bn::regular_bg_items::bg_event_ranwid_the_elder;
    }
#endif

#ifdef HAS_BG_EVENT_REFLECTIONS
    if(event_id == "REFLECTIONS")
    {
        return bn::regular_bg_items::bg_event_reflections;
    }
#endif

#ifdef HAS_BG_EVENT_RELIC_TRADER
    if(event_id == "RELIC_TRADER")
    {
        return bn::regular_bg_items::bg_event_relic_trader;
    }
#endif

#ifdef HAS_BG_EVENT_ROOM_FULL_OF_CHEESE
    if(event_id == "ROOM_FULL_OF_CHEESE")
    {
        return bn::regular_bg_items::bg_event_room_full_of_cheese;
    }
#endif

#ifdef HAS_BG_EVENT_ROUND_TEA_PARTY
    if(event_id == "ROUND_TEA_PARTY")
    {
        return bn::regular_bg_items::bg_event_round_tea_party;
    }
#endif

#ifdef HAS_BG_EVENT_SAPPHIRE_SEED
    if(event_id == "SAPPHIRE_SEED")
    {
        return bn::regular_bg_items::bg_event_sapphire_seed;
    }
#endif

#ifdef HAS_BG_EVENT_SELF_HELP_BOOK
    if(event_id == "SELF_HELP_BOOK")
    {
        return bn::regular_bg_items::bg_event_self_help_book;
    }
#endif

#ifdef HAS_BG_EVENT_SLIPPERY_BRIDGE
    if(event_id == "SLIPPERY_BRIDGE")
    {
        return bn::regular_bg_items::bg_event_slippery_bridge;
    }
#endif

#ifdef HAS_BG_EVENT_SPIRALING_WHIRLPOOL
    if(event_id == "SPIRALING_WHIRLPOOL")
    {
        return bn::regular_bg_items::bg_event_spiraling_whirlpool;
    }
#endif

#ifdef HAS_BG_EVENT_SPIRIT_GRAFTER
    if(event_id == "SPIRIT_GRAFTER")
    {
        return bn::regular_bg_items::bg_event_spirit_grafter;
    }
#endif

#ifdef HAS_BG_EVENT_STONE_OF_ALL_TIME
    if(event_id == "STONE_OF_ALL_TIME")
    {
        return bn::regular_bg_items::bg_event_stone_of_all_time;
    }
#endif

#ifdef HAS_BG_EVENT_SUNKEN_STATUE
    if(event_id == "SUNKEN_STATUE")
    {
        return bn::regular_bg_items::bg_event_sunken_statue;
    }
#endif

#ifdef HAS_BG_EVENT_SUNKEN_TREASURY
    if(event_id == "SUNKEN_TREASURY")
    {
        return bn::regular_bg_items::bg_event_sunken_treasury;
    }
#endif

#ifdef HAS_BG_EVENT_SYMBIOTE
    if(event_id == "SYMBIOTE")
    {
        return bn::regular_bg_items::bg_event_symbiote;
    }
#endif

#ifdef HAS_BG_EVENT_TABLET_OF_TRUTH
    if(event_id == "TABLET_OF_TRUTH")
    {
        return bn::regular_bg_items::bg_event_tablet_of_truth;
    }
#endif

#ifdef HAS_BG_EVENT_TEA_MASTER
    if(event_id == "TEA_MASTER")
    {
        return bn::regular_bg_items::bg_event_tea_master;
    }
#endif

#ifdef HAS_BG_EVENT_THE_FUTURE_OF_POTIONS
    if(event_id == "THE_FUTURE_OF_POTIONS")
    {
        return bn::regular_bg_items::bg_event_the_future_of_potions;
    }
#endif

#ifdef HAS_BG_EVENT_THE_LANTERN_KEY
    if(event_id == "THE_LANTERN_KEY")
    {
        return bn::regular_bg_items::bg_event_the_lantern_key;
    }
#endif

#ifdef HAS_BG_EVENT_THE_LEGENDS_WERE_TRUE
    if(event_id == "THE_LEGENDS_WERE_TRUE")
    {
        return bn::regular_bg_items::bg_event_the_legends_were_true;
    }
#endif

#ifdef HAS_BG_EVENT_THIS_OR_THAT
    if(event_id == "THIS_OR_THAT")
    {
        return bn::regular_bg_items::bg_event_this_or_that;
    }
#endif

#ifdef HAS_BG_EVENT_TINKER_TIME
    if(event_id == "TINKER_TIME")
    {
        return bn::regular_bg_items::bg_event_tinker_time;
    }
#endif

#ifdef HAS_BG_EVENT_TRASH_HEAP
    if(event_id == "TRASH_HEAP")
    {
        return bn::regular_bg_items::bg_event_trash_heap;
    }
#endif

#ifdef HAS_BG_EVENT_TRIAL
    if(event_id == "TRIAL")
    {
        return bn::regular_bg_items::bg_event_trial;
    }
#endif

#ifdef HAS_BG_EVENT_UNREST_SITE
    if(event_id == "UNREST_SITE")
    {
        return bn::regular_bg_items::bg_event_unrest_site;
    }
#endif

#ifdef HAS_BG_EVENT_WAR_HISTORIAN_REPY
    if(event_id == "WAR_HISTORIAN_REPY")
    {
        return bn::regular_bg_items::bg_event_war_historian_repy;
    }
#endif

#ifdef HAS_BG_EVENT_WATERLOGGED_SCRIPTORIUM
    if(event_id == "WATERLOGGED_SCRIPTORIUM")
    {
        return bn::regular_bg_items::bg_event_waterlogged_scriptorium;
    }
#endif

#ifdef HAS_BG_EVENT_WELCOME_TO_WONGOS
    if(event_id == "WELCOME_TO_WONGOS")
    {
        return bn::regular_bg_items::bg_event_welcome_to_wongos;
    }
#endif

#ifdef HAS_BG_EVENT_WELLSPRING
    if(event_id == "WELLSPRING")
    {
        return bn::regular_bg_items::bg_event_wellspring;
    }
#endif

#ifdef HAS_BG_EVENT_WHISPERING_HOLLOW
    if(event_id == "WHISPERING_HOLLOW")
    {
        return bn::regular_bg_items::bg_event_whispering_hollow;
    }
#endif

#ifdef HAS_BG_EVENT_WOOD_CARVINGS
    if(event_id == "WOOD_CARVINGS")
    {
        return bn::regular_bg_items::bg_event_wood_carvings;
    }
#endif

#ifdef HAS_BG_EVENT_ZEN_WEAVER
    if(event_id == "ZEN_WEAVER")
    {
        return bn::regular_bg_items::bg_event_zen_weaver;
    }
#endif

#ifdef HAS_BG_EVENT_MYSTIC_ALTAR
    if(event_id == "MYSTIC_ALTAR")
    {
        return bn::regular_bg_items::bg_event_mystic_altar;
    }
#endif

    return bn::regular_bg_items::bg_event_battleworn_dummy;
}

}
