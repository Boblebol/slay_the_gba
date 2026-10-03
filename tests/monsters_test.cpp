// Monsters whose every move is a single attack, Block, or a status they give themselves: starting HP, then
// the intent, the damage, the Block and the statuses of each turn over at least one full cycle of their moves.
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <string_view>
#include "engine/combat.h"

using namespace engine;

namespace {

CombatEngine make_combat(CharacterId hero, EnemyType enemy_type) {
    CombatEngine combat;
    combat.start_combat(enemy_type, hero);
    return combat;
}

// What one enemy turn announces before it is played, and what it leaves behind once played.
struct Turn {
    EnemyIntentType intent;
    int damage;       // shown by the intent, and lost by a player without Block
    int block;        // Block the enemy holds after its turn
    int strength;     // Strength the enemy holds after its turn
    int ritual = 0;   // Ritual the enemy holds after its turn
};

// Lets the enemy play `turns` one after the other against a player who never plays a card.
void check_monster(EnemyType type, std::string_view name, int hp, std::initializer_list<Turn> turns) {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, type);
    assert(combat.enemies.size() == 1);
    Enemy& enemy = combat.enemies[0];
    assert(enemy.type == type);
    assert(enemy.name == name);
    assert(enemy.hp == hp && enemy.max_hp == hp);
    assert(enemy.block == 0 && enemy.statuses.empty());

    int turn_number = 1;
    for (const Turn& turn : turns) {
        assert(combat.turn_number == turn_number);
        assert(enemy.current_intent.type == turn.intent);
        assert(enemy.intent_damage(combat.player) == turn.damage);

        combat.player.hp = combat.player.max_hp; // long cycles must not kill the player
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == turn.damage);
        assert(enemy.block == turn.block);
        assert(enemy.get_status(StatusType::STRENGTH) == turn.strength);
        assert(enemy.get_status(StatusType::RITUAL) == turn.ritual);
        // Nothing else: no other status, and no move costs the monster HP.
        assert(enemy.statuses.size() == static_cast<size_t>((turn.strength != 0) + (turn.ritual != 0)));
        assert(enemy.hp == hp);
        ++turn_number;
    }
}

constexpr EnemyIntentType ATTACK = EnemyIntentType::ATTACK;
constexpr EnemyIntentType BUFF = EnemyIntentType::BUFF;
constexpr EnemyIntentType DEFEND = EnemyIntentType::DEFEND;
constexpr EnemyIntentType ATTACK_DEFEND = EnemyIntentType::ATTACK_DEFEND;

void test_fuzzy_wurm_crawler() {
    // Acid Goop 4, Inhale (+7 Strength), Acid Goop 4, and again.
    check_monster(EnemyType::FUZZY_WURM_CRAWLER, "Rampeluche", 55, {
        { ATTACK, 4, 0, 0 },
        { BUFF, 0, 0, 7 },
        { ATTACK, 11, 0, 7 },
        { ATTACK, 11, 0, 7 },
        { BUFF, 0, 0, 14 },
        { ATTACK, 18, 0, 14 },
    });
    std::cout << "[PASS] Fuzzy Wurm Crawler: 55 HP, Acid Goop 4, Inhale +7 Strength, Acid Goop, loop" << std::endl;
}

void test_crossbow_ruby_raider() {
    // Reload (3 Block), Fire 14, and again. Its Block is gone when its next turn starts.
    check_monster(EnemyType::CROSSBOW_RUBY_RAIDER, "Rôdeur Arbalétrier", 18, {
        { DEFEND, 0, 3, 0 },
        { ATTACK, 14, 0, 0 },
        { DEFEND, 0, 3, 0 },
        { ATTACK, 14, 0, 0 },
    });
    std::cout << "[PASS] Crossbow Ruby Raider: 18 HP, Reload 3 Block, Fire 14, loop" << std::endl;
}

void test_brute_ruby_raider() {
    // Beat 7, Roar (+3 Strength), and again.
    check_monster(EnemyType::BRUTE_RUBY_RAIDER, "Rôdeur Brute", 30, {
        { ATTACK, 7, 0, 0 },
        { BUFF, 0, 0, 3 },
        { ATTACK, 10, 0, 3 },
        { BUFF, 0, 0, 6 },
        { ATTACK, 13, 0, 6 },
    });
    std::cout << "[PASS] Brute Ruby Raider: 30 HP, Beat 7, Roar +3 Strength, loop" << std::endl;
}

