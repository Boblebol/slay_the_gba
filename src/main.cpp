#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_string.h"
#include "bn_sstream.h"
#include "bn_vector.h"
#include "bn_sprite_ptr.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_sprite_text_generator.h"

#include "common_variable_8x8_sprite_font.h"

#include "bn_sprite_items_player_ironclad.h"
#include "bn_sprite_items_player_silent.h"
#include "bn_sprite_items_player_defect.h"
#include "bn_sprite_items_player_regent.h"
#include "bn_sprite_items_player_necrobinder.h"
#include "bn_sprite_items_enemy_cultist.h"
#include "bn_sprite_items_pedestal.h"
#include "bn_sprite_items_map_node_monster.h"
#include "bn_sprite_items_map_node_elite.h"
#include "bn_sprite_items_map_node_rest.h"
#include "bn_sprite_items_map_node_shop.h"
#include "bn_sprite_items_map_node_treasure.h"
#include "bn_sprite_items_map_node_event.h"
#include "bn_sprite_items_map_node_boss.h"
#include "bn_sprite_items_map_player_pawn.h"
#include "bn_sprite_items_map_cursor_select.h"
#include "bn_sprite_items_map_path_dot.h"
#include "bn_sprite_items_map_pawn_ironclad.h"
#include "bn_sprite_items_map_pawn_silent.h"
#include "bn_sprite_items_map_pawn_defect.h"
#include "bn_sprite_items_map_pawn_regent.h"
#include "bn_sprite_items_map_pawn_necrobinder.h"
#include "bn_sprite_items_map_cursor_ironclad.h"
#include "bn_sprite_items_map_cursor_silent.h"
#include "bn_sprite_items_map_cursor_defect.h"
#include "bn_sprite_items_map_cursor_regent.h"
#include "bn_sprite_items_map_cursor_necrobinder.h"

#include "engine/run.h"
#include "engine/navigation.h"
#include "engine/pokemon_battle_ui.h"
#include "engine/event_manager.h"
#include "engine/character.h"

#if defined(HERO_SILENT)
constexpr auto CURRENT_HERO = engine::CharacterId::SILENT;
#define HERO_SPRITE_ITEM bn::sprite_items::player_silent
#define MAP_PAWN_SPRITE_ITEM bn::sprite_items::map_pawn_silent
#define MAP_CURSOR_SPRITE_ITEM bn::sprite_items::map_cursor_silent
#define BG_THEME_COLOR bn::color(1, 5, 2)
#define HERO_ZONE_NAME "MARAIS DU VENIN"
#define HERO_CAMPFIRE_UPGRADE "AFFUTAGE DES DAGUES"

#elif defined(HERO_DEFECT)
constexpr auto CURRENT_HERO = engine::CharacterId::DEFECT;
#define HERO_SPRITE_ITEM bn::sprite_items::player_defect
#define MAP_PAWN_SPRITE_ITEM bn::sprite_items::map_pawn_defect
#define MAP_CURSOR_SPRITE_ITEM bn::sprite_items::map_cursor_defect
#define BG_THEME_COLOR bn::color(1, 2, 7)
#define HERO_ZONE_NAME "COMPLEXE ASTRAL"
#define HERO_CAMPFIRE_UPGRADE "REPROGRAMMATION"

#elif defined(HERO_REGENT)
constexpr auto CURRENT_HERO = engine::CharacterId::REGENT;
#define HERO_SPRITE_ITEM bn::sprite_items::player_regent
#define MAP_PAWN_SPRITE_ITEM bn::sprite_items::map_pawn_regent
#define MAP_CURSOR_SPRITE_ITEM bn::sprite_items::map_cursor_regent
#define BG_THEME_COLOR bn::color(4, 1, 5)
#define HERO_ZONE_NAME "PALAIS SOUVERAIN"
#define HERO_CAMPFIRE_UPGRADE "BENEDICTION ROYALE"

#elif defined(HERO_NECROBINDER)
constexpr auto CURRENT_HERO = engine::CharacterId::NECROBINDER;
#define HERO_SPRITE_ITEM bn::sprite_items::player_necrobinder
#define MAP_PAWN_SPRITE_ITEM bn::sprite_items::map_pawn_necrobinder
#define MAP_CURSOR_SPRITE_ITEM bn::sprite_items::map_cursor_necrobinder
#define BG_THEME_COLOR bn::color(3, 1, 4)
#define HERO_ZONE_NAME "CRYPTE D'OUTRE-TOMBE"
#define HERO_CAMPFIRE_UPGRADE "RITUEL DE RESURRECTION"

#else
constexpr auto CURRENT_HERO = engine::CharacterId::IRONCLAD;
#define HERO_SPRITE_ITEM bn::sprite_items::player_ironclad
#define MAP_PAWN_SPRITE_ITEM bn::sprite_items::map_pawn_ironclad
#define MAP_CURSOR_SPRITE_ITEM bn::sprite_items::map_cursor_ironclad
#define BG_THEME_COLOR bn::color(6, 1, 2)
#define HERO_ZONE_NAME "FORGES DU CARNAGE"
#define HERO_CAMPFIRE_UPGRADE "FORGE INFERNALE"
#endif

namespace {

bn::string_view to_bn(std::string_view sv) {
    return bn::string_view(sv.data(), static_cast<int>(sv.size()));
}

bn::string_view to_bn(const char* str) {
    int len = 0;
    while (str && str[len] != '\0') ++len;
    return bn::string_view(str, len);
}

enum class GameScene : uint8_t {
    TITLE,
    PALETTE_SELECT,
    MAP_NAV,
    DECK_VIEW,
    COMBAT,
    COMBAT_REWARDS,
    EVENT,
    CAMPFIRE,
    SHOP,
    VICTORY,
    GAME_OVER
};

} // namespace

