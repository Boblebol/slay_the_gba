// Monsters whose every move is a single attack, Block, or a status they give themselves: starting HP, then
// the intent, the damage, the Block and the statuses of each turn over at least one full cycle of their moves.
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <map>
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
    int hits = 1;     // times the attack hits, `damage` each
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
        assert(combat.player.max_hp - combat.player.hp == turn.damage * turn.hits);
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
constexpr EnemyIntentType ATTACK_BUFF = EnemyIntentType::ATTACK_BUFF;
constexpr EnemyIntentType STUN = EnemyIntentType::STUN;
constexpr EnemyIntentType ESCAPE = EnemyIntentType::ESCAPE;
constexpr EnemyIntentType ATTACK_DEBUFF = EnemyIntentType::ATTACK_DEBUFF;
constexpr EnemyIntentType STATUS = EnemyIntentType::STATUS;

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

void test_tracker_ruby_raider() {
    // 21 to 25 HP, Track (2 Frail) once, then Unleash the Hounds: 1 damage, 8 hits, every turn.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TRACKER_RUBY_RAIDER);
    Enemy& raider = combat.enemies[0];
    assert(raider.name == "Rôdeur Traqueur" && raider.hp >= 21 && raider.hp <= 25 && raider.max_hp == raider.hp);
    assert(raider.current_intent.type == DEBUFF && raider.intent_damage(combat.player) == 0);
    assert(raider.current_intent.debuffs[0].type == StatusType::FRAIL && raider.current_intent.debuffs[0].amount == 2);
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::FRAIL) >= 1 && combat.player.hp == combat.player.max_hp);
    for (int turn = 2; turn <= 4; ++turn) {
        assert(raider.current_intent.type == ATTACK && raider.current_intent.hits == 8);
        assert(raider.intent_damage(combat.player) == 1 && raider.block == 0);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 8);
    }
    std::cout << "[PASS] Tracker Ruby Raider: 21 to 25 HP, Track 2 Frail, then Hounds 1 x8" << std::endl;
}

void test_tracker_ruby_raider_ascension() {
    // 22 to 26 HP and 9 hits at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TRACKER_RUBY_RAIDER);
        Enemy& raider = combat.enemies[0];
        raider.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = raider.hp;
        assert(raider.ascension_hp() == 22 && raider.hp_spread() == 4);
        assert(base >= 21 && base <= 25);
        raider.ascend(true, true);
        assert(raider.hp == base + 1 && raider.hp >= 22 && raider.hp <= 26);
        combat.end_player_turn();
        assert(raider.current_intent.type == ATTACK && raider.current_intent.hits == 9);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 9);
    }
    std::cout << "[PASS] Tracker Ruby Raider: Ascension gives 22 to 26 HP and 9 hits" << std::endl;
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
        run.combat.plan_enemies();
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

void test_living_shield() {
    // Alone: one Shield Slam 6, then Smash (16 damage, then +3 Strength) until the end.
    check_monster(EnemyType::LIVING_SHIELD, "Bouclier vivant", 55, {
        { ATTACK, 6, 0, 0 },
        { ATTACK_BUFF, 16, 0, 3 },
        { ATTACK_BUFF, 19, 0, 6 },
        { ATTACK_BUFF, 22, 0, 9 },
    });
    std::cout << "[PASS] Living Shield: 55 HP, Shield Slam 6, then alone Smash 16 and +3 Strength each turn" << std::endl;
}

// A fight against `types`, the player never playing a card.
CombatEngine make_pack(std::initializer_list<EnemyType> types) {
    CombatEngine combat;
    FixedVector<EnemyType, 3> pack;
    for (EnemyType type : types) pack.push_back(type);
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    return combat;
}

void pass_turn(CombatEngine& combat) {
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.state == CombatState::PLAYER_TURN);
}

void test_phase_with_entry_condition() {
    // With someone to protect, the Living Shield stays in its first phase: Shield Slam, turn after turn.
    CombatEngine combat = make_pack({ EnemyType::LIVING_SHIELD, EnemyType::SEWER_CLAM });
    Enemy& shield = combat.enemies[0];
    for (int turn = 1; turn <= 4; ++turn) {
        assert(shield.phase == 0 && shield.phase_turn == turn);
        assert(shield.current_intent.type == ATTACK && shield.current_intent.base_damage == 6);
        pass_turn(combat);
    }
    assert(shield.get_status(StatusType::STRENGTH) == 0);

    // Its ally dies during the player's turn: the move already announced is played, and the next plan enters the
    // second phase, where the turns are counted from 1 again.
    combat.enemies[1].hp = 0;
    assert(shield.current_intent.type == ATTACK && shield.intent_damage(combat.player) == 6);
    pass_turn(combat);
    assert(combat.player.max_hp - combat.player.hp == 6);
    assert(shield.phase == 1 && shield.phase_turn == 1);
    assert(shield.current_intent.type == ATTACK_BUFF && shield.intent_damage(combat.player) == 16);
    pass_turn(combat);
    assert(combat.player.max_hp - combat.player.hp == 16 && shield.get_status(StatusType::STRENGTH) == 3);
    assert(shield.phase == 1 && shield.phase_turn == 2 && shield.intent_damage(combat.player) == 19);

    // A phase is never left for an earlier one, nor entered twice.
    assert(!shield.enter_phase(0) && !shield.enter_phase(1) && shield.phase == 1 && shield.phase_turn == 2);
    assert(shield.enter_phase(2) && shield.phase == 2 && shield.phase_turn == 0);

    // The place of each monster and the allies it has left are what it is given to decide.
    CombatEngine trio = make_pack({ EnemyType::LIVING_SHIELD, EnemyType::LIVING_SHIELD, EnemyType::LIVING_SHIELD });
    trio.enemies[0].hp = 0;
    pass_turn(trio);
    assert(trio.enemies[1].phase == 0 && trio.enemies[2].phase == 0); // one ally each
    trio.enemies[2].hp = 0;
    pass_turn(trio);
    assert(trio.enemies[1].phase == 1);

    // A new fight starts the pattern again.
    combat.start_combat(EnemyType::LIVING_SHIELD, CharacterId::IRONCLAD);
    assert(combat.enemies[0].phase == 0 && combat.enemies[0].phase_turn == 1 && combat.enemies[0].last_move == NO_MOVE);
    std::cout << "[PASS] Intent phases: entered when the condition is met at planning, never left, reset by a new fight"
              << std::endl;
}

void test_weighted_draw_without_repetition() {
    enum Move : uint8_t { BITE, GROWL, LEAP };
    const MoveOption options[] = { { BITE, 6 }, { GROWL, 3 }, { LEAP, 1 } };

    // Never the same move twice in a row, and each move drawn in proportion to its weight among the ones allowed.
    Enemy enemy;
    enemy.init(EnemyType::SEWER_CLAM);
    enemy.reset_pattern(2024);
    std::map<uint8_t, int> drawn;
    std::map<uint8_t, int> after_bite;
    uint8_t previous = NO_MOVE;
    const int draws = 30000;
    for (int i = 0; i < draws; ++i) {
        uint8_t move = enemy.draw_move(options);
        assert(move == BITE || move == GROWL || move == LEAP);
        assert(move != previous && enemy.last_move == move);
        if (previous == BITE) ++after_bite[move];
        ++drawn[move];
        previous = move;
    }
    // After a Bite only Growl (3) and Leap (1) are left: three quarters, one quarter.
    int bites = after_bite[GROWL] + after_bite[LEAP];
    assert(bites > 10000);
    assert(after_bite[GROWL] * 100 / bites >= 72 && after_bite[GROWL] * 100 / bites <= 78);
    assert(drawn[BITE] > drawn[GROWL] && drawn[GROWL] > drawn[LEAP] && drawn[LEAP] > 0);

    // A move that can repeat follows its weight alone: 6 out of 10.
    const MoveOption repeating[] = { { BITE, 6, true }, { GROWL, 3, true }, { LEAP, 1, true } };
    enemy.reset_pattern(7);
    int bite_count = 0;
    int repeats = 0;
    previous = NO_MOVE;
    for (int i = 0; i < draws; ++i) {
        uint8_t move = enemy.draw_move(repeating);
        bite_count += move == BITE;
        repeats += move == previous;
        previous = move;
    }
    assert(bite_count * 100 / draws >= 58 && bite_count * 100 / draws <= 62 && repeats > 0);

    // A scripted move counts as the last one played: the draw that follows leaves it out.
    const MoveOption two[] = { { BITE, 200 }, { GROWL, 1 } };
    for (uint32_t seed = 1; seed < 50; ++seed) {
        enemy.reset_pattern(seed);
        enemy.use_move(BITE);
        assert(enemy.draw_move(two) == GROWL);
    }
    // Nothing else to play: the move is played again. No option at all: no move.
    const MoveOption only[] = { { LEAP, 1 } };
    enemy.reset_pattern(3);
    assert(enemy.draw_move(only) == LEAP && enemy.draw_move(only) == LEAP);
    assert(enemy.draw_move({}) == NO_MOVE);

    // Same seed, same moves; and in a fight each monster gets a stream of its own, without touching the shuffles.
    Enemy first;
    first.init(EnemyType::SEWER_CLAM);
    Enemy second = first;
    first.reset_pattern(99);
    second.reset_pattern(99);
    for (int i = 0; i < 50; ++i) assert(first.draw_move(options) == second.draw_move(options));
    CombatEngine pack = make_pack({ EnemyType::SEWER_CLAM, EnemyType::SEWER_CLAM });
    assert(pack.enemies[0].move_seed != pack.enemies[1].move_seed);
    std::cout << "[PASS] Move draws: by weight, never twice in a row unless allowed, one stream per monster" << std::endl;
}

void test_living_shield_ascension() {
    Enemy shield;
    shield.init(EnemyType::LIVING_SHIELD);
    assert(shield.ascension_hp() == 65);
    struct Level {
        bool more_hp;
        bool more_damage;
        int hp;
        int smash;
    };
    for (const Level& level : { Level{ false, false, 55, 16 }, Level{ true, false, 65, 16 }, Level{ false, true, 55, 18 },
                                Level{ true, true, 65, 18 } }) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::LIVING_SHIELD);
        Enemy& foe = combat.enemies[0];
        foe.ascend(level.more_hp, level.more_damage);
        combat.plan_enemies();
        assert(foe.hp == level.hp && foe.max_hp == level.hp);
        assert(foe.intent_damage(combat.player) == 6); // Shield Slam has no Ascension value
        pass_turn(combat);
        assert(foe.current_intent.type == ATTACK_BUFF && foe.intent_damage(combat.player) == level.smash);
    }
    std::cout << "[PASS] Living Shield: Ascension gives 65 HP and Smash 18" << std::endl;
}

void test_stun_intent() {
    // A stunned monster loses its turn: no damage, no Block, no status, and its pattern goes on the turn after.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SEWER_CLAM);
    Enemy& clam = combat.enemies[0];
    assert(clam.current_intent.type == ATTACK && clam.intent_damage(combat.player) == 10);
    clam.stun();
    assert(clam.current_intent.type == STUN && clam.intent_damage(combat.player) == 0);
    assert(!clam.current_intent.debuffs_player());
    combat.end_player_turn();
    assert(combat.state == CombatState::PLAYER_TURN);
    assert(combat.player.hp == combat.player.max_hp && clam.block == 0 && clam.statuses.empty() && clam.hp == 56);
    assert(combat.last_enemy_actions[0].intent_type == STUN && combat.last_enemy_actions[0].damage_to_player == 0);
    // Turn 2 of the Clam: Pressurize, as if nothing had happened.
    assert(clam.current_intent.type == BUFF);

    // The statuses of a stunned monster still wear off, and its Block still falls.
    clam.add_status(StatusType::VULNERABLE, 1);
    clam.block = 7;
    clam.stun();
    combat.end_player_turn();
    assert(clam.get_status(StatusType::VULNERABLE) == 0 && clam.block == 0);
    std::cout << "[PASS] Stun intent: the monster loses its turn, then follows its pattern again" << std::endl;
}

void test_heal_intent() {
    // The monster gives itself HP back, never above its maximum, and the record of the turn says how many.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SEWER_CLAM);
    Enemy& clam = combat.enemies[0];
    clam.hp = 20;
    clam.current_intent = EnemyIntent{};
    clam.current_intent.type = EnemyIntentType::HEAL;
    clam.current_intent.heal = 30;
    combat.end_player_turn();
    assert(clam.hp == 50 && combat.last_enemy_actions[0].healed == 30);
    assert(combat.player.hp == combat.player.max_hp);

    clam.current_intent = EnemyIntent{};
    clam.current_intent.type = EnemyIntentType::HEAL;
    clam.current_intent.heal = 30;
    combat.end_player_turn();
    assert(clam.hp == 56 && clam.max_hp == 56 && combat.last_enemy_actions[0].healed == 6);

    // With an attack (Ponder: 11 damage and 30 HP): it hits, then heals. The next move heals nothing.
    clam.hp = 10;
    clam.current_intent = EnemyIntent{};
    clam.current_intent.type = EnemyIntentType::ATTACK;
    clam.current_intent.base_damage = 11;
    clam.current_intent.heal = 30;
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 11 && clam.hp == 40);
    assert(combat.last_enemy_actions[0].healed == 30);
    combat.end_player_turn();
    assert(clam.hp == 40 && combat.last_enemy_actions[0].healed == 0);

    // A monster killed by what its attack sent back (Thorns) is not healed.
    CombatEngine thorns = make_combat(CharacterId::IRONCLAD, EnemyType::SEWER_CLAM);
    thorns.player.add_status(StatusType::THORNS, 5);
    thorns.enemies[0].hp = 3;
    thorns.enemies[0].current_intent.heal = 30;
    thorns.end_player_turn();
    assert(!thorns.enemies[0].is_alive() && thorns.state == CombatState::VICTORY);
    std::cout << "[PASS] Heal intent: HP given back up to the maximum, alone or after an attack" << std::endl;
}

