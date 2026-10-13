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
    int plating = 0;  // Plating the enemy holds after its turn
    int intangible = 0; // Intangible the enemy holds after its turn
};

// Lets the enemy play `turns` one after the other against a player who never plays a card.
void check_monster(EnemyType type, std::string_view name, int hp, std::initializer_list<Turn> turns) {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, type);
    assert(combat.enemies.size() == 1);
    Enemy& enemy = combat.enemies[0];
    assert(enemy.type == type);
    assert(enemy.name == name);
    assert(enemy.hp == hp && enemy.max_hp == hp);
    // Nothing but the Plating some monsters start the fight with.
    assert(enemy.block == 0 && enemy.statuses.size() == static_cast<size_t>(enemy.get_status(StatusType::PLATED_ARMOR) > 0));

    int turn_number = 1;
    for (const Turn& turn : turns) {
        assert(combat.turn_number == turn_number);
        assert(enemy.current_intent.type == turn.intent);
        assert(enemy.intent_damage(combat.player) == turn.damage);

        combat.player.hp = combat.player.max_hp; // long cycles must not kill the player
        // The Beckons the player holds at the end of the turn cost HP too.
        int held_loss = 0;
        for (const DeckCard& card : combat.hand) {
            if (card.id == CardId::BECKON) held_loss += CardDef::get_value(card.id, card.upgraded);
        }
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == turn.damage * turn.hits + held_loss);
        assert(enemy.block == turn.block);
        assert(enemy.get_status(StatusType::STRENGTH) == turn.strength);
        assert(enemy.get_status(StatusType::RITUAL) == turn.ritual);
        assert(enemy.get_status(StatusType::PLATED_ARMOR) == turn.plating);
        assert(enemy.get_status(StatusType::INTANGIBLE) == turn.intangible);
        // Nothing else: no other status, and no move costs the monster HP.
        assert(enemy.statuses.size() == static_cast<size_t>((turn.strength != 0) + (turn.ritual != 0) +
                                                            (turn.plating != 0) + (turn.intangible != 0)));
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
constexpr EnemyIntentType SUMMON = EnemyIntentType::SUMMON;

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

template <typename Pile>
int count_cards(const Pile& pile, CardId id) {
    int n = 0;
    for (const DeckCard& card : pile) n += card.id == id;
    return n;
}

void test_mecha_knight() {
    // 300 HP. Charge 25, Flamethrower 8 + 4 Burn, Windup 15 Block + 5 Strength, Heavy Cleave 35, then the
    // last three again from Flamethrower. Strength adds to every attack that follows.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::MECHA_KNIGHT);
    Enemy& knight = combat.enemies[0];
    assert(knight.type == EnemyType::MECHA_KNIGHT && knight.name == "Méca-Chevalier");
    assert(knight.hp == 300 && knight.max_hp == 300 && knight.block == 0 && knight.statuses.empty());

    int burns = 0;
    int strength = 0;
    auto play = [&](EnemyIntentType intent, int damage) {
        assert(damage >= 0);
        assert(knight.current_intent.type == intent && knight.intent_damage(combat.player) == damage);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
    };
    auto total_burns = [&]() {
        return count_cards(combat.draw_pile, CardId::BURN) + count_cards(combat.hand, CardId::BURN) +
               count_cards(combat.discard_pile, CardId::BURN);
    };

    play(ATTACK, 25); // Charge
    assert(combat.player.hp == combat.player.max_hp - 25 && knight.block == 0 && total_burns() == 0);
    play(ATTACK_DEBUFF, 8); // Flamethrower
    burns += 4;
    assert(total_burns() == burns && knight.block == 0);
    play(BUFF, 0); // Windup
    strength += 5;
    assert(knight.block == 15 && knight.get_status(StatusType::STRENGTH) == strength);
    const int burns_in_hand = count_cards(combat.hand, CardId::BURN);
    combat.player.hp = combat.player.max_hp;
    assert(knight.current_intent.type == ATTACK && knight.intent_damage(combat.player) == 40); // 35 + 5 Strength
    combat.end_player_turn();
    assert(combat.player.hp == combat.player.max_hp - 40 - 2 * burns_in_hand);
    assert(knight.block == 0);
    // The cycle starts again at Flamethrower, with the Strength kept.
    assert(knight.current_intent.type == ATTACK_DEBUFF && knight.intent_damage(combat.player) == 13);
    assert(knight.current_intent.status_card == CardId::BURN && knight.current_intent.status_cards == 4);
    std::cout << "[PASS] Mecha Knight: 300 HP, Charge, Flamethrower + Burn, Windup, Heavy Cleave, loop" << std::endl;
}

void mecha_pass(CombatEngine& combat) {
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.state == CombatState::PLAYER_TURN);
}

void test_mecha_knight_ascension() {
    // 320 HP, Charge 30, Flamethrower 12, Heavy Cleave 40 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::MECHA_KNIGHT);
    Enemy& knight = combat.enemies[0];
    assert(knight.ascension_hp() == 320);
    knight.ascend(true, true);
    combat.plan_enemies();
    assert(knight.hp == 320 && knight.max_hp == 320);
    assert(knight.current_intent.type == ATTACK && knight.intent_damage(combat.player) == 30);
    mecha_pass(combat);
    assert(knight.current_intent.type == ATTACK_DEBUFF && knight.intent_damage(combat.player) == 12);
    mecha_pass(combat);
    assert(knight.current_intent.type == BUFF && knight.current_intent.block == 15);
    mecha_pass(combat);
    assert(knight.current_intent.type == ATTACK && knight.intent_damage(combat.player) == 45); // 40 + 5 Strength
    std::cout << "[PASS] Mecha Knight: Ascension gives 320 HP, Charge 30, Flamethrower 12, Heavy Cleave 40" << std::endl;
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

void test_the_lost() {
    // Debilitating Smog (takes 2 Strength from the player and gains 2), Eye Lasers (4 x 2 + its Strength), and again.
    check_monster(EnemyType::THE_LOST, "L'Égaré", 93, {
        Turn{ DEBUFF, 0, 0, 2 }, Turn{ ATTACK, 6, 0, 2, 0, 2 },
        Turn{ DEBUFF, 0, 0, 4 }, Turn{ ATTACK, 8, 0, 4, 0, 2 },
        Turn{ DEBUFF, 0, 0, 6 }, Turn{ ATTACK, 10, 0, 6, 0, 2 } });

    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::THE_LOST);
    Enemy& enemy = combat.enemies[0];
    assert(enemy.current_intent.debuffs_player());
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::STRENGTH) == -2 && enemy.get_status(StatusType::STRENGTH) == 2);
    assert(!enemy.current_intent.debuffs_player());
    combat.end_player_turn();
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::STRENGTH) == -4 && enemy.get_status(StatusType::STRENGTH) == 4);

    // Killed, it gives back all the Strength it stole.
    enemy.hp = 1;
    combat.hand.clear();
    combat.hand.push_back(CardId::STRIKE);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, 0);
    assert(played && !enemy.is_alive());
    assert(combat.player.get_status(StatusType::STRENGTH) == 0);
    std::cout << "[PASS] The Lost: 93 HP, Debilitating Smog steals 2 Strength, Eye Lasers 4x2 + Strength" << std::endl;
}

void test_the_lost_ascension() {
    // 99 HP when the monsters are tougher (from A7), Eye Lasers 5 x 2 when they hit harder (from A2).
    auto fight = [](uint8_t ascension) {
        RunManager run;
        run.start_run(CharacterId::IRONCLAD, ascension, PaletteVariant::CLASSIC, 777);
        run.map.nodes[0].type = RoomType::MONSTER;
        run.enter_room(0);
        FixedVector<EnemyType, 3> pack;
        pack.push_back(EnemyType::THE_LOST);
        run.combat.start_combat_multi(pack, run.character);
        Enemy& foe = run.combat.enemies[0];
        foe.ascend(run.get_enemy_hp_multiplier() > 100, run.get_enemy_damage_multiplier() > 100);
        run.combat.plan_enemies();
        return run;
    };
    struct Level {
        uint8_t ascension;
        int hp;
        int laser;
    };
    for (const Level& level : { Level{ 0, 93, 4 }, Level{ 2, 93, 5 }, Level{ 7, 99, 5 }, Level{ 9, 99, 5 } }) {
        RunManager run = fight(level.ascension);
        Enemy& foe = run.combat.enemies[0];
        assert(foe.hp == level.hp && foe.max_hp == level.hp && foe.damage_percent == 100);
        assert(foe.current_intent.type == DEBUFF);
        run.combat.end_player_turn();
        assert(foe.current_intent.base_damage == level.laser && foe.current_intent.hits == 2);
        assert(foe.intent_damage(run.combat.player) == level.laser + 2);
    }
    Enemy lost;
    lost.init(EnemyType::THE_LOST);
    assert(lost.ascension_hp() == 99);
    std::cout << "[PASS] The Lost: Ascension gives 99 HP and Eye Lasers 5" << std::endl;
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

template <typename Pile>
int count_in(const Pile& pile, CardId id) {
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

void test_zapbot() {
    // Zap every turn: 14 damage, no Block, no status.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::ZAPBOT);
    Enemy& bot = combat.enemies[0];
    assert(bot.type == EnemyType::ZAPBOT && bot.name == "Déchargebot");
    assert(bot.hp >= 18 && bot.hp <= 23 && bot.max_hp == bot.hp && bot.max_hp == 18 + bot.hp_bonus);
    const int16_t hp = bot.hp;
    assert(bot.block == 0 && bot.statuses.empty());
    for (int turn = 1; turn <= 4; ++turn) {
        assert(combat.turn_number == turn);
        assert(bot.current_intent.type == ATTACK && bot.intent_damage(combat.player) == 14);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == 14);
        assert(bot.block == 0 && bot.statuses.empty() && bot.hp == hp);
    }
    std::cout << "[PASS] Zapbot: 18 to 23 HP, Zap 14 damage every turn" << std::endl;
}

void test_zapbot_ascension() {
    // 19 to 24 HP (18 to 23 plus 1) and a Zap of 15 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::ZAPBOT);
        Enemy& bot = combat.enemies[0];
        bot.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = bot.hp;
        assert(bot.ascension_hp() == 19 && bot.hp_spread() == 5);
        assert(base >= 18 && base <= 23 && bot.max_hp == 18 + bot.hp_bonus);
        bot.ascend(true, true);
        combat.plan_enemies();
        assert(bot.hp == base + 1 && bot.max_hp == base + 1 && bot.hp >= 19 && bot.hp <= 24);
        assert(bot.current_intent.type == ATTACK && bot.intent_damage(combat.player) == 15);
    }
    std::cout << "[PASS] Zapbot: Ascension gives 19 to 24 HP and a Zap of 15" << std::endl;
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

void test_parafright() {
    // Slam 16 every turn.
    check_monster(EnemyType::PARAFRIGHT, "Pareffroi", 21, {
        { ATTACK, 16, 0, 0 },
        { ATTACK, 16, 0, 0 },
        { ATTACK, 16, 0, 0 },
        { ATTACK, 16, 0, 0 },
    });
    std::cout << "[PASS] Parafright: 21 HP, Slam 16 every turn" << std::endl;
}

void test_parafright_ascension() {
    // Same 21 HP (no Ascension HP), Slam 17 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::PARAFRIGHT);
    Enemy& ghost = combat.enemies[0];
    ghost.ascend(true, true);
    combat.plan_enemies();
    assert(ghost.hp == 21 && ghost.max_hp == 21);
    for (int turn = 1; turn <= 3; ++turn) {
        assert(ghost.current_intent.type == ATTACK && ghost.intent_damage(combat.player) == 17);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 17);
    }
    std::cout << "[PASS] Parafright: Ascension gives a Slam of 17" << std::endl;
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

void test_globe_head() {
    // Shocking Slap 13 (2 Frail), Thunder Strike 6x3, Galvanic Burst 16 (+2 Strength), and again.
    check_monster(EnemyType::GLOBE_HEAD, "Globocéphale", 148, {
        { ATTACK_DEBUFF, 13, 0, 0, 0, 1 },
        { ATTACK, 6, 0, 0, 0, 3 },
        { ATTACK_BUFF, 16, 0, 2 },
        { ATTACK_DEBUFF, 15, 0, 2 },
        { ATTACK, 8, 0, 2, 0, 3 },
        { ATTACK_BUFF, 18, 0, 4 },
    });
    std::cout << "[PASS] Globe Head: 148 HP, Shocking Slap 13, Thunder Strike 6x3, Galvanic Burst 16, loop" << std::endl;
}

void test_globe_head_ascension() {
    // 158 HP, Shocking Slap 14, Thunder Strike 7x3 and Galvanic Burst 17 (+2 Strength) at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::GLOBE_HEAD);
    Enemy& head = combat.enemies[0];
    head.ascend(true, true);
    combat.plan_enemies();
    assert(head.hp == 158 && head.max_hp == 158);
    assert(head.current_intent.type == ATTACK_DEBUFF && head.intent_damage(combat.player) == 14);
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 14);
    assert(combat.player.get_status(StatusType::FRAIL) == 2);
    combat.player.hp = combat.player.max_hp;
    assert(head.current_intent.type == ATTACK && head.current_intent.hits == 3);
    assert(head.intent_damage(combat.player) == 7);
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 21);
    combat.player.hp = combat.player.max_hp;
    assert(head.current_intent.type == ATTACK_BUFF && head.intent_damage(combat.player) == 17);
    combat.end_player_turn();
    assert(head.get_status(StatusType::STRENGTH) == 2);
    std::cout << "[PASS] Globe Head: Ascension gives 158 HP, Slap 14, Thunder Strike 7x3, Burst 17" << std::endl;
}

void test_guardbot() {
    // Guard 15 Block, every turn. Its HP are drawn between 16 and 20 at the start of the fight.
    CombatEngine probe = make_combat(CharacterId::IRONCLAD, EnemyType::GUARDBOT);
    const Enemy& bot = probe.enemies[0];
    assert(bot.hp >= 16 && bot.hp <= 20 && bot.max_hp == 16 + bot.hp_bonus && bot.hp_spread() == 4);
    check_monster(EnemyType::GUARDBOT, "Gardebot", bot.hp, {
        { DEFEND, 0, 15, 0 },
        { DEFEND, 0, 15, 0 },
        { DEFEND, 0, 15, 0 },
    });
    std::cout << "[PASS] Guardbot: 16 HP, Guard 15 Block every turn" << std::endl;
}

void test_guardbot_ascension() {
    // 17 to 21 HP at Ascension; the move has no damage and keeps its 15 Block.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::GUARDBOT);
        Enemy& bot = combat.enemies[0];
        bot.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = bot.hp;
        assert(bot.ascension_hp() == 17 && bot.hp_spread() == 4);
        bot.ascend(true, true);
        combat.plan_enemies();
        assert(bot.hp == base + 1 && bot.max_hp == base + 1 && bot.hp >= 17 && bot.hp <= 21);
        assert(bot.current_intent.type == DEFEND && bot.current_intent.block == 15);
    }
    std::cout << "[PASS] Guardbot: Ascension gives 17 to 21 HP, Guard 15 Block" << std::endl;
}

void test_bowlbug_nectar() {
    // Thrash 3, Buff (+15 Strength), then Thrash again for good: 3 + 15 Strength = 18 damage.
    check_monster(EnemyType::BOWLBUG_NECTAR, "Sphéropode (Nectar)", 35, {
        { ATTACK, 3, 0, 0 },
        { BUFF, 0, 0, 15 },
        { ATTACK, 18, 0, 15 },
        { ATTACK, 18, 0, 15 },
        { ATTACK, 18, 0, 15 },
    });
    std::cout << "[PASS] Bowlbug (Nectar): 35 HP, Thrash 3, Buff +15 Strength, then Thrash 18 on loop" << std::endl;
}

void test_bowlbug_nectar_ascension() {
    // 36 HP and +16 Strength at Ascension; Thrash stays at 3 (19 once buffed).
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::BOWLBUG_NECTAR);
    Enemy& bug = combat.enemies[0];
    bug.ascend(true, true);
    combat.plan_enemies();
    assert(bug.hp == 36 && bug.max_hp == 36);
    assert(bug.current_intent.type == ATTACK && bug.intent_damage(combat.player) == 3);
    combat.end_player_turn();
    combat.player.hp = combat.player.max_hp;
    assert(bug.current_intent.type == EnemyIntentType::BUFF);
    combat.end_player_turn();
    assert(bug.get_status(StatusType::STRENGTH) == 16);
    combat.player.hp = combat.player.max_hp;
    assert(bug.current_intent.type == ATTACK && bug.intent_damage(combat.player) == 19);
    std::cout << "[PASS] Bowlbug (Nectar): Ascension gives 36 HP and +16 Strength" << std::endl;
}

void test_devoted_sculptor() {
    // Forbidden Incantation (Ritual 9) once, then Savage 12 for good: Ritual adds its Strength every turn.
    check_monster(EnemyType::DEVOTED_SCULPTOR, "Sculpteur dévoué", 162, {
        { BUFF, 0, 0, 0, 9 },
        { ATTACK, 12, 0, 9, 9 },
        { ATTACK, 21, 0, 18, 9 },
        { ATTACK, 30, 0, 27, 9 },
    });
    std::cout << "[PASS] Devoted Sculptor: 162 HP, Forbidden Incantation Ritual 9, then Savage 12 (+9 each turn)" << std::endl;
}

void test_devoted_sculptor_ascension() {
    // 172 HP and Savage 15 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::DEVOTED_SCULPTOR);
    Enemy& sculptor = combat.enemies[0];
    sculptor.ascend(true, true);
    combat.plan_enemies();
    assert(sculptor.hp == 172 && sculptor.max_hp == 172);
    combat.end_player_turn(); // Forbidden Incantation
    combat.player.hp = combat.player.max_hp;
    assert(sculptor.current_intent.type == ATTACK && sculptor.intent_damage(combat.player) == 15);
    std::cout << "[PASS] Devoted Sculptor: Ascension gives 172 HP, Savage 15" << std::endl;
}

void test_axebot() {
    // 70 to 78 HP. Boot Up 10 Block and 1 Strength, then Hammer Uppercut (14, 2 Frail, 2 Weak) and One-Two (10 x2)
    // in turn, each hit 1 stronger once the Strength is up.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::AXEBOT);
    Enemy& bot = combat.enemies[0];
    assert(bot.type == EnemyType::AXEBOT && bot.name == "Hachebots");
    assert(bot.hp >= 70 && bot.hp <= 78 && bot.max_hp == bot.hp && bot.max_hp == 70 + bot.hp_bonus);
    const int16_t hp = bot.hp;
    assert(bot.block == 0 && bot.statuses.empty());
    assert(bot.current_intent.type == BUFF && bot.current_intent.block == 10);
    combat.end_player_turn();
    assert(bot.block == 10 && bot.get_status(StatusType::STRENGTH) == 1);
    for (int cycle = 0; cycle < 2; ++cycle) {
        combat.player.hp = combat.player.max_hp;
        assert(bot.current_intent.type == ATTACK_DEBUFF && bot.intent_damage(combat.player) == 15);
        assert(bot.current_intent.debuffs[0].type == StatusType::FRAIL && bot.current_intent.debuffs[0].amount == 2);
        assert(bot.current_intent.debuffs[1].type == StatusType::WEAK && bot.current_intent.debuffs[1].amount == 2);
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 15);
        assert(combat.player.get_status(StatusType::FRAIL) >= 2 && combat.player.get_status(StatusType::WEAK) >= 2);
        combat.player.hp = combat.player.max_hp;
        assert(bot.current_intent.type == ATTACK && bot.current_intent.hits == 2);
        assert(bot.intent_damage(combat.player) == 11);
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 22);
        assert(bot.get_status(StatusType::STRENGTH) == 1 && bot.hp == hp);
    }
    std::cout << "[PASS] Axebot: 70 to 78 HP, Boot Up, Hammer Uppercut, One-Two, loop" << std::endl;
}

void test_axebot_ascension() {
    // 76 to 84 HP (70 to 78 plus 6), Hammer Uppercut 18 and One-Two 11x2 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::AXEBOT);
        Enemy& bot = combat.enemies[0];
        bot.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = bot.hp;
        assert(bot.ascension_hp() == 76 && bot.hp_spread() == 8);
        assert(base >= 70 && base <= 78 && bot.max_hp == 70 + bot.hp_bonus);
        bot.ascend(true, true);
        combat.plan_enemies();
        assert(bot.hp == base + 6 && bot.max_hp == base + 6);
        combat.end_player_turn(); // Boot Up
        combat.player.hp = combat.player.max_hp;
        assert(bot.current_intent.type == ATTACK_DEBUFF && bot.intent_damage(combat.player) == 19);
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 19);
        combat.player.hp = combat.player.max_hp;
        assert(bot.current_intent.type == ATTACK && bot.current_intent.hits == 2 && bot.intent_damage(combat.player) == 12);
    }
    std::cout << "[PASS] Axebot: Ascension gives 76 to 84 HP, Hammer Uppercut 18, One-Two 11x2" << std::endl;
}

void test_turret_operator() {
    // Unload 3x5 twice, Reload (+1 Strength), and again: the Strength makes each hit stronger.
    check_monster(EnemyType::TURRET_OPERATOR, "Opérateur de tourelle", 41, {
        { ATTACK, 3, 0, 0, 0, 5 },
        { ATTACK, 3, 0, 0, 0, 5 },
        { BUFF, 0, 0, 1 },
        { ATTACK, 4, 0, 1, 0, 5 },
        { ATTACK, 4, 0, 1, 0, 5 },
        { BUFF, 0, 0, 2 },
        { ATTACK, 5, 0, 2, 0, 5 },
    });
    std::cout << "[PASS] Turret Operator: 41 HP, Unload 3x5 twice, Reload +1 Strength, loop" << std::endl;
}

void test_turret_operator_ascension() {
    // 51 HP and Unload 4x5 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TURRET_OPERATOR);
    Enemy& operator_ = combat.enemies[0];
    operator_.ascend(true, true);
    combat.plan_enemies();
    assert(operator_.hp == 51 && operator_.max_hp == 51);
    assert(operator_.current_intent.type == ATTACK && operator_.current_intent.hits == 5);
    assert(operator_.intent_damage(combat.player) == 4);
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 20);
    std::cout << "[PASS] Turret Operator: Ascension gives 51 HP, Unload 4x5" << std::endl;
}

void test_bowlbug_silk() {
    // Toxic Spit (1 Weak), Thrash 4x2, and again.
    check_monster(EnemyType::BOWLBUG_SILK, "Sphéropode (Soie)", 40, {
        { DEBUFF, 0, 0, 0 },
        { ATTACK, 4, 0, 0, 0, 2 },
        { DEBUFF, 0, 0, 0 },
        { ATTACK, 4, 0, 0, 0, 2 },
    });
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::BOWLBUG_SILK);
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::WEAK) == 1);
    std::cout << "[PASS] Bowlbug Silk: 40 HP, Toxic Spit 1 Weak, Thrash 4x2, loop" << std::endl;
}

void test_bowlbug_silk_ascension() {
    // 41 HP, Thrash 5x2 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::BOWLBUG_SILK);
    Enemy& bug = combat.enemies[0];
    bug.ascend(true, true);
    combat.plan_enemies();
    assert(bug.hp == 41 && bug.max_hp == 41);
    combat.end_player_turn(); // Toxic Spit
    combat.player.hp = combat.player.max_hp;
    assert(bug.current_intent.type == ATTACK && bug.current_intent.hits == 2);
    assert(bug.intent_damage(combat.player) == 5);
    std::cout << "[PASS] Bowlbug Silk: Ascension gives 41 HP, Thrash 5x2" << std::endl;
}

