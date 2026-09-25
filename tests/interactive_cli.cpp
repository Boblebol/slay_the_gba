#include <iostream>
#include <string>
#include "engine/combat.h"

using namespace engine;

void render_screen(const CombatEngine& combat) {
    std::cout << "\n======================================================\n";
    std::cout << "  IRONCLAD [HP: " << combat.player.hp << "/" << combat.player.max_hp << "]";
    if (combat.player.block > 0) std::cout << " (Block: " << combat.player.block << ")";
    std::cout << "   |   " << combat.enemy.name << " [HP: " << combat.enemy.hp << "/" << combat.enemy.max_hp << "]";
    if (combat.enemy.block > 0) std::cout << " (Block: " << combat.enemy.block << ")";
    std::cout << "\n";

    std::cout << "  ENERGY: " << static_cast<int>(combat.player.energy) << "/" << static_cast<int>(combat.player.max_energy);
    std::cout << "           |   INTENT: " << combat.enemy.current_intent.description << "\n";

    // Statuses
    int str = combat.enemy.get_status(StatusType::STRENGTH);
    int vuln = combat.enemy.get_status(StatusType::VULNERABLE);
    if (str > 0 || vuln > 0) {
        std::cout << "                            |   Buffs: ";
        if (str > 0) std::cout << "[Strength +" << str << "] ";
        if (vuln > 0) std::cout << "[Vulnerable: " << vuln << "] ";
        std::cout << "\n";
    }

    std::cout << "------------------------------------------------------\n";
    if (combat.combat_log[0] != '\0') {
        std::cout << "  LOG: " << combat.combat_log << "\n";
        std::cout << "------------------------------------------------------\n";
    }

    if (combat.state == CombatState::PLAYER_TURN) {
        std::cout << "  YOUR HAND (" << combat.hand.size() << " cards) [Draw: " 
                  << combat.draw_pile.size() << " | Discard: " << combat.discard_pile.size() << "]:\n";
        for (size_t i = 0; i < combat.hand.size(); ++i) {
            const CardDef& c = CardDef::get(combat.hand[i]);
            std::cout << "    [" << (i + 1) << "] " << c.name 
                      << " (" << static_cast<int>(c.cost) << "E) - " << c.description << "\n";
        }
        std::cout << "  Actions: Type 1-" << combat.hand.size() << " to play a card, 'e' to end turn, 'q' to quit.\n";
    } else if (combat.state == CombatState::VICTORY) {
        std::cout << "  🏆 VICTORY! Cultist was defeated!\n";
        std::cout << "  Burning Blood heals 6 HP. Ironclad HP: " << combat.player.hp << "/" << combat.player.max_hp << "\n";
    } else if (combat.state == CombatState::DEFEAT) {
        std::cout << "  💀 DEFEAT! You were slain in the Spire...\n";
    }
    std::cout << "======================================================\n";
}

int main() {
    std::cout << "======================================================\n";
    std::cout << "        SLAY THE GBA - Terminal Prototype             \n";
    std::cout << "======================================================\n";

    CombatEngine combat;
    combat.start_combat(EnemyType::CULTIST);

    while (combat.state == CombatState::PLAYER_TURN) {
        render_screen(combat);
        std::cout << "\n> ";
        std::string input;
        if (!(std::cin >> input)) break;

        if (input == "q" || input == "quit") {
            std::cout << "Exiting...\n";
            break;
        }

        if (input == "e" || input == "end") {
            combat.end_player_turn();
            continue;
        }

        try {
            int card_idx = std::stoi(input) - 1;
            if (card_idx >= 0 && card_idx < static_cast<int>(combat.hand.size())) {
                if (!combat.play_card(card_idx)) {
                    std::cout << ">>> Not enough energy or invalid play! <<<\n";
                }
            } else {
                std::cout << ">>> Invalid card number! <<<\n";
            }
        } catch (...) {
            std::cout << ">>> Unknown command. Use card number, 'e', or 'q'. <<<\n";
        }
    }

    render_screen(combat);
    return 0;
}