void test_assassin_ruby_raider() {
    // Killshot 10, every turn.
    check_monster(EnemyType::ASSASSIN_RUBY_RAIDER, "Rôdeur assassin", 18, {
        { ATTACK, 10, 0, 0 },
        { ATTACK, 10, 0, 0 },
        { ATTACK, 10, 0, 0 },
    });
    std::cout << "[PASS] Assassin Ruby Raider: 18 HP, Killshot 10 every turn" << std::endl;
}

void test_axe_ruby_raider() {
    // Swing (5 damage, 5 Block) twice, Big Swing 12, and again.
    check_monster(EnemyType::AXE_RUBY_RAIDER, "Rôdeur à la hache", 20, {
        { ATTACK_DEFEND, 5, 5, 0 },
        { ATTACK_DEFEND, 5, 5, 0 },
        { ATTACK, 12, 0, 0 },
        { ATTACK_DEFEND, 5, 5, 0 },
        { ATTACK_DEFEND, 5, 5, 0 },
        { ATTACK, 12, 0, 0 },
    });
    std::cout << "[PASS] Axe Ruby Raider: 20 HP, Swing 5 + 5 Block twice, Big Swing 12, loop" << std::endl;
}

void test_calcified_cultist() {
    // Incantation (Ritual 2) once, then Dark Strike 9: Ritual grants Strength from the turn after it was gained.
    check_monster(EnemyType::CALCIFIED_CULTIST, "Adepte calcifié", 38, {
        { BUFF, 0, 0, 0, 2 },
        { ATTACK, 9, 0, 2, 2 },
        { ATTACK, 11, 0, 4, 2 },
        { ATTACK, 13, 0, 6, 2 },
    });
    std::cout << "[PASS] Calcified Cultist: 38 HP, Incantation Ritual 2, then Dark Strike 9, 11, 13" << std::endl;
}

void test_damp_cultist() {
    // Incantation (Ritual 5) once, then Dark Strike 1.
    check_monster(EnemyType::DAMP_CULTIST, "Adepte moisi", 51, {
        { BUFF, 0, 0, 0, 5 },
        { ATTACK, 1, 0, 5, 5 },
        { ATTACK, 6, 0, 10, 5 },
        { ATTACK, 11, 0, 15, 5 },
    });
    std::cout << "[PASS] Damp Cultist: 51 HP, Incantation Ritual 5, then Dark Strike 1, 6, 11" << std::endl;
}

void test_sewer_clam() {
    // Jet 10, Pressurize (+4 Strength), and again.
    check_monster(EnemyType::SEWER_CLAM, "Palourde d'égout", 56, {
        { ATTACK, 10, 0, 0 },
        { BUFF, 0, 0, 4 },
        { ATTACK, 14, 0, 4 },
        { BUFF, 0, 0, 8 },
        { ATTACK, 18, 0, 8 },
    });
    std::cout << "[PASS] Sewer Clam: 56 HP, Jet 10, Pressurize +4 Strength, loop" << std::endl;
}

void test_monster_block_absorbs_damage() {
    // The Block a monster gained on its turn protects it during the player's next turn.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::AXE_RUBY_RAIDER);
    Enemy& raider = combat.enemies[0];
    combat.end_player_turn(); // Swing: 5 Block
    assert(raider.block == 5);

    combat.hand.clear();
    combat.hand.push_back(CardId::STRIKE);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, 0);
    assert(played);
    assert(raider.block == 0);
    assert(raider.hp == 20 - 1); // Strike 6 against 5 Block
    std::cout << "[PASS] Monster Block: the Block of a Swing absorbs the next Strike" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Monsters ===" << std::endl;
    test_fuzzy_wurm_crawler();
    test_crossbow_ruby_raider();
    test_brute_ruby_raider();
    test_assassin_ruby_raider();
    test_axe_ruby_raider();
    test_calcified_cultist();
    test_damp_cultist();
    test_sewer_clam();
    test_monster_block_absorbs_damage();
    std::cout << "=== All Monsters Passed! ===" << std::endl;
    return 0;
}