void test_bowlbug_rock() {
    // Headbutt 15 every turn while it hurts.
    check_monster(EnemyType::BOWLBUG_ROCK, "Sphéropode (Pierre)", 45, {
        { ATTACK, 15, 0, 0 },
        { ATTACK, 15, 0, 0 },
        { ATTACK, 15, 0, 0 },
    });
    // A Headbutt the Block takes in full throws it off balance: Dizzy next turn, then Headbutt again.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::BOWLBUG_ROCK);
    Enemy& bug = combat.enemies[0];
    combat.player.block = 20;
    combat.end_player_turn();
    assert(combat.player.hp == combat.player.max_hp);
    assert(bug.current_intent.type == STUN);
    combat.end_player_turn(); // Dizzy: no damage
    assert(combat.player.hp == combat.player.max_hp);
    assert(bug.current_intent.type == ATTACK && bug.intent_damage(combat.player) == 15);
    combat.end_player_turn(); // Headbutt again, unblocked: no stun
    assert(combat.player.hp == combat.player.max_hp - 15);
    assert(bug.current_intent.type == ATTACK);
    std::cout << "[PASS] Rock Bowlbug: 45 HP, Headbutt 15, off balance when blocked then Dizzy" << std::endl;
}

void test_bowlbug_rock_ascension() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::BOWLBUG_ROCK);
    Enemy& bug = combat.enemies[0];
    bug.ascend(true, true);
    combat.plan_enemies();
    assert(bug.hp == 46 && bug.max_hp == 46);
    assert(bug.current_intent.type == ATTACK && bug.intent_damage(combat.player) == 16);
    std::cout << "[PASS] Rock Bowlbug: Ascension gives 46 HP, Headbutt 16" << std::endl;
}

void test_bowlbug_egg() {
    // Bite (7 damage, 7 Block) every turn.
    check_monster(EnemyType::BOWLBUG_EGG, "Sphéropode (Œuf)", 21, {
        { ATTACK_DEFEND, 7, 7, 0 },
        { ATTACK_DEFEND, 7, 7, 0 },
        { ATTACK_DEFEND, 7, 7, 0 },
    });
    std::cout << "[PASS] Bowlbug Egg: 21 HP, Bite 7 damage + 7 Block every turn" << std::endl;
}

void test_bowlbug_egg_ascension() {
    // 23 to 24 HP (21 to 22 plus 2) and a Bite of 8 at Ascension; the Block stays 7.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::BOWLBUG_EGG);
        Enemy& egg = combat.enemies[0];
        egg.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = egg.hp;
        assert(egg.ascension_hp() == 23 && egg.hp_spread() == 1);
        assert(base >= 21 && base <= 22 && egg.max_hp == 21 + egg.hp_bonus);
        egg.ascend(true, true);
        combat.plan_enemies();
        assert(egg.hp == base + 2 && egg.max_hp == base + 2 && egg.hp >= 23 && egg.hp <= 24);
        assert(egg.current_intent.type == ATTACK_DEFEND && egg.intent_damage(combat.player) == 8);
        combat.end_player_turn();
        assert(egg.block == 7);
    }
    std::cout << "[PASS] Bowlbug Egg: Ascension gives 23 to 24 HP and a Bite of 8" << std::endl;
}

void test_skulking_colony() {
    // Zoom 14 twice, Inertia 9 (+2 Strength), Piercing Stabs 7x2, and again with the Strength it has gained.
    check_monster(EnemyType::SKULKING_COLONY, "Colonie de rôdeurs", 75, {
        { ATTACK, 14, 0, 0 },
        { ATTACK, 14, 0, 0 },
        { ATTACK_BUFF, 9, 0, 2 },
        { ATTACK, 9, 0, 2, 0, 2 },
        { ATTACK, 16, 0, 2 },
        { ATTACK, 16, 0, 2 },
        { ATTACK_BUFF, 11, 0, 4 },
        { ATTACK, 11, 0, 4, 0, 2 },
    });
    std::cout << "[PASS] Skulking Colony: 75 HP, Zoom 14 x2, Inertia 9 (+2 Strength), Piercing Stabs 7x2, loop" << std::endl;
}

void test_skulking_colony_ascension() {
    // 80 HP, Zoom 16, Inertia 11 (+4 Strength) and Piercing Stabs 8x2 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SKULKING_COLONY);
    Enemy& colony = combat.enemies[0];
    colony.ascend(true, true);
    combat.plan_enemies();
    assert(colony.hp == 80 && colony.max_hp == 80);
    assert(colony.current_intent.type == ATTACK && colony.intent_damage(combat.player) == 16);
    combat.end_player_turn();
    combat.player.hp = combat.player.max_hp;
    assert(colony.current_intent.type == ATTACK && colony.intent_damage(combat.player) == 16);
    combat.end_player_turn();
    combat.player.hp = combat.player.max_hp;
    assert(colony.current_intent.type == ATTACK_BUFF && colony.intent_damage(combat.player) == 11);
    combat.end_player_turn();
    assert(colony.get_status(StatusType::STRENGTH) == 4);
    combat.player.hp = combat.player.max_hp;
    assert(colony.current_intent.type == ATTACK && colony.current_intent.hits == 2);
    assert(colony.intent_damage(combat.player) == 12);
    std::cout << "[PASS] Skulking Colony: Ascension gives 80 HP, Zoom 16, Inertia 11 (+4 Strength), Stabs 8x2" << std::endl;
}

void test_terror_eel() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TERROR_EEL);
    Enemy& eel = combat.enemies[0];
    assert(eel.name == "Murène de l'effroi" && eel.hp == 140 && eel.max_hp == 140);
    // Crash 16, Thrash 3x3 (+6 Vigor), Crash 16 + 6 Vigor, Thrash, and again, while above half its HP.
    struct Turn { EnemyIntentType intent; int damage; int hits; int vigor; };
    int turn_number = 1;
    for (const Turn& turn : { Turn{ ATTACK, 16, 1, 0 }, Turn{ ATTACK_BUFF, 3, 3, 6 }, Turn{ ATTACK, 22, 1, 0 },
                              Turn{ ATTACK_BUFF, 3, 3, 6 }, Turn{ ATTACK, 22, 1, 0 } }) {
        assert(combat.turn_number == turn_number++);
        assert(eel.current_intent.type == turn.intent && eel.current_intent.hits == turn.hits);
        assert(eel.intent_damage(combat.player) == turn.damage || turn.vigor == 0);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == turn.damage * turn.hits);
        assert(eel.get_status(StatusType::VIGOR) == turn.vigor);
        assert(combat.player.get_status(StatusType::VULNERABLE) == 0);
    }
    // At half its HP or less it is stunned, casts Terror (Vulnerable 99), then Crash and Thrash again.
    eel.hp = 70;
    combat.plan_enemies();
    assert(eel.current_intent.type == STUN);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp == combat.player.hp && combat.player.get_status(StatusType::VULNERABLE) == 0);
    assert(eel.current_intent.type == DEBUFF && eel.current_intent.debuffs_player());
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::VULNERABLE) == 99);
    assert(eel.current_intent.type == ATTACK);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 16 * 3 / 2);
    assert(eel.current_intent.type == ATTACK_BUFF && eel.current_intent.hits == 3);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 3 * (3 * 3 / 2));
    assert(eel.get_status(StatusType::VIGOR) == 6 && eel.current_intent.type == ATTACK);
    std::cout << "[PASS] Terror Eel: 140 HP, Crash 16, Thrash 3x3 +6 Vigor, stun and Terror at half HP" << std::endl;
}

void test_terror_eel_ascension() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TERROR_EEL);
    Enemy& eel = combat.enemies[0];
    eel.ascend(true, true);
    combat.plan_enemies();
    assert(eel.hp == 150 && eel.max_hp == 150);
    assert(eel.current_intent.type == ATTACK && eel.intent_damage(combat.player) == 18);
    combat.end_player_turn();
    assert(eel.current_intent.type == ATTACK_BUFF && eel.current_intent.hits == 3);
    assert(eel.intent_damage(combat.player) == 4);
    std::cout << "[PASS] Terror Eel: Ascension gives 150 HP, Crash 18, Thrash 4x3" << std::endl;
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

void test_magi_knight() {
    // Power Shield 6 + 5 Block, Dampen, then Ram 10, Prep 5 Block and Magic Bomb 35 in a loop. The Block fades
    // at the monster's next turn.
    check_monster(EnemyType::MAGI_KNIGHT, "Chevalier mage", 82, {
        { ATTACK_DEFEND, 6, 5, 0 },
        { DEBUFF, 0, 0, 0 },
        { ATTACK, 10, 0, 0 },
        { DEFEND, 0, 5, 0 },
        { ATTACK, 35, 0, 0 },
        { ATTACK, 10, 0, 0 },
        { DEFEND, 0, 5, 0 },
        { ATTACK, 35, 0, 0 },
    });
    std::cout << "[PASS] Magi Knight: 82 HP, Power Shield, Dampen, then Ram, Prep, Magic Bomb in a loop" << std::endl;
}

void test_magi_knight_ascension() {
    // 89 HP, Power Shield 7, Ram 11 and Magic Bomb 40 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::MAGI_KNIGHT);
    Enemy& knight = combat.enemies[0];
    knight.ascend(true, true);
    combat.plan_enemies();
    assert(knight.hp == 89 && knight.max_hp == 89);
    assert(knight.intent_damage(combat.player) == 7);
    combat.end_player_turn(); // Power Shield
    combat.end_player_turn(); // Dampen
    assert(knight.current_intent.type == ATTACK && knight.intent_damage(combat.player) == 11);
    combat.end_player_turn(); // Ram
    combat.end_player_turn(); // Prep
    assert(knight.current_intent.type == ATTACK && knight.intent_damage(combat.player) == 40);
    std::cout << "[PASS] Magi Knight: Ascension gives 89 HP, Power Shield 7, Ram 11, Magic Bomb 40" << std::endl;
}

void test_bygone_effigy() {
    // Sleep (a lost turn), Wake (+10 Strength), then Slashes 13 every turn, 10 stronger.
    check_monster(EnemyType::BYGONE_EFFIGY, "Idole Oubliée", 127, {
        { STUN, 0, 0, 0 },
        { BUFF, 0, 0, 10 },
        { ATTACK, 23, 0, 10 },
        { ATTACK, 23, 0, 10 },
        { ATTACK, 23, 0, 10 },
    });

    // Hurt while it sleeps: it skips the Wake, loses one more turn, then Slashes without the Strength.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::BYGONE_EFFIGY);
    Enemy& effigy = combat.enemies[0];
    combat.damage_enemy(effigy, 5);
    combat.end_player_turn(); // Sleep, already planned
    assert(effigy.phase == 1 && effigy.current_intent.type == STUN);
    combat.end_player_turn(); // Sleep again
    for (int turn = 0; turn < 3; ++turn) {
        combat.player.hp = combat.player.max_hp;
        assert(effigy.current_intent.type == ATTACK && effigy.intent_damage(combat.player) == 13);
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 13 && effigy.get_status(StatusType::STRENGTH) == 0);
    }
    std::cout << "[PASS] Bygone Effigy: 127 HP, Sleep, Wake +10 Strength, Slashes 13, Sleep again once hurt" << std::endl;
}

void test_bygone_effigy_ascension() {
    // 132 HP and Slashes 15 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::BYGONE_EFFIGY);
    Enemy& effigy = combat.enemies[0];
    assert(effigy.ascension_hp() == 132);
    effigy.ascend(true, true);
    combat.plan_enemies();
    assert(effigy.hp == 132 && effigy.max_hp == 132);
    combat.end_player_turn(); // Sleep
    combat.end_player_turn(); // Wake
    combat.player.hp = combat.player.max_hp;
    assert(effigy.current_intent.type == ATTACK && effigy.intent_damage(combat.player) == 25);
    std::cout << "[PASS] Bygone Effigy: Ascension gives 132 HP and Slashes 15" << std::endl;
}

void test_byrdonis() {
    // Swoop 17 on the odd turns, Peck 3 x3 on the even ones.
    check_monster(EnemyType::BYRDONIS, "Oizodon", 81, {
        { ATTACK, 17, 0, 0 },
        { ATTACK, 3, 0, 0, 0, 3 },
        { ATTACK, 17, 0, 0 },
        { ATTACK, 3, 0, 0, 0, 3 },
        { ATTACK, 17, 0, 0 },
    });
    std::cout << "[PASS] Byrdonis: 81 HP, Swoop 17 then Peck 3x3" << std::endl;
}

void test_byrdonis_ascension() {
    // 81 to 84 HP, exactly 90 at Ascension; Swoop 19 and Peck 4 x3.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::BYRDONIS);
        Enemy& bird = combat.enemies[0];
        bird.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        assert(bird.hp >= 81 && bird.hp <= 84 && bird.hp_spread() == 3);
        bird.ascend(true, true);
        combat.plan_enemies();
        assert(bird.hp == 90 && bird.max_hp == 90);
        assert(bird.current_intent.type == ATTACK && bird.intent_damage(combat.player) == 19);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(bird.current_intent.type == ATTACK && bird.current_intent.hits == 3 &&
               bird.intent_damage(combat.player) == 4);
    }
    std::cout << "[PASS] Byrdonis: Ascension gives 90 HP, Swoop 19 and Peck 4x3" << std::endl;
}

// Frantic Escapes in every pile of the player.
int frantic_escapes(const CombatEngine& combat) {
    int count = count_in(combat.draw_pile, CardId::FRANTIC_ESCAPE) + count_in(combat.discard_pile, CardId::FRANTIC_ESCAPE);
    for (const DeckCard& card : combat.hand) count += card.id == CardId::FRANTIC_ESCAPE;
    return count;
}

void test_the_insatiable() {
    // Liquify Ground, Thrash (8x2), Lunging Bite (28), Salivate (+2 Strength), then Thrash again.
    check_monster(EnemyType::THE_INSATIABLE, "L'Insatiable", 321, {
        { STATUS, 0, 0, 0 },
        { ATTACK, 8, 0, 0, 0, 2 },
        { ATTACK, 28, 0, 0 },
        { BUFF, 0, 0, 2 },
    });

    // Liquify Ground: the Sandpit (4) held by the boss, and 6 Frantic Escapes shuffled into the draw pile.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::THE_INSATIABLE);
    Enemy& worm = combat.enemies[0];
    assert(worm.current_intent.power == PowerId::SANDPIT && !worm.current_intent.power_on_player);
    assert(worm.current_intent.status_card == CardId::FRANTIC_ESCAPE && worm.current_intent.status_pile == Pile::DRAW);
    combat.end_player_turn();
    assert(combat.enemy_power_amount(0, PowerId::SANDPIT) == INSATIABLE_SANDPIT);
    assert(combat.last_enemy_actions[0].cards_added == CardId::FRANTIC_ESCAPE &&
           combat.last_enemy_actions[0].cards_added_count == INSATIABLE_FRANTIC_ESCAPES);
    assert(frantic_escapes(combat) == INSATIABLE_FRANTIC_ESCAPES);

    // The whole cycle after it, twice: Thrash, Lunging Bite, Salivate, Thrash. Each Salivate adds 2 Strength.
    for (int cycle = 0; cycle < 2; ++cycle) {
        const int strength = 2 * cycle;
        const int moves[4][3] = { { 8, 2, 0 }, { 28, 1, 0 }, { 0, 1, 2 }, { 8, 2, 2 } };
        for (const auto& move : moves) {
            const bool attack = move[0] > 0;
            assert(worm.current_intent.type == (attack ? ATTACK : BUFF));
            assert(worm.intent_damage(combat.player) == (attack ? move[0] + strength + (move[2] ? 2 : 0) : 0));
            assert(worm.current_intent.hits == move[1]);
            // The player escapes once a turn, so that the Sandpit never runs out.
            const int16_t sandpit = combat.enemy_power_amount(0, PowerId::SANDPIT);
            assert(play_alone(combat, DeckCard(CardId::FRANTIC_ESCAPE)));
            assert(combat.enemy_power_amount(0, PowerId::SANDPIT) == sandpit + 1);
            combat.player.hp = combat.player.max_hp;
            combat.player.block = 0;
            combat.end_player_turn();
            assert(combat.state == CombatState::PLAYER_TURN);
            assert(combat.enemy_power_amount(0, PowerId::SANDPIT) == sandpit); // +1, then -1 on the boss's turn
            assert(combat.player.max_hp - combat.player.hp == (attack ? (move[0] + strength + (move[2] ? 2 : 0)) * move[1] : 0));
            assert(worm.get_status(StatusType::STRENGTH) == strength + move[2]);
        }
    }
    std::cout << "[PASS] The Insatiable: 321 HP, Liquify Ground (Sandpit 4, 6 Frantic Escapes), Thrash 8x2, "
                 "Lunging Bite 28, Salivate +2 Strength, loop" << std::endl;
}

void test_the_insatiable_ascension() {
    // 341 HP, Thrash 9x2, Lunging Bite 31 and Salivate +3 Strength at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::THE_INSATIABLE);
    Enemy& worm = combat.enemies[0];
    assert(worm.ascension_hp() == 341 && worm.hp_spread() == 0);
    worm.ascend(true, true);
    combat.plan_enemies();
    assert(worm.hp == 341 && worm.max_hp == 341);
    assert(worm.current_intent.type == STATUS);
    combat.end_player_turn();
    assert(worm.current_intent.type == ATTACK && worm.intent_damage(combat.player) == 9 && worm.current_intent.hits == 2);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(worm.current_intent.type == ATTACK && worm.intent_damage(combat.player) == 31);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(worm.current_intent.type == BUFF && worm.current_intent.status_amount == 3);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(worm.get_status(StatusType::STRENGTH) == 3);
    assert(worm.current_intent.type == ATTACK && worm.intent_damage(combat.player) == 12 && worm.current_intent.hits == 2);
    std::cout << "[PASS] The Insatiable: Ascension gives 341 HP, Thrash 9x2, Lunging Bite 31, Salivate +3" << std::endl;
}

void test_sandpit() {
    // Without an escape, the Sandpit loses 1 on each of the boss's turns: at 0 the player is devoured, full HP
    // and Block notwithstanding, before the boss moves.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::THE_INSATIABLE);
    combat.end_player_turn(); // Liquify Ground
    for (int turn = 1; turn < INSATIABLE_SANDPIT; ++turn) {
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.enemy_power_amount(0, PowerId::SANDPIT) == INSATIABLE_SANDPIT - turn);
    }
    combat.player.hp = combat.player.max_hp;
    combat.player.block = 999;
    combat.end_player_turn();
    assert(combat.state == CombatState::DEFEAT && combat.player.hp == 0);

    // Frantic Escape: +1 Sandpit, and that copy costs 1 more each time it is played.
    CombatEngine escape = make_combat(CharacterId::IRONCLAD, EnemyType::THE_INSATIABLE);
    escape.end_player_turn();
    DeckCard card(CardId::FRANTIC_ESCAPE);
    assert(escape.effective_cost(card) == 1);
    escape.hand.clear();
    escape.hand.push_back(card);
    escape.player.energy = 3;
    assert(escape.play_card(0, 0));
    assert(escape.player.energy == 2 && escape.enemy_power_amount(0, PowerId::SANDPIT) == INSATIABLE_SANDPIT + 1);
    // The copy played went to the discard pile, now costing 2.
    const DeckCard& played = escape.discard_pile[escape.discard_pile.size() - 1];
    assert(played.id == CardId::FRANTIC_ESCAPE && escape.effective_cost(played) == 2);
    escape.hand.push_back(played);
    escape.discard_pile.erase(escape.discard_pile.size() - 1);
    assert(escape.play_card(0, 0));
    assert(escape.player.energy == 0 && escape.enemy_power_amount(0, PowerId::SANDPIT) == INSATIABLE_SANDPIT + 2);
    assert(escape.effective_cost(escape.discard_pile[escape.discard_pile.size() - 1]) == 3);

    // The Sandpit dies with its holder: nothing devours the player of a won fight.
    escape.damage_enemy(escape.enemies[0], 999);
    assert(!escape.enemies[0].is_alive());
    assert(escape.enemy_power_amount(0, PowerId::SANDPIT) == 0);
    std::cout << "[PASS] Sandpit: -1 each boss turn, devoured at 0; Frantic Escape +1 and costs 1 more" << std::endl;
}

void test_the_insatiable_whole_fight() {
    // The whole fight, until the boss dies: the player escapes whenever a Frantic Escape it can pay is in hand and
    // deals 40 damage a turn. A single phase: Liquify Ground once, then the four moves in a loop to the end.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::THE_INSATIABLE);
    Enemy& worm = combat.enemies[0];
    int escapes = 0;
    int turn = 1;
    while (combat.state == CombatState::PLAYER_TURN && worm.is_alive()) {
        // The move announced this turn.
        if (turn == 1) {
            assert(worm.current_intent.power == PowerId::SANDPIT);
        } else {
            const int step = (turn - 2) % 4;
            assert(worm.current_intent.power == PowerId::NONE);
            if (step == 0 || step == 3) assert(worm.current_intent.base_damage == 8 && worm.current_intent.hits == 2);
            if (step == 1) assert(worm.current_intent.base_damage == 28 && worm.current_intent.hits == 1);
            if (step == 2) assert(worm.current_intent.type == BUFF && worm.current_intent.status_amount == 2);
        }
        for (size_t i = combat.hand.size(); i-- > 0;) {
            if (i < combat.hand.size() && combat.hand[i].id == CardId::FRANTIC_ESCAPE &&
                combat.effective_cost(combat.hand[i]) <= combat.player.energy && combat.play_card(static_cast<int>(i), 0)) {
                ++escapes;
            }
        }
        combat.damage_enemy(worm, 40);
        if (!worm.is_alive()) break;
        combat.player.hp = combat.player.max_hp;
        const int16_t sandpit = combat.enemy_power_amount(0, PowerId::SANDPIT);
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN); // never devoured
        if (turn > 1) assert(combat.enemy_power_amount(0, PowerId::SANDPIT) == sandpit - 1);
        ++turn;
    }
    assert(!worm.is_alive() && combat.player.is_alive() && escapes > 0);
    assert(turn == 9); // 321 HP, 40 a turn
    assert(combat.enemy_power_amount(0, PowerId::SANDPIT) == 0);
    std::cout << "[PASS] The Insatiable: a whole fight to the boss's death, escaping the Sandpit" << std::endl;
}

void test_lagavulin_matriarch() {
    // Asleep 3 turns behind 12 Plating (-1 at each of its turns), then awake without it: Slash 19, Disembowel 9x2,
    // Slash 12 with 12 Block, Soul Siphon (+2 Strength), and again 2 stronger.
    check_monster(EnemyType::LAGAVULIN_MATRIARCH, "Matriarche Lagavulin", 222, {
        { STUN, 0, 11, 0, 0, 1, 11 },
        { STUN, 0, 10, 0, 0, 1, 10 },
        { STUN, 0, 9, 0, 0, 1, 0 }, // wakes: the Plating is gone, its last Block stays until its next turn
        { ATTACK, 19, 0, 0 },
        { ATTACK, 9, 0, 0, 0, 2 },
        { ATTACK_DEFEND, 12, 12, 0 },
        { DEBUFF, 0, 0, 2 },
        { ATTACK, 21, 0, 2 },
        { ATTACK, 11, 0, 2, 0, 2 },
        { ATTACK_DEFEND, 14, 12, 2 },
        { DEBUFF, 0, 0, 4 },
    });

    // Soul Siphon takes 2 Strength and 2 Dexterity from the player.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::LAGAVULIN_MATRIARCH);
    Enemy& boss = combat.enemies[0];
    for (int turn = 0; turn < 6; ++turn) {
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
    }
    assert(boss.current_intent.type == DEBUFF && boss.current_intent.debuffs_player());
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::STRENGTH) == -2 && combat.player.get_status(StatusType::DEXTERITY) == -2);
    assert(boss.get_status(StatusType::STRENGTH) == 2);
    std::cout << "[PASS] Lagavulin Matriarch: 222 HP, Asleep 3 turns with 12 Plating, Slash 19, Disembowel 9x2, "
                 "Slash 12 + 12 Block, Soul Siphon" << std::endl;
}