int main()
{
    bn::core::init();
    bn::bg_palettes::set_transparent_color(BG_THEME_COLOR);

    bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
    text_generator.set_center_alignment();

    // Core Engines
    engine::RunManager run;
    engine::MapNavigator nav(run);
    engine::PokemonBattleUI pui;

    GameScene current_scene = GameScene::TITLE;
    GameScene previous_scene = GameScene::MAP_NAV;
    int selected_palette_idx = 0;
    auto palettes = engine::get_character_palettes(CURRENT_HERO);

    // Yu-Gi-Oh Arena Positions: Player Duelist & 1-3 Enemy Monster Zones
    constexpr int PLAYER_BASE_X = -78;
    constexpr int PLAYER_BASE_Y = 4;

    bn::sprite_ptr player_sprite = HERO_SPRITE_ITEM.create_sprite(PLAYER_BASE_X, PLAYER_BASE_Y);
    player_sprite.set_z_order(1);

    bn::sprite_ptr enemy_sprites[3] = {
        bn::sprite_items::enemy_cultist.create_sprite(0, -36),
        bn::sprite_items::enemy_cultist.create_sprite(48, -36),
        bn::sprite_items::enemy_cultist.create_sprite(-48, -36)
    };
    for (int i = 0; i < 3; ++i) {
        enemy_sprites[i].set_z_order(1);
        enemy_sprites[i].set_visible(false);
    }

    bn::sprite_ptr player_pawn = MAP_PAWN_SPRITE_ITEM.create_sprite(0, 0);
    player_pawn.set_z_order(0);
    player_pawn.set_visible(false);

    bn::sprite_ptr cursor_select = MAP_CURSOR_SPRITE_ITEM.create_sprite(0, 0);
    cursor_select.set_z_order(0);
    cursor_select.set_visible(false);

    bn::vector<bn::sprite_ptr, 12> map_node_sprites;
    bn::vector<bn::sprite_ptr, 12> map_dot_sprites;

    int frame_count = 0;
    int reward_card_cursor = 0;
    int event_choice_cursor = 0;
    int campfire_choice_cursor = 0;
    int shop_choice_cursor = 0;
    char event_feedback[80] = {0};

    // Combat Animation States
    int combat_selected_card = 0;
    int combat_selected_enemy = 0;
    bool combat_target_mode = false;
    int anim_player_lunge = 0;
    int anim_player_shield = 0;
    int anim_player_hurt = 0;
    int anim_enemy_hurt[3] = {0, 0, 0};
    int anim_enemy_attack_timer = 0;
    int anim_enemy_attacker_idx = 0;
    int anim_screen_shake = 0;
    int anim_combat_banner_timer = 0;
    char combat_banner_text[48] = {0};

    // Map Walking Hop Animation States
    int anim_pawn_walk_timer = 0;
    int anim_pawn_start_x = 0;
    int anim_pawn_start_y = 0;
    int anim_pawn_target_x = 0;
    int anim_pawn_target_y = 0;

    bn::vector<bn::sprite_ptr, 64> text_sprites;

    while(true)
    {
        ++frame_count;
        text_sprites.clear();
        map_node_sprites.clear();
        map_dot_sprites.clear();

        // -------------------------------------------------------------
        // VISIBILITY OF BATTLE & MAP SPRITES
        // -------------------------------------------------------------
        bool in_battle = (current_scene == GameScene::COMBAT);
        player_sprite.set_visible(in_battle);
        for (int i = 0; i < 3; ++i) {
            bool enemy_active = in_battle && (i < static_cast<int>(run.combat.enemies.size())) && run.combat.enemies[i].is_alive();
            enemy_sprites[i].set_visible(enemy_active);
        }

        bool in_map = (current_scene == GameScene::MAP_NAV);
        player_pawn.set_visible(in_map);
        cursor_select.set_visible(in_map);

        // -------------------------------------------------------------
        // SCENE 1: TITLE SCREEN
        // -------------------------------------------------------------
        if (current_scene == GameScene::TITLE)
        {
            int logo_y = -42 + ((frame_count / 24) % 2 == 0 ? 0 : 1);
            text_generator.generate(0, logo_y, "=== SLAY THE GBA ===", text_sprites);
            const auto& cdef = engine::get_character(CURRENT_HERO);
            text_generator.generate(0, -22, to_bn(cdef.name), text_sprites);
            text_generator.generate(0, -6, HERO_ZONE_NAME, text_sprites);
            if ((frame_count / 25) % 2 == 0) {
                text_generator.generate(0, 16, "Press START ou A pour jouer", text_sprites);
            }
            text_generator.generate(0, 46, "(c) 2026 Mega Crit & Alexandre", text_sprites);

            if (bn::keypad::start_pressed() || bn::keypad::a_pressed()) {
                current_scene = GameScene::PALETTE_SELECT;
            }
        }
        // -------------------------------------------------------------
        // SCENE 2: PALETTE SELECTION (Pokémon Shiny/Shadow/Golden)
        // -------------------------------------------------------------
        else if (current_scene == GameScene::PALETTE_SELECT)
        {
            text_generator.generate(0, -60, "CHOISISSEZ VOTRE TEINTE :", text_sprites);
            for (size_t i = 0; i < palettes.size(); ++i) {
                bn::string<40> p_str;
                bn::ostringstream ss(p_str);
                if (static_cast<int>(i) == selected_palette_idx) ss << "> ";
                else ss << "  ";
                ss << to_bn(palettes[i].name);
                text_generator.generate(0, -25 + static_cast<int>(i) * 20, p_str, text_sprites);
            }
            text_generator.generate(0, 65, "A: Confirmer  Haut/Bas: Choisir", text_sprites);

            if (bn::keypad::up_pressed() && selected_palette_idx > 0) {
                --selected_palette_idx;
            }
            if (bn::keypad::down_pressed() && selected_palette_idx + 1 < static_cast<int>(palettes.size())) {
                ++selected_palette_idx;
            }
            if (bn::keypad::a_pressed() || bn::keypad::start_pressed()) {
                run.start_run(CURRENT_HERO, 0, palettes[selected_palette_idx].variant, 12345);
                nav.reset_to_player_floor();
                current_scene = GameScene::MAP_NAV;
            }
        }
        // -------------------------------------------------------------
        // SCENE 3: MAP NAVIGATION (Mario World Style with Logos & Pawn)
        // -------------------------------------------------------------
        else if (current_scene == GameScene::MAP_NAV)
        {
            // Thematic Act & Realm Name Banner
            text_generator.generate(0, -70, HERO_ZONE_NAME, text_sprites);

            // Header Slay the Spire / Mario World HUD
            bn::string<40> h_info;
            bn::ostringstream h_ss(h_info);
            h_ss << "ETG " << nav.get_current_floor() << "/16 | PV " << run.hp << "/" << run.max_hp 
                 << " | " << run.gold << " G | " << run.master_deck.size() << " C";
            text_generator.generate(0, -58, h_info, text_sprites);

            // Display 3 floors centered on nav.scroll_floor
            int cur_f = nav.scroll_floor;
            int f_start = std::max(0, cur_f - 1);
            int f_end = std::min(15, f_start + 2);

            // Mario World Sprite Grid (3 floors, 3 lanes)
            for (int f = f_end; f >= f_start; --f) {
                int floor_y = 22;
                if (f == f_end) floor_y = -34;
                else if (f == f_end - 1) floor_y = -6;

                // Path dots between floors (Mario stone road)
                if (f < f_end) {
                    int dot_y = (floor_y == 22) ? 8 : -20;
                    map_dot_sprites.push_back(bn::sprite_items::map_path_dot.create_sprite(-50, dot_y));
                    map_dot_sprites.push_back(bn::sprite_items::map_path_dot.create_sprite(0, dot_y));
                    map_dot_sprites.push_back(bn::sprite_items::map_path_dot.create_sprite(50, dot_y));
                }

                if (f == 15) {
                    int boss_pulse = ((frame_count / 14) % 2 == 0) ? 0 : 1;
                    map_node_sprites.push_back(bn::sprite_items::map_node_boss.create_sprite(0, floor_y - boss_pulse));
                } else {
                    for (int l = 0; l < 3; ++l) {
                        int lane_x = -50 + l * 50;
                        for (size_t i = 0; i < run.map.nodes.size(); ++i) {
                            if (run.map.nodes[i].floor == f && run.map.nodes[i].lane == l) {
                                switch (run.map.nodes[i].type) {
                                    case engine::RoomType::MONSTER:
                                        map_node_sprites.push_back(bn::sprite_items::map_node_monster.create_sprite(lane_x, floor_y));
                                        break;
                                    case engine::RoomType::ELITE:
                                        map_node_sprites.push_back(bn::sprite_items::map_node_elite.create_sprite(lane_x, floor_y));
                                        break;
                                    case engine::RoomType::REST_SITE:
                                        map_node_sprites.push_back(bn::sprite_items::map_node_rest.create_sprite(lane_x, floor_y));
                                        break;
                                    case engine::RoomType::SHOP:
                                        map_node_sprites.push_back(bn::sprite_items::map_node_shop.create_sprite(lane_x, floor_y));
                                        break;
                                    case engine::RoomType::TREASURE:
                                        map_node_sprites.push_back(bn::sprite_items::map_node_treasure.create_sprite(lane_x, floor_y));
                                        break;
                                    case engine::RoomType::EVENT:
                                    default:
                                        map_node_sprites.push_back(bn::sprite_items::map_node_event.create_sprite(lane_x, floor_y));
                                        break;
                                }
                                break;
                            }
                        }
                    }
                }
            }

            // Mario Player Pawn: Chibi avatar with animated walking hop or idle breathing
            int bounce_phase = (frame_count / 8) % 4;
            int idle_bounce = (bounce_phase == 0) ? 0 : (bounce_phase == 1) ? 2 : (bounce_phase == 2) ? 3 : 1;

            if (anim_pawn_walk_timer > 0) {
                --anim_pawn_walk_timer;
                int progress = 18 - anim_pawn_walk_timer; // 1 to 18
                int cur_px = anim_pawn_start_x + (anim_pawn_target_x - anim_pawn_start_x) * progress / 18;
                int cur_py = anim_pawn_start_y + (anim_pawn_target_y - anim_pawn_start_y) * progress / 18;
                // 3 hops along the path
                int hop_arc = (progress % 6 < 3) ? 5 : 0;
                player_pawn.set_position(cur_px, cur_py - 12 - hop_arc);
                player_pawn.set_visible(true);

                if (anim_pawn_target_x < anim_pawn_start_x) player_pawn.set_horizontal_flip(true);
                else player_pawn.set_horizontal_flip(false);

                if (anim_pawn_walk_timer == 0) {
                    player_pawn.set_horizontal_flip(false);
                    nav.select_current_route();
                    if (run.phase == engine::RunPhase::IN_COMBAT) {
                        combat_selected_card = 0;
                        combat_selected_enemy = run.combat.get_first_alive_enemy_index();
                        combat_target_mode = false;
                        current_scene = GameScene::COMBAT;
                    } else if (run.phase == engine::RunPhase::EVENT) {
                        event_choice_cursor = 0;
                        event_feedback[0] = '\0';
                        current_scene = GameScene::EVENT;
                    } else if (run.phase == engine::RunPhase::CAMPFIRE) {
                        campfire_choice_cursor = 0;
                        current_scene = GameScene::CAMPFIRE;
                    } else if (run.phase == engine::RunPhase::SHOP) {
                        shop_choice_cursor = 0;
                        current_scene = GameScene::SHOP;
                    } else if (run.phase == engine::RunPhase::TREASURE) {
                        current_scene = GameScene::MAP_NAV;
                    }
                }
            } else {
                player_pawn.set_horizontal_flip(false);
                if (run.current_node_index >= 0 && static_cast<size_t>(run.current_node_index) < run.map.nodes.size()) {
                    const auto& cur_n = run.map.nodes[run.current_node_index];
                    if (cur_n.floor >= f_start && cur_n.floor <= f_end) {
                        int p_x = -50 + cur_n.lane * 50;
                        int p_y = (cur_n.floor == f_end) ? -34 : (cur_n.floor == f_end - 1) ? -6 : 22;
                        player_pawn.set_position(p_x, p_y - 12 - idle_bounce);
                        player_pawn.set_visible(true);
                    } else {
                        player_pawn.set_visible(false);
                    }
                } else {
                    // Entrance to the Spire
                    player_pawn.set_position(0, 36 - idle_bounce);
                    player_pawn.set_visible(true);
                }
            }

            // Mario Selector Ring: Pulsing indicator on the chosen target route!
            int target_idx = nav.get_selected_target_node();
            if (target_idx >= 0 && static_cast<size_t>(target_idx) < run.map.nodes.size()) {
                const auto& tgt_n = run.map.nodes[target_idx];
                if (tgt_n.floor >= f_start && tgt_n.floor <= f_end) {
                    int t_x = (tgt_n.floor == 15) ? 0 : (-50 + tgt_n.lane * 50);
                    int t_y = (tgt_n.floor == f_end) ? -34 : (tgt_n.floor == f_end - 1) ? -6 : 22;
                    int pulse_offset = ((frame_count / 6) % 4 == 1) ? -1 : ((frame_count / 6) % 4 == 3) ? 1 : 0;
                    cursor_select.set_position(t_x, t_y + pulse_offset);
                    cursor_select.set_visible(true);
                } else {
                    cursor_select.set_visible(false);
                }

                // Active Route Description Banner
                bn::string<48> r_str;
                bn::ostringstream rss(r_str);
                rss << ">>> " << to_bn(run.map.get_room_icon(tgt_n.type)) << " "
                    << to_bn(run.map.get_room_name(tgt_n.type)) << " (" 
                    << (tgt_n.lane == 0 ? "Gauche" : tgt_n.lane == 1 ? "Centre" : "Droite") << ") <<<";
                text_generator.generate(0, 48, r_str, text_sprites);
            } else {
                cursor_select.set_visible(false);
            }

            text_generator.generate(0, 65, "A: ENTRER EN SALLE   SEL: DECK", text_sprites);

            if (anim_pawn_walk_timer == 0) {
                if (bn::keypad::up_pressed()) nav.scroll_up();
                if (bn::keypad::down_pressed()) nav.scroll_down();
                if (bn::keypad::left_pressed()) nav.prev_route();
                if (bn::keypad::right_pressed()) nav.next_route();

                if (bn::keypad::select_pressed()) {
                    previous_scene = GameScene::MAP_NAV;
                    nav.toggle_deck_view();
                    current_scene = GameScene::DECK_VIEW;
                }

                if (bn::keypad::a_pressed()) {
                    if (target_idx >= 0 && static_cast<size_t>(target_idx) < run.map.nodes.size()) {
                        const auto& tgt_n = run.map.nodes[target_idx];
                        anim_pawn_target_x = (tgt_n.floor == 15) ? 0 : (-50 + tgt_n.lane * 50);
                        anim_pawn_target_y = (tgt_n.floor == f_end) ? -34 : (tgt_n.floor == f_end - 1) ? -6 : 22;

                        if (run.current_node_index >= 0 && static_cast<size_t>(run.current_node_index) < run.map.nodes.size()) {
                            const auto& cur_n = run.map.nodes[run.current_node_index];
                            anim_pawn_start_x = -50 + cur_n.lane * 50;
                            anim_pawn_start_y = (cur_n.floor == f_end) ? -34 : (cur_n.floor == f_end - 1) ? -6 : 22;
                        } else {
                            anim_pawn_start_x = 0;
                            anim_pawn_start_y = 36;
                        }
                        anim_pawn_walk_timer = 18;
                    } else {
                        nav.select_current_route();
                        if (run.phase == engine::RunPhase::IN_COMBAT) {
                            combat_selected_card = 0;
                            combat_selected_enemy = run.combat.get_first_alive_enemy_index();
                            combat_target_mode = false;
                            current_scene = GameScene::COMBAT;
                        } else if (run.phase == engine::RunPhase::EVENT) {
                            event_choice_cursor = 0;
                            event_feedback[0] = '\0';
                            current_scene = GameScene::EVENT;
                        } else if (run.phase == engine::RunPhase::CAMPFIRE) {
                            campfire_choice_cursor = 0;
                            current_scene = GameScene::CAMPFIRE;
                        } else if (run.phase == engine::RunPhase::SHOP) {
                            shop_choice_cursor = 0;
                            current_scene = GameScene::SHOP;
                        } else if (run.phase == engine::RunPhase::TREASURE) {
                            current_scene = GameScene::MAP_NAV;
                        }
                    }
                }
            }
        }
        // -------------------------------------------------------------
        // SCENE 4: DECK INSPECTOR OVERLAY
        // -------------------------------------------------------------
        else if (current_scene == GameScene::DECK_VIEW)
        {
            text_generator.generate(0, -70, "=== REGISTRE DU DECK ===", text_sprites);
            bn::string<40> d_stat;
            bn::ostringstream dss(d_stat);
            dss << run.master_deck.size() << " Cartes  |  " << run.relics.size() << " Reliques";
            text_generator.generate(0, -55, d_stat, text_sprites);

            int start_idx = nav.deck_scroll_index;
            for (int i = 0; i < 4 && (start_idx + i) < static_cast<int>(run.master_deck.size()); ++i) {
                engine::DeckCard dc = run.master_deck[start_idx + i];
                bn::string<48> c_line;
                bn::ostringstream css(c_line);
                css << (start_idx + i + 1) << ". " 
                    << to_bn(engine::CardDef::get_display_name(dc.id, dc.upgraded))
                    << " (" << static_cast<int>(engine::CardDef::get_cost(dc.id, dc.upgraded)) << "E)";
                text_generator.generate(0, -30 + i * 18, c_line, text_sprites);
            }

            text_generator.generate(0, 52, "Haut/Bas: Defiler les cartes", text_sprites);
            text_generator.generate(0, 68, "B ou SELECT: Retour", text_sprites);

            if (bn::keypad::up_pressed()) nav.deck_scroll_up();
            if (bn::keypad::down_pressed()) nav.deck_scroll_down();
            if (bn::keypad::b_pressed() || bn::keypad::select_pressed()) {
                current_scene = previous_scene;
            }
        }
        // -------------------------------------------------------------
        // SCENE 5: YU-GI-OH STYLE GBA DUEL ARENA (1 to 3 Enemies)
        // -------------------------------------------------------------
        else if (current_scene == GameScene::COMBAT)
        {
            // Screen Shake Calculation
            int shake_x = 0;
            int shake_y = 0;
            if (anim_screen_shake > 0) {
                shake_x = (anim_screen_shake % 4 < 2) ? 3 : -3;
                shake_y = (anim_screen_shake % 3 == 0) ? 2 : -2;
                --anim_screen_shake;
            }

            // Position enemy monster zones at top
            int num_enemies = static_cast<int>(run.combat.enemies.size());
            for (int i = 0; i < num_enemies && i < 3; ++i) {
                int base_x = 0;
                if (num_enemies == 2) {
                    base_x = (i == 0) ? -48 : 48;
                } else if (num_enemies >= 3) {
                    base_x = -62 + i * 62;
                }

                int enemy_hover = (((frame_count + i * 13) / 14) % 2 == 0) ? -1 : 1;
                int cur_x = base_x + shake_x;
                int cur_y = -36 + enemy_hover + shake_y;

                if (anim_enemy_hurt[i] > 0) {
                    cur_x += (anim_enemy_hurt[i] % 4 < 2) ? 4 : -4;
                    cur_y -= (anim_enemy_hurt[i] > 8) ? 3 : 0;
                    bool flash_vis = (anim_enemy_hurt[i] % 4 < 3);
                    enemy_sprites[i].set_visible(run.combat.enemies[i].is_alive() && flash_vis);
                    --anim_enemy_hurt[i];
                }

                // Enemy Attack Lunge (during enemy attack animation)
                if (anim_enemy_attack_timer > 0 && anim_enemy_attacker_idx == i) {
                    if (anim_enemy_attack_timer > 14) {
                        int progress = 28 - anim_enemy_attack_timer; // 0 to 14
                        cur_x += -progress * 2;
                        cur_y += progress * 2;
                    } else {
                        int progress = anim_enemy_attack_timer; // 14 to 0
                        cur_x += -progress * 2;
                        cur_y += progress * 2;
                    }
                }

                enemy_sprites[i].set_position(cur_x, cur_y);

                if (run.combat.enemies[i].is_alive()) {
                    // Target indicator
                    if (combat_target_mode && combat_selected_enemy == i) {
                        text_generator.generate(base_x, -68, "[V]", text_sprites);
                    }

                    // Enemy Name & HP
                    bn::string<32> e_stat;
                    bn::ostringstream ess(e_stat);
                    ess << to_bn(run.combat.enemies[i].name) << " " << run.combat.enemies[i].hp << "/" << run.combat.enemies[i].max_hp;
                    if (run.combat.enemies[i].block > 0) ess << "+" << run.combat.enemies[i].block << "B";
                    text_generator.generate(base_x, -56, e_stat, text_sprites);

                    // Enemy Intent Badge
                    bn::string<32> i_stat;
                    bn::ostringstream iss(i_stat);
                    if (run.combat.enemies[i].current_intent.type == engine::EnemyIntentType::ATTACK) {
                        iss << "! ATK " << run.combat.enemies[i].current_intent.base_damage;
                    } else if (run.combat.enemies[i].current_intent.type == engine::EnemyIntentType::ATTACK_DEFEND) {
                        iss << "! " << run.combat.enemies[i].current_intent.base_damage << "D+" << run.combat.enemies[i].current_intent.block << "B";
                    } else {
                        iss << "+ " << to_bn(run.combat.enemies[i].current_intent.description);
                    }
                    text_generator.generate(base_x, -20, i_stat, text_sprites);
                }
            }

            // Player Duelist Middle Zone with Breathing & Reactions
            int player_breathing = ((frame_count / 14) % 2 == 0) ? 0 : 1;
            int cur_player_x = PLAYER_BASE_X + shake_x;
            int cur_player_y = PLAYER_BASE_Y + player_breathing + shake_y;

            if (anim_player_lunge > 0) {
                if (anim_player_lunge > 9) {
                    cur_player_x += (18 - anim_player_lunge) * 5;
                } else {
                    cur_player_x += anim_player_lunge * 3;
                }
                --anim_player_lunge;
            }

            if (anim_player_shield > 0) {
                cur_player_x -= (anim_player_shield > 8) ? 4 : 2;
                --anim_player_shield;
            }

            if (anim_player_hurt > 0) {
                cur_player_x -= (anim_player_hurt % 4 < 2) ? 5 : -2;
                bool flash_vis = (anim_player_hurt % 4 < 3);
                player_sprite.set_visible(flash_vis);
                --anim_player_hurt;
            } else {
                player_sprite.set_visible(true);
            }

            player_sprite.set_position(cur_player_x, cur_player_y);

            // Player Stats
            bn::string<40> p_stat;
            bn::ostringstream pss(p_stat);
            pss << to_bn(engine::get_character(CURRENT_HERO).name) << " PV " 
                << run.combat.player.hp << "/" << run.combat.player.max_hp;
            if (run.combat.player.block > 0) pss << " (+" << run.combat.player.block << "B)";
            pss << " | NRG " << static_cast<int>(run.combat.player.energy) << "/" << static_cast<int>(run.combat.player.max_energy);
            text_generator.generate(24, -2, p_stat, text_sprites);

            // Hero Resource
            bn::string<40> r_stat;
            bn::ostringstream rss(r_stat);
            if (CURRENT_HERO == engine::CharacterId::DEFECT) {
                rss << "Orbes: " << run.combat.player.orbs.size() << "/3";
            } else if (CURRENT_HERO == engine::CharacterId::REGENT) {
                rss << "Etoiles: " << static_cast<int>(run.combat.player.sovereign_stars) << "/5";
            } else if (CURRENT_HERO == engine::CharacterId::NECROBINDER) {
                rss << "Ames: " << static_cast<int>(run.combat.player.souls) << " | Osty: " << run.combat.player.minion_hp;
            } else if (CURRENT_HERO == engine::CharacterId::SILENT) {
                int poi = (combat_selected_enemy >= 0 && combat_selected_enemy < static_cast<int>(run.combat.enemies.size()))
                    ? run.combat.enemies[combat_selected_enemy].get_status(engine::StatusType::POISON) : 0;
                int dex = run.combat.player.get_status(engine::StatusType::DEXTERITY);
                rss << "Venin: " << poi << " | Dex: " << (dex >= 0 ? "+" : "") << dex;
            } else {
                int str = run.combat.player.get_status(engine::StatusType::STRENGTH);
                rss << "Rage/Force: " << (str >= 0 ? "+" : "") << str;
            }
            text_generator.generate(24, 10, r_stat, text_sprites);

            // Floating Action Feedback Banner
            if (anim_combat_banner_timer > 0) {
                --anim_combat_banner_timer;
                int banner_hop = (anim_combat_banner_timer % 6 < 3) ? -1 : 1;
                text_generator.generate(0, 14 + banner_hop, to_bn(combat_banner_text), text_sprites);
            }

            // Yu-Gi-Oh Hand Tray at Bottom
            if (!run.combat.hand.empty() && combat_selected_card < static_cast<int>(run.combat.hand.size())) {
                engine::DeckCard dc = run.combat.hand[combat_selected_card];
                
                // Preview Line 1: Animated Arrow & Cost
                bn::string<40> c_prev;
                bn::ostringstream cps(c_prev);
                cps << ((frame_count / 10) % 2 == 0 ? "▲ " : "► ")
                    << "[" << (combat_selected_card + 1) << "] " 
                    << to_bn(engine::CardDef::get_display_name(dc.id, dc.upgraded))
                    << " (" << static_cast<int>(engine::CardDef::get_cost(dc.id, dc.upgraded)) << "E)";
                text_generator.generate(0, 25, c_prev, text_sprites);

                // Preview Line 2: Description
                text_generator.generate(0, 37, to_bn(engine::CardDef::get_desc(dc.id, dc.upgraded)), text_sprites);

                // Hand Slots with Animated Stars
                bn::string<40> h_slots;
                bn::ostringstream hss(h_slots);
                hss << "Main: ";
                for (size_t j = 0; j < run.combat.hand.size(); ++j) {
                    if (static_cast<int>(j) == combat_selected_card) {
                        hss << ((frame_count / 8) % 2 == 0 ? "★[" : "☆[") << (j + 1) << "]★ ";
                    } else {
                        hss << "[" << (j + 1) << "] ";
                    }
                }
                text_generator.generate(0, 51, h_slots, text_sprites);
            }

            // Command Prompt
            if (combat_target_mode) {
                text_generator.generate(0, 66, "G/D: Cible  A: ATTAQUER  B: Annuler", text_sprites);
            } else if (anim_enemy_attack_timer > 0) {
                text_generator.generate(0, 66, ">>> TOUR ENNEMI EN COURS... <<<", text_sprites);
            } else {
                text_generator.generate(0, 66, "G/D: Carte  A: Jouer  START: Fin Tour", text_sprites);
            }

            // Animated Enemy Turn Sequence
            if (anim_enemy_attack_timer > 0) {
                --anim_enemy_attack_timer;
                if (anim_enemy_attack_timer == 14) {
                    run.combat.execute_enemy_turn();
                    anim_player_hurt = 16;
                    anim_screen_shake = 8;
                    anim_combat_banner_timer = 24;
                    const char* warn_msg = ">> ATTAQUE ENNEMIE ! <<";
                    int idx = 0;
                    while (warn_msg[idx] != '\0' && idx < 47) {
                        combat_banner_text[idx] = warn_msg[idx];
                        ++idx;
                    }
                    combat_banner_text[idx] = '\0';
                }
                if (anim_enemy_attack_timer == 0) {
                    if (run.combat.player.is_alive() && run.combat.has_alive_enemies()) {
                        run.combat.start_player_turn();
                        combat_selected_card = 0;
                        combat_selected_enemy = run.combat.get_first_alive_enemy_index();
                        combat_target_mode = false;
                    }
                }
            }
            // Player Input Handling (only when not in enemy attack animation)
            else if (combat_target_mode) {
                if (bn::keypad::left_pressed() || bn::keypad::up_pressed()) {
                    for (int step = 1; step <= 3; ++step) {
                        int c = (combat_selected_enemy - step + 3) % 3;
                        if (c < static_cast<int>(run.combat.enemies.size()) && run.combat.enemies[c].is_alive()) {
                            combat_selected_enemy = c;
                            break;
                        }
                    }
                }
                if (bn::keypad::right_pressed() || bn::keypad::down_pressed()) {
                    for (int step = 1; step <= 3; ++step) {
                        int c = (combat_selected_enemy + step) % 3;
                        if (c < static_cast<int>(run.combat.enemies.size()) && run.combat.enemies[c].is_alive()) {
                            combat_selected_enemy = c;
                            break;
                        }
                    }
                }
                if (bn::keypad::b_pressed()) {
                    combat_target_mode = false;
                }
                if (bn::keypad::a_pressed()) {
                    bool played = run.combat.play_card(combat_selected_card, combat_selected_enemy);
                    if (played) {
                        anim_player_lunge = 18;
                        anim_screen_shake = 6;
                        anim_enemy_hurt[combat_selected_enemy] = 18;
                        anim_combat_banner_timer = 24;
                        const char* atk_msg = (CURRENT_HERO == engine::CharacterId::IRONCLAD) ? ">> FRAPPE DU SANG ! <<" :
                                              (CURRENT_HERO == engine::CharacterId::SILENT) ? ">> DAGUE DU VENIN ! <<" :
                                              (CURRENT_HERO == engine::CharacterId::DEFECT) ? ">> IMPACT STATIQUE ! <<" :
                                              (CURRENT_HERO == engine::CharacterId::REGENT) ? ">> SENTENCE ROYALE ! <<" :
                                              ">> TRANCHE SPECTRALE ! <<";
                        int idx = 0;
                        while (atk_msg[idx] != '\0' && idx < 47) { combat_banner_text[idx] = atk_msg[idx]; ++idx; }
                        combat_banner_text[idx] = '\0';

                        combat_target_mode = false;
                        if (combat_selected_card >= static_cast<int>(run.combat.hand.size())) {
                            combat_selected_card = std::max(0, static_cast<int>(run.combat.hand.size()) - 1);
                        }
                        combat_selected_enemy = run.combat.get_first_alive_enemy_index();
                    }
                }
            } else {
                if (bn::keypad::left_pressed() && combat_selected_card > 0) {
                    --combat_selected_card;
                }
                if (bn::keypad::right_pressed() && combat_selected_card + 1 < static_cast<int>(run.combat.hand.size())) {
                    ++combat_selected_card;
                }
                if (bn::keypad::select_pressed()) {
                    previous_scene = GameScene::COMBAT;
                    nav.toggle_deck_view();
                    current_scene = GameScene::DECK_VIEW;
                }
                if (bn::keypad::start_pressed() || bn::keypad::r_pressed()) {
                    run.combat.end_player_turn();
                    anim_enemy_attack_timer = 28;
                    anim_enemy_attacker_idx = run.combat.get_first_alive_enemy_index();
                    combat_target_mode = false;
                }
                if (bn::keypad::a_pressed() && !run.combat.hand.empty()) {
                    if (run.combat.can_play_card(combat_selected_card)) {
                        engine::DeckCard cur_card = run.combat.hand[combat_selected_card];
                        const auto& cdef = engine::CardDef::get(cur_card.id);
                        if (cdef.type == engine::CardType::ATTACK && cdef.target == engine::TargetType::SINGLE_ENEMY && run.combat.count_alive_enemies() > 1) {
                            combat_target_mode = true;
                            combat_selected_enemy = run.combat.get_first_alive_enemy_index();
                        } else {
                            int tgt = run.combat.get_first_alive_enemy_index();
                            bool played = run.combat.play_card(combat_selected_card, tgt);
                            if (played) {
                                if (cdef.type == engine::CardType::ATTACK) {
                                    anim_player_lunge = 18;
                                    anim_screen_shake = 6;
                                    anim_combat_banner_timer = 24;
                                    if (cdef.target == engine::TargetType::ALL_ENEMIES) {
                                        for (int k = 0; k < 3; ++k) anim_enemy_hurt[k] = 18;
                                    } else if (tgt >= 0 && tgt < 3) {
                                        anim_enemy_hurt[tgt] = 18;
                                    }
                                    const char* atk_msg = (CURRENT_HERO == engine::CharacterId::IRONCLAD) ? ">> FRAPPE DU SANG ! <<" :
                                                          (CURRENT_HERO == engine::CharacterId::SILENT) ? ">> DAGUE DU VENIN ! <<" :
                                                          (CURRENT_HERO == engine::CharacterId::DEFECT) ? ">> IMPACT STATIQUE ! <<" :
                                                          (CURRENT_HERO == engine::CharacterId::REGENT) ? ">> SENTENCE ROYALE ! <<" :
                                                          ">> TRANCHE SPECTRALE ! <<";
                                    int idx = 0;
                                    while (atk_msg[idx] != '\0' && idx < 47) { combat_banner_text[idx] = atk_msg[idx]; ++idx; }
                                    combat_banner_text[idx] = '\0';
                                } else if (cdef.type == engine::CardType::SKILL) {
                                    anim_player_shield = 16;
                                    anim_combat_banner_timer = 24;
                                    const char* def_msg = ">> BOUCLIER ACTIF ! <<";
                                    int idx = 0;
                                    while (def_msg[idx] != '\0' && idx < 47) { combat_banner_text[idx] = def_msg[idx]; ++idx; }
                                    combat_banner_text[idx] = '\0';
                                } else {
                                    anim_combat_banner_timer = 24;
                                    const char* pow_msg = ">> POUVOIR EVEILLE ! <<";
                                    int idx = 0;
                                    while (pow_msg[idx] != '\0' && idx < 47) { combat_banner_text[idx] = pow_msg[idx]; ++idx; }
                                    combat_banner_text[idx] = '\0';
                                }

                                if (combat_selected_card >= static_cast<int>(run.combat.hand.size())) {
                                    combat_selected_card = std::max(0, static_cast<int>(run.combat.hand.size()) - 1);
                                }
                                combat_selected_enemy = run.combat.get_first_alive_enemy_index();
                            }
                        }
                    }
                }
            }

            if (run.combat.state == engine::CombatState::VICTORY) {
                run.proceed_after_combat();
                reward_card_cursor = 0;
                current_scene = GameScene::COMBAT_REWARDS;
            } else if (run.combat.state == engine::CombatState::DEFEAT) {
                current_scene = GameScene::GAME_OVER;
            }
        }
        // -------------------------------------------------------------
        // SCENE 6: COMBAT REWARDS SCREEN
        // -------------------------------------------------------------
        else if (current_scene == GameScene::COMBAT_REWARDS)
        {
            text_generator.generate(0, -60, "=== VICTOIRE ! ===", text_sprites);
            bn::string<40> g_str;
            bn::ostringstream gss(g_str);
            gss << "Or obtenu: +" << run.gold_reward << " Pieces";
            text_generator.generate(0, -40, g_str, text_sprites);

            text_generator.generate(0, -20, "Choisissez une carte :", text_sprites);
            for (size_t i = 0; i < run.current_card_rewards.size(); ++i) {
                engine::DeckCard rc = run.current_card_rewards[i];
                bn::string<40> rc_str;
                bn::ostringstream rcss(rc_str);
                if (static_cast<int>(i) == reward_card_cursor) rcss << "> ";
                else rcss << "  ";
                rcss << to_bn(engine::CardDef::get_display_name(rc.id, rc.upgraded));
                text_generator.generate(0, 0 + static_cast<int>(i) * 18, rc_str, text_sprites);
            }

            text_generator.generate(0, 65, "A: Choisir  START: Passer", text_sprites);

            if (bn::keypad::up_pressed() && reward_card_cursor > 0) --reward_card_cursor;
            if (bn::keypad::down_pressed() && reward_card_cursor + 1 < static_cast<int>(run.current_card_rewards.size())) ++reward_card_cursor;

            if (bn::keypad::a_pressed()) {
                run.pick_card_reward(reward_card_cursor);
                current_scene = GameScene::MAP_NAV;
            } else if (bn::keypad::start_pressed()) {
                run.skip_card_rewards();
                current_scene = GameScene::MAP_NAV;
            }
        }
        // -------------------------------------------------------------
        // SCENE 7: POKÉMON-STYLE EVENT ENCOUNTER
        // -------------------------------------------------------------
        else if (current_scene == GameScene::EVENT)
        {
            text_generator.generate(0, -65, to_bn(run.current_event.title), text_sprites);

            if (event_feedback[0] == '\0') {
                for (size_t i = 0; i < run.current_event.choices.size(); ++i) {
                    bn::string<48> ec_str;
                    bn::ostringstream ecss(ec_str);
                    if (static_cast<int>(i) == event_choice_cursor) ecss << "> ";
                    else ecss << "  ";
                    ecss << to_bn(run.current_event.choices[i].label);
                    text_generator.generate(0, -20 + static_cast<int>(i) * 22, ec_str, text_sprites);
                }
                text_generator.generate(0, 65, "A: Choisir  Haut/Bas: Naviguer", text_sprites);

                if (bn::keypad::up_pressed() && event_choice_cursor > 0) --event_choice_cursor;
                if (bn::keypad::down_pressed() && event_choice_cursor + 1 < static_cast<int>(run.current_event.choices.size())) ++event_choice_cursor;

                if (bn::keypad::a_pressed()) {
                    run.choose_event_choice(event_choice_cursor, event_feedback, sizeof(event_feedback));
                }
            } else {
                text_generator.generate(0, 0, to_bn(event_feedback), text_sprites);
                text_generator.generate(0, 50, "Appuyez sur A pour continuer", text_sprites);
                if (bn::keypad::a_pressed()) {
                    current_scene = GameScene::MAP_NAV;
                }
            }
        }
        // -------------------------------------------------------------
        // SCENE 8: CAMPFIRE
        // -------------------------------------------------------------
        else if (current_scene == GameScene::CAMPFIRE)
        {
            text_generator.generate(0, -60, ((frame_count / 15) % 2 == 0 ? "=== ! FEU DE CAMP ! ===" : "=== * FEU DE CAMP * ==="), text_sprites);
            bn::string<40> c1;
            bn::ostringstream c1ss(c1);
            c1ss << (campfire_choice_cursor == 0 ? "> [1] REPOS (+30% PV)" : "  [1] REPOS (+30% PV)");
            text_generator.generate(0, -20, c1, text_sprites);

            bn::string<40> c2;
            bn::ostringstream c2ss(c2);
            c2ss << (campfire_choice_cursor == 1 ? "> [2] " : "  [2] ") << HERO_CAMPFIRE_UPGRADE;
            text_generator.generate(0, 10, c2, text_sprites);

            text_generator.generate(0, 65, "A: Confirmer  Haut/Bas: Choisir", text_sprites);

            if (bn::keypad::up_pressed() || bn::keypad::down_pressed()) {
                campfire_choice_cursor = 1 - campfire_choice_cursor;
            }
            if (bn::keypad::a_pressed()) {
                if (campfire_choice_cursor == 0) {
                    run.rest_at_campfire();
                } else {
                    for (size_t i = 0; i < run.master_deck.size(); ++i) {
                        if (!run.master_deck[i].upgraded) {
                            run.smith_at_campfire(static_cast<int>(i));
                            break;
                        }
                    }
                }
                current_scene = GameScene::MAP_NAV;
            }
        }
        // -------------------------------------------------------------
        // SCENE 9: SHOP
        // -------------------------------------------------------------
        else if (current_scene == GameScene::SHOP)
        {
            text_generator.generate(0, -65, ((frame_count / 15) % 2 == 0 ? "=== $ MARCHAND DE LA TOUR $ ===" : "=== * MARCHAND DE LA TOUR * ==="), text_sprites);
            bn::string<32> g_str;
            bn::ostringstream gss(g_str);
            gss << "Or disponible: " << run.gold << " G";
            text_generator.generate(0, -45, g_str, text_sprites);

            text_generator.generate(0, -15, (shop_choice_cursor == 0 ? "> [1] Acheter Carte" : "  [1] Acheter Carte"), text_sprites);
            text_generator.generate(0, 8, (shop_choice_cursor == 1 ? "> [2] Retirer Carte (75G)" : "  [2] Retirer Carte (75G)"), text_sprites);
            text_generator.generate(0, 31, (shop_choice_cursor == 2 ? "> [3] Quitter Boutique" : "  [3] Quitter Boutique"), text_sprites);

            if (bn::keypad::up_pressed() && shop_choice_cursor > 0) --shop_choice_cursor;
            if (bn::keypad::down_pressed() && shop_choice_cursor < 2) ++shop_choice_cursor;

            if (bn::keypad::a_pressed()) {
                if (shop_choice_cursor == 0 && !run.current_shop.cards.empty()) {
                    run.buy_shop_card(0);
                    run.leave_shop();
                    current_scene = GameScene::MAP_NAV;
                } else if (shop_choice_cursor == 1 && run.master_deck.size() > 1) {
                    run.remove_card_at_shop(0);
                    run.leave_shop();
                    current_scene = GameScene::MAP_NAV;
                } else {
                    run.leave_shop();
                    current_scene = GameScene::MAP_NAV;
                }
            }
        }
        // -------------------------------------------------------------
        // SCENE 10: VICTORY & GAME OVER
        // -------------------------------------------------------------
        else if (current_scene == GameScene::VICTORY)
        {
            player_sprite.set_visible(true);
            int bounce_y = abs((frame_count % 30) - 15) / 2;
            player_sprite.set_position(0, -5 - bounce_y);
            player_sprite.set_horizontal_flip((frame_count / 15) % 2 == 0);

            text_generator.generate(0, -45, ((frame_count / 12) % 2 == 0 ? "=== VICTOIRE TOTALE ! ===" : "* VICTOIRE TOTALE ! *"), text_sprites);
            text_generator.generate(0, 16, "Vous avez triomphe de la Tour !", text_sprites);
            if ((frame_count / 25) % 2 == 0) {
                text_generator.generate(0, 42, "Press START pour rejouer", text_sprites);
            }
            if (bn::keypad::start_pressed()) {
                player_sprite.set_horizontal_flip(false);
                current_scene = GameScene::TITLE;
            }
        }
        else if (current_scene == GameScene::GAME_OVER)
        {
            player_sprite.set_visible(true);
            player_sprite.set_position(0, 4 + ((frame_count / 20) % 2 == 0 ? 0 : 1));

            text_generator.generate(0, -35, "=== DEFAITE DANS LA TOUR ===", text_sprites);
            text_generator.generate(0, -10, "Votre aventure s'arrete ici...", text_sprites);
            if ((frame_count / 25) % 2 == 0) {
                text_generator.generate(0, 35, "Press START pour reessayer", text_sprites);
            }
            if (bn::keypad::start_pressed()) current_scene = GameScene::TITLE;
        }

        bn::core::update();
    }
}