void test_escape_intent() {
    // A monster that runs away is out of the fight without being killed: nothing reacts to a death.
    CombatEngine combat = make_pack({ EnemyType::SEWER_CLAM, EnemyType::FAT_GREMLIN });
    combat.add_power(PowerId::GREMLIN_HORN, 1); // 1 energy and 1 card for each enemy killed
    Enemy& gremlin = combat.enemies[1];
    assert(gremlin.current_intent.type == STUN);
    pass_turn(combat);
    assert(gremlin.current_intent.type == ESCAPE && gremlin.is_alive() && !gremlin.escaped);
    size_t hand = combat.hand.size();
    pass_turn(combat);
    assert(gremlin.escaped && !gremlin.is_alive() && gremlin.block == 0);
    assert(combat.last_enemy_actions[1].escaped && !combat.last_enemy_actions[0].escaped);
    assert(combat.player.energy == combat.player.max_energy && combat.hand.size() == hand); // no kill reward
    assert(combat.enemies[0].is_alive() && !combat.every_enemy_escaped());
    // The fight goes on against the others, and the one that left plans nothing more.
    assert(combat.state == CombatState::PLAYER_TURN && combat.enemies[0].is_alive());
    pass_turn(combat);
    assert(gremlin.escaped && gremlin.hp == 0);

    // A thief that runs away keeps what it stole.
    CombatEngine thief = make_combat(CharacterId::IRONCLAD, EnemyType::THE_FORGOTTEN);
    thief.end_player_turn();
    assert(thief.player.get_status(StatusType::DEXTERITY) == -2);
    thief.enemies[0].current_intent = EnemyIntent{};
    thief.enemies[0].current_intent.type = ESCAPE;
    thief.end_player_turn();
    assert(thief.enemies[0].escaped && thief.player.get_status(StatusType::DEXTERITY) == -2);
    assert(thief.state == CombatState::VICTORY && thief.every_enemy_escaped());

    // Killed before it leaves, it is a kill like any other.
    CombatEngine killed = make_combat(CharacterId::IRONCLAD, EnemyType::FAT_GREMLIN);
    killed.enemies[0].hp = 1;
    killed.hand.clear();
    killed.hand.push_back(CardId::STRIKE);
    killed.player.energy = killed.player.max_energy;
    bool played = killed.play_card(0, 0);
    assert(played && killed.state == CombatState::VICTORY);
    assert(!killed.enemies[0].escaped && !killed.every_enemy_escaped());
    std::cout << "[PASS] Escape intent: out of the fight, not a kill, keeps what it stole" << std::endl;
}

void test_escape_gives_no_reward() {
    auto fight = [](std::initializer_list<EnemyType> types) {
        RunManager run;
        run.start_run(CharacterId::SILENT, 0, PaletteVariant::CLASSIC, 777);
        run.map.nodes[0].type = RoomType::MONSTER;
        run.enter_room(0);
        FixedVector<EnemyType, 3> pack;
        for (EnemyType type : types) pack.push_back(type);
        run.combat.start_combat_multi(pack, run.character);
        return run;
    };

    // Every enemy ran away: the fight ends, back to the map with no gold, no card and nothing counted as won.
    RunManager fled = fight({ EnemyType::FAT_GREMLIN });
    int16_t gold = fled.gold;
    size_t deck = fled.master_deck.size();
    fled.combat.end_player_turn();
    fled.combat.player.hp = 33;
    fled.combat.end_player_turn();
    assert(fled.combat.state == CombatState::VICTORY && fled.combat.every_enemy_escaped());
    fled.proceed_after_combat();
    assert(fled.phase == RunPhase::MAP_NAVIGATION);
    assert(fled.gold == gold && fled.gold_reward == 0 && fled.master_deck.size() == deck);
    assert(fled.hook_count(RunHook::COMBAT_WON) == 0 && fled.hook_count(RunHook::REWARDS_GENERATED) == 0);
    assert(fled.hp == 33); // what the fight cost is kept

    // One enemy killed, the other gone: the fight is won and gives its rewards.
    RunManager won = fight({ EnemyType::SEWER_CLAM, EnemyType::FAT_GREMLIN });
    won.combat.end_player_turn();
    won.combat.player.hp = won.combat.player.max_hp;
    won.combat.end_player_turn();
    assert(won.combat.enemies[1].escaped && won.combat.state == CombatState::PLAYER_TURN);
    won.combat.enemies[0].hp = 1;
    won.combat.hand.clear();
    won.combat.hand.push_back(CardId::STRIKE);
    won.combat.player.energy = won.combat.player.max_energy;
    bool played = won.combat.play_card(0, 0);
    assert(played && won.combat.state == CombatState::VICTORY && !won.combat.every_enemy_escaped());
    won.proceed_after_combat();
    assert(won.phase == RunPhase::REWARD_SCREEN && won.gold_reward > 0);
    assert(won.hook_count(RunHook::COMBAT_WON) == 1);
    std::cout << "[PASS] Escape intent: no reward when every enemy ran away, the usual ones when one was killed"
              << std::endl;
}

void test_fat_gremlin() {
    // Spawned (stunned), then Flee. Its HP are drawn between 13 and 17 at the start of the fight.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FAT_GREMLIN);
    Enemy& gremlin = combat.enemies[0];
    assert(gremlin.type == EnemyType::FAT_GREMLIN && gremlin.name == "Gros diablotin");
    assert(gremlin.hp >= 13 && gremlin.hp <= 17 && gremlin.max_hp == gremlin.hp && gremlin.max_hp == 13 + gremlin.hp_bonus);
    assert(gremlin.block == 0 && gremlin.statuses.empty());

    assert(combat.turn_number == 1 && gremlin.current_intent.type == STUN && gremlin.intent_damage(combat.player) == 0);
    int16_t hp = gremlin.hp;
    combat.end_player_turn();
    assert(combat.state == CombatState::PLAYER_TURN && combat.player.hp == combat.player.max_hp);
    assert(gremlin.hp == hp && gremlin.block == 0 && gremlin.statuses.empty());

    assert(combat.turn_number == 2 && gremlin.current_intent.type == ESCAPE && gremlin.intent_damage(combat.player) == 0);
    combat.end_player_turn();
    assert(gremlin.escaped && combat.player.hp == combat.player.max_hp);
    assert(combat.state == CombatState::VICTORY && combat.every_enemy_escaped());

    // Kept in the fight by a stun, it tries to flee again: Flee loops.
    CombatEngine held = make_pack({ EnemyType::FAT_GREMLIN, EnemyType::SEWER_CLAM });
    pass_turn(held);
    for (int turn = 0; turn < 3; ++turn) {
        assert(held.enemies[0].current_intent.type == ESCAPE);
        held.enemies[0].stun();
        pass_turn(held);
        assert(!held.enemies[0].escaped && held.enemies[0].is_alive());
    }

    // Every HP of the range is met over the fights of a run, and a monster with fixed HP never moves.
    bool seen[5] = {};
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 2024);
    for (int i = 0; i < 300; ++i) {
        FixedVector<EnemyType, 3> pack;
        pack.push_back(EnemyType::FAT_GREMLIN);
        pack.push_back(EnemyType::SEWER_CLAM);
        run.combat.start_combat_multi(pack, run.character);
        seen[run.combat.enemies[0].hp - 13] = true;
        assert(run.combat.enemies[1].hp == 56 && run.combat.enemies[1].hp_bonus == 0);
        run.combat.end_player_turn(); // the shuffles move the fight's random sequence on
    }
    for (bool hit : seen) assert(hit);
    std::cout << "[PASS] Fat Gremlin: 13 to 17 HP, Spawned (stunned), then Flee" << std::endl;
}

void test_fat_gremlin_ascension() {
    // 14 to 18 HP at Ascension, on the same draw; it has no attack to raise.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FAT_GREMLIN);
        Enemy& gremlin = combat.enemies[0];
        gremlin.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = gremlin.hp;
        assert(gremlin.ascension_hp() == 14 && gremlin.hp_spread() == 4);
        gremlin.ascend(false, true);
        assert(gremlin.hp == base);
        gremlin.ascend(true, true);
        combat.plan_enemies();
        assert(gremlin.hp == base + 1 && gremlin.max_hp == base + 1 && gremlin.hp >= 14 && gremlin.hp <= 18);
        assert(gremlin.current_intent.type == STUN);
    }
    std::cout << "[PASS] Fat Gremlin: Ascension gives 14 to 18 HP" << std::endl;
}

void test_sneaky_gremlin() {
    // Spawned (stunned), then Tackle 9 every turn. Its HP are drawn between 10 and 14 at the start of the fight.
    CombatEngine probe = make_combat(CharacterId::IRONCLAD, EnemyType::SNEAKY_GREMLIN);
    const Enemy& gremlin = probe.enemies[0];
    assert(gremlin.hp >= 10 && gremlin.hp <= 14 && gremlin.max_hp == 10 + gremlin.hp_bonus && gremlin.hp_spread() == 4);
    check_monster(EnemyType::SNEAKY_GREMLIN, "Diablotin sournois", gremlin.hp, {
        { STUN,   0, 0, 0 },
        { ATTACK, 9, 0, 0 },
        { ATTACK, 9, 0, 0 },
        { ATTACK, 9, 0, 0 },
    });
    std::cout << "[PASS] Sneaky Gremlin: 10 to 14 HP, Spawned (stunned), then Tackle 9" << std::endl;
}

void test_sneaky_gremlin_ascension() {
    // 11 to 15 HP at Ascension, on the same draw; Tackle 10 at Ascension 9.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SNEAKY_GREMLIN);
        Enemy& gremlin = combat.enemies[0];
        gremlin.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = gremlin.hp;
        assert(base >= 10 && base <= 14 && gremlin.ascension_hp() == 11);
        gremlin.ascend(true, true);
        combat.plan_enemies();
        assert(gremlin.hp == base + 1 && gremlin.max_hp == base + 1 && gremlin.hp >= 11 && gremlin.hp <= 15);
        assert(gremlin.current_intent.type == STUN);
        pass_turn(combat);
        assert(gremlin.current_intent.type == ATTACK && gremlin.intent_damage(combat.player) == 10);
        pass_turn(combat);
        assert(gremlin.current_intent.type == ATTACK && gremlin.intent_damage(combat.player) == 10);
    }
    std::cout << "[PASS] Sneaky Gremlin: Ascension gives 11 to 15 HP and Tackle 10" << std::endl;
}

// Puts `card` alone in hand with full energy and plays it on the first enemy.
bool play_alone(CombatEngine& combat, DeckCard card) {
    combat.hand.clear();
    combat.hand.push_back(card);
    combat.player.energy = combat.player.max_energy;
    return combat.play_card(0, 0);
}

const ActivePower* held_power(const CombatEngine& combat, int holder, PowerId id) {
    for (const ActivePower& power : combat.powers) {
        if (power.id == id && power.holder == holder) return &power;
    }
    return nullptr;
}