void test_lagavulin_matriarch_woken() {
    // Hurt while it sleeps: the Sleep already announced is lost, then it wakes, drops its Plating and Slashes.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::LAGAVULIN_MATRIARCH);
    Enemy& boss = combat.enemies[0];
    combat.end_player_turn(); // Sleep, untouched: 11 Block from its Plating
    assert(boss.phase == 0 && boss.current_intent.type == STUN && boss.block == 11);
    combat.damage_enemy(boss, 11); // all taken by the Block: still asleep
    combat.end_player_turn();
    assert(boss.phase == 0 && boss.current_intent.type == STUN);
    combat.damage_enemy(boss, 15); // 10 Block, 5 HP
    assert(boss.hp == 217);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // the Sleep announced
    assert(combat.player.hp == combat.player.max_hp);
    assert(boss.phase == 1 && boss.get_status(StatusType::PLATED_ARMOR) == 0);
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 19);
    combat.end_player_turn();
    assert(boss.block == 0 && combat.player.max_hp - combat.player.hp == 19);
    std::cout << "[PASS] Lagavulin Matriarch: woken by HP loss, it drops its Plating and Slashes" << std::endl;
}

void test_lagavulin_matriarch_ascension() {
    // 233 HP, Slash 21, Disembowel 10x2 and Slash 14 with 14 Block at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::LAGAVULIN_MATRIARCH);
    Enemy& boss = combat.enemies[0];
    assert(boss.ascension_hp() == 233 && boss.hp_spread() == 0);
    boss.ascend(true, true);
    combat.plan_enemies();
    assert(boss.hp == 233 && boss.max_hp == 233);
    assert(boss.get_status(StatusType::PLATED_ARMOR) == LAGAVULIN_PLATING);
    for (int turn = 0; turn < LAGAVULIN_SLEEP_TURNS; ++turn) {
        assert(boss.current_intent.type == STUN);
        combat.end_player_turn();
    }
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 21);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 10 && boss.current_intent.hits == 2);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(boss.current_intent.type == ATTACK_DEFEND && boss.intent_damage(combat.player) == 14 &&
           boss.current_intent.block == 14);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(boss.block == 14);
    std::cout << "[PASS] Lagavulin Matriarch: Ascension gives 233 HP, Slash 21, Disembowel 10x2, Slash 14 + 14 Block"
              << std::endl;
}

void test_lagavulin_matriarch_whole_fight() {
    // The whole fight, until the boss dies: the player deals 30 damage a turn from turn 2. Two phases: asleep
    // (turn 1 untouched, turn 2 hurt through its Plating Block, the Sleep announced still lost), then awake from
    // turn 3 to the end, its four moves in a loop.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::LAGAVULIN_MATRIARCH);
    Enemy& boss = combat.enemies[0];
    int turn = 1;
    int woken_on = 0;
    while (combat.state == CombatState::PLAYER_TURN && boss.is_alive()) {
        if (woken_on == 0 && boss.phase == 1) {
            woken_on = turn;
            assert(boss.get_status(StatusType::PLATED_ARMOR) == 0);
        }
        if (boss.phase == 0) {
            assert(boss.current_intent.type == STUN && boss.get_status(StatusType::PLATED_ARMOR) > 0);
        } else {
            const int step = (turn - woken_on) % 4;
            const EnemyIntentType expected[4] = { ATTACK, ATTACK, ATTACK_DEFEND, DEBUFF };
            assert(boss.current_intent.type == expected[step]);
            if (step == 1) assert(boss.current_intent.hits == 2);
        }
        if (turn > 1) combat.damage_enemy(boss, 30);
        if (!boss.is_alive()) break;
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        ++turn;
    }
    assert(!boss.is_alive() && combat.player.is_alive());
    assert(woken_on == 3);
    std::cout << "[PASS] Lagavulin Matriarch: a whole fight to the boss's death, asleep then awake" << std::endl;
}

void test_knowledge_demon() {
    // Curse of Knowledge, Slap 17, Knowledge Overwhelming 8x3, Ponder 11 (+2 Strength, heals 30), and again 2 stronger.
    check_monster(EnemyType::KNOWLEDGE_DEMON, "Démon de la Connaissance", 379, {
        { DEBUFF, 0, 0, 0 },
        { ATTACK, 17, 0, 0 },
        { ATTACK, 8, 0, 0, 0, 3 },
        { ATTACK_BUFF, 11, 0, 2 },
        { DEBUFF, 0, 0, 2 },
        { ATTACK, 19, 0, 2 },
        { ATTACK, 10, 0, 2, 0, 3 },
        { ATTACK_BUFF, 13, 0, 4 },
    });
    std::cout << "[PASS] Knowledge Demon: 379 HP, Curse of Knowledge, Slap 17, Knowledge Overwhelming 8x3, "
                 "Ponder 11 +2 Strength" << std::endl;
}

void test_knowledge_demon_curses() {
    // Each Curse of Knowledge puts its own curse on the player: Mind Rot (1 card fewer), then Waste Away (1 energy
    // less), then Disintegration (6 damage at the end of each turn). Then no more curses.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::KNOWLEDGE_DEMON);
    Enemy& demon = combat.enemies[0];
    const int8_t energy = combat.player.energy;
    const size_t hand = combat.hand.size();
    assert(demon.current_intent.power == PowerId::MIND_ROT && demon.current_intent.power_on_player);
    combat.end_player_turn();
    assert(combat.power_amount(PowerId::MIND_ROT) == KNOWLEDGE_DEMON_MIND_ROT);
    assert(combat.hand.size() == hand - 1 && combat.player.energy == energy);
    for (int turn = 2; turn <= 4; ++turn) {
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
    }
    assert(demon.current_intent.type == DEBUFF && demon.current_intent.power == PowerId::WASTE_AWAY);
    combat.end_player_turn();
    assert(combat.power_amount(PowerId::WASTE_AWAY) == KNOWLEDGE_DEMON_WASTE_AWAY);
    assert(combat.player.energy == energy - 1 && combat.hand.size() == hand - 1);
    for (int turn = 6; turn <= 8; ++turn) {
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
    }
    assert(demon.current_intent.type == DEBUFF && demon.current_intent.power == PowerId::DISINTEGRATION);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.power_amount(PowerId::DISINTEGRATION) == KNOWLEDGE_DEMON_DISINTEGRATION);
    assert(combat.player.hp == combat.player.max_hp);
    // Slap 21 (4 Strength), and the Disintegration first.
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == KNOWLEDGE_DEMON_DISINTEGRATION + 21);
    // Three curses given: the fourth cycle starts with Slap.
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(demon.phase == 1 && demon.current_intent.type == ATTACK && demon.current_intent.power == PowerId::NONE);

    // Artifact cancels a curse.
    CombatEngine guarded = make_combat(CharacterId::IRONCLAD, EnemyType::KNOWLEDGE_DEMON);
    guarded.player.add_status(StatusType::ARTIFACT, 1);
    guarded.end_player_turn();
    assert(!guarded.has_power(PowerId::MIND_ROT) && guarded.player.get_status(StatusType::ARTIFACT) == 0);

    // Ponder heals it by 30, never above its Max HP.
    CombatEngine hurt = make_combat(CharacterId::IRONCLAD, EnemyType::KNOWLEDGE_DEMON);
    Enemy& pondering = hurt.enemies[0];
    for (int turn = 1; turn <= 3; ++turn) {
        hurt.player.hp = hurt.player.max_hp;
        hurt.end_player_turn();
    }
    assert(pondering.current_intent.type == ATTACK_BUFF && pondering.current_intent.heal == KNOWLEDGE_DEMON_PONDER_HEAL);
    hurt.damage_enemy(pondering, 50);
    hurt.player.hp = hurt.player.max_hp;
    hurt.end_player_turn();
    assert(pondering.hp == 379 - 50 + 30 && pondering.get_status(StatusType::STRENGTH) == 2);
    std::cout << "[PASS] Knowledge Demon: Mind Rot, Waste Away, Disintegration, then no more curses; Ponder heals 30"
              << std::endl;
}

void test_knowledge_demon_ascension() {
    // 399 HP, Slap 18, Knowledge Overwhelming 9x3 and Ponder 13 with +3 Strength at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::KNOWLEDGE_DEMON);
    Enemy& demon = combat.enemies[0];
    assert(demon.ascension_hp() == 399 && demon.hp_spread() == 0);
    demon.ascend(true, true);
    combat.plan_enemies();
    assert(demon.hp == 399 && demon.max_hp == 399);
    assert(demon.current_intent.type == DEBUFF);
    combat.end_player_turn();
    assert(demon.current_intent.type == ATTACK && demon.intent_damage(combat.player) == 18);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(demon.current_intent.type == ATTACK && demon.intent_damage(combat.player) == 9 && demon.current_intent.hits == 3);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(demon.current_intent.type == ATTACK_BUFF && demon.intent_damage(combat.player) == 13);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(demon.get_status(StatusType::STRENGTH) == KNOWLEDGE_DEMON_PONDER_STRENGTH_ASC);
    std::cout << "[PASS] Knowledge Demon: Ascension gives 399 HP, Slap 18, Knowledge Overwhelming 9x3, Ponder 13 +3 "
                 "Strength" << std::endl;
}

void test_knowledge_demon_whole_fight() {
    // The whole fight, until the boss dies: the player deals 30 damage a turn. Two phases: three cycles opened by
    // Curse of Knowledge (turns 1 to 12), then Slap, Knowledge Overwhelming and Ponder in a loop from turn 13.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::KNOWLEDGE_DEMON);
    Enemy& demon = combat.enemies[0];
    int turn = 1;
    int phase_two_on = 0;
    while (combat.state == CombatState::PLAYER_TURN && demon.is_alive()) {
        if (phase_two_on == 0 && demon.phase == 1) phase_two_on = turn;
        const EnemyIntent& intent = demon.current_intent;
        if (demon.phase == 0) {
            const EnemyIntentType expected[4] = { DEBUFF, ATTACK, ATTACK, ATTACK_BUFF };
            assert(intent.type == expected[(turn - 1) % 4]);
            if ((turn - 1) % 4 == 0) assert(intent.power_on_player && intent.power != PowerId::NONE);
        } else {
            const EnemyIntentType expected[3] = { ATTACK, ATTACK, ATTACK_BUFF };
            assert(intent.type == expected[(turn - phase_two_on) % 3] && intent.power == PowerId::NONE);
        }
        combat.damage_enemy(demon, 30);
        if (!demon.is_alive()) break;
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        ++turn;
    }
    assert(!demon.is_alive() && combat.player.is_alive());
    assert(phase_two_on == 13);
    assert(turn == 17); // 379 HP, 30 a turn, healed 30 by each of the 4 Ponders played before
    assert(combat.has_power(PowerId::MIND_ROT) && combat.has_power(PowerId::WASTE_AWAY) &&
           combat.has_power(PowerId::DISINTEGRATION));
    std::cout << "[PASS] Knowledge Demon: a whole fight to the boss's death, three curses then the loop" << std::endl;
}

int cards_anywhere(const CombatEngine& combat, CardId id) {
    return count_cards(combat.draw_pile, id) + count_cards(combat.hand, id) + count_cards(combat.discard_pile, id);
}

void test_test_subject() {
    // First life: Bite 20, Skull Bash 14 + Vulnerable 1, in a loop. The Bite after a Skull Bash hits a Vulnerable
    // player: 30.
    check_monster(EnemyType::TEST_SUBJECT, "Cobaye", TEST_SUBJECT_HP[0], {
        { ATTACK, 20, 0, 0 },
        { ATTACK_DEBUFF, 14, 0, 0 },
        { ATTACK, 30, 0, 0 },
        { ATTACK_DEBUFF, 14, 0, 0 },
        { ATTACK, 30, 0, 0 },
    });
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TEST_SUBJECT);
    combat.end_player_turn();
    assert(combat.enemies[0].current_intent.type == ATTACK_DEBUFF && combat.enemies[0].current_intent.debuffs_player());
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::VULNERABLE) == 1);
    std::cout << "[PASS] Test Subject: 100 HP, Bite 20, Skull Bash 14 + Vulnerable" << std::endl;
}

void test_test_subject_respawn() {
    // Killed in its first life, it respawns: 150 HP, its statuses gone, and Respawn gives it Painful Stabs.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TEST_SUBJECT);
    Enemy& boss = combat.enemies[0];
    boss.add_status(StatusType::VULNERABLE, 2);
    combat.damage_enemy(boss, 500);
    assert(boss.is_alive() && combat.has_alive_enemies() && combat.state == CombatState::PLAYER_TURN);
    assert(boss.phase == 1 && boss.hp == TEST_SUBJECT_HP[1] && boss.max_hp == TEST_SUBJECT_HP[1]);
    assert(boss.statuses.empty() && boss.block == 0);
    assert(boss.current_intent.type == BUFF && boss.current_intent.power == PowerId::PAINFUL_STABS);
    combat.end_player_turn();
    assert(combat.player.hp == combat.player.max_hp);
    assert(combat.enemy_power_amount(0, PowerId::PAINFUL_STABS) == 1);

    // Multi-Claw every turn. Painful Stabs: a Wound into the discard pile for each hit that costs the player HP.
    for (int turn = 0; turn < 3; ++turn) {
        assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 10);
        combat.player.hp = combat.player.max_hp;
        const int wounds = cards_anywhere(combat, CardId::WOUND);
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 10);
        assert(cards_anywhere(combat, CardId::WOUND) == wounds + 1);
    }
    // A hit the Block takes in full gives no Wound.
    combat.player.block = 20;
    combat.player.hp = combat.player.max_hp;
    const int wounds = cards_anywhere(combat, CardId::WOUND);
    combat.end_player_turn();
    assert(combat.player.hp == combat.player.max_hp && cards_anywhere(combat, CardId::WOUND) == wounds);

    // Killed again: 200 HP, and Respawn gives it Nemesis, Intangible one turn out of two.
    combat.damage_enemy(boss, 500);
    assert(boss.is_alive() && boss.phase == 2 && boss.hp == TEST_SUBJECT_HP[2]);
    assert(boss.current_intent.type == BUFF && boss.current_intent.power == PowerId::NEMESIS);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.hp == combat.player.max_hp);
    assert(combat.enemy_power_amount(0, PowerId::NEMESIS) == 1 && combat.enemy_power_amount(0, PowerId::PAINFUL_STABS) == 1);
    assert(boss.get_status(StatusType::INTANGIBLE) == 1);
    combat.damage_enemy(boss, 30);
    assert(boss.hp == TEST_SUBJECT_HP[2] - 1);

    // Lacerate 10x3: three Wounds. Its Intangible wears off.
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 10 && boss.current_intent.hits == 3);
    combat.player.hp = combat.player.max_hp;
    const int before = cards_anywhere(combat, CardId::WOUND);
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 30 && cards_anywhere(combat, CardId::WOUND) == before + 3);
    assert(boss.get_status(StatusType::INTANGIBLE) == 0);
    combat.damage_enemy(boss, 30);
    assert(boss.hp == TEST_SUBJECT_HP[2] - 31);

    // Big Pounce 45, and Intangible again.
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 45);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 45 && boss.get_status(StatusType::INTANGIBLE) == 1);

    // Burning Growl: +2 Strength, 3 Burns into the discard pile.
    assert(boss.current_intent.type == STATUS && boss.current_intent.status_card == CardId::BURN);
    const int burns = cards_anywhere(combat, CardId::BURN);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(boss.get_status(StatusType::STRENGTH) == 2 && cards_anywhere(combat, CardId::BURN) == burns + 3);
    assert(boss.get_status(StatusType::INTANGIBLE) == 0);
    // Then Lacerate again, 2 stronger.
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 12 && boss.current_intent.hits == 3);

    // No life left: this time it dies.
    combat.damage_enemy(boss, 500);
    assert(!boss.is_alive() && boss.phase == 2 && !combat.has_alive_enemies());
    std::cout << "[PASS] Test Subject: respawns twice, Painful Stabs then Nemesis, then dies" << std::endl;
}

void test_test_subject_thorns_respawn() {
    // Killed by Thorns during its own turn: it respawns, and Respawn is its next move.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TEST_SUBJECT);
    Enemy& boss = combat.enemies[0];
    boss.hp = 2;
    combat.player.add_status(StatusType::THORNS, 5);
    combat.end_player_turn();
    assert(combat.state == CombatState::PLAYER_TURN);
    assert(boss.phase == 1 && boss.hp == TEST_SUBJECT_HP[1]);
    assert(boss.current_intent.type == BUFF && boss.current_intent.power == PowerId::PAINFUL_STABS);
    std::cout << "[PASS] Test Subject: killed by Thorns on its turn, it announces Respawn" << std::endl;
}

void test_test_subject_ascension() {
    // Ascension: 110, 165 then 220 HP; Bite 22, Skull Bash 16, Multi-Claw 11, Lacerate 11x3, Big Pounce 45, and
    // Burning Growl +3 Strength with 5 Burns.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TEST_SUBJECT);
    Enemy& boss = combat.enemies[0];
    assert(boss.ascension_hp() == TEST_SUBJECT_HP_ASC[0] && boss.hp_spread() == 0);
    boss.ascend(true, true);
    combat.plan_enemies();
    assert(boss.hp == 110 && boss.max_hp == 110);
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 22);
    combat.end_player_turn();
    assert(boss.current_intent.type == ATTACK_DEBUFF && boss.intent_damage(combat.player) == 16);
    combat.damage_enemy(boss, 500);
    assert(boss.phase == 1 && boss.hp == 165 && boss.max_hp == 165);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // Respawn
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 11);
    combat.damage_enemy(boss, 500);
    assert(boss.phase == 2 && boss.hp == 220 && boss.max_hp == 220);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // Respawn
    combat.player.clear_status(StatusType::VULNERABLE);
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 11 && boss.current_intent.hits == 3);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 45);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(boss.current_intent.type == STATUS && boss.current_intent.status_cards == 5);
    const int burns = cards_anywhere(combat, CardId::BURN);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(boss.get_status(StatusType::STRENGTH) == 3 && cards_anywhere(combat, CardId::BURN) == burns + 5);
    std::cout << "[PASS] Test Subject: Ascension gives 110/165/220 HP, Bite 22, Skull Bash 16, Multi-Claw 11, "
                 "Lacerate 11x3, Growl +3 Strength and 5 Burns" << std::endl;
}

void test_test_subject_whole_fight() {
    // The whole fight, until the boss dies: the player deals 50 damage a turn. Three lives: Bite and Skull Bash
    // (dead on turn 2), Respawn then Multi-Claw (dead on turn 5), Respawn then Lacerate, Big Pounce, Burning Growl,
    // Intangible one turn out of two (dead on turn 13).
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TEST_SUBJECT);
    Enemy& boss = combat.enemies[0];
    int turn = 1;
    int phase_started[3] = { 1, 0, 0 };
    const EnemyIntentType third_life[3] = { ATTACK, ATTACK, STATUS };
    while (combat.state == CombatState::PLAYER_TURN) {
        const int since = turn - phase_started[boss.phase];
        if (boss.phase == 0) {
            assert(boss.current_intent.type == (since % 2 == 0 ? ATTACK : ATTACK_DEBUFF));
        } else if (since > 0) {
            assert(boss.current_intent.type == (boss.phase == 1 ? ATTACK : third_life[(since - 1) % 3]));
        }
        const uint8_t phase = boss.phase;
        combat.damage_enemy(boss, 50);
        if (!boss.is_alive()) break;
        if (boss.phase != phase) {
            // Respawned: full HP of its next life, Respawn announced.
            phase_started[boss.phase] = turn;
            assert(boss.hp == TEST_SUBJECT_HP[boss.phase] && boss.current_intent.type == BUFF);
        }
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN && combat.player.is_alive());
        ++turn;
    }
    assert(!boss.is_alive() && !combat.has_alive_enemies());
    assert(phase_started[1] == 2 && phase_started[2] == 5 && turn == 13);
    assert(cards_anywhere(combat, CardId::WOUND) > 0 && cards_anywhere(combat, CardId::BURN) > 0);
    std::cout << "[PASS] Test Subject: a whole fight to the boss's death, through its three lives" << std::endl;
}

void test_queen() {
    // Alone (as once the Torch Head Amalgam is dead): Puppet Strings, You Are Mine (Vulnerable 99: every hit after it
    // deals 50% more), then Off With Your Head 3x5, Execution 15, Enrage +2 Strength, in a loop.
    check_monster(EnemyType::QUEEN, "Reine", 400, {
        { DEBUFF, 0, 0, 0 },
        { DEBUFF, 0, 0, 0 },
        { ATTACK, 4, 0, 0, 0, 5 },
        { ATTACK, 22, 0, 0 },
        { BUFF, 0, 0, 2 },
        { ATTACK, 7, 0, 2, 0, 5 },
        { ATTACK, 25, 0, 2 },
        { BUFF, 0, 0, 4 },
    });
    std::cout << "[PASS] Queen: 400 HP, Puppet Strings, You Are Mine, then Off With Your Head 3x5, Execution 15, "
                 "Enrage +2 Strength, loop" << std::endl;
}

void test_torch_head_amalgam() {
    // Strong Tackle 26, Tackle 18, then Beam 8x3, Tackle 14, Tackle 14 in a loop.
    check_monster(EnemyType::TORCH_HEAD_AMALGAM, "Amalgame Torchetête", 199, {
        { ATTACK, 26, 0, 0 },
        { ATTACK, 18, 0, 0 },
        { ATTACK, 8, 0, 0, 0, 3 },
        { ATTACK, 14, 0, 0 },
        { ATTACK, 14, 0, 0 },
        { ATTACK, 8, 0, 0, 0, 3 },
        { ATTACK, 14, 0, 0 },
        { ATTACK, 14, 0, 0 },
    });
    std::cout << "[PASS] Torch Head Amalgam: 199 HP, Strong Tackle 26, Tackle 18, then Beam 8x3, Tackle 14, "
                 "Tackle 14, loop" << std::endl;
}

void test_queen_debuffs() {
    // Puppet Strings: 3 Chains of Binding. The first 3 cards of each new hand are Bound (cannot be played this turn),
    // one fewer after each turn. You Are Mine: 99 Weak, Frail and Vulnerable.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::QUEEN);
    Enemy& queen = combat.enemies[0];
    assert(queen.current_intent.power == PowerId::CHAINS_OF_BINDING && queen.current_intent.power_on_player);
    for (size_t i = 0; i < combat.hand.size(); ++i) assert(!combat.hand[i].bound_this_turn);
    combat.end_player_turn();
    assert(combat.power_amount(PowerId::CHAINS_OF_BINDING) == QUEEN_CHAINS_OF_BINDING);
    for (int i = 0; i < 3; ++i) {
        assert(combat.hand[i].bound_this_turn && combat.play_blocker(i) == PlayBlocker::NOT_PLAYABLE);
        assert(!combat.play_card(i, 0));
    }
    assert(!combat.hand[3].bound_this_turn && combat.can_play_card(3));
    assert(queen.current_intent.type == DEBUFF && queen.current_intent.debuffs[2].type == StatusType::VULNERABLE);
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::WEAK) >= 98 && combat.player.get_status(StatusType::FRAIL) >= 98 &&
           combat.player.get_status(StatusType::VULNERABLE) >= 98);
    assert(combat.power_amount(PowerId::CHAINS_OF_BINDING) == 2);
    assert(combat.hand[0].bound_this_turn && combat.hand[1].bound_this_turn && !combat.hand[2].bound_this_turn);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.hand[0].bound_this_turn && !combat.hand[1].bound_this_turn);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    for (size_t i = 0; i < combat.hand.size(); ++i) assert(!combat.hand[i].bound_this_turn && combat.can_play_card(static_cast<int>(i)));

    // Artifact cancels Chains of Binding.
    CombatEngine guarded = make_combat(CharacterId::IRONCLAD, EnemyType::QUEEN);
    guarded.player.add_status(StatusType::ARTIFACT, 1);
    guarded.end_player_turn();
    assert(!guarded.has_power(PowerId::CHAINS_OF_BINDING) && guarded.player.get_status(StatusType::ARTIFACT) == 0);
    for (size_t i = 0; i < guarded.hand.size(); ++i) assert(!guarded.hand[i].bound_this_turn);
    std::cout << "[PASS] Queen: Chains of Binding binds the first cards of the hand, You Are Mine 99 Weak, Frail, "
                 "Vulnerable" << std::endl;
}

