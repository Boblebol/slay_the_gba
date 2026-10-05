// Monsters whose every move is a single attack, Block, or a status they give themselves: starting HP, then
// the intent, the damage, the Block and the statuses of each turn over at least one full cycle of their moves.
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <string_view>
#include "engine/combat.h"
#include "engine/run.h"

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
constexpr EnemyIntentType DEBUFF = EnemyIntentType::DEBUFF;

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

// An enemy that does nothing but what the test writes into its intent.
CombatEngine make_debuffer(std::initializer_list<IntentDebuff> debuffs, EnemyIntentType type = EnemyIntentType::DEBUFF,
                           int16_t damage = 0) {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::ASSASSIN_RUBY_RAIDER);
    Enemy& enemy = combat.enemies[0];
    enemy.current_intent = EnemyIntent{};
    enemy.current_intent.type = type;
    enemy.current_intent.base_damage = damage;
    size_t slot = 0;
    for (const IntentDebuff& debuff : debuffs) enemy.current_intent.debuffs[slot++] = debuff;
    return combat;
}

void test_debuff_intents() {
    // Weak, Vulnerable, Frail: the player holds them once the enemy played, and the record of the turn says so.
    for (StatusType type : { StatusType::WEAK, StatusType::VULNERABLE, StatusType::FRAIL }) {
        CombatEngine combat = make_debuffer({ { type, 2 } });
        assert(combat.enemies[0].current_intent.debuffs_player());
        assert(combat.enemies[0].intent_damage(combat.player) == 0);
        combat.end_player_turn();
        assert(combat.player.get_status(type) == 2 && combat.player.hp == combat.player.max_hp);
        const EnemyActionRecord& record = combat.last_enemy_actions[0];
        assert(record.intent_type == DEBUFF && record.debuff_attempted);
        assert(record.debuffs[0].type == type && record.debuffs[0].amount == 2);
        assert(record.debuffs[1].type == StatusType::NONE);
    }
    // An intent without debuff announces none.
    CombatEngine plain = make_combat(CharacterId::IRONCLAD, EnemyType::ASSASSIN_RUBY_RAIDER);
    assert(!plain.enemies[0].current_intent.debuffs_player());
    plain.end_player_turn();
    assert(!plain.last_enemy_actions[0].debuff_attempted && plain.player.statuses.empty());

    // Strength and Dexterity taken away, two statuses in one move.
    CombatEngine drained = make_debuffer({ { StatusType::STRENGTH, -1 }, { StatusType::DEXTERITY, -2 } });
    drained.end_player_turn();
    assert(drained.player.get_status(StatusType::STRENGTH) == -1 && drained.player.get_status(StatusType::DEXTERITY) == -2);
    assert(drained.last_enemy_actions[0].debuffs[1].type == StatusType::DEXTERITY);
    // They last: nothing wears them off at the end of the round.
    drained.end_player_turn();
    assert(drained.player.get_status(StatusType::STRENGTH) == -1 && drained.player.get_status(StatusType::DEXTERITY) == -2);

    // The debuffs change what the player's cards do: a Strike of 6 deals 4 when Weak, a Defend of 5 gives 3 when
    // Frail, and 3 with 2 Dexterity less.
    auto play = [](CombatEngine& combat, CardId card) {
        combat.hand.clear();
        combat.hand.push_back(card);
        combat.player.energy = combat.player.max_energy;
        combat.player.block = 0;
        bool played = combat.play_card(0, 0);
        assert(played);
    };
    CombatEngine weak = make_debuffer({ { StatusType::WEAK, 1 } });
    weak.end_player_turn();
    int16_t hp = weak.enemies[0].hp;
    play(weak, CardId::STRIKE);
    assert(hp - weak.enemies[0].hp == 4);
    CombatEngine frail = make_debuffer({ { StatusType::FRAIL, 1 } });
    frail.end_player_turn();
    play(frail, CardId::DEFEND);
    assert(frail.player.block == 3);
    play(drained, CardId::DEFEND);
    assert(drained.player.block == 3);

    // Attack, then debuff: the hit comes first, so the Vulnerable of this move does not raise its own damage.
    CombatEngine struck = make_debuffer({ { StatusType::VULNERABLE, 1 } }, EnemyIntentType::ATTACK_DEBUFF, 10);
    assert(struck.enemies[0].intent_damage(struck.player) == 10);
    struck.end_player_turn();
    assert(struck.player.max_hp - struck.player.hp == 10 && struck.player.get_status(StatusType::VULNERABLE) == 1);
    std::cout << "[PASS] Debuff intents: Weak, Vulnerable, Frail, Strength and Dexterity loss, alone or after an attack"
              << std::endl;
}

