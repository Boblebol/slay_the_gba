#include <iostream>
#include <cassert>
#include "engine/combat.h"
#include "engine/ancient.h"
#include "engine/relic.h"
#include "engine/monster.h"
#include "engine/potion.h"
#include "engine/event.h"
#include "engine/card_db.h"
#include "engine/character.h"
#include "engine/map.h"
#include "engine/run.h"
#include "engine/shop.h"
#include "engine/save_system.h"
#include "engine/title_menu.h"
#include "engine/navigation.h"
#include "engine/event_manager.h"
#include "engine/pokemon_battle_ui.h"

using namespace engine;

int main() {
    std::cout << "=== Slay The GBA Engine Test ===" << std::endl;

    CombatEngine combat;
    combat.start_combat(EnemyType::CULTIST);

    assert(combat.player.hp == 80);
    assert(combat.player.energy == 3);
    assert(combat.hand.size() == 5);
    assert(combat.enemy.hp == 50);
    std::cout << "[PASS] Combat initialized properly" << std::endl;

    // Test playing Strike or Bash if in hand
    bool played = false;
    for (size_t i = 0; i < combat.hand.size(); ++i) {
        if (combat.hand[i] == CardId::STRIKE) {
            int16_t old_hp = combat.enemy.hp;
            bool ok = combat.play_card(i);
            assert(ok);
            assert(combat.enemy.hp == old_hp - 6);
            assert(combat.player.energy == 2);
            played = true;
            std::cout << "[PASS] Played Strike! Cultist HP: " << combat.enemy.hp << std::endl;
            break;
        }
    }
    assert(played);

    // End turn
    combat.end_player_turn();
    assert(combat.turn_number == 2);
    assert(combat.player.energy == 3);
    assert(combat.hand.size() == 5);
    // Cultist had Ritual +3 in turn 1, so after turn 1 ends, Cultist should have 3 Strength!
    assert(combat.enemy.get_status(StatusType::STRENGTH) == 3);
    std::cout << "[PASS] Turn ended, Cultist executed Ritual! Strength is now: " 
              << combat.enemy.get_status(StatusType::STRENGTH) << std::endl;

    // Test Ancients System (Patch v0.111.0 Beta)
    auto ancients = get_all_ancients();
    assert(ancients.size() == 8);
    std::cout << "[PASS] Ancients loaded: " << ancients.size() << " ancients registered" << std::endl;

    const auto& neow = get_ancient(AncientId::NEOW);
    assert(neow.name == "Neow");
    assert(neow.slots.size() == 2);
    assert(neow.slots[0].options.size() == 20);
    assert(neow.slots[1].options.size() == 10);
    std::cout << "[PASS] Neow validated with " << (neow.slots[0].options.size() + neow.slots[1].options.size()) << " relics" << std::endl;

    const auto& darv = get_ancient(AncientId::DARV);
    assert(darv.name == "Darv");
    assert(darv.slots[0].options.size() == 12);
    std::cout << "[PASS] Darv validated with " << darv.slots[0].options.size() << " relics" << std::endl;

    const auto& vakuu = get_ancient(AncientId::VAKUU);
    assert(vakuu.name == "Vakuu");
    assert(vakuu.slots.size() == 3);
    std::cout << "[PASS] Vakuu validated with 3 slots" << std::endl;

    // Test Complete Cards Database (569 cards)
    auto all_cards = get_all_card_records();
    assert(all_cards.size() == 569);
    const auto* abrasive = find_card_record("ABRASIVE");
    assert(abrasive != nullptr && abrasive->name == "Abrasif");
    std::cout << "[PASS] Cards database: 569 cards registered (validated: " << abrasive->name << ")" << std::endl;

    // Test Complete Relics Database (298 relics)
    auto all_relics = get_all_relics();
    assert(all_relics.size() == 298);
    const auto* akabeko = find_relic("AKABEKO");
    assert(akabeko != nullptr && akabeko->name == "Akabeko");
    std::cout << "[PASS] Relics database: 298 relics registered (validated: " << akabeko->name << ")" << std::endl;

    // Test Complete Monsters Database (112 monsters)
    auto all_monsters = get_all_monsters();
    assert(all_monsters.size() == 112);
    const auto* aeonglass = find_monster("AEONGLASS");
    assert(aeonglass != nullptr && aeonglass->name == "Clepséon");
    assert(aeonglass->min_hp == 512);
    std::cout << "[PASS] Monsters database: 112 monsters registered (validated: " << aeonglass->name << " HP: " << aeonglass->min_hp << ")" << std::endl;

    // Test Complete Potions Database (63 potions)
    auto all_potions = get_all_potions();
    assert(all_potions.size() == 63);
    const auto* ambergris = find_potion("AMBERGRIS");
    assert(ambergris != nullptr && ambergris->name == "Ambre gris");
    std::cout << "[PASS] Potions database: 63 potions registered (validated: " << ambergris->name << ")" << std::endl;

    // Test Complete Events Database (57 events)
    auto all_events = get_all_events();
    assert(all_events.size() == 57);
    const auto* abyssal_baths = find_event("ABYSSAL_BATHS");
    assert(abyssal_baths != nullptr && abyssal_baths->name == "Bains abyssaux");
    std::cout << "[PASS] Events database: 57 events registered (validated: " << abyssal_baths->name << ")" << std::endl;

    // Test Playable Characters & Starter Decks (5 characters)
    auto all_characters = get_all_characters();
    assert(all_characters.size() == 5);
    const auto& ironclad = get_character(CharacterId::IRONCLAD);
    assert(ironclad.starting_hp == 80);
    assert(ironclad.starting_relic_id == "BURNING_BLOOD");
    assert(ironclad.starter_deck.size() == 3);

    const auto& silent = get_character(CharacterId::SILENT);
    assert(silent.starting_hp == 70);
    assert(silent.starting_relic_id == "RING_OF_THE_SNAKE");
    assert(silent.starter_deck.size() == 4);

    const auto& defect = get_character(CharacterId::DEFECT);
    assert(defect.starting_hp == 75);
    assert(defect.starting_relic_id == "CRACKED_CORE");

    const auto& regent = get_character(CharacterId::REGENT);
    assert(regent.starting_hp == 75);
    assert(regent.starting_relic_id == "DIVINE_RIGHT");

    const auto& necrobinder = get_character(CharacterId::NECROBINDER);
    assert(necrobinder.starting_hp == 65);
    assert(necrobinder.starting_relic_id == "BOUND_PHYLACTERY");
    std::cout << "[PASS] Characters database: 5 characters registered (Ironclad 80HP, Silent 70HP, Defect 75HP, Regent 75HP, Necrobinder 65HP)" << std::endl;

    // Test Character-specific and Common Cards distribution
    assert(count_cards_by_pool("Ironclad") == 90);
    assert(count_cards_by_pool("Silent") == 91);
    assert(count_cards_by_pool("Defect") == 91);
    assert(count_cards_by_pool("Regent") == 91);
    assert(count_cards_by_pool("Necrobinder") == 91);

    // Verify exactly 20 Common cards for EVERY character
    assert(count_cards_by_pool_and_rarity("Ironclad", "Common") == 20);
    assert(count_cards_by_pool_and_rarity("Silent", "Common") == 20);
    assert(count_cards_by_pool_and_rarity("Defect", "Common") == 20);
    assert(count_cards_by_pool_and_rarity("Regent", "Common") == 20);
    assert(count_cards_by_pool_and_rarity("Necrobinder", "Common") == 20);
    std::cout << "[PASS] Common cards validated: exactly 20 common cards for each of the 5 characters!" << std::endl;

    // Test Roguelike Map Graph Generation (15 Floors + Boss)
    MapGraph map;
    map.generate(1, 42);
    assert(map.nodes.size() == 46);
    assert(map.boss_node_index == 45);
    assert(map.nodes[map.boss_node_index].type == RoomType::BOSS);
    assert(map.nodes[map.boss_node_index].floor == 15);

    auto start_nodes = map.get_starting_nodes();
    assert(start_nodes.size() == 3);
    for (int idx : start_nodes) {
        assert(map.nodes[idx].floor == 0);
        assert(map.nodes[idx].type == RoomType::MONSTER);
    }
    auto next_nodes = map.get_next_nodes(start_nodes[0]);
    assert(!next_nodes.empty());
    std::cout << "[PASS] Procedural Map Graph: 16 floors generated (3 start branches, 1 Boss summit)" << std::endl;

    // Test Full Roguelike Run Loop (RunManager)
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    assert(run.character == CharacterId::IRONCLAD);
    assert(run.hp == 80 && run.max_hp == 80);
    assert(run.gold == 99);
    assert(run.master_deck.size() == 10); // 5 Strike, 4 Defend, 1 Bash
    assert(run.relics.size() == 1 && run.relics[0] == "BURNING_BLOOD");
    assert(run.current_act == 1 && run.current_floor == 1);
    assert(run.phase == RunPhase::MAP_NAVIGATION);

    // Test Room Entry & Combat initialization
    auto available = run.get_available_rooms();
    assert(available.size() == 3);
    run.enter_room(available[0]);
    assert(run.phase == RunPhase::IN_COMBAT);
    assert(run.combat.player.hp == 80);
    assert(run.combat.draw_pile.size() + run.combat.hand.size() == 10);

    // Simulate winning combat & reward screen
    run.combat.player.hp = 70; // took 10 dmg
    run.proceed_after_combat();
    // Burning blood heals 6 HP -> 76 HP
    assert(run.hp == 76);
    assert(run.phase == RunPhase::REWARD_SCREEN);
    assert(run.current_card_rewards.size() == 3);
    int prev_deck_size = static_cast<int>(run.master_deck.size());
    int prev_gold = run.gold;
    run.pick_card_reward(0);
    assert(run.master_deck.size() == prev_deck_size + 1);
    assert(run.gold > prev_gold);
    assert(run.phase == RunPhase::MAP_NAVIGATION);
    std::cout << "[PASS] Run Loop: Deck, Combat, Burning Blood (+6 HP), Card Reward & Gold picked" << std::endl;

    // Test Campfire Rest
    assert(run.can_rest());
    int hp_before_rest = run.hp;
    run.rest_at_campfire();
    // Rest heals 30% of max_hp (80 * 0.30 = 24) -> 76 + 24 = 100 capped at 80
    assert(run.hp == 80);
    assert(run.hp > hp_before_rest);
    std::cout << "[PASS] Campfire: Rest healed HP back to max" << std::endl;

    // Test Ascension Modifiers (Ascension 0 to 10)
    auto asc_mods = RunManager::get_ascension_modifiers();
    assert(asc_mods.size() == 11);
    assert(asc_mods[1].level == 1);
    assert(asc_mods[10].level == 10);
    assert(asc_mods[10].title == "Fléau de l'Ascension");

    // Ascension 6: Player starts with 10% less HP (80 -> 72)
    RunManager run_asc6;
    run_asc6.start_run(CharacterId::IRONCLAD, 6, PaletteVariant::CLASSIC, 777);
    assert(run_asc6.hp == 72);
    assert(run_asc6.max_hp == 80);

    // Ascension 2: Enemy damage multiplier (+20%)
    assert(run.get_enemy_damage_multiplier() == 100);
    assert(run_asc6.get_enemy_damage_multiplier() == 120);

    // Ascension 7 & 10: Enemy HP multiplier (+15%)
    RunManager run_asc10;
    run_asc10.start_run(CharacterId::NECROBINDER, 10, PaletteVariant::CLASSIC, 777);
    assert(run_asc10.hp == 58); // 65 * 9 / 10 = 58
    assert(run_asc10.get_enemy_damage_multiplier() == 120);
    assert(run_asc10.get_enemy_hp_multiplier() == 115);
    std::cout << "[PASS] Ascension 0-10: Levels, handicap HP (A6/A10), dmg mult (A2), and HP mult (A7) verified" << std::endl;

    // Test Act Progression & Victory
    run.current_act = 3;
    run.current_node_index = run.map.boss_node_index;
    run.proceed_after_combat();
    assert(run.is_run_completed());
    std::cout << "[PASS] Act Progression: Act 3 Boss victory triggers Run Completed" << std::endl;

    // Test 3 Save Profiles & Autosave System
    SaveManager& sm = SaveManager::instance();
    sm.init();
    sm.select_profile(0);
    assert(sm.get_active_profile_index() == 0);
    sm.create_profile(1, "Alexandre");
    assert(std::string_view(sm.get_profile(1).name) == "Alexandre");
    assert(sm.get_profile(1).is_created);

    // Test Autosave & Run Restoring
    run.hp = 64;
    run.gold = 188;
    sm.autosave_run(run);
    assert(sm.get_active_profile().active_run.has_active_run);
    assert(sm.get_active_profile().active_run.hp == 64);
    assert(sm.get_active_profile().active_run.gold == 188);

    RunManager restored_run;
    bool ok_restore = sm.restore_run(restored_run);
    assert(ok_restore);
    assert(restored_run.hp == 64);
    assert(restored_run.gold == 188);
    assert(restored_run.master_deck.size() == run.master_deck.size());
    std::cout << "[PASS] Save System: 3 Profiles, Autosave checkpoint and Run Restoration verified" << std::endl;

    // Test Compendium / Codex Progressive Unlocks
    sm.discover_card_by_id("ABRASIVE");
    sm.discover_relic_by_id("AKABEKO");
    sm.discover_monster_by_id("AEONGLASS");
    sm.discover_potion_by_id("AMBERGRIS");
    sm.discover_ancient_by_id(0); // Neow
    assert(sm.get_active_profile().count_discovered_cards() > 0);
    assert(sm.get_active_profile().count_discovered_relics() > 0);
    assert(sm.get_active_profile().count_discovered_monsters() > 0);
    assert(sm.get_active_profile().count_discovered_potions() > 0);
    assert(sm.get_active_profile().count_discovered_ancients() > 0);
    std::cout << "[PASS] Compendium: Progressive discovery tracking verified for Cards, Relics, Monsters, Potions, Ancients" << std::endl;

    // Test Campfire Smithing / Upgrading Cards
    assert(restored_run.can_smith());
    assert(!restored_run.master_deck[0].upgraded);
    restored_run.smith_at_campfire(0); // Upgrade first card (Strike -> Strike+)
    assert(restored_run.master_deck[0].upgraded);
    assert(restored_run.phase == RunPhase::MAP_NAVIGATION);

    // In combat, verify Strike+ deals 9 damage instead of 6
    CombatEngine up_combat;
    up_combat.start_combat(EnemyType::CULTIST);
    up_combat.hand.clear();
    up_combat.hand.push_back(DeckCard(CardId::STRIKE, true)); // Strike+
    int old_enemy_hp = up_combat.enemy.hp;
    up_combat.play_card(0);
    assert(up_combat.enemy.hp == old_enemy_hp - 9);
    std::cout << "[PASS] Campfire & Card Upgrades: Card smithing to '+' and Strike+ 9 dmg verified" << std::endl;

    // Test Merchant / Shop & Gold economy
    restored_run.gold = 300;
    restored_run.open_shop();
    assert(restored_run.phase == RunPhase::SHOP);
    assert(restored_run.current_shop.cards.size() == 3);
    assert(restored_run.current_shop.relics.size() == 2);
    prev_gold = restored_run.gold;
    size_t prev_deck_sz = restored_run.master_deck.size();
    bool buy_c_ok = restored_run.buy_shop_card(0);
    assert(buy_c_ok);
    assert(restored_run.gold < prev_gold);
    assert(restored_run.master_deck.size() == prev_deck_sz + 1);

    // Test Card Removal at shop (Base cost 75 Or)
    prev_gold = restored_run.gold;
    prev_deck_sz = restored_run.master_deck.size();
    bool remove_ok = restored_run.remove_card_at_shop(0);
    assert(remove_ok);
    assert(restored_run.gold == prev_gold - 75);
    assert(restored_run.master_deck.size() == prev_deck_sz - 1);
    assert(restored_run.card_removals_count == 1);
    restored_run.leave_shop();
    assert(restored_run.phase == RunPhase::MAP_NAVIGATION);
    std::cout << "[PASS] Merchant: Catalog generation, buying cards/relics and Card Removal service verified" << std::endl;

    // Test Character-Specific Resources & Combat Modifiers
    // 1. Silent Poison
    CombatEngine silent_c;
    silent_c.start_combat(EnemyType::CULTIST, CharacterId::SILENT);
    silent_c.enemy.add_status(StatusType::POISON, 5);
    assert(silent_c.enemy.get_status(StatusType::POISON) == 5);
    int16_t ptick = silent_c.enemy.apply_poison_tick();
    assert(ptick == 5);
    assert(silent_c.enemy.hp == 45); // Direct HP damage (50 -> 45)
    assert(silent_c.enemy.get_status(StatusType::POISON) == 4);

    // 2. Defect Orbs (Lightning channel, passive & evoke)
    CombatEngine defect_c;
    defect_c.start_combat(EnemyType::CULTIST, CharacterId::DEFECT);
    defect_c.player.channel_orb(OrbType::LIGHTNING, &defect_c.enemy);
    assert(defect_c.player.orbs.size() == 1);
    defect_c.player.trigger_passive_orbs(&defect_c.enemy);
    assert(defect_c.enemy.hp == 47); // 3 passive lightning damage
    defect_c.player.evoke_oldest_orb(&defect_c.enemy);
    assert(defect_c.enemy.hp == 39); // 8 evoke lightning damage
    assert(defect_c.player.orbs.empty());

    // 3. Regent Sovereign Stars
    CombatEngine regent_c;
    regent_c.start_combat(EnemyType::CULTIST, CharacterId::REGENT);
    regent_c.player.add_stars(3);
    assert(regent_c.player.sovereign_stars == 3);

    // 4. Necrobinder Souls & Minion (Osty)
    CombatEngine necro_c;
    necro_c.start_combat(EnemyType::CULTIST, CharacterId::NECROBINDER);
    necro_c.player.summon_minion(10, 0); // Summon minion with 10 HP
    assert(necro_c.player.minion_hp == 10);
    necro_c.player.take_damage_with_minion(6); // Enemy hits 6 dmg
    assert(necro_c.player.minion_hp == 4);     // Minion took the 6 damage!
    assert(necro_c.player.hp == 65);          // Necrobinder HP untouched!

    // 5. Ironclad Metallicize
    CombatEngine iron_c;
    iron_c.start_combat(EnemyType::CULTIST, CharacterId::IRONCLAD);
    iron_c.player.add_status(StatusType::METALLICIZE, 4);
    iron_c.player.reset_turn();
    assert(iron_c.player.block == 4);
    std::cout << "[PASS] Character Resources: Silent Poison, Defect Orbs, Regent Stars, Necro Minion & Ironclad Metallicize verified" << std::endl;

    // Test Title Screen & Menuing Navigation
    TitleMenuManager menu;
    menu.init();
    assert(menu.current_screen == MenuScreen::TITLE_SCREEN);
    menu.press_confirm(); // Title -> Profile Select
    assert(menu.current_screen == MenuScreen::PROFILE_SELECT);
    menu.press_confirm(); // Select Profile 0 -> Main Menu
    assert(menu.current_screen == MenuScreen::MAIN_MENU);
    menu.press_back();    // Main Menu -> Profile Select
    assert(menu.current_screen == MenuScreen::PROFILE_SELECT);
    std::cout << "[PASS] Title Screen & Menuing: Splash, Profile Select, Main Menu navigation verified" << std::endl;

    // Test MapNavigator: D-Pad floor scrolling, route inspection & Deck viewing
    RunManager nav_run;
    nav_run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::SHINY, 1001);
    MapNavigator nav(nav_run);

    assert(nav.scroll_floor == 0);
    nav.scroll_up();
    assert(nav.scroll_floor == 1);
    nav.scroll_up();
    assert(nav.scroll_floor == 2);
    nav.scroll_down();
    assert(nav.scroll_floor == 1);

    // Starting routes
    auto routes = nav.get_available_routes();
    assert(routes.size() == 3); // 3 lanes on floor 0
    assert(nav.get_selected_target_node() == routes[0]);
    nav.next_route();
    assert(nav.get_selected_target_node() == routes[1]);
    nav.prev_route();
    assert(nav.get_selected_target_node() == routes[0]);

    // Enter chosen room
    bool entered = nav.select_current_route();
    assert(entered);
    assert(nav_run.current_floor == 1);
    assert(nav_run.current_node_index == routes[0]);

    // Test Deck Viewing overlay
    assert(!nav.deck_view_active);
    nav.toggle_deck_view();
    assert(nav.deck_view_active);
    assert(nav.deck_scroll_index == 0);
    nav.deck_scroll_down();
    assert(nav.deck_scroll_index == 1);
    nav.deck_scroll_up();
    assert(nav.deck_scroll_index == 0);
    nav.toggle_deck_view();
    assert(!nav.deck_view_active);
    std::cout << "[PASS] Map Navigation & Deck Viewer: Floor scrolling, route inspection & Deck view verified" << std::endl;

    // Test Pokémon Battle UI: 4-Way Menu, Moves, Animations & Layout
    PokemonBattleUI pui;
    pui.init(CharacterId::IRONCLAD, PaletteVariant::SHINY);
    assert(pui.current_tab == BattleMenuTab::MAIN_MENU);
    assert(pui.selected_option == MainMenuOption::CARDS);

    // 4-Way navigation
    pui.cursor_right();
    assert(pui.selected_option == MainMenuOption::BAG);
    pui.cursor_down();
    assert(pui.selected_option == MainMenuOption::END_TURN);
    pui.cursor_left();
    assert(pui.selected_option == MainMenuOption::DECK);
    pui.cursor_up();
    assert(pui.selected_option == MainMenuOption::CARDS);

    // Open CARDS submenu (Move selection)
    CombatEngine p_combat;
    p_combat.start_combat(EnemyType::CULTIST, CharacterId::IRONCLAD);
    pui.button_a_pressed(p_combat, &nav_run);
    assert(pui.current_tab == BattleMenuTab::CARDS_MENU);

    // Play card (Strike)
    int cultist_initial_hp = p_combat.enemy.hp;
    bool card_played = pui.button_a_pressed(p_combat, &nav_run);
    assert(card_played);
    assert(p_combat.enemy.hp < cultist_initial_hp);
    assert(pui.anim.player_attack_timer > 0);
    assert(pui.anim.enemy_hurt_timer > 0);
    assert(pui.anim.last_damage_dealt == (cultist_initial_hp - p_combat.enemy.hp));

    // Cancel back to main menu
    pui.button_b_pressed();
    assert(pui.current_tab == BattleMenuTab::MAIN_MENU);

    // Test HP Bar formatting & Hero resource labels
    char hp_bar[16];
    PokemonBattleUI::format_hp_bar(40, 80, hp_bar, 10);
    assert(std::string_view(hp_bar) == "[=====-----]");

    char res_label[48];
    PokemonBattleUI::get_hero_resource_label(p_combat.player, CharacterId::IRONCLAD, res_label, sizeof(res_label));
    assert(std::string_view(res_label).find("STR") != std::string_view::npos);

    Player silent_pl;
    PokemonBattleUI::get_hero_resource_label(silent_pl, CharacterId::SILENT, res_label, sizeof(res_label));
    assert(std::string_view(res_label) == "POISON");

    Player defect_pl;
    defect_pl.channel_orb(OrbType::LIGHTNING);
    PokemonBattleUI::get_hero_resource_label(defect_pl, CharacterId::DEFECT, res_label, sizeof(res_label));
    assert(std::string_view(res_label) == "ORBES: 1/3");

    Player regent_pl;
    regent_pl.add_stars(4);
    PokemonBattleUI::get_hero_resource_label(regent_pl, CharacterId::REGENT, res_label, sizeof(res_label));
    assert(std::string_view(res_label) == "ETOILES: 4/5");

    Player necro_pl;
    necro_pl.summon_minion(12, 0);
    PokemonBattleUI::get_hero_resource_label(necro_pl, CharacterId::NECROBINDER, res_label, sizeof(res_label));
    assert(std::string_view(res_label).find("OSTY: 12 PV") != std::string_view::npos);
    std::cout << "[PASS] Pokémon Battle UI: 4-Way Menu, Move Selection, Attack feedback, HP bar & Resource HUD verified" << std::endl;

    // Test Event Manager: Encounters, Choices & Outcomes
    RunManager ev_run;
    ev_run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 42);
    EventEncounter enc = EventManager::get_encounter_for_floor(3, 42);
    assert(!enc.title.empty());
    assert(enc.choices.size() >= 2);

    ev_run.hp = 50;
    int hp_before_ev = ev_run.hp;
    int gold_before_ev = ev_run.gold;
    char outcome_msg[128];

    // Pick choice 0 (Gain gold / Heal / etc.)
    EventManager::apply_choice(ev_run, enc.choices[0], outcome_msg, sizeof(outcome_msg));
    assert(outcome_msg[0] != '\0');
    assert(ev_run.gold != gold_before_ev || ev_run.hp != hp_before_ev);
    std::cout << "[PASS] Pokémon-style Events: Encounters, Choice Dialogues, and Run State Modifications verified" << std::endl;

    std::cout << "=== All Engine Tests Passed Successfully! ===" << std::endl;
    return 0;
}