void test_queen_ascension() {
    // Ascension: 419 and 211 HP; Off With Your Head 4x5, Execution 18; Strong Tackle 32, Tackle 22, Beam 8x3,
    // Tackle 16.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::QUEEN);
    Enemy& queen = combat.enemies[0];
    assert(queen.ascension_hp() == 419 && queen.hp_spread() == 0);
    queen.ascend(true, true);
    combat.plan_enemies();
    assert(queen.hp == 419 && queen.max_hp == 419);
    combat.end_player_turn();
    combat.end_player_turn();
    combat.player.clear_status(StatusType::VULNERABLE);
    assert(queen.current_intent.type == ATTACK && queen.intent_damage(combat.player) == 4 && queen.current_intent.hits == 5);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    combat.player.clear_status(StatusType::VULNERABLE);
    assert(queen.current_intent.type == ATTACK && queen.intent_damage(combat.player) == 18);

    CombatEngine torch = make_combat(CharacterId::IRONCLAD, EnemyType::TORCH_HEAD_AMALGAM);
    Enemy& amalgam = torch.enemies[0];
    assert(amalgam.ascension_hp() == 211 && amalgam.hp_spread() == 0);
    amalgam.ascend(true, true);
    torch.plan_enemies();
    assert(amalgam.hp == 211 && amalgam.max_hp == 211);
    const int damage[] = { 32, 22, 8, 16, 16, 8 };
    for (int expected : damage) {
        assert(amalgam.intent_damage(torch.player) == expected);
        torch.player.hp = torch.player.max_hp;
        torch.end_player_turn();
    }
    std::cout << "[PASS] Queen and Torch Head Amalgam: Ascension gives 419 and 211 HP, Off With Your Head 4x5, "
                 "Execution 18, Strong Tackle 32, Tackle 22, Tackle 16" << std::endl;
}

void test_queen_whole_fight() {
    // The QueenBoss fight, until both are dead: while the Amalgam lives the Queen plays Burn Bright For Me (20 Block,
    // +1 Strength) every turn; once it is dead she plays Off With Your Head, Execution and Enrage.
    CombatEngine combat;
    const EnemyType pack[] = { EnemyType::QUEEN, EnemyType::TORCH_HEAD_AMALGAM };
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    Enemy& queen = combat.enemies[0];
    Enemy& amalgam = combat.enemies[1];
    assert(queen.hp == 400 && amalgam.hp == 199 && queen.phase == 0);
    auto play_turn = [&combat]() {
        combat.player.hp = combat.player.max_hp;
        const int16_t before = combat.player.hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        return before - combat.player.hp;
    };
    // Turn 1: Puppet Strings, Strong Tackle 26. Turn 2: You Are Mine, then Tackle 18 on a Vulnerable player: 27.
    assert(queen.current_intent.type == DEBUFF && amalgam.intent_damage(combat.player) == 26);
    assert(play_turn() == 26 && combat.power_amount(PowerId::CHAINS_OF_BINDING) == 3);
    assert(queen.current_intent.type == DEBUFF && amalgam.intent_damage(combat.player) == 18);
    assert(play_turn() == 27 && combat.player.get_status(StatusType::VULNERABLE) > 0);
    combat.player.clear_status(StatusType::VULNERABLE);
    combat.player.clear_status(StatusType::WEAK);
    combat.player.clear_status(StatusType::FRAIL);
    // Phase 1, the Amalgam alive: Burn Bright For Me every turn, beside Beam 8x3 and Tackle 14.
    assert(queen.phase == 1 && queen.current_intent.type == DEFEND && queen.current_intent.block == QUEEN_BURN_BRIGHT_BLOCK);
    assert(play_turn() == 24 && queen.block == 20 && queen.get_status(StatusType::STRENGTH) == 1);
    assert(queen.phase == 1 && queen.current_intent.type == DEFEND);
    assert(play_turn() == 14 && queen.block == 20 && queen.get_status(StatusType::STRENGTH) == 2);
    // The Amalgam dies: the Queen still plays the move she announced, then enters her last phase.
    combat.damage_enemy(amalgam, 999);
    assert(!amalgam.is_alive() && combat.has_alive_enemies() && queen.current_intent.type == DEFEND);
    assert(play_turn() == 0 && queen.get_status(StatusType::STRENGTH) == 3);
    assert(queen.phase == 2);
    // Phase 2: Off With Your Head (3+3)x5, Execution 15+3, Enrage +2, Off With Your Head (3+5)x5.
    assert(queen.current_intent.type == ATTACK && queen.current_intent.hits == 5 && queen.intent_damage(combat.player) == 6);
    assert(play_turn() == 30);
    assert(queen.current_intent.type == ATTACK && queen.intent_damage(combat.player) == 18);
    assert(play_turn() == 18);
    assert(queen.current_intent.type == BUFF);
    assert(play_turn() == 0 && queen.get_status(StatusType::STRENGTH) == 5);
    assert(queen.phase == 2 && queen.current_intent.type == ATTACK && queen.intent_damage(combat.player) == 8);
    // The Queen dies: the fight is won.
    combat.damage_enemy(queen, 999);
    assert(!queen.is_alive() && !combat.has_alive_enemies());
    std::cout << "[PASS] Queen and Torch Head Amalgam: a whole fight, Burn Bright while the Amalgam lives, then "
                 "Off With Your Head, Execution, Enrage" << std::endl;
}

void test_waterfall_giant() {
    // Pressurize, then Stomp 15 (Weak), Ram 10, Siphon (heal), Pressure Gun 20, Pressure Up 13, and again.
    check_monster(EnemyType::WATERFALL_GIANT, "Géant de la Cascade", 240, {
        { BUFF, 0, 0, 0 },
        { ATTACK_DEBUFF, 15, 0, 0 },
        { ATTACK_BUFF, 10, 0, 0 },
        { EnemyIntentType::HEAL, 0, 0, 0 },
        { ATTACK, WATERFALL_GIANT_PRESSURE_GUN, 0, 0 },
        { ATTACK_BUFF, 13, 0, 0 },
        { ATTACK_DEBUFF, 15, 0, 0 },
        { ATTACK_BUFF, 10, 0, 0 },
    });

    // Steam Eruption: 15 from Pressurize, then 3 more with every move. Stomp makes the player Weak; Siphon heals.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::WATERFALL_GIANT);
    Enemy& boss = combat.enemies[0];
    const int16_t steam_after[] = { 15, 18, 21, 24, 27, 30, 33 };
    for (int turn = 0; turn < 7; ++turn) {
        combat.player.hp = combat.player.max_hp;
        if (turn == 3) {
            assert(boss.current_intent.type == EnemyIntentType::HEAL);
            combat.damage_enemy(boss, 40);
        }
        combat.end_player_turn();
        assert(combat.enemy_power_amount(0, PowerId::STEAM_ERUPTION) == steam_after[turn]);
        if (turn == 1) assert(combat.player.get_status(StatusType::WEAK) == 1);
        if (turn == 3) assert(boss.hp == 240 - 40 + WATERFALL_GIANT_SIPHON_HEAL);
    }
    std::cout << "[PASS] Waterfall Giant: 240 HP, Pressurize 15, Stomp 15 + Weak, Ram 10, Siphon, Pressure Gun, "
                 "Pressure Up 13, 3 Steam Eruption a move" << std::endl;
}

void test_steam_eruption() {
    // Killed with Steam Eruption: it stays at 1 HP, beyond harm, Overheats (a lost turn), then Explodes for its Steam
    // Eruption at the end of the player's next turn and dies of it.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::WATERFALL_GIANT);
    Enemy& boss = combat.enemies[0];
    combat.end_player_turn(); // Pressurize: 15
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // Stomp: 18
    assert(combat.enemy_power_amount(0, PowerId::STEAM_ERUPTION) == 18);
    combat.player.hp = combat.player.max_hp;
    combat.damage_enemy(boss, 999);
    assert(boss.is_alive() && boss.hp == 1 && boss.phase == 1 && boss.statuses.empty());
    assert(boss.current_intent.type == STUN && boss.explosion == 18);
    combat.damage_enemy(boss, 50); // beyond harm
    assert(boss.hp == 1 && combat.state == CombatState::PLAYER_TURN);
    combat.end_player_turn(); // About to Blow
    assert(combat.player.hp == combat.player.max_hp && boss.is_alive());
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 18);
    combat.damage_enemy(boss, 50);
    assert(boss.hp == 1);
    combat.end_player_turn(); // Explosion
    assert(combat.player.max_hp - combat.player.hp == 18);
    assert(!boss.is_alive() && combat.state == CombatState::VICTORY);

    // Killed before it holds any Steam Eruption: it simply dies.
    CombatEngine early = make_combat(CharacterId::IRONCLAD, EnemyType::WATERFALL_GIANT);
    early.damage_enemy(early.enemies[0], 999);
    assert(!early.enemies[0].is_alive() && !early.has_alive_enemies());

    // The Explosion can kill the player.
    CombatEngine deadly = make_combat(CharacterId::IRONCLAD, EnemyType::WATERFALL_GIANT);
    deadly.end_player_turn();
    deadly.damage_enemy(deadly.enemies[0], 999);
    deadly.player.hp = 10;
    deadly.end_player_turn();
    deadly.end_player_turn();
    assert(deadly.state == CombatState::DEFEAT);
    std::cout << "[PASS] Waterfall Giant: killed, it Overheats beyond harm, then Explodes for its Steam Eruption"
              << std::endl;
}

void test_waterfall_giant_ascension() {
    // 250 HP, Pressurize 20, Stomp 16, Ram 11, Pressure Gun 22 and Pressure Up 14 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::WATERFALL_GIANT);
    Enemy& boss = combat.enemies[0];
    assert(boss.ascension_hp() == 250 && boss.hp_spread() == 0);
    boss.ascend(true, true);
    combat.plan_enemies();
    assert(boss.hp == 250 && boss.max_hp == 250);
    assert(boss.current_intent.type == BUFF && boss.current_intent.power_amount == WATERFALL_GIANT_PRESSURIZE_ASC);
    combat.end_player_turn();
    assert(combat.enemy_power_amount(0, PowerId::STEAM_ERUPTION) == 20);
    const int expected[] = { 16, 11, 0, WATERFALL_GIANT_PRESSURE_GUN_ASC, 14 };
    for (int damage : expected) {
        assert(boss.intent_damage(combat.player) == damage);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
    }
    std::cout << "[PASS] Waterfall Giant: Ascension gives 250 HP, Pressurize 20, Stomp 16, Ram 11, Pressure Gun 22, "
                 "Pressure Up 14" << std::endl;
}

void test_waterfall_giant_whole_fight() {
    // The whole fight, until the boss dies: the player deals 30 damage a turn. Phase 0 (Pressurize, then its five
    // moves in a loop) until it is killed on turn 9 with 36 Steam Eruption; phase 1: Overheat on turn 9 (beyond
    // harm), Explosion on turn 10, which kills it.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::WATERFALL_GIANT);
    Enemy& boss = combat.enemies[0];
    const EnemyIntentType loop[5] = { ATTACK_DEBUFF, ATTACK_BUFF, EnemyIntentType::HEAL, ATTACK, ATTACK_BUFF };
    int turn = 1;
    int blown_on = 0;
    while (combat.state == CombatState::PLAYER_TURN) {
        if (boss.phase == 0) {
            assert(boss.current_intent.type == (turn == 1 ? BUFF : loop[(turn - 2) % 5]));
        }
        combat.damage_enemy(boss, 30);
        assert(boss.is_alive());
        if (blown_on == 0 && boss.phase == 1) {
            blown_on = turn;
            assert(boss.hp == 1 && boss.explosion == 36);
        }
        if (boss.phase == 1) {
            assert(boss.hp == 1);
            assert(boss.current_intent.type == (turn == blown_on ? STUN : ATTACK));
        }
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        if (combat.state == CombatState::VICTORY) assert(combat.player.max_hp - combat.player.hp == 36);
        ++turn;
    }
    assert(combat.state == CombatState::VICTORY && !boss.is_alive() && combat.player.is_alive());
    assert(blown_on == 9 && turn == 11);
    std::cout << "[PASS] Waterfall Giant: a whole fight to the boss's death, through About to Blow and its Explosion"
              << std::endl;
}

int vantom_wounds(const CombatEngine& combat) {
    return count_cards(combat.draw_pile, CardId::WOUND) + count_cards(combat.hand, CardId::WOUND) +
           count_cards(combat.discard_pile, CardId::WOUND);
}

void test_vantom() {
    // Ink Blot 7, Inky Lance 6x2, Dismember 26 (3 Wounds), Prepare (+2 Strength), and again 2 stronger.
    check_monster(EnemyType::VANTOM, "Vantôme", 173, {
        { ATTACK, 7, 0, 0 },
        { ATTACK, 6, 0, 0, 0, 2 },
        { ATTACK_DEBUFF, 26, 0, 0 },
        { BUFF, 0, 0, 2 },
        { ATTACK, 9, 0, 2 },
        { ATTACK, 8, 0, 2, 0, 2 },
        { ATTACK_DEBUFF, 28, 0, 2 },
        { BUFF, 0, 0, 4 },
    });

    // Dismember puts 3 Wounds in the player's discard pile, after the hit.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::VANTOM);
    Enemy& boss = combat.enemies[0];
    combat.end_player_turn();
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    combat.player.hp = combat.player.max_hp;
    assert(vantom_wounds(combat) == 0);
    assert(boss.current_intent.type == ATTACK_DEBUFF && boss.current_intent.status_card == CardId::WOUND &&
           boss.current_intent.status_cards == 3);
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 26);
    assert(vantom_wounds(combat) == 3);
    std::cout << "[PASS] Vantom: 173 HP, Ink Blot 7, Inky Lance 6x2, Dismember 26 + 3 Wounds, Prepare +2 Strength"
              << std::endl;
}

void test_vantom_ascension() {
    // 183 HP, Ink Blot 8, Inky Lance 7x2 and Dismember 30 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::VANTOM);
    Enemy& boss = combat.enemies[0];
    assert(boss.ascension_hp() == 183 && boss.hp_spread() == 0);
    boss.ascend(true, true);
    combat.plan_enemies();
    assert(boss.hp == 183 && boss.max_hp == 183);
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 8);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 7 && boss.current_intent.hits == 2);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(boss.current_intent.type == ATTACK_DEBUFF && boss.intent_damage(combat.player) == 30);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 30 && vantom_wounds(combat) == 3);
    assert(boss.current_intent.type == BUFF);
    std::cout << "[PASS] Vantom: Ascension gives 183 HP, Ink Blot 8, Inky Lance 7x2, Dismember 30" << std::endl;
}

void test_vantom_whole_fight() {
    // The whole fight, until the boss dies, at 20 damage a turn: a single phase, its four moves in a loop, the
    // Strength of each Prepare carried into the next cycle.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::VANTOM);
    Enemy& boss = combat.enemies[0];
    int turn = 1;
    while (combat.state == CombatState::PLAYER_TURN && boss.is_alive()) {
        const int step = (turn - 1) % 4;
        const int strength = 2 * ((turn - 1) / 4);
        const EnemyIntentType expected[4] = { ATTACK, ATTACK, ATTACK_DEBUFF, BUFF };
        const int damage[4] = { 7, 6, 26, 0 };
        assert(boss.phase == 0);
        assert(boss.current_intent.type == expected[step]);
        assert(boss.get_status(StatusType::STRENGTH) == strength);
        assert(boss.intent_damage(combat.player) == (step == 3 ? 0 : damage[step] + strength));
        combat.damage_enemy(boss, 20);
        if (!boss.is_alive()) break;
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        ++turn;
    }
    assert(!boss.is_alive() && combat.player.is_alive());
    assert(turn == 9);                  // 173 HP, 20 a turn
    assert(vantom_wounds(combat) == 6); // two Dismembers
    std::cout << "[PASS] Vantom: a whole fight to the boss's death, two full cycles" << std::endl;
}

// Beckons in every pile of the player.
int beckons(const CombatEngine& combat) {
    int count = count_in(combat.draw_pile, CardId::BECKON) + count_in(combat.discard_pile, CardId::BECKON);
    for (const DeckCard& card : combat.hand) count += card.id == CardId::BECKON;
    return count;
}

void test_soul_fysh() {
    // Beckon, De-Gas 16, Gaze 7, Fade (Intangible 2 on itself, 1 left once the round ends), Scream 13 with
    // Vulnerable 3, twice: the second De-Gas and Gaze hit a Vulnerable player.
    check_monster(EnemyType::SOUL_FYSH, "Poissâme", 211, {
        { STATUS, 0, 0, 0 },
        { ATTACK, 16, 0, 0 },
        { ATTACK, 7, 0, 0 },
        { BUFF, 0, 0, 0, 0, 1, 0, 1 },
        { ATTACK_DEBUFF, 13, 0, 0 },
        { STATUS, 0, 0, 0 },
        { ATTACK, 24, 0, 0 },
        { ATTACK, 10, 0, 0 },
        { BUFF, 0, 0, 0, 0, 1, 0, 1 },
        { ATTACK_DEBUFF, 13, 0, 0 },
    });

    // Beckon shuffles 2 Beckons into the draw pile, Gaze puts 1 in the discard pile after its attack.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SOUL_FYSH);
    Enemy& fysh = combat.enemies[0];
    assert(fysh.current_intent.status_card == CardId::BECKON && fysh.current_intent.status_pile == Pile::DRAW);
    combat.hand.clear();
    combat.end_player_turn();
    assert(combat.last_enemy_actions[0].cards_added == CardId::BECKON &&
           combat.last_enemy_actions[0].cards_added_count == SOUL_FYSH_BECKONS);
    assert(beckons(combat) == SOUL_FYSH_BECKONS);
    combat.hand.clear();
    combat.end_player_turn(); // De-Gas
    assert(fysh.current_intent.status_card == CardId::BECKON && fysh.current_intent.status_pile == Pile::DISCARD);
    combat.hand.clear();
    const int before = beckons(combat);
    combat.end_player_turn(); // Gaze
    assert(combat.last_enemy_actions[0].cards_added == CardId::BECKON &&
           combat.last_enemy_actions[0].cards_added_count == SOUL_FYSH_GAZE_BECKONS);
    assert(beckons(combat) == before + SOUL_FYSH_GAZE_BECKONS);

    // Fade: Intangible 2 on itself, so each hit of the player's next turn deals 1.
    assert(fysh.current_intent.type == BUFF && fysh.current_intent.status_type == StatusType::INTANGIBLE &&
           fysh.current_intent.status_amount == SOUL_FYSH_INTANGIBLE);
    combat.hand.clear();
    combat.end_player_turn();
    assert(fysh.get_status(StatusType::INTANGIBLE) == 1);
    combat.damage_enemy(fysh, 30);
    assert(fysh.hp == 210);

    // Scream: 13 damage and Vulnerable 3 on the player. Intangible is gone after it.
    assert(fysh.current_intent.type == ATTACK_DEBUFF && fysh.current_intent.debuffs[0].type == StatusType::VULNERABLE);
    combat.player.hp = combat.player.max_hp;
    combat.hand.clear();
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 13);
    assert(combat.player.get_status(StatusType::VULNERABLE) == SOUL_FYSH_SCREAM_VULNERABLE);
    assert(fysh.get_status(StatusType::INTANGIBLE) == 0);
    std::cout << "[PASS] Soul Fysh: 211 HP, Beckon, De-Gas 16, Gaze 7, Fade (Intangible 2), Scream 13 + Vulnerable 3"
              << std::endl;
}

void test_beckon() {
    // Unplayable; held at the end of the turn, it costs 6 HP.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SOUL_FYSH);
    combat.hand.clear();
    combat.hand.push_back(DeckCard(CardId::BECKON));
    combat.player.energy = combat.player.max_energy;
    assert(!combat.can_play_card(0));
    assert(CardDef::get(CardId::BECKON).type == CardType::STATUS);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // the Soul Fysh's Beckon deals no damage
    assert(combat.player.max_hp - combat.player.hp == 6);
    std::cout << "[PASS] Beckon: unplayable, 6 HP lost when held at the end of the turn" << std::endl;
}

void test_soul_fysh_ascension() {
    // 221 HP, De-Gas 18, Gaze 8 and Scream 15 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SOUL_FYSH);
    Enemy& fysh = combat.enemies[0];
    assert(fysh.ascension_hp() == 221 && fysh.hp_spread() == 0);
    fysh.ascend(true, true);
    combat.plan_enemies();
    assert(fysh.hp == 221 && fysh.max_hp == 221);
    const int damage[5] = { 0, 18, 8, 0, 15 };
    for (int step = 0; step < 5; ++step) {
        assert(fysh.intent_damage(combat.player) == damage[step]);
        combat.player.hp = combat.player.max_hp;
        combat.hand.clear();
        combat.end_player_turn();
    }
    std::cout << "[PASS] Soul Fysh: Ascension gives 221 HP, De-Gas 18, Gaze 8, Scream 15" << std::endl;
}

void test_soul_fysh_whole_fight() {
    // The whole fight, until the boss dies: the player deals 30 damage a turn. One phase, its five moves in a loop;
    // on the turn after Fade, the Intangible boss loses 1 HP only.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SOUL_FYSH);
    Enemy& fysh = combat.enemies[0];
    const EnemyIntentType expected[5] = { STATUS, ATTACK, ATTACK, BUFF, ATTACK_DEBUFF };
    int turn = 1;
    int intangible_turns = 0;
    while (combat.state == CombatState::PLAYER_TURN && fysh.is_alive()) {
        assert(fysh.current_intent.type == expected[(turn - 1) % 5]);
        const int16_t hp_before = fysh.hp;
        const bool intangible = fysh.get_status(StatusType::INTANGIBLE) > 0;
        combat.damage_enemy(fysh, 30);
        if (intangible) {
            ++intangible_turns;
            assert(turn % 5 == 0 && hp_before - fysh.hp == 1);
        }
        if (!fysh.is_alive()) break;
        combat.player.hp = combat.player.max_hp;
        combat.player.clear_status(StatusType::VULNERABLE);
        combat.hand.clear();
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        ++turn;
    }
    assert(!fysh.is_alive() && combat.player.is_alive());
    assert(intangible_turns >= 1);
    std::cout << "[PASS] Soul Fysh: a whole fight to the boss's death, through its Fade" << std::endl;
}

int aeonglass_withers(const CombatEngine& combat) {
    return count_cards(combat.draw_pile, CardId::WITHER) + count_cards(combat.hand, CardId::WITHER) +
           count_cards(combat.discard_pile, CardId::WITHER);
}

void test_aeonglass() {
    // Ebb 22 + 33 Block, Eye Lasers 11x2, Increasing Intensity (+2 Strength, 1 Wither), and again 2 stronger.
    check_monster(EnemyType::AEONGLASS, "Clepséon", 512, {
        { ATTACK_DEFEND, 22, 33, 0 },
        { ATTACK, 11, 0, 0, 0, 2 },
        { STATUS, 0, 0, 2 },
        { ATTACK_DEFEND, 24, 33, 2 },
        { ATTACK, 13, 0, 2, 0, 2 },
        { STATUS, 0, 0, 4 },
    });

    // Increasing Intensity puts 1 Wither, an unplayable Status, in the player's discard pile.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::AEONGLASS);
    Enemy& boss = combat.enemies[0];
    combat.end_player_turn();
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    combat.player.hp = combat.player.max_hp;
    assert(aeonglass_withers(combat) == 0);
    assert(boss.current_intent.type == STATUS && boss.current_intent.status_card == CardId::WITHER &&
           boss.current_intent.status_cards == 1 && boss.current_intent.status_pile == Pile::DISCARD);
    combat.end_player_turn();
    assert(combat.player.hp == combat.player.max_hp);
    assert(aeonglass_withers(combat) == 1 && boss.get_status(StatusType::STRENGTH) == AEONGLASS_STRENGTH);
    assert(CardDef::get(CardId::WITHER).type == CardType::STATUS &&
           CardDef::has_keyword(CardId::WITHER, false, KEYWORD_UNPLAYABLE));
    std::cout << "[PASS] Aeonglass: 512 HP, Ebb 22 + 33 Block, Eye Lasers 11x2, Increasing Intensity +2 Strength + "
                 "1 Wither"
              << std::endl;
}

