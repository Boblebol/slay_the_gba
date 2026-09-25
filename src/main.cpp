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

#include "engine/combat.h"

namespace {

bn::string_view to_bn(std::string_view sv) {
    return bn::string_view(sv.data(), sv.size());
}

} // namespace

int main()
{
    bn::core::init();

    // Dark fantasy atmospheric background (slate navy / charcoal)
    bn::bg_palettes::set_transparent_color(bn::color(3, 4, 6));

    bn::sprite_text_generator text_generator(common::variable_8x8_sprite_font);
    text_generator.set_center_alignment();

    engine::CombatEngine combat;
    combat.start_combat(engine::EnemyType::CULTIST);

    bn::vector<bn::sprite_ptr, 64> text_sprites;

    while(true)
    {
        // Handle input
        if(combat.state == engine::CombatState::PLAYER_TURN)
        {
            if(bn::keypad::left_pressed())
            {
                if(combat.selected_card_index > 0)
                {
                    --combat.selected_card_index;
                }
            }
            else if(bn::keypad::right_pressed())
            {
                if(combat.selected_card_index + 1 < static_cast<int>(combat.hand.size()))
                {
                    ++combat.selected_card_index;
                }
            }
            else if(bn::keypad::a_pressed())
            {
                combat.play_card(combat.selected_card_index);
            }
            else if(bn::keypad::start_pressed() || bn::keypad::r_pressed())
            {
                combat.end_player_turn();
            }
        }
        else if(combat.state == engine::CombatState::VICTORY || combat.state == engine::CombatState::DEFEAT)
        {
            if(bn::keypad::a_pressed() || bn::keypad::start_pressed() || bn::keypad::select_pressed())
            {
                combat.start_combat(engine::EnemyType::CULTIST);
            }
        }

        if(bn::keypad::select_pressed())
        {
            combat.start_combat(engine::EnemyType::CULTIST);
        }

        // Render UI
        text_sprites.clear();

        // 1. Top HUD: Player and Enemy HP & Block
        {
            bn::string<40> p_str;
            bn::ostringstream ss(p_str);
            ss << "IRONCLAD: " << combat.player.hp << "/" << combat.player.max_hp;
            if(combat.player.block > 0) {
                ss << " +" << combat.player.block << "B";
            }
            text_generator.generate(-58, -70, p_str, text_sprites);

            bn::string<40> e_str;
            bn::ostringstream ess(e_str);
            ess << to_bn(combat.enemy.name) << ": " << combat.enemy.hp << "/" << combat.enemy.max_hp;
            if(combat.enemy.block > 0) {
                ess << " +" << combat.enemy.block << "B";
            }
            text_generator.generate(58, -70, e_str, text_sprites);
        }

        // 2. Secondary Top HUD: Energy & Intent
        {
            bn::string<32> e_hud;
            bn::ostringstream ss(e_hud);
            ss << "ENERGY: " << static_cast<int>(combat.player.energy) << "/" << static_cast<int>(combat.player.max_energy);
            text_generator.generate(-58, -58, e_hud, text_sprites);

            bn::string<40> intent_str;
            bn::ostringstream iss(intent_str);
            iss << "Intent: " << to_bn(combat.enemy.current_intent.description);
            text_generator.generate(58, -58, intent_str, text_sprites);
        }

        // 3. Statuses line
        {
            bn::string<64> stat_str;
            bn::ostringstream sss(stat_str);
            int str = combat.enemy.get_status(engine::StatusType::STRENGTH);
            int vuln = combat.enemy.get_status(engine::StatusType::VULNERABLE);
            if(str > 0 || vuln > 0) {
                sss << "Enemy: ";
                if(str > 0) sss << "STR+" << str << " ";
                if(vuln > 0) sss << "VULN:" << vuln;
            }
            if(!stat_str.empty()) {
                text_generator.generate(58, -46, stat_str, text_sprites);
            }
        }

        // 4. Combat Log / Center Announcement
        if(combat.state == engine::CombatState::VICTORY)
        {
            text_generator.generate(0, -15, "=== VICTORY! ===", text_sprites);
            text_generator.generate(0, -2, "Press A / START to restart", text_sprites);
        }
        else if(combat.state == engine::CombatState::DEFEAT)
        {
            text_generator.generate(0, -15, "=== DEFEATED ===", text_sprites);
            text_generator.generate(0, -2, "Press A / START to retry", text_sprites);
        }
        else
        {
            if(combat.combat_log[0] != '\0')
            {
                text_generator.generate(0, -15, combat.combat_log, text_sprites);
            }
        }

        // 5. Card Inspection Detail
        if(combat.state == engine::CombatState::PLAYER_TURN && !combat.hand.empty())
        {
            engine::CardId card_id = combat.hand[combat.selected_card_index];
            const engine::CardDef& def = engine::CardDef::get(card_id);

            bn::string<48> card_title;
            bn::ostringstream css(card_title);
            css << "[" << static_cast<int>(def.cost) << "E] " << to_bn(def.name);
            text_generator.generate(0, 15, card_title, text_sprites);

            text_generator.generate(0, 28, to_bn(def.description), text_sprites);
        }

        // 6. Hand row display
        if(combat.state == engine::CombatState::PLAYER_TURN && !combat.hand.empty())
        {
            bn::string<64> hand_str;
            bn::ostringstream hss(hand_str);

            for(size_t i = 0; i < combat.hand.size(); ++i)
            {
                const engine::CardDef& c = engine::CardDef::get(combat.hand[i]);
                if(static_cast<int>(i) == combat.selected_card_index)
                {
                    hss << "<" << to_bn(c.name) << "> ";
                }
                else
                {
                    hss << to_bn(c.name.substr(0, std::min<size_t>(3, c.name.size()))) << " ";
                }
            }
            text_generator.generate(0, 48, hand_str, text_sprites);
        }

        // 7. Footer / Controls reminder
        text_generator.generate(0, 70, "DPAD:Choose  A:Play  START:End", text_sprites);

        bn::core::update();
    }
}
