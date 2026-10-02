#include <iostream>
#include <string>
#include <algorithm>
#include "engine/run.h"
#include "engine/navigation.h"
#include "engine/pokemon_battle_ui.h"
#include "engine/event_manager.h"
#include "engine/character.h"

using namespace engine;

// Energy cost as text: "X" for the cards that spend all the energy.
static std::string cost_label(CardId id, bool upgraded) {
    uint8_t cost = CardDef::get_cost(id, upgraded);
    return cost == COST_X ? "X" : std::to_string(static_cast<int>(cost));
}

const char* get_hero_realm(CharacterId c) {
    switch (c) {
        case CharacterId::SILENT: return "MARAIS DU VENIN";
        case CharacterId::DEFECT: return "COMPLEXE ASTRAL";
        case CharacterId::REGENT: return "PALAIS SOUVERAIN";
        case CharacterId::NECROBINDER: return "CRYPTE D'OUTRE-TOMBE";
        case CharacterId::IRONCLAD:
        default: return "FORGES DU CARNAGE";
    }
}

const char* get_hero_duel_theme(CharacterId c) {
    switch (c) {
        case CharacterId::SILENT: return "DUEL DU VENIN & DE L'OMBRE";
        case CharacterId::DEFECT: return "DUEL DES ORBES CYBERNETIQUES";
        case CharacterId::REGENT: return "DUEL DE LA MAJESTE STELLAIRE";
        case CharacterId::NECROBINDER: return "DUEL DES MORTS ET D'OSTY";
        case CharacterId::IRONCLAD:
        default: return "DUEL DU SANG ET DU FER";
    }
}

void render_deck_view(const RunManager& run) {
    const auto& cdef = get_character(run.character);
    auto palettes = get_character_palettes(run.character);

    std::cout << "\n======================================================\n";
    std::cout << "  📖 REGISTRE DU DECK — " << cdef.name << " (" << palettes[static_cast<size_t>(run.palette)].name << ")\n";
    std::cout << "  PV: " << run.hp << "/" << run.max_hp << "  |  OR: " << run.gold << " 💰  |  ETAGE: " << (int)run.current_floor << "/16\n";
    std::cout << "------------------------------------------------------\n";
    std::cout << "  RELIQUES (" << run.relics.size() << ") : ";
    for (size_t i = 0; i < run.relics.size(); ++i) {
        std::cout << "[" << run.relics[i] << "] ";
    }
    std::cout << "\n------------------------------------------------------\n";
    std::cout << "  CARTES DU DECK (" << run.master_deck.size() << " cartes) :\n";
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        CardId cid = run.master_deck[i].id;
        bool up = run.master_deck[i].upgraded;
        std::cout << "   " << (i + 1 < 10 ? " " : "") << (i + 1) << ". " 
                  << CardDef::get_display_name(cid, up) << " (" 
                  << cost_label(cid, up) << "E) : " 
                  << CardDef::get_desc(cid, up) << "\n";
    }
    std::cout << "======================================================\n";
    std::cout << "Appuyez sur Entrée pour revenir...";
    std::string dummy;
    std::getline(std::cin, dummy);
}