void test_enemy_power_holder() {
    // A power held by an enemy: its own amount, apart from the player's powers and from the other enemies'.
    CombatEngine combat = make_pack({ EnemyType::SEWER_CLAM, EnemyType::SEWER_CLAM });
    combat.add_enemy_power(0, PowerId::PERSONAL_HIVE, 1);
    combat.add_enemy_power(0, PowerId::PERSONAL_HIVE, 2);
    combat.add_enemy_power(1, PowerId::PERSONAL_HIVE, 5);
    assert(combat.enemy_power_amount(0, PowerId::PERSONAL_HIVE) == 3);
    assert(combat.enemy_power_amount(1, PowerId::PERSONAL_HIVE) == 5);
    assert(combat.enemy_power_amount(0, PowerId::TANGLED) == 0 && combat.powers.size() == 2);
    assert(held_power(combat, 0, PowerId::PERSONAL_HIVE)->stacks == 2 && !held_power(combat, -1, PowerId::PERSONAL_HIVE));
    // Nothing for an enemy that does not exist, nor for a dead one.
    combat.add_enemy_power(7, PowerId::PERSONAL_HIVE, 1);
    combat.add_enemy_power(-1, PowerId::PERSONAL_HIVE, 1);
    assert(combat.powers.size() == 2);

    // The power leaves with its holder, killed or gone; the other enemy keeps its own.
    combat.enemies[0].hp = 1;
    assert(play_alone(combat, CardId::STRIKE));
    assert(!combat.enemies[0].is_alive());
    assert(combat.enemy_power_amount(0, PowerId::PERSONAL_HIVE) == 0 && combat.enemy_power_amount(1, PowerId::PERSONAL_HIVE) == 5);
    combat.add_enemy_power(0, PowerId::PERSONAL_HIVE, 1);
    assert(combat.powers.size() == 1);
    combat.enemies[1].current_intent = EnemyIntent{};
    combat.enemies[1].current_intent.type = ESCAPE;
    combat.end_player_turn();
    assert(combat.enemies[1].escaped && combat.powers.empty());

    // A move can give its user a power: it holds it once the move is played.
    CombatEngine giver = make_combat(CharacterId::IRONCLAD, EnemyType::SEWER_CLAM);
    giver.enemies[0].current_intent = EnemyIntent{};
    giver.enemies[0].current_intent.type = BUFF;
    giver.enemies[0].current_intent.power = PowerId::PERSONAL_HIVE;
    giver.enemies[0].current_intent.power_amount = 2;
    assert(giver.enemy_power_amount(0, PowerId::PERSONAL_HIVE) == 0);
    giver.end_player_turn();
    assert(giver.enemy_power_amount(0, PowerId::PERSONAL_HIVE) == 2 && giver.last_enemy_actions[0].power == PowerId::NONE);

    // A new fight starts without the powers of the last one.
    giver.start_combat(EnemyType::SEWER_CLAM, CharacterId::IRONCLAD);
    giver.reset_combat_state();
    assert(giver.powers.empty());
    std::cout << "[PASS] Enemy powers: held by one enemy, given by a move, gone with their holder" << std::endl;
}

void test_enemy_power_turn_hook() {
    // Start of the holder's turn: the power counts the turns its holder started, and only those.
    CombatEngine combat = make_pack({ EnemyType::SEWER_CLAM, EnemyType::SEWER_CLAM });
    combat.add_enemy_power(1, PowerId::PERSONAL_HIVE, 1);
    combat.add_power(PowerId::RAGE, 3); // a power of the player, next to it
    assert(held_power(combat, 1, PowerId::PERSONAL_HIVE)->turns == 0);
    for (int turn = 1; turn <= 3; ++turn) {
        pass_turn(combat);
        assert(held_power(combat, 1, PowerId::PERSONAL_HIVE)->turns == turn);
        // The player's power counts the player's turns, as before.
        assert(held_power(combat, -1, PowerId::RAGE)->turns == turn);
    }
    // A stunned holder still starts its turn; a dead one starts none.
    combat.enemies[1].stun();
    pass_turn(combat);
    assert(held_power(combat, 1, PowerId::PERSONAL_HIVE)->turns == 4);
    std::cout << "[PASS] Enemy powers: the start of the holder's turn is heard, and counted" << std::endl;
}

void test_enemy_power_hit_hook() {
    // Hit received (Personal Hive): unblocked attack damage on the holder puts Dazed in the player's draw pile.
    CombatEngine combat = make_pack({ EnemyType::SEWER_CLAM, EnemyType::SEWER_CLAM });
    combat.add_enemy_power(0, PowerId::PERSONAL_HIVE, 2);
    auto dazed = [&combat]() {
        int count = 0;
        for (const DeckCard& card : combat.draw_pile) count += card.id == CardId::DAZED;
        return count;
    };
    assert(dazed() == 0);
    assert(play_alone(combat, CardId::STRIKE));
    assert(dazed() == 2);
    // The other enemy holds nothing: hitting it adds nothing.
    combat.hand.clear();
    combat.hand.push_back(CardId::STRIKE);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, 1);
    assert(played && dazed() == 2);
    // A hit its Block absorbs entirely is not damage taken.
    combat.enemies[0].block = 50;
    assert(play_alone(combat, CardId::STRIKE));
    assert(dazed() == 2);
    // The blow that kills the holder is not answered: its powers are silent once it is out.
    combat.enemies[0].block = 0;
    combat.enemies[0].hp = 1;
    assert(play_alone(combat, CardId::STRIKE));
    assert(dazed() == 2 && !combat.enemies[0].is_alive());
    assert(combat.enemy_power_amount(0, PowerId::PERSONAL_HIVE) == 0);
    std::cout << "[PASS] Enemy powers: a hit received by the holder is heard (Personal Hive)" << std::endl;
}

void test_enemy_power_card_hook() {
    // Card played by the player (Tender): each card costs 1 Strength and 1 Dexterity until the end of the turn.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SEWER_CLAM);
    assert(combat.afflict_player(PowerId::TENDER, 1, false));
    assert(play_alone(combat, CardId::DEFEND));
    assert(combat.player.get_status(StatusType::STRENGTH) == -1 && combat.player.get_status(StatusType::DEXTERITY) == -1);
    int16_t hp = combat.enemies[0].hp;
    assert(play_alone(combat, CardId::STRIKE)); // the second card: 6 - 2 Strength
    assert(hp - combat.enemies[0].hp == 4);
    assert(combat.player.get_status(StatusType::STRENGTH) == -2);
    // The loss ends with the turn; the power stays.
    pass_turn(combat);
    assert(combat.player.get_status(StatusType::STRENGTH) == 0 && combat.player.get_status(StatusType::DEXTERITY) == 0);
    assert(combat.has_power(PowerId::TENDER));

    // Artifact cancels a power an enemy puts on the player, and is used up.
    CombatEngine guarded = make_combat(CharacterId::IRONCLAD, EnemyType::SEWER_CLAM);
    guarded.player.add_status(StatusType::ARTIFACT, 1);
    assert(!guarded.afflict_player(PowerId::TENDER, 1, false));
    assert(!guarded.has_power(PowerId::TENDER) && guarded.player.get_status(StatusType::ARTIFACT) == 0);
    assert(guarded.afflict_player(PowerId::TENDER, 1, false));
    assert(!guarded.afflict_player(PowerId::NONE, 1, false) && !guarded.afflict_player(PowerId::TANGLED, 0, true));
    std::cout << "[PASS] Enemy powers: a card played by the player is heard (Tender); Artifact cancels them" << std::endl;
}

void test_multi_hit_attack() {
    // Each hit goes through Block on its own and is answered by Thorns on its own.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SEWER_CLAM);
    Enemy& clam = combat.enemies[0];
    clam.current_intent = EnemyIntent{};
    clam.current_intent.type = ATTACK;
    clam.current_intent.base_damage = 6;
    clam.current_intent.hits = 3;
    assert(clam.intent_damage(combat.player) == 6);
    combat.player.block = 8;
    combat.player.add_status(StatusType::THORNS, 2);
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 18 - 8);
    assert(clam.hp == 56 - 3 * 2);
    assert(combat.last_enemy_actions[0].hits == 3 && combat.last_enemy_actions[0].damage_to_player == 10);

    // Strength is added to every hit, and the hits stop when the attacker dies of what comes back.
    CombatEngine strong = make_combat(CharacterId::IRONCLAD, EnemyType::SEWER_CLAM);
    Enemy& foe = strong.enemies[0];
    foe.add_status(StatusType::STRENGTH, 2);
    foe.current_intent = EnemyIntent{};
    foe.current_intent.type = ATTACK;
    foe.current_intent.base_damage = 5;
    foe.current_intent.hits = 4;
    foe.hp = 5;
    strong.player.add_status(StatusType::THORNS, 3);
    strong.end_player_turn();
    assert(!foe.is_alive() && strong.player.max_hp - strong.player.hp == 2 * 7);
    std::cout << "[PASS] Multi-hit attacks: each hit on its own against Block and Thorns" << std::endl;
}

void test_vine_shambler() {
    // Swipe 6x2, Grasping Vines 8 and Tangled, Chomp 16, and again.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::VINE_SHAMBLER);
    Enemy& shambler = combat.enemies[0];
    assert(shambler.name == "Bourbelianes" && shambler.hp == 61 && shambler.max_hp == 61);
    assert(shambler.block == 0 && shambler.statuses.empty());
    struct Turn {
        EnemyIntentType intent;
        int damage; // of one hit
        int hits;
        bool tangled; // the player is Tangled on the turn that follows
    };
    int turn_number = 1;
    for (const Turn& turn : { Turn{ ATTACK, 6, 2, false }, Turn{ ATTACK_DEBUFF, 8, 1, true }, Turn{ ATTACK, 16, 1, false },
                              Turn{ ATTACK, 6, 2, false }, Turn{ ATTACK_DEBUFF, 8, 1, true }, Turn{ ATTACK, 16, 1, false } }) {
        assert(combat.turn_number == turn_number++);
        assert(shambler.current_intent.type == turn.intent && shambler.current_intent.hits == turn.hits);
        assert(shambler.intent_damage(combat.player) == turn.damage);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == turn.damage * turn.hits);
        assert(combat.has_power(PowerId::TANGLED) == turn.tangled);
        assert(combat.last_enemy_actions[0].power == (turn.tangled ? PowerId::TANGLED : PowerId::NONE));
        assert(shambler.block == 0 && shambler.statuses.empty() && shambler.hp == 61);
    }

    // Tangled: Attacks cost 1 more energy this turn, the other cards do not; it is gone the turn after.
    CombatEngine tangled = make_combat(CharacterId::IRONCLAD, EnemyType::VINE_SHAMBLER);
    pass_turn(tangled);
    assert(tangled.effective_cost(DeckCard(CardId::STRIKE)) == 1 && tangled.effective_cost(DeckCard(CardId::BASH)) == 2);
    pass_turn(tangled);
    assert(tangled.has_power(PowerId::TANGLED) && tangled.power_amount(PowerId::TANGLED) == 1);
    assert(tangled.effective_cost(DeckCard(CardId::STRIKE)) == 2 && tangled.effective_cost(DeckCard(CardId::BASH)) == 3);
    assert(tangled.effective_cost(DeckCard(CardId::DEFEND)) == 1);
    // Three energy: one Strike at 2, and not enough left for a second.
    tangled.hand.clear();
    tangled.hand.push_back(CardId::STRIKE);
    tangled.hand.push_back(CardId::STRIKE);
    tangled.player.energy = 3;
    bool first = tangled.play_card(0, 0);
    assert(first && tangled.player.energy == 1 && !tangled.can_play_card(0));
    pass_turn(tangled);
    assert(!tangled.has_power(PowerId::TANGLED) && tangled.effective_cost(DeckCard(CardId::STRIKE)) == 1);

    // Artifact keeps the vines off.
    CombatEngine guarded = make_combat(CharacterId::IRONCLAD, EnemyType::VINE_SHAMBLER);
    pass_turn(guarded);
    guarded.player.add_status(StatusType::ARTIFACT, 1);
    pass_turn(guarded);
    assert(!guarded.has_power(PowerId::TANGLED) && guarded.player.get_status(StatusType::ARTIFACT) == 0);
    assert(guarded.last_enemy_actions[0].power == PowerId::NONE && guarded.last_enemy_actions[0].debuff_attempted);
    std::cout << "[PASS] Vine Shambler: 61 HP, Swipe 6x2, Grasping Vines 8 and Tangled, Chomp 16, loop" << std::endl;
}

void test_vine_shambler_ascension() {
    struct Level {
        bool more_hp;
        bool more_damage;
        int hp;
        int swipe;
        int vines;
        int chomp;
    };
    for (const Level& level : { Level{ false, false, 61, 6, 8, 16 }, Level{ true, false, 64, 6, 8, 16 },
                                Level{ false, true, 61, 7, 9, 18 }, Level{ true, true, 64, 7, 9, 18 } }) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::VINE_SHAMBLER);
        Enemy& foe = combat.enemies[0];
        foe.ascend(level.more_hp, level.more_damage);
        combat.plan_enemies();
        assert(foe.hp == level.hp && foe.max_hp == level.hp && foe.damage_percent == 100);
        assert(foe.intent_damage(combat.player) == level.swipe && foe.current_intent.hits == 2);
        pass_turn(combat);
        assert(foe.intent_damage(combat.player) == level.vines);
        pass_turn(combat);
        assert(foe.intent_damage(combat.player) == level.chomp);
    }
    std::cout << "[PASS] Vine Shambler: Ascension gives 64 HP, Swipe 7x2, Grasping Vines 9, Chomp 18" << std::endl;
}

int count_in(const FixedVector<DeckCard, 50>& pile, CardId id) {
    int count = 0;
    for (const DeckCard& card : pile) count += card.id == id;
    return count;
}

// A Clam whose next move puts `count` copies of `card` in `pile`.
CombatEngine make_card_giver(CardId card, uint8_t count, Pile pile, EnemyIntentType type = EnemyIntentType::STATUS,
                             int16_t damage = 0) {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SEWER_CLAM);
    Enemy& enemy = combat.enemies[0];
    enemy.current_intent = EnemyIntent{};
    enemy.current_intent.type = type;
    enemy.current_intent.base_damage = damage;
    enemy.current_intent.status_card = card;
    enemy.current_intent.status_cards = count;
    enemy.current_intent.status_pile = pile;
    return combat;
}

