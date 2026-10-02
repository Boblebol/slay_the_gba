#include "engine/pokemon_battle_ui.h"
#include <algorithm>

namespace engine {

namespace {

void safe_copy_cstr(char* dest, size_t dest_len, const char* src) {
    if (!dest || dest_len == 0 || !src) return;
    size_t i = 0;
    while (src[i] != '\0' && i + 1 < dest_len) {
        dest[i] = src[i];
        ++i;
    }
    dest[i] = '\0';
}

void safe_copy_sv(char* dest, size_t dest_len, std::string_view src) {
    if (!dest || dest_len == 0) return;
    size_t count = std::min(dest_len - 1, src.size());
    for (size_t i = 0; i < count; ++i) {
        dest[i] = src[i];
    }
    dest[count] = '\0';
}

template<size_t N>
void append_lit(char* buf, size_t& pos, size_t max_len, const char (&lit)[N]) {
    for (size_t i = 0; i < N - 1 && pos + 1 < max_len; ++i) {
        buf[pos++] = lit[i];
    }
    buf[pos] = '\0';
}

void append_view(char* buf, size_t& pos, size_t max_len, std::string_view str) {
    for (size_t i = 0; i < str.size() && pos + 1 < max_len; ++i) {
        buf[pos++] = str[i];
    }
    buf[pos] = '\0';
}

void append_int(char* buf, size_t& pos, size_t max_len, int val) {
    if (val < 0) {
        if (pos + 1 < max_len) buf[pos++] = '-';
        val = -val;
    }
    char tmp[16];
    int t_pos = 0;
    if (val == 0) tmp[t_pos++] = '0';
    while (val > 0 && t_pos < 15) {
        tmp[t_pos++] = '0' + (val % 10);
        val /= 10;
    }
    while (t_pos > 0 && pos + 1 < max_len) {
        buf[pos++] = tmp[--t_pos];
    }
    buf[pos] = '\0';
}

} // namespace

void PokemonBattleUI::init(CharacterId hero, PaletteVariant pal) {
    (void)pal;
    current_tab = BattleMenuTab::MAIN_MENU;
    selected_option = MainMenuOption::CARDS;
    selected_card_index = 0;
    selected_potion_index = 0;
    deck_scroll_index = 0;
    anim = BattleFeedbackAnim{};

    const auto& cdef = get_character(hero);
    char buf[80];
    buf[0] = '\0';
    size_t pos = 0;
    append_lit(buf, pos, sizeof(buf), "Que doit faire ");
    append_view(buf, pos, sizeof(buf), cdef.name);
    append_lit(buf, pos, sizeof(buf), " ?");
    set_dialogue(buf);
}

void PokemonBattleUI::set_dialogue(const char* msg) {
    safe_copy_cstr(dialogue_text, sizeof(dialogue_text), msg);
}

void PokemonBattleUI::set_dialogue(std::string_view msg) {
    safe_copy_sv(dialogue_text, sizeof(dialogue_text), msg);
}

void PokemonBattleUI::cursor_left() {
    if (current_tab == BattleMenuTab::MAIN_MENU) {
        if (selected_option == MainMenuOption::BAG) selected_option = MainMenuOption::CARDS;
        else if (selected_option == MainMenuOption::END_TURN) selected_option = MainMenuOption::DECK;
    } else if (current_tab == BattleMenuTab::CARDS_MENU) {
        if (selected_card_index > 0) --selected_card_index;
    }
}

void PokemonBattleUI::cursor_right() {
    if (current_tab == BattleMenuTab::MAIN_MENU) {
        if (selected_option == MainMenuOption::CARDS) selected_option = MainMenuOption::BAG;
        else if (selected_option == MainMenuOption::DECK) selected_option = MainMenuOption::END_TURN;
    } else if (current_tab == BattleMenuTab::CARDS_MENU) {
        ++selected_card_index;
    }
}

void PokemonBattleUI::cursor_up() {
    if (current_tab == BattleMenuTab::MAIN_MENU) {
        if (selected_option == MainMenuOption::DECK) selected_option = MainMenuOption::CARDS;
        else if (selected_option == MainMenuOption::END_TURN) selected_option = MainMenuOption::BAG;
    } else if (current_tab == BattleMenuTab::CARDS_MENU) {
        if (selected_card_index >= 2) selected_card_index -= 2;
    } else if (current_tab == BattleMenuTab::DECK_VIEW) {
        if (deck_scroll_index > 0) --deck_scroll_index;
    }
}

void PokemonBattleUI::cursor_down() {
    if (current_tab == BattleMenuTab::MAIN_MENU) {
        if (selected_option == MainMenuOption::CARDS) selected_option = MainMenuOption::DECK;
        else if (selected_option == MainMenuOption::BAG) selected_option = MainMenuOption::END_TURN;
    } else if (current_tab == BattleMenuTab::CARDS_MENU) {
        selected_card_index += 2;
    } else if (current_tab == BattleMenuTab::DECK_VIEW) {
        ++deck_scroll_index;
    }
}

bool PokemonBattleUI::button_a_pressed(CombatEngine& combat, RunManager* run_ctx) {
    if (combat.state != CombatState::PLAYER_TURN) return false;

    if (current_tab == BattleMenuTab::MAIN_MENU) {
        switch (selected_option) {
            case MainMenuOption::CARDS:
                current_tab = BattleMenuTab::CARDS_MENU;
                selected_card_index = 0;
                set_dialogue("Choisissez une carte a jouer.");
                return true;
            case MainMenuOption::BAG:
                current_tab = BattleMenuTab::BAG_MENU;
                selected_potion_index = 0;
                set_dialogue("Choisissez une potion dans votre sac.");
                return true;
            case MainMenuOption::DECK:
                current_tab = BattleMenuTab::DECK_VIEW;
                deck_scroll_index = 0;
                set_dialogue("Consultation du Deck.");
                return true;
            case MainMenuOption::END_TURN:
                trigger_end_turn(combat);
                return true;
            default:
                break;
        }
    } else if (current_tab == BattleMenuTab::CARDS_MENU) {
        if (combat.hand.empty()) return false;
        if (selected_card_index >= static_cast<int>(combat.hand.size())) {
            selected_card_index = static_cast<int>(combat.hand.size()) - 1;
        }

        DeckCard card = combat.hand[selected_card_index];
        int prev_enemy_hp = combat.enemy.hp;

        bool played = combat.play_card(selected_card_index);
        if (played) {
            int dmg = prev_enemy_hp - combat.enemy.hp;
            anim.player_attack_timer = 14;
            if (dmg > 0) {
                anim.enemy_hurt_timer = 18;
                anim.last_damage_dealt = dmg;
                anim.damage_popup_timer = 24;
            }

            char buf[80];
            buf[0] = '\0';
            size_t pos = 0;
            std::string_view cname = CardDef::get_display_name(card.id, card.upgraded);
            if (dmg > 0) {
                append_view(buf, pos, sizeof(buf), cname);
                append_lit(buf, pos, sizeof(buf), " inflige ");
                append_int(buf, pos, sizeof(buf), dmg);
                append_lit(buf, pos, sizeof(buf), " degats !");
            } else {
                append_lit(buf, pos, sizeof(buf), "Vous jouez ");
                append_view(buf, pos, sizeof(buf), cname);
                append_lit(buf, pos, sizeof(buf), " !");
            }
            set_dialogue(buf);

            if (selected_card_index >= static_cast<int>(combat.hand.size())) {
                selected_card_index = std::max(0, static_cast<int>(combat.hand.size()) - 1);
            }
            return true;
        } else {
            set_dialogue("Energie insuffisante pour cette carte !");
            return false;
        }
    } else if (current_tab == BattleMenuTab::BAG_MENU) {
        if (run_ctx && !run_ctx->potions.empty() && selected_potion_index < static_cast<int>(run_ctx->potions.size())) {
            std::string_view pid = run_ctx->potions[selected_potion_index];
            if (!combat.use_potion(pid, combat.selected_target_index)) {
                set_dialogue("Cette potion ne peut pas etre utilisee.");
                return false;
            }
            if (pid == "FIRE_POTION") {
                anim.enemy_hurt_timer = 18;
                anim.last_damage_dealt = 20;
                set_dialogue("Potion incendiaire ! 20 degats infliges !");
            } else if (pid == "BLOCK_POTION") {
                set_dialogue("Potion d'armure ! +12 Armure recue !");
            } else if (pid == "ENERGY_POTION") {
                set_dialogue("Potion d'energie ! +2 Energie recuperee !");
            } else {
                set_dialogue("Potion sanguine ! +20% PV restaures !");
            }
            run_ctx->potions.erase(selected_potion_index);
            current_tab = BattleMenuTab::MAIN_MENU;
            return true;
        } else {
            set_dialogue("Aucune potion utilisable.");
        }
    }
    return false;
}

void PokemonBattleUI::button_b_pressed() {
    if (current_tab != BattleMenuTab::MAIN_MENU) {
        current_tab = BattleMenuTab::MAIN_MENU;
        set_dialogue("Que voulez-vous faire ?");
    }
}

void PokemonBattleUI::toggle_deck_shortcut() {
    if (current_tab == BattleMenuTab::DECK_VIEW) {
        current_tab = BattleMenuTab::MAIN_MENU;
    } else {
        current_tab = BattleMenuTab::DECK_VIEW;
        deck_scroll_index = 0;
    }
}

void PokemonBattleUI::trigger_end_turn(CombatEngine& combat) {
    current_tab = BattleMenuTab::ENEMY_TURN_ANIM;
    set_dialogue("Fin de votre tour. L'ennemi s'apprete a attaquer...");

    // Discard the hand, resolve end of turn effects and play the enemy turn
    combat.end_player_turn();
    anim.enemy_attack_timer = 20;
    anim.player_hurt_timer = 16;

    if (combat.state == CombatState::PLAYER_TURN) {
        current_tab = BattleMenuTab::MAIN_MENU;
        selected_option = MainMenuOption::CARDS;
        set_dialogue("Votre tour commence ! Choisissez une action.");
    }
}

void PokemonBattleUI::format_hp_bar(int cur_hp, int max_hp, char* out_buf, size_t bar_length) {
    if (!out_buf || bar_length == 0) return;
    int filled = 0;
    if (max_hp > 0) {
        filled = (std::clamp(cur_hp, 0, max_hp) * static_cast<int>(bar_length)) / max_hp;
    }
    out_buf[0] = '[';
    size_t i = 0;
    for (; i < static_cast<size_t>(filled) && i < bar_length; ++i) {
        out_buf[1 + i] = '=';
    }
    for (; i < bar_length; ++i) {
        out_buf[1 + i] = '-';
    }
    out_buf[1 + bar_length] = ']';
    out_buf[2 + bar_length] = '\0';
}

void PokemonBattleUI::get_hero_resource_label(const Player& player, CharacterId hero, char* out_buf, size_t max_len) {
    if (!out_buf || max_len == 0) return;
    out_buf[0] = '\0';
    size_t pos = 0;
    switch (hero) {
        case CharacterId::SILENT:
            append_lit(out_buf, pos, max_len, "POISON");
            break;
        case CharacterId::DEFECT:
            append_lit(out_buf, pos, max_len, "ORBES: ");
            append_int(out_buf, pos, max_len, static_cast<int>(player.orbs.size()));
            append_lit(out_buf, pos, max_len, "/");
            append_int(out_buf, pos, max_len, static_cast<int>(player.orb_slots));
            break;
        case CharacterId::REGENT:
            append_lit(out_buf, pos, max_len, "ETOILES: ");
            append_int(out_buf, pos, max_len, static_cast<int>(player.sovereign_stars));
            break;
        case CharacterId::NECROBINDER:
            append_lit(out_buf, pos, max_len, "AMES: ");
            append_int(out_buf, pos, max_len, static_cast<int>(player.souls));
            if (player.minion_hp > 0) {
                append_lit(out_buf, pos, max_len, " | OSTY: ");
                append_int(out_buf, pos, max_len, player.minion_hp);
                append_lit(out_buf, pos, max_len, " PV");
            }
            break;
        default: { // IRONCLAD
            int str = player.get_status(StatusType::STRENGTH);
            int met = player.get_status(StatusType::METALLICIZE);
            append_lit(out_buf, pos, max_len, "STR: +");
            append_int(out_buf, pos, max_len, str);
            append_lit(out_buf, pos, max_len, " | METAL: ");
            append_int(out_buf, pos, max_len, met);
            break;
        }
    }
}

} // namespace engine
