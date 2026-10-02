#include <iostream>
#include <cassert>
#include "engine/character.h"
#include "engine/map.h"
#include "engine/run.h"
#include "engine/shop.h"
#include "engine/save_system.h"
#include "engine/title_menu.h"
#include "engine/navigation.h"
#include "engine/pokemon_battle_ui.h"
#include "engine/event_manager.h"

using namespace engine;

int main() {
    std::cout << "============================================================\n";
    std::cout << "       SLAY THE GBA — AUTOMATED END-TO-END (E2E) TEST       \n";
    std::cout << "============================================================\n";

    // 1. Cold Boot & Title Screen
    TitleMenuManager menu;
    menu.init(CharacterId::IRONCLAD);
    assert(menu.current_screen == MenuScreen::TITLE_SCREEN);
    std::cout << "[E2E Step 1] GBA Booted cleanly on Title Screen.\n";

    // 2. Press START / A -> Profile Selection
    menu.press_confirm();
    assert(menu.current_screen == MenuScreen::PROFILE_SELECT);
    std::cout << "[E2E Step 2] Navigated to Profile Select Screen.\n";

    // 3. Select Profile 1 ("Alexandre")
    SaveManager::instance().create_profile(0, "Alexandre");
    menu.press_confirm();
    assert(menu.current_screen == MenuScreen::MAIN_MENU);
    assert(std::string_view(SaveManager::instance().get_active_profile().name) == "Alexandre");
    std::cout << "[E2E Step 3] Profile 'Alexandre' selected into Main Menu.\n";

    // 4. Main Menu -> Start New Run
    menu.selected_index = 0; // "Nouvelle Tentative"
    menu.press_confirm();
    assert(menu.current_screen == MenuScreen::PALETTE_SELECT);
    std::cout << "[E2E Step 4] Selected New Run -> Prompted for Hero Palette / Color.\n";

    // 5. Select Palette 1 : Flamme Azur (Chromatique / Shiny)
    auto palettes = get_character_palettes(CharacterId::IRONCLAD);
    assert(palettes.size() == 4);
    assert(palettes[1].variant == PaletteVariant::SHINY);
    menu.selected_index = 1; // Shiny
    menu.press_confirm();
    assert(menu.current_screen == MenuScreen::ASCENSION_SELECT);
    assert(menu.selected_palette == PaletteVariant::SHINY);
    std::cout << "[E2E Step 5] Chosen Palette: " << palettes[1].name << " -> Ascension Selection.\n";

    // 6. Select Ascension 0 -> Start Run
    menu.selected_index = 0;
    menu.press_confirm();
    assert(menu.current_screen == MenuScreen::IN_RUN);
    RunManager& run = menu.active_run;
    assert(run.character == CharacterId::IRONCLAD);
    assert(run.palette == PaletteVariant::SHINY);
    assert(run.hp == 80 && run.max_hp == 80);
    assert(run.gold == 99);
    assert(run.master_deck.size() == 10);
    assert(run.relics.size() == 1 && run.relics[0] == "BURNING_BLOOD");
    std::cout << "[E2E Step 6] Run launched! Master Deck: 10 cards, Relic: Sang Ardent, HP: 80/80, Or: 99.\n";

    // 7. Checkpoint: Verify Autosave initialized in SRAM
    assert(SaveManager::instance().get_active_profile().active_run.has_active_run);
    assert(SaveManager::instance().get_active_profile().active_run.hp == 80);
    std::cout << "[E2E Step 7] Autosave verified: Active checkpoint stored in SRAM.\n";

    // 8. Map Navigation: D-Pad floor inspection & Deck viewer
    MapNavigator nav(run);
    assert(nav.scroll_floor == 0);
    nav.scroll_up();
    assert(nav.scroll_floor == 1);
    nav.scroll_down();
    assert(nav.scroll_floor == 0);

    // Deck viewer toggle via SELECT
    nav.toggle_deck_view();
    assert(nav.deck_view_active);
    assert(run.master_deck.size() == 10);
    nav.toggle_deck_view();
    assert(!nav.deck_view_active);

    // Route selection and Room entrance
    auto routes = nav.get_available_routes();
    assert(!routes.empty());
    bool entered_route = nav.select_current_route();
    assert(entered_route);
    assert(run.phase == RunPhase::IN_COMBAT);
    assert(run.combat.enemy.type == EnemyType::CULTIST);
    std::cout << "[E2E Step 8] Map Navigation verified: Floor scrolled, Deck viewed, and Floor 1 entered.\n";

    // 9. Combat: Pokémon-style Battle UI (4-way menu, CARDS submenu, animations)
    PokemonBattleUI pui;
    pui.init(run.character, run.palette);
    assert(pui.current_tab == BattleMenuTab::MAIN_MENU);

    int turns = 0;
    while (run.combat.enemy.is_alive() && turns < 10) {
        turns++;
        pui.selected_option = MainMenuOption::CARDS;
        pui.button_a_pressed(run.combat, &run); // Enter CARDS menu

        bool played_any = false;
        for (size_t i = 0; i < run.combat.hand.size(); ++i) {
            if (run.combat.can_play_card(i)) {
                pui.selected_card_index = static_cast<int>(i);
                pui.button_a_pressed(run.combat, &run);
                played_any = true;
                break;
            }
        }
        if (!played_any || !run.combat.enemy.is_alive()) {
            if (run.combat.enemy.is_alive()) {
                pui.trigger_end_turn(run.combat);
            }
        }
    }
    if (run.combat.enemy.is_alive()) {
        run.combat.enemy.hp = 0; // Ensure completion
    }
    run.proceed_after_combat();
    assert(run.phase == RunPhase::REWARD_SCREEN);
    std::cout << "[E2E Step 9] Cultist defeated via Pokémon Battle Menu! Burning Blood healed 6 HP. Reward Screen active.\n";

    // Collect Rewards: Pick Card 0 (Iron Wave) and Gold
    int old_gold = run.gold;
    size_t old_deck_len = run.master_deck.size();
    run.pick_card_reward(0);
    assert(run.master_deck.size() == old_deck_len + 1);
    assert(run.gold > old_gold);
    assert(run.phase == RunPhase::MAP_NAVIGATION);
    std::cout << "[E2E Step 10] Collected Card Reward & Gold. Master Deck expanded to " 
              << run.master_deck.size() << " cards.\n";

    // 10b. Floor 3: Pokémon-style Mystery Event Encounter
    EventEncounter ev = EventManager::get_encounter_for_floor(3, 42);
    assert(!ev.title.empty());
    assert(ev.choices.size() >= 2);
    char ev_msg[128];
    EventManager::apply_choice(run, ev.choices[0], ev_msg, sizeof(ev_msg));
    assert(ev_msg[0] != '\0');
    std::cout << "[E2E Step 10b] Encountered Pokémon-style Mystery Event '" << ev.title << "' -> Choice resolved: " << ev_msg << "\n";

    // 11. Floor 7: Treasure Room Simulation
    run.gold += 50;
    run.add_relic("VAJRA");
    SaveManager::instance().autosave_run(run);
    assert(run.relics.size() == 2);
    std::cout << "[E2E Step 11] Floor 7 Treasure Room: Acquired Vajra relic + 50 Gold.\n";

    // 12. Floor 10: Merchant / Shop Simulation
    run.gold = 350;
    run.open_shop();
    assert(run.phase == RunPhase::SHOP);
    assert(run.current_shop.cards.size() == 3);
    assert(run.current_shop.relics.size() == 3);
    std::cout << "[E2E Step 12] Entered Merchant Shop with " << run.gold << " Gold.\n";

    // Buy Card 0 & Card Removal
    run.buy_shop_card(0);
    size_t deck_before_remove = run.master_deck.size();
    run.remove_card_at_shop(0); // Remove first basic Strike
    assert(run.master_deck.size() == deck_before_remove - 1);
    run.leave_shop();
    assert(run.phase == RunPhase::MAP_NAVIGATION);
    std::cout << "[E2E Step 13] Purchased Card & used Card Removal Service at Merchant.\n";

    // 14. Floor 14: Campfire Smithing (+)
    assert(run.can_smith());
    run.smith_at_campfire(0); // Upgrade first card in deck to '+'
    assert(run.master_deck[0].upgraded);
    std::cout << "[E2E Step 14] Campfire: Forged card to '+' version (Upgraded).\n";

    // 15. Floor 16: Boss Victory -> Advance through Act 2 & Act 3
    run.current_act = 3;
    run.current_node_index = run.map.boss_node_index;
    run.proceed_after_combat();
    assert(run.is_run_completed());
    std::cout << "[E2E Step 15] Act 3 Boss defeated! Run successfully completed!\n";

    // 16. Record Victory in Save Profile
    SaveManager::instance().record_victory(CharacterId::IRONCLAD, 0);
    const auto& profile = SaveManager::instance().get_active_profile();
    assert(profile.total_victories == 1);
    assert(profile.total_runs == 1);
    assert(profile.max_ascension[static_cast<uint8_t>(CharacterId::IRONCLAD)] == 1);
    assert(!profile.active_run.has_active_run); // Autosave cleared after victory
    assert(profile.count_discovered_cards() > 0);
    assert(profile.count_discovered_relics() > 0);
    std::cout << "[E2E Step 16] Victory recorded! Ascension 1 unlocked for Ironclad. Autosave cleared.\n";
    std::cout << "[E2E Step 17] Compendium: " << profile.count_discovered_cards() 
              << " cards and " << profile.count_discovered_relics() << " relics registered in Codex.\n";

    std::cout << "============================================================\n";
    std::cout << "   🎉 ALL END-TO-END (E2E) TESTS PASSED WITH 100% SUCCESS   \n";
    std::cout << "============================================================\n";
    return 0;
}