void test_status_intent() {
    // N cards into the pile the move names. The move is over before the player's turn starts, so the cards of
    // the discard pile wait there, and the ones of the draw pile can be drawn at once.
    CombatEngine discard = make_card_giver(CardId::SLIMED, 2, Pile::DISCARD);
    assert(discard.enemies[0].current_intent.gives_cards() && discard.enemies[0].intent_damage(discard.player) == 0);
    size_t cards = discard.draw_pile.size() + discard.hand.size() + discard.discard_pile.size();
    discard.end_player_turn();
    assert(discard.state == CombatState::PLAYER_TURN && discard.player.hp == discard.player.max_hp);
    assert(count_in(discard.discard_pile, CardId::SLIMED) == 2);
    assert(discard.draw_pile.size() + discard.hand.size() + discard.discard_pile.size() == cards + 2);
    const EnemyActionRecord& record = discard.last_enemy_actions[0];
    assert(record.intent_type == STATUS && record.cards_added == CardId::SLIMED && record.cards_added_count == 2);

    CombatEngine draw = make_card_giver(CardId::BURN, 4, Pile::DRAW);
    cards = draw.draw_pile.size() + draw.hand.size() + draw.discard_pile.size();
    draw.end_player_turn();
    int in_hand = 0;
    for (const DeckCard& card : draw.hand) in_hand += card.id == CardId::BURN;
    assert(count_in(draw.draw_pile, CardId::BURN) + in_hand == 4 && count_in(draw.discard_pile, CardId::BURN) == 0);
    assert(draw.draw_pile.size() + draw.hand.size() + draw.discard_pile.size() == cards + 4);

    // Into the hand: there for the turn that starts.
    CombatEngine hand = make_card_giver(CardId::WOUND, 1, Pile::HAND);
    hand.end_player_turn();
    assert(hand.last_enemy_actions[0].cards_added_count == 1);

    // A move without cards announces none and adds none.
    CombatEngine plain = make_combat(CharacterId::IRONCLAD, EnemyType::SEWER_CLAM);
    assert(!plain.enemies[0].current_intent.gives_cards());
    plain.end_player_turn();
    assert(plain.last_enemy_actions[0].cards_added_count == 0 && plain.last_enemy_actions[0].cards_added == CardId::NONE);

    // With an attack (Dismember: 26 damage and 3 Wounds): the hit, then the cards.
    CombatEngine struck = make_card_giver(CardId::WOUND, 3, Pile::DISCARD, ATTACK, 26);
    struck.end_player_turn();
    assert(struck.player.max_hp - struck.player.hp == 26 && count_in(struck.discard_pile, CardId::WOUND) == 3);

    // A pile that is full takes no more, and the record counts what went in.
    CombatEngine full = make_card_giver(CardId::DAZED, 5, Pile::DISCARD);
    full.hand.clear(); // nothing of the hand joins the discard pile at the end of the turn
    while (full.discard_pile.size() < full.discard_pile.capacity() - 2) full.discard_pile.push_back(CardId::STRIKE);
    full.draw_pile.clear();
    for (int i = 0; i < 10; ++i) full.draw_pile.push_back(CardId::DEFEND);
    full.end_player_turn();
    assert(full.last_enemy_actions[0].cards_added_count == 2);
    std::cout << "[PASS] Status intent: N cards into the discard pile, the draw pile or the hand, alone or after an attack"
              << std::endl;
}

void test_status_cards_are_not_the_players() {
    // Cards an enemy adds are not cards the player created: Arsenal (a card created: Strength) stays silent, and
    // the tally of cards created does not move. The same card created by the player counts.
    CombatEngine combat = make_card_giver(CardId::DAZED, 3, Pile::DISCARD);
    combat.add_power(PowerId::ARSENAL, 1);
    combat.end_player_turn();
    assert(count_in(combat.discard_pile, CardId::DAZED) == 3);
    assert(combat.player.get_status(StatusType::STRENGTH) == 0 && combat.counters.combat_cards_created == 0);
    combat.create_card(CardId::DAZED, Pile::DISCARD);
    assert(combat.player.get_status(StatusType::STRENGTH) == 1 && combat.counters.combat_cards_created == 1);

    // Once in the piles they are cards like the others: a Dazed drawn is Unplayable and leaves at the end of the turn.
    CombatEngine drawn = make_card_giver(CardId::DAZED, 1, Pile::HAND);
    drawn.end_player_turn();
    int index = -1;
    for (size_t i = 0; i < drawn.hand.size(); ++i) {
        if (drawn.hand[i].id == CardId::DAZED) index = static_cast<int>(i);
    }
    assert(index >= 0 && !drawn.can_play_card(index));
    drawn.enemies[0].current_intent = EnemyIntent{};
    drawn.enemies[0].current_intent.type = DEFEND;
    drawn.end_player_turn();
    assert(count_in(drawn.exhaust_pile, CardId::DAZED) == 1); // Ethereal
    std::cout << "[PASS] Status intent: the cards are not counted as created by the player" << std::endl;
}

void test_eye_with_teeth() {
    // Distract every turn: 3 Dazed into the discard pile.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::EYE_WITH_TEETH);
    Enemy& eye = combat.enemies[0];
    assert(eye.type == EnemyType::EYE_WITH_TEETH && eye.name == "Œil denté" && eye.hp == 6 && eye.max_hp == 6);
    assert(eye.block == 0 && eye.statuses.empty());
    // The discard pile is counted with the Dazed already drawn back and exhausted, so count every pile.
    auto dazed_everywhere = [&combat]() {
        int count = count_in(combat.draw_pile, CardId::DAZED) + count_in(combat.discard_pile, CardId::DAZED) +
                    count_in(combat.exhaust_pile, CardId::DAZED);
        for (const DeckCard& card : combat.hand) count += card.id == CardId::DAZED;
        return count;
    };
    for (int turn = 1; turn <= 4; ++turn) {
        assert(combat.turn_number == turn);
        assert(eye.current_intent.type == STATUS && eye.intent_damage(combat.player) == 0);
        assert(eye.current_intent.status_card == CardId::DAZED && eye.current_intent.status_cards == 3 &&
               eye.current_intent.status_pile == Pile::DISCARD);
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN && combat.player.hp == combat.player.max_hp);
        assert(dazed_everywhere() == 3 * turn);
        assert(combat.last_enemy_actions[0].cards_added == CardId::DAZED && combat.last_enemy_actions[0].cards_added_count == 3);
        assert(eye.block == 0 && eye.statuses.empty() && eye.hp == 6);
    }

    // The reference gives it no Ascension value: it follows the percentages of the run, like the first monsters.
    assert(eye.ascension_hp() == 0 && eye.hp_spread() == 0);
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 9, PaletteVariant::CLASSIC, 777);
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    Enemy scaled;
    scaled.init(EnemyType::EYE_WITH_TEETH);
    assert(scaled.max_hp * run.get_enemy_hp_multiplier() / 100 == 6); // 6 HP + 15 %, rounded down
    std::cout << "[PASS] Eye With Teeth: 6 HP, Distract (3 Dazed into the discard pile) every turn" << std::endl;
}

void test_stabbot() {
    // Stab every turn: 11 damage and 1 Frail on the player, Frail wearing off at the end of the round.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::STABBOT);
    Enemy& bot = combat.enemies[0];
    assert(bot.type == EnemyType::STABBOT && bot.name == "Poignard-bot");
    assert(bot.hp >= 18 && bot.hp <= 23 && bot.max_hp == bot.hp && bot.max_hp == 18 + bot.hp_bonus);
    const int16_t hp = bot.hp;
    assert(bot.block == 0 && bot.statuses.empty());
    for (int turn = 1; turn <= 4; ++turn) {
        assert(combat.turn_number == turn);
        assert(bot.current_intent.type == ATTACK_DEBUFF && bot.intent_damage(combat.player) == 11);
        assert(bot.current_intent.debuffs[0].type == StatusType::FRAIL && bot.current_intent.debuffs[0].amount == 1);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == 11);
        assert(combat.player.get_status(StatusType::FRAIL) >= 1); // renewed each turn, stacking on the one still running
        assert(combat.last_enemy_actions[0].debuffs[0].type == StatusType::FRAIL);
        assert(bot.block == 0 && bot.statuses.empty() && bot.hp == hp);
    }
    std::cout << "[PASS] Stabbot: 18 to 23 HP, Stab 11 damage + 1 Frail every turn" << std::endl;
}

void test_stabbot_ascension() {
    // 19 to 24 HP (18 to 23 plus 1) and a Stab of 12 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::STABBOT);
        Enemy& bot = combat.enemies[0];
        bot.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = bot.hp;
        assert(bot.ascension_hp() == 19 && bot.hp_spread() == 5);
        assert(base >= 18 && base <= 23 && bot.max_hp == 18 + bot.hp_bonus);
        bot.ascend(true, true);
        combat.plan_enemies();
        assert(bot.hp == base + 1 && bot.max_hp == base + 1 && bot.hp >= 19 && bot.hp <= 24);
        assert(bot.current_intent.type == ATTACK_DEBUFF && bot.intent_damage(combat.player) == 12);
        assert(bot.current_intent.debuffs[0].type == StatusType::FRAIL);
    }
    std::cout << "[PASS] Stabbot: Ascension gives 19 to 24 HP and a Stab of 12" << std::endl;
}

void test_noisebot() {
    // Noise every turn: 2 Dazed into the discard pile, no damage.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::NOISEBOT);
    Enemy& bot = combat.enemies[0];
    assert(bot.type == EnemyType::NOISEBOT && bot.name == "Sonobot");
    assert(bot.hp >= 18 && bot.hp <= 23 && bot.max_hp == bot.hp && bot.max_hp == 18 + bot.hp_bonus);
    assert(bot.block == 0 && bot.statuses.empty());
    for (int turn = 1; turn <= 4; ++turn) {
        assert(combat.turn_number == turn);
        assert(bot.current_intent.type == STATUS && bot.current_intent.status_card == CardId::DAZED);
        assert(bot.current_intent.status_cards == 2 && bot.current_intent.status_pile == Pile::DISCARD);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.hp == combat.player.max_hp && bot.block == 0 && bot.statuses.empty());
    }
    std::cout << "[PASS] Noisebot: 18 to 23 HP, Noise (2 Dazed into the discard pile) every turn" << std::endl;
}

void test_noisebot_ascension() {
    // 19 to 24 HP (18 to 23 plus 1) at Ascension, same Noise.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::NOISEBOT);
        Enemy& bot = combat.enemies[0];
        bot.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = bot.hp;
        assert(bot.ascension_hp() == 19 && bot.hp_spread() == 5);
        assert(base >= 18 && base <= 23 && bot.max_hp == 18 + bot.hp_bonus);
        bot.ascend(true, true);
        combat.plan_enemies();
        assert(bot.hp == base + 1 && bot.max_hp == base + 1 && bot.hp >= 19 && bot.hp <= 24);
        assert(bot.current_intent.type == STATUS && bot.current_intent.status_cards == 2);
    }
    std::cout << "[PASS] Noisebot: Ascension gives 19 to 24 HP" << std::endl;
}

void test_cubex_construct() {
    // Charge Up (+2 Strength), two Repeater Blasts 7 (+2 Strength each), Expel 5x2, then the blasts and Expel again.
    // The Strength of a blast is gained after it hits.
    check_monster(EnemyType::CUBEX_CONSTRUCT, "Assemblage Cubex", 65, {
        { BUFF, 0, 0, 2 },
        { ATTACK_BUFF, 9, 0, 4 },
        { ATTACK_BUFF, 11, 0, 6 },
        { ATTACK, 11, 0, 6, 0, 2 },
        { ATTACK_BUFF, 13, 0, 8 },
        { ATTACK_BUFF, 15, 0, 10 },
        { ATTACK, 15, 0, 10, 0, 2 },
    });
    std::cout << "[PASS] Cubex Construct: 65 HP, Charge Up, 2 Repeater Blasts 7, Expel 5x2, loop" << std::endl;
}

void test_cubex_construct_ascension() {
    // 70 HP, Repeater Blast 8 and Expel 6x2 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::CUBEX_CONSTRUCT);
    Enemy& cube = combat.enemies[0];
    cube.ascend(true, true);
    combat.plan_enemies();
    assert(cube.hp == 70 && cube.max_hp == 70);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // Charge Up: +2 Strength
    assert(cube.current_intent.type == ATTACK_BUFF && cube.intent_damage(combat.player) == 10); // 8 + 2
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(cube.current_intent.type == ATTACK && cube.current_intent.hits == 2);
    assert(cube.intent_damage(combat.player) == 6 + 6); // 6 + 6 Strength
    std::cout << "[PASS] Cubex Construct: Ascension gives 70 HP, Repeater Blast 8, Expel 6x2" << std::endl;
}