void test_debuff_duration() {
    // A debuff of 1 turn given by an enemy lasts the whole next turn of the player, then wears off.
    CombatEngine combat = make_debuffer({ { StatusType::WEAK, 1 } });
    combat.end_player_turn();
    assert(combat.state == CombatState::PLAYER_TURN && combat.player.get_status(StatusType::WEAK) == 1);
    combat.enemies[0].current_intent = EnemyIntent{};
    combat.enemies[0].current_intent.type = EnemyIntentType::DEFEND;
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::WEAK) == 0);

    // Given again while it still runs, it is not worn down the round it is renewed: 1 + 2 stays 3.
    CombatEngine renewed = make_debuffer({ { StatusType::FRAIL, 1 } });
    renewed.end_player_turn();
    renewed.enemies[0].current_intent = EnemyIntent{};
    renewed.enemies[0].current_intent.type = EnemyIntentType::DEBUFF;
    renewed.enemies[0].current_intent.debuffs[0] = { StatusType::FRAIL, 2 };
    renewed.end_player_turn();
    assert(renewed.player.get_status(StatusType::FRAIL) == 3);

    // A debuff the player already had before the enemies' turn keeps wearing off as before.
    CombatEngine old = make_combat(CharacterId::IRONCLAD, EnemyType::ASSASSIN_RUBY_RAIDER);
    old.player.add_status(StatusType::VULNERABLE, 2);
    old.end_player_turn();
    assert(old.player.get_status(StatusType::VULNERABLE) == 1);
    std::cout << "[PASS] Debuff intents: a debuff of 1 turn lasts the player's next turn" << std::endl;
}

void test_debuff_and_artifact() {
    // Artifact cancels one debuff and is used up: of two statuses, only the second one lands.
    CombatEngine combat = make_debuffer({ { StatusType::WEAK, 2 }, { StatusType::DEXTERITY, -2 } });
    combat.player.add_status(StatusType::ARTIFACT, 1);
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::WEAK) == 0 && combat.player.get_status(StatusType::ARTIFACT) == 0);
    assert(combat.player.get_status(StatusType::DEXTERITY) == -2);
    const EnemyActionRecord& record = combat.last_enemy_actions[0];
    assert(record.debuff_attempted && record.debuffs[0].type == StatusType::NONE &&
           record.debuffs[1].type == StatusType::DEXTERITY);

    // With enough Artifact nothing lands, and the record still says the move tried.
    CombatEngine immune = make_debuffer({ { StatusType::WEAK, 2 }, { StatusType::FRAIL, 2 } });
    immune.player.add_status(StatusType::ARTIFACT, 2);
    immune.end_player_turn();
    assert(immune.player.statuses.empty());
    assert(immune.last_enemy_actions[0].debuff_attempted &&
           immune.last_enemy_actions[0].debuffs[0].type == StatusType::NONE &&
           immune.last_enemy_actions[0].debuffs[1].type == StatusType::NONE);
    std::cout << "[PASS] Debuff intents: Artifact cancels them one by one" << std::endl;
}

