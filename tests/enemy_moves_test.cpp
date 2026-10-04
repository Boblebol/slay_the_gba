// Enemy moves that reach the player (#218, #219, #220, #222): one case per kind of move, the pattern rules
// (sequence, draw with no repeat, phase at an HP threshold, place in the group), then every monster of the move
// tables over at least one full cycle of its moves.
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <set>
#include <string_view>
#include "engine/combat.h"

using namespace engine;

namespace {

constexpr EnemyIntentType ATTACK = EnemyIntentType::ATTACK;
constexpr EnemyIntentType BUFF = EnemyIntentType::BUFF;
constexpr EnemyIntentType DEFEND = EnemyIntentType::DEFEND;
constexpr EnemyIntentType ATTACK_DEFEND = EnemyIntentType::ATTACK_DEFEND;
constexpr EnemyIntentType ATTACK_BUFF = EnemyIntentType::ATTACK_BUFF;
constexpr EnemyIntentType ATTACK_DEBUFF = EnemyIntentType::ATTACK_DEBUFF;
constexpr EnemyIntentType DEBUFF = EnemyIntentType::DEBUFF;
constexpr EnemyIntentType STATUS = EnemyIntentType::STATUS;
constexpr EnemyIntentType STUN = EnemyIntentType::STUN;

CombatEngine make_combat(EnemyType enemy_type) {
    CombatEngine combat;
    combat.start_combat(enemy_type, CharacterId::IRONCLAD);
    return combat;
}

CombatEngine make_combat(std::initializer_list<EnemyType> types) {
    FixedVector<EnemyType, 3> group;
    for (EnemyType type : types) group.push_back(type);
    CombatEngine combat;
    combat.start_combat_multi(group, CharacterId::IRONCLAD);
    return combat;
}

// A fight against a Crossbow Ruby Raider whose next move is `intent`.
CombatEngine make_combat_with(const EnemyIntent& intent) {
    CombatEngine combat = make_combat(EnemyType::CROSSBOW_RUBY_RAIDER);
    combat.enemies[0].current_intent = intent;
    return combat;
}

EnemyIntent attack_intent(int16_t damage, uint8_t hits = 1) {
    EnemyIntent intent;
    intent.type = EnemyIntentType::ATTACK;
    intent.base_damage = damage;
    intent.hits = hits;
    return intent;
}

int count_cards(const CombatEngine& combat, CardId id, Pile pile) {
    int count = 0;
    if (pile == Pile::DISCARD) {
        for (const DeckCard& card : combat.discard_pile) count += card.id == id;
    } else if (pile == Pile::DRAW) {
        for (const DeckCard& card : combat.draw_pile) count += card.id == id;
    } else {
        for (const DeckCard& card : combat.hand) count += card.id == id;
    }
    return count;
}

// The enemy turn, against a player who plays nothing and starts it at full HP.
void pass_turn(CombatEngine& combat) {
    combat.player.hp = combat.player.max_hp;
    combat.end_player_turn();
}

int hp_lost(const CombatEngine& combat) {
    return combat.player.max_hp - combat.player.hp;
}

// ---------------------------------------------------------------------------------------------------------
// Kinds of moves
// ---------------------------------------------------------------------------------------------------------

void test_multi_hit_attack() {
    // 5 x 3: the intent shows one hit, the player takes the three.
    CombatEngine combat = make_combat_with(attack_intent(5, 3));
    assert(combat.enemies[0].intent_damage(combat.player) == 5);
    pass_turn(combat);
    assert(hp_lost(combat) == 15);

    // Block absorbs hit after hit: 5 x 3 against 7 Block loses 8 HP.
    CombatEngine blocked = make_combat_with(attack_intent(5, 3));
    blocked.player.hp = blocked.player.max_hp;
    blocked.player.block = 7;
    blocked.end_player_turn();
    assert(hp_lost(blocked) == 8);

    // Strength is added to each hit, Vulnerable applies to each hit.
    CombatEngine strong = make_combat_with(attack_intent(2, 4));
    strong.enemies[0].add_status(StatusType::STRENGTH, 3);
    strong.player.add_status(StatusType::VULNERABLE, 2);
    assert(strong.enemies[0].intent_damage(strong.player) == 7); // (2 + 3) x 1.5
    pass_turn(strong);
    assert(hp_lost(strong) == 28);

    // Thorns answer every hit, and the hits stop when they kill the attacker.
    CombatEngine thorns = make_combat_with(attack_intent(1, 8));
    thorns.player.add_status(StatusType::THORNS, 2);
    thorns.enemies[0].hp = 6;
    pass_turn(thorns);
    assert(!thorns.enemies[0].is_alive());
    assert(hp_lost(thorns) == 3); // dead after its third hit
    assert(thorns.state == CombatState::VICTORY);

    // Vigor is added to every hit of the attack, then spent.
    CombatEngine vigor = make_combat_with(attack_intent(3, 3));
    vigor.enemies[0].add_status(StatusType::VIGOR, 6);
    pass_turn(vigor);
    assert(hp_lost(vigor) == 27);
    assert(vigor.enemies[0].get_status(StatusType::VIGOR) == 0);

    // A single hit is still one hit, whatever `hits` holds.
    CombatEngine single = make_combat_with(attack_intent(9, 0));
    pass_turn(single);
    assert(hp_lost(single) == 9);
    std::cout << "[PASS] Multi-hit attacks: each hit meets Block, Strength, Vulnerable, Thorns and Vigor" << std::endl;
}

void test_debuff_on_player() {
    // Frail 2 given on the enemy's turn lasts the player's next two turns.
    EnemyIntent frail;
    frail.type = EnemyIntentType::DEBUFF;
    frail.player_status = StatusType::FRAIL;
    frail.player_status_amount = 2;
    CombatEngine combat = make_combat_with(frail);
    pass_turn(combat);
    assert(hp_lost(combat) == 0);
    assert(combat.player.get_status(StatusType::FRAIL) == 2);
    combat.enemies[0].current_intent = EnemyIntent{};
    combat.enemies[0].current_intent.type = EnemyIntentType::DEFEND;
    pass_turn(combat);
    assert(combat.player.get_status(StatusType::FRAIL) == 1);
    combat.enemies[0].current_intent = EnemyIntent{};
    combat.enemies[0].current_intent.type = EnemyIntentType::DEFEND;
    pass_turn(combat);
    assert(combat.player.get_status(StatusType::FRAIL) == 0);

    // Weak 1 is not lost the round it is given: the player is Weak on the next turn.
    EnemyIntent weak = attack_intent(8);
    weak.type = EnemyIntentType::ATTACK_DEBUFF;
    weak.player_status = StatusType::WEAK;
    weak.player_status_amount = 1;
    CombatEngine weakened = make_combat_with(weak);
    pass_turn(weakened);
    assert(hp_lost(weakened) == 8);
    assert(weakened.player.get_status(StatusType::WEAK) == 1);

    // The debuff comes after the hits: an attack that gives Vulnerable does not profit from it.
    EnemyIntent scream = attack_intent(10, 2);
    scream.type = EnemyIntentType::ATTACK_DEBUFF;
    scream.player_status = StatusType::VULNERABLE;
    scream.player_status_amount = 3;
    CombatEngine screamed = make_combat_with(scream);
    pass_turn(screamed);
    assert(hp_lost(screamed) == 20);
    assert(screamed.player.get_status(StatusType::VULNERABLE) == 3);
    // The next attack does: 10 becomes 15, and Vulnerable goes down by 1 at the end of that round.
    screamed.enemies[0].current_intent = attack_intent(10);
    assert(screamed.enemies[0].intent_damage(screamed.player) == 15);
    pass_turn(screamed);
    assert(hp_lost(screamed) == 15);
    assert(screamed.player.get_status(StatusType::VULNERABLE) == 2);

    // A debuff the player already had keeps wearing off when no enemy adds to it.
    CombatEngine old = make_combat_with(attack_intent(1));
    old.player.add_status(StatusType::WEAK, 2);
    pass_turn(old);
    assert(old.player.get_status(StatusType::WEAK) == 1);

    // Artifact cancels the debuff, and is used up.
    CombatEngine artifact = make_combat_with(frail);
    artifact.player.add_status(StatusType::ARTIFACT, 1);
    pass_turn(artifact);
    assert(artifact.player.get_status(StatusType::FRAIL) == 0);
    assert(artifact.player.get_status(StatusType::ARTIFACT) == 0);

    // Losing Strength and Dexterity are debuffs too.
    EnemyIntent siphon;
    siphon.type = EnemyIntentType::DEBUFF;
    siphon.player_status = StatusType::STRENGTH;
    siphon.player_status_amount = -2;
    CombatEngine drained = make_combat_with(siphon);
    pass_turn(drained);
    assert(drained.player.get_status(StatusType::STRENGTH) == -2);
    std::cout << "[PASS] Debuffs on the player: given after the hits, kept for their full length, Artifact cancels"
              << std::endl;
}

void test_status_cards() {
    // Two Slimed in the discard pile.
    EnemyIntent sticky;
    sticky.type = EnemyIntentType::STATUS;
    sticky.card = CardId::SLIMED;
    sticky.card_count = 2;
    CombatEngine combat = make_combat_with(sticky);
    const size_t cards = combat.draw_pile.size() + combat.hand.size() + combat.discard_pile.size();
    assert(count_cards(combat, CardId::SLIMED, Pile::DISCARD) == 0);
    combat.player.hp = combat.player.max_hp;
    combat.hand.clear(); // nothing but the move reaches the discard pile
    combat.end_player_turn();
    assert(hp_lost(combat) == 0);
    assert(combat.draw_pile.size() + combat.hand.size() + combat.discard_pile.size() == cards - 5 + 2);
    assert(count_cards(combat, CardId::SLIMED, Pile::DISCARD) + count_cards(combat, CardId::SLIMED, Pile::DRAW) +
           count_cards(combat, CardId::SLIMED, Pile::HAND) == 2);
    assert(combat.counters.combat_cards_created == 2);

    // Into the draw pile when the move says so.
    EnemyIntent daze;
    daze.type = EnemyIntentType::STATUS;
    daze.card = CardId::DAZED;
    daze.card_count = 3;
    daze.card_to_draw_pile = true;
    CombatEngine dazed = make_combat_with(daze);
    const size_t drawn_from = dazed.draw_pile.size();
    dazed.player.hp = dazed.player.max_hp;
    dazed.end_player_turn();
    // 3 Dazed joined the draw pile, then the next hand of 5 was drawn from it.
    assert(dazed.draw_pile.size() == drawn_from + 3 - 5);
    assert(count_cards(dazed, CardId::DAZED, Pile::DRAW) + count_cards(dazed, CardId::DAZED, Pile::HAND) == 3);
    assert(count_cards(dazed, CardId::DAZED, Pile::DISCARD) == 0);

    // A monster killed by Thorns in the middle of its move adds nothing.
    EnemyIntent bite = attack_intent(2, 2);
    bite.type = EnemyIntentType::ATTACK_DEBUFF;
    bite.card = CardId::WOUND;
    bite.card_count = 3;
    CombatEngine thorns = make_combat_with(bite);
    thorns.player.add_status(StatusType::THORNS, 50);
    pass_turn(thorns);
    assert(thorns.state == CombatState::VICTORY);
    assert(count_cards(thorns, CardId::WOUND, Pile::DISCARD) == 0);
    std::cout << "[PASS] Status cards: added to the discard pile, or to the draw pile" << std::endl;
}

void test_attack_then_buff() {
    // Repeater Blast: the hit deals what the intent announced, then the Strength comes.
    EnemyIntent blast = attack_intent(7);
    blast.type = EnemyIntentType::ATTACK_BUFF;
    blast.status_type = StatusType::STRENGTH;
    blast.status_amount = 2;
    CombatEngine combat = make_combat_with(blast);
    assert(combat.enemies[0].intent_damage(combat.player) == 7);
    pass_turn(combat);
    assert(hp_lost(combat) == 7);
    assert(combat.enemies[0].get_status(StatusType::STRENGTH) == 2);

    // Vigor gained by the move is kept for the next attack.
    EnemyIntent thrash = attack_intent(3, 3);
    thrash.type = EnemyIntentType::ATTACK_BUFF;
    thrash.status_type = StatusType::VIGOR;
    thrash.status_amount = 6;
    CombatEngine eel = make_combat_with(thrash);
    pass_turn(eel);
    assert(hp_lost(eel) == 9);
    assert(eel.enemies[0].get_status(StatusType::VIGOR) == 6);
    std::cout << "[PASS] Attack and buff: the status is gained after the hits" << std::endl;
}

void test_stun_heal_escape() {
    // Stunned: nothing happens.
    EnemyIntent stun;
    stun.type = EnemyIntentType::STUN;
    CombatEngine stunned = make_combat_with(stun);
    pass_turn(stunned);
    assert(hp_lost(stunned) == 0);
    assert(stunned.enemies[0].block == 0 && stunned.enemies[0].statuses.empty());
    assert(stunned.state == CombatState::PLAYER_TURN);

    // Heal: up to its Max HP.
    EnemyIntent heal;
    heal.type = EnemyIntentType::HEAL;
    heal.heal = 6;
    CombatEngine healed = make_combat_with(heal);
    healed.enemies[0].hp = 10;
    pass_turn(healed);
    assert(healed.enemies[0].hp == 16);
    healed.enemies[0].current_intent = heal;
    pass_turn(healed);
    assert(healed.enemies[0].hp == 18); // 18 Max HP

    // Escape: the monster is gone, and the fight goes on against the others.
    EnemyIntent flee;
    flee.type = EnemyIntentType::ESCAPE;
    CombatEngine fight = make_combat({ EnemyType::ASSASSIN_RUBY_RAIDER, EnemyType::CROSSBOW_RUBY_RAIDER });
    fight.enemies[1].current_intent = flee;
    pass_turn(fight);
    assert(fight.enemies[1].escaped && !fight.enemies[1].is_alive());
    assert(!fight.enemies[0].escaped && fight.enemies[0].is_alive());
    assert(fight.state == CombatState::PLAYER_TURN);
    assert(fight.fatal_kills == 0);

    // The last monster escaping ends the fight.
    CombatEngine alone = make_combat_with(flee);
    pass_turn(alone);
    assert(alone.enemies[0].escaped);
    assert(alone.state == CombatState::VICTORY);
    std::cout << "[PASS] Special intents: stunned, heal, escape" << std::endl;
}

void test_intent_kinds() {
    assert(intent_attacks(ATTACK) && intent_attacks(ATTACK_DEFEND) && intent_attacks(ATTACK_BUFF) &&
           intent_attacks(ATTACK_DEBUFF));
    assert(!intent_attacks(BUFF) && !intent_attacks(DEFEND) && !intent_attacks(DEBUFF) && !intent_attacks(STATUS) &&
           !intent_attacks(STUN) && !intent_attacks(EnemyIntentType::HEAL) &&
           !intent_attacks(EnemyIntentType::ESCAPE));
    // The values the engine had before keep their number.
    assert(static_cast<int>(ATTACK) == 0 && static_cast<int>(BUFF) == 1 && static_cast<int>(DEFEND) == 2 &&
           static_cast<int>(ATTACK_DEFEND) == 3);
    assert(static_cast<int>(EnemyType::SEWER_CLAM) == 12 && static_cast<int>(EnemyType::CUBEX_CONSTRUCT) == 13);
    std::cout << "[PASS] Intent kinds: which ones attack, and the old values unchanged" << std::endl;
}

// ---------------------------------------------------------------------------------------------------------
// Patterns
// ---------------------------------------------------------------------------------------------------------

// What a monster announces for a turn.
struct Move {
    std::string_view name;
    EnemyIntentType type;
    int damage = 0;
    int hits = 1;
    int block = 0;
    StatusType self_status = StatusType::NONE;
    int self_amount = 0;
    StatusType player_status = StatusType::NONE;
    int player_amount = 0;
    CardId card = CardId::NONE;
    int card_count = 0;
};

void check_intent(const Enemy& enemy, const Move& move) {
    const EnemyIntent& intent = enemy.current_intent;
    assert(intent.description == move.name);
    assert(intent.type == move.type);
    assert(intent.base_damage == move.damage);
    assert(intent.hits == move.hits);
    assert(intent.block == move.block);
    assert(intent.status_type == move.self_status && intent.status_amount == move.self_amount);
    assert(intent.player_status == move.player_status && intent.player_status_amount == move.player_amount);
    assert(intent.card_count == move.card_count);
    assert(move.card_count == 0 || intent.card == move.card);
    assert(!intent.card_to_draw_pile && intent.heal == 0);
    assert(intent_attacks(intent.type) == (move.damage > 0));
}

// Name, HP, then the moves announced turn after turn by a monster whose pattern draws nothing.
void check_sequence(EnemyType type, std::string_view name, int hp, std::initializer_list<Move> moves,
                    EnemyRank rank = EnemyRank::ALONE) {
    assert(!Enemy::opens_with_draw(type, rank));
    Enemy enemy;
    enemy.init(type, rank);
    assert(enemy.type == type && enemy.name == name);
    assert(enemy.hp == hp && enemy.max_hp == hp);
    assert(enemy.block == 0 && enemy.statuses.empty());
    int turn = 1;
    for (const Move& move : moves) {
        assert(!enemy.wants_roll());
        if (turn > 1) enemy.plan_turn(turn);
        check_intent(enemy, move);
        ++turn;
    }
}

const Move& move_named(std::initializer_list<Move> moves, const EnemyIntent& intent) {
    for (const Move& move : moves) {
        if (move.name == intent.description && move.type == intent.type) return move;
    }
    assert(false && "the monster announced a move it does not have");
    return *moves.begin();
}

// The plain xorshift of the engine, to feed the draws.
uint32_t next_roll(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

void test_sequence_and_loop() {
    // Charge Up once, then Repeater Blast twice and Expel in a loop that does not come back to Charge Up.
    const Move charge{ "Chargement", BUFF, 0, 1, 0, StatusType::STRENGTH, 2 };
    const Move blast{ "Tir répéteur", ATTACK_BUFF, 7, 1, 0, StatusType::STRENGTH, 2 };
    const Move expel{ "Éjection", ATTACK, 5, 2 };
    check_sequence(EnemyType::CUBEX_CONSTRUCT, "Assemblage Cubex", 65,
                   { charge, blast, blast, expel, blast, blast, expel, blast });

    // Played out: Strength 2, then 9 and +2, 11 and +2, 2 hits of 11, 13 and +2.
    CombatEngine combat = make_combat(EnemyType::CUBEX_CONSTRUCT);
    const int lost[] = { 0, 9, 11, 22, 13 };
    const int strength[] = { 2, 4, 6, 6, 8 };
    for (int turn = 0; turn < 5; ++turn) {
        pass_turn(combat);
        assert(hp_lost(combat) == lost[turn]);
        assert(combat.enemies[0].get_status(StatusType::STRENGTH) == strength[turn]);
    }
    std::cout << "[PASS] Patterns: a sequence, and a loop that skips the opening move" << std::endl;
}

void test_draw_without_repeat() {
    // Inklet: Jab, then Piercing Gaze and Whirlwind, never the same twice in a row. Both can come first.
    std::set<std::string_view> second_moves;
    for (uint32_t seed = 1; seed <= 40; ++seed) {
        uint32_t state = seed * 2654435761u;
        Enemy inklet;
        inklet.init(EnemyType::INKLET);
        assert(inklet.current_intent.description == "Coup sec" && inklet.current_intent.base_damage == 3);
        std::string_view previous;
        for (int turn = 2; turn <= 12; ++turn) {
            assert(inklet.wants_roll());
            inklet.plan_turn(turn, next_roll(state));
            std::string_view move = inklet.current_intent.description;
            assert(move == "Regard perçant" || move == "Tourbillon");
            assert(move != previous);
            if (turn == 2) second_moves.insert(move);
            previous = move;
        }
    }
    assert(second_moves.size() == 2);

    // Mawler: Claw, then a draw where Roar comes once per fight and nothing comes twice in a row.
    std::set<std::string_view> seen;
    int fights_with_roar = 0;
    for (uint32_t seed = 1; seed <= 40; ++seed) {
        uint32_t state = seed * 2654435761u;
        Enemy mawler;
        mawler.init(EnemyType::MAWLER);
        assert(mawler.current_intent.description == "Griffe");
        std::string_view previous = "Griffe";
        int roars = 0;
        for (int turn = 2; turn <= 30; ++turn) {
            mawler.plan_turn(turn, next_roll(state));
            std::string_view move = mawler.current_intent.description;
            assert(move != previous);
            roars += move == "Rugissement";
            seen.insert(move);
            previous = move;
        }
        assert(roars <= 1);
        fights_with_roar += roars;
    }
    assert(seen == (std::set<std::string_view>{ "Déchirure", "Rugissement", "Griffe" }));
    assert(fights_with_roar >= 35); // 29 draws: a fight without a Roar is very unlikely

    // Fossil Stalker: its draw has no rule, so a move can come twice in a row.
    bool repeated = false;
    uint32_t state = 7;
    Enemy stalker;
    stalker.init(EnemyType::FOSSIL_STALKER);
    std::string_view previous = stalker.current_intent.description;
    for (int turn = 2; turn <= 40; ++turn) {
        stalker.plan_turn(turn, next_roll(state));
        repeated = repeated || stalker.current_intent.description == previous;
        previous = stalker.current_intent.description;
    }
    assert(repeated);
    std::cout << "[PASS] Patterns: draws with no repeat, once per fight, or free" << std::endl;
}

void test_first_move_drawn() {
    // Flyconid opens with Frail Spores or Smash, never with Vulnerable Spores, which only comes later.
    assert(Enemy::opens_with_draw(EnemyType::FLYCONID));
    assert(!Enemy::opens_with_draw(EnemyType::INKLET));
    assert(!Enemy::opens_with_draw(EnemyType::CULTIST));
    std::set<EnemyIntentType> openers;
    std::set<EnemyIntentType> later;
    for (uint32_t seed = 1; seed <= 60; ++seed) {
        uint32_t state = seed * 2654435761u;
        Enemy flyconid;
        flyconid.init(EnemyType::FLYCONID, EnemyRank::ALONE, next_roll(state));
        openers.insert(flyconid.current_intent.type);
        EnemyIntentType previous = flyconid.current_intent.type;
        for (int turn = 2; turn <= 10; ++turn) {
            assert(flyconid.wants_roll());
            flyconid.plan_turn(turn, next_roll(state));
            assert(flyconid.current_intent.type != previous);
            previous = flyconid.current_intent.type;
            later.insert(previous);
        }
    }
    assert(openers == (std::set<EnemyIntentType>{ ATTACK_DEBUFF, ATTACK }));
    assert(later == (std::set<EnemyIntentType>{ ATTACK_DEBUFF, ATTACK, DEBUFF }));

    // In a fight the first move comes from the combat's generator, and the next ones too.
    CombatEngine combat = make_combat(EnemyType::FLYCONID);
    EnemyIntentType previous = combat.enemies[0].current_intent.type;
    assert(previous == ATTACK_DEBUFF || previous == ATTACK);
    for (int turn = 0; turn < 12; ++turn) {
        pass_turn(combat);
        assert(combat.enemies[0].current_intent.type != previous);
        previous = combat.enemies[0].current_intent.type;
    }
    std::cout << "[PASS] Patterns: a first move drawn at random" << std::endl;
}

void test_phase_at_hp_threshold() {
    const Move crash{ "Fracas", ATTACK, 16 };
    const Move thrash{ "Furie", ATTACK_BUFF, 3, 3, 0, StatusType::VIGOR, 6 };
    const Move stunned{ "Étourdi", STUN };
    const Move terror{ "Aura de terreur", DEBUFF, 0, 1, 0, StatusType::NONE, 0, StatusType::VULNERABLE, 99 };

    // Above half its HP: Crash and Thrash in turn.
    check_sequence(EnemyType::TERROR_EEL, "Murène de l'effroi", 140, { crash, thrash, crash, thrash, crash });

    // The first plan at half its HP or less: stunned, Terror, then Crash and Thrash again, once per fight.
    Enemy eel;
    eel.init(EnemyType::TERROR_EEL);
    eel.plan_turn(2);
    check_intent(eel, thrash);
    eel.hp = 71;
    eel.plan_turn(3);
    check_intent(eel, crash);
    assert(!eel.phase_changed);
    eel.hp = 70;
    const Move after[] = { stunned, terror, crash, thrash, crash, thrash };
    int turn = 4;
    for (const Move& move : after) {
        eel.plan_turn(turn++);
        check_intent(eel, move);
        assert(eel.phase_changed);
    }

    // Played out: 16, 9 and 6 Vigor, then the player brings it to half its HP.
    CombatEngine combat = make_combat(EnemyType::TERROR_EEL);
    Enemy& foe = combat.enemies[0];
    pass_turn(combat);
    assert(hp_lost(combat) == 16);
    pass_turn(combat);
    assert(hp_lost(combat) == 9 && foe.get_status(StatusType::VIGOR) == 6);
    foe.hp = 60;
    pass_turn(combat); // Crash was already announced: 16 + 6 Vigor
    assert(hp_lost(combat) == 22 && foe.get_status(StatusType::VIGOR) == 0);
    assert(foe.current_intent.type == STUN);
    pass_turn(combat);
    assert(hp_lost(combat) == 0);
    assert(foe.current_intent.type == DEBUFF);
    pass_turn(combat);
    assert(hp_lost(combat) == 0 && combat.player.get_status(StatusType::VULNERABLE) == 99);
    assert(foe.intent_damage(combat.player) == 24); // Crash 16 on a Vulnerable player
    pass_turn(combat);
    assert(hp_lost(combat) == 24);
    assert(combat.player.get_status(StatusType::VULNERABLE) == 98);
    std::cout << "[PASS] Patterns: a second phase at an HP threshold, entered once" << std::endl;
}

void test_place_in_the_group() {
    const Move butt{ "Derrière", ATTACK, 12 };
    const Move slice{ "Slice", ATTACK_DEFEND, 6, 1, 5 };
    const Move hiss{ "Chuintement", BUFF, 0, 1, 0, StatusType::STRENGTH, 2 };
    check_sequence(EnemyType::NIBBIT, "Grignotin", 42, { butt, slice, hiss, butt, slice, hiss });
    check_sequence(EnemyType::NIBBIT, "Grignotin", 42, { slice, hiss, butt, slice, hiss, butt }, EnemyRank::FRONT);
    check_sequence(EnemyType::NIBBIT, "Grignotin", 42, { hiss, butt, slice, hiss, butt, slice }, EnemyRank::BACK);

    // In a fight: alone, or the first and the second of a pair.
    CombatEngine alone = make_combat(EnemyType::NIBBIT);
    assert(alone.enemies[0].current_intent.description == "Derrière");
    CombatEngine pair = make_combat({ EnemyType::NIBBIT, EnemyType::NIBBIT });
    assert(pair.enemies[0].current_intent.description == "Slice");
    assert(pair.enemies[1].current_intent.description == "Chuintement");
    pass_turn(pair);
    assert(hp_lost(pair) == 6);
    assert(pair.enemies[0].block == 5 && pair.enemies[1].get_status(StatusType::STRENGTH) == 2);
    assert(pair.enemies[0].current_intent.description == "Chuintement");
    assert(pair.enemies[1].current_intent.description == "Derrière");
    assert(pair.enemies[1].intent_damage(pair.player) == 14);
    std::cout << "[PASS] Patterns: the first move depends on the place in the group" << std::endl;
}

// ---------------------------------------------------------------------------------------------------------
// Monsters
// ---------------------------------------------------------------------------------------------------------

void test_act1a_monsters() {
    check_sequence(EnemyType::SNAPPING_JAXFRUIT, "Jaxier à clapet", 31, {
        { "Orbe d'énergie", ATTACK_BUFF, 3, 1, 0, StatusType::STRENGTH, 2 },
        { "Orbe d'énergie", ATTACK_BUFF, 3, 1, 0, StatusType::STRENGTH, 2 },
        { "Orbe d'énergie", ATTACK_BUFF, 3, 1, 0, StatusType::STRENGTH, 2 },
    });
    CombatEngine jaxfruit = make_combat(EnemyType::SNAPPING_JAXFRUIT);
    for (int lost : { 3, 5, 7 }) {
        pass_turn(jaxfruit);
        assert(hp_lost(jaxfruit) == lost);
    }

    const Move clump{ "Tir groupé", ATTACK, 8 };
    const Move sticky2{ "Tir adhésif", STATUS, 0, 1, 0, StatusType::NONE, 0, StatusType::NONE, 0, CardId::SLIMED, 2 };
    check_sequence(EnemyType::LEAF_SLIME_M, "Limon feuillu (M)", 32, { sticky2, clump, sticky2, clump });
    CombatEngine leaf = make_combat(EnemyType::LEAF_SLIME_M);
    pass_turn(leaf);
    assert(hp_lost(leaf) == 0);
    assert(count_cards(leaf, CardId::SLIMED, Pile::DISCARD) == 2);
    pass_turn(leaf);
    assert(hp_lost(leaf) == 8);

    const Move track{ "Traque", DEBUFF, 0, 1, 0, StatusType::NONE, 0, StatusType::FRAIL, 2 };
    const Move hounds{ "Lâcher les chiens", ATTACK, 1, 8 };
    check_sequence(EnemyType::TRACKER_RUBY_RAIDER, "Rôdeur Traqueur", 21, { track, hounds, hounds, hounds });
    CombatEngine tracker = make_combat(EnemyType::TRACKER_RUBY_RAIDER);
    pass_turn(tracker);
    assert(hp_lost(tracker) == 0 && tracker.player.get_status(StatusType::FRAIL) == 2);
    pass_turn(tracker);
    assert(hp_lost(tracker) == 8 && tracker.player.get_status(StatusType::FRAIL) == 1);

    // Twig Slime (M): Sticky Shot, then Pokey Pounce or Sticky Shot, never two Sticky Shots in a row.
    const std::initializer_list<Move> twig_moves = {
        { "Petit saut", ATTACK, 11 },
        { "Tir collant", STATUS, 0, 1, 0, StatusType::NONE, 0, StatusType::NONE, 0, CardId::SLIMED, 1 },
    };
    std::set<std::string_view> twig_seen;
    bool pounced_twice = false;
    for (uint32_t seed = 1; seed <= 20; ++seed) {
        uint32_t state = seed * 2654435761u;
        Enemy twig;
        twig.init(EnemyType::TWIG_SLIME_M);
        assert(twig.name == "Limon brindille (M)" && twig.hp == 26 && twig.max_hp == 26);
        assert(twig.current_intent.description == "Tir collant");
        std::string_view previous = "Tir collant";
        for (int turn = 2; turn <= 12; ++turn) {
            twig.plan_turn(turn, next_roll(state));
            check_intent(twig, move_named(twig_moves, twig.current_intent));
            std::string_view move = twig.current_intent.description;
            assert(!(move == "Tir collant" && previous == "Tir collant"));
            assert(turn != 2 || move == "Petit saut");
            pounced_twice = pounced_twice || (move == "Petit saut" && previous == "Petit saut");
            twig_seen.insert(move);
            previous = move;
        }
    }
    assert(twig_seen.size() == 2 && pounced_twice);

    // Inklet and Mawler: their moves, whatever the draw gives (the rules are in test_draw_without_repeat).
    const std::initializer_list<Move> inklet_moves = {
        { "Coup sec", ATTACK, 3 }, { "Tourbillon", ATTACK, 2, 3 }, { "Regard perçant", ATTACK, 10 },
    };
    const std::initializer_list<Move> mawler_moves = {
        { "Déchirure", ATTACK, 14 },
        { "Rugissement", DEBUFF, 0, 1, 0, StatusType::NONE, 0, StatusType::VULNERABLE, 3 },
        { "Griffe", ATTACK, 4, 2 },
    };
    const std::initializer_list<Move> flyconid_moves = {
        { "Spores fragilisantes", DEBUFF, 0, 1, 0, StatusType::NONE, 0, StatusType::VULNERABLE, 2 },
        { "Spores fragilisantes", ATTACK_DEBUFF, 8, 1, 0, StatusType::NONE, 0, StatusType::FRAIL, 2 },
        { "Fracas", ATTACK, 11 },
    };
    uint32_t state = 99;
    Enemy inklet, mawler, flyconid;
    inklet.init(EnemyType::INKLET);
    mawler.init(EnemyType::MAWLER);
    flyconid.init(EnemyType::FLYCONID, EnemyRank::ALONE, next_roll(state));
    assert(inklet.name == "Encrelin" && inklet.hp == 11 && inklet.max_hp == 11);
    assert(mawler.name == "Écharneur" && mawler.hp == 72 && mawler.max_hp == 72);
    assert(flyconid.name == "Mycoptère" && flyconid.hp == 47 && flyconid.max_hp == 47);
    for (int turn = 2; turn <= 30; ++turn) {
        check_intent(inklet, move_named(inklet_moves, inklet.current_intent));
        check_intent(mawler, move_named(mawler_moves, mawler.current_intent));
        check_intent(flyconid, move_named(flyconid_moves, flyconid.current_intent));
        inklet.plan_turn(turn, next_roll(state));
        mawler.plan_turn(turn, next_roll(state));
        flyconid.plan_turn(turn, next_roll(state));
    }

    // Three Inklets: Jab 3 each on the first turn.
    CombatEngine inklets = make_combat({ EnemyType::INKLET, EnemyType::INKLET, EnemyType::INKLET });
    pass_turn(inklets);
    assert(hp_lost(inklets) == 9);
    std::cout << "[PASS] Act 1 (A): Cubex Construct, Inklet, Nibbit, Snapping Jaxfruit, Twig and Leaf Slimes (M), "
                 "Flyconid, Tracker Ruby Raider, Mawler" << std::endl;
}

void test_act1b_monsters() {
    // Toadpole: Spiken, Spike Spit, Whirl; the ones behind the first start on Whirl.
    const Move spiken{ "Épines", BUFF, 0, 1, 0, StatusType::THORNS, 2 };
    const Move spit{ "Jet d'épines", ATTACK, 3, 3, 0, StatusType::THORNS, 2 };
    const Move whirl{ "Tourbillonnement", ATTACK, 7 };
    check_sequence(EnemyType::TOADPOLE, "Crapautard", 21, { spiken, spit, whirl, spiken, spit, whirl });
    check_sequence(EnemyType::TOADPOLE, "Crapautard", 21, { spiken, spit, whirl, spiken }, EnemyRank::FRONT);
    check_sequence(EnemyType::TOADPOLE, "Crapautard", 21, { whirl, spiken, spit, whirl }, EnemyRank::BACK);
    CombatEngine toads = make_combat({ EnemyType::TOADPOLE, EnemyType::TOADPOLE });
    pass_turn(toads); // Spiken, Whirl 7
    assert(hp_lost(toads) == 7 && toads.enemies[0].get_status(StatusType::THORNS) == 2);
    pass_turn(toads); // Spike Spit 3 x 3, Spiken
    assert(hp_lost(toads) == 9);
    assert(toads.enemies[0].get_status(StatusType::THORNS) == 4 && toads.enemies[1].get_status(StatusType::THORNS) == 2);
    // Its Thorns hurt whoever hits it.
    toads.hand.clear();
    toads.hand.push_back(CardId::STRIKE);
    toads.player.energy = toads.player.max_energy;
    toads.player.hp = toads.player.max_hp;
    assert(toads.play_card(0, 1));
    assert(hp_lost(toads) == 2);

    const Move ready{ "PRET", DEFEND, 0, 1, 10 };
    const Move fast{ "Poing rapide", ATTACK_DEBUFF, 5, 2, 0, StatusType::NONE, 0, StatusType::FRAIL, 1 };
    const Move strong{ "Poing puissant", ATTACK, 14 };
    check_sequence(EnemyType::PUNCH_CONSTRUCT, "Assemblage de pugilat", 55,
                   { ready, fast, strong, ready, fast, strong });
    CombatEngine punch = make_combat(EnemyType::PUNCH_CONSTRUCT);
    pass_turn(punch);
    assert(hp_lost(punch) == 0 && punch.enemies[0].block == 10);
    pass_turn(punch);
    assert(hp_lost(punch) == 10 && punch.enemies[0].block == 0);
    assert(punch.player.get_status(StatusType::FRAIL) == 1);
    pass_turn(punch);
    assert(hp_lost(punch) == 14 && punch.player.get_status(StatusType::FRAIL) == 0);

    const Move gimme{ "Donne-moi ça !", ATTACK, 7, 2 };
    const Move smash{ "Fracas double", ATTACK_DEBUFF, 6, 2, 0, StatusType::NONE, 0, StatusType::WEAK, 2 };
    const Move hehe{ "Héhé", ATTACK_BUFF, 8, 1, 0, StatusType::STRENGTH, 2 };
    check_sequence(EnemyType::GREMLIN_MERC, "Diablotin mercenaire", 47, { gimme, smash, hehe, gimme, smash, hehe });
    CombatEngine merc = make_combat(EnemyType::GREMLIN_MERC);
    const int merc_lost[] = { 14, 12, 8, 18, 16 };
    for (int lost : merc_lost) {
        pass_turn(merc);
        assert(hp_lost(merc) == lost);
    }
    assert(merc.enemies[0].get_status(StatusType::STRENGTH) == 2 && merc.player.get_status(StatusType::WEAK) == 2);

    const Move haunt{ "Hanter", DEBUFF, 0, 1, 0, StatusType::NONE, 0, StatusType::WEAK, 3, CardId::DAZED, 5 };
    const Move swipe{ "Griffe", ATTACK, 13 };
    const Move stomp{ "Piétinement", ATTACK, 4, 3 };
    check_sequence(EnemyType::HAUNTED_SHIP, "Navire hanté", 63, { haunt, swipe, stomp, swipe, stomp, swipe });
    CombatEngine ship = make_combat(EnemyType::HAUNTED_SHIP);
    pass_turn(ship);
    assert(hp_lost(ship) == 0 && ship.player.get_status(StatusType::WEAK) == 3);
    assert(count_cards(ship, CardId::DAZED, Pile::DISCARD) == 5);
    pass_turn(ship);
    assert(hp_lost(ship) == 13);
    pass_turn(ship);
    assert(hp_lost(ship) == 12);

    const Move whip{ "Fouet", ATTACK, 3, 2 };
    const Move glomp{ "Câlin brutal", ATTACK, 8 };
    const Move goop{ "Mucus", DEBUFF, 0, 1, 0, StatusType::NONE, 0, StatusType::FRAIL, 2 };
    check_sequence(EnemyType::CORPSE_SLUG, "Putrilimace", 25, { whip, glomp, goop, whip, glomp, goop });
    CombatEngine slugs = make_combat({ EnemyType::CORPSE_SLUG, EnemyType::CORPSE_SLUG, EnemyType::CORPSE_SLUG });
    pass_turn(slugs);
    assert(hp_lost(slugs) == 18);
    pass_turn(slugs);
    assert(hp_lost(slugs) == 24);
    pass_turn(slugs);
    assert(hp_lost(slugs) == 0 && slugs.player.get_status(StatusType::FRAIL) == 6);

    const Move kick{ "Coup de pied marin", ATTACK, 11 };
    const Move spin{ "Coup de pied tournoyant", ATTACK, 2, 4 };
    const Move burp{ "Jet de bulles", BUFF, 0, 1, 7, StatusType::STRENGTH, 1 };
    check_sequence(EnemyType::SEAPUNK, "Voyou marin", 44, { kick, spin, burp, kick, spin, burp });
    CombatEngine seapunk = make_combat(EnemyType::SEAPUNK);
    const int punk_lost[] = { 11, 8, 0, 12, 12 };
    const int punk_block[] = { 0, 0, 7, 0, 0 };
    for (int turn = 0; turn < 5; ++turn) {
        pass_turn(seapunk);
        assert(hp_lost(seapunk) == punk_lost[turn] && seapunk.enemies[0].block == punk_block[turn]);
    }

    // Sludge Spinner: Oil Spray, then a draw among its three moves with no repeat.
    const std::initializer_list<Move> spinner_moves = {
        { "Spray huileux", ATTACK_DEBUFF, 8, 1, 0, StatusType::NONE, 0, StatusType::WEAK, 1 },
        { "Heurt", ATTACK, 11 },
        { "Rage", ATTACK_BUFF, 6, 1, 0, StatusType::STRENGTH, 3 },
    };
    // Fossil Stalker: Latch, then a draw among its three moves.
    const std::initializer_list<Move> stalker_moves = {
        { "Charge", ATTACK_DEBUFF, 9, 1, 0, StatusType::NONE, 0, StatusType::FRAIL, 1 },
        { "Accrochage", ATTACK, 12 },
        { "Fouet", ATTACK, 3, 2 },
    };
    std::set<std::string_view> spinner_seen, stalker_seen;
    for (uint32_t seed = 1; seed <= 10; ++seed) {
        uint32_t state = seed * 2654435761u;
        Enemy spinner, stalker;
        spinner.init(EnemyType::SLUDGE_SPINNER);
        stalker.init(EnemyType::FOSSIL_STALKER);
        assert(spinner.name == "Tisse-boue" && spinner.hp == 37 && spinner.max_hp == 37);
        assert(stalker.name == "Traqueur fossilisé" && stalker.hp == 51 && stalker.max_hp == 51);
        assert(spinner.current_intent.description == "Spray huileux");
        assert(stalker.current_intent.description == "Accrochage");
        std::string_view previous = "Spray huileux";
        for (int turn = 2; turn <= 20; ++turn) {
            check_intent(spinner, move_named(spinner_moves, spinner.current_intent));
            check_intent(stalker, move_named(stalker_moves, stalker.current_intent));
            spinner.plan_turn(turn, next_roll(state));
            stalker.plan_turn(turn, next_roll(state));
            assert(spinner.current_intent.description != previous);
            previous = spinner.current_intent.description;
            spinner_seen.insert(previous);
            stalker_seen.insert(stalker.current_intent.description);
        }
    }
    assert(spinner_seen.size() == 3 && stalker_seen.size() == 3);
    CombatEngine spinner_fight = make_combat(EnemyType::SLUDGE_SPINNER);
    pass_turn(spinner_fight);
    assert(hp_lost(spinner_fight) == 8 && spinner_fight.player.get_status(StatusType::WEAK) == 1);
    std::cout << "[PASS] Act 1 (B): Toadpole, Sludge Spinner, Punch Construct, Gremlin Merc, Haunted Ship, "
                 "Corpse Slug, Fossil Stalker, Seapunk" << std::endl;
}

void test_act1_elites() {
    const Move swoop{ "Piqué", ATTACK, 17 };
    const Move peck{ "Coup de bec", ATTACK, 3, 3 };
    check_sequence(EnemyType::BYRDONIS, "Oizodon", 81, { swoop, peck, swoop, peck });
    CombatEngine byrdonis = make_combat(EnemyType::BYRDONIS);
    pass_turn(byrdonis);
    assert(hp_lost(byrdonis) == 17);
    pass_turn(byrdonis);
    assert(hp_lost(byrdonis) == 9);

    const Move sleep{ "Sommeil", STUN };
    const Move wake{ "Éveil", BUFF, 0, 1, 0, StatusType::STRENGTH, 10 };
    const Move slashes{ "Bourrasque", ATTACK, 13 };
    check_sequence(EnemyType::BYGONE_EFFIGY, "Idole Oubliée", 127, { sleep, wake, slashes, slashes, slashes });
    CombatEngine effigy = make_combat(EnemyType::BYGONE_EFFIGY);
    const int effigy_lost[] = { 0, 0, 23, 23 };
    for (int lost : effigy_lost) {
        pass_turn(effigy);
        assert(hp_lost(effigy) == lost);
    }

    const Move zoom{ "Bourdonnement", ATTACK, 14 };
    const Move inertia{ "Inertie", ATTACK_BUFF, 9, 1, 0, StatusType::STRENGTH, 2 };
    const Move stabs{ "Coups perforants", ATTACK, 7, 2 };
    check_sequence(EnemyType::SKULKING_COLONY, "Colonie de rôdeurs", 75,
                   { zoom, zoom, inertia, stabs, zoom, zoom, inertia, stabs });
    CombatEngine colony = make_combat(EnemyType::SKULKING_COLONY);
    const int colony_lost[] = { 14, 14, 9, 18, 16 };
    for (int lost : colony_lost) {
        pass_turn(colony);
        assert(hp_lost(colony) == lost);
    }
    // Terror Eel: see test_phase_at_hp_threshold.
    std::cout << "[PASS] Act 1 elites: Byrdonis, Bygone Effigy, Skulking Colony, Terror Eel" << std::endl;
}

void test_hand_written_monsters_draw_nothing() {
    // The monsters written by hand take no number from the generator: their fights shuffle as before.
    for (int i = 0; i <= static_cast<int>(EnemyType::SEWER_CLAM); ++i) {
        const EnemyType type = static_cast<EnemyType>(i);
        assert(!Enemy::opens_with_draw(type));
        Enemy enemy;
        enemy.init(type);
        assert(!enemy.wants_roll());
        assert(enemy.current_intent.hits == 1 && enemy.current_intent.card_count == 0);
        assert(enemy.current_intent.player_status == StatusType::NONE);
    }
    std::cout << "[PASS] Monsters written by hand: no draw, single hits, nothing given to the player" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Enemy moves ===" << std::endl;
    test_multi_hit_attack();
    test_debuff_on_player();
    test_status_cards();
    test_attack_then_buff();
    test_stun_heal_escape();
    test_intent_kinds();
    test_sequence_and_loop();
    test_draw_without_repeat();
    test_first_move_drawn();
    test_phase_at_hp_threshold();
    test_place_in_the_group();
    test_act1a_monsters();
    test_act1b_monsters();
    test_act1_elites();
    test_hand_written_monsters_draw_nothing();
    std::cout << "=== All Enemy Moves Passed! ===" << std::endl;
    return 0;
}