void test_punch_construct() {
    // Ready 10 Block, Fast Punch 5x2 (1 Frail), Strong Punch 14, and again. The Block fades at its next turn.
    check_monster(EnemyType::PUNCH_CONSTRUCT, "Assemblage de pugilat", 55, {
        { DEFEND, 0, 10, 0 },
        { ATTACK_DEBUFF, 5, 0, 0, 0, 2 },
        { ATTACK, 14, 0, 0 },
        { DEFEND, 0, 10, 0 },
        { ATTACK_DEBUFF, 5, 0, 0, 0, 2 },
        { ATTACK, 14, 0, 0 },
    });
    std::cout << "[PASS] Punch Construct: 55 HP, Ready 10 Block, Fast Punch 5x2, Strong Punch 14, loop" << std::endl;
}

void test_punch_construct_ascension() {
    // 60 HP, Fast Punch 6x2 and Strong Punch 16 at Ascension; the Fast Punch gives 1 Frail.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::PUNCH_CONSTRUCT);
    Enemy& golem = combat.enemies[0];
    golem.ascend(true, true);
    combat.plan_enemies();
    assert(golem.hp == 60 && golem.max_hp == 60);
    combat.end_player_turn(); // Ready
    combat.player.hp = combat.player.max_hp;
    assert(golem.current_intent.type == ATTACK_DEBUFF && golem.current_intent.hits == 2);
    assert(golem.intent_damage(combat.player) == 6);
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 12);
    assert(combat.player.get_status(StatusType::FRAIL) == 1);
    combat.player.hp = combat.player.max_hp;
    assert(golem.current_intent.type == ATTACK && golem.intent_damage(combat.player) == 16);
    std::cout << "[PASS] Punch Construct: Ascension gives 60 HP, Fast Punch 6x2, Strong Punch 16" << std::endl;
}

void test_infested_prism() {
    // Jab 15, Radiate 11 + 11 Block, Whirlwind 5x3, Pulsate 8 + 20 Block + Vital Spark, and again.
    check_monster(EnemyType::INFESTED_PRISM, "Prisme infesté", 161, {
        { ATTACK, 15, 0, 0 },
        { ATTACK_DEFEND, 11, 11, 0 },
        { ATTACK, 5, 0, 0, 0, 3 },
        { ATTACK_DEFEND, 8, 20, 0 },
        { ATTACK, 15, 0, 0 },
        { ATTACK_DEFEND, 11, 11, 0 },
        { ATTACK, 5, 0, 0, 0, 3 },
        { ATTACK_DEFEND, 8, 20, 0 },
    });
    std::cout << "[PASS] Infested Prism: 161 HP, Jab 15, Radiate 11/11, Whirlwind 5x3, Pulsate 8/20, loop" << std::endl;
}

void test_infested_prism_ascension() {
    // 171 HP at Ascension; Jab 17, Radiate 13/13, Whirlwind 6x3, Pulsate 10/22 with 3 Vital Spark.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::INFESTED_PRISM);
    Enemy& prism = combat.enemies[0];
    prism.ascend(true, true);
    combat.plan_enemies();
    assert(prism.hp == 171 && prism.max_hp == 171);
    const int damage[] = { 17, 13, 6, 10 };
    const int block[] = { 0, 13, 0, 22 };
    for (int i = 0; i < 4; ++i) {
        combat.player.hp = combat.player.max_hp;
        assert(prism.intent_damage(combat.player) == damage[i]);
        combat.end_player_turn();
        assert(prism.block == block[i]);
        assert(combat.player.max_hp - combat.player.hp == damage[i] * (i == 2 ? 3 : 1));
    }
    assert(combat.enemy_power_amount(0, PowerId::VITAL_SPARK) == 3);
    std::cout << "[PASS] Infested Prism: Ascension gives 171 HP, Jab 17, Radiate 13/13, Whirlwind 6x3, Pulsate 10/22" << std::endl;
}

void test_infested_prism_vital_spark() {
    // Pulsate gives 2 Vital Spark: each unblocked hit gives 1 energy, twice a turn, and the count starts over.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::INFESTED_PRISM);
    for (int i = 0; i < 4; ++i) combat.end_player_turn(); // up to Pulsate
    assert(combat.enemy_power_amount(0, PowerId::VITAL_SPARK) == 2);
    Enemy& prism = combat.enemies[0];
    prism.block = 0;
    combat.hand.clear();
    for (int i = 0; i < 3; ++i) combat.hand.push_back(CardId::STRIKE);
    combat.player.energy = 3;
    for (int i = 0; i < 3; ++i) assert(combat.play_card(0, 0));
    assert(combat.player.energy == 3 - 3 + 2); // the third hit gives nothing
    combat.end_player_turn();
    prism.block = 0;
    combat.hand.clear();
    combat.hand.push_back(CardId::STRIKE);
    assert(combat.play_card(0, 0));
    assert(combat.player.energy == combat.player.max_energy); // the first hit of a new turn gives 1 again
    std::cout << "[PASS] Infested Prism: Vital Spark gives 1 energy per unblocked hit, up to its amount each turn" << std::endl;
}

void test_inklet() {
    // Jab 3 on the odd turns; on the even ones Whirlwind (2 x3) or Piercing Gaze (10), drawn at random.
    bool whirlwind = false, gaze = false;
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::INKLET);
        Enemy& inklet = combat.enemies[0];
        inklet.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        assert(inklet.type == EnemyType::INKLET && inklet.name == "Encrelin");
        assert(inklet.hp >= 11 && inklet.hp <= 17 && inklet.max_hp == 11 + inklet.hp_bonus && inklet.hp_spread() == 6);
        const int16_t hp = inklet.hp;
        for (int turn = 1; turn <= 8; ++turn) {
            assert(combat.turn_number == turn);
            assert(inklet.current_intent.type == ATTACK);
            int hits = inklet.current_intent.hits;
            int damage = inklet.intent_damage(combat.player) * hits;
            if (turn % 2 == 1) {
                assert(hits == 1 && damage == 3);
            } else if (hits == 3) {
                assert(damage == 6);
                whirlwind = true;
            } else {
                assert(hits == 1 && damage == 10);
                gaze = true;
            }
            combat.player.hp = combat.player.max_hp;
            combat.end_player_turn();
            assert(combat.state == CombatState::PLAYER_TURN);
            assert(combat.player.max_hp - combat.player.hp == damage);
            assert(inklet.block == 0 && inklet.statuses.empty() && inklet.hp == hp);
        }
    }
    assert(whirlwind && gaze);
    std::cout << "[PASS] Inklet: 11 to 17 HP, Jab 3, then Whirlwind 2x3 or Piercing Gaze 10 at random" << std::endl;
}

void test_inklet_ascension() {
    // 12 to 18 HP (11 to 17 plus 1), Jab 4, Whirlwind 3x3 and Piercing Gaze 11 at Ascension.
    bool whirlwind = false, gaze = false;
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::INKLET);
        Enemy& inklet = combat.enemies[0];
        inklet.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = inklet.hp;
        assert(inklet.ascension_hp() == 12 && base >= 11 && base <= 17);
        inklet.ascend(true, true);
        combat.plan_enemies();
        assert(inklet.hp == base + 1 && inklet.max_hp == base + 1 && inklet.hp >= 12 && inklet.hp <= 18);
        for (int turn = 1; turn <= 8; ++turn) {
            if (turn % 2 == 1) {
                assert(inklet.intent_damage(combat.player) == 4 && inklet.current_intent.hits == 1);
            } else if (inklet.current_intent.hits == 3) {
                assert(inklet.intent_damage(combat.player) == 3);
                whirlwind = true;
            } else {
                assert(inklet.current_intent.hits == 1 && inklet.intent_damage(combat.player) == 11);
                gaze = true;
            }
            combat.player.hp = combat.player.max_hp;
            combat.end_player_turn();
        }
    }
    assert(whirlwind && gaze);
    std::cout << "[PASS] Inklet: Ascension gives 12 to 18 HP, Jab 4, Whirlwind 3x3, Piercing Gaze 11" << std::endl;
}

// Plays `turns` of the enemy at `index`: its intent, the damage the player loses, its Block and its Strength.
void check_nibbit_cycle(CombatEngine& combat, size_t index, std::initializer_list<Turn> turns, int strength_step) {
    Enemy& nibbit = combat.enemies[index];
    int turn_number = 1;
    int strength = 0;
    for (const Turn& turn : turns) {
        assert(combat.turn_number == turn_number);
        assert(nibbit.current_intent.type == turn.intent);
        assert(nibbit.intent_damage(combat.player) == turn.damage);
        if (turn.intent == BUFF) strength += strength_step;
        pass_turn(combat);
        assert(nibbit.block == turn.block);
        assert(nibbit.get_status(StatusType::STRENGTH) == strength);
        ++turn_number;
    }
}

void test_nibbit() {
    // Alone: Butt 12, Slice 6 + 5 Block, Hiss (+2 Strength), and again.
    CombatEngine alone = make_pack({ EnemyType::NIBBIT });
    Enemy& single = alone.enemies[0];
    assert(single.type == EnemyType::NIBBIT && single.name == "Grignotin");
    assert(single.hp >= 42 && single.hp <= 46 && single.max_hp == single.hp && single.max_hp == 42 + single.hp_bonus);
    assert(single.block == 0 && single.statuses.empty());
    check_nibbit_cycle(alone, 0, {
        { ATTACK, 12, 0, 0 },
        { ATTACK_DEFEND, 6, 5, 0 },
        { BUFF, 0, 0, 0 },
        { ATTACK, 14, 0, 0 },
        { ATTACK_DEFEND, 8, 5, 0 },
        { BUFF, 0, 0, 0 },
    }, 2);

    // Two Nibbits: the one in front starts with Slice, the one behind with Hiss. Each follows the same cycle.
    CombatEngine pair = make_pack({ EnemyType::NIBBIT, EnemyType::NIBBIT });
    assert(pair.enemies[0].current_intent.type == ATTACK_DEFEND);
    assert(pair.enemies[1].current_intent.type == BUFF);
    const EnemyIntentType front[] = { ATTACK_DEFEND, BUFF, ATTACK, ATTACK_DEFEND, BUFF, ATTACK };
    const EnemyIntentType behind[] = { BUFF, ATTACK, ATTACK_DEFEND, BUFF, ATTACK, ATTACK_DEFEND };
    for (int turn = 0; turn < 6; ++turn) {
        assert(pair.enemies[0].current_intent.type == front[turn]);
        assert(pair.enemies[1].current_intent.type == behind[turn]);
        pass_turn(pair);
    }
    // The pattern is kept when the partner dies: it does not start over as if alone.
    CombatEngine left = make_pack({ EnemyType::NIBBIT, EnemyType::NIBBIT });
    pass_turn(left);
    left.enemies[0].hp = 0;
    pass_turn(left);
    assert(left.enemies[1].current_intent.type == ATTACK_DEFEND);
    std::cout << "[PASS] Nibbit: 42 to 46 HP, Butt 12, Slice 6 + 5 Block, Hiss +2 Strength, start by position" << std::endl;
}

void test_nibbit_ascension() {
    // 44 to 48 HP, Butt 13, Slice 7 + 6 Block and Hiss +3 Strength at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_pack({ EnemyType::NIBBIT });
        Enemy& nibbit = combat.enemies[0];
        nibbit.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = nibbit.hp;
        assert(nibbit.ascension_hp() == 44 && nibbit.hp_spread() == 4);
        assert(base >= 42 && base <= 46 && nibbit.max_hp == 42 + nibbit.hp_bonus);
        nibbit.ascend(true, true);
        combat.plan_enemies();
        assert(nibbit.hp == base + 2 && nibbit.max_hp == base + 2 && nibbit.hp >= 44 && nibbit.hp <= 48);
        assert(nibbit.current_intent.type == ATTACK && nibbit.intent_damage(combat.player) == 13);
        pass_turn(combat);
        assert(nibbit.current_intent.type == ATTACK_DEFEND && nibbit.intent_damage(combat.player) == 7);
        pass_turn(combat);
        assert(nibbit.block == 6);
        assert(nibbit.current_intent.type == BUFF);
        pass_turn(combat);
        assert(nibbit.get_status(StatusType::STRENGTH) == 3);
    }
    std::cout << "[PASS] Nibbit: Ascension gives 44 to 48 HP, Butt 13, Slice 7 + 6 Block, Hiss +3 Strength" << std::endl;
}

void test_gas_bomb() {
    // Explode every turn: 8 damage.
    check_monster(EnemyType::GAS_BOMB, "Bombe à gaz", 7, {
        { ATTACK, 8, 0, 0 },
        { ATTACK, 8, 0, 0 },
        { ATTACK, 8, 0, 0 },
    });
    std::cout << "[PASS] Gas Bomb: 7 HP, Explode 8 every turn" << std::endl;
}

void test_gas_bomb_ascension() {
    // 8 HP and Explode 9 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::GAS_BOMB);
    Enemy& bomb = combat.enemies[0];
    assert(bomb.ascension_hp() == 8);
    bomb.ascend(true, true);
    combat.plan_enemies();
    assert(bomb.hp == 8 && bomb.max_hp == 8);
    for (int turn = 1; turn <= 3; ++turn) {
        assert(bomb.current_intent.type == ATTACK && bomb.intent_damage(combat.player) == 9);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 9);
    }
    std::cout << "[PASS] Gas Bomb: Ascension gives 8 HP and Explode 9" << std::endl;
}