void test_aeonglass_ascension() {
    // 535 HP, Ebb 26, Eye Lasers 12x2 and Increasing Intensity +3 Strength with 2 Withers at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::AEONGLASS);
    Enemy& boss = combat.enemies[0];
    assert(boss.ascension_hp() == 535 && boss.hp_spread() == 0);
    boss.ascend(true, true);
    combat.plan_enemies();
    assert(boss.hp == 535 && boss.max_hp == 535);
    assert(boss.current_intent.type == ATTACK_DEFEND && boss.intent_damage(combat.player) == 26 &&
           boss.current_intent.block == 33);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 26 && boss.block == 33);
    assert(boss.current_intent.type == ATTACK && boss.intent_damage(combat.player) == 12 && boss.current_intent.hits == 2);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 24);
    assert(boss.current_intent.type == STATUS && boss.current_intent.status_cards == 2);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(aeonglass_withers(combat) == 2 && boss.get_status(StatusType::STRENGTH) == AEONGLASS_STRENGTH_ASC);
    std::cout << "[PASS] Aeonglass: Ascension gives 535 HP, Ebb 26, Eye Lasers 12x2, +3 Strength and 2 Withers"
              << std::endl;
}

void test_aeonglass_whole_fight() {
    // The whole fight, until the boss dies, at 60 damage a turn: a single phase, its three moves in a loop, the
    // Strength of each Increasing Intensity carried into the next cycle, Ebb's Block soaking up part of the blows.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::AEONGLASS);
    Enemy& boss = combat.enemies[0];
    int turn = 1;
    while (combat.state == CombatState::PLAYER_TURN && boss.is_alive()) {
        const int step = (turn - 1) % 3;
        const int strength = AEONGLASS_STRENGTH * ((turn - 1) / 3);
        const EnemyIntentType expected[3] = { ATTACK_DEFEND, ATTACK, STATUS };
        const int damage[3] = { 22, 11, 0 };
        assert(boss.phase == 0);
        assert(boss.current_intent.type == expected[step]);
        assert(boss.get_status(StatusType::STRENGTH) == strength);
        assert(boss.intent_damage(combat.player) == (step == 2 ? 0 : damage[step] + strength));
        // Ebb's 33 Block lasts until the boss's next turn: it is up during the Eye Lasers turn.
        assert(boss.block == (step == 1 ? 33 : 0));
        combat.damage_enemy(boss, 60);
        if (!boss.is_alive()) break;
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        ++turn;
    }
    assert(!boss.is_alive() && combat.player.is_alive());
    assert(turn == 11);                     // 512 HP, 60 a turn, 33 of them blocked on the turns after Ebb
    assert(aeonglass_withers(combat) == 3); // three Increasing Intensities
    std::cout << "[PASS] Aeonglass: a whole fight to the boss's death, three full cycles" << std::endl;
}

void test_ceremonial_beast() {
    // Phase 0: Stamp (Plow 150 on itself), then Plow (18 damage, +2 Strength) every turn.
    check_monster(EnemyType::CEREMONIAL_BEAST, "Cerf Auguste", 252, {
        { BUFF, 0, 0, 0 },
        { ATTACK_BUFF, 18, 0, 2 },
        { ATTACK_BUFF, 20, 0, 4 },
        { ATTACK_BUFF, 22, 0, 6 },
    });
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::CEREMONIAL_BEAST);
    Enemy& beast = combat.enemies[0];
    assert(beast.current_intent.power == PowerId::PLOW && !beast.current_intent.power_on_player);
    combat.end_player_turn();
    assert(combat.enemy_power_amount(0, PowerId::PLOW) == CEREMONIAL_BEAST_PLOW);
    std::cout << "[PASS] Ceremonial Beast: 252 HP, Stamp (Plow 150), then Plow 18 +2 Strength each turn" << std::endl;
}

void test_plow() {
    // The Plow holds while the Beast has at least 150 HP. Below, it loses all its Strength, is stunned, and
    // enters its second phase: Beast Cry (Ringing), Stomp 15, Crush 17 (+3 Strength), in a loop.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::CEREMONIAL_BEAST);
    Enemy& beast = combat.enemies[0];
    combat.end_player_turn(); // Stamp
    combat.end_player_turn(); // Plow: +2 Strength
    combat.player.hp = combat.player.max_hp;
    assert(beast.get_status(StatusType::STRENGTH) == 2);
    combat.damage_enemy(beast, 252 - CEREMONIAL_BEAST_PLOW);
    assert(beast.hp == CEREMONIAL_BEAST_PLOW && beast.phase == 0);
    assert(beast.current_intent.type == ATTACK_BUFF && combat.enemy_power_amount(0, PowerId::PLOW) == CEREMONIAL_BEAST_PLOW);
    combat.damage_enemy(beast, 1);
    assert(beast.phase == 1 && beast.current_intent.type == STUN);
    assert(beast.get_status(StatusType::STRENGTH) == 0 && combat.enemy_power_amount(0, PowerId::PLOW) == 0);
    // Stunned: the turn is lost.
    combat.end_player_turn();
    assert(combat.player.hp == combat.player.max_hp);
    // Beast Cry: the player is Ringing for their next turn, and can play only one card.
    assert(beast.current_intent.type == DEBUFF && beast.current_intent.power == PowerId::RINGING);
    combat.end_player_turn();
    assert(combat.has_power(PowerId::RINGING));
    combat.hand.clear();
    combat.hand.push_back(CardId::DEFEND);
    combat.hand.push_back(CardId::DEFEND);
    combat.player.energy = 3;
    assert(combat.play_blocker(0) == PlayBlocker::NONE);
    assert(combat.play_card(0, 0));
    assert(combat.play_blocker(0) == PlayBlocker::PLAY_LIMIT);
    assert(!combat.play_card(0, 0));
    // Stomp 15; Ringing is gone with the turn.
    assert(beast.current_intent.type == ATTACK && beast.intent_damage(combat.player) == 15);
    combat.player.block = 0;
    combat.end_player_turn();
    assert(!combat.has_power(PowerId::RINGING));
    assert(combat.player.max_hp - combat.player.hp == 15);
    combat.player.hp = combat.player.max_hp;
    // Crush 17 and +3 Strength, then Beast Cry again, and Stomp with the Strength.
    assert(beast.current_intent.type == ATTACK_BUFF && beast.intent_damage(combat.player) == 17);
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 17 && beast.get_status(StatusType::STRENGTH) == 3);
    combat.player.hp = combat.player.max_hp;
    assert(beast.current_intent.type == DEBUFF && beast.current_intent.power == PowerId::RINGING);
    combat.end_player_turn();
    assert(beast.current_intent.type == ATTACK && beast.intent_damage(combat.player) == 18);
    // The Plow is spent: no second stun however low its HP.
    combat.damage_enemy(beast, 100);
    assert(beast.is_alive() && beast.current_intent.type == ATTACK);

    // Artifact cancels Ringing.
    CombatEngine guarded = make_combat(CharacterId::IRONCLAD, EnemyType::CEREMONIAL_BEAST);
    guarded.end_player_turn();
    guarded.damage_enemy(guarded.enemies[0], 252 - CEREMONIAL_BEAST_PLOW + 1);
    guarded.end_player_turn(); // stunned
    guarded.player.add_status(StatusType::ARTIFACT, 1);
    guarded.end_player_turn(); // Beast Cry
    assert(!guarded.has_power(PowerId::RINGING) && guarded.player.get_status(StatusType::ARTIFACT) == 0);
    std::cout << "[PASS] Plow: below 150 HP, the Beast loses its Strength, is stunned, then Beast Cry (Ringing: "
                 "one card), Stomp 15, Crush 17 +3 Strength" << std::endl;
}

void test_ceremonial_beast_ascension() {
    // 262 HP, Plow 160, Plow 20, Stomp 17 and Crush 19 (+4 Strength) at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::CEREMONIAL_BEAST);
    Enemy& beast = combat.enemies[0];
    assert(beast.ascension_hp() == 262 && beast.hp_spread() == 0);
    beast.ascend(true, true);
    combat.plan_enemies();
    assert(beast.hp == 262 && beast.max_hp == 262);
    assert(beast.current_intent.type == BUFF && beast.current_intent.power_amount == CEREMONIAL_BEAST_PLOW_ASC);
    combat.end_player_turn();
    assert(combat.enemy_power_amount(0, PowerId::PLOW) == CEREMONIAL_BEAST_PLOW_ASC);
    assert(beast.current_intent.type == ATTACK_BUFF && beast.intent_damage(combat.player) == 20);
    combat.damage_enemy(beast, 262 - CEREMONIAL_BEAST_PLOW_ASC + 1);
    assert(beast.current_intent.type == STUN);
    combat.end_player_turn(); // stunned
    combat.end_player_turn(); // Beast Cry
    assert(beast.current_intent.type == ATTACK && beast.intent_damage(combat.player) == 17);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(beast.current_intent.type == ATTACK_BUFF && beast.intent_damage(combat.player) == 19 &&
           beast.current_intent.status_amount == 4);
    std::cout << "[PASS] Ceremonial Beast: Ascension gives 262 HP, Plow 160, Plow 20, Stomp 17, Crush 19 +4" << std::endl;
}

void test_ceremonial_beast_whole_fight() {
    // The whole fight, until the Beast dies, at 20 damage a turn: Stamp, Plow until its HP fall below 150 (phase
    // change: Strength lost, stunned), then Beast Cry, Stomp, Crush in a loop to the end.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::CEREMONIAL_BEAST);
    Enemy& beast = combat.enemies[0];
    int turn = 1;
    int phase_change = 0;
    int ringing_turns = 0;
    while (combat.state == CombatState::PLAYER_TURN && beast.is_alive()) {
        ringing_turns += combat.has_power(PowerId::RINGING);
        if (beast.phase == 0) {
            assert(beast.current_intent.type == (turn == 1 ? BUFF : ATTACK_BUFF));
            assert(beast.get_status(StatusType::STRENGTH) == 2 * (turn > 1 ? turn - 2 : 0));
        }
        const uint8_t phase_before = beast.phase;
        combat.damage_enemy(beast, 20);
        if (!beast.is_alive()) break;
        if (phase_before == 0 && beast.phase == 1) {
            // The phase changes on the hit that takes it below the Plow.
            assert(beast.hp < CEREMONIAL_BEAST_PLOW && beast.hp + 20 >= CEREMONIAL_BEAST_PLOW);
            assert(beast.current_intent.type == STUN && beast.get_status(StatusType::STRENGTH) == 0);
            phase_change = turn;
        } else if (beast.phase == 1) {
            const int step = (turn - phase_change - 1) % 3;
            const EnemyIntentType expected[3] = { DEBUFF, ATTACK, ATTACK_BUFF };
            assert(beast.current_intent.type == expected[step]);
        }
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        ++turn;
    }
    assert(!beast.is_alive() && combat.player.is_alive());
    assert(phase_change == 6); // 252 - 6 x 20 = 132 < 150
    assert(turn == 13);        // 252 HP, 20 a turn
    assert(ringing_turns == 2);
    std::cout << "[PASS] Ceremonial Beast: a whole fight to its death, through the Plow's phase change" << std::endl;
}

void test_wriggler() {
    // Nasty Bite (6 damage), Wriggle (+2 Strength and an Infection), and again with the Strength kept.
    // The starting HP is 17 to 21: the roll of the fight is read, then checked against that range.
    const int hp = make_combat(CharacterId::IRONCLAD, EnemyType::WRIGGLER).enemies[0].hp;
    assert(hp >= 17 && hp <= 21);
    check_monster(EnemyType::WRIGGLER, "Larve", hp, {
        { ATTACK, 6, 0, 0 },
        { STATUS, 0, 0, 2 },
        { ATTACK, 8, 0, 2 },
        { STATUS, 0, 0, 4 },
        { ATTACK, 10, 0, 4 },
    });
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::WRIGGLER);
    combat.end_player_turn();
    assert(count_in(combat.discard_pile, CardId::INFECTION) == 0);
    assert(combat.enemies[0].current_intent.gives_cards() && combat.enemies[0].current_intent.status_card == CardId::INFECTION);
    combat.end_player_turn();
    assert(combat.last_enemy_actions[0].cards_added == CardId::INFECTION && combat.last_enemy_actions[0].cards_added_count == 1);
    // The next turn draws from a reshuffled deck: the Infection is in one of the player's piles.
    int infections = count_in(combat.discard_pile, CardId::INFECTION) + count_in(combat.draw_pile, CardId::INFECTION);
    for (const DeckCard& card : combat.hand) infections += card.id == CardId::INFECTION;
    assert(infections == 1);
    std::cout << "[PASS] Wriggler: 17 to 21 HP, Nasty Bite 6, Wriggle (+2 Strength, 1 Infection), loop" << std::endl;
}

void test_wriggler_ascension() {
    // 18 to 22 HP (17 to 21 plus 1) and a Nasty Bite of 7 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::WRIGGLER);
        Enemy& worm = combat.enemies[0];
        worm.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = worm.hp;
        assert(worm.ascension_hp() == 18 && worm.hp_spread() == 4);
        assert(base >= 17 && base <= 21 && worm.max_hp == 17 + worm.hp_bonus);
        worm.ascend(true, true);
        combat.plan_enemies();
        assert(worm.hp == worm.max_hp && worm.hp >= 18 && worm.hp <= 22);
        assert(worm.current_intent.type == ATTACK && worm.intent_damage(combat.player) == 7);
    }
    std::cout << "[PASS] Wriggler: Ascension gives 18 to 22 HP and a Nasty Bite of 7" << std::endl;
}

void test_phrog_parasite() {
    // Infect (3 Infections), Lash (4 damage x4), and again.
    check_monster(EnemyType::PHROG_PARASITE, "Parasite Krapo", 61, {
        { STATUS, 0, 0, 0 },
        { ATTACK, 4, 0, 0, 0, 4 },
        { STATUS, 0, 0, 0 },
        { ATTACK, 4, 0, 0, 0, 4 },
    });
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::PHROG_PARASITE);
    Enemy& parasite = combat.enemies[0];
    assert(parasite.current_intent.gives_cards() && parasite.current_intent.status_card == CardId::INFECTION);
    combat.end_player_turn();
    assert(combat.last_enemy_actions[0].cards_added == CardId::INFECTION && combat.last_enemy_actions[0].cards_added_count == 3);
    assert(count_in(combat.discard_pile, CardId::INFECTION) == 3);
    assert(CardDef::get(CardId::INFECTION).type == CardType::STATUS &&
           CardDef::has_keyword(CardId::INFECTION, false, KEYWORD_UNPLAYABLE));
    std::cout << "[PASS] Phrog Parasite: 61 HP, Infect (3 Infections), Lash 4x4, loop" << std::endl;
}

void test_phrog_parasite_ascension() {
    // 66 HP at least at Ascension (the engine adds the same 0 to 3 as at A0, so up to 69) and a Lash of 5x4.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::PHROG_PARASITE);
        Enemy& parasite = combat.enemies[0];
        parasite.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = parasite.hp;
        assert(parasite.ascension_hp() == 66 && parasite.hp_spread() == 3);
        assert(base >= 61 && base <= 64);
        parasite.ascend(true, true);
        combat.plan_enemies();
        assert(parasite.hp == parasite.max_hp && parasite.hp >= 66 && parasite.hp <= 69);
        combat.end_player_turn();
        assert(parasite.current_intent.type == ATTACK && parasite.current_intent.hits == 4 &&
               parasite.intent_damage(combat.player) == 5);
    }
    std::cout << "[PASS] Phrog Parasite: Ascension gives 66 HP minimum and a Lash of 5x4" << std::endl;
}

bool is_fabricator_bot(EnemyType type) {
    return type == EnemyType::STABBOT || type == EnemyType::NOISEBOT || type == EnemyType::ZAPBOT ||
           type == EnemyType::GUARDBOT;
}

void test_fabricator() {
    // 150 HP. With room for a bot: Fabricate (summons one bot) or Fabricating Strike (18 and one bot), 1 in 2.
    // With 3 enemies alive: Disintegrate 11.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FABRICATOR);
    Enemy& fabricator = combat.enemies[0];
    assert(fabricator.name == "Machiniste" && fabricator.hp == 150 && fabricator.max_hp == 150);
    bool fabricate = false, strike = false, disintegrate = false;
    for (int turn = 0; turn < 40; ++turn) {
        combat.player.hp = combat.player.max_hp = 999;
        for (size_t i = 1; i < combat.enemies.size(); ++i) combat.enemies[i].hp = 0;
        if (turn % 4 == 3) {
            // Two bots in the fight: no room left, Disintegrate, then it fabricates again.
            while (combat.enemies.size() < 3) combat.summon_enemy(EnemyType::ZAPBOT, 0);
            for (size_t i = 1; i < combat.enemies.size(); ++i) combat.enemies[i].hp = combat.enemies[i].max_hp;
            combat.plan_enemies();
            assert(fabricator.current_intent.type == ATTACK && fabricator.current_intent.summons == 0);
            assert(fabricator.intent_damage(combat.player) == 11);
            disintegrate = true;
            continue;
        }
        combat.plan_enemies();
        const EnemyIntent intent = fabricator.current_intent;
        assert(intent.summons == 1 && intent.summon_minions && is_fabricator_bot(intent.summon_type));
        if (intent.type == SUMMON) {
            fabricate = true;
            assert(fabricator.intent_damage(combat.player) == 0);
        } else {
            strike = true;
            assert(intent.type == ATTACK && fabricator.intent_damage(combat.player) == 18);
        }
        const size_t before = combat.count_alive_enemies();
        const int16_t hp = combat.player.hp;
        combat.end_player_turn();
        assert(combat.count_alive_enemies() == before + 1);
        // The bot is a Minion of the Fabricator and has not acted yet: only a Strike hurt the player.
        const uint8_t summoned = combat.last_enemy_actions[0].summoned;
        assert(summoned != 0 && (summoned & (summoned - 1)) == 0);
        size_t slot = 0;
        while (!(summoned & (1u << slot))) ++slot;
        const Enemy& bot = combat.enemies[slot];
        assert(bot.minion && bot.leader == 0 && bot.type == intent.summon_type);
        assert(combat.player.hp == hp - (intent.type == SUMMON ? 0 : 18));
    }
    assert(fabricate && strike && disintegrate);
    std::cout << "[PASS] Fabricator: 150 HP, Fabricate and Fabricating Strike bring a bot, Disintegrate 11 without room" << std::endl;
}

void test_fabricator_ascension() {
    // 155 HP, Fabricating Strike 21 and Disintegrate 13 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FABRICATOR);
    Enemy& fabricator = combat.enemies[0];
    assert(fabricator.ascension_hp() == 155);
    fabricator.ascend(true, true);
    assert(fabricator.hp == 155 && fabricator.max_hp == 155);
    bool strike = false;
    for (int turn = 0; turn < 20 && !strike; ++turn) {
        for (size_t i = 1; i < combat.enemies.size(); ++i) combat.enemies[i].hp = 0;
        combat.plan_enemies();
        if (fabricator.current_intent.type == ATTACK) {
            strike = true;
            assert(fabricator.intent_damage(combat.player) == 21);
        }
    }
    assert(strike);
    while (combat.enemies.size() < 3) combat.summon_enemy(EnemyType::ZAPBOT, 0);
    combat.plan_enemies();
    assert(fabricator.current_intent.type == ATTACK && fabricator.intent_damage(combat.player) == 13);
    std::cout << "[PASS] Fabricator: Ascension gives 155 HP, Strike 21 and Disintegrate 13" << std::endl;
}

void test_tough_egg() {
    // Hatch on its first turn, which does nothing to the player, then Nibble 4 every turn. Its HP are drawn
    // between 14 and 18 at the start of the fight.
    CombatEngine probe = make_combat(CharacterId::IRONCLAD, EnemyType::TOUGH_EGG);
    const Enemy& egg = probe.enemies[0];
    assert(egg.hp >= 14 && egg.hp <= 18 && egg.max_hp == 14 + egg.hp_bonus && egg.hp_spread() == 4);
    check_monster(EnemyType::TOUGH_EGG, "Œuf robuste", egg.hp, {
        { SUMMON, 0, 0, 0 },
        { ATTACK, 4, 0, 0 },
        { ATTACK, 4, 0, 0 },
        { ATTACK, 4, 0, 0 },
    });
    std::cout << "[PASS] Tough Egg: 14 to 18 HP, Hatch, then Nibble 4 every turn" << std::endl;
}

void test_tough_egg_ascension() {
    // 15 to 19 HP (14 to 18 plus 1) and a Nibble of 5 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TOUGH_EGG);
        Enemy& egg = combat.enemies[0];
        egg.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = egg.hp;
        assert(egg.ascension_hp() == 15 && base >= 14 && base <= 18);
        egg.ascend(true, true);
        combat.plan_enemies();
        assert(egg.hp == base + 1 && egg.max_hp == base + 1 && egg.hp >= 15 && egg.hp <= 19);
        assert(egg.current_intent.type == SUMMON);
        combat.end_player_turn();
        assert(egg.current_intent.type == ATTACK && egg.intent_damage(combat.player) == 5);
    }
    std::cout << "[PASS] Tough Egg: Ascension gives 15 to 19 HP and a Nibble of 5" << std::endl;
}

void test_the_obscura() {
    // Illusion (a Parafright joins as a Minion) first, then Hardening Strike 6 (6 Block), then a draw among
    // Piercing Gaze 10, Sail and Hardening Strike that never repeats the move just played.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::THE_OBSCURA);
    Enemy& obscura = combat.enemies[0];
    assert(obscura.type == EnemyType::THE_OBSCURA && obscura.name == "L'Obscura");
    assert(obscura.hp == 123 && obscura.max_hp == 123 && obscura.block == 0 && obscura.statuses.empty());
    assert(obscura.current_intent.type == SUMMON && obscura.current_intent.summon_type == EnemyType::PARAFRIGHT &&
           obscura.current_intent.summons == 1 && obscura.intent_damage(combat.player) == 0);
    combat.end_player_turn();
    assert(combat.enemies.size() == 2 && combat.enemies[1].type == EnemyType::PARAFRIGHT);
    assert(combat.enemies[1].minion && combat.enemies[1].leader == 0 && combat.enemies[1].hp == 21);

    bool gaze = false, sail = false, strike = false;
    int previous = -1;
    for (int turn = 2; turn <= 40; ++turn) {
        combat.player.hp = combat.player.max_hp;
        combat.player.block = 0;
        obscura.block = 0;
        int kind;
        if (obscura.current_intent.type == ATTACK) {
            assert(obscura.intent_damage(combat.player) == 10);
            kind = 1;
            gaze = true;
        } else if (obscura.current_intent.type == BUFF) {
            kind = 2;
            sail = true;
        } else {
            assert(obscura.current_intent.type == ATTACK_DEFEND && obscura.intent_damage(combat.player) == 6 &&
                   obscura.current_intent.block == 6);
            kind = 3;
            strike = true;
        }
        if (turn == 2) assert(kind == 3);
        assert(kind != previous);
        previous = kind;
        combat.end_player_turn();
        if (kind == 3) assert(obscura.block == 6);
    }
    assert(gaze && sail && strike);
    std::cout << "[PASS] The Obscura: 123 HP, Illusion, then Hardening Strike and a draw without repeats" << std::endl;
}

void test_the_obscura_ascension() {
    // 129 HP, Piercing Gaze 11 and Hardening Strike 7 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::THE_OBSCURA);
    Enemy& obscura = combat.enemies[0];
    obscura.ascend(true, true);
    combat.plan_enemies();
    assert(obscura.hp == 129 && obscura.max_hp == 129);
    assert(obscura.current_intent.type == SUMMON);
    bool gaze = false, strike = false;
    for (int turn = 2; turn <= 40 && !(gaze && strike); ++turn) {
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        if (obscura.current_intent.type == ATTACK) {
            assert(obscura.intent_damage(combat.player) == 11);
            gaze = true;
        } else if (obscura.current_intent.type == ATTACK_DEFEND) {
            assert(obscura.intent_damage(combat.player) == 7);
            strike = true;
        }
    }
    assert(gaze && strike);
    std::cout << "[PASS] The Obscura: Ascension gives 129 HP, a Gaze of 11 and a Strike of 7" << std::endl;
}

