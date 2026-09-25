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

#include "bn_sprite_items_player_back.h"
#include "bn_sprite_items_enemy_cultist.h"
#include "bn_sprite_items_pedestal.h"

#include "engine/combat.h"

namespace {

bn::string_view to_bn(std::string_view sv) {
    return bn::string_view(sv.data(), sv.size());
}

} // namespace

int main()
{
    bn::core::init();

    // Dark Spire dungeon atmosphere (deep midnight slate blue)
    bn::bg_palettes::set_transparent_color(bn::color(3, 4, 6));

    bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
    text_generator.set_center_alignment();

    engine::CombatEngine combat;
    combat.start_combat(engine::EnemyType::CULTIST);

    // Pokémon-style base positions
    // Player: bottom-left (viewed from behind 3/4)
    constexpr int PLAYER_BASE_X = -65;
    constexpr int PLAYER_BASE_Y = 12;

    // Enemy: top-right (facing player)
    constexpr int ENEMY_BASE_X = 65;
    constexpr int ENEMY_BASE_Y = -36;

    // Create combat arena pedestals
    bn::sprite_ptr player_pedestal = bn::sprite_items::pedestal.create_sprite(PLAYER_BASE_X, PLAYER_BASE_Y + 16);
    player_pedestal.set_z_order(3);

    bn::sprite_ptr enemy_pedestal = bn::sprite_items::pedestal.create_sprite(ENEMY_BASE_X, ENEMY_BASE_Y + 20);
    enemy_pedestal.set_z_order(3);

    // Create Battler Sprites
    bn::sprite_ptr player_sprite = bn::sprite_items::player_back.create_sprite(PLAYER_BASE_X, PLAYER_BASE_Y);
    player_sprite.set_z_order(1);

    bn::sprite_ptr enemy_sprite = bn::sprite_items::enemy_cultist.create_sprite(ENEMY_BASE_X, ENEMY_BASE_Y);
    enemy_sprite.set_z_order(1);

    // Animation & feedback timers
    int frame_count = 0;
    int player_attack_anim = 0;
    int enemy_hurt_anim = 0;
    int player_hurt_anim = 0;
    int enemy_attack_anim = 0;
    int damage_popup_timer = 0;
    int last_damage_dealt = 0;

    // Turn sequence state
    enum class SceneState {
        PLAYER_INPUT,
        ENEMY_ATTACKING,
        VICTORY,
        DEFEAT
    };
    SceneState scene_state = SceneState::PLAYER_INPUT;

    bn::vector<bn::sprite_ptr, 64> text_sprites;

    while(true)
    {
        ++frame_count;

        // -------------------------------------------------------------
        // 1. INPUT HANDLING (Supports ALL keyboard & gamepad keys!)
        // -------------------------------------------------------------
        // Note: mGBA default keyboard mapping has:
        // Key 'X' = Button A, Key 'Z' = Button B, Key 'A' = Button L, Key 'S' = Button R,
        // Key 'Enter' = Button START, Key 'Backspace' = Button SELECT.
        // We support ALL of them so whatever key the player presses, IT WORKS!
        bool play_card_key = bn::keypad::a_pressed() || bn::keypad::b_pressed() || 
                             bn::keypad::l_pressed() || bn::keypad::up_pressed();

        bool end_turn_key = bn::keypad::start_pressed() || bn::keypad::r_pressed() || 
                            bn::keypad::select_pressed();

        if(scene_state == SceneState::PLAYER_INPUT && combat.state == engine::CombatState::PLAYER_TURN)
        {
            if(bn::keypad::left_pressed())
            {
                if(combat.selected_card_index > 0)
                {
                    --combat.selected_card_index;
                }
            }
            if(bn::keypad::right_pressed())
            {
                if(combat.selected_card_index + 1 < static_cast<int>(combat.hand.size()))
                {
                    ++combat.selected_card_index;
                }
            }

            if(play_card_key && !combat.hand.empty())
            {
                engine::CardId card_id = combat.hand[combat.selected_card_index];
                const engine::CardDef& def = engine::CardDef::get(card_id);

                int old_enemy_hp = combat.enemy.hp;
                bool played = combat.play_card(combat.selected_card_index);

                if(played)
                {
                    if(def.type == engine::CardType::ATTACK)
                    {
                        // Trigger player attack lunge & enemy hurt shake!
                        player_attack_anim = 14;
                        enemy_hurt_anim = 18;
                        last_damage_dealt = old_enemy_hp - combat.enemy.hp;
                        damage_popup_timer = 24;
                    }

                    if(combat.state == engine::CombatState::VICTORY)
                    {
                        scene_state = SceneState::VICTORY;
                    }
                }
            }
            else if(end_turn_key)
            {
                // Discard hand and start enemy attack animation
                while(!combat.hand.empty())
                {
                    combat.discard_pile.push_back(combat.hand.back());
                    combat.hand.pop_back();
                }
                combat.player.tick_end_of_turn_statuses();
                combat.set_log("Enemy Turn...");

                scene_state = SceneState::ENEMY_ATTACKING;
                enemy_attack_anim = 30; // 30 frames animation
            }
        }
        else if(scene_state == SceneState::ENEMY_ATTACKING)
        {
            // Enemy attack sequence
            if(enemy_attack_anim == 15)
            {
                // Mid-animation: enemy strikes!
                combat.enemy.execute_turn(combat.player);
                player_hurt_anim = 16;

                if(!combat.player.is_alive())
                {
                    combat.state = engine::CombatState::DEFEAT;
                    scene_state = SceneState::DEFEAT;
                }
            }
            else if(enemy_attack_anim <= 0)
            {
                // Enemy turn finishes, start new player turn
                combat.turn_number++;
                combat.enemy.plan_turn(combat.turn_number);
                combat.start_player_turn();
                combat.set_log("Your Turn!");
                scene_state = SceneState::PLAYER_INPUT;
            }
            --enemy_attack_anim;
        }
        else if(scene_state == SceneState::VICTORY || scene_state == SceneState::DEFEAT)
        {
            if(play_card_key || end_turn_key)
            {
                combat.start_combat(engine::EnemyType::CULTIST);
                scene_state = SceneState::PLAYER_INPUT;
            }
        }

        // -------------------------------------------------------------
        // 2. SPRITE POSITIONS & ANIMATIONS (Pokemon-style game feel)
        // -------------------------------------------------------------
        // Idle breathing (subtle 1px floating)
        int idle_bob = (frame_count / 24) % 2;

        // Player animation: lunge forward when attacking, shake when hurt
        int cur_player_x = PLAYER_BASE_X;
        int cur_player_y = PLAYER_BASE_Y + idle_bob;

        if(player_attack_anim > 0)
        {
            // Lunges diagonally toward enemy
            cur_player_x += (player_attack_anim > 7) ? 14 : 6;
            cur_player_y -= (player_attack_anim > 7) ? 6 : 2;
            --player_attack_anim;
        }
        if(player_hurt_anim > 0)
        {
            cur_player_x += (player_hurt_anim % 4 < 2) ? 3 : -3;
            --player_hurt_anim;
        }
        player_sprite.set_position(cur_player_x, cur_player_y);

        // Enemy animation: shake violently when hurt, lunge when attacking
        int cur_enemy_x = ENEMY_BASE_X;
        int cur_enemy_y = ENEMY_BASE_Y + (1 - idle_bob);

        if(enemy_attack_anim > 10)
        {
            // Lunges toward player
            cur_enemy_x -= (enemy_attack_anim > 20) ? 14 : 6;
            cur_enemy_y += (enemy_attack_anim > 20) ? 6 : 2;
        }
        if(enemy_hurt_anim > 0)
        {
            // Fast lateral shake
            cur_enemy_x += (enemy_hurt_anim % 4 < 2) ? 4 : -4;
            // Damage flicker
            enemy_sprite.set_visible((enemy_hurt_anim % 4) != 1);
            --enemy_hurt_anim;
        }
        else
        {
            enemy_sprite.set_visible(true);
        }
        enemy_sprite.set_position(cur_enemy_x, cur_enemy_y);

        // -------------------------------------------------------------
        // 3. UI RENDERING (Pokemon Combat HUD & Slay the Spire Cards)
        // -------------------------------------------------------------
        text_sprites.clear();

        // [TOP-LEFT] Enemy Pokémon-style Status Box
        {
            bn::string<40> e_info;
            bn::ostringstream e_ss(e_info);
            e_ss << to_bn(combat.enemy.name) << " HP:" << combat.enemy.hp << "/" << combat.enemy.max_hp;
            if(combat.enemy.block > 0) e_ss << " +" << combat.enemy.block << "B";
            text_generator.generate(-50, -70, e_info, text_sprites);

            // Intent / Buff line
            bn::string<40> intent_str;
            bn::ostringstream iss(intent_str);
            iss << "Intent: " << to_bn(combat.enemy.current_intent.description);
            int str = combat.enemy.get_status(engine::StatusType::STRENGTH);
            int vuln = combat.enemy.get_status(engine::StatusType::VULNERABLE);
            if(str > 0) iss << " STR+" << str;
            if(vuln > 0) iss << " VULN:" << vuln;
            text_generator.generate(-50, -58, intent_str, text_sprites);
        }

        // [MIDDLE-RIGHT] Player Pokémon-style Status Box
        {
            bn::string<40> p_info;
            bn::ostringstream p_ss(p_info);
            p_ss << "IRONCLAD  HP:" << combat.player.hp << "/" << combat.player.max_hp;
            if(combat.player.block > 0) p_ss << " +" << combat.player.block << "B";
            text_generator.generate(50, -6, p_info, text_sprites);

            // Energy meter
            bn::string<32> e_meter;
            bn::ostringstream em_ss(e_meter);
            em_ss << "ENERGY: " << static_cast<int>(combat.player.energy) << "/" << static_cast<int>(combat.player.max_energy);
            text_generator.generate(50, 6, e_meter, text_sprites);
        }

        // Floating Damage indicator when enemy takes a hit
        if(damage_popup_timer > 0)
        {
            bn::string<16> dmg_str;
            bn::ostringstream dss(dmg_str);
            dss << "-" << last_damage_dealt << " HP!";
            text_generator.generate(ENEMY_BASE_X, ENEMY_BASE_Y - 26, dmg_str, text_sprites);
            --damage_popup_timer;
        }

        // [BOTTOM] Pokémon-style Attack Selection & Card Inspector Box
        if(scene_state == SceneState::VICTORY)
        {
            text_generator.generate(0, 38, "=== VICTORY! ===", text_sprites);
            text_generator.generate(0, 52, "Burning Blood healed +6 HP!", text_sprites);
            text_generator.generate(0, 68, "Press A/X to battle again", text_sprites);
        }
        else if(scene_state == SceneState::DEFEAT)
        {
            text_generator.generate(0, 38, "=== DEFEATED! ===", text_sprites);
            text_generator.generate(0, 52, "You fell to the Cultist...", text_sprites);
            text_generator.generate(0, 68, "Press A/X to retry", text_sprites);
        }
        else
        {
            // Selected Card Detail
            if(!combat.hand.empty() && combat.selected_card_index < static_cast<int>(combat.hand.size()))
            {
                engine::CardId card_id = combat.hand[combat.selected_card_index];
                const engine::CardDef& def = engine::CardDef::get(card_id);

                bn::string<48> card_header;
                bn::ostringstream ch_ss(card_header);
                ch_ss << "> [" << static_cast<int>(def.cost) << "E] " << to_bn(def.name) << " <";
                text_generator.generate(0, 34, card_header, text_sprites);

                text_generator.generate(0, 46, to_bn(def.description), text_sprites);
            }

            // Cards in Hand row (Miniature cards selector)
            if(!combat.hand.empty())
            {
                bn::string<64> hand_str;
                bn::ostringstream h_ss(hand_str);
                for(size_t i = 0; i < combat.hand.size(); ++i)
                {
                    const engine::CardDef& c = engine::CardDef::get(combat.hand[i]);
                    if(static_cast<int>(i) == combat.selected_card_index)
                    {
                        h_ss << "[" << to_bn(c.name.substr(0, std::min<size_t>(3, c.name.size()))) << "] ";
                    }
                    else
                    {
                        h_ss << to_bn(c.name.substr(0, std::min<size_t>(3, c.name.size()))) << " ";
                    }
                }
                text_generator.generate(0, 58, hand_str, text_sprites);
            }

            // Action hints (clearly showing both GBA buttons and PC keys)
            text_generator.generate(0, 71, "A/X/L:Play  START/Enter:End", text_sprites);
        }

        bn::core::update();
    }
}