void test_flyconid() {
    // Calm: Frail Spores (8 damage + 2 Frail) or Smash (11), never the same twice in a row. Hurt, it opens with
    // Vulnerable Spores, then draws among the three moves, again never the same twice in a row.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FLYCONID);
    Enemy& fly = combat.enemies[0];
    assert(fly.type == EnemyType::FLYCONID && fly.name == "Mycoptère");
    assert(fly.hp >= 47 && fly.hp <= 49 && fly.max_hp == fly.hp && fly.max_hp == 47 + fly.hp_bonus);
    bool saw_frail = false, saw_smash = false;
    int previous = -1;
    for (int turn = 1; turn <= 8; ++turn) {
        assert(fly.phase == 0 && fly.block == 0 && fly.statuses.empty());
        const EnemyIntent& intent = fly.current_intent;
        int move;
        if (intent.type == ATTACK_DEBUFF) {
            assert(fly.intent_damage(combat.player) == 8);
            assert(intent.debuffs[0].type == StatusType::FRAIL && intent.debuffs[0].amount == 2);
            move = 1;
            saw_frail = true;
        } else {
            assert(intent.type == ATTACK && fly.intent_damage(combat.player) == 11);
            move = 2;
            saw_smash = true;
        }
        assert(move != previous);
        previous = move;
        combat.player.hp = combat.player.max_hp;
        combat.player.statuses.clear();
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == (move == 1 ? 8 : 11));
    }
    assert(saw_frail && saw_smash);

    // Hurt: Vulnerable Spores at once (2 Vulnerable on the player, no damage), then a draw that cannot repeat.
    CombatEngine hurt = make_combat(CharacterId::IRONCLAD, EnemyType::FLYCONID);
    Enemy& bug = hurt.enemies[0];
    hurt.damage_enemy(bug, 5);
    hurt.player.hp = hurt.player.max_hp;
    hurt.end_player_turn();
    assert(bug.phase == 1);
    assert(bug.current_intent.type == DEBUFF && bug.intent_damage(hurt.player) == 0);
    assert(bug.current_intent.debuffs[0].type == StatusType::VULNERABLE && bug.current_intent.debuffs[0].amount == 2);
    hurt.player.hp = hurt.player.max_hp;
    hurt.end_player_turn();
    assert(hurt.player.max_hp == hurt.player.hp && hurt.player.get_status(StatusType::VULNERABLE) >= 1);
    bool seen[3] = { true, false, false };
    int last = 0;
    for (int turn = 0; turn < 40; ++turn) {
        const EnemyIntent& intent = bug.current_intent;
        int move = intent.type == DEBUFF ? 0 : (intent.type == ATTACK_DEBUFF ? 1 : 2);
        assert(bug.phase == 1 && move != last);
        if (move == 1) assert(intent.base_damage == 8 && intent.debuffs[0].type == StatusType::FRAIL);
        if (move == 2) assert(intent.type == ATTACK && intent.base_damage == 11);
        seen[move] = true;
        last = move;
        hurt.player.hp = hurt.player.max_hp;
        hurt.end_player_turn();
        assert(hurt.state == CombatState::PLAYER_TURN);
    }
    assert(seen[0] && seen[1] && seen[2]);
    std::cout << "[PASS] Flyconid: 47 to 49 HP, Frail Spores / Smash, Vulnerable Spores once hurt" << std::endl;
}

void test_flyconid_ascension() {
    // 51 to 53 HP (47 to 49 plus 4), Frail Spores of 9 and a Smash of 12 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FLYCONID);
        Enemy& fly = combat.enemies[0];
        fly.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = fly.hp;
        assert(fly.ascension_hp() == 51 && fly.hp_spread() == 2);
        assert(base >= 47 && base <= 49 && fly.max_hp == 47 + fly.hp_bonus);
        fly.ascend(true, true);
        combat.plan_enemies();
        assert(fly.hp == base + 4 && fly.max_hp == base + 4 && fly.hp >= 51 && fly.hp <= 53);
        if (fly.current_intent.type == ATTACK_DEBUFF) {
            assert(fly.intent_damage(combat.player) == 9);
        } else {
            assert(fly.current_intent.type == ATTACK && fly.intent_damage(combat.player) == 12);
        }
    }
    std::cout << "[PASS] Flyconid: Ascension gives 51 to 53 HP, Spores of 9 and a Smash of 12" << std::endl;
}

void test_corpse_slug() {
    // Whip Slap (3 x2), Glomp (8), Goop (2 Frail on the player), and again.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::CORPSE_SLUG);
    Enemy& slug = combat.enemies[0];
    assert(slug.type == EnemyType::CORPSE_SLUG && slug.name == "Putrilimace");
    assert(slug.hp >= 25 && slug.hp <= 27 && slug.max_hp == slug.hp && slug.max_hp == 25 + slug.hp_bonus);
    const int16_t hp = slug.hp;
    for (int turn = 1; turn <= 6; ++turn) {
        assert(combat.turn_number == turn);
        assert(slug.block == 0 && slug.statuses.empty() && slug.hp == hp);
        combat.player.hp = combat.player.max_hp;
        combat.player.statuses.clear();
        const EnemyIntent& intent = slug.current_intent;
        if (turn % 3 == 1) {
            assert(intent.type == ATTACK && intent.hits == 2 && slug.intent_damage(combat.player) == 3);
            combat.end_player_turn();
            assert(combat.player.max_hp - combat.player.hp == 6);
            assert(combat.player.statuses.empty());
        } else if (turn % 3 == 2) {
            assert(intent.type == ATTACK && intent.hits == 1 && slug.intent_damage(combat.player) == 8);
            combat.end_player_turn();
            assert(combat.player.max_hp - combat.player.hp == 8);
            assert(combat.player.statuses.empty());
        } else {
            assert(intent.type == DEBUFF && slug.intent_damage(combat.player) == 0);
            assert(intent.debuffs[0].type == StatusType::FRAIL && intent.debuffs[0].amount == 2);
            combat.end_player_turn();
            assert(combat.player.hp == combat.player.max_hp);
            assert(combat.player.get_status(StatusType::FRAIL) >= 1);
        }
        assert(combat.state == CombatState::PLAYER_TURN);
    }
    std::cout << "[PASS] Corpse Slug: 25 to 27 HP, Whip Slap 3x2, Glomp 8, Goop 2 Frail" << std::endl;
}

void test_corpse_slug_ascension() {
    // 27 to 29 HP (25 to 27 plus 2) and a Glomp of 9 at Ascension; the Whip Slap stays at 3 x2.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::CORPSE_SLUG);
        Enemy& slug = combat.enemies[0];
        slug.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = slug.hp;
        assert(slug.ascension_hp() == 27 && slug.hp_spread() == 2);
        assert(base >= 25 && base <= 27 && slug.max_hp == 25 + slug.hp_bonus);
        slug.ascend(true, true);
        combat.plan_enemies();
        assert(slug.hp == base + 2 && slug.max_hp == base + 2 && slug.hp >= 27 && slug.hp <= 29);
        assert(slug.current_intent.hits == 2 && slug.intent_damage(combat.player) == 3);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(slug.current_intent.type == ATTACK && slug.intent_damage(combat.player) == 9);
    }
    std::cout << "[PASS] Corpse Slug: Ascension gives 27 to 29 HP and a Glomp of 9" << std::endl;
}

void test_gremlin_merc() {
    // Gimme 7 x2, Double Smash 6 x2 + 2 Weak, Hehe 8 + 2 Strength, and again. Each Hehe adds to what follows.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::GREMLIN_MERC);
    Enemy& merc = combat.enemies[0];
    assert(merc.type == EnemyType::GREMLIN_MERC && merc.name == "Diablotin mercenaire");
    assert(merc.hp >= 47 && merc.hp <= 49 && merc.max_hp == 47 + merc.hp_bonus && merc.block == 0);
    struct Step { EnemyIntentType intent; int damage; int hits; int weak; int strength; };
    const Step steps[] = {
        { ATTACK, 7, 2, 0, 0 }, { EnemyIntentType::ATTACK_DEBUFF, 6, 2, 2, 0 }, { EnemyIntentType::ATTACK_BUFF, 8, 1, 0, 2 },
        { ATTACK, 9, 2, 0, 2 }, { EnemyIntentType::ATTACK_DEBUFF, 8, 2, 2, 2 }, { EnemyIntentType::ATTACK_BUFF, 10, 1, 0, 4 },
    };
    for (const Step& step : steps) {
        assert(merc.current_intent.type == step.intent);
        assert(merc.intent_damage(combat.player) == step.damage && merc.current_intent.hits == step.hits);
        combat.player.hp = combat.player.max_hp;
        combat.player.clear_status(StatusType::WEAK);
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == step.damage * step.hits);
        assert(combat.player.get_status(StatusType::WEAK) == step.weak);
        assert(merc.get_status(StatusType::STRENGTH) == step.strength && merc.block == 0);
    }
    std::cout << "[PASS] Gremlin Merc: 47 to 49 HP, Gimme 7 x2, Double Smash 6 x2 + 2 Weak, Hehe 8 + 2 Strength" << std::endl;
}

void test_gremlin_merc_ascension() {
    // 51 to 53 HP, Gimme 8 x2, Double Smash 7 x2 and Hehe 9 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::GREMLIN_MERC);
        Enemy& merc = combat.enemies[0];
        merc.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = merc.hp;
        assert(merc.ascension_hp() == 51 && merc.hp_spread() == 2);
        assert(base >= 47 && base <= 49 && merc.max_hp == 47 + merc.hp_bonus);
        merc.ascend(true, true);
        combat.plan_enemies();
        assert(merc.hp == base + 4 && merc.hp >= 51 && merc.hp <= 53);
        const int damage[] = { 8, 7, 9 };
        for (int damage_of : damage) {
            assert(merc.intent_damage(combat.player) == damage_of);
            combat.player.hp = combat.player.max_hp;
            combat.end_player_turn();
        }
    }
    std::cout << "[PASS] Gremlin Merc: Ascension gives 51 to 53 HP, Gimme 8 x2, Double Smash 7 x2, Hehe 9" << std::endl;
}

void test_haunted_ship() {
    // Haunt (3 Weak and 5 Dazed), then Swipe (13) and Stomp (4 x3), and again from Swipe.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::HAUNTED_SHIP);
    Enemy& ship = combat.enemies[0];
    assert(ship.type == EnemyType::HAUNTED_SHIP && ship.name == "Navire hanté" && ship.hp == 63 && ship.max_hp == 63);
    assert(ship.block == 0 && ship.statuses.empty());
    auto dazed_everywhere = [&combat]() {
        int count = count_in(combat.draw_pile, CardId::DAZED) + count_in(combat.discard_pile, CardId::DAZED) +
                    count_in(combat.exhaust_pile, CardId::DAZED);
        for (const DeckCard& card : combat.hand) count += card.id == CardId::DAZED;
        return count;
    };
    assert(ship.current_intent.type == DEBUFF && ship.intent_damage(combat.player) == 0);
    assert(ship.current_intent.debuffs[0].type == StatusType::WEAK && ship.current_intent.debuffs[0].amount == 3);
    assert(ship.current_intent.status_card == CardId::DAZED && ship.current_intent.status_cards == 5);
    combat.end_player_turn();
    assert(combat.player.hp == combat.player.max_hp && combat.player.get_status(StatusType::WEAK) >= 1);
    assert(dazed_everywhere() == 5);
    assert(combat.last_enemy_actions[0].cards_added == CardId::DAZED && combat.last_enemy_actions[0].cards_added_count == 5);
    for (int cycle = 0; cycle < 2; ++cycle) {
        combat.player.statuses.clear();
        assert(ship.current_intent.type == ATTACK && ship.intent_damage(combat.player) == 13 && ship.current_intent.hits == 1);
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 13);
        combat.player.hp = combat.player.max_hp;
        assert(ship.current_intent.type == ATTACK && ship.current_intent.hits == 3 && ship.intent_damage(combat.player) == 4);
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 12);
        combat.player.hp = combat.player.max_hp;
        assert(dazed_everywhere() == 5);
    }
    assert(ship.block == 0 && ship.statuses.empty() && ship.hp == 63);
    std::cout << "[PASS] Haunted Ship: 63 HP, Haunt then Swipe 13 and Stomp 4x3" << std::endl;
}

void test_haunted_ship_ascension() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::HAUNTED_SHIP);
    Enemy& ship = combat.enemies[0];
    assert(ship.ascension_hp() == 67 && ship.hp_spread() == 0);
    ship.ascend(true, true);
    combat.plan_enemies();
    assert(ship.hp == 67 && ship.max_hp == 67);
    combat.end_player_turn();
    assert(ship.current_intent.type == ATTACK && ship.intent_damage(combat.player) == 14);
    combat.end_player_turn();
    assert(ship.current_intent.hits == 3 && ship.intent_damage(combat.player) == 5);
    std::cout << "[PASS] Haunted Ship: Ascension gives 67 HP, Swipe 14 and Stomp 5x3" << std::endl;
}