void test_the_forgotten() {
    // Miasma (8 Block, takes 2 Dexterity from the player and gains 2), Dread (13 + its Dexterity), and again. Its
    // Block follows its Dexterity like the player's: 8, then 10, then 12.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::THE_FORGOTTEN);
    Enemy& enemy = combat.enemies[0];
    assert(enemy.name == "L'Oublié" && enemy.hp == 106 && enemy.max_hp == 106);
    assert(enemy.block == 0 && enemy.statuses.empty());

    struct Turn {
        EnemyIntentType intent;
        int damage;
        int block;
        int dexterity;        // of the Forgotten after its turn
        int player_dexterity; // of the player after that turn
    };
    int turn_number = 1;
    for (const Turn& turn : { Turn{ DEBUFF, 0, 8, 2, -2 }, Turn{ ATTACK, 15, 0, 2, -2 }, Turn{ DEBUFF, 0, 10, 4, -4 },
                              Turn{ ATTACK, 17, 0, 4, -4 }, Turn{ DEBUFF, 0, 12, 6, -6 }, Turn{ ATTACK, 19, 0, 6, -6 } }) {
        assert(combat.turn_number == turn_number++);
        assert(enemy.current_intent.type == turn.intent);
        assert(enemy.current_intent.debuffs_player() == (turn.intent == DEBUFF));
        assert(enemy.intent_damage(combat.player) == turn.damage);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == turn.damage);
        assert(enemy.block == turn.block);
        assert(enemy.get_status(StatusType::DEXTERITY) == turn.dexterity && enemy.statuses.size() == 1);
        assert(combat.player.get_status(StatusType::DEXTERITY) == turn.player_dexterity);
        assert(enemy.hp == 106);
    }

    // Possess Speed: killed, it gives back all the Dexterity it stole.
    assert(enemy.stolen.type == StatusType::DEXTERITY && enemy.stolen.amount == -6);
    combat.player.add_status(StatusType::DEXTERITY, 1); // gained by the player on their own: not part of the loot
    enemy.hp = 1;
    combat.hand.clear();
    combat.hand.push_back(CardId::STRIKE);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, 0);
    assert(played && !enemy.is_alive());
    assert(combat.player.get_status(StatusType::DEXTERITY) == 1);

    // Artifact keeps the player's Dexterity: nothing is stolen, so nothing is given back, but it still gains its own.
    CombatEngine guarded = make_combat(CharacterId::IRONCLAD, EnemyType::THE_FORGOTTEN);
    guarded.player.add_status(StatusType::ARTIFACT, 1);
    guarded.end_player_turn();
    assert(guarded.player.get_status(StatusType::DEXTERITY) == 0 && guarded.enemies[0].stolen.amount == 0);
    assert(guarded.enemies[0].get_status(StatusType::DEXTERITY) == 2);
    guarded.enemies[0].return_stolen(guarded.player);
    assert(guarded.player.get_status(StatusType::DEXTERITY) == 0);
    std::cout << "[PASS] The Forgotten: 106 HP, Miasma 8 Block and 2 Dexterity stolen, Dread 13 + Dexterity, gives it back"
              << std::endl;
}

void test_the_forgotten_ascension() {
    // In a run, its own Ascension values replace the percentages: 111 HP when the monsters are tougher (from A7),
    // Dread 15 when they hit harder (from A2).
    auto fight = [](uint8_t ascension) {
        RunManager run;
        run.start_run(CharacterId::IRONCLAD, ascension, PaletteVariant::CLASSIC, 777);
        run.map.nodes[0].type = RoomType::MONSTER;
        run.enter_room(0);
        FixedVector<EnemyType, 3> pack;
        pack.push_back(EnemyType::THE_FORGOTTEN);
        run.combat.start_combat_multi(pack, run.character);
        Enemy& foe = run.combat.enemies[0];
        foe.ascend(run.get_enemy_hp_multiplier() > 100, run.get_enemy_damage_multiplier() > 100);
        return run;
    };
    struct Level {
        uint8_t ascension;
        int hp;
        int dread;
    };
    for (const Level& level : { Level{ 0, 106, 13 }, Level{ 2, 106, 15 }, Level{ 7, 111, 15 }, Level{ 9, 111, 15 } }) {
        RunManager run = fight(level.ascension);
        Enemy& foe = run.combat.enemies[0];
        assert(foe.hp == level.hp && foe.max_hp == level.hp && foe.damage_percent == 100);
        assert(foe.current_intent.type == DEBUFF && foe.current_intent.block == 8);
        run.combat.end_player_turn();
        assert(foe.current_intent.base_damage == level.dread);
        assert(foe.intent_damage(run.combat.player) == level.dread + 2);
    }
    // The monsters without values of their own keep the percentages of the run.
    Enemy clam;
    clam.init(EnemyType::SEWER_CLAM);
    assert(clam.ascension_hp() == 0);
    Enemy forgotten;
    forgotten.init(EnemyType::THE_FORGOTTEN);
    assert(forgotten.ascension_hp() == 111);
    std::cout << "[PASS] The Forgotten: Ascension gives 111 HP and Dread 15" << std::endl;
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
    test_debuff_intents();
    test_debuff_duration();
    test_debuff_and_artifact();
    test_the_forgotten();
    test_the_forgotten_ascension();
    std::cout << "=== All Monsters Passed! ===" << std::endl;
    return 0;
}
