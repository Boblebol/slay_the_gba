// Osty, the Necrobinder's companion (#104): how Summon gives him HP, how he takes the enemies' attacks for the
// player, how he attacks, heals and dies, and what reacts to it. One case per rule; the numbers are those of
// bible/data (cards, relics, potions).
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

// A Necrobinder against `count` enemies with 100 HP each. No relic: Osty is dead when the fight starts.
CombatEngine make_fight(int count = 1) {
    FixedVector<EnemyType, 3> pack;
    for (int i = 0; i < count; ++i) pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::NECROBINDER);
    for (Enemy& foe : combat.enemies) {
        foe.max_hp = 100;
        foe.hp = 100;
    }
    return combat;
}

// The enemy's next move is a plain attack for `damage`; 0 makes it do nothing.
void set_attack(Enemy& foe, int16_t damage) {
    foe.current_intent = EnemyIntent();
    foe.current_intent.type = damage > 0 ? EnemyIntentType::ATTACK : EnemyIntentType::BUFF;
    foe.current_intent.base_damage = damage;
}

// Ends the turn with every enemy attacking for `damage`.
void end_turn(CombatEngine& combat, int16_t damage) {
    for (Enemy& foe : combat.enemies) set_attack(foe, damage);
    combat.end_player_turn();
}

// Puts `card` alone in hand with full energy and plays it.
void play(CombatEngine& combat, DeckCard card, int target = -1) {
    combat.hand.clear();
    combat.hand.push_back(card);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, target);
    assert(played);
}

void test_summon_gives_hp_and_max_hp() {
    CombatEngine combat = make_fight();
    Player& player = combat.player;
    assert(!player.minion_alive() && player.minion_hp == 0 && player.minion_max_hp == 0);

    // Dead: Osty comes with N HP.
    player.summon_minion(5);
    assert(player.minion_alive() && player.minion_hp == 5 && player.minion_max_hp == 5);
    // Alive: N more Max HP, and as many HP.
    player.summon_minion(3);
    assert(player.minion_hp == 8 && player.minion_max_hp == 8);
    // A hurt Osty keeps his wound: both numbers go up by N.
    end_turn(combat, 6);
    assert(player.minion_hp == 2 && player.minion_max_hp == 8);
    player.summon_minion(4);
    assert(player.minion_hp == 6 && player.minion_max_hp == 12);
    std::cout << "[PASS] Summon N: N HP for a dead Osty, N more HP and Max HP for a living one" << std::endl;
}

void test_osty_takes_the_hits() {
    // The player's Block absorbs the attack first, Osty takes the rest: the player's HP is not touched.
    CombatEngine soak = make_fight();
    soak.player.summon_minion(15);
    soak.player.block = 5;
    int16_t hp = soak.player.hp;
    end_turn(soak, 11);
    assert(soak.player.block == 0);
    assert(soak.player.minion_hp == 9 && soak.player.minion_max_hp == 15);
    assert(soak.player.hp == hp);
    assert(soak.last_enemy_actions.size() == 1);
    assert(soak.last_enemy_actions[0].damage_blocked == 11 && soak.last_enemy_actions[0].damage_to_player == 0);

    // An attack that is not blocked at all by Osty when he is dead: Block, then HP.
    CombatEngine alone = make_fight();
    hp = alone.player.hp;
    end_turn(alone, 11);
    assert(alone.player.hp == hp - 11);

    // Damage that is not an enemy's attack does not go through Osty.
    CombatEngine burn = make_fight();
    burn.player.summon_minion(10);
    hp = burn.player.hp;
    burn.damage_player(4);
    burn.lose_hp(3);
    assert(burn.player.minion_hp == 10 && burn.player.hp == hp - 7);
    std::cout << "[PASS] Osty takes what the player's Block lets through of the enemies' attacks" << std::endl;
}