void test_ovicopter() {
    // Lay Eggs (a Tough Egg, Minion of it), Smash 16, Tenderizer 7 with 2 Vulnerable, and again while there is room
    // for an egg; with two eggs already in the fight, Nutritional Paste (+3 Strength) takes the place of Lay Eggs.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::OVICOPTER);
    Enemy& bird = combat.enemies[0];
    assert(bird.name == "Ovicoptère" && bird.hp >= 124 && bird.hp <= 130 && bird.hp == bird.max_hp);
    assert(bird.hp_spread() == 6 && bird.max_hp == 124 + bird.hp_bonus);
    const int16_t hp = bird.hp;
    combat.player.max_hp = combat.player.hp = 500;
    auto play = [&]() {
        combat.player.hp = combat.player.max_hp;
        combat.player.block = 0;
        combat.player.statuses.clear(); // Vulnerable would raise the damage seen
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
    };
    // Turn 1: Lay Eggs, which hurts nobody and brings a Minion egg in.
    assert(bird.current_intent.type == SUMMON && bird.current_intent.summons == 1);
    assert(bird.current_intent.summon_type == EnemyType::TOUGH_EGG && bird.current_intent.summon_minions);
    assert(bird.intent_damage(combat.player) == 0);
    play();
    assert(combat.enemies.size() == 2 && combat.enemies[1].type == EnemyType::TOUGH_EGG);
    assert(combat.enemies[1].minion && combat.enemies[1].leader == 0);
    // Turn 2: Smash 16.
    assert(bird.current_intent.type == ATTACK && bird.intent_damage(combat.player) == 16);
    play();
    // Turn 3: Tenderizer 7 and 2 Vulnerable.
    assert(bird.current_intent.type == ATTACK_DEBUFF && bird.intent_damage(combat.player) == 7);
    play();
    assert(combat.player.get_status(StatusType::VULNERABLE) == 2);
    // Turn 4: one egg in the fight, room for another: Lay Eggs again.
    assert(bird.current_intent.type == SUMMON);
    play();
    assert(combat.count_alive_enemies() == 3);
    play(); // Smash
    play(); // Tenderizer
    // Turn 7: three enemies, no room: Nutritional Paste.
    assert(combat.turn_number == 7);
    assert(bird.current_intent.type == BUFF && bird.current_intent.status_type == StatusType::STRENGTH);
    assert(bird.current_intent.status_amount == 3 && bird.intent_damage(combat.player) == 0);
    play();
    assert(bird.get_status(StatusType::STRENGTH) == 3 && bird.hp == hp && bird.block == 0);
    // Then Smash 16 + 3 and Tenderizer 7 + 3.
    assert(bird.current_intent.type == ATTACK && bird.intent_damage(combat.player) == 19);
    play();
    assert(bird.current_intent.type == ATTACK_DEBUFF && bird.intent_damage(combat.player) == 10);
    // Eggs dead: the next cycle lays again.
    play();
    for (size_t i = 1; i < combat.enemies.size(); ++i) combat.enemies[i].hp = 0;
    combat.plan_enemies();
    assert(bird.current_intent.type == SUMMON);
    std::cout << "[PASS] Ovicopter: Lay Eggs, Smash 16, Tenderizer 7 + Vulnerable, Nutritional Paste when full" << std::endl;
}

void test_ovicopter_ascension() {
    // 126 to 132 HP, Smash 17, Tenderizer 8 and Nutritional Paste of 4 Strength at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::OVICOPTER);
        Enemy& bird = combat.enemies[0];
        bird.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = bird.hp;
        assert(bird.ascension_hp() == 126 && base >= 124 && base <= 130);
        bird.ascend(true, true);
        combat.plan_enemies();
        assert(bird.hp == base + 2 && bird.max_hp == base + 2 && bird.hp >= 126 && bird.hp <= 132);
        assert(bird.current_intent.type == SUMMON);
        combat.player.hp = combat.player.max_hp = 500;
        combat.end_player_turn();
        assert(bird.current_intent.type == ATTACK && bird.intent_damage(combat.player) == 17);
        combat.end_player_turn();
        assert(bird.current_intent.type == ATTACK_DEBUFF && bird.intent_damage(combat.player) == 8);
    }
    CombatEngine full = make_combat(CharacterId::IRONCLAD, EnemyType::OVICOPTER);
    Enemy& bird = full.enemies[0];
    bird.ascend(true, true);
    PlanContext crowded;
    crowded.allies = 2;
    bird.plan_turn(4, crowded);
    assert(bird.current_intent.type == BUFF && bird.current_intent.status_amount == 4);
    std::cout << "[PASS] Ovicopter: Ascension gives 126 to 132 HP, Smash 17, Tenderizer 8, Paste +4" << std::endl;
}

void test_shrinker_beetle() {
    // Shrinker (the player is Shrunk: their attacks deal 30% less damage for 3 turns), then Chomp 7 and Stomp 13
    // in turn.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SHRINKER_BEETLE);
    Enemy& beetle = combat.enemies[0];
    assert(beetle.name == "Scarabrideur" && beetle.hp >= 38 && beetle.hp <= 40 && beetle.hp == beetle.max_hp);
    assert(beetle.hp_spread() == 2 && beetle.max_hp == 38 + beetle.hp_bonus);
    const int16_t hp = beetle.hp;
    combat.player.max_hp = combat.player.hp = 500;
    auto play = [&]() {
        combat.player.hp = combat.player.max_hp;
        combat.player.block = 0;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
    };
    // Turn 1: Shrinker, which hurts nobody and Shrinks the player for 3 turns.
    assert(beetle.current_intent.type == DEBUFF && beetle.intent_damage(combat.player) == 0);
    assert(beetle.current_intent.power == PowerId::SHRINK && beetle.current_intent.power_on_player);
    assert(!combat.has_power(PowerId::SHRINK));
    play();
    assert(combat.player.hp == 500 && combat.last_enemy_actions[0].power == PowerId::SHRINK);
    assert(combat.power_amount(PowerId::SHRINK) == SHRINK_TURNS && SHRINK_TURNS == 3);
    // Then Chomp 7, Stomp 13, Chomp 7, Stomp 13: the player is Shrunk on the three turns after Shrinker.
    constexpr int DAMAGE[] = { 7, 13, 7, 13 };
    for (int turn = 0; turn < 4; ++turn) {
        assert(combat.has_power(PowerId::SHRINK) == (turn < 3));
        beetle.block = 0;
        const int16_t before = beetle.hp;
        combat.hit(beetle, 10); // 10 damage, 7 while Shrunk
        assert(before - beetle.hp == (turn < 3 ? 7 : 10));
        beetle.hp = before;
        assert(beetle.current_intent.type == ATTACK && beetle.intent_damage(combat.player) == DAMAGE[turn]);
        play();
        assert(combat.player.max_hp - combat.player.hp == DAMAGE[turn]);
        assert(beetle.block == 0 && beetle.statuses.empty() && beetle.hp == hp);
    }
    assert(combat.power_amount(PowerId::SHRINK) == 0);

    // 30% less, rounded down, after the rest: a Strike of 6 deals 4.
    CombatEngine strike = make_combat(CharacterId::IRONCLAD, EnemyType::SHRINKER_BEETLE);
    strike.end_player_turn();
    strike.enemies[0].block = 0;
    const int16_t full = strike.enemies[0].hp;
    strike.hit(strike.enemies[0], 6);
    assert(full - strike.enemies[0].hp == 4);

    // Artifact keeps the player from being Shrunk.
    CombatEngine guarded = make_combat(CharacterId::IRONCLAD, EnemyType::SHRINKER_BEETLE);
    guarded.player.add_status(StatusType::ARTIFACT, 1);
    guarded.end_player_turn();
    assert(!guarded.has_power(PowerId::SHRINK) && guarded.player.get_status(StatusType::ARTIFACT) == 0);
    std::cout << "[PASS] Shrinker Beetle: 38 to 40 HP, Shrinker (-30% attack damage, 3 turns), Chomp 7, Stomp 13"
              << std::endl;
}

void test_shrinker_beetle_ascension() {
    // 40 to 42 HP, Chomp 8 and Stomp 14 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SHRINKER_BEETLE);
        Enemy& beetle = combat.enemies[0];
        beetle.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        const int16_t base = beetle.hp;
        assert(beetle.ascension_hp() == 40 && base >= 38 && base <= 40);
        beetle.ascend(true, true);
        combat.plan_enemies();
        assert(beetle.hp == base + 2 && beetle.max_hp == base + 2 && beetle.hp >= 40 && beetle.hp <= 42);
        assert(beetle.current_intent.type == DEBUFF);
        combat.player.hp = combat.player.max_hp = 500;
        combat.end_player_turn();
        assert(beetle.current_intent.type == ATTACK && beetle.intent_damage(combat.player) == 8);
        combat.end_player_turn();
        assert(beetle.current_intent.type == ATTACK && beetle.intent_damage(combat.player) == 14);
    }
    std::cout << "[PASS] Shrinker Beetle: Ascension gives 40 to 42 HP, Chomp 8, Stomp 14" << std::endl;
}

void test_owl_magistrate() {
    // Magistrate's Scrutiny 16, Peck Assault 4x6, Judicial Flight (Soar), Verdict 33 with 4 Vulnerable, and again.
    check_monster(EnemyType::OWL_MAGISTRATE, "Magistrix", 231, {
        { ATTACK, 16, 0, 0 },
        { ATTACK, 4, 0, 0, 0, 6 },
        { BUFF, 0, 0, 0 },
        { ATTACK_DEBUFF, 33, 0, 0 },
    });

    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::OWL_MAGISTRATE);
    Enemy& owl = combat.enemies[0];
    combat.player.max_hp = combat.player.hp = 500;
    // The damage the player's attack of `base` deals to the owl.
    auto dealt = [&](int16_t base) {
        owl.block = 0;
        const int16_t before = owl.hp;
        combat.hit(owl, base);
        const int lost = before - owl.hp;
        owl.hp = before;
        return lost;
    };
    combat.end_player_turn(); // Scrutiny
    combat.end_player_turn(); // Peck Assault
    assert(owl.current_intent.type == BUFF && owl.current_intent.power == PowerId::SOAR);
    assert(!owl.current_intent.power_on_player && dealt(10) == 10);
    combat.end_player_turn(); // Judicial Flight: it Soars, attacks deal it half their damage
    assert(combat.enemy_power_amount(0, PowerId::SOAR) == 1 && !combat.has_power(PowerId::SHRINK));
    assert(dealt(10) == 5 && dealt(7) == 3);
    // Verdict: it lands when its turn starts, then strikes for 33 and gives 4 Vulnerable.
    assert(owl.current_intent.type == ATTACK_DEBUFF && owl.intent_damage(combat.player) == 33);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 33);
    assert(combat.enemy_power_amount(0, PowerId::SOAR) == 0 && dealt(10) == 10);
    assert(combat.player.get_status(StatusType::VULNERABLE) == 4);
    // And again: Scrutiny on a Vulnerable player.
    assert(owl.current_intent.type == ATTACK && owl.intent_damage(combat.player) == 24);
    assert(owl.hp == 231 && owl.statuses.empty());
    std::cout << "[PASS] Owl Magistrate: 231 HP, Scrutiny 16, Peck Assault 4x6, Soar (half damage until it lands), "
                 "Verdict 33 + 4 Vulnerable"
              << std::endl;
}

void test_owl_magistrate_ascension() {
    // 247 HP, Scrutiny 17 and Verdict 36 at Ascension; Peck Assault stays 4x6.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::OWL_MAGISTRATE);
    Enemy& owl = combat.enemies[0];
    assert(owl.ascension_hp() == 247);
    owl.ascend(true, true);
    combat.plan_enemies();
    assert(owl.hp == 247 && owl.max_hp == 247);
    combat.player.hp = combat.player.max_hp = 500;
    assert(owl.current_intent.type == ATTACK && owl.intent_damage(combat.player) == 17);
    combat.end_player_turn();
    assert(owl.intent_damage(combat.player) == 4 && owl.current_intent.hits == 6);
    combat.end_player_turn();
    assert(owl.current_intent.type == BUFF);
    combat.end_player_turn();
    assert(owl.current_intent.type == ATTACK_DEBUFF && owl.intent_damage(combat.player) == 36);
    std::cout << "[PASS] Owl Magistrate: Ascension gives 247 HP, Scrutiny 17, Verdict 36" << std::endl;
}

void test_scroll_of_biting() {
    // Chomp 14, More Teeth (Strength 2), Chew 5x2 (7x2 with the Strength): one full cycle of its three moves.
    const int hp = make_combat(CharacterId::IRONCLAD, EnemyType::SCROLL_OF_BITING).enemies[0].hp;
    assert(hp >= 30 && hp <= 37);
    check_monster(EnemyType::SCROLL_OF_BITING, "Parchemord", hp, {
        { ATTACK, 14, 0, 0 },
        { BUFF, 0, 0, 2 },
        { ATTACK, 7, 0, 2, 0, 2 },
    });

    // Then a draw between Chomp and Chew each turn: Chomp never twice in a row, Chew as often as drawn.
    bool chomp = false, chew_twice = false;
    for (uint32_t i = 0; i < 40; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SCROLL_OF_BITING);
        Enemy& scroll = combat.enemies[0];
        scroll.reset_pattern(i * 7919 + 1);
        scroll.plan_turn(1);
        scroll.roll_hp(i * 104729 + 3);
        assert(scroll.hp >= 30 && scroll.hp <= 37 && scroll.hp_spread() == 7 && scroll.max_hp == 30 + scroll.hp_bonus);
        combat.player.max_hp = 500;
        bool last_chew = false;
        bool last_chomp = false;
        for (int turn = 1; turn <= 10; ++turn) {
            const EnemyIntent& intent = scroll.current_intent;
            if (turn == 2) {
                assert(intent.type == BUFF && intent.status_type == StatusType::STRENGTH && intent.status_amount == 2);
            } else {
                assert(intent.type == ATTACK);
                const bool is_chomp = intent.hits == 1;
                assert(is_chomp ? scroll.intent_damage(combat.player) == 14 + 2 * (turn > 2)
                                : intent.hits == 2 && scroll.intent_damage(combat.player) == 7);
                if (turn == 1) assert(is_chomp);
                if (turn == 3) assert(!is_chomp);
                if (turn > 3) {
                    assert(!(is_chomp && last_chomp));
                    chomp = chomp || is_chomp;
                    chew_twice = chew_twice || (!is_chomp && last_chew);
                }
                last_chomp = is_chomp;
                last_chew = !is_chomp;
            }
            if (turn == 2) last_chomp = last_chew = false;
            combat.player.hp = combat.player.max_hp;
            combat.end_player_turn();
        }
        assert(scroll.get_status(StatusType::STRENGTH) == 2);
    }
    assert(chomp && chew_twice);
    std::cout << "[PASS] Scroll of Biting: 30 to 37 HP, Chomp 14, More Teeth (+2 Strength), Chew 5x2, then Chomp "
                 "(never twice) or Chew"
              << std::endl;
}

void test_scroll_of_biting_ascension() {
    // 33 HP and more at Ascension, Chomp 16 and Chew 6x2.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SCROLL_OF_BITING);
    Enemy& scroll = combat.enemies[0];
    scroll.roll_hp(1);
    const int bonus = scroll.hp_bonus;
    assert(scroll.ascension_hp() == 33);
    scroll.ascend(true, true);
    combat.plan_enemies();
    assert(scroll.hp == 33 + bonus && scroll.max_hp == scroll.hp);
    combat.player.hp = combat.player.max_hp = 500;
    assert(scroll.current_intent.type == ATTACK && scroll.intent_damage(combat.player) == 16);
    combat.end_player_turn();
    assert(scroll.current_intent.type == BUFF);
    combat.end_player_turn();
    assert(scroll.current_intent.type == ATTACK && scroll.current_intent.hits == 2);
    assert(scroll.intent_damage(combat.player) == 6 + 2);
    std::cout << "[PASS] Scroll of Biting: Ascension gives 33 HP and more, Chomp 16, Chew 6x2" << std::endl;
}

// Slimed cards anywhere in the player's piles.
int slimed_total(const CombatEngine& combat) {
    return count_in(combat.draw_pile, CardId::SLIMED) + count_in(combat.hand, CardId::SLIMED) +
           count_in(combat.discard_pile, CardId::SLIMED);
}

void test_fogmog() {
    // Illusion Spores (an Eye With Teeth joins as its Minion), Swipe 8 (+1 Strength), then Swipe or Headbutt 14.
    check_monster(EnemyType::FOGMOG, "Fongibrume", 74, {
        { SUMMON, 0, 0, 0 },
        { ATTACK_BUFF, 8, 0, 1 },
    });
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FOGMOG);
    Enemy& fog = combat.enemies[0];
    assert(fog.current_intent.summon_type == EnemyType::EYE_WITH_TEETH && fog.current_intent.summons == 1);
    combat.player.max_hp = combat.player.hp = 500;
    combat.end_player_turn();
    assert(combat.count_alive_enemies() == 2 && combat.enemies[1].type == EnemyType::EYE_WITH_TEETH);
    assert(combat.enemies[1].minion && combat.enemies[1].leader == 0);

    bool swipe = false, headbutt = false;
    for (uint32_t i = 0; i < 40; ++i) {
        CombatEngine run = make_combat(CharacterId::IRONCLAD, EnemyType::FOGMOG);
        Enemy& boss = run.enemies[0];
        boss.reset_pattern(i * 7919 + 1);
        boss.plan_turn(1);
        run.player.max_hp = 30000;
        run.player.hp = run.player.max_hp;
        run.end_player_turn();
        run.end_player_turn();
        int strength = 1;
        bool last_headbutt = false, last_swipe = false;
        for (int turn = 3; turn <= 12; ++turn) {
            const EnemyIntent& intent = boss.current_intent;
            const bool is_headbutt = intent.type == ATTACK;
            assert(is_headbutt || intent.type == ATTACK_BUFF);
            assert(boss.intent_damage(run.player) == (is_headbutt ? 14 : 8) + strength);
            assert(!(is_headbutt && last_headbutt) && !(!is_headbutt && last_swipe));
            swipe = swipe || !is_headbutt;
            headbutt = headbutt || is_headbutt;
            if (!is_headbutt) ++strength;
            last_headbutt = is_headbutt;
            last_swipe = !is_headbutt;
            run.end_player_turn();
            assert(boss.get_status(StatusType::STRENGTH) == strength);
        }
    }
    assert(swipe && headbutt);

    // Ascension: 78 HP, Swipe 9 and Headbutt 16.
    CombatEngine asc = make_combat(CharacterId::IRONCLAD, EnemyType::FOGMOG);
    Enemy& ascended = asc.enemies[0];
    assert(ascended.ascension_hp() == 78);
    ascended.ascend(true, true);
    asc.plan_enemies();
    assert(ascended.hp == 78 && ascended.max_hp == 78 && ascended.move_damage("HEADBUTT") == 16);
    asc.player.hp = asc.player.max_hp = 500;
    asc.end_player_turn();
    assert(ascended.current_intent.type == ATTACK_BUFF && ascended.intent_damage(asc.player) == 9);
    std::cout << "[PASS] Fogmog: 74 HP (78), Illusion Spores brings an Eye With Teeth, Swipe 8 (+1 Strength), Headbutt 14 (16)"
              << std::endl;
}

void test_snapping_jaxfruit() {
    // Energy Orb every turn: 3 damage and +2 Strength.
    const int hp = make_combat(CharacterId::IRONCLAD, EnemyType::SNAPPING_JAXFRUIT).enemies[0].hp;
    assert(hp >= 31 && hp <= 33);
    check_monster(EnemyType::SNAPPING_JAXFRUIT, "Jaxier à clapet", hp, {
        { ATTACK_BUFF, 3, 0, 2 },
        { ATTACK_BUFF, 5, 0, 4 },
        { ATTACK_BUFF, 7, 0, 6 },
    });
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SNAPPING_JAXFRUIT);
        Enemy& fruit = combat.enemies[0];
        fruit.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        const int16_t base = fruit.hp;
        assert(base >= 31 && base <= 33 && fruit.hp_spread() == 2 && fruit.ascension_hp() == 34);
        fruit.ascend(true, true);
        combat.plan_enemies();
        assert(fruit.hp == base + 3 && fruit.hp >= 34 && fruit.hp <= 36);
        assert(fruit.current_intent.type == ATTACK_BUFF && fruit.intent_damage(combat.player) == 4);
    }
    std::cout << "[PASS] Snapping Jaxfruit: 31 to 33 HP (34 to 36), Energy Orb 3 (4) and +2 Strength" << std::endl;
}

void test_twig_slime_s() {
    const int hp = make_combat(CharacterId::IRONCLAD, EnemyType::TWIG_SLIME_S).enemies[0].hp;
    assert(hp >= 7 && hp <= 11);
    check_monster(EnemyType::TWIG_SLIME_S, "Limon brindille (S)", hp, {
        { ATTACK, 4, 0, 0 },
        { ATTACK, 4, 0, 0 },
        { ATTACK, 4, 0, 0 },
    });
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TWIG_SLIME_S);
    Enemy& slime = combat.enemies[0];
    slime.roll_hp(5);
    assert(slime.hp_spread() == 4 && slime.ascension_hp() == 8);
    slime.ascend(true, true);
    combat.plan_enemies();
    assert(slime.hp >= 8 && slime.hp <= 12 && slime.intent_damage(combat.player) == 5);
    std::cout << "[PASS] Twig Slime (S): 7 to 11 HP (8 to 12), Tackle 4 (5)" << std::endl;
}

void test_twig_slime_m() {
    // Sticky Shot (1 Slimed), then Pokey Pounce 11 (as often as drawn) or Sticky Shot (never twice).
    const int hp = make_combat(CharacterId::IRONCLAD, EnemyType::TWIG_SLIME_M).enemies[0].hp;
    assert(hp >= 26 && hp <= 28);
    check_monster(EnemyType::TWIG_SLIME_M, "Limon brindille (M)", hp, {
        { STATUS, 0, 0, 0 },
    });
    bool pounce_twice = false, sticky_again = false;
    for (uint32_t i = 0; i < 40; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TWIG_SLIME_M);
        Enemy& slime = combat.enemies[0];
        slime.reset_pattern(i * 7919 + 1);
        slime.plan_turn(1);
        combat.player.max_hp = 30000;
        combat.player.hp = combat.player.max_hp;
        assert(slime.current_intent.type == STATUS && slime.current_intent.status_card == CardId::SLIMED);
        assert(slime.current_intent.status_cards == 1);
        int slimed = 0;
        bool last_pounce = false;
        for (int turn = 1; turn <= 12; ++turn) {
            const EnemyIntent intent = slime.current_intent;
            if (turn == 2) assert(intent.type == ATTACK);
            if (intent.type == STATUS) {
                assert(turn == 1 || last_pounce);
                if (turn > 1) sticky_again = true;
                ++slimed;
                last_pounce = false;
            } else {
                assert(intent.type == ATTACK && slime.intent_damage(combat.player) == 11);
                pounce_twice = pounce_twice || last_pounce;
                last_pounce = true;
            }
            combat.end_player_turn();
            assert(slimed_total(combat) == slimed);
        }
    }
    assert(pounce_twice && sticky_again);

    CombatEngine asc = make_combat(CharacterId::IRONCLAD, EnemyType::TWIG_SLIME_M);
    asc.enemies[0].ascend(true, true);
    asc.plan_enemies();
    assert(asc.enemies[0].ascension_hp() == 27 && asc.enemies[0].hp >= 27 && asc.enemies[0].hp <= 29);
    asc.player.hp = asc.player.max_hp = 500;
    asc.end_player_turn();
    assert(asc.enemies[0].intent_damage(asc.player) == 12);
    std::cout << "[PASS] Twig Slime (M): 26 to 28 HP (27 to 29), Sticky Shot (1 Slimed), Pokey Pounce 11 (12)"
              << std::endl;
}

