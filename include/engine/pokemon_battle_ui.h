#pragma once

#include <cstdint>
#include <string_view>
#include "engine/combat.h"
#include "engine/run.h"
#include "engine/fixed_vector.h"

namespace engine {

enum class BattleMenuTab : uint8_t {
    MAIN_MENU,
    CARDS_MENU,
    BAG_MENU,
    DECK_VIEW,
    ENEMY_TURN_ANIM
};

enum class MainMenuOption : uint8_t {
    CARDS = 0,
    BAG,
    DECK,
    END_TURN
};

struct BattleFeedbackAnim {
    int player_attack_timer = 0;
    int enemy_hurt_timer = 0;
    int player_hurt_timer = 0;
    int enemy_attack_timer = 0;
    int damage_popup_timer = 0;
    int last_damage_dealt = 0;
    int idle_frame = 0;
};

class PokemonBattleUI {
public:
    BattleMenuTab current_tab = BattleMenuTab::MAIN_MENU;
    MainMenuOption selected_option = MainMenuOption::CARDS;
    int selected_card_index = 0;
    int selected_potion_index = 0;
    int deck_scroll_index = 0;

    BattleFeedbackAnim anim;

    char dialogue_text[80] = {0};

    void init(CharacterId hero, PaletteVariant pal);

    // Menu Navigation
    void cursor_left();
    void cursor_right();
    void cursor_up();
    void cursor_down();

    // Confirm (A button) & Cancel (B button)
    bool button_a_pressed(CombatEngine& combat, RunManager* run_ctx = nullptr);
    void button_b_pressed();
    void toggle_deck_shortcut();

    // End Turn trigger
    void trigger_end_turn(CombatEngine& combat);

    // Dialog & message helpers
    void set_dialogue(const char* msg);
    void set_dialogue(std::string_view msg);

    // Pokémon UI formatters
    static void format_hp_bar(int cur_hp, int max_hp, char* out_buf, size_t bar_length = 10);
    static void get_hero_resource_label(const Player& player, CharacterId hero, char* out_buf, size_t max_len);
};

} // namespace engine