void test_osty_dies() {
    // The hit that kills Osty goes on: what he did not absorb lands on the Block, then on the HP.
    CombatEngine through = make_fight();
    Player& player = through.player;
    player.summon_minion(5);
    int16_t hp = player.hp;
    through.player.block = 0;
    set_attack(through.enemies[0], 11);
    through.enemies[0].execute_turn(player, 0, false);
    assert(!player.minion_alive() && player.minion_hp == 0 && player.minion_max_hp == 0);
    assert(player.hp == hp - 6);

    CombatEngine blocked = make_fight();
    blocked.player.summon_minion(5);
    blocked.player.block = 4;
    hp = blocked.player.hp;
    set_attack(blocked.enemies[0], 11);
    blocked.enemies[0].execute_turn(blocked.player, 0, false);
    assert(blocked.player.block == 0 && blocked.player.hp == hp - 2);

    // A hit for exactly his HP kills him too.
    CombatEngine exact = make_fight();
    exact.player.summon_minion(5);
    hp = exact.player.hp;
    end_turn(exact, 5);
    assert(!exact.player.minion_alive() && exact.player.minion_max_hp == 0 && exact.player.hp == hp);

    // Summoned again, he is a new Osty: the Max HP of the one that died is not kept.
    exact.player.summon_minion(3);
    assert(exact.player.minion_hp == 3 && exact.player.minion_max_hp == 3);

    // "Osty dies": whatever his HP.
    CombatEngine killed = make_fight();
    killed.player.summon_minion(20);
    killed.kill_osty();
    assert(!killed.player.minion_alive() && killed.player.minion_max_hp == 0);
    killed.kill_osty(); // already dead: nothing
    assert(killed.player.minion_hp == 0);
    std::cout << "[PASS] Osty's death: the rest of the hit goes on, his Max HP is lost, Summon brings a new one" << std::endl;
}

void test_osty_heals() {
    CombatEngine combat = make_fight();
    Player& player = combat.player;
    // Dead: nothing to heal, and healing does not bring him back.
    combat.heal_osty(5);
    assert(!player.minion_alive());

    player.summon_minion(10);
    end_turn(combat, 7);
    assert(player.minion_hp == 3);
    combat.heal_osty(5);
    assert(player.minion_hp == 8 && player.minion_max_hp == 10);
    // Never above his Max HP.
    combat.heal_osty(5);
    assert(player.minion_hp == 10 && player.minion_max_hp == 10);
    std::cout << "[PASS] Osty heals up to his Max HP, and only when he is alive" << std::endl;
}

void test_osty_attacks() {
    CombatEngine combat = make_fight(2);
    // Dead: he cannot attack.
    assert(combat.osty_attack(combat.enemies[0], 6) == 0);
    combat.osty_attack_all(6);
    assert(combat.enemies[0].hp == 100 && combat.enemies[1].hp == 100 && combat.counters.osty_attacks == 0);

    combat.player.summon_minion(1);
    assert(combat.osty_attack(combat.enemies[0], 6) == 6);
    assert(combat.enemies[0].hp == 94 && combat.counters.osty_attacks == 1);
    // An attack on every enemy is one attack.
    combat.osty_attack_all(11);
    assert(combat.enemies[0].hp == 83 && combat.enemies[1].hp == 89 && combat.counters.osty_attacks == 2);

    // His hits are attack hits: the player's Strength and Weak and the target's Vulnerable and Block apply.
    combat.player.add_status(StatusType::STRENGTH, 2);
    combat.enemies[1].add_status(StatusType::VULNERABLE, 1);
    combat.enemies[1].block = 3;
    assert(combat.osty_attack(combat.enemies[1], 6) == 9); // (6 + 2) x 1.5 = 12, 3 blocked
    assert(combat.enemies[1].hp == 80);

    // The tally is the turn's.
    end_turn(combat, 0);
    assert(combat.counters.osty_attacks == 0);
    std::cout << "[PASS] Osty's attacks: impossible when dead, counted once each, Strength and Vulnerable apply" << std::endl;
}

void test_unleash() {
    assert(CardDef::get(CardId::UNLEASH).type == CardType::ATTACK && CardDef::get(CardId::UNLEASH).target == TargetType::SINGLE_ENEMY);
    for (bool upgraded : { false, true }) {
        assert(CardDef::get_cost(CardId::UNLEASH, upgraded) == 1);
        // Osty deals 6 (9) damage, plus his current HP.
        CombatEngine combat = make_fight();
        combat.player.summon_minion(10);
        end_turn(combat, 3);
        assert(combat.player.minion_hp == 7 && combat.player.minion_max_hp == 10);
        play(combat, DeckCard(CardId::UNLEASH, upgraded));
        assert(combat.enemies[0].hp == 100 - ((upgraded ? 9 : 6) + 7));
        assert(combat.player.energy == 2 && combat.counters.osty_attacks == 1);
        assert(combat.player.minion_hp == 7); // attacking costs him nothing
    }

    // Vulnerable applies to the whole hit.
    CombatEngine weak_spot = make_fight();
    weak_spot.player.summon_minion(4);
    weak_spot.enemies[0].add_status(StatusType::VULNERABLE, 1);
    play(weak_spot, CardId::UNLEASH);
    assert(weak_spot.enemies[0].hp == 100 - 15);

    // Osty dead: the card cannot be played.
    CombatEngine dead = make_fight();
    dead.hand.clear();
    dead.hand.push_back(CardId::UNLEASH);
    assert(dead.play_blocker(0) == PlayBlocker::CONDITION && !dead.play_card(0));
    assert(dead.enemies[0].hp == 100 && dead.player.energy == 3 && dead.hand.size() == 1);
    dead.player.summon_minion(1);
    assert(dead.can_play_card(0) && dead.play_card(0));
    assert(dead.enemies[0].hp == 100 - 7);
    std::cout << "[PASS] Unleash: Osty deals 6 (9) plus his current HP, and cannot when he is dead" << std::endl;
}