void test_sludge_spinner() {
    // Calm: Oil Spray (8 damage + 1 Weak) every turn. Hurt, it opens with Rage (6 damage, +3 Strength to itself),
    // then draws among Oil Spray, Slam (11) and Rage, never the same move twice in a row.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SLUDGE_SPINNER);
    Enemy& spinner = combat.enemies[0];
    assert(spinner.type == EnemyType::SLUDGE_SPINNER && spinner.name == "Tisse-boue");
    assert(spinner.hp >= 37 && spinner.hp <= 39 && spinner.max_hp == 37 + spinner.hp_bonus);
    for (int turn = 1; turn <= 3; ++turn) {
        const EnemyIntent& intent = spinner.current_intent;
        assert(spinner.phase == 0 && intent.type == ATTACK_DEBUFF && spinner.intent_damage(combat.player) == 8);
        assert(intent.debuffs[0].type == StatusType::WEAK && intent.debuffs[0].amount == 1);
        combat.player.hp = combat.player.max_hp;
        combat.player.statuses.clear();
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 8 && combat.player.get_status(StatusType::WEAK) >= 1);
    }

    CombatEngine hurt = make_combat(CharacterId::IRONCLAD, EnemyType::SLUDGE_SPINNER);
    Enemy& angry = hurt.enemies[0];
    hurt.damage_enemy(angry, 5);
    hurt.player.hp = hurt.player.max_hp;
    hurt.end_player_turn();
    assert(angry.phase == 1);
    assert(angry.current_intent.type == ATTACK_BUFF && angry.intent_damage(hurt.player) == 6);
    assert(angry.current_intent.status_type == StatusType::STRENGTH && angry.current_intent.status_amount == 3);
    hurt.player.hp = hurt.player.max_hp;
    hurt.player.statuses.clear();
    hurt.end_player_turn();
    assert(hurt.player.max_hp - hurt.player.hp == 6 && angry.get_status(StatusType::STRENGTH) == 3);
    bool seen[3] = { false, false, true };
    int last = 2;
    for (int turn = 0; turn < 40; ++turn) {
        const EnemyIntent& intent = angry.current_intent;
        int move = intent.type == ATTACK_DEBUFF ? 0 : (intent.type == ATTACK_BUFF ? 2 : 1);
        assert(angry.phase == 1 && move != last);
        if (move == 0) assert(intent.base_damage == 8 && intent.debuffs[0].type == StatusType::WEAK);
        if (move == 1) assert(intent.type == ATTACK && intent.base_damage == 11);
        if (move == 2) assert(intent.base_damage == 6 && intent.status_amount == 3);
        seen[move] = true;
        last = move;
        hurt.player.hp = hurt.player.max_hp;
        hurt.end_player_turn();
        assert(hurt.state == CombatState::PLAYER_TURN);
    }
    assert(seen[0] && seen[1] && seen[2]);
    std::cout << "[PASS] Sludge Spinner: 37 to 39 HP, Oil Spray, Rage once hurt then Slam / Oil Spray / Rage" << std::endl;
}

void test_sludge_spinner_ascension() {
    // 41 HP at least, an Oil Spray of 9, a Slam of 12 and a Rage of 7 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SLUDGE_SPINNER);
        Enemy& spinner = combat.enemies[0];
        spinner.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = spinner.hp;
        assert(spinner.ascension_hp() == 41 && spinner.hp_spread() == 2);
        assert(base >= 37 && base <= 39);
        spinner.ascend(true, true);
        combat.plan_enemies();
        assert(spinner.hp == base + 4 && spinner.hp >= 41);
        assert(spinner.current_intent.type == ATTACK_DEBUFF && spinner.intent_damage(combat.player) == 9);
    }
    CombatEngine hurt = make_combat(CharacterId::IRONCLAD, EnemyType::SLUDGE_SPINNER);
    Enemy& angry = hurt.enemies[0];
    angry.ascend(true, true);
    hurt.plan_enemies();
    hurt.damage_enemy(angry, 5);
    hurt.player.hp = hurt.player.max_hp;
    hurt.end_player_turn();
    assert(angry.current_intent.type == ATTACK_BUFF && angry.intent_damage(hurt.player) == 7);
    bool saw_slam = false;
    for (int turn = 0; turn < 40; ++turn) {
        if (angry.current_intent.type == ATTACK) {
            assert(angry.current_intent.base_damage == 12);
            saw_slam = true;
        }
        hurt.player.hp = hurt.player.max_hp;
        hurt.end_player_turn();
    }
    assert(saw_slam);
    std::cout << "[PASS] Sludge Spinner: Ascension gives 41+ HP, Oil Spray of 9, Slam of 12, Rage of 7" << std::endl;
}

void check_toadpole_cycle(CombatEngine& combat, size_t index, const EnemyIntentType* intents, int damage_step) {
    // Spiken (Thorns 2), Spike Spit (3 x 3, Thorns 2) and Whirl (7), in the order given, over two cycles.
    Enemy& toad = combat.enemies[index];
    combat.enemies[1 - index].hp = 0; // its partner would add its own damage; the pattern is kept without it
    int thorns = 0;
    for (int turn = 0; turn < 6; ++turn) {
        const EnemyIntent& intent = toad.current_intent;
        assert(intent.type == intents[turn]);
        int hits = 0, damage = 0;
        if (intent.type == BUFF) {
            assert(toad.intent_damage(combat.player) == 0);
            thorns += 2;
        } else if (intent.type == ATTACK_BUFF) {
            assert(intent.hits == 3 && toad.intent_damage(combat.player) == 3 + damage_step);
            hits = 3;
            damage = 3 + damage_step;
            thorns += 2;
        } else {
            assert(intent.type == ATTACK && intent.hits == 1 && toad.intent_damage(combat.player) == 7 + damage_step);
            hits = 1;
            damage = 7 + damage_step;
        }
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == hits * damage);
        assert(toad.block == 0 && toad.get_status(StatusType::THORNS) == thorns);
    }
}

void test_toadpole() {
    CombatEngine pair = make_pack({ EnemyType::TOADPOLE, EnemyType::TOADPOLE });
    for (Enemy& toad : pair.enemies) {
        assert(toad.type == EnemyType::TOADPOLE && toad.name == "Crapautard");
        assert(toad.hp >= 21 && toad.hp <= 25 && toad.max_hp == toad.hp && toad.max_hp == 21 + toad.hp_bonus);
        assert(toad.block == 0 && toad.statuses.empty());
    }
    // The one in front starts with Spiken, the one behind with Whirl; each loops through the same three moves.
    assert(pair.enemies[0].current_intent.type == BUFF);
    assert(pair.enemies[1].current_intent.type == ATTACK);
    const EnemyIntentType front[] = { BUFF, ATTACK_BUFF, ATTACK, BUFF, ATTACK_BUFF, ATTACK };
    const EnemyIntentType behind[] = { ATTACK, BUFF, ATTACK_BUFF, ATTACK, BUFF, ATTACK_BUFF };
    CombatEngine front_fight = make_pack({ EnemyType::TOADPOLE, EnemyType::TOADPOLE });
    check_toadpole_cycle(front_fight, 0, front, 0);
    CombatEngine behind_fight = make_pack({ EnemyType::TOADPOLE, EnemyType::TOADPOLE });
    check_toadpole_cycle(behind_fight, 1, behind, 0);
    std::cout << "[PASS] Toadpole: 21 to 25 HP, Spiken, Spike Spit 3x3 and Whirl 7 by position, Thorns 2 each time"
              << std::endl;
}

void test_toadpole_ascension() {
    // 22 to 26 HP, Spike Spit 4x3 and Whirl 8 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_pack({ EnemyType::TOADPOLE, EnemyType::TOADPOLE });
        Enemy& toad = combat.enemies[0];
        toad.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = toad.hp;
        assert(toad.ascension_hp() == 22 && toad.hp_spread() == 4);
        assert(base >= 21 && base <= 25 && toad.max_hp == 21 + toad.hp_bonus);
        toad.ascend(true, true);
        combat.plan_enemies();
        assert(toad.hp == base + 1 && toad.max_hp == base + 1 && toad.hp >= 22 && toad.hp <= 26);
        const EnemyIntentType front[] = { BUFF, ATTACK_BUFF, ATTACK, BUFF, ATTACK_BUFF, ATTACK };
        check_toadpole_cycle(combat, 0, front, 1);
    }
    std::cout << "[PASS] Toadpole: Ascension gives 22 to 26 HP, Spike Spit 4x3 and Whirl 8" << std::endl;
}

void test_hunter_killer() {
    // Tenderizing Goop (Tender 1 on the player), Bite 17, Puncture 7x3, then a draw between Bite (never twice
    // in a row) and Puncture.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::HUNTER_KILLER);
    Enemy& hunter = combat.enemies[0];
    assert(hunter.type == EnemyType::HUNTER_KILLER && hunter.name == "Chasseur-tueur");
    assert(hunter.hp == 121 && hunter.max_hp == 121 && hunter.block == 0 && hunter.statuses.empty());

    assert(hunter.current_intent.type == DEBUFF && hunter.intent_damage(combat.player) == 0);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.state == CombatState::PLAYER_TURN);
    assert(combat.player.hp == combat.player.max_hp && combat.has_power(PowerId::TENDER));
    assert(combat.power_amount(PowerId::TENDER) == 1);

    assert(hunter.current_intent.type == ATTACK && hunter.current_intent.hits == 1);
    assert(hunter.intent_damage(combat.player) == 17);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 17);

    assert(hunter.current_intent.type == ATTACK && hunter.current_intent.hits == 3);
    assert(hunter.intent_damage(combat.player) == 7);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 21);

    bool saw_bite = false, saw_puncture = false;
    bool previous_bite = false;
    for (int turn = 0; turn < 40; ++turn) {
        const EnemyIntent& intent = hunter.current_intent;
        assert(intent.type == ATTACK);
        bool bite = intent.hits == 1;
        if (bite) {
            assert(!previous_bite && hunter.intent_damage(combat.player) == 17);
        } else {
            assert(intent.hits == 3 && hunter.intent_damage(combat.player) == 7);
        }
        saw_bite |= bite;
        saw_puncture |= !bite;
        previous_bite = bite;
        assert(hunter.block == 0 && hunter.statuses.empty() && hunter.hp == 121);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == (bite ? 17 : 21));
    }
    assert(saw_bite && saw_puncture);
    std::cout << "[PASS] Hunter Killer: 121 HP, Tenderizing Goop, Bite, Puncture, then a draw" << std::endl;
}

void test_hunter_killer_ascension() {
    // 126 HP, Bite 19 and Puncture 8x3 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::HUNTER_KILLER);
    Enemy& hunter = combat.enemies[0];
    assert(hunter.ascension_hp() == 126);
    hunter.ascend(true, true);
    combat.plan_enemies();
    assert(hunter.hp == 126 && hunter.max_hp == 126);
    assert(hunter.current_intent.type == DEBUFF);
    pass_turn(combat);
    assert(hunter.current_intent.type == ATTACK && hunter.intent_damage(combat.player) == 19);
    pass_turn(combat);
    assert(hunter.current_intent.hits == 3 && hunter.intent_damage(combat.player) == 8);
    std::cout << "[PASS] Hunter Killer: Ascension gives 126 HP, a Bite of 19 and a Puncture of 8x3" << std::endl;
}

void test_mawler() {
    // Claw (4 x2) first, then a draw among Rip and Tear (14), Roar (3 Vulnerable) and Claw, never the move just
    // played, and Roar only once.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::MAWLER);
    Enemy& mawler = combat.enemies[0];
    assert(mawler.type == EnemyType::MAWLER && mawler.name == "Écharneur");
    assert(mawler.hp == 72 && mawler.max_hp == 72);
    int previous = -1;
    int roars = 0;
    bool saw_rip = false, saw_claw = false;
    for (int turn = 1; turn <= 40; ++turn) {
        assert(mawler.block == 0);
        combat.player.statuses.clear(); // Vulnerable from Roar would raise the damage below
        const EnemyIntent& intent = mawler.current_intent;
        int move;
        int expected_damage;
        if (intent.type == DEBUFF) {
            assert(mawler.intent_damage(combat.player) == 0);
            assert(intent.debuffs[0].type == StatusType::VULNERABLE && intent.debuffs[0].amount == 3);
            move = 1;
            expected_damage = 0;
            ++roars;
        } else if (intent.hits == 2) {
            assert(intent.type == ATTACK && mawler.intent_damage(combat.player) == 4);
            move = 2;
            expected_damage = 8;
            saw_claw = true;
        } else {
            assert(intent.type == ATTACK && intent.hits == 1 && mawler.intent_damage(combat.player) == 14);
            move = 0;
            expected_damage = 14;
            saw_rip = true;
        }
        if (turn == 1) assert(move == 2);
        assert(move != previous);
        previous = move;
        combat.player.hp = combat.player.max_hp;
        combat.player.statuses.clear();
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == expected_damage);
        if (move == 1) assert(combat.player.get_status(StatusType::VULNERABLE) >= 1);
    }
    assert(roars == 1 && saw_rip && saw_claw);
    std::cout << "[PASS] Mawler: 72 HP, Claw first, then Rip and Tear / Roar (once) / Claw, no repeat" << std::endl;
}