void test_leaf_slime_s() {
    // A draw between Tackle 3 and Goop (1 Slimed), never the same twice in a row.
    bool tackle = false, goop = false;
    for (uint32_t i = 0; i < 40; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::LEAF_SLIME_S);
        Enemy& slime = combat.enemies[0];
        assert(slime.name == "Limon feuillu (S)" && slime.hp >= 11 && slime.hp <= 15 && slime.hp_spread() == 4);
        slime.reset_pattern(i * 7919 + 1);
        slime.plan_turn(1);
        combat.player.max_hp = 30000;
        combat.player.hp = combat.player.max_hp;
        int slimed = 0;
        bool last_goop = false;
        for (int turn = 1; turn <= 10; ++turn) {
            const bool is_goop = slime.current_intent.type == STATUS;
            if (is_goop) {
                assert(slime.current_intent.status_card == CardId::SLIMED && slime.current_intent.status_cards == 1);
                ++slimed;
            } else {
                assert(slime.current_intent.type == ATTACK && slime.intent_damage(combat.player) == 3);
            }
            if (turn > 1) assert(is_goop != last_goop);
            tackle = tackle || !is_goop;
            goop = goop || is_goop;
            last_goop = is_goop;
            combat.end_player_turn();
            assert(slimed_total(combat) == slimed);
        }
    }
    assert(tackle && goop);
    CombatEngine asc = make_combat(CharacterId::IRONCLAD, EnemyType::LEAF_SLIME_S);
    asc.enemies[0].ascend(true, true);
    asc.plan_enemies();
    assert(asc.enemies[0].ascension_hp() == 12 && asc.enemies[0].hp >= 12 && asc.enemies[0].hp <= 16);
    assert(asc.enemies[0].move_damage("TACKLE") == 4);
    std::cout << "[PASS] Leaf Slime (S): 11 to 15 HP (12 to 16), Tackle 3 (4) or Goop (1 Slimed), never twice" << std::endl;
}

void test_leaf_slime_m() {
    // Sticky Shot (2 Slimed), Clump Shot 8, and again.
    const int hp = make_combat(CharacterId::IRONCLAD, EnemyType::LEAF_SLIME_M).enemies[0].hp;
    assert(hp >= 32 && hp <= 35);
    check_monster(EnemyType::LEAF_SLIME_M, "Limon feuillu (M)", hp, {
        { STATUS, 0, 0, 0 },
        { ATTACK, 8, 0, 0 },
        { STATUS, 0, 0, 0 },
        { ATTACK, 8, 0, 0 },
    });
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::LEAF_SLIME_M);
    combat.player.max_hp = combat.player.hp = 500;
    assert(combat.enemies[0].current_intent.status_card == CardId::SLIMED && combat.enemies[0].current_intent.status_cards == 2);
    combat.end_player_turn();
    assert(slimed_total(combat) == 2);
    combat.end_player_turn();
    combat.end_player_turn();
    assert(slimed_total(combat) == 4);

    CombatEngine asc = make_combat(CharacterId::IRONCLAD, EnemyType::LEAF_SLIME_M);
    asc.enemies[0].ascend(true, true);
    asc.plan_enemies();
    assert(asc.enemies[0].ascension_hp() == 33 && asc.enemies[0].hp >= 33 && asc.enemies[0].hp <= 36);
    asc.player.hp = asc.player.max_hp = 500;
    asc.end_player_turn();
    assert(asc.enemies[0].intent_damage(asc.player) == 9);
    std::cout << "[PASS] Leaf Slime (M): 32 to 35 HP (33 to 36), Sticky Shot (2 Slimed), Clump Shot 8 (9)" << std::endl;
}

void test_slithering_strangler() {
    // Constrict (3 damage at the end of each of the player's turns while it lives), then Thwack 7 + 5 Block or Lash 12.
    const int hp = make_combat(CharacterId::IRONCLAD, EnemyType::SLITHERING_STRANGLER).enemies[0].hp;
    assert(hp >= 53 && hp <= 55);
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SLITHERING_STRANGLER);
    Enemy& strangler = combat.enemies[0];
    assert(strangler.name == "Étrangleur" && strangler.hp_spread() == 2);
    combat.player.max_hp = combat.player.hp = 500;
    assert(strangler.current_intent.type == DEBUFF && strangler.current_intent.power == PowerId::CONSTRICT);
    assert(strangler.current_intent.power_on_player && strangler.intent_damage(combat.player) == 0);
    combat.end_player_turn();
    assert(combat.player.hp == 500 && combat.power_amount(PowerId::CONSTRICT) == 3);

    bool thwack = false, lash = false, thwack_twice = false;
    bool last_thwack = false;
    for (int turn = 2; turn <= 40; ++turn) {
        const EnemyIntent intent = strangler.current_intent;
        const bool is_thwack = intent.type == ATTACK_DEFEND;
        const int damage = is_thwack ? 7 : 12;
        assert(is_thwack || intent.type == ATTACK);
        assert(strangler.intent_damage(combat.player) == damage);
        thwack = thwack || is_thwack;
        lash = lash || !is_thwack;
        thwack_twice = thwack_twice || (is_thwack && last_thwack);
        last_thwack = is_thwack;
        combat.player.hp = 500;
        combat.end_player_turn();
        // The Constrict of the end of the player's turn, then the strangler's move.
        assert(500 - combat.player.hp == 3 + damage);
        assert(strangler.block == (is_thwack ? 5 : 0));
        assert(strangler.statuses.empty());
    }
    assert(thwack && lash && thwack_twice);

    // Without the Strangler, Constrict does nothing; Artifact keeps the player from being constricted.
    CombatEngine free_player = make_combat(CharacterId::IRONCLAD, EnemyType::SLITHERING_STRANGLER);
    free_player.end_player_turn();
    free_player.enemies[0].hp = 0;
    free_player.player.hp = 500;
    free_player.player.max_hp = 500;
    free_player.end_player_turn();
    assert(free_player.player.hp == 500);
    CombatEngine guarded = make_combat(CharacterId::IRONCLAD, EnemyType::SLITHERING_STRANGLER);
    guarded.player.add_status(StatusType::ARTIFACT, 1);
    guarded.end_player_turn();
    assert(!guarded.has_power(PowerId::CONSTRICT) && guarded.player.get_status(StatusType::ARTIFACT) == 0);

    // Ascension: 54 to 56 HP, Thwack 8, Lash 13.
    CombatEngine asc = make_combat(CharacterId::IRONCLAD, EnemyType::SLITHERING_STRANGLER);
    Enemy& ascended = asc.enemies[0];
    assert(ascended.ascension_hp() == 54);
    ascended.ascend(true, true);
    asc.plan_enemies();
    assert(ascended.hp >= 54 && ascended.hp <= 56);
    assert(ascended.move_damage("THWACK") == 8 && ascended.move_damage("LASH") == 13);
    std::cout << "[PASS] Slithering Strangler: 53 to 55 HP (54 to 56), Constrict 3, Thwack 7 + 5 Block (8), Lash 12 (13)"
              << std::endl;
}

void test_living_fog() {
    // Advanced Gas 8 (Smoggy), then Bloat 5 (a Gas Bomb joins) and Super Gas Blast 8, Bloat and Super Gas Blast...
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::LIVING_FOG);
    Enemy& fog = combat.enemies[0];
    assert(fog.name == "Brouillard vivant" && fog.hp == 80 && fog.max_hp == 80 && fog.hp_spread() == 0);
    combat.player.max_hp = combat.player.hp = 500;
    assert(fog.current_intent.type == ATTACK_DEBUFF && fog.intent_damage(combat.player) == 8);
    assert(fog.current_intent.power == PowerId::SMOGGY && fog.current_intent.power_on_player);
    combat.end_player_turn();
    assert(combat.player.hp == 492 && combat.has_power(PowerId::SMOGGY) && combat.count_alive_enemies() == 1);
    for (int cycle = 0; cycle < 2; ++cycle) {
        const int bombs = combat.count_alive_enemies() - 1;
        assert(fog.current_intent.type == ATTACK && fog.intent_damage(combat.player) == 5);
        assert(fog.current_intent.summon_type == EnemyType::GAS_BOMB && fog.current_intent.summons == 1);
        combat.player.hp = 500;
        combat.end_player_turn();
        // The new bomb has not acted yet; the earlier ones explode (8 each).
        assert(500 - combat.player.hp == 5 + 8 * bombs);
        assert(combat.count_alive_enemies() == bombs + 2);
        assert(combat.enemies[combat.enemies.size() - 1].type == EnemyType::GAS_BOMB);
        assert(combat.enemies[combat.enemies.size() - 1].minion);
        assert(fog.current_intent.type == ATTACK && fog.intent_damage(combat.player) == 8 &&
               fog.current_intent.summons == 0);
        combat.player.hp = 500;
        combat.end_player_turn();
        assert(500 - combat.player.hp == 8 + 8 * (bombs + 1));
        assert(fog.block == 0 && fog.statuses.empty() && fog.hp == 80);
    }

    // Smoggy: only one Skill each turn. Attacks are not limited.
    CombatEngine smog = make_combat(CharacterId::IRONCLAD, EnemyType::LIVING_FOG);
    smog.end_player_turn();
    smog.hand.clear();
    smog.hand.push_back(CardId::DEFEND);
    smog.hand.push_back(CardId::DEFEND);
    smog.hand.push_back(CardId::STRIKE);
    smog.hand.push_back(CardId::STRIKE);
    smog.player.energy = 10;
    assert(smog.can_play_card(0) && smog.can_play_card(1));
    assert(smog.play_card(0, 0));
    assert(smog.play_blocker(0) == PlayBlocker::PLAY_LIMIT && !smog.play_card(0, 0));
    assert(smog.can_play_card(1) && smog.play_card(1, 0) && smog.can_play_card(1) && smog.play_card(1, 0));
    smog.end_player_turn();
    smog.hand.clear();
    smog.hand.push_back(CardId::DEFEND);
    smog.player.energy = 10;
    assert(smog.can_play_card(0));
    CombatEngine guarded = make_combat(CharacterId::IRONCLAD, EnemyType::LIVING_FOG);
    guarded.player.add_status(StatusType::ARTIFACT, 1);
    guarded.end_player_turn();
    assert(!guarded.has_power(PowerId::SMOGGY));

    // Ascension: 82 HP, Advanced Gas 9, Bloat 6, Super Gas Blast 9.
    CombatEngine asc = make_combat(CharacterId::IRONCLAD, EnemyType::LIVING_FOG);
    Enemy& ascended = asc.enemies[0];
    ascended.ascend(true, true);
    asc.plan_enemies();
    assert(ascended.hp == 82 && ascended.max_hp == 82 && ascended.intent_damage(asc.player) == 9);
    assert(ascended.move_damage("BLOAT") == 6 && ascended.move_damage("SUPER_GAS_BLAST") == 9);
    std::cout << "[PASS] Living Fog: 80 HP (82), Advanced Gas 8 (Smoggy), Bloat 5 (Gas Bomb), Super Gas Blast 8"
              << std::endl;
}

void test_two_tailed_rat() {
    // A draw among Scratch 8, Disease Bite 6, Screech (1 Frail) and Call for Backup (a rat joins, once per rat),
    // never the same move twice in a row.
    bool scratch = false, bite = false, screech = false, call = false;
    for (uint32_t i = 0; i < 60; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TWO_TAILED_RAT);
        Enemy& rat = combat.enemies[0];
        assert(rat.name == "Rat bicaudé" && rat.hp >= 17 && rat.hp <= 21 && rat.hp_spread() == 4);
        rat.reset_pattern(i * 7919 + 1);
        rat.plan_turn(1);
        combat.player.max_hp = 30000;
        combat.player.hp = combat.player.max_hp;
        int calls = 0;
        std::string_view last;
        for (int turn = 1; turn <= 12; ++turn) {
            const EnemyIntent intent = rat.current_intent;
            assert(intent.description != last);
            last = intent.description;
            const size_t before = combat.enemies.size();
            combat.player.hp = combat.player.max_hp;
            if (intent.type == SUMMON) {
                ++calls;
                call = true;
                assert(intent.summon_type == EnemyType::TWO_TAILED_RAT && intent.summons == 1 && !intent.summon_minions);
                assert(rat.intent_damage(combat.player) == 0);
            } else if (intent.type == DEBUFF) {
                screech = true;
                assert(intent.debuffs[0].type == StatusType::FRAIL && intent.debuffs[0].amount == 1);
            } else {
                assert(intent.type == ATTACK);
                const int damage = rat.intent_damage(combat.player);
                assert(damage == 8 || damage == 6);
                (damage == 8 ? scratch : bite) = true;
            }
            combat.end_player_turn();
            if (intent.type == SUMMON) assert(combat.enemies.size() == before + 1 && !combat.enemies[before].minion);
            if (intent.type == DEBUFF) assert(combat.player.get_status(StatusType::FRAIL) > 0);
            assert(rat.statuses.empty() && rat.block == 0);
        }
        assert(calls <= 1);
    }
    assert(scratch && bite && screech && call);

    CombatEngine asc = make_combat(CharacterId::IRONCLAD, EnemyType::TWO_TAILED_RAT);
    Enemy& ascended = asc.enemies[0];
    assert(ascended.ascension_hp() == 18);
    ascended.ascend(true, true);
    asc.plan_enemies();
    assert(ascended.hp >= 18 && ascended.hp <= 22);
    assert(ascended.move_damage("SCRATCH") == 9 && ascended.move_damage("DISEASE_BITE") == 7);
    std::cout << "[PASS] Two-Tailed Rat: 17 to 21 HP (18 to 22), Scratch 8 (9), Disease Bite 6 (7), Screech (Frail), Call for "
                 "Backup once"
              << std::endl;
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

void test_louse_progenitor() {
    // Web Cannon 9 + 2 Frail, Curl and Grow (14 Block, +5 Strength), Pounce 14, and again with the Strength kept.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::LOUSE_PROGENITOR);
    Enemy& louse = combat.enemies[0];
    assert(louse.type == EnemyType::LOUSE_PROGENITOR && louse.name == "Progéniteur de poux");
    assert(louse.hp >= 134 && louse.hp <= 136 && louse.max_hp == 134 + louse.hp_bonus && louse.block == 0);
    struct Step { EnemyIntentType intent; int damage; int block; int strength; int frail; };
    const Step steps[] = {
        { ATTACK_DEBUFF, 9, 0, 0, 2 }, { BUFF, 0, 14, 5, 0 }, { ATTACK, 19, 0, 5, 0 },
        { ATTACK_DEBUFF, 14, 0, 5, 2 }, { BUFF, 0, 14, 10, 0 }, { ATTACK, 24, 0, 10, 0 },
    };
    for (const Step& step : steps) {
        assert(louse.current_intent.type == step.intent);
        assert(louse.intent_damage(combat.player) == step.damage);
        combat.player.hp = combat.player.max_hp;
        combat.player.clear_status(StatusType::FRAIL);
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == step.damage);
        assert(combat.player.get_status(StatusType::FRAIL) == step.frail);
        assert(louse.block == step.block && louse.get_status(StatusType::STRENGTH) == step.strength);
    }
    std::cout << "[PASS] Louse Progenitor: 134 to 136 HP, Web Cannon 9 + 2 Frail, Curl and Grow 14 Block +5 Strength, Pounce 14" << std::endl;
}

void test_louse_progenitor_ascension() {
    // 138 to 140 HP, Web Cannon 10, Curl and Grow 18 Block and +7 Strength, Pounce 16 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::LOUSE_PROGENITOR);
        Enemy& louse = combat.enemies[0];
        louse.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = louse.hp;
        assert(louse.ascension_hp() == 138 && louse.hp_spread() == 2);
        assert(base >= 134 && base <= 136 && louse.max_hp == 134 + louse.hp_bonus);
        louse.ascend(true, true);
        combat.plan_enemies();
        assert(louse.hp == base + 4 && louse.hp >= 138 && louse.hp <= 140);
        assert(louse.intent_damage(combat.player) == 10);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(louse.current_intent.type == BUFF);
        combat.end_player_turn();
        assert(louse.block == 18 && louse.get_status(StatusType::STRENGTH) == 7);
        combat.player.hp = combat.player.max_hp;
        assert(louse.current_intent.type == ATTACK && louse.intent_damage(combat.player) == 16 + 7);
    }
    std::cout << "[PASS] Louse Progenitor: Ascension gives 138 to 140 HP, Web Cannon 10, 18 Block +7 Strength, Pounce 16" << std::endl;
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

void test_spectral_knight() {
    // 93 HP. Hex (every card Ethereal) once, Soul Slash 15, then Soul Slash or Soul Flame 3 x3 at random,
    // Soul Flame never twice in a row.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SPECTRAL_KNIGHT);
    Enemy& knight = combat.enemies[0];
    assert(knight.name == "Chevalier spectral" && knight.hp == 93 && knight.max_hp == 93 && knight.hp_spread() == 0);
    assert(knight.current_intent.type == DEBUFF && knight.intent_damage(combat.player) == 0);
    assert(combat.enemy_power_amount(0, PowerId::HEX) == 0);
    combat.end_player_turn();
    assert(combat.enemy_power_amount(0, PowerId::HEX) == 2 && combat.player.hp == combat.player.max_hp);
    assert(knight.current_intent.type == ATTACK && knight.intent_damage(combat.player) == 15 && knight.current_intent.hits == 1);
    // Hex makes every card Ethereal: the hand is exhausted at the end of the turn instead of discarded.
    combat.player.hp = combat.player.max_hp;
    const size_t exhausted_before = combat.exhaust_pile.size();
    assert(!combat.hand.empty());
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 15);
    assert(combat.exhaust_pile.size() > exhausted_before);
    bool saw_slash = false;
    bool saw_flame = false;
    bool last_was_flame = false;
    for (int turn = 3; turn <= 40; ++turn) {
        const bool flame = knight.current_intent.hits == 3;
        assert(knight.current_intent.type == ATTACK && knight.block == 0);
        assert(knight.intent_damage(combat.player) == (flame ? 3 : 15));
        assert(!(flame && last_was_flame));
        saw_flame = saw_flame || flame;
        saw_slash = saw_slash || !flame;
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == (flame ? 9 : 15));
        last_was_flame = flame;
    }
    assert(saw_slash && saw_flame);
    std::cout << "[PASS] Spectral Knight: 93 HP, Hex, Soul Slash 15, then Slash or Flame 3 x3 without repeating the Flame" << std::endl;
}

void test_spectral_knight_ascension() {
    // 97 HP, Soul Slash 17 and Soul Flame 4 x3 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SPECTRAL_KNIGHT);
    Enemy& knight = combat.enemies[0];
    assert(knight.ascension_hp() == 97);
    knight.ascend(true, true);
    combat.plan_enemies();
    assert(knight.hp == 97 && knight.max_hp == 97);
    assert(knight.current_intent.type == DEBUFF);
    combat.end_player_turn();
    assert(knight.current_intent.type == ATTACK && knight.intent_damage(combat.player) == 17);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 17);
    for (int turn = 3; turn <= 20; ++turn) {
        const bool flame = knight.current_intent.hits == 3;
        assert(knight.intent_damage(combat.player) == (flame ? 4 : 17));
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == (flame ? 12 : 17));
    }
    std::cout << "[PASS] Spectral Knight: Ascension gives 97 HP, Soul Slash 17 and Soul Flame 4 x3" << std::endl;
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

void test_osty() {
    // Titos, summoned by the Entomancer: 1 HP and no move, so it does nothing turn after turn.
    check_monster(EnemyType::OSTY, "Titos", 1, {
        { STUN, 0, 0, 0 }, { STUN, 0, 0, 0 }, { STUN, 0, 0, 0 }, { STUN, 0, 0, 0 },
    });
    std::cout << "[PASS] Titos: 1 HP, does nothing every turn" << std::endl;
}

void test_osty_ascension() {
    // The bible gives no Ascension value: HP stays at 1 and the turn stays empty.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::OSTY);
    Enemy& osty = combat.enemies[0];
    assert(osty.ascension_hp() == 0 && osty.hp_spread() == 0);
    osty.ascend(true, true);
    combat.plan_enemies();
    assert(osty.hp == 1 && osty.max_hp == 1);
    assert(osty.current_intent.type == STUN && osty.intent_damage(combat.player) == 0);
    std::cout << "[PASS] Titos: Ascension changes neither HP nor behaviour" << std::endl;
}

void test_chomper() {
    // Clamp 8 x2, Screech (3 Dazed into the discard pile), and again.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::CHOMPER);
    Enemy& chomper = combat.enemies[0];
    assert(chomper.type == EnemyType::CHOMPER && chomper.name == "Croquebot");
    assert(chomper.hp >= 60 && chomper.hp <= 64 && chomper.max_hp == chomper.hp && chomper.max_hp == 60 + chomper.hp_bonus);
    assert(chomper.block == 0 && chomper.statuses.empty());
    for (int turn = 1; turn <= 6; ++turn) {
        assert(combat.turn_number == turn);
        combat.player.hp = combat.player.max_hp;
        if (turn % 2 == 1) {
            assert(chomper.current_intent.type == ATTACK && chomper.current_intent.hits == 2);
            assert(chomper.intent_damage(combat.player) == 8);
            combat.end_player_turn();
            assert(combat.player.max_hp - combat.player.hp == 16);
        } else {
            assert(chomper.current_intent.type == STATUS && chomper.current_intent.status_card == CardId::DAZED);
            assert(chomper.current_intent.status_cards == 3 && chomper.current_intent.status_pile == Pile::DISCARD);
            combat.end_player_turn();
            assert(combat.player.hp == combat.player.max_hp);
        }
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(chomper.block == 0 && chomper.statuses.empty());
    }
    std::cout << "[PASS] Chomper: 60 to 64 HP, Clamp 8 x2 then Screech (3 Dazed) in a loop" << std::endl;
}

void test_chomper_ascension() {
    // 63 to 67 HP (60 to 64 plus 3) and Clamp 9 x2 at Ascension.
    for (int i = 0; i < 20; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::CHOMPER);
        Enemy& chomper = combat.enemies[0];
        chomper.roll_hp(static_cast<uint32_t>(i * 7919 + 1));
        int16_t base = chomper.hp;
        assert(chomper.ascension_hp() == 63 && chomper.hp_spread() == 4);
        assert(base >= 60 && base <= 64 && chomper.max_hp == 60 + chomper.hp_bonus);
        chomper.ascend(true, true);
        combat.plan_enemies();
        assert(chomper.hp == base + 3 && chomper.max_hp == base + 3 && chomper.hp >= 63 && chomper.hp <= 67);
        assert(chomper.current_intent.type == ATTACK && chomper.current_intent.hits == 2);
        assert(chomper.intent_damage(combat.player) == 9);
    }
    std::cout << "[PASS] Chomper: Ascension gives 63 to 67 HP and Clamp 9 x2" << std::endl;
}

void test_slumbering_beetle() {
    // Asleep it only snores, turn after turn, and costs the player nothing.
    check_monster(EnemyType::SLUMBERING_BEETLE, "Scarabée dormeur", 86, {
        { STUN, 0, 0, 0 },
        { STUN, 0, 0, 0 },
        { STUN, 0, 0, 0 },
    });
    // The first hit wakes it: Roll Out 16, +2 Strength after each, every turn.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SLUMBERING_BEETLE);
    Enemy& beetle = combat.enemies[0];
    combat.damage_enemy(beetle, 5);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // still snoring: the intent was planned asleep
    assert(combat.player.hp == combat.player.max_hp);
    for (int turn = 0; turn < 4; ++turn) {
        const int strength = 2 * turn;
        assert(beetle.current_intent.type == ATTACK_BUFF);
        assert(beetle.intent_damage(combat.player) == 16 + strength);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == 16 + strength);
        assert(beetle.get_status(StatusType::STRENGTH) == strength + 2 && beetle.block == 0);
    }
    std::cout << "[PASS] Slumbering Beetle: 86 HP, Snore while asleep, Roll Out 16 +2 Strength once hurt" << std::endl;
}