void test_bodyguard() {
    assert(CardDef::get(CardId::BODYGUARD).type == CardType::SKILL);
    for (bool upgraded : { false, true }) {
        assert(CardDef::get_cost(CardId::BODYGUARD, upgraded) == 1);
        CombatEngine combat = make_fight();
        combat.player.block = 0;
        play(combat, DeckCard(CardId::BODYGUARD, upgraded));
        int16_t summon = upgraded ? 7 : 5;
        assert(combat.player.minion_hp == summon && combat.player.minion_max_hp == summon);
        assert(combat.player.block == 0 && combat.player.energy == 2);
        // Played again: Osty grows.
        play(combat, DeckCard(CardId::BODYGUARD, upgraded));
        assert(combat.player.minion_hp == 2 * summon && combat.player.minion_max_hp == 2 * summon);
    }
    std::cout << "[PASS] Bodyguard: Summon 5 (7), no Block" << std::endl;
}

void test_other_summons() {
    // Every other way to summon follows the same rule: HP and Max HP.
    CombatEngine cards = make_fight();
    play(cards, CardId::PULL_AGGRO);
    assert(cards.player.minion_hp == 4 && cards.player.minion_max_hp == 4 && cards.player.block == 7);
    play(cards, CardId::AFTERLIFE);
    assert(cards.player.minion_hp == 10 && cards.player.minion_max_hp == 10);
    cards.draw_pile.push_back(CardId::DEFEND);
    play(cards, CardId::CLEANSE);
    if (cards.choice_pending()) cards.resolve_choice_default();
    assert(cards.player.minion_hp == 13 && cards.player.minion_max_hp == 13);
    cards.player.max_energy = 3;
    play(cards, CardId::REANIMATE);
    assert(cards.player.minion_hp == 33 && cards.player.minion_max_hp == 33);
    play(cards, CardId::INVOKE);
    assert(cards.player.minion_hp == 33);
    end_turn(cards, 0);
    assert(cards.player.minion_hp == 35 && cards.player.minion_max_hp == 35);

    CombatEngine brew = make_fight();
    assert(brew.use_potion("BONE_BREW"));
    assert(brew.player.minion_hp == 15 && brew.player.minion_max_hp == 15);
    std::cout << "[PASS] Pull Aggro, Afterlife, Cleanse, Reanimate, Invoke and Bone Brew raise Osty's HP and Max HP" << std::endl;
}