void render_yugioh_combat(const RunManager& run, const CombatEngine& combat) {
    const auto& cdef = get_character(run.character);
    auto palettes = get_character_palettes(run.character);

    std::cout << "\n==================================================================================\n";
    std::cout << "  ⚔️  ARÈNE DE DUEL YU-GI-OH — " << get_hero_duel_theme(run.character) << " (Étage " << (int)run.current_floor << "/16)\n";
    std::cout << "----------------------------------------------------------------------------------\n";

    // 1 to 3 Enemy Monster Zones
    for (size_t i = 0; i < combat.enemies.size(); ++i) {
        const auto& e = combat.enemies[i];
        if (e.is_alive()) {
            char hp_bar[16];
            PokemonBattleUI::format_hp_bar(e.hp, e.max_hp, hp_bar, 8);
            std::cout << "  [ZONE " << (i + 1) << "] " << e.name << "  PV: " << hp_bar << " " << e.hp << "/" << e.max_hp;
            if (e.block > 0) std::cout << "  🛡️ +" << e.block << "B";
            std::cout << " | Intent: ⚠️ " << e.current_intent.description;
            int vuln = e.get_status(StatusType::VULNERABLE);
            int poison = e.get_status(StatusType::POISON);
            if (vuln > 0) std::cout << " [VULN:" << vuln << "]";
            if (poison > 0) std::cout << " [POISON:" << poison << "]";
            std::cout << "\n";
        } else {
            std::cout << "  [ZONE " << (i + 1) << "] " << e.name << " [K.O. - VAINCU]\n";
        }
    }

    std::cout << "----------------------------------------------------------------------------------\n";
    // Hero Duelist Zone
    char p_hp_bar[16];
    PokemonBattleUI::format_hp_bar(combat.player.hp, combat.player.max_hp, p_hp_bar, 10);
    char res_buf[48];
    PokemonBattleUI::get_hero_resource_label(combat.player, run.character, res_buf, sizeof(res_buf));

    std::cout << "  👤 DUÉLISTE : " << cdef.name << " [" << palettes[static_cast<size_t>(run.palette)].name << "]\n";
    std::cout << "  PV: " << p_hp_bar << " " << combat.player.hp << "/" << combat.player.max_hp;
    if (combat.player.block > 0) std::cout << "  🛡️ +" << combat.player.block << "B";
    std::cout << "  |  ÉNERGIE: [●][●][●] (" << (int)combat.player.energy << "/" << (int)combat.player.max_energy << ")\n";
    std::cout << "  RESSOURCE: " << res_buf << "\n";

    std::cout << "----------------------------------------------------------------------------------\n";
    std::cout << "  🎴 MAIN DE CARTES YU-GI-OH (" << combat.hand.size() << " cartes) :\n";
    for (size_t i = 0; i < combat.hand.size(); ++i) {
        CardId cid = combat.hand[i].id;
        bool up = combat.hand[i].upgraded;
        std::cout << "    [" << (i + 1) << "] " << CardDef::get_display_name(cid, up)
                  << " (" << cost_label(cid, up) << "E) : " 
                  << CardDef::get_desc(cid, up) << "\n";
    }
    std::cout << "==================================================================================\n";
    std::cout << "  Commandes : [1-" << combat.hand.size() << "] Jouer Carte, [e] Fin Tour, [m] Mon Deck, [q] Quitter\n";
    std::cout << "> ";
}

void render_map_screen(const RunManager& run, const MapNavigator& nav) {
    std::cout << "\n======================================================\n";
    std::cout << "     🗺️  CARTE : " << get_hero_realm(run.character) << " (ACTE " << (int)run.current_act << ")\n";
    std::cout << "  PV: " << run.hp << "/" << run.max_hp << "  |  OR: " << run.gold 
              << " 💰  |  Étage Actuel: " << nav.get_current_floor() << "/16\n";
    std::cout << "======================================================\n";

    // Show 4 floors centered on scroll_floor
    int min_f = std::max(0, nav.scroll_floor - 1);
    int max_f = std::min(15, min_f + 3);

    for (int f = max_f; f >= min_f; --f) {
        std::cout << "  Étage " << (f + 1 < 10 ? " " : "") << (f + 1) << " : ";
        if (f == 15) {
            std::cout << "           👑 [BOSS DE L'ACTE]            ";
        } else {
            for (int lane = 0; lane < 3; ++lane) {
                // Find node
                bool found = false;
                for (size_t i = 0; i < run.map.nodes.size(); ++i) {
                    if (run.map.nodes[i].floor == f && run.map.nodes[i].lane == lane) {
                        bool is_cur = (static_cast<int>(i) == run.current_node_index);
                        std::cout << (is_cur ? "👉" : "  ") << nav.get_room_icon(run.map.nodes[i].type) << "   ";
                        found = true;
                        break;
                    }
                }
                if (!found) std::cout << "        ";
            }
        }
        std::cout << "\n";
    }

    std::cout << "------------------------------------------------------\n";
    std::cout << "  ROUTES DISPONIBLES DEPUIS VOTRE POSITION :\n";
    auto routes = nav.get_available_routes();
    for (size_t i = 0; i < routes.size(); ++i) {
        int n_idx = routes[i];
        const auto& node = run.map.nodes[n_idx];
        std::string lane_name = (node.lane == 0 ? "Gauche" : node.lane == 1 ? "Centre" : "Droite");
        std::cout << "    [" << (i + 1) << "] Voie " << lane_name << " (Étage " << (node.floor + 1) 
                  << ") : " << nav.get_room_label(node.type) << " " << nav.get_room_icon(node.type) << "\n";
    }
    std::cout << "------------------------------------------------------\n";
    std::cout << "  Commandes : [1-" << routes.size() << "] Entrer dans la salle, [u/d] Scroll Carte, [m] Voir Deck, [q] Quitter\n";
    std::cout << "> ";
}