void test_slumbering_beetle_ascension() {
    // 89 HP and a Roll Out of 18 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SLUMBERING_BEETLE);
    Enemy& beetle = combat.enemies[0];
    beetle.ascend(true, true);
    combat.plan_enemies();
    assert(beetle.hp == 89 && beetle.max_hp == 89);
    combat.damage_enemy(beetle, 5);
    combat.end_player_turn();
    combat.player.hp = combat.player.max_hp;
    assert(beetle.current_intent.type == ATTACK_BUFF && beetle.intent_damage(combat.player) == 18);
    std::cout << "[PASS] Slumbering Beetle: Ascension gives 89 HP and Roll Out 18" << std::endl;
}

void test_tunneler() {
    // Bite 13, Burrow (32 Block and Burrowed), Below 23, and Below again.
    check_monster(EnemyType::TUNNELER, "Tunnelier", 87, {
        { ATTACK, 13, 0, 0 },
        { DEFEND, 0, 32, 0 },
        { ATTACK, 23, 0, 0 },
        { ATTACK, 23, 0, 0 },
        { ATTACK, 23, 0, 0 },
    });

    // The Block of the Burrow broken before it acts again: Dizzy (a lost turn), then Bite, Burrow and Below.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TUNNELER);
    Enemy& digger = combat.enemies[0];
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // Bite
    assert(digger.current_intent.type == DEFEND && digger.current_intent.block == 32);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // Burrow
    assert(digger.block == 32 && combat.enemy_power_amount(0, PowerId::BURROWED) == 1);
    assert(digger.current_intent.type == ATTACK && digger.phase == 0);
    combat.damage_enemy(digger, 20);
    assert(digger.block == 12 && combat.enemy_power_amount(0, PowerId::BURROWED) == 1);
    combat.damage_enemy(digger, 12);
    assert(digger.block == 0 && combat.enemy_power_amount(0, PowerId::BURROWED) == 0 && digger.hp == 87);
    assert(digger.phase == 1 && digger.current_intent.type == STUN && digger.intent_damage(combat.player) == 0);
    // Dizzy, Bite, Burrow, Below, Below: no damage the lost turn, and the cycle goes on.
    const int expected[][2] = { { 0, 0 }, { 13, 0 }, { 0, 32 }, { 23, 0 }, { 23, 0 } };
    for (const auto& turn : expected) {
        assert(digger.current_intent.type == (turn[0] == 0 ? (turn[1] ? DEFEND : STUN) : ATTACK));
        assert(digger.intent_damage(combat.player) == turn[0]);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.player.max_hp - combat.player.hp == turn[0]);
        assert(digger.block == turn[1]);
    }
    std::cout << "[PASS] Tunneler: 87 HP, Bite 13, Burrow 32 Block, Below 23, Dizzy once the Burrow is broken" << std::endl;
}

void test_tunneler_ascension() {
    // 92 HP, Bite 15 and Below 26 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::TUNNELER);
    Enemy& digger = combat.enemies[0];
    assert(digger.ascension_hp() == 92);
    digger.ascend(true, true);
    combat.plan_enemies();
    assert(digger.hp == 92 && digger.max_hp == 92);
    assert(digger.current_intent.type == ATTACK && digger.intent_damage(combat.player) == 15);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(digger.current_intent.type == DEFEND && digger.current_intent.block == 32);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
    assert(digger.current_intent.type == ATTACK && digger.intent_damage(combat.player) == 26);
    std::cout << "[PASS] Tunneler: Ascension gives 92 HP, Bite 15 and Below 26" << std::endl;
}

void test_entomancer() {
    // Bees 3x7, Spear 18, Pheromone Spit (Personal Hive 1, Strength 3), twice over: the Strength it gains adds to
    // each hit that follows.
    check_monster(EnemyType::ENTOMANCER, "Entomancien", 145, {
        { ATTACK, 3, 0, 0, 0, 7 },
        { ATTACK, 18, 0, 0 },
        { BUFF, 0, 0, 3 },
        { ATTACK, 6, 0, 3, 0, 7 },
        { ATTACK, 21, 0, 3 },
        { BUFF, 0, 0, 6 },
        { ATTACK, 9, 0, 6, 0, 7 },
    });
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::ENTOMANCER);
    assert(combat.enemy_power_amount(0, PowerId::PERSONAL_HIVE) == 0);
    pass_turn(combat);
    pass_turn(combat);
    assert(combat.enemy_power_amount(0, PowerId::PERSONAL_HIVE) == 0);
    pass_turn(combat); // Pheromone Spit
    assert(combat.enemy_power_amount(0, PowerId::PERSONAL_HIVE) == 1);
    pass_turn(combat);
    pass_turn(combat);
    pass_turn(combat); // the second Pheromone Spit
    assert(combat.enemy_power_amount(0, PowerId::PERSONAL_HIVE) == 2);
    std::cout << "[PASS] Entomancer: 145 HP, Bees 3x7, Spear 18, Pheromone Spit (Personal Hive 1, Strength 3)" << std::endl;
}

void test_entomancer_ascension() {
    // 165 HP, Bees 3x8 and Spear 20 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::ENTOMANCER);
    Enemy& entomancer = combat.enemies[0];
    assert(entomancer.ascension_hp() == 165);
    entomancer.ascend(true, true);
    combat.plan_enemies();
    assert(entomancer.hp == 165 && entomancer.max_hp == 165);
    assert(entomancer.current_intent.type == ATTACK && entomancer.current_intent.hits == 8);
    assert(entomancer.intent_damage(combat.player) == 3);
    pass_turn(combat);
    assert(entomancer.current_intent.hits == 1 && entomancer.intent_damage(combat.player) == 20);
    std::cout << "[PASS] Entomancer: Ascension gives 165 HP, Bees 3x8 and a Spear of 20" << std::endl;
}

void test_flail_knight() {
    // Ram 15 first, then a draw among War Chant (+3 Strength), Flail 9x2 and Ram 15; War Chant never twice in a row.
    bool chant = false, flail = false, ram_again = false;
    for (int i = 0; i < 30; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FLAIL_KNIGHT);
        Enemy& knight = combat.enemies[0];
        assert(knight.type == EnemyType::FLAIL_KNIGHT && knight.name == "Chevalier au fléau");
        assert(knight.hp == 101 && knight.max_hp == 101);
        bool previous_chant = false;
        int strength = 0;
        for (int turn = 1; turn <= 12; ++turn) {
            const EnemyIntent intent = knight.current_intent;
            if (turn == 1) assert(intent.type == ATTACK && intent.hits == 1 && knight.intent_damage(combat.player) == 15);
            int expected = 0;
            if (intent.type == BUFF) {
                assert(turn > 1 && !previous_chant);
                assert(knight.intent_damage(combat.player) == 0);
                chant = true;
            } else if (intent.hits == 2) {
                assert(knight.intent_damage(combat.player) == 9 + strength);
                expected = 2 * (9 + strength);
                flail = true;
            } else {
                assert(intent.type == ATTACK && knight.intent_damage(combat.player) == 15 + strength);
                expected = 15 + strength;
                if (turn > 1) ram_again = true;
            }
            previous_chant = intent.type == BUFF;
            combat.player.hp = combat.player.max_hp;
            combat.end_player_turn();
            assert(combat.state == CombatState::PLAYER_TURN);
            assert(combat.player.max_hp - combat.player.hp == expected);
            if (previous_chant) strength += 3;
            assert(knight.get_status(StatusType::STRENGTH) == strength);
            assert(knight.block == 0 && knight.hp == 101);
        }
    }
    assert(chant && flail && ram_again);
    std::cout << "[PASS] Flail Knight: 101 HP, Ram 15, then War Chant +3 Strength, Flail 9x2 or Ram at random" << std::endl;
}

void test_flail_knight_ascension() {
    // 108 HP, Flail 10x2 and Ram 17 at Ascension.
    bool flail = false, ram = false;
    for (int i = 0; i < 30; ++i) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FLAIL_KNIGHT);
        Enemy& knight = combat.enemies[0];
        knight.ascend(true, true);
        combat.plan_enemies();
        assert(knight.hp == 108 && knight.max_hp == 108);
        for (int turn = 1; turn <= 8; ++turn) {
            const EnemyIntent intent = knight.current_intent;
            if (intent.type == ATTACK && intent.hits == 2) {
                assert(knight.intent_damage(combat.player) == 10 + knight.get_status(StatusType::STRENGTH));
                flail = true;
            } else if (intent.type == ATTACK) {
                assert(knight.intent_damage(combat.player) == 17 + knight.get_status(StatusType::STRENGTH));
                ram = true;
            }
            combat.player.hp = combat.player.max_hp;
            combat.end_player_turn();
        }
    }
    assert(flail && ram);
    std::cout << "[PASS] Flail Knight: Ascension gives 108 HP, Flail 10x2, Ram 17" << std::endl;
}

void test_soul_nexus() {
    // Calm: Soul Burn (29) every turn. Hurt, it opens with Life Drain (18, Weak 2, Vulnerable 2), then draws
    // among Soul Burn, Maelstrom (6 four times) and Life Drain, never the move it just played.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SOUL_NEXUS);
    Enemy& nexus = combat.enemies[0];
    assert(nexus.type == EnemyType::SOUL_NEXUS && nexus.name == "Amalg'âmes");
    assert(nexus.hp == 234 && nexus.max_hp == 234);
    for (int turn = 1; turn <= 4; ++turn) {
        assert(nexus.phase == 0 && nexus.block == 0 && nexus.statuses.empty());
        assert(nexus.current_intent.type == ATTACK && nexus.intent_damage(combat.player) == 29);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        assert(combat.player.max_hp - combat.player.hp == 29);
    }

    CombatEngine hurt = make_combat(CharacterId::IRONCLAD, EnemyType::SOUL_NEXUS);
    Enemy& beast = hurt.enemies[0];
    hurt.damage_enemy(beast, 5);
    hurt.player.hp = hurt.player.max_hp;
    hurt.end_player_turn();
    assert(beast.phase == 1);
    assert(beast.current_intent.type == ATTACK_DEBUFF && beast.intent_damage(hurt.player) == 18);
    assert(beast.current_intent.debuffs[0].type == StatusType::WEAK && beast.current_intent.debuffs[0].amount == 2);
    assert(beast.current_intent.debuffs[1].type == StatusType::VULNERABLE && beast.current_intent.debuffs[1].amount == 2);
    bool seen[3] = { false, false, false }; // Soul Burn, Maelstrom, Life Drain
    int previous = -1;
    for (int turn = 0; turn < 60; ++turn) {
        const EnemyIntent& intent = beast.current_intent;
        assert(beast.phase == 1);
        hurt.player.hp = hurt.player.max_hp;
        hurt.player.statuses.clear();
        int kind, expected;
        if (intent.type == ATTACK_DEBUFF) {
            assert(intent.base_damage == 18 && intent.hits == 1);
            assert(intent.debuffs[0].type == StatusType::WEAK && intent.debuffs[0].amount == 2);
            assert(intent.debuffs[1].type == StatusType::VULNERABLE && intent.debuffs[1].amount == 2);
            kind = 2;
            expected = 18;
        } else if (intent.hits == 4) {
            assert(intent.type == ATTACK && intent.base_damage == 6);
            kind = 1;
            expected = 24;
        } else {
            assert(intent.type == ATTACK && intent.base_damage == 29);
            kind = 0;
            expected = 29;
        }
        assert(kind != previous || turn == 0); // never twice in a row after the opening
        if (turn == 0) assert(kind == 2);
        seen[kind] = true;
        previous = kind;
        hurt.end_player_turn();
        assert(hurt.state == CombatState::PLAYER_TURN);
        assert(hurt.player.max_hp - hurt.player.hp >= expected);
        if (kind == 2) {
            assert(hurt.player.get_status(StatusType::WEAK) == 2 && hurt.player.get_status(StatusType::VULNERABLE) == 2);
        }
    }
    assert(seen[0] && seen[1] && seen[2]);
    std::cout << "[PASS] Soul Nexus: 234 HP, Soul Burn until hurt, then Life Drain and a draw without repeating" << std::endl;
}

void test_soul_nexus_ascension() {
    // 254 HP, Soul Burn of 31, Maelstrom of 7 four times and Life Drain of 19 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SOUL_NEXUS);
    Enemy& nexus = combat.enemies[0];
    assert(nexus.ascension_hp() == 254);
    nexus.ascend(true, true);
    combat.plan_enemies();
    assert(nexus.hp == 254 && nexus.max_hp == 254);
    assert(nexus.current_intent.type == ATTACK && nexus.intent_damage(combat.player) == 31);
    combat.damage_enemy(nexus, 1);
    combat.end_player_turn();
    assert(nexus.current_intent.type == ATTACK_DEBUFF && nexus.intent_damage(combat.player) == 19);
    bool burn = false, maelstrom = false;
    for (int turn = 0; turn < 60 && !(burn && maelstrom); ++turn) {
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
        if (nexus.current_intent.hits == 4) {
            assert(nexus.current_intent.base_damage == 7);
            maelstrom = true;
        } else if (nexus.current_intent.type == ATTACK) {
            assert(nexus.current_intent.base_damage == 31);
            burn = true;
        }
    }
    assert(burn && maelstrom);
    std::cout << "[PASS] Soul Nexus: Ascension gives 254 HP, Soul Burn 31, Maelstrom 7x4, Life Drain 19" << std::endl;
}

} // namespace

void test_slimed_berserker() {
    // Vomit Ichor (10 Slimed), Furious Pummeling 4x4, Leeching Hug (+3 Strength, 3 Weak), Smother 30, and again.
    // Its Strength adds to every hit that follows.
    check_monster(EnemyType::SLIMED_BERSERKER, "Berserker de vase", 261, {
        { STATUS, 0, 0, 0 },
        { ATTACK, 4, 0, 0, 0, 4 },
        { BUFF, 0, 0, 3 },
        { ATTACK, 33, 0, 3 },
        { STATUS, 0, 0, 3 },
        { ATTACK, 7, 0, 3, 0, 4 },
        { BUFF, 0, 0, 6 },
        { ATTACK, 36, 0, 6 },
    });

    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SLIMED_BERSERKER);
    Enemy& berserker = combat.enemies[0];
    assert(berserker.current_intent.status_card == CardId::SLIMED && berserker.current_intent.status_cards == 10);
    auto slimed = [&] {
        int count = 0;
        for (const DeckCard& card : combat.discard_pile) count += card.id == CardId::SLIMED;
        return count;
    };
    combat.end_player_turn();
    assert(slimed() >= 10);
    combat.end_player_turn(); // Furious Pummeling
    combat.player.hp = combat.player.max_hp;
    assert(berserker.current_intent.type == BUFF);
    assert(berserker.current_intent.debuffs[0].type == StatusType::WEAK && berserker.current_intent.debuffs[0].amount == 3);
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::WEAK) >= 3 && berserker.get_status(StatusType::STRENGTH) == 3);
    std::cout << "[PASS] Slimed Berserker: 261 HP, Vomit Ichor, Pummeling, Leeching Hug, Smother, loop" << std::endl;
}

void test_slimed_berserker_ascension() {
    // 281 HP, Furious Pummeling 5x4 and Smother 33 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::SLIMED_BERSERKER);
    Enemy& berserker = combat.enemies[0];
    berserker.ascend(true, true);
    combat.plan_enemies();
    assert(berserker.hp == 281 && berserker.max_hp == 281);
    combat.end_player_turn(); // Vomit Ichor
    combat.player.hp = combat.player.max_hp;
    assert(berserker.current_intent.type == ATTACK && berserker.current_intent.hits == 4);
    assert(berserker.intent_damage(combat.player) == 5);
    combat.end_player_turn();
    assert(combat.player.max_hp - combat.player.hp == 20);
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // Leeching Hug
    combat.player.hp = combat.player.max_hp;
    assert(berserker.current_intent.type == ATTACK && berserker.intent_damage(combat.player) == 36);
    std::cout << "[PASS] Slimed Berserker: Ascension gives 281 HP, Pummeling 5x4, Smother 33" << std::endl;
}

void test_frog_knight() {
    // Tongue Lash 13 (2 Frail), Strike Down Evil 21, For the Queen (+5 Strength), and again.
    check_monster(EnemyType::FROG_KNIGHT, "Chevalier rainette", 191, {
        { ATTACK_DEBUFF, 13, 0, 0 },
        { ATTACK, 21, 0, 0 },
        { BUFF, 0, 0, 5 },
        { ATTACK_DEBUFF, 18, 0, 5 },
        { ATTACK, 26, 0, 5 },
        { BUFF, 0, 0, 10 },
    });
    std::cout << "[PASS] Frog Knight: 191 HP, Tongue Lash 13, Strike Down Evil 21, For the Queen +5 Strength, loop" << std::endl;
}

void test_frog_knight_beetle_charge() {
    // Hurt below half its HP, it opens a phase with Beetle Charge 35, once, then goes back to the cycle.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FROG_KNIGHT);
    Enemy& knight = combat.enemies[0];
    assert(knight.current_intent.type == ATTACK_DEBUFF);
    combat.end_player_turn(); // Tongue Lash
    assert(combat.player.get_status(StatusType::FRAIL) == 2);
    knight.hp = 95; // 191 / 2 = 95: not below half yet
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // Strike Down Evil
    knight.hp = 94;
    combat.plan_enemies();
    assert(knight.current_intent.type == ATTACK && knight.intent_damage(combat.player) == 35);
    const EnemyIntentType after[] = { ATTACK_DEBUFF, ATTACK, BUFF, ATTACK_DEBUFF };
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn(); // Beetle Charge
    for (EnemyIntentType intent : after) {
        assert(knight.current_intent.type == intent);
        combat.player.hp = combat.player.max_hp;
        combat.end_player_turn();
    }
    std::cout << "[PASS] Frog Knight: Beetle Charge 35 once below half HP, then Tongue Lash, Strike, For the Queen" << std::endl;
}

void test_frog_knight_ascension() {
    // 199 HP, Tongue Lash 14, Strike Down Evil 23 and Beetle Charge 40 at Ascension.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::FROG_KNIGHT);
    Enemy& knight = combat.enemies[0];
    knight.ascend(true, true);
    combat.plan_enemies();
    assert(knight.hp == 199 && knight.max_hp == 199);
    assert(knight.current_intent.type == ATTACK_DEBUFF && knight.intent_damage(combat.player) == 14);
    combat.end_player_turn();
    combat.player.hp = combat.player.max_hp;
    assert(knight.current_intent.type == ATTACK && knight.intent_damage(combat.player) == 23);
    knight.hp = 50;
    combat.plan_enemies();
    assert(knight.current_intent.type == ATTACK && knight.intent_damage(combat.player) == 40);
    std::cout << "[PASS] Frog Knight: Ascension gives 199 HP, Tongue Lash 14, Strike 23, Beetle Charge 40" << std::endl;
}

int main() {
    std::cout << "=== Monsters ===" << std::endl;
    test_fuzzy_wurm_crawler();
    test_osty();
    test_osty_ascension();
    test_crossbow_ruby_raider();
    test_brute_ruby_raider();
    test_assassin_ruby_raider();
    test_axe_ruby_raider();
    test_tracker_ruby_raider();
    test_tracker_ruby_raider_ascension();
    test_mecha_knight();
    test_mecha_knight_ascension();
    test_calcified_cultist();
    test_damp_cultist();
    test_sewer_clam();
    test_monster_block_absorbs_damage();
    test_debuff_intents();
    test_debuff_duration();
    test_debuff_and_artifact();
    test_the_forgotten();
    test_the_forgotten_ascension();
    test_the_lost();
    test_the_lost_ascension();
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
    test_zapbot();
    test_zapbot_ascension();
    test_spectral_knight();
    test_spectral_knight_ascension();
    test_noisebot();
    test_noisebot_ascension();
    test_parafright();
    test_parafright_ascension();
    test_cubex_construct();
    test_cubex_construct_ascension();
    test_punch_construct();
    test_punch_construct_ascension();
    test_slimed_berserker();
    test_slimed_berserker_ascension();
    test_globe_head();
    test_globe_head_ascension();
    test_guardbot();
    test_guardbot_ascension();
    test_bowlbug_nectar();
    test_bowlbug_nectar_ascension();
    test_devoted_sculptor();
    test_devoted_sculptor_ascension();
    test_axebot();
    test_axebot_ascension();
    test_turret_operator();
    test_turret_operator_ascension();
    test_bowlbug_silk();
    test_bowlbug_silk_ascension();
    test_bowlbug_rock();
    test_bowlbug_rock_ascension();
    test_bowlbug_egg();
    test_bowlbug_egg_ascension();
    test_skulking_colony();
    test_skulking_colony_ascension();
    test_terror_eel();
    test_terror_eel_ascension();
    test_infested_prism();
    test_infested_prism_ascension();
    test_infested_prism_vital_spark();
    test_magi_knight();
    test_magi_knight_ascension();
    test_bygone_effigy();
    test_bygone_effigy_ascension();
    test_byrdonis();
    test_byrdonis_ascension();
    test_fabricator();
    test_fabricator_ascension();
    test_tough_egg();
    test_tough_egg_ascension();
    test_the_obscura();
    test_the_obscura_ascension();
    test_ovicopter();
    test_ovicopter_ascension();
    test_shrinker_beetle();
    test_shrinker_beetle_ascension();
    test_owl_magistrate();
    test_owl_magistrate_ascension();
    test_scroll_of_biting();
    test_scroll_of_biting_ascension();
    test_fogmog();
    test_snapping_jaxfruit();
    test_twig_slime_s();
    test_twig_slime_m();
    test_leaf_slime_s();
    test_leaf_slime_m();
    test_slithering_strangler();
    test_living_fog();
    test_two_tailed_rat();
    test_phrog_parasite();
    test_wriggler();
    test_wriggler_ascension();
    test_the_insatiable();
    test_the_insatiable_ascension();
    test_sandpit();
    test_the_insatiable_whole_fight();
    test_lagavulin_matriarch();
    test_lagavulin_matriarch_woken();
    test_lagavulin_matriarch_ascension();
    test_lagavulin_matriarch_whole_fight();
    test_knowledge_demon();
    test_knowledge_demon_curses();
    test_knowledge_demon_ascension();
    test_knowledge_demon_whole_fight();
    test_test_subject();
    test_test_subject_respawn();
    test_test_subject_thorns_respawn();
    test_test_subject_ascension();
    test_test_subject_whole_fight();
    test_queen();
    test_torch_head_amalgam();
    test_queen_debuffs();
    test_queen_ascension();
    test_queen_whole_fight();
    test_waterfall_giant();
    test_steam_eruption();
    test_waterfall_giant_ascension();
    test_waterfall_giant_whole_fight();
    test_vantom();
    test_vantom_ascension();
    test_vantom_whole_fight();
    test_soul_fysh();
    test_beckon();
    test_soul_fysh_ascension();
    test_soul_fysh_whole_fight();
    test_aeonglass();
    test_aeonglass_ascension();
    test_aeonglass_whole_fight();
    test_ceremonial_beast();
    test_plow();
    test_ceremonial_beast_ascension();
    test_ceremonial_beast_whole_fight();
    test_phrog_parasite_ascension();
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
    test_louse_progenitor();
    test_louse_progenitor_ascension();
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
    test_soul_nexus();
    test_soul_nexus_ascension();
    test_seapunk();
    test_seapunk_ascension();
    test_chomper();
    test_chomper_ascension();
    test_slumbering_beetle();
    test_slumbering_beetle_ascension();
    test_tunneler();
    test_tunneler_ascension();
    test_frog_knight();
    test_frog_knight_beetle_charge();
    test_frog_knight_ascension();
    test_entomancer();
    test_entomancer_ascension();
    test_flail_knight();
    test_flail_knight_ascension();
    std::cout << "=== All Monsters Passed! ===" << std::endl;
    return 0;
}