void test_bone_flute() {
    CombatEngine combat = make_fight(2);
    combat.add_power(PowerId::BONE_FLUTE, 2);
    combat.player.block = 0;
    // No attack, no Block.
    combat.osty_attack(combat.enemies[0], 6);
    assert(combat.player.block == 0);
    combat.player.summon_minion(3);
    combat.osty_attack(combat.enemies[0], 6);
    assert(combat.player.block == 2);
    combat.osty_attack_all(6); // one attack, whatever the number of enemies
    assert(combat.player.block == 4);
    play(combat, CardId::UNLEASH);
    assert(combat.player.block == 6);
    // The player's own attacks are not Osty's.
    play(combat, CardId::STRIKE);
    assert(combat.player.block == 6);

    // A run gives the relic its effect, and it can drop.
    RunManager run;
    run.start_run(CharacterId::NECROBINDER, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("BONE_FLUTE");
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    assert(run.combat.power_amount(PowerId::BONE_FLUTE) == 2);
    bool drops = false;
    for (std::string_view id : RunManager::active_relics()) drops = drops || id == "BONE_FLUTE";
    assert(drops);
    std::cout << "[PASS] Bone Flute: 2 Block whenever Osty attacks" << std::endl;
}

void test_phylactery_relics() {
    // Bound Phylactery, the starting relic: Summon 1 at the start of each turn, so Osty is alive on turn 1.
    RunManager bound;
    bound.start_run(CharacterId::NECROBINDER, 0, PaletteVariant::CLASSIC, 777);
    assert(bound.has_relic("BOUND_PHYLACTERY"));
    bound.map.nodes[0].type = RoomType::MONSTER;
    bound.enter_room(0);
    assert(bound.combat.player.minion_hp == 1 && bound.combat.player.minion_max_hp == 1);
    end_turn(bound.combat, 0);
    assert(bound.combat.player.minion_hp == 2 && bound.combat.player.minion_max_hp == 2);
    // Hurt, he still gains 1 HP and 1 Max HP.
    end_turn(bound.combat, 1);
    assert(bound.combat.player.minion_hp == 2 && bound.combat.player.minion_max_hp == 3);
    // Killed during the enemies' turn, a new Osty is there at the start of the next one.
    end_turn(bound.combat, 30);
    assert(bound.combat.player.minion_hp == 1 && bound.combat.player.minion_max_hp == 1);

    // Phylactery Unbound: Summon 5 at the start of the combat, then 2 at the start of each turn.
    RunManager unbound;
    unbound.start_run(CharacterId::NECROBINDER, 0, PaletteVariant::CLASSIC, 777);
    unbound.relics.clear();
    unbound.add_relic("PHYLACTERY_UNBOUND");
    unbound.map.nodes[0].type = RoomType::MONSTER;
    unbound.enter_room(0);
    assert(unbound.combat.player.minion_hp == 7 && unbound.combat.player.minion_max_hp == 7);
    end_turn(unbound.combat, 0);
    assert(unbound.combat.player.minion_hp == 9 && unbound.combat.player.minion_max_hp == 9);

    // The next combat starts from nothing: Osty's HP are the combat's.
    unbound.combat.player.summon_minion(40);
    unbound.map.nodes[0].type = RoomType::MONSTER;
    unbound.enter_room(0);
    assert(unbound.combat.player.minion_hp == 7 && unbound.combat.player.minion_max_hp == 7);
    std::cout << "[PASS] Bound Phylactery summons 1 a turn, Phylactery Unbound 5 at the start and 2 a turn" << std::endl;
}

void test_calcify() {
    CombatEngine combat = make_fight(2);
    combat.player.summon_minion(2);
    combat.add_power(PowerId::CALCIFY, 4);
    assert(combat.osty_attack(combat.enemies[0], 6) == 10);
    combat.osty_attack_all(1);
    assert(combat.enemies[0].hp == 100 - 10 - 5 && combat.enemies[1].hp == 100 - 5);
    play(combat, CardId::UNLEASH, 1);
    assert(combat.enemies[1].hp == 95 - (6 + 2 + 4));
    // Only Osty's attacks.
    play(combat, CardId::STRIKE, 1);
    assert(combat.enemies[1].hp == 83 - 6);
    std::cout << "[PASS] Calcify power: Osty's attacks deal 4 more damage" << std::endl;
}

void test_necro_mastery() {
    // Osty hit by an enemy: every enemy loses the HP he lost, whatever its Block.
    CombatEngine combat = make_fight(2);
    combat.add_power(PowerId::NECRO_MASTERY);
    combat.player.summon_minion(10);
    set_attack(combat.enemies[0], 4);
    set_attack(combat.enemies[1], 0);
    combat.enemies[1].current_intent.block = 20;
    combat.end_player_turn();
    assert(combat.player.minion_hp == 6);
    assert(combat.enemies[0].hp == 96 && combat.enemies[1].hp == 96 && combat.enemies[1].block == 20);

    // Only what he lost counts when the hit kills him.
    int16_t hp = combat.player.hp;
    set_attack(combat.enemies[0], 9);
    set_attack(combat.enemies[1], 0);
    combat.player.block = 0;
    combat.end_player_turn();
    assert(!combat.player.minion_alive() && combat.player.hp == hp - 3);
    assert(combat.enemies[0].hp == 90 && combat.enemies[1].hp == 90);

    // Made to die, he loses the HP he had left.
    combat.player.summon_minion(12);
    combat.kill_osty();
    assert(combat.enemies[0].hp == 78 && combat.enemies[1].hp == 78);
    // Dead already, or healed: no HP lost.
    combat.kill_osty();
    combat.heal_osty(5);
    assert(combat.enemies[0].hp == 78);

    // It can end the combat during the enemies' turn.
    CombatEngine lethal = make_fight(2);
    lethal.add_power(PowerId::NECRO_MASTERY);
    lethal.player.summon_minion(10);
    lethal.enemies[0].hp = 5;
    lethal.enemies[1].hp = 5;
    end_turn(lethal, 6);
    assert(lethal.state == CombatState::VICTORY);
    assert(lethal.player.minion_hp == 4); // the second enemy died before it could attack
    std::cout << "[PASS] Necro Mastery power: every enemy loses the HP Osty loses" << std::endl;
}

} // namespace

int main() {
    test_summon_gives_hp_and_max_hp();
    test_osty_takes_the_hits();
    test_osty_dies();
    test_osty_heals();
    test_osty_attacks();
    test_unleash();
    test_bodyguard();
    test_other_summons();
    test_bone_flute();
    test_phylactery_relics();
    test_calcify();
    test_necro_mastery();
    std::cout << "All Osty tests passed" << std::endl;
    return 0;
}