int main() {
    std::cout << "======================================================\n";
    std::cout << "   SLAY THE GBA — Simulateur Complet Pokémon/Spire   \n";
    std::cout << "======================================================\n";

    std::cout << "Choisissez la Cartouche GBA (1 Héros par ROM) :\n";
    std::cout << "  [1] Soldat de Fer   (slay_ironclad.gba)\n";
    std::cout << "  [2] La Silencieuse  (slay_silent.gba)\n";
    std::cout << "  [3] Le Défectueux   (slay_defect.gba)\n";
    std::cout << "  [4] Le Régent       (slay_regent.gba)\n";
    std::cout << "  [5] La Nécrolieuse  (slay_necrobinder.gba)\n";
    std::cout << "Cartouche [1-5]: ";

    std::string choice;
    CharacterId hero = CharacterId::IRONCLAD;
    if (std::cin >> choice) {
        if (choice == "2") hero = CharacterId::SILENT;
        else if (choice == "3") hero = CharacterId::DEFECT;
        else if (choice == "4") hero = CharacterId::REGENT;
        else if (choice == "5") hero = CharacterId::NECROBINDER;
    }

    auto palettes = get_character_palettes(hero);
    std::cout << "\nChoisissez la Couleur du Héros (Style Pokémon) :\n";
    for (size_t i = 0; i < palettes.size(); ++i) {
        std::cout << "  [" << (i + 1) << "] " << palettes[i].name << " — " << palettes[i].description << "\n";
    }
    std::cout << "Couleur [1-4]: ";

    PaletteVariant pal = PaletteVariant::CLASSIC;
    if (std::cin >> choice) {
        if (choice == "2") pal = PaletteVariant::SHINY;
        else if (choice == "3") pal = PaletteVariant::SHADOW;
        else if (choice == "4") pal = PaletteVariant::GOLDEN;
    }

    RunManager run;
    run.start_run(hero, 0, pal, 1337);

    MapNavigator nav(run);
    PokemonBattleUI pui;
    pui.init(hero, pal);

    bool running = true;
    while (running && !run.is_run_completed()) {
        if (run.phase == RunPhase::MAP_NAVIGATION) {
            render_map_screen(run, nav);
            std::string input;
            if (!(std::cin >> input)) break;

            if (input == "q" || input == "quit") {
                std::cout << "Partie interrompue.\n";
                break;
            } else if (input == "u") {
                nav.scroll_up();
            } else if (input == "d") {
                nav.scroll_down();
            } else if (input == "m" || input == "deck") {
                render_deck_view(run);
            } else {
                try {
                    int route_idx = std::stoi(input) - 1;
                    auto routes = nav.get_available_routes();
                    if (route_idx >= 0 && route_idx < static_cast<int>(routes.size())) {
                        std::cout << "\n🏃 [ANIMATION CARTE] Votre pion sautille le long des pavés vers la salle...\n";
                        run.enter_room(routes[route_idx]);
                        nav.reset_to_player_floor();
                        if (run.phase == RunPhase::IN_COMBAT) {
                            pui.init(run.character, run.palette);
                        }
                    } else {
                        std::cout << ">>> Numéro de route invalide ! <<<\n";
                    }
                } catch (...) {
                    std::cout << ">>> Commande invalide ! <<<\n";
                }
            }
        } else if (run.phase == RunPhase::IN_COMBAT) {
            render_yugioh_combat(run, run.combat);
            std::string input;
            if (!(std::cin >> input)) break;

            if (input == "q" || input == "quit") {
                break;
            } else if (input == "m" || input == "deck") {
                render_deck_view(run);
            } else if (input == "e" || input == "end") {
                std::cout << "\n⚠️ [ANIMATION DUEL] Fin de tour ! Les monstres fondent en piqué sur vous...\n";
                run.combat.end_player_turn();
                run.combat.execute_enemy_turn();
                if (run.combat.player.is_alive() && run.combat.has_alive_enemies()) {
                    run.combat.start_player_turn();
                }
            } else {
                try {
                    int card_idx = std::stoi(input) - 1;
                    if (card_idx >= 0 && card_idx < static_cast<int>(run.combat.hand.size())) {
                        if (!run.combat.can_play_card(card_idx)) {
                            std::cout << "\n>>> ⚠️ ÉNERGIE INSUFFISANTE POUR CETTE CARTE ! <<<\n";
                            const auto& cdef = CardDef::get(run.combat.hand[card_idx].id);
                            if (cdef.type == CardType::ATTACK) {
                                std::cout << "\n💥 [ANIMATION DUEL] " << cdef.name << " — Charge éclair et impact avec tremblement !\n";
                            } else if (cdef.type == CardType::SKILL) {
                                std::cout << "\n🛡️ [ANIMATION DUEL] " << cdef.name << " — Déploiement de bouclier défensif !\n";
                            } else {
                                std::cout << "\n⚡ [ANIMATION DUEL] " << cdef.name << " — Éveil d'une puissance légendaire !\n";
                            }

                            DeckCard dc = run.combat.hand[card_idx];
                            int target_idx = run.combat.get_first_alive_enemy_index();

                            if (cdef.type == CardType::ATTACK && cdef.target == TargetType::SINGLE_ENEMY && run.combat.count_alive_enemies() > 1) {
                                std::cout << "Choisissez la zone du monstre à attaquer [1-" << run.combat.enemies.size() << "] : ";
                                std::string t_in;
                                if (std::cin >> t_in) {
                                    int user_tgt = std::stoi(t_in) - 1;
                                    if (user_tgt >= 0 && user_tgt < static_cast<int>(run.combat.enemies.size()) && run.combat.enemies[user_tgt].is_alive()) {
                                        target_idx = user_tgt;
                                    }
                                }
                            }

                            bool played = run.combat.play_card(card_idx, target_idx);
                            if (played) {
                                std::cout << "\n>>> ⚡ Vous jouez " << CardDef::get_display_name(dc.id, dc.upgraded) << " ! <<<\n";
                            }
                            // The CLI has no card picker: a choice asked by the card is answered with the first cards.
                            while (run.combat.choice_pending()) {
                                run.combat.resolve_choice_default();
                                std::cout << ">>> Choix de carte résolu automatiquement. <<<\n";
                            }
                        }
                    } else {
                        std::cout << "\n>>> Numéro de carte invalide ! <<<\n";
                    }
                } catch (...) {
                    std::cout << "\n>>> Commande invalide ! <<<\n";
                }
            }

            if (run.combat.state == CombatState::VICTORY) {
                std::cout << "\n🏆 VICTOIRE DU DUEL YU-GI-OH ! Les monstres ennemis sont terrassés !\n";
                run.proceed_after_combat();
            } else if (run.combat.state == CombatState::DEFEAT) {
                std::cout << "\n💀 DÉFAITE ! Vos points de vie tombent à zéro...\n";
                break;
            }
        } else if (run.phase == RunPhase::REWARD_SCREEN) {
            std::cout << "\n🎁 RÉCOMPENSES DE FIN DE COMBAT !\n";
            std::cout << "  Vous recevez : " << run.gold_reward << " Pièces d'or 💰\n";
            std::cout << "  Choisissez 1 Carte à ajouter à votre deck :\n";
            for (size_t i = 0; i < run.current_card_rewards.size(); ++i) {
                CardId cid = run.current_card_rewards[i].id;
                bool up = run.current_card_rewards[i].upgraded;
                std::cout << "    [" << (i + 1) << "] " << CardDef::get_display_name(cid, up)
                          << " (" << cost_label(cid, up) << "E) : " 
                          << CardDef::get_desc(cid, up) << "\n";
            }
            std::cout << "    [s] Passer les cartes\nChoix: ";
            std::string r_choice;
            if (std::cin >> r_choice) {
                if (r_choice == "s") {
                    run.skip_card_rewards();
                } else {
                    try {
                        int idx = std::stoi(r_choice) - 1;
                        run.pick_card_reward(idx);
                    } catch (...) {
                        run.skip_card_rewards();
                    }
                }
            }
        } else if (run.phase == RunPhase::EVENT) {
            std::cout << "\n======================================================\n";
            std::cout << "  ❓ ÉVÉNEMENT MYSTÈRE — " << run.current_event.title << "\n";
            std::cout << "------------------------------------------------------\n";
            std::cout << "  « " << run.current_event.dialogue << " »\n";
            std::cout << "------------------------------------------------------\n";
            for (size_t i = 0; i < run.current_event.choices.size(); ++i) {
                std::cout << "    [" << (i + 1) << "] " << run.current_event.choices[i].label 
                          << " (" << run.current_event.choices[i].cost_benefit << ")\n";
            }
            std::cout << "Votre décision [1-" << run.current_event.choices.size() << "]: ";
            std::string ev_choice;
            if (std::cin >> ev_choice) {
                try {
                    int c_idx = std::stoi(ev_choice) - 1;
                    char msg[128];
                    run.choose_event_choice(c_idx, msg, sizeof(msg));
                    std::cout << "\n💬 " << msg << "\n";
                } catch (...) {
                    run.phase = RunPhase::MAP_NAVIGATION;
                }
            }
        } else if (run.phase == RunPhase::CAMPFIRE) {
            std::cout << "\n🔥 FEU DE CAMP DE LA TOUR\n";
            std::cout << "  [1] Se reposer (Récupérer 30% PV = +" << (run.max_hp * 3 / 10) << " PV)\n";
            std::cout << "  [2] Forger (Améliorer une carte en version +)\n";
            std::cout << "Choix [1-2]: ";
            std::string camp_choice;
            if (std::cin >> camp_choice) {
                if (camp_choice == "1") {
                    run.rest_at_campfire();
                    std::cout << "Vous vous reposez auprès des braises. PV: " << run.hp << "/" << run.max_hp << "\n";
                } else {
                    for (size_t i = 0; i < run.master_deck.size(); ++i) {
                        if (!run.master_deck[i].upgraded) {
                            run.smith_at_campfire(static_cast<int>(i));
                            std::cout << "Votre carte a été forgée en version (+) !\n";
                            break;
                        }
                    }
                }
            }
        } else if (run.phase == RunPhase::SHOP) {
            std::cout << "\n🛒 LE MARCHAND DE LA TOUR\n";
            std::cout << "  Votre or : " << run.gold << " 💰\n";
            std::cout << "  [1] Acheter une carte\n";
            std::cout << "  [2] Acheter une relique\n";
            std::cout << "  [3] Acheter une potion\n";
            std::cout << "  [4] Retirer une carte (75 Or)\n";
            std::cout << "  [5] Quitter la boutique\n";
            std::cout << "Choix [1-5]: ";
            std::string shop_choice;
            if (std::cin >> shop_choice) {
                if (shop_choice == "5") {
                    run.leave_shop();
                } else if (shop_choice == "1" && !run.current_shop.cards.empty()) {
                    run.buy_shop_card(0);
                    run.leave_shop();
                } else if (shop_choice == "4" && run.master_deck.size() > 1) {
                    run.remove_card_at_shop(0);
                    run.leave_shop();
                } else {
                    run.leave_shop();
                }
            }
        } else if (run.phase == RunPhase::TREASURE) {
            std::cout << "\n💎 SALLE AU TRÉSOR ! Vous ouvrez le coffre et trouvez 50 Or et la relique Vajra !\n";
            run.phase = RunPhase::MAP_NAVIGATION;
        } else {
            run.phase = RunPhase::MAP_NAVIGATION;
        }
    }

    if (run.is_run_completed()) {
        std::cout << "\n🎉 VICTOIRE ABSOLUE ! Vous avez gravi les 3 Actes de la Tour !\n";
    }

    return 0;
}