void test_mawler_ascension() {
    // 76 HP, Rip and Tear of 16 and a Claw of 5 x2 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::MAWLER);
    Enemy& mawler = combat.enemies[0];
    assert(mawler.ascension_hp() == 76);
    mawler.ascend(true, true);
    combat.plan_enemies();
    assert(mawler.hp == 76 && mawler.max_hp == 76);
    assert(mawler.current_intent.hits == 2 && mawler.intent_damage(combat.player) == 5);
    bool saw_rip = false;
    for (int turn = 0; turn < 20; ++turn) {
        combat.player.hp = combat.player.max_hp;
        combat.player.statuses.clear();
        combat.end_player_turn();
        combat.player.statuses.clear();
        if (mawler.current_intent.type == ATTACK && mawler.current_intent.hits == 1) {
            assert(mawler.intent_damage(combat.player) == 16);
            saw_rip = true;
        }
    }
    assert(saw_rip);
    std::cout << "[PASS] Mawler: Ascension gives 76 HP, Rip and Tear of 16 and a Claw of 5 x2" << std::endl;
}

void test_fossil_stalker() {
    // Calm: Latch (12) every turn. Hurt, it opens with Lash (3 twice), then draws among Latch, Tackle (9 and
    // 1 Frail) and Lash, a move being allowed to repeat.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FOSSIL_STALKER);
    Enemy& stalker = combat.enemies[0];
    assert(stalker.type == EnemyType::FOSSIL_STALKER && stalker.name == "Traqueur fossilisé");
    assert(stalker.hp >= 51 && stalker.hp <= 53 && stalker.max_hp == stalker.hp && stalker.max_hp == 51 + stalker.hp_bonus);
    for (int turn = 1; turn <= 4; ++turn) {
        assert(stalker.phase == 0 && stalker.block == 0 && stalker.statuses.empty());
        assert(stalker.current_intent.type == ATTACK && stalker.intent_damage(combat.player) == 12);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == 12);
    }

    CombatEngine hurt = make_combat(CharacterId::IRONCLAD, EnemyType::FOSSIL_STALKER);
    Enemy& beast = hurt.enemies[0];
    hurt.damage_enemy(beast, 5);
    hurt.player.hp = hurt.player.max_hp;
    hurt.end_player_turn();
    assert(beast.phase == 1);
    assert(beast.current_intent.type == ATTACK && beast.current_intent.hits == 2 && beast.intent_damage(hurt.player) == 3);
    hurt.player.hp = hurt.player.max_hp;
    hurt.end_player_turn();
    assert(hurt.player.max_hp - hurt.player.hp == 6);
    bool seen[3] = { false, false, false }; // Latch, Tackle, Lash
    for (int turn = 0; turn < 60; ++turn) {
        const EnemyIntent& intent = beast.current_intent;
        assert(beast.phase == 1);
        hurt.player.hp = hurt.player.max_hp;
        hurt.player.statuses.clear();
        int expected;
        if (intent.type == ATTACK_DEBUFF) {
            assert(intent.base_damage == 9 && intent.hits == 1);
            assert(intent.debuffs[0].type == StatusType::FRAIL && intent.debuffs[0].amount == 1);
            seen[1] = true;
            expected = 9;
        } else if (intent.hits == 2) {
            assert(intent.type == ATTACK && intent.base_damage == 3);
            seen[2] = true;
            expected = 6;
        } else {
            assert(intent.type == ATTACK && intent.base_damage == 12);
            seen[0] = true;
            expected = 12;
        }
        hurt.end_player_turn();
        assert(hurt.state == CombatState::PLAYER_TURN);
        assert(hurt.player.max_hp - hurt.player.hp == expected);
    }
    assert(seen[0] && seen[1] && seen[2]);
    std::cout << "[PASS] Fossil Stalker: 51 to 53 HP, Latch until hurt, then Lash and a draw of Latch / Tackle / Lash" << std::endl;
}

void test_fossil_stalker_ascension() {
    // 54 to 56 HP (51 to 53 plus 3), Latch of 14, Tackle of 11 and Lash of 4 twice at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FOSSIL_STALKER);
        Enemy& stalker = combat.enemies[0];
        stalker.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = stalker.hp;
        assert(stalker.ascension_hp() == 54 && stalker.hp_spread() == 2);
        assert(base >= 51 && base <= 53 && stalker.max_hp == 51 + stalker.hp_bonus);
        stalker.ascend(true, true);
        combat.plan_enemies();
        assert(stalker.hp == base + 3 && stalker.max_hp == base + 3 && stalker.hp >= 54 && stalker.hp <= 56);
        assert(stalker.current_intent.type == ATTACK && stalker.intent_damage(combat.player) == 14);
        combat.damage_enemy(stalker, 1);
        combat.end_player_turn();
        assert(stalker.current_intent.hits == 2 && stalker.intent_damage(combat.player) == 4);
        bool tackle = false;
        for (int turn = 0; turn < 60 && !tackle; ++turn) {
            combat.player.hp = combat.player.max_hp;
            combat.end_player_turn();
            if (stalker.current_intent.type == ATTACK_DEBUFF) {
                assert(stalker.intent_damage(combat.player) == 11);
                tackle = true;
            }
        }
        assert(tackle);
    }
    std::cout << "[PASS] Fossil Stalker: Ascension gives 54 to 56 HP, Latch 14, Tackle 11, Lash 4x2" << std::endl;
}

void test_spiny_toad() {
    // Protruding Spikes (+5 Thorns), Spike Explosion (23 damage, -5 Thorns), Tongue Lash (17 damage), twice over.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SPINY_TOAD);
    Enemy& toad = combat.enemies[0];
    assert(toad.type == EnemyType::SPINY_TOAD && toad.name == "Crapineux");
    assert(toad.hp >= 116 && toad.hp <= 119 && toad.max_hp == toad.hp && toad.max_hp == 116 + toad.hp_bonus);
    const int16_t hp = toad.hp;
    assert(toad.block == 0 && toad.statuses.empty());
    const EnemyIntentType intents[3] = { EnemyIntentType::BUFF, EnemyIntentType::ATTACK_BUFF, EnemyIntentType::ATTACK };
    const int damage[3] = { 0, 23, 17 };
    const int thorns[3] = { 5, 0, 0 };
    for (int turn = 1; turn <= 6; ++turn) {
        const int phase = (turn - 1) % 3;
        assert(combat.turn_number == turn);
        assert(toad.current_intent.type == intents[phase]);
        assert(toad.intent_damage(combat.player) == damage[phase]);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == damage[phase]);
        assert(toad.get_status(StatusType::THORNS) == thorns[phase]);
        assert(toad.statuses.size() == (thorns[phase] != 0 ? 1u : 0u)); // the spent Thorns leave no status behind
        assert(toad.block == 0 && toad.hp == hp);
    }
    std::cout << "[PASS] Spiny Toad: 116 to 119 HP, +5 Thorns, Spike Explosion 23 (-5 Thorns), Tongue Lash 17" << std::endl;
}

void test_spiny_toad_ascension() {
    // 121 to 124 HP (116 to 119 plus 5), Spike Explosion of 25 and Tongue Lash of 19 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SPINY_TOAD);
        Enemy& toad = combat.enemies[0];
        toad.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = toad.hp;
        assert(toad.ascension_hp() == 121 && toad.hp_spread() == 3);
        assert(base >= 116 && base <= 119 && toad.max_hp == 116 + toad.hp_bonus);
        toad.ascend(true, true);
        combat.plan_enemies();
        assert(toad.hp == base + 5 && toad.max_hp == base + 5 && toad.hp >= 121 && toad.hp <= 124);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn(); // Protruding Spikes
        assert(toad.current_intent.type == EnemyIntentType::ATTACK_BUFF && toad.intent_damage(combat.player) == 25);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn(); // Spike Explosion
        assert(combat.player.max_hp - combat.player.hp == 25);
        assert(toad.current_intent.type == EnemyIntentType::ATTACK && toad.intent_damage(combat.player) == 19);
    }
    std::cout << "[PASS] Spiny Toad: Ascension gives 121 to 124 HP, Spike Explosion 25 and Tongue Lash 19" << std::endl;
}

void test_seapunk() {
    // Sea Kick 11, Spinning Kick 2 x4, Bubble Burp (7 Block, +1 Strength), and again with the Strength kept.
    CombatEngine probe = make_combat(CharacterId::IRONCLAD, EnemyType::SEAPUNK);
    assert(probe.enemies[0].hp == 44 + probe.enemies[0].hp_bonus);
    check_monster(EnemyType::SEAPUNK, "Voyou marin", probe.enemies[0].hp, {
        { ATTACK, 11, 0, 0 },
        { ATTACK, 2, 0, 0, 0, 4 },
        { BUFF, 0, 7, 1 },
        { ATTACK, 12, 0, 1 },
        { ATTACK, 3, 0, 1, 0, 4 },
        { BUFF, 0, 7, 2 },
        { ATTACK, 13, 0, 2 },
    });
    std::cout << "[PASS] Seapunk: 44 HP, Sea Kick 11, Spinning Kick 2x4, Bubble Burp 7 Block + 1 Strength" << std::endl;
}

void test_seapunk_ascension() {
    // 47 to 49 HP (44 to 46 plus 3), Sea Kick 13, Bubble Burp 8 Block + 2 Strength at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SEAPUNK);
        Enemy& punk = combat.enemies[0];
        punk.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        const int16_t base = punk.hp;
        assert(punk.ascension_hp() == 47 && punk.hp_spread() == 2);
        assert(base >= 44 && base <= 46 && punk.max_hp == 44 + punk.hp_bonus);
        punk.ascend(true, true);
        combat.plan_enemies();
        assert(punk.hp == base + 3 && punk.max_hp == base + 3 && punk.hp >= 47 && punk.hp <= 49);
        assert(punk.current_intent.type == ATTACK && punk.intent_damage(combat.player) == 13);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(punk.current_intent.type == ATTACK && punk.current_intent.hits == 4 &&
               punk.intent_damage(combat.player) == 2);
        combat.end_player_turn();
        assert(punk.current_intent.type == BUFF);
        combat.end_player_turn();
        assert(punk.block == 8 && punk.get_status(StatusType::STRENGTH) == 2);
    }
    std::cout << "[PASS] Seapunk: Ascension gives 47 to 49 HP, Sea Kick 13, Bubble Burp 8 Block + 2 Strength" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Monsters ===" << std::endl;
    test_fuzzy_wurm_crawler();
    test_crossbow_ruby_raider();
    test_brute_ruby_raider();
    test_assassin_ruby_raider();
    test_axe_ruby_raider();
    test_tracker_ruby_raider();
    test_tracker_ruby_raider_ascension();
    test_calcified_cultist();
    test_damp_cultist();
    test_sewer_clam();
    test_monster_block_absorbs_damage();
    test_debuff_intents();
    test_debuff_duration();
    test_debuff_and_artifact();
    test_the_forgotten();
    test_the_forgotten_ascension();
    test_living_shield();
    test_phase_with_entry_condition();
    test_weighted_draw_without_repetition();
    test_living_shield_ascension();
    test_stun_intent();
    test_heal_intent();
    test_escape_intent();
    test_escape_gives_no_reward();
    test_fat_gremlin();
    test_fat_gremlin_ascension();
    test_enemy_power_holder();
    test_enemy_power_turn_hook();
    test_enemy_power_hit_hook();
    test_enemy_power_card_hook();
    test_multi_hit_attack();
    test_vine_shambler();
    test_vine_shambler_ascension();
    test_status_intent();
    test_status_cards_are_not_the_players();
    test_eye_with_teeth();
    test_stabbot();
    test_stabbot_ascension();
    test_spiny_toad();
    test_spiny_toad_ascension();
    test_noisebot();
    test_noisebot_ascension();
    test_cubex_construct();
    test_cubex_construct_ascension();
    test_punch_construct();
    test_punch_construct_ascension();
    test_infested_prism();
    test_infested_prism_ascension();
    test_infested_prism_vital_spark();
    test_inklet();
    test_inklet_ascension();
    test_nibbit();
    test_nibbit_ascension();
    test_gas_bomb();
    test_gas_bomb_ascension();
    test_flyconid();
    test_flyconid_ascension();
    test_corpse_slug();
    test_corpse_slug_ascension();
    test_gremlin_merc();
    test_gremlin_merc_ascension();
    test_sneaky_gremlin();
    test_sneaky_gremlin_ascension();
    test_haunted_ship();
    test_haunted_ship_ascension();
    test_sludge_spinner();
    test_sludge_spinner_ascension();
    test_toadpole();
    test_toadpole_ascension();
    test_hunter_killer();
    test_hunter_killer_ascension();
    test_mawler();
    test_mawler_ascension();
    test_fossil_stalker();
    test_fossil_stalker_ascension();
    test_seapunk();
    test_seapunk_ascension();
    std::cout << "=== All Monsters Passed! ===" << std::endl;
    return 0;
}
