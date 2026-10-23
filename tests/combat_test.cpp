// Combat rules audit: one test per mechanic, grouped by hero, on hand-built situations.
#include <algorithm>
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

// Jaw Worm opens with Chomp (11 damage, no block): a predictable punching bag.
CombatEngine make_combat(CharacterId hero, EnemyType enemy_type = EnemyType::JAW_WORM) {
    CombatEngine combat;
    combat.start_combat(enemy_type, hero);
    return combat;
}

// Puts `card` alone in hand with full energy and plays it.
void play(CombatEngine& combat, DeckCard card, int target = -1) {
    combat.hand.clear();
    combat.hand.push_back(card);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, target);
    assert(played);
}

int count_in_piles(const CombatEngine& combat, CardId id) {
    int count = 0;
    for (const DeckCard& card : combat.hand) count += card.id == id;
    for (const DeckCard& card : combat.draw_pile) count += card.id == id;
    for (const DeckCard& card : combat.discard_pile) count += card.id == id;
    for (const DeckCard& card : combat.exhaust_pile) count += card.id == id;
    return count;
}

void make_enemy_idle(Enemy& enemy) {
    enemy.current_intent = EnemyIntent{};
    enemy.current_intent.type = EnemyIntentType::DEFEND;
}

void test_attack_formula() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    Enemy& worm = combat.enemies[0];

    play(combat, CardId::STRIKE);
    assert(worm.hp == 42 - 6);

    combat.player.add_status(StatusType::STRENGTH, 2);
    play(combat, CardId::STRIKE);
    assert(worm.hp == 36 - 8);
    combat.player.add_status(StatusType::STRENGTH, -2);
    assert(combat.player.statuses.empty()); // a spent status frees its slot

    // Weak and Vulnerable are applied together and rounded once: 9 x 0.75 x 1.5 = 10.125 -> 10.
    worm.hp = 42;
    combat.player.add_status(StatusType::WEAK, 1);
    worm.add_status(StatusType::VULNERABLE, 1);
    play(combat, DeckCard(CardId::STRIKE, true));
    assert(worm.hp == 42 - 10);

    // Negative Strength never heals the target.
    combat.player.add_status(StatusType::STRENGTH, -20);
    play(combat, CardId::STRIKE);
    assert(worm.hp == 32);

    // Not enough energy: the card stays in hand.
    combat.hand.clear();
    combat.hand.push_back(CardId::BASH);
    combat.player.energy = 1;
    assert(!combat.play_card(0));
    assert(combat.hand.size() == 1);
    std::cout << "[PASS] Attack formula: Strength, Weak x0.75, Vulnerable x1.5 rounded once, energy check" << std::endl;
}

void test_turn_flow() {
    // Player debuffs last through the enemy attack, then wear off at the end of the round.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    combat.player.add_status(StatusType::VULNERABLE, 1);
    combat.end_player_turn();
    assert(combat.player.hp == 80 - 16); // Chomp 11 x 1.5
    assert(combat.player.get_status(StatusType::VULNERABLE) == 0);
    assert(combat.player.energy == 3 && combat.hand.size() == 5);

    // An empty draw pile reshuffles the discard pile.
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    for (int i = 0; i < 3; ++i) combat.discard_pile.push_back(CardId::STRIKE);
    combat.draw_cards(2);
    assert(combat.hand.size() == 2 && combat.draw_pile.size() == 1 && combat.discard_pile.empty());

    // Killing the last enemy wins immediately.
    combat.enemies[0].hp = 5;
    play(combat, CardId::STRIKE);
    assert(combat.state == CombatState::VICTORY);

    // Poison on the player is checked at the start of its turn.
    CombatEngine poisoned = make_combat(CharacterId::SILENT);
    make_enemy_idle(poisoned.enemies[0]);
    poisoned.player.hp = 3;
    poisoned.player.add_status(StatusType::POISON, 5);
    poisoned.end_player_turn();
    assert(poisoned.player.hp == 0);
    assert(poisoned.state == CombatState::DEFEAT);
    std::cout << "[PASS] Turn flow: player debuffs last the round, reshuffle, victory, lethal poison on the player" << std::endl;
}

void test_cultist_ritual() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD, EnemyType::CULTIST);
    combat.end_player_turn(); // Incantation: Ritual 3, no Strength yet
    assert(combat.player.hp == 80);
    assert(combat.enemies[0].get_status(StatusType::STRENGTH) == 0);
    combat.end_player_turn(); // Dark Strike 6, then +3 Strength
    assert(combat.player.hp == 74);
    assert(combat.enemies[0].get_status(StatusType::STRENGTH) == 3);
    combat.end_player_turn(); // Dark Strike 6 + 3
    assert(combat.player.hp == 65);
    std::cout << "[PASS] Cultist: Ritual starts granting Strength the turn after Incantation (0, 6, 9 damage)" << std::endl;
}

void test_ironclad() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    Enemy& worm = combat.enemies[0];

    // Bash: 8 damage and 2 Vulnerable, which lasts two enemy turns.
    play(combat, CardId::BASH);
    assert(worm.hp == 34 && worm.get_status(StatusType::VULNERABLE) == 2);
    play(combat, CardId::STRIKE);
    assert(worm.hp == 25);
    combat.end_player_turn();
    assert(worm.get_status(StatusType::VULNERABLE) == 1);
    play(combat, CardId::STRIKE);
    assert(worm.hp == 16);
    combat.end_player_turn(); // Thrash: the worm gains 5 block
    assert(worm.get_status(StatusType::VULNERABLE) == 0);
    play(combat, CardId::STRIKE);
    assert(worm.hp == 16 - (6 - 5));

    // Iron Wave: block and damage.
    CombatEngine wave = make_combat(CharacterId::IRONCLAD);
    play(wave, CardId::IRON_WAVE);
    assert(wave.player.block == 5 && wave.enemies[0].hp == 37);

    // Cleave hits every enemy, each with its own Vulnerable.
    CombatEngine cleave;
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::CULTIST);
    cleave.start_combat_multi(pack, CharacterId::IRONCLAD);
    cleave.enemies[2].add_status(StatusType::VULNERABLE, 1);
    play(cleave, CardId::CLEAVE);
    assert(cleave.enemies[0].hp == 10 && cleave.enemies[1].hp == 10 && cleave.enemies[2].hp == 38);

    // Anger adds a copy of itself to the discard pile.
    CombatEngine anger = make_combat(CharacterId::IRONCLAD);
    anger.discard_pile.clear();
    play(anger, CardId::ANGER);
    assert(anger.enemies[0].hp == 36);
    assert(count_in_piles(anger, CardId::ANGER) == 2);

    // Metallicize is a Power: it leaves the piles, and its block already protects on the turn it is played.
    CombatEngine metal = make_combat(CharacterId::IRONCLAD);
    play(metal, CardId::METALLICIZE);
    assert(count_in_piles(metal, CardId::METALLICIZE) == 0);
    metal.end_player_turn();
    assert(metal.player.hp == 80 - (11 - 3));
    std::cout << "[PASS] Ironclad: Bash + Vulnerable duration, Iron Wave, Cleave, Anger copy, Metallicize power" << std::endl;
}

void test_ironclad_commons() {
    // Twin Strike hits twice, each hit gets Strength.
    CombatEngine twin = make_combat(CharacterId::IRONCLAD);
    twin.player.add_status(StatusType::STRENGTH, 1);
    play(twin, CardId::TWIN_STRIKE);
    assert(twin.enemies[0].hp == 42 - 2 * 6);
    play(twin, DeckCard(CardId::TWIN_STRIKE, true));
    assert(twin.enemies[0].hp == 30 - 2 * 8);

    // Pommel Strike deals damage and draws (2 cards once upgraded).
    CombatEngine pommel = make_combat(CharacterId::IRONCLAD);
    play(pommel, CardId::POMMEL_STRIKE);
    assert(pommel.enemies[0].hp == 42 - 9);
    assert(pommel.hand.size() == 1);
    play(pommel, DeckCard(CardId::POMMEL_STRIKE, true));
    assert(pommel.enemies[0].hp == 33 - 10);
    assert(pommel.hand.size() == 2);

    // Shrug It Off blocks and draws.
    CombatEngine shrug = make_combat(CharacterId::IRONCLAD);
    shrug.player.block = 0;
    play(shrug, CardId::SHRUG_IT_OFF);
    assert(shrug.player.block == 8);
    assert(shrug.hand.size() == 1);
    assert(count_in_piles(shrug, CardId::SHRUG_IT_OFF) == 1); // back in the discard pile

    // Taunt blocks and makes its target Vulnerable (the other enemies are untouched).
    CombatEngine taunt;
    FixedVector<EnemyType, 3> duo;
    duo.push_back(EnemyType::LOUSE);
    duo.push_back(EnemyType::JAW_WORM);
    taunt.start_combat_multi(duo, CharacterId::IRONCLAD);
    taunt.player.block = 0;
    play(taunt, DeckCard(CardId::TAUNT, true), 1);
    assert(taunt.player.block == 7);
    assert(taunt.enemies[1].get_status(StatusType::VULNERABLE) == 2);
    assert(taunt.enemies[0].get_status(StatusType::VULNERABLE) == 0);

    // All four are offered after Ironclad combats.
    std::span<const CardId> pool = RunManager::card_reward_pool(CharacterId::IRONCLAD);
    for (CardId id : { CardId::TWIN_STRIKE, CardId::POMMEL_STRIKE, CardId::SHRUG_IT_OFF, CardId::TAUNT }) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
    std::cout << "[PASS] Ironclad commons: Twin Strike, Pommel Strike, Shrug It Off, Taunt" << std::endl;
}

void test_silent() {
    // Neutralize: Weak reduces the enemy's next attack by 25 %, then wears off.
    CombatEngine combat = make_combat(CharacterId::SILENT);
    Enemy& worm = combat.enemies[0];
    play(combat, CardId::NEUTRALIZE);
    assert(worm.hp == 39 && worm.get_status(StatusType::WEAK) == 1);
    combat.end_player_turn();
    assert(combat.player.hp == 70 - 8); // Chomp 11 x 0.75
    assert(worm.get_status(StatusType::WEAK) == 0);

    // Poison Stab: poison ticks at the start of the enemy turn, then decreases.
    CombatEngine stab = make_combat(CharacterId::SILENT);
    play(stab, CardId::POISON_STAB);
    assert(stab.enemies[0].hp == 36 && stab.enemies[0].get_status(StatusType::POISON) == 3);
    stab.end_player_turn();
    assert(stab.enemies[0].hp == 33 && stab.enemies[0].get_status(StatusType::POISON) == 2);

    // Lethal poison kills the enemy before it can attack.
    CombatEngine deadly = make_combat(CharacterId::SILENT);
    deadly.enemies[0].hp = 4;
    play(deadly, CardId::DEADLY_POISON);
    assert(deadly.enemies[0].get_status(StatusType::POISON) == 5);
    deadly.end_player_turn();
    assert(!deadly.enemies[0].is_alive());
    assert(deadly.player.hp == 70);
    assert(deadly.state == CombatState::VICTORY);
    assert(deadly.last_enemy_actions.empty());

    // Poison that runs out frees its status slot.
    Enemy dummy;
    dummy.init(EnemyType::CULTIST);
    dummy.add_status(StatusType::POISON, 1);
    dummy.apply_poison_tick();
    assert(dummy.statuses.empty());

    // Survivor block benefits from Dexterity.
    CombatEngine survivor = make_combat(CharacterId::SILENT);
    survivor.player.add_status(StatusType::DEXTERITY, 2);
    play(survivor, CardId::SURVIVOR);
    assert(survivor.player.block == 10);
    std::cout << "[PASS] Silent: Neutralize Weak, Poison Stab tick, lethal poison before the attack, Survivor + Dexterity" << std::endl;
}

void test_silent_commons() {
    // Slice costs nothing.
    CombatEngine slice = make_combat(CharacterId::SILENT);
    slice.hand.clear();
    slice.hand.push_back(DeckCard(CardId::SLICE, true));
    slice.player.energy = 0;
    assert(slice.play_card(0));
    assert(slice.enemies[0].hp == 42 - 9);

    // Backflip blocks (with Dexterity) and draws 2.
    CombatEngine flip = make_combat(CharacterId::SILENT);
    flip.player.block = 0;
    flip.player.add_status(StatusType::DEXTERITY, 1);
    play(flip, CardId::BACKFLIP);
    assert(flip.player.block == 6);
    assert(flip.hand.size() == 2);

    // Sucker Punch hits and weakens.
    CombatEngine punch = make_combat(CharacterId::SILENT);
    play(punch, DeckCard(CardId::SUCKER_PUNCH, true));
    assert(punch.enemies[0].hp == 42 - 10);
    assert(punch.enemies[0].get_status(StatusType::WEAK) == 2);

    // Dagger Spray hits every enemy twice, and skips the ones killed by the first volley.
    CombatEngine spray;
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::JAW_WORM);
    spray.start_combat_multi(pack, CharacterId::SILENT);
    spray.enemies[0].hp = 3;
    int16_t worm_hp = spray.enemies[1].hp;
    play(spray, CardId::DAGGER_SPRAY);
    assert(!spray.enemies[0].is_alive());
    assert(spray.enemies[1].hp == worm_hp - 2 * 4);

    std::span<const CardId> pool = RunManager::card_reward_pool(CharacterId::SILENT);
    for (CardId id : { CardId::SLICE, CardId::BACKFLIP, CardId::SUCKER_PUNCH, CardId::DAGGER_SPRAY }) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
    std::cout << "[PASS] Silent commons: Slice, Backflip, Sucker Punch, Dagger Spray" << std::endl;
}

void test_defect() {
    // Zap channels Lightning; its passive hits at the end of the turn.
    CombatEngine combat = make_combat(CharacterId::DEFECT);
    play(combat, CardId::ZAP);
    assert(combat.player.orbs.size() == 1 && combat.enemies[0].hp == 42);
    combat.end_player_turn();
    assert(combat.enemies[0].hp == 39);
    // The scene shows the Lightning damage before the enemies act from the HP kept at the end of the turn.
    assert(combat.turn_end_enemy_hp.size() == 1 && combat.turn_end_enemy_hp[0] == 39);
    // Mercury Hourglass hits at the start of the next turn: not part of the end of the turn.
    CombatEngine hourglass = make_combat(CharacterId::DEFECT);
    hourglass.add_power(PowerId::MERCURY_HOURGLASS, 3);
    hourglass.end_player_turn();
    assert(hourglass.enemies[0].hp == 39 && hourglass.turn_end_enemy_hp[0] == 42);

    // Frost passive block (2, Dexterity ignored) protects during the enemy turn.
    CombatEngine frost = make_combat(CharacterId::DEFECT);
    frost.player.add_status(StatusType::DEXTERITY, 5);
    frost.channel_orb(OrbType::FROST);
    frost.end_player_turn();
    assert(frost.player.hp == 75 - (11 - 2));

    // Dualcast evokes the same orb twice.
    CombatEngine dual = make_combat(CharacterId::DEFECT);
    dual.channel_orb(OrbType::LIGHTNING);
    dual.channel_orb(OrbType::FROST);
    play(dual, CardId::DUALCAST);
    assert(dual.enemies[0].hp == 42 - 16);
    assert(dual.player.orbs.size() == 1 && dual.player.orbs[0] == OrbType::FROST);
    assert(dual.player.block == 0);

    // Dark charges +6 per turn and deals its charge on evoke.
    CombatEngine dark = make_combat(CharacterId::DEFECT);
    make_enemy_idle(dark.enemies[0]);
    dark.channel_orb(OrbType::DARK);
    assert(dark.player.orbs[0].charge == 6);
    dark.end_player_turn();
    assert(dark.player.orbs[0].charge == 12);
    play(dark, CardId::DUALCAST);
    assert(dark.enemies[0].hp == 42 - 24);

    // Channeling into full slots evokes the oldest orb first.
    CombatEngine full = make_combat(CharacterId::DEFECT);
    Enemy& worm = full.enemies[0];
    for (int i = 0; i < 3; ++i) full.channel_orb(OrbType::LIGHTNING);
    assert(worm.hp == 42);
    full.channel_orb(OrbType::FROST);
    assert(worm.hp == 34);
    assert(full.player.orbs.size() == 3 && full.player.orbs[2] == OrbType::FROST);

    // Plasma: +1 energy at the start of each turn, +2 on evoke.
    CombatEngine plasma = make_combat(CharacterId::DEFECT);
    plasma.channel_orb(OrbType::PLASMA);
    plasma.end_player_turn();
    assert(plasma.player.energy == 4);
    plasma.evoke_orb(0);
    assert(plasma.player.energy == 6);
    std::cout << "[PASS] Defect: Zap + Lightning passive, Frost block, Dualcast twice, Dark charge, full slots, Plasma energy" << std::endl;
}

void test_defect_commons() {
    // Beam Cell: free chip damage and Vulnerable.
    CombatEngine beam = make_combat(CharacterId::DEFECT);
    beam.hand.clear();
    beam.hand.push_back(CardId::BEAM_CELL);
    beam.player.energy = 0;
    assert(beam.play_card(0));
    assert(beam.enemies[0].hp == 42 - 3);
    assert(beam.enemies[0].get_status(StatusType::VULNERABLE) == 1);

    // Ball Lightning and Cold Snap hit, then channel their orb.
    CombatEngine orbs = make_combat(CharacterId::DEFECT);
    play(orbs, CardId::BALL_LIGHTNING);
    assert(orbs.enemies[0].hp == 42 - 7);
    play(orbs, DeckCard(CardId::COLD_SNAP, true));
    assert(orbs.enemies[0].hp == 35 - 9);
    assert(orbs.player.orbs.size() == 2);
    assert(orbs.player.orbs[0] == OrbType::LIGHTNING && orbs.player.orbs[1] == OrbType::FROST);

    // With every slot taken, channeling evokes the oldest orb on the target (Lightning: 8).
    play(orbs, CardId::BALL_LIGHTNING);
    int16_t before = orbs.enemies[0].hp;
    play(orbs, CardId::BALL_LIGHTNING);
    assert(orbs.player.orbs.size() == 3 && orbs.player.orbs[0] == OrbType::FROST);
    assert(orbs.enemies[0].hp == before - 7 - 8);

    // Leap is plain block.
    CombatEngine leap = make_combat(CharacterId::DEFECT);
    leap.player.block = 0;
    play(leap, DeckCard(CardId::LEAP, true));
    assert(leap.player.block == 12);

    std::span<const CardId> pool = RunManager::card_reward_pool(CharacterId::DEFECT);
    for (CardId id : { CardId::BEAM_CELL, CardId::BALL_LIGHTNING, CardId::COLD_SNAP, CardId::LEAP }) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
    assert(std::find(pool.begin(), pool.end(), CardId::STRIKE) == pool.end());
    std::cout << "[PASS] Defect commons: Beam Cell, Ball Lightning, Cold Snap, Leap" << std::endl;
}

void test_regent() {
    // Stars are a resource: Venerate makes them, Falling Star spends 2.
    CombatEngine combat = make_combat(CharacterId::REGENT);
    Enemy& worm = combat.enemies[0];
    combat.hand.clear();
    combat.hand.push_back(CardId::FALLING_STAR);
    assert(!combat.can_play_card(0)); // free in energy, but no Star yet
    play(combat, CardId::VENERATE);
    assert(combat.player.block == 0 && combat.player.sovereign_stars == 2 && combat.player.energy == 2);
    assert(combat.counters.stars_gained == 2);
    play(combat, CardId::FALLING_STAR);
    assert(worm.hp == 42 - 8 && combat.player.sovereign_stars == 0 && combat.player.energy == 3);
    assert(worm.get_status(StatusType::WEAK) == 1 && worm.get_status(StatusType::VULNERABLE) == 1);
    assert(combat.counters.stars_spent == 2);
    assert(CardDef::get_star_cost(CardId::FALLING_STAR, true) == 2 && CardDef::get_value(CardId::FALLING_STAR, true) == 12);

    // Stars are not capped and are kept from one turn to the next; the turn counters restart.
    combat.gain_stars(10);
    assert(combat.player.sovereign_stars == 10);
    make_enemy_idle(worm);
    combat.end_player_turn();
    assert(combat.player.sovereign_stars == 10 && combat.counters.stars_gained == 0 && combat.counters.stars_spent == 0);

    // Strike and Defend get nothing from the Stars.
    play(combat, CardId::STRIKE);
    play(combat, CardId::DEFEND);
    assert(worm.hp == 34 - 6 && combat.player.block == 5);
    std::cout << "[PASS] Regent: Stars gained, spent, kept between turns, Venerate, Falling Star" << std::endl;
}

void test_regent_commons() {
    // Solar Strike hits and gains Stars.
    CombatEngine solar = make_combat(CharacterId::REGENT);
    play(solar, DeckCard(CardId::SOLAR_STRIKE, true));
    assert(solar.enemies[0].hp == 42 - 10);
    assert(solar.player.sovereign_stars == 2);

    // Gather Light blocks and gains a Star.
    CombatEngine gather = make_combat(CharacterId::REGENT);
    gather.player.block = 0;
    play(gather, CardId::GATHER_LIGHT);
    assert(gather.player.block == 8 && gather.player.sovereign_stars == 1);

    // Celestial Might: 3 hits (4 once upgraded), each with Strength, for 2 energy.
    CombatEngine might = make_combat(CharacterId::REGENT);
    might.player.add_status(StatusType::STRENGTH, 1);
    play(might, CardId::CELESTIAL_MIGHT);
    assert(might.enemies[0].hp == 42 - 3 * 7);
    assert(might.player.energy == might.player.max_energy - 2);
    might.enemies[0].hp = 42;
    play(might, DeckCard(CardId::CELESTIAL_MIGHT, true));
    assert(might.enemies[0].hp == 42 - 4 * 7);

    std::span<const CardId> pool = RunManager::card_reward_pool(CharacterId::REGENT);
    for (CardId id : { CardId::SOLAR_STRIKE, CardId::GATHER_LIGHT, CardId::CELESTIAL_MIGHT }) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
    assert(std::find(pool.begin(), pool.end(), CardId::DEFEND) == pool.end());
    std::cout << "[PASS] Regent commons: Solar Strike, Gather Light, Celestial Might" << std::endl;
}

void test_necrobinder() {
    CombatEngine combat = make_combat(CharacterId::NECROBINDER);
    combat.enemies[0].add_status(StatusType::VULNERABLE, 1);
    combat.player.summon_minion(2);
    play(combat, CardId::UNLEASH);
    assert(combat.enemies[0].hp == 42 - 12); // Osty deals 6 + his 2 HP, and Vulnerable applies to Unleash too

    // Bodyguard summons for 5: Osty soaks the hit first, then the block, then the Necrobinder.
    CombatEngine guard = make_combat(CharacterId::NECROBINDER);
    play(guard, CardId::BODYGUARD);
    assert(guard.player.block == 0 && guard.player.minion_hp == 5);
    guard.player.block = 4;
    guard.end_player_turn();
    assert(guard.player.minion_hp == 0);
    assert(guard.player.hp == 65 - (11 - 5 - 4));
    std::cout << "[PASS] Necrobinder: Unleash + Vulnerable, Bodyguard summon and Osty absorbing damage" << std::endl;
}

// Enters the first map room after forcing its type, like a player walking into it.
void enter_room_of_type(RunManager& run, RoomType type) {
    int index = type == RoomType::BOSS ? run.map.boss_node_index : 0;
    run.map.nodes[index].type = type;
    run.enter_room(index);
}

void test_necrobinder_commons() {
    // Pull Aggro (Râblure) summons Osty's HP and blocks, for 2 energy.
    CombatEngine aggro = make_combat(CharacterId::NECROBINDER);
    aggro.player.block = 0;
    int16_t osty_hp = aggro.player.minion_hp;
    play(aggro, CardId::PULL_AGGRO);
    assert(aggro.player.minion_hp == osty_hp + 4);
    assert(aggro.player.block == 7);
    assert(aggro.player.energy == aggro.player.max_energy - 2);

    // Drain Power hits and upgrades random non-upgraded cards of the discard pile (never itself).
    CombatEngine drain = make_combat(CharacterId::NECROBINDER);
    drain.discard_pile.clear();
    drain.discard_pile.push_back(CardId::STRIKE);
    drain.discard_pile.push_back(DeckCard(CardId::DEFEND, true));
    drain.discard_pile.push_back(CardId::UNLEASH);
    drain.discard_pile.push_back(CardId::BODYGUARD);
    play(drain, CardId::DRAIN_POWER);
    assert(drain.enemies[0].hp == 42 - 10);
    int upgraded = 0;
    for (const DeckCard& card : drain.discard_pile) upgraded += card.upgraded;
    assert(upgraded == 1 + 2);
    assert(drain.discard_pile.back() == DeckCard(CardId::DRAIN_POWER, false));

    // With fewer upgradable cards than asked, it upgrades what it can.
    CombatEngine short_drain = make_combat(CharacterId::NECROBINDER);
    short_drain.discard_pile.clear();
    short_drain.discard_pile.push_back(CardId::STRIKE);
    play(short_drain, DeckCard(CardId::DRAIN_POWER, true));
    assert(short_drain.enemies[0].hp == 42 - 12);
    assert(short_drain.discard_pile[0].upgraded);

    std::span<const CardId> pool = RunManager::card_reward_pool(CharacterId::NECROBINDER);
    for (CardId id : { CardId::PULL_AGGRO, CardId::DRAIN_POWER }) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
    assert(std::find(pool.begin(), pool.end(), CardId::STRIKE) == pool.end());
    std::cout << "[PASS] Necrobinder commons: Pull Aggro, Drain Power" << std::endl;
}

void test_more_commons() {
    // Body Slam deals damage equal to the block (Strength applies), and costs 0 once upgraded.
    CombatEngine slam = make_combat(CharacterId::IRONCLAD);
    slam.player.block = 12;
    slam.player.add_status(StatusType::STRENGTH, 2);
    play(slam, CardId::BODY_SLAM);
    assert(slam.enemies[0].hp == 42 - 14);
    assert(CardDef::get_cost(CardId::BODY_SLAM, true) == 0);

    // Deflect: free block.
    CombatEngine deflect = make_combat(CharacterId::SILENT);
    deflect.player.block = 0;
    deflect.hand.clear();
    deflect.hand.push_back(DeckCard(CardId::DEFLECT, true));
    deflect.player.energy = 0;
    assert(deflect.play_card(0));
    assert(deflect.player.block == 7);

    // Coolheaded channels Frost and draws.
    CombatEngine cool = make_combat(CharacterId::DEFECT);
    play(cool, DeckCard(CardId::COOLHEADED, true));
    assert(cool.player.orbs.size() == 1 && cool.player.orbs[0] == OrbType::FROST);
    assert(cool.hand.size() == 2);

    // Sweeping Beam hits every enemy and draws.
    CombatEngine beam;
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::JAW_WORM);
    beam.start_combat_multi(pack, CharacterId::DEFECT);
    int16_t louse_hp = beam.enemies[0].hp;
    play(beam, CardId::SWEEPING_BEAM);
    assert(beam.enemies[0].hp == (louse_hp > 6 ? louse_hp - 6 : 0) && beam.enemies[1].hp == 42 - 6);
    assert(beam.hand.size() == 1);

    for (auto [hero, id] : { std::pair{ CharacterId::IRONCLAD, CardId::BODY_SLAM },
                             std::pair{ CharacterId::SILENT, CardId::DEFLECT },
                             std::pair{ CharacterId::DEFECT, CardId::COOLHEADED },
                             std::pair{ CharacterId::DEFECT, CardId::SWEEPING_BEAM } }) {
        std::span<const CardId> pool = RunManager::card_reward_pool(hero);
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
    std::cout << "[PASS] More commons: Body Slam, Deflect, Coolheaded, Sweeping Beam" << std::endl;
}

// A fight against three enemies: two Lice (18 HP) and a Jaw Worm (42 HP).
CombatEngine make_pack(CharacterId hero) {
    CombatEngine combat;
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::JAW_WORM);
    combat.start_combat_multi(pack, hero);
    return combat;
}

void assert_in_pool(CharacterId hero, std::initializer_list<CardId> cards) {
    std::span<const CardId> pool = RunManager::card_reward_pool(hero);
    for (CardId id : cards) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
}

void test_ironclad_existing_mechanics() {
    // Thunderclap: damage then Vulnerable on every enemy.
    CombatEngine clap = make_pack(CharacterId::IRONCLAD);
    play(clap, CardId::THUNDERCLAP);
    assert(clap.enemies[0].hp == 14 && clap.enemies[1].hp == 14 && clap.enemies[2].hp == 38);
    for (const Enemy& foe : clap.enemies) assert(foe.get_status(StatusType::VULNERABLE) == 1);
    assert(CardDef::get_value(CardId::THUNDERCLAP, true) == 7);

    // Dismantle hits twice when the target is already Vulnerable (8 x 1.5 = 12 per hit).
    CombatEngine dismantle = make_combat(CharacterId::IRONCLAD);
    play(dismantle, CardId::DISMANTLE);
    assert(dismantle.enemies[0].hp == 42 - 8);
    dismantle.enemies[0].add_status(StatusType::VULNERABLE, 1);
    play(dismantle, CardId::DISMANTLE);
    assert(dismantle.enemies[0].hp == 34 - 24);

    // Inflame is a Power: Strength for the whole combat, and the card leaves play.
    CombatEngine inflame = make_combat(CharacterId::IRONCLAD);
    inflame.discard_pile.clear();
    play(inflame, DeckCard(CardId::INFLAME, true));
    assert(inflame.player.get_status(StatusType::STRENGTH) == 3);
    assert(count_in_piles(inflame, CardId::INFLAME) == 0);

    // Perfected Strike: 6 + 2 per card named "Frappe" in the deck, itself included.
    CombatEngine perfected = make_combat(CharacterId::IRONCLAD);
    perfected.draw_pile.clear();
    perfected.discard_pile.clear();
    perfected.draw_pile.push_back(CardId::STRIKE);
    perfected.draw_pile.push_back(CardId::DEFEND);
    perfected.discard_pile.push_back(CardId::TWIN_STRIKE);
    perfected.discard_pile.push_back(CardId::POMMEL_STRIKE);
    play(perfected, CardId::PERFECTED_STRIKE);
    assert(perfected.enemies[0].hp == 42 - (6 + 2 * 4));
    assert(CardDef::get_secondary(CardId::PERFECTED_STRIKE, true) == 3);

    // Bludgeon: 32 damage for 3 energy, 42 once upgraded.
    CombatEngine bludgeon = make_combat(CharacterId::IRONCLAD);
    play(bludgeon, CardId::BLUDGEON);
    assert(bludgeon.enemies[0].hp == 10 && bludgeon.player.energy == 0);
    assert(CardDef::get_value(CardId::BLUDGEON, true) == 42);

    // Bully: +2 damage per Vulnerable stack, then Vulnerable applies: (4 + 2 x 3) x 1.5 = 15.
    CombatEngine bully = make_combat(CharacterId::IRONCLAD);
    bully.enemies[0].add_status(StatusType::VULNERABLE, 3);
    play(bully, CardId::BULLY);
    assert(bully.enemies[0].hp == 42 - 15 && bully.player.energy == 3);

    // Sword Boomerang: 3 hits of 3 spread at random over the living enemies, 4 hits once upgraded.
    CombatEngine boomerang = make_pack(CharacterId::IRONCLAD);
    play(boomerang, CardId::SWORD_BOOMERANG);
    assert(boomerang.enemies[0].hp + boomerang.enemies[1].hp + boomerang.enemies[2].hp == 18 + 18 + 42 - 9);
    boomerang.enemies[0].hp = 0;
    boomerang.enemies[1].hp = 0;
    int16_t survivor_hp = boomerang.enemies[2].hp;
    play(boomerang, DeckCard(CardId::SWORD_BOOMERANG, true));
    assert(boomerang.enemies[2].hp == survivor_hp - 12); // only the survivor can be hit
    assert(CardDef::get(CardId::SWORD_BOOMERANG).target == TargetType::RANDOM_ENEMY);

    assert_in_pool(CharacterId::IRONCLAD, { CardId::THUNDERCLAP, CardId::DISMANTLE, CardId::INFLAME,
                                            CardId::PERFECTED_STRIKE, CardId::BLUDGEON, CardId::BULLY,
                                            CardId::SWORD_BOOMERANG });
    std::cout << "[PASS] Ironclad: Thunderclap, Dismantle, Inflame, Perfected Strike, Bludgeon, Bully, Sword Boomerang"
              << std::endl;

    // Break: 20 damage and 5 Vulnerable (30 and 7 upgraded).
    CombatEngine brk = make_combat(CharacterId::IRONCLAD);
    play(brk, CardId::BREAK);
    assert(brk.enemies[0].hp == 22 && brk.enemies[0].get_status(StatusType::VULNERABLE) == 5);
    assert(CardDef::get_value(CardId::BREAK, true) == 30 && CardDef::get_secondary(CardId::BREAK, true) == 7);

    // Conflagration: 2 damage to every enemy, 4 times.
    CombatEngine fire = make_pack(CharacterId::IRONCLAD);
    play(fire, CardId::CONFLAGRATION);
    assert(fire.enemies[0].hp == 10 && fire.enemies[1].hp == 10 && fire.enemies[2].hp == 34);

    // Expect a Fight: 15 Block plus 5 per point of Strength; negative Strength adds nothing.
    CombatEngine expect = make_combat(CharacterId::IRONCLAD);
    expect.player.block = 0;
    expect.player.add_status(StatusType::STRENGTH, 2);
    play(expect, CardId::EXPECT_A_FIGHT);
    assert(expect.player.block == 25);
    expect.player.block = 0;
    expect.player.add_status(StatusType::STRENGTH, -5);
    play(expect, DeckCard(CardId::EXPECT_A_FIGHT, true));
    assert(expect.player.block == 16);

    // Pillage: draws until a card that is not an Attack comes up.
    CombatEngine pillage = make_combat(CharacterId::IRONCLAD);
    pillage.draw_pile.clear();
    pillage.discard_pile.clear();
    pillage.draw_pile.push_back(CardId::STRIKE);  // never reached
    pillage.draw_pile.push_back(CardId::DEFEND);  // third draw: stops here
    pillage.draw_pile.push_back(CardId::STRIKE);
    pillage.draw_pile.push_back(CardId::BASH);    // top of the pile
    play(pillage, CardId::PILLAGE);
    assert(pillage.enemies[0].hp == 36);
    assert(pillage.hand.size() == 3 && pillage.hand[2].id == CardId::DEFEND && pillage.draw_pile.size() == 1);

    // Uppercut: 13 damage, 1 Weak and 1 Vulnerable (2 and 2 upgraded).
    CombatEngine uppercut = make_combat(CharacterId::IRONCLAD);
    play(uppercut, DeckCard(CardId::UPPERCUT, true));
    assert(uppercut.enemies[0].hp == 29);
    assert(uppercut.enemies[0].get_status(StatusType::WEAK) == 2);
    assert(uppercut.enemies[0].get_status(StatusType::VULNERABLE) == 2);

    // Fight Me!: two hits of 5, then 3 Strength for the hero and 1 for the enemy.
    CombatEngine fight = make_combat(CharacterId::IRONCLAD);
    play(fight, CardId::FIGHT_ME);
    assert(fight.enemies[0].hp == 32);
    assert(fight.player.get_status(StatusType::STRENGTH) == 3);
    assert(fight.enemies[0].get_status(StatusType::STRENGTH) == 1);

    // Break has the Ancient rarity: it comes from an Ancient, never from a combat reward.
    std::span<const CardId> ironclad_pool = RunManager::card_reward_pool(CharacterId::IRONCLAD);
    assert(std::find(ironclad_pool.begin(), ironclad_pool.end(), CardId::BREAK) == ironclad_pool.end());
    assert_in_pool(CharacterId::IRONCLAD, { CardId::CONFLAGRATION, CardId::EXPECT_A_FIGHT, CardId::PILLAGE,
                                            CardId::UPPERCUT, CardId::FIGHT_ME });
    std::cout << "[PASS] Ironclad: Break, Conflagration, Expect a Fight, Pillage, Uppercut, Fight Me!" << std::endl;
}

void test_silent_existing_mechanics() {
    // Leg Sweep: Weak on the target, Block for the hero.
    CombatEngine sweep = make_combat(CharacterId::SILENT);
    sweep.player.block = 0;
    play(sweep, CardId::LEG_SWEEP);
    assert(sweep.enemies[0].get_status(StatusType::WEAK) == 2 && sweep.player.block == 11);

    // Haze: Poison and Weak on every enemy.
    CombatEngine haze = make_pack(CharacterId::SILENT);
    play(haze, DeckCard(CardId::HAZE, true));
    for (const Enemy& foe : haze.enemies) {
        assert(foe.get_status(StatusType::POISON) == 6 && foe.get_status(StatusType::WEAK) == 2);
    }

    // Precise Cut: 13 damage, 2 less per other card in hand, never below 0.
    CombatEngine cut = make_combat(CharacterId::SILENT);
    cut.hand.clear();
    cut.hand.push_back(CardId::PRECISE_CUT);
    cut.hand.push_back(CardId::DEFEND);
    cut.hand.push_back(CardId::DEFEND);
    assert(cut.play_card(0));
    assert(cut.enemies[0].hp == 42 - 9 && cut.player.energy == 3);
    while (cut.hand.size() < 9) cut.hand.push_back(CardId::DEFEND);
    cut.hand.push_back(CardId::PRECISE_CUT);
    assert(cut.play_card(9));
    assert(cut.enemies[0].hp == 33);

    // Bouncing Flask: 3 Poison on a random enemy, 3 times.
    CombatEngine flask = make_pack(CharacterId::SILENT);
    play(flask, CardId::BOUNCING_FLASK);
    int flask_poison = 0;
    for (const Enemy& foe : flask.enemies) flask_poison += foe.get_status(StatusType::POISON);
    assert(flask_poison == 9);
    assert(CardDef::get_secondary(CardId::BOUNCING_FLASK, true) == 4);

    // Flechettes: one hit of 5 per Skill left in hand.
    CombatEngine darts = make_combat(CharacterId::SILENT);
    darts.hand.clear();
    darts.hand.push_back(CardId::FLECHETTES);
    darts.hand.push_back(CardId::DEFEND);
    darts.hand.push_back(CardId::SURVIVOR);
    darts.hand.push_back(CardId::STRIKE);
    assert(darts.play_card(0));
    assert(darts.enemies[0].hp == 42 - 10);

    // Footwork: Dexterity for the whole combat adds to every Block gain.
    CombatEngine footwork = make_combat(CharacterId::SILENT);
    play(footwork, CardId::FOOTWORK);
    assert(footwork.player.get_status(StatusType::DEXTERITY) == 2);
    footwork.player.block = 0;
    play(footwork, CardId::DEFEND);
    assert(footwork.player.block == 7);

    // Escape Plan: draws a card, and gives Block only when it is a Skill.
    CombatEngine escape = make_combat(CharacterId::SILENT);
    escape.player.block = 0;
    escape.draw_pile.clear();
    escape.draw_pile.push_back(CardId::STRIKE);
    play(escape, CardId::ESCAPE_PLAN);
    assert(escape.hand.size() == 1 && escape.player.block == 0);
    escape.draw_pile.clear();
    escape.draw_pile.push_back(CardId::DEFEND);
    play(escape, DeckCard(CardId::ESCAPE_PLAN, true));
    assert(escape.hand.size() == 1 && escape.player.block == 5);

    // Dash: Block and damage together.
    CombatEngine dash = make_combat(CharacterId::SILENT);
    dash.player.block = 0;
    play(dash, DeckCard(CardId::DASH, true));
    assert(dash.player.block == 13 && dash.enemies[0].hp == 29);

    // Bubble Bubble only works on an enemy that is already Poisoned.
    CombatEngine bubble = make_combat(CharacterId::SILENT);
    play(bubble, CardId::BUBBLE_BUBBLE);
    assert(bubble.enemies[0].get_status(StatusType::POISON) == 0);
    bubble.enemies[0].add_status(StatusType::POISON, 1);
    play(bubble, CardId::BUBBLE_BUBBLE);
    assert(bubble.enemies[0].get_status(StatusType::POISON) == 10);

    // Outbreak: 9 Poison on every enemy, which ticks at once (9 damage, 8 Poison left).
    CombatEngine outbreak = make_pack(CharacterId::SILENT);
    play(outbreak, CardId::OUTBREAK);
    assert(outbreak.enemies[0].hp == 9 && outbreak.enemies[2].hp == 33);
    for (const Enemy& foe : outbreak.enemies) assert(foe.get_status(StatusType::POISON) == 8);

    assert_in_pool(CharacterId::SILENT, { CardId::LEG_SWEEP, CardId::HAZE, CardId::PRECISE_CUT, CardId::BOUNCING_FLASK,
                                          CardId::FLECHETTES, CardId::FOOTWORK, CardId::ESCAPE_PLAN, CardId::DASH,
                                          CardId::BUBBLE_BUBBLE, CardId::OUTBREAK });
    std::cout << "[PASS] Silent: Leg Sweep, Haze, Precise Cut, Bouncing Flask, Flechettes, Footwork, Escape Plan, "
                 "Dash, Bubble Bubble, Outbreak" << std::endl;
}

void test_defect_existing_mechanics() {
    // Shadow Shield: Block and a Dark orb.
    CombatEngine shield = make_combat(CharacterId::DEFECT);
    shield.player.block = 0;
    play(shield, CardId::SHADOW_SHIELD);
    assert(shield.player.block == 11);
    assert(shield.player.orbs.size() == 1 && shield.player.orbs[0] == OrbType::DARK);

    // Meteor Strike costs 5: unplayable with 3 energy, then 24 damage and 3 Plasma orbs.
    CombatEngine meteor = make_combat(CharacterId::DEFECT);
    meteor.hand.clear();
    meteor.hand.push_back(CardId::METEOR_STRIKE);
    assert(!meteor.play_card(0));
    meteor.player.energy = 5;
    assert(meteor.play_card(0));
    assert(meteor.enemies[0].hp == 18 && meteor.player.energy == 0);
    assert(meteor.player.orbs.size() == 3);
    for (OrbType orb : meteor.player.orbs) assert(orb == OrbType::PLASMA);

    // Glacier: Block and 2 Frost orbs. Ice Lance: 19 damage and 3 Frost orbs.
    CombatEngine glacier = make_combat(CharacterId::DEFECT);
    glacier.player.block = 0;
    play(glacier, DeckCard(CardId::GLACIER, true));
    assert(glacier.player.block == 9 && glacier.player.orbs.size() == 2 && glacier.player.orbs[1] == OrbType::FROST);
    CombatEngine lance = make_combat(CharacterId::DEFECT);
    play(lance, CardId::ICE_LANCE);
    assert(lance.enemies[0].hp == 23 && lance.player.orbs.size() == 3 && lance.player.energy == 0);

    // Skim: 3 cards, 4 once upgraded.
    CombatEngine skim = make_combat(CharacterId::DEFECT);
    play(skim, CardId::SKIM);
    assert(skim.hand.size() == 3);
    CombatEngine skim_plus = make_combat(CharacterId::DEFECT);
    play(skim_plus, DeckCard(CardId::SKIM, true));
    assert(skim_plus.hand.size() == 4);

    // All for One: every card costing 0 comes back from the discard pile.
    CombatEngine all = make_combat(CharacterId::DEFECT);
    all.discard_pile.clear();
    all.discard_pile.push_back(CardId::BEAM_CELL);
    all.discard_pile.push_back(CardId::STRIKE);
    all.discard_pile.push_back(CardId::GO_FOR_THE_EYES);
    play(all, CardId::ALL_FOR_ONE);
    assert(all.enemies[0].hp == 32);
    assert(all.hand.size() == 2 && all.hand[0].id == CardId::BEAM_CELL && all.hand[1].id == CardId::GO_FOR_THE_EYES);
    assert(all.discard_pile.size() == 2); // the Strike and All for One itself

    // Null: damage, Weak and a Dark orb.
    CombatEngine null = make_combat(CharacterId::DEFECT);
    play(null, DeckCard(CardId::NULL_CARD, true));
    assert(null.enemies[0].hp == 29 && null.enemies[0].get_status(StatusType::WEAK) == 2);
    assert(null.player.orbs.size() == 1 && null.player.orbs[0] == OrbType::DARK);

    // Go for the Eyes: Weak only when the enemy is about to attack.
    CombatEngine eyes = make_combat(CharacterId::DEFECT);
    play(eyes, CardId::GO_FOR_THE_EYES);
    assert(eyes.enemies[0].hp == 39 && eyes.enemies[0].get_status(StatusType::WEAK) == 1);
    make_enemy_idle(eyes.enemies[0]);
    play(eyes, CardId::GO_FOR_THE_EYES);
    assert(eyes.enemies[0].get_status(StatusType::WEAK) == 1);

    // Scrape: draws 4 cards and keeps only those costing 0.
    CombatEngine scrape = make_combat(CharacterId::DEFECT);
    scrape.draw_pile.clear();
    scrape.discard_pile.clear();
    scrape.draw_pile.push_back(CardId::STRIKE);
    scrape.draw_pile.push_back(CardId::BEAM_CELL);
    scrape.draw_pile.push_back(CardId::DEFEND);
    scrape.draw_pile.push_back(CardId::GO_FOR_THE_EYES);
    play(scrape, CardId::SCRAPE);
    assert(scrape.enemies[0].hp == 35);
    assert(scrape.hand.size() == 2 && scrape.draw_pile.empty());
    assert(scrape.discard_pile.size() == 3); // Strike, Defend and Scrape itself

    assert_in_pool(CharacterId::DEFECT, { CardId::SHADOW_SHIELD, CardId::METEOR_STRIKE, CardId::GLACIER,
                                          CardId::ICE_LANCE, CardId::SKIM, CardId::ALL_FOR_ONE, CardId::NULL_CARD,
                                          CardId::GO_FOR_THE_EYES, CardId::SCRAPE });
    std::cout << "[PASS] Defect: Shadow Shield, Meteor Strike, Glacier, Ice Lance, Skim, All for One, Null, "
                 "Go for the Eyes, Scrape" << std::endl;
}

void test_necrobinder_existing_mechanics() {
    // Bury costs 4: 52 damage once the energy is there.
    CombatEngine bury = make_combat(CharacterId::NECROBINDER, EnemyType::CEREMONIAL_BEAST);
    bury.hand.clear();
    bury.hand.push_back(CardId::BURY);
    assert(!bury.play_card(0));
    bury.player.energy = 4;
    assert(bury.play_card(0));
    assert(bury.enemies[0].hp == 252 - 52);

    // Misery: damage, then the target's debuffs are copied to every other enemy.
    CombatEngine misery = make_pack(CharacterId::NECROBINDER);
    misery.enemies[2].add_status(StatusType::VULNERABLE, 2);
    misery.enemies[2].add_status(StatusType::POISON, 4);
    play(misery, CardId::MISERY, 2);
    assert(misery.enemies[2].hp == 42 - 10); // 7 x 1.5
    for (int other : { 0, 1 }) {
        assert(misery.enemies[other].get_status(StatusType::VULNERABLE) == 2);
        assert(misery.enemies[other].get_status(StatusType::POISON) == 4);
        assert(misery.enemies[other].get_status(StatusType::WEAK) == 0);
    }

    // Misery+ has Retain: it stays in hand when the turn ends, the plain one is discarded.
    CombatEngine retain = make_combat(CharacterId::NECROBINDER);
    make_enemy_idle(retain.enemies[0]);
    retain.hand.clear();
    retain.hand.push_back(DeckCard(CardId::MISERY, true));
    retain.hand.push_back(CardId::MISERY);
    retain.end_player_turn();
    assert(retain.state == CombatState::PLAYER_TURN);
    int kept = 0;
    for (const DeckCard& card : retain.hand) kept += card.id == CardId::MISERY;
    assert(kept == 1 && retain.hand[0] == DeckCard(CardId::MISERY, true));

    // Undeath: Block and a copy of itself in the discard pile.
    CombatEngine undeath = make_combat(CharacterId::NECROBINDER);
    undeath.player.block = 0;
    undeath.discard_pile.clear();
    play(undeath, DeckCard(CardId::UNDEATH, true));
    assert(undeath.player.block == 9 && undeath.player.energy == 3);
    assert(undeath.discard_pile.size() == 2 && undeath.discard_pile[0] == DeckCard(CardId::UNDEATH, true));

    assert_in_pool(CharacterId::NECROBINDER, { CardId::BURY, CardId::MISERY, CardId::UNDEATH });
    std::cout << "[PASS] Necrobinder: Bury, Misery (Retain once upgraded), Undeath" << std::endl;
}

void test_exhaust() {
    // An Exhaust card leaves the combat: it goes to the exhaust pile, never to the discard pile.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    combat.discard_pile.clear();
    play(combat, CardId::IMPERVIOUS);
    assert(combat.player.block == 30);
    assert(combat.exhaust_pile.size() == 1 && combat.exhaust_pile[0].id == CardId::IMPERVIOUS);
    assert(combat.discard_pile.empty() && combat.exhausted_this_turn == 1);
    assert(CardDef::has_keyword(CardId::IMPERVIOUS, false, KEYWORD_EXHAUST));
    assert(!CardDef::has_keyword(CardId::STRIKE, false, KEYWORD_EXHAUST));

    // The turn counter restarts each turn, the pile lasts the whole combat and is emptied by the next one.
    make_enemy_idle(combat.enemies[0]);
    combat.end_player_turn();
    assert(combat.exhausted_this_turn == 0 && combat.exhaust_pile.size() == 1);
    assert(count_in_piles(combat, CardId::IMPERVIOUS) == 1);
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    assert(combat.exhaust_pile.empty());

    // An upgrade can remove the keyword (Mirage) or add Retain (Wisp, Calculated Gamble).
    assert(CardDef::has_keyword(CardId::MIRAGE, false, KEYWORD_EXHAUST));
    assert(!CardDef::has_keyword(CardId::MIRAGE, true, KEYWORD_EXHAUST));
    assert(!CardDef::has_keyword(CardId::WISP, false, KEYWORD_RETAIN));
    assert(CardDef::has_keyword(CardId::WISP, true, KEYWORD_RETAIN));
    assert(CardDef::has_keyword(CardId::CALCULATED_GAMBLE, true, KEYWORD_RETAIN));
    assert(CardDef::has_keyword(CardId::MISERY, true, KEYWORD_RETAIN));
    std::cout << "[PASS] Exhaust: pile, per-turn counter, keywords changed by upgrades" << std::endl;
}

void test_ironclad_exhaust() {
    // Tremble: Vulnerable, then exhausted.
    CombatEngine tremble = make_combat(CharacterId::IRONCLAD);
    play(tremble, DeckCard(CardId::TREMBLE, true));
    assert(tremble.enemies[0].get_status(StatusType::VULNERABLE) == 4);
    assert(tremble.exhaust_pile.size() == 1);

    // Ashen Strike: 6 damage plus 3 per card in the exhaust pile.
    CombatEngine ashen = make_combat(CharacterId::IRONCLAD);
    play(ashen, CardId::ASHEN_STRIKE);
    assert(ashen.enemies[0].hp == 36);
    ashen.exhaust_card(CardId::DEFEND);
    ashen.exhaust_card(CardId::DEFEND);
    play(ashen, CardId::ASHEN_STRIKE);
    assert(ashen.enemies[0].hp == 36 - 12);

    // Evil Eye: its Block twice when a card was exhausted this turn.
    CombatEngine eye = make_combat(CharacterId::IRONCLAD);
    eye.player.block = 0;
    play(eye, CardId::EVIL_EYE);
    assert(eye.player.block == 8);
    eye.exhaust_card(CardId::DEFEND);
    play(eye, DeckCard(CardId::EVIL_EYE, true));
    assert(eye.player.block == 8 + 22);

    // Molten Fist: damage first, then the enemy's Vulnerable is doubled.
    CombatEngine fist = make_combat(CharacterId::IRONCLAD);
    fist.enemies[0].add_status(StatusType::VULNERABLE, 2);
    play(fist, CardId::MOLTEN_FIST);
    assert(fist.enemies[0].hp == 42 - 15 && fist.enemies[0].get_status(StatusType::VULNERABLE) == 4);
    assert(fist.exhaust_pile.size() == 1);

    // Cinder and True Grit exhaust a random card of the hand.
    CombatEngine cinder = make_combat(CharacterId::IRONCLAD);
    cinder.hand.clear();
    cinder.hand.push_back(CardId::CINDER);
    cinder.hand.push_back(CardId::DEFEND);
    cinder.discard_pile.clear();
    assert(cinder.play_card(0));
    assert(cinder.enemies[0].hp == 24 && cinder.hand.empty());
    assert(cinder.exhaust_pile.size() == 1 && cinder.exhaust_pile[0].id == CardId::DEFEND);
    assert(cinder.discard_pile.size() == 1 && cinder.discard_pile[0].id == CardId::CINDER);
    CombatEngine grit = make_combat(CharacterId::IRONCLAD);
    grit.player.block = 0;
    play(grit, CardId::TRUE_GRIT); // alone in hand: nothing to exhaust
    assert(grit.player.block == 7 && grit.exhaust_pile.empty());
    grit.hand.clear();
    grit.hand.push_back(DeckCard(CardId::TRUE_GRIT, true));
    grit.hand.push_back(CardId::STRIKE);
    assert(grit.play_card(0));
    assert(grit.player.block == 16 && grit.exhaust_pile.size() == 1 && grit.hand.empty());

    // Second Wind: every non-Attack of the hand is exhausted for Block.
    CombatEngine wind = make_combat(CharacterId::IRONCLAD);
    wind.player.block = 0;
    wind.hand.clear();
    wind.hand.push_back(CardId::SECOND_WIND);
    wind.hand.push_back(CardId::DEFEND);
    wind.hand.push_back(CardId::STRIKE);
    wind.hand.push_back(CardId::SHRUG_IT_OFF);
    assert(wind.play_card(0));
    assert(wind.player.block == 10 && wind.exhaust_pile.size() == 2);
    assert(wind.hand.size() == 1 && wind.hand[0].id == CardId::STRIKE);

    assert_in_pool(CharacterId::IRONCLAD, { CardId::ASHEN_STRIKE, CardId::TREMBLE, CardId::EVIL_EYE, CardId::MOLTEN_FIST,
                                            CardId::CINDER, CardId::SECOND_WIND, CardId::TRUE_GRIT });
    std::cout << "[PASS] Ironclad Exhaust: Ashen Strike, Tremble, Evil Eye, Molten Fist, Cinder, Second Wind, True Grit"
              << std::endl;

    // Dominate: Vulnerable, then 1 Strength per Vulnerable on the enemy.
    CombatEngine dominate = make_combat(CharacterId::IRONCLAD);
    dominate.enemies[0].add_status(StatusType::VULNERABLE, 2);
    play(dominate, CardId::DOMINATE);
    assert(dominate.enemies[0].get_status(StatusType::VULNERABLE) == 3);
    assert(dominate.player.get_status(StatusType::STRENGTH) == 3 && dominate.exhaust_pile.size() == 1);

    // Fiend Fire: exhausts the hand, one hit of 7 per card, then itself.
    CombatEngine fiend = make_combat(CharacterId::IRONCLAD);
    fiend.hand.clear();
    fiend.hand.push_back(CardId::FIEND_FIRE);
    fiend.hand.push_back(CardId::STRIKE);
    fiend.hand.push_back(CardId::DEFEND);
    fiend.hand.push_back(CardId::DEFEND);
    assert(fiend.play_card(0));
    assert(fiend.enemies[0].hp == 42 - 21 && fiend.hand.empty() && fiend.exhaust_pile.size() == 4);

    // Thrash: two hits, then a random Attack of the hand is exhausted and its damage kept by this copy.
    CombatEngine thrash = make_combat(CharacterId::IRONCLAD);
    thrash.hand.clear();
    thrash.hand.push_back(CardId::THRASH);
    thrash.hand.push_back(CardId::DEFEND);
    thrash.hand.push_back(CardId::BLUDGEON);
    thrash.discard_pile.clear();
    assert(thrash.play_card(0));
    assert(thrash.enemies[0].hp == 42 - 8);
    assert(thrash.exhaust_pile.size() == 1 && thrash.exhaust_pile[0].id == CardId::BLUDGEON);
    assert(thrash.hand.size() == 1 && thrash.discard_pile[0].id == CardId::THRASH && thrash.discard_pile[0].bonus == 32);
    thrash.enemies[0].hp = 100;
    thrash.hand.clear();
    thrash.hand.push_back(thrash.discard_pile[0]);
    thrash.player.energy = 3;
    assert(thrash.play_card(0));
    assert(thrash.enemies[0].hp == 100 - 2 * 36);

    // Pact's End needs 3 exhausted cards.
    CombatEngine pact = make_pack(CharacterId::IRONCLAD);
    play(pact, CardId::PACTS_END);
    assert(pact.enemies[2].hp == 42);
    for (int n = 0; n < 3; ++n) pact.exhaust_card(CardId::DEFEND);
    play(pact, CardId::PACTS_END);
    assert(pact.enemies[0].hp == 0 && pact.enemies[1].hp == 0 && pact.enemies[2].hp == 24);
    assert(pact.exhaust_pile.size() == 3); // the card itself is discarded

    // Not Yet heals, never above the maximum.
    CombatEngine heal = make_combat(CharacterId::IRONCLAD);
    heal.player.hp = 60;
    play(heal, CardId::NOT_YET);
    assert(heal.player.hp == 70);
    play(heal, DeckCard(CardId::NOT_YET, true));
    assert(heal.player.hp == 80);

    assert_in_pool(CharacterId::IRONCLAD, { CardId::DOMINATE, CardId::FIEND_FIRE, CardId::THRASH, CardId::IMPERVIOUS,
                                            CardId::PACTS_END, CardId::NOT_YET });
    std::cout << "[PASS] Ironclad Exhaust: Dominate, Fiend Fire, Thrash, Impervious, Pact's End, Not Yet" << std::endl;
}

void test_other_heroes_exhaust() {
    // Adrenaline: free, energy and 2 cards.
    CombatEngine adrenaline = make_combat(CharacterId::SILENT);
    play(adrenaline, DeckCard(CardId::ADRENALINE, true));
    assert(adrenaline.player.energy == 5 && adrenaline.hand.size() == 2 && adrenaline.exhaust_pile.size() == 1);

    // Calculated Gamble: the whole hand is swapped for as many new cards.
    CombatEngine gamble = make_combat(CharacterId::SILENT);
    gamble.hand.clear();
    gamble.hand.push_back(CardId::CALCULATED_GAMBLE);
    gamble.hand.push_back(CardId::STRIKE);
    gamble.hand.push_back(CardId::STRIKE);
    gamble.discard_pile.clear();
    assert(gamble.play_card(0));
    assert(gamble.hand.size() == 2 && gamble.discard_pile.size() == 2 && gamble.exhaust_pile.size() == 1);

    // Mirage: Block equal to the Poison on every enemy; the upgraded card is not exhausted.
    CombatEngine mirage = make_pack(CharacterId::SILENT);
    mirage.player.block = 0;
    mirage.enemies[0].add_status(StatusType::POISON, 4);
    mirage.enemies[2].add_status(StatusType::POISON, 7);
    play(mirage, DeckCard(CardId::MIRAGE, true));
    assert(mirage.player.block == 11 && mirage.exhaust_pile.empty());
    play(mirage, CardId::MIRAGE);
    assert(mirage.exhaust_pile.size() == 1);

    assert_in_pool(CharacterId::SILENT, { CardId::ADRENALINE, CardId::CALCULATED_GAMBLE, CardId::MIRAGE });
    std::cout << "[PASS] Silent Exhaust: Adrenaline, Calculated Gamble, Mirage" << std::endl;

    // Rainbow: one Lightning, one Frost, one Dark.
    CombatEngine rainbow = make_combat(CharacterId::DEFECT);
    play(rainbow, CardId::RAINBOW);
    assert(rainbow.player.orbs.size() == 3 && rainbow.player.orbs[0] == OrbType::LIGHTNING);
    assert(rainbow.player.orbs[1] == OrbType::FROST && rainbow.player.orbs[2] == OrbType::DARK);
    assert(rainbow.exhaust_pile.size() == 1);

    // Chill: one Frost per living enemy.
    CombatEngine chill = make_pack(CharacterId::DEFECT);
    chill.enemies[1].hp = 0;
    play(chill, DeckCard(CardId::CHILL, true));
    assert(chill.player.orbs.size() == 2 && chill.player.orbs[0] == OrbType::FROST && chill.exhaust_pile.empty());

    // Fusion: a Plasma orb, worth 1 energy at the start of each turn.
    CombatEngine fusion = make_combat(CharacterId::DEFECT);
    make_enemy_idle(fusion.enemies[0]);
    play(fusion, CardId::FUSION);
    assert(fusion.player.orbs.size() == 1 && fusion.player.orbs[0] == OrbType::PLASMA);
    fusion.end_player_turn();
    assert(fusion.player.energy == 4);

    // Reboot: hand and discard pile go back into the draw pile, then 4 cards are drawn.
    CombatEngine reboot = make_combat(CharacterId::DEFECT);
    reboot.hand.clear();
    reboot.hand.push_back(CardId::REBOOT);
    reboot.hand.push_back(CardId::STRIKE);
    reboot.discard_pile.clear();
    reboot.discard_pile.push_back(CardId::DEFEND);
    size_t reboot_pile = reboot.draw_pile.size();
    assert(reboot.play_card(0));
    assert(reboot.hand.size() == 4 && reboot.discard_pile.empty());
    assert(reboot.draw_pile.size() == reboot_pile + 2 - 4);

    // Supercritical and Double Energy; the upgraded Double Energy costs 0.
    CombatEngine energy = make_combat(CharacterId::DEFECT);
    play(energy, CardId::SUPERCRITICAL);
    assert(energy.player.energy == 7);
    play(energy, CardId::DOUBLE_ENERGY); // play() refills to 3, the card costs 1
    assert(energy.player.energy == 4);
    assert(CardDef::get_cost(CardId::DOUBLE_ENERGY, true) == 0);
    assert(energy.exhaust_pile.size() == 2);

    assert_in_pool(CharacterId::DEFECT, { CardId::RAINBOW, CardId::CHILL, CardId::FUSION, CardId::REBOOT,
                                          CardId::SUPERCRITICAL, CardId::DOUBLE_ENERGY });
    std::cout << "[PASS] Defect Exhaust: Rainbow, Chill, Fusion, Reboot, Supercritical, Double Energy" << std::endl;

    // Shared Fate: both sides lose Strength.
    CombatEngine fate = make_combat(CharacterId::NECROBINDER);
    play(fate, DeckCard(CardId::SHARED_FATE, true));
    assert(fate.player.get_status(StatusType::STRENGTH) == -2 && fate.enemies[0].get_status(StatusType::STRENGTH) == -3);

    // Wisp: 1 energy; the upgraded card stays in hand at the end of the turn.
    CombatEngine wisp = make_combat(CharacterId::NECROBINDER);
    make_enemy_idle(wisp.enemies[0]);
    play(wisp, CardId::WISP);
    assert(wisp.player.energy == 4 && wisp.exhaust_pile.size() == 1);
    wisp.hand.clear();
    wisp.hand.push_back(DeckCard(CardId::WISP, true));
    wisp.end_player_turn();
    int wisps = 0;
    for (const DeckCard& card : wisp.hand) wisps += card.id == CardId::WISP;
    assert(wisps == 1);

    // Putrefy: Weak and Vulnerable.
    CombatEngine putrefy = make_combat(CharacterId::NECROBINDER);
    play(putrefy, CardId::PUTREFY);
    assert(putrefy.enemies[0].get_status(StatusType::WEAK) == 2 && putrefy.enemies[0].get_status(StatusType::VULNERABLE) == 2);

    // Afterlife and Reanimate summon Titos.
    CombatEngine summon = make_combat(CharacterId::NECROBINDER);
    play(summon, CardId::AFTERLIFE);
    assert(summon.player.minion_hp == 6);
    play(summon, DeckCard(CardId::REANIMATE, true));
    assert(summon.player.minion_hp == 31 && summon.player.energy == 0 && summon.exhaust_pile.size() == 2);

    assert_in_pool(CharacterId::NECROBINDER, { CardId::SHARED_FATE, CardId::WISP, CardId::PUTREFY, CardId::REANIMATE,
                                               CardId::AFTERLIFE });
    std::cout << "[PASS] Necrobinder Exhaust: Shared Fate, Wisp, Putrefy, Reanimate, Afterlife" << std::endl;
}

// Ends the turn with every enemy doing nothing, so only the player's own effects show.
void end_turn_idle(CombatEngine& combat) {
    for (Enemy& foe : combat.enemies) make_enemy_idle(foe);
    combat.end_player_turn();
}

void pad_draw_pile(CombatEngine& combat, int count) {
    for (int i = 0; i < count; ++i) combat.draw_pile.push_back(CardId::DEFEND);
}

int count_in_hand(const CombatEngine& combat, CardId id) {
    int count = 0;
    for (const DeckCard& card : combat.hand) count += card.id == id;
    return count;
}

void test_card_keywords() {
    // Ethereal: exhausted when still in hand at the end of the turn, discarded as usual when played.
    CombatEngine ethereal = make_combat(CharacterId::NECROBINDER);
    ethereal.hand.clear();
    ethereal.hand.push_back(CardId::DEFILE);
    ethereal.hand.push_back(CardId::STRIKE);
    end_turn_idle(ethereal);
    assert(ethereal.exhaust_pile.size() == 1 && ethereal.exhaust_pile[0] == CardId::DEFILE);
    assert(ethereal.counters.combat_ethereal_played == 0);
    play(ethereal, CardId::DEFILE);
    assert(ethereal.enemies[0].hp == 42 - 13 && ethereal.exhaust_pile.size() == 1);
    assert(ethereal.counters.combat_ethereal_played == 1);

    // Ethereal given to one copy during the combat.
    DeckCard haunted(CardId::STRIKE);
    haunted.added_keywords = KEYWORD_ETHEREAL;
    ethereal.hand.clear();
    ethereal.hand.push_back(haunted);
    ethereal.hand.push_back(CardId::STRIKE);
    end_turn_idle(ethereal);
    assert(ethereal.exhaust_pile.size() == 2 && ethereal.exhaust_pile[1].added_keywords == KEYWORD_ETHEREAL);

    // Defy, Fear, Parse and Enfeebling Touch are Ethereal too.
    for (CardId id : { CardId::DEFY, CardId::FEAR, CardId::PARSE, CardId::ENFEEBLING_TOUCH }) {
        assert(CardDef::has_keyword(id, false, KEYWORD_ETHEREAL) && CardDef::has_keyword(id, true, KEYWORD_ETHEREAL));
    }
    CombatEngine defy = make_combat(CharacterId::NECROBINDER);
    play(defy, CardId::DEFY);
    assert(defy.player.block == 6 && defy.enemies[0].get_status(StatusType::WEAK) == 1);
    play(defy, DeckCard(CardId::FEAR, true));
    assert(defy.enemies[0].hp == 42 - 8 && defy.enemies[0].get_status(StatusType::VULNERABLE) == 2);
    play(defy, CardId::PARSE);
    assert(defy.hand.size() == 3);
    std::cout << "[PASS] Ethereal: end of turn exhaust, per-copy keyword, Defile, Defy, Fear, Parse" << std::endl;

    // Innate: drawn first, wherever the shuffle put them.
    CombatEngine innate = make_combat(CharacterId::SILENT);
    innate.hand.clear();
    innate.discard_pile.clear();
    innate.draw_pile.clear();
    innate.draw_pile.push_back(CardId::BACKSTAB);
    for (int i = 0; i < 8; ++i) innate.draw_pile.push_back(CardId::STRIKE);
    innate.draw_pile.push_back(CardId::ASSASSINATE);
    for (int i = 0; i < 3; ++i) innate.draw_pile.push_back(CardId::DEFEND);
    innate.open_first_turn();
    assert(innate.hand.size() == 5 && innate.draw_pile.size() == 8);
    assert(count_in_hand(innate, CardId::BACKSTAB) == 1 && count_in_hand(innate, CardId::ASSASSINATE) == 1);
    assert(innate.counters.combat_cards_drawn == 5 && innate.counters.cards_drawn == 0);

    // More Innate cards than the opening draw: all of them are drawn.
    CombatEngine boot = make_combat(CharacterId::DEFECT);
    boot.hand.clear();
    boot.discard_pile.clear();
    boot.draw_pile.clear();
    for (int i = 0; i < 7; ++i) boot.draw_pile.push_back(CardId::BOOT_SEQUENCE);
    for (int i = 0; i < 5; ++i) boot.draw_pile.push_back(CardId::STRIKE);
    boot.open_first_turn();
    assert(boot.hand.size() == 7 && count_in_hand(boot, CardId::BOOT_SEQUENCE) == 7);

    play(boot, CardId::BOOT_SEQUENCE);
    assert(boot.player.block == 10 && boot.player.energy == 3 && boot.exhaust_pile.size() == 1);
    play(innate, DeckCard(CardId::ASSASSINATE, true));
    assert(innate.enemies[0].hp == 42 - 13 && innate.enemies[0].get_status(StatusType::VULNERABLE) == 2);
    assert(innate.exhaust_pile.size() == 1);
    innate.enemies[0].statuses.clear();
    play(innate, CardId::BACKSTAB);
    assert(innate.enemies[0].hp == 29 - 11 && innate.exhaust_pile.size() == 2);
    play(innate, CardId::SUPPRESS);
    assert(innate.enemies[0].hp == 18 - 11 && innate.enemies[0].get_status(StatusType::WEAK) == 3);
    assert(CardDef::has_keyword(CardId::SUPPRESS, false, KEYWORD_INNATE) && innate.exhaust_pile.size() == 2);
    std::cout << "[PASS] Innate: opening hand, Assassinate, Backstab, Suppress, Boot Sequence" << std::endl;

    // Retain from the card definition (Snakebite, Reap, Sow).
    CombatEngine retain = make_combat(CharacterId::SILENT);
    retain.hand.clear();
    retain.hand.push_back(CardId::SNAKEBITE);
    retain.hand.push_back(CardId::STRIKE);
    end_turn_idle(retain);
    assert(retain.hand.size() == 6 && retain.hand[0] == CardId::SNAKEBITE);
    play(retain, DeckCard(CardId::SNAKEBITE, true));
    assert(retain.enemies[0].get_status(StatusType::POISON) == 10 && retain.player.energy == 1);

    // Retain given to one copy for the rest of the combat.
    DeckCard held(CardId::STRIKE);
    held.added_keywords = KEYWORD_RETAIN;
    retain.hand.clear();
    retain.hand.push_back(held);
    pad_draw_pile(retain, 10);
    end_turn_idle(retain);
    end_turn_idle(retain);
    assert(retain.hand[0].added_keywords == KEYWORD_RETAIN && retain.hand.size() == 6);

    // Expertise: the cards it draws stay for one end of turn only.
    CombatEngine expertise = make_combat(CharacterId::SILENT);
    expertise.hand.clear();
    expertise.hand.push_back(CardId::EXPERTISE);
    expertise.hand.push_back(CardId::STRIKE);
    assert(expertise.play_card(0));
    assert(expertise.hand.size() == 3 && expertise.hand[1].added_keywords == KEYWORD_RETAIN_ONCE);
    end_turn_idle(expertise);
    assert(expertise.hand.size() == 7);
    for (const DeckCard& card : expertise.hand) assert(card.added_keywords == 0);
    end_turn_idle(expertise);
    assert(expertise.hand.size() == 5);

    CombatEngine reap = make_pack(CharacterId::NECROBINDER);
    play(reap, CardId::REAP, 2);
    assert(reap.enemies[2].hp == 42 - 27 && reap.player.energy == 0);
    play(reap, DeckCard(CardId::SOW, true));
    assert(reap.enemies[0].hp == 18 - 11 && reap.enemies[1].hp == 18 - 11 && reap.enemies[2].hp == 15 - 11);
    assert(CardDef::has_keyword(CardId::REAP, false, KEYWORD_RETAIN) && CardDef::has_keyword(CardId::SOW, true, KEYWORD_RETAIN));
    std::cout << "[PASS] Retain: keyword, per-copy, one turn (Expertise), Snakebite, Reap, Sow" << std::endl;

    // Return: a played card comes back to the hand at the start of the next turn.
    CombatEngine bolas = make_combat(CharacterId::IRONCLAD);
    DeckCard thrown(CardId::STRIKE);
    thrown.added_keywords = KEYWORD_RETURN;
    play(bolas, thrown);
    assert(bolas.returning_cards.size() == 1 && bolas.discard_pile.empty());
    end_turn_idle(bolas);
    assert(bolas.returning_cards.empty() && bolas.hand.size() == 6 && bolas.hand[0].added_keywords == KEYWORD_RETURN);
    std::cout << "[PASS] Return: the card comes back next turn" << std::endl;
}

void test_x_cost() {
    // X = all the energy left; the effect repeats X times.
    CombatEngine skewer = make_combat(CharacterId::SILENT);
    play(skewer, CardId::SKEWER);
    assert(skewer.enemies[0].hp == 42 - 3 * 8 && skewer.player.energy == 0);
    assert(skewer.counters.energy_spent == 3 && skewer.counters.hits_dealt == 3);
    assert(CardDef::is_x_cost(CardId::SKEWER, false) && !CardDef::is_x_cost(CardId::STRIKE, false));

    // Playable with no energy: X = 0.
    skewer.hand.clear();
    skewer.hand.push_back(DeckCard(CardId::SKEWER, true));
    skewer.player.energy = 0;
    assert(skewer.can_play_card(0) && skewer.play_card(0));
    assert(skewer.enemies[0].hp == 18 && skewer.player.energy == 0);

    CombatEngine whirlwind = make_pack(CharacterId::IRONCLAD);
    whirlwind.hand.clear();
    whirlwind.hand.push_back(CardId::WHIRLWIND);
    whirlwind.player.energy = 2;
    assert(whirlwind.play_card(0));
    assert(whirlwind.enemies[0].hp == 8 && whirlwind.enemies[1].hp == 8 && whirlwind.enemies[2].hp == 32);

    // Malaise+: X + 1.
    CombatEngine malaise = make_combat(CharacterId::SILENT);
    play(malaise, DeckCard(CardId::MALAISE, true));
    assert(malaise.enemies[0].get_status(StatusType::STRENGTH) == -4 && malaise.enemies[0].get_status(StatusType::WEAK) == 4);
    assert(malaise.exhaust_pile.size() == 1);

    // Tempest: X Lightning orbs, X + 1 once upgraded (the fourth pushes the first one out).
    CombatEngine tempest = make_combat(CharacterId::DEFECT);
    tempest.hand.clear();
    tempest.hand.push_back(CardId::TEMPEST);
    tempest.player.energy = 2;
    assert(tempest.play_card(0));
    assert(tempest.player.orbs.size() == 2 && tempest.player.orbs[1] == OrbType::LIGHTNING && tempest.enemies[0].hp == 42);
    tempest.player.orbs.clear();
    play(tempest, DeckCard(CardId::TEMPEST, true));
    assert(tempest.player.orbs.size() == 3 && tempest.enemies[0].hp < 42);

    // Heavenly Drill: X is doubled from 4.
    CombatEngine drill = make_combat(CharacterId::REGENT);
    drill.enemies[0].hp = 200;
    play(drill, CardId::HEAVENLY_DRILL);
    assert(drill.enemies[0].hp == 200 - 3 * 8);
    drill.hand.clear();
    drill.hand.push_back(CardId::HEAVENLY_DRILL);
    drill.player.energy = 4;
    assert(drill.play_card(0));
    assert(drill.enemies[0].hp == 176 - 8 * 8);

    CombatEngine eradicate = make_combat(CharacterId::NECROBINDER);
    play(eradicate, DeckCard(CardId::ERADICATE, true));
    assert(eradicate.enemies[0].hp == 42 - 3 * 14);
    assert(CardDef::has_keyword(CardId::ERADICATE, false, KEYWORD_RETAIN));
    std::cout << "[PASS] X cost: Skewer, Whirlwind, Malaise, Tempest, Heavenly Drill, Eradicate" << std::endl;
}

void test_delayed_effects() {
    // Energy next turn.
    CombatEngine sidestep = make_combat(CharacterId::SILENT);
    play(sidestep, DeckCard(CardId::SIDESTEP, true));
    assert(sidestep.player.energy == 3 && sidestep.delayed_effects.size() == 1);
    end_turn_idle(sidestep);
    assert(sidestep.player.energy == 5 && sidestep.delayed_effects.empty());
    end_turn_idle(sidestep);
    assert(sidestep.player.energy == 3);

    // Block next turn, with the Dexterity of the moment the card was played.
    CombatEngine dodge = make_combat(CharacterId::SILENT);
    play(dodge, CardId::DODGE_AND_ROLL);
    assert(dodge.player.block == 4);
    end_turn_idle(dodge);
    assert(dodge.player.block == 4);
    dodge.player.add_status(StatusType::DEXTERITY, 2);
    play(dodge, DeckCard(CardId::DODGE_AND_ROLL, true));
    assert(dodge.player.block == 4 + 8);
    end_turn_idle(dodge);
    assert(dodge.player.block == 8);

    // Blur: Block survives one start of turn.
    CombatEngine blur = make_combat(CharacterId::SILENT);
    play(blur, CardId::BLUR);
    end_turn_idle(blur);
    assert(blur.player.block == 5 && blur.keep_block_turns == 0);
    end_turn_idle(blur);
    assert(blur.player.block == 0);

    // Predator: two more cards in the next opening draw.
    CombatEngine predator = make_combat(CharacterId::SILENT);
    pad_draw_pile(predator, 4);
    play(predator, CardId::PREDATOR);
    assert(predator.enemies[0].hp == 42 - 15);
    end_turn_idle(predator);
    assert(predator.hand.size() == 7 && predator.counters.cards_drawn == 0);
    assert_in_pool(CharacterId::SILENT, { CardId::ANTICIPATE, CardId::ASSASSINATE, CardId::BACKSTAB, CardId::BLUR,
                                          CardId::DODGE_AND_ROLL, CardId::FINISHER, CardId::MALAISE, CardId::MEMENTO_MORI,
                                          CardId::MURDER, CardId::PIERCING_WAIL, CardId::PREDATOR, CardId::SIDESTEP,
                                          CardId::SKEWER, CardId::EXPERTISE, CardId::SNAKEBITE });
    std::span<const CardId> silent_pool = RunManager::card_reward_pool(CharacterId::SILENT);
    assert(std::find(silent_pool.begin(), silent_pool.end(), CardId::SUPPRESS) == silent_pool.end()); // Ancient rarity
    std::cout << "[PASS] Next turn: Sidestep, Dodge and Roll, Blur, Predator" << std::endl;

    CombatEngine battery = make_combat(CharacterId::DEFECT);
    play(battery, DeckCard(CardId::CHARGE_BATTERY, true));
    assert(battery.player.block == 10);
    end_turn_idle(battery);
    assert(battery.player.energy == 4 && battery.player.block == 0);

    // Lightning Rod: one orb at the start of each of the next two turns.
    CombatEngine rod = make_combat(CharacterId::DEFECT);
    play(rod, CardId::LIGHTNING_ROD);
    assert(rod.player.block == 4 && rod.player.orbs.empty());
    end_turn_idle(rod);
    assert(rod.player.orbs.size() == 1 && rod.player.orbs[0] == OrbType::LIGHTNING);
    end_turn_idle(rod);
    assert(rod.player.orbs.size() == 2 && rod.delayed_effects.empty());
    end_turn_idle(rod);
    assert(rod.player.orbs.size() == 2);
    assert_in_pool(CharacterId::DEFECT, { CardId::BOOT_SEQUENCE, CardId::CHARGE_BATTERY, CardId::FTL, CardId::HELIX_DRILL,
                                          CardId::LIGHTNING_ROD, CardId::TEMPEST });

    CombatEngine glitter = make_combat(CharacterId::REGENT);
    play(glitter, CardId::GLITTERSTREAM);
    assert(glitter.player.block == 11);
    end_turn_idle(glitter);
    assert(glitter.player.block == 5);
    play(glitter, DeckCard(CardId::HEGEMONY, true));
    assert(glitter.enemies[0].hp == 42 - 18);
    end_turn_idle(glitter);
    assert(glitter.player.energy == 6);

    // Convergence+: energy and Stars next turn, and the whole hand stays.
    CombatEngine convergence = make_combat(CharacterId::REGENT);
    convergence.hand.clear();
    convergence.hand.push_back(DeckCard(CardId::CONVERGENCE, true));
    convergence.hand.push_back(CardId::STRIKE);
    convergence.hand.push_back(CardId::DEFEND);
    assert(convergence.play_card(0));
    assert(convergence.retain_hand && convergence.player.sovereign_stars == 0);
    end_turn_idle(convergence);
    assert(convergence.hand.size() == 7 && convergence.player.energy == 4 && convergence.player.sovereign_stars == 2);
    assert(!convergence.retain_hand);
    end_turn_idle(convergence);
    assert(convergence.hand.size() == 5);

    CombatEngine delay = make_combat(CharacterId::NECROBINDER);
    play(delay, DeckCard(CardId::DELAY, true));
    assert(delay.player.block == 13);
    end_turn_idle(delay);
    assert(delay.player.energy == 5);

    // Invoke: Osty and the energy both arrive next turn.
    CombatEngine invoke = make_combat(CharacterId::NECROBINDER);
    play(invoke, CardId::INVOKE);
    assert(invoke.player.minion_hp == 0 && invoke.player.energy == 2);
    end_turn_idle(invoke);
    assert(invoke.player.minion_hp == 2 && invoke.player.energy == 5);

    // Countdown: damage to every enemy at the end of the third turn.
    CombatEngine bomb = make_pack(CharacterId::IRONCLAD);
    bomb.enemies[2].hp = 100;
    bomb.schedule(DelayedType::DAMAGE_ALL, 40, 3);
    end_turn_idle(bomb);
    end_turn_idle(bomb);
    assert(bomb.enemies[2].hp == 100 && bomb.enemies[0].hp == 18);
    end_turn_idle(bomb);
    assert(bomb.enemies[2].hp == 60 && !bomb.enemies[0].is_alive() && bomb.delayed_effects.empty());

    // A new combat starts with nothing scheduled.
    sidestep.schedule(DelayedType::ENERGY, 2);
    sidestep.start_combat(EnemyType::JAW_WORM, CharacterId::SILENT);
    assert(sidestep.delayed_effects.empty() && sidestep.player.energy == 3);
    std::cout << "[PASS] Next turn: Charge Battery, Lightning Rod, Glitterstream, Hegemony, Convergence, Delay, Invoke, countdown" << std::endl;
}

void test_temporary_stats() {
    // Strength until the end of the turn.
    CombatEngine setup = make_combat(CharacterId::IRONCLAD);
    play(setup, CardId::SETUP_STRIKE);
    assert(setup.enemies[0].hp == 42 - 7 && setup.player.get_status(StatusType::STRENGTH) == 3);
    play(setup, CardId::STRIKE);
    assert(setup.enemies[0].hp == 35 - 9);
    setup.player.add_status(StatusType::STRENGTH, 1); // permanent Strength is kept
    end_turn_idle(setup);
    assert(setup.player.get_status(StatusType::STRENGTH) == 1 && setup.temp_strength == 0);

    CombatEngine anticipate = make_combat(CharacterId::SILENT);
    play(anticipate, DeckCard(CardId::ANTICIPATE, true));
    play(anticipate, CardId::DEFEND);
    assert(anticipate.player.block == 9);
    end_turn_idle(anticipate);
    assert(anticipate.player.get_status(StatusType::DEXTERITY) == 0);

    // The enemy gets its Strength back after its turn: Chomp (11) hits for 1.
    CombatEngine mangle = make_combat(CharacterId::IRONCLAD);
    mangle.enemies[0].add_status(StatusType::STRENGTH, 3);
    play(mangle, CardId::MANGLE);
    assert(mangle.enemies[0].hp == 42 - 20 && mangle.enemies[0].get_status(StatusType::STRENGTH) == -7);
    assert(mangle.enemies[0].intent_damage(mangle.player) == 4);
    int16_t hp_before = mangle.player.hp;
    mangle.end_player_turn();
    assert(mangle.player.hp == hp_before - 4);
    assert(mangle.enemies[0].get_status(StatusType::STRENGTH) == 3 && mangle.enemies[0].temp_strength_loss == 0);

    CombatEngine wail = make_pack(CharacterId::SILENT);
    play(wail, DeckCard(CardId::PIERCING_WAIL, true));
    for (const Enemy& foe : wail.enemies) assert(foe.get_status(StatusType::STRENGTH) == -8);
    assert(wail.exhaust_pile.size() == 1);
    end_turn_idle(wail);
    for (const Enemy& foe : wail.enemies) assert(foe.get_status(StatusType::STRENGTH) == 0);

    CombatEngine crush = make_pack(CharacterId::REGENT);
    play(crush, DeckCard(CardId::CRUSH_UNDER, true));
    assert(crush.enemies[0].hp == 9 && crush.enemies[2].hp == 33);
    for (const Enemy& foe : crush.enemies) assert(foe.temp_strength_loss == 2);

    CombatEngine touch = make_combat(CharacterId::NECROBINDER);
    play(touch, CardId::ENFEEBLING_TOUCH);
    assert(touch.enemies[0].get_status(StatusType::STRENGTH) == -8);
    end_turn_idle(touch);
    assert(touch.enemies[0].statuses.empty());
    std::cout << "[PASS] Temporary stats: Setup Strike, Anticipate, Mangle, Piercing Wail, Crush Under, Enfeebling Touch" << std::endl;
}

void test_hp_loss() {
    // Losing HP ignores Block and is counted.
    CombatEngine wall = make_combat(CharacterId::IRONCLAD);
    wall.player.block = 10;
    play(wall, CardId::BLOOD_WALL);
    assert(wall.player.hp == 78 && wall.player.block == 26);
    assert(wall.counters.hp_lost == 2 && wall.counters.combat_hp_losses == 1);
    play(wall, DeckCard(CardId::HEMOKINESIS, true));
    assert(wall.player.hp == 76 && wall.enemies[0].hp == 42 - 20 && wall.counters.hp_lost == 4);
    play(wall, DeckCard(CardId::BLOODLETTING, true));
    assert(wall.player.hp == 73 && wall.player.energy == 6 && wall.counters.combat_hp_losses == 3);

    CombatEngine breakthrough = make_pack(CharacterId::IRONCLAD);
    play(breakthrough, CardId::BREAKTHROUGH);
    assert(breakthrough.player.hp == 79 && breakthrough.enemies[0].hp == 9 && breakthrough.enemies[2].hp == 33);

    CombatEngine offering = make_combat(CharacterId::IRONCLAD);
    play(offering, CardId::OFFERING);
    assert(offering.player.hp == 74 && offering.player.energy == 5 && offering.hand.size() == 3);
    assert(offering.exhaust_pile.size() == 1);

    // Spite hits again only when HP were lost this turn.
    CombatEngine spite = make_combat(CharacterId::IRONCLAD);
    play(spite, CardId::SPITE);
    assert(spite.enemies[0].hp == 42 - 5);
    play(spite, CardId::BLOODLETTING);
    play(spite, CardId::SPITE);
    assert(spite.enemies[0].hp == 37 - 10);
    play(spite, DeckCard(CardId::SPITE, true));
    assert(spite.enemies[0].hp == 27 - 15);

    // The turn counter restarts; damage taken from an enemy counts as a loss for the combat.
    spite.end_player_turn(); // Chomp: 11
    assert(spite.player.hp == 80 - 3 - 11);
    assert(spite.counters.hp_lost == 0 && spite.counters.combat_hp_losses == 2);

    // Paying HP can be lethal.
    CombatEngine lethal = make_combat(CharacterId::IRONCLAD);
    lethal.player.hp = 2;
    play(lethal, CardId::HEMOKINESIS);
    assert(lethal.player.hp == 0 && lethal.state == CombatState::DEFEAT);

    // Forgotten Ritual pays only after another card was exhausted this turn (its own exhaust comes too late).
    CombatEngine ritual = make_combat(CharacterId::IRONCLAD);
    play(ritual, CardId::FORGOTTEN_RITUAL);
    assert(ritual.player.energy == 2 && ritual.exhaust_pile.size() == 1);
    play(ritual, DeckCard(CardId::FORGOTTEN_RITUAL, true));
    assert(ritual.player.energy == 6);

    assert_in_pool(CharacterId::IRONCLAD, { CardId::BLOOD_WALL, CardId::BLOODLETTING, CardId::BREAKTHROUGH,
                                            CardId::FORGOTTEN_RITUAL, CardId::HEMOKINESIS, CardId::MANGLE, CardId::OFFERING,
                                            CardId::SETUP_STRIKE, CardId::SPITE, CardId::WHIRLWIND });
    std::cout << "[PASS] HP loss: Blood Wall, Hemokinesis, Bloodletting, Breakthrough, Offering, Spite, Forgotten Ritual" << std::endl;
}

void test_counters() {
    // Attacks already played this turn.
    CombatEngine finisher = make_combat(CharacterId::SILENT);
    play(finisher, CardId::FINISHER);
    assert(finisher.enemies[0].hp == 42 && finisher.counters.attacks_played == 1);
    play(finisher, CardId::STRIKE);
    play(finisher, CardId::FINISHER);
    assert(finisher.enemies[0].hp == 42 - 6 - 2 * 6);
    assert(finisher.counters.attacks_played == 3 && finisher.counters.hits_dealt == 3);
    assert(finisher.counters.combat_cards_played == 3 && finisher.counters.energy_spent == 3);
    end_turn_idle(finisher);
    assert(finisher.counters.attacks_played == 0 && finisher.counters.hits_dealt == 0);
    assert(finisher.counters.energy_spent == 0 && finisher.counters.combat_cards_played == 3);
    finisher.start_combat(EnemyType::JAW_WORM, CharacterId::SILENT);
    assert(finisher.counters.combat_cards_played == 0 && finisher.counters.combat_cards_drawn == 5);

    // Cards discarded by effects this turn.
    CombatEngine memento = make_combat(CharacterId::SILENT);
    memento.hand.clear();
    memento.hand.push_back(CardId::CALCULATED_GAMBLE);
    memento.hand.push_back(CardId::STRIKE);
    memento.hand.push_back(CardId::DEFEND);
    assert(memento.play_card(0));
    assert(memento.counters.cards_discarded == 2 && memento.counters.cards_drawn == 2);
    play(memento, CardId::MEMENTO_MORI);
    assert(memento.enemies[0].hp == 42 - (9 + 2 * 4));

    // Murder counts every card drawn since the combat started: 5 in the opening hand.
    CombatEngine murder = make_combat(CharacterId::SILENT);
    play(murder, CardId::MURDER);
    assert(murder.enemies[0].hp == 42 - 6 && murder.player.energy == 0);
    assert(CardDef::get_cost(CardId::MURDER, true) == 2);

    // FTL draws while fewer than 3 cards were played this turn.
    CombatEngine ftl = make_combat(CharacterId::DEFECT);
    play(ftl, CardId::FTL);
    assert(ftl.enemies[0].hp == 42 - 5 && ftl.hand.size() == 1);
    play(ftl, CardId::DEFEND);
    play(ftl, CardId::DEFEND);
    play(ftl, CardId::FTL);
    assert(ftl.hand.empty());
    play(ftl, DeckCard(CardId::FTL, true));
    assert(ftl.hand.empty() && ftl.counters.cards_played() == 5);

    // Helix Drill: one hit per energy spent on the other cards.
    CombatEngine helix = make_combat(CharacterId::DEFECT);
    play(helix, CardId::HELIX_DRILL);
    assert(helix.enemies[0].hp == 42);
    play(helix, CardId::DEFEND);
    play(helix, CardId::LEAP);
    assert(helix.counters.energy_spent == 2);
    play(helix, DeckCard(CardId::HELIX_DRILL, true));
    assert(helix.enemies[0].hp == 42 - 2 * 5);

    CombatEngine lunar = make_combat(CharacterId::REGENT);
    play(lunar, CardId::DEFEND);
    play(lunar, CardId::KNOW_THY_PLACE);
    assert(lunar.enemies[0].get_status(StatusType::WEAK) == 1 && lunar.enemies[0].get_status(StatusType::VULNERABLE) == 1);
    assert(lunar.exhaust_pile.size() == 1);
    play(lunar, DeckCard(CardId::KNOW_THY_PLACE, true));
    assert(lunar.exhaust_pile.size() == 1 && lunar.counters.skills_played == 3);
    lunar.enemies[0].statuses.clear();
    play(lunar, CardId::LUNAR_BLAST);
    assert(lunar.enemies[0].hp == 42 - 3 * 4);

    pad_draw_pile(lunar, 4);
    play(lunar, CardId::PROPHESIZE);
    assert(lunar.hand.size() == 6 && lunar.counters.cards_drawn == 6);

    // Cards created during the combat (Anger copies itself).
    CombatEngine anger = make_combat(CharacterId::IRONCLAD);
    play(anger, CardId::ANGER);
    assert(anger.counters.combat_cards_created == 1);
    CombatEngine massive = make_combat(CharacterId::REGENT);
    play(massive, CardId::SUPERMASSIVE);
    assert(massive.enemies[0].hp == 42 - 5);
    massive.counters.combat_cards_created = 2;
    play(massive, DeckCard(CardId::SUPERMASSIVE, true));
    assert(massive.enemies[0].hp == 37 - (5 + 2 * 4));
    assert_in_pool(CharacterId::REGENT, { CardId::CRUSH_UNDER, CardId::GLITTERSTREAM, CardId::HEAVENLY_DRILL,
                                          CardId::HEGEMONY, CardId::KNOW_THY_PLACE, CardId::LUNAR_BLAST, CardId::PROPHESIZE,
                                          CardId::SUPERMASSIVE, CardId::CONVERGENCE });

    // Death March: cards drawn by effects this turn, not the opening hand.
    CombatEngine march = make_combat(CharacterId::NECROBINDER);
    play(march, CardId::DEATH_MARCH);
    assert(march.enemies[0].hp == 42 - 8);
    play(march, CardId::PARSE);
    play(march, CardId::DEATH_MARCH);
    assert(march.enemies[0].hp == 34 - (8 + 3 * 4));

    // Pull from Below: one hit per Ethereal card played this combat.
    CombatEngine pull = make_combat(CharacterId::NECROBINDER);
    play(pull, CardId::PULL_FROM_BELOW);
    assert(pull.enemies[0].hp == 42);
    play(pull, CardId::DEFY);
    play(pull, CardId::PARSE);
    play(pull, DeckCard(CardId::PULL_FROM_BELOW, true));
    assert(pull.enemies[0].hp == 42 - 2 * 7);
    assert_in_pool(CharacterId::NECROBINDER, { CardId::DEATH_MARCH, CardId::DEFILE, CardId::DEFY, CardId::DELAY,
                                               CardId::ENFEEBLING_TOUCH, CardId::FEAR, CardId::INVOKE, CardId::PARSE,
                                               CardId::PULL_FROM_BELOW, CardId::REAP, CardId::SOW, CardId::ERADICATE });
    std::cout << "[PASS] Counters: Finisher, Memento Mori, Murder, FTL, Helix Drill, Lunar Blast, Supermassive, Death March, Pull from Below" << std::endl;
}

void test_regent_stars() {
    // A Star cost keeps the card in hand until the Stars are there.
    CombatEngine stars = make_combat(CharacterId::REGENT);
    stars.hand.clear();
    stars.hand.push_back(CardId::CLOAK_OF_STARS);
    assert(!stars.can_play_card(0));
    stars.gain_stars(1);
    assert(stars.play_card(0));
    assert(stars.player.block == 7 && stars.player.sovereign_stars == 0 && stars.player.energy == 3);
    stars.gain_stars(2);
    play(stars, DeckCard(CardId::ALIGNMENT, true));
    assert(stars.player.energy == 6 && stars.player.sovereign_stars == 0);

    CombatEngine pulse = make_pack(CharacterId::REGENT);
    pulse.gain_stars(3);
    play(pulse, CardId::ASTRAL_PULSE);
    assert(pulse.enemies[0].hp == 6 && pulse.enemies[1].hp == 6 && pulse.enemies[2].hp == 30);
    assert(pulse.player.sovereign_stars == 0);

    CombatEngine comet = make_combat(CharacterId::REGENT);
    comet.gain_stars(9);
    play(comet, CardId::COMET);
    assert(comet.enemies[0].hp == 42 - 33 && comet.player.sovereign_stars == 4);
    assert(comet.enemies[0].get_status(StatusType::WEAK) == 3 && comet.enemies[0].get_status(StatusType::VULNERABLE) == 3);
    comet.enemies[0].hp = 100;
    comet.enemies[0].statuses.clear();
    play(comet, DeckCard(CardId::DEVASTATE, true));
    assert(comet.enemies[0].hp == 100 - 45 && comet.player.sovereign_stars == 0 && comet.player.energy == 2);

    CombatEngine gamma = make_combat(CharacterId::REGENT);
    gamma.gain_stars(4);
    play(gamma, DeckCard(CardId::GAMMA_BLAST, true));
    assert(gamma.enemies[0].hp == 42 - 18 && gamma.enemies[0].get_status(StatusType::WEAK) == 2);
    assert(gamma.enemies[0].get_status(StatusType::VULNERABLE) == 2);
    gamma.enemies[0].hp = 42;
    gamma.enemies[0].statuses.clear();
    play(gamma, CardId::GUIDING_STAR);
    assert(gamma.enemies[0].hp == 42 - 12 && gamma.hand.size() == 2 && gamma.player.sovereign_stars == 0);

    // Crescent Spear counts the cards with a Star cost left in the piles, and itself.
    CombatEngine spear = make_combat(CharacterId::REGENT);
    spear.draw_pile.clear();
    spear.discard_pile.clear();
    spear.draw_pile.push_back(CardId::FALLING_STAR);
    spear.draw_pile.push_back(CardId::STRIKE);
    spear.discard_pile.push_back(CardId::COMET);
    spear.gain_stars(1);
    play(spear, CardId::CRESCENT_SPEAR);
    assert(spear.enemies[0].hp == 42 - (8 + 3 * 2));

    CombatEngine dying = make_pack(CharacterId::REGENT);
    dying.gain_stars(3);
    play(dying, CardId::DYING_STAR);
    assert(dying.enemies[0].hp == 9 && dying.enemies[2].hp == 33);
    for (const Enemy& foe : dying.enemies) assert(foe.get_status(StatusType::STRENGTH) == -9);
    assert(CardDef::has_keyword(CardId::DYING_STAR, false, KEYWORD_ETHEREAL));
    std::cout << "[PASS] Star costs: Cloak of Stars, Alignment, Astral Pulse, Comet, Devastate, Gamma Blast, Guiding Star, Crescent Spear, Dying Star" << std::endl;

    // Glow: a card now and one more next turn. Hidden Cache: Stars now and next turn. Radiate counts them.
    CombatEngine glow = make_combat(CharacterId::REGENT);
    pad_draw_pile(glow, 8);
    play(glow, DeckCard(CardId::GLOW, true));
    assert(glow.player.sovereign_stars == 2 && glow.hand.size() == 1);
    end_turn_idle(glow);
    assert(glow.hand.size() == 6 && glow.player.sovereign_stars == 2);
    play(glow, CardId::HIDDEN_CACHE);
    assert(glow.player.sovereign_stars == 3);
    end_turn_idle(glow);
    assert(glow.player.sovereign_stars == 6 && glow.counters.stars_gained == 3);
    play(glow, CardId::RADIATE);
    assert(glow.enemies[0].hp == 42 - 3 * 3);

    CombatEngine meteor = make_pack(CharacterId::REGENT);
    meteor.gain_stars(2);
    play(meteor, CardId::METEOR_SHOWER);
    assert(meteor.enemies[0].hp == 4 && meteor.enemies[2].hp == 28);
    for (const Enemy& foe : meteor.enemies) {
        assert(foe.get_status(StatusType::WEAK) == 2 && foe.get_status(StatusType::VULNERABLE) == 2);
    }

    // Particle Wall comes straight back to the hand, as long as Stars pay for it.
    CombatEngine wall = make_combat(CharacterId::REGENT);
    wall.gain_stars(4);
    play(wall, CardId::PARTICLE_WALL);
    assert(wall.player.block == 9 && wall.hand.size() == 1 && wall.hand[0] == CardId::PARTICLE_WALL);
    assert(wall.discard_pile.empty() && wall.play_card(0));
    assert(wall.player.block == 18 && wall.player.sovereign_stars == 0 && !wall.can_play_card(0));

    CombatEngine resonance = make_pack(CharacterId::REGENT);
    resonance.gain_stars(2);
    play(resonance, DeckCard(CardId::RESONANCE, true));
    assert(resonance.player.get_status(StatusType::STRENGTH) == 2);
    for (const Enemy& foe : resonance.enemies) assert(foe.get_status(StatusType::STRENGTH) == -1);

    CombatEngine gamble = make_combat(CharacterId::REGENT);
    gamble.gain_stars(5);
    play(gamble, CardId::ROYAL_GAMBLE);
    assert(gamble.player.sovereign_stars == 9 && gamble.exhaust_pile.size() == 1);
    assert(CardDef::has_keyword(CardId::ROYAL_GAMBLE, true, KEYWORD_RETAIN));
    assert(!CardDef::has_keyword(CardId::ROYAL_GAMBLE, false, KEYWORD_RETAIN));

    CombatEngine seven = make_pack(CharacterId::REGENT);
    for (Enemy& foe : seven.enemies) foe.hp = 100;
    seven.gain_stars(7);
    play(seven, CardId::SEVEN_STARS);
    for (const Enemy& foe : seven.enemies) assert(foe.hp == 100 - 7 * 7);
    assert(seven.player.energy == 1 && seven.player.sovereign_stars == 0);
    assert(CardDef::get_cost(CardId::SEVEN_STARS, true) == 1 && CardDef::get_star_cost(CardId::SEVEN_STARS, true) == 7);

    // Shining Strike goes back on top of the draw pile.
    CombatEngine shining = make_combat(CharacterId::REGENT);
    play(shining, CardId::SHINING_STRIKE);
    assert(shining.enemies[0].hp == 42 - 8 && shining.player.sovereign_stars == 2);
    assert(shining.draw_pile.back() == CardId::SHINING_STRIKE && shining.discard_pile.empty());

    // Stardust: X is the number of Stars, all spent.
    CombatEngine dust = make_combat(CharacterId::REGENT);
    dust.gain_stars(4);
    play(dust, CardId::STARDUST);
    assert(dust.enemies[0].hp == 42 - 4 * 5 && dust.player.sovereign_stars == 0 && dust.player.energy == 3);
    assert(dust.counters.stars_spent == 4);
    dust.hand.clear();
    dust.hand.push_back(DeckCard(CardId::STARDUST, true));
    assert(dust.can_play_card(0) && dust.play_card(0));
    assert(dust.enemies[0].hp == 22);

    assert_in_pool(CharacterId::REGENT, { CardId::ALIGNMENT, CardId::ASTRAL_PULSE, CardId::CLOAK_OF_STARS, CardId::COMET,
                                          CardId::CRESCENT_SPEAR, CardId::DEVASTATE, CardId::DYING_STAR, CardId::GAMMA_BLAST,
                                          CardId::GLOW, CardId::GUIDING_STAR, CardId::HIDDEN_CACHE, CardId::PARTICLE_WALL,
                                          CardId::RADIATE, CardId::RESONANCE, CardId::ROYAL_GAMBLE, CardId::SEVEN_STARS,
                                          CardId::SHINING_STRIKE, CardId::STARDUST });
    std::span<const CardId> pool = RunManager::card_reward_pool(CharacterId::REGENT);
    assert(std::find(pool.begin(), pool.end(), CardId::METEOR_SHOWER) == pool.end()); // Ancient rarity
    std::cout << "[PASS] Stars: Glow, Hidden Cache, Radiate, Meteor Shower, Particle Wall, Resonance, Royal Gamble, Seven Stars, Shining Strike, Stardust" << std::endl;
}

void test_fatal() {
    // Sunder: the energy comes back only when the hit kills.
    CombatEngine sunder = make_pack(CharacterId::DEFECT);
    play(sunder, CardId::SUNDER, 2);
    assert(sunder.enemies[2].hp == 42 - 26 && sunder.player.energy == 0 && sunder.fatal_kills == 0);
    play(sunder, CardId::SUNDER, 0);
    assert(!sunder.enemies[0].is_alive() && sunder.player.energy == 3 && sunder.fatal_kills == 1);

    // Killing a minion is not a Fatal blow.
    sunder.enemies[1].minion = true;
    play(sunder, DeckCard(CardId::SUNDER, true), 1);
    assert(!sunder.enemies[1].is_alive() && sunder.player.energy == 0 && sunder.fatal_kills == 0);

    // The tally belongs to the card being played.
    play(sunder, CardId::STRIKE, 2);
    assert(sunder.fatal_kills == 0);

    CombatEngine knockout = make_pack(CharacterId::REGENT);
    play(knockout, CardId::KNOCKOUT_BLOW, 2);
    assert(knockout.enemies[2].hp == 42 - 30 && knockout.player.sovereign_stars == 0);
    play(knockout, DeckCard(CardId::KNOCKOUT_BLOW, true), 0);
    assert(knockout.player.sovereign_stars == 5 && knockout.counters.stars_gained == 5);

    // Echoing Slash: each enemy killed by a sweep adds a sweep, kills of the extra sweeps included.
    CombatEngine echo = make_pack(CharacterId::SILENT);
    play(echo, CardId::ECHOING_SLASH);
    assert(echo.enemies[0].hp == 8 && echo.enemies[1].hp == 8 && echo.enemies[2].hp == 32);
    echo.enemies[0].hp = 10;
    echo.enemies[1].hp = 15;
    echo.enemies[2].hp = 42;
    play(echo, CardId::ECHOING_SLASH);
    assert(!echo.enemies[0].is_alive() && !echo.enemies[1].is_alive());
    assert(echo.enemies[2].hp == 42 - 3 * 10 && echo.fatal_kills == 2);
    assert(CardDef::get_value(CardId::ECHOING_SLASH, true) == 13);

    // Feed: the Max HP gained is kept by the run after the fight.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(run, RoomType::MONSTER);
    int16_t max_before = run.max_hp;
    int16_t hp_before = run.combat.player.hp;
    for (Enemy& foe : run.combat.enemies) foe.hp = 5;
    while (run.combat.has_alive_enemies()) {
        play(run.combat, DeckCard(CardId::FEED, true));
    }
    int kills = static_cast<int>(run.combat.enemies.size());
    assert(run.combat.player.max_hp == max_before + 4 * kills && run.combat.player.hp == hp_before + 4 * kills);
    assert(run.combat.exhaust_pile.size() == static_cast<size_t>(kills));
    run.proceed_after_combat();
    assert(run.max_hp == max_before + 4 * kills);

    CombatEngine feed = make_combat(CharacterId::IRONCLAD);
    play(feed, CardId::FEED);
    assert(feed.enemies[0].hp == 42 - 10 && feed.player.max_hp == 80);

    assert_in_pool(CharacterId::DEFECT, { CardId::SUNDER });
    assert_in_pool(CharacterId::REGENT, { CardId::KNOCKOUT_BLOW });
    assert_in_pool(CharacterId::SILENT, { CardId::ECHOING_SLASH, CardId::GRAND_FINALE });
    assert_in_pool(CharacterId::IRONCLAD, { CardId::FEED, CardId::BATTLE_TRANCE });
    std::cout << "[PASS] Fatal: Sunder, Knockout Blow, Echoing Slash, Feed, minions do not count" << std::endl;
}

void test_play_conditions() {
    // Grand Finale needs an empty draw pile.
    CombatEngine finale = make_pack(CharacterId::SILENT);
    for (Enemy& foe : finale.enemies) foe.hp = 100;
    finale.hand.clear();
    finale.hand.push_back(CardId::GRAND_FINALE);
    assert(!finale.draw_pile.empty());
    assert(finale.play_blocker(0) == PlayBlocker::CONDITION && !finale.play_card(0));
    finale.draw_pile.clear();
    assert(finale.play_blocker(0) == PlayBlocker::NONE && finale.play_card(0));
    for (const Enemy& foe : finale.enemies) assert(foe.hp == 40);
    assert(CardDef::get_value(CardId::GRAND_FINALE, true) == 75);

    // A hand made of Attacks only.
    CombatEngine clash = make_combat(CharacterId::IRONCLAD);
    clash.hand.clear();
    clash.hand.push_back(CardId::STRIKE);
    clash.hand.push_back(CardId::BASH);
    assert(clash.play_condition_met(PlayCondition::HAND_ALL_ATTACKS));
    clash.hand.push_back(CardId::DEFEND);
    assert(!clash.play_condition_met(PlayCondition::HAND_ALL_ATTACKS));
    assert(clash.play_condition_met(PlayCondition::NONE));

    // Battle Trance: nothing more is drawn this turn, whatever asks for it.
    CombatEngine trance = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(trance, 10);
    play(trance, DeckCard(CardId::BATTLE_TRANCE, true));
    assert(trance.hand.size() == 4 && trance.no_draw_this_turn && trance.player.energy == 3);
    trance.hand.clear();
    play(trance, CardId::POMMEL_STRIKE);
    assert(trance.hand.empty() && !trance.draw_card());
    end_turn_idle(trance);
    assert(trance.hand.size() == 5 && !trance.no_draw_this_turn);

    // A card that must be played before the others.
    CombatEngine first = make_combat(CharacterId::IRONCLAD);
    DeckCard demanding(CardId::DEFEND);
    demanding.added_keywords = KEYWORD_PLAY_FIRST;
    first.hand.clear();
    first.hand.push_back(CardId::STRIKE);
    first.hand.push_back(demanding);
    assert(first.play_blocker(0) == PlayBlocker::PLAY_FIRST && first.play_blocker(1) == PlayBlocker::NONE);
    assert(first.play_card(1));
    assert(first.play_blocker(0) == PlayBlocker::NONE);

    // A card that caps the turn at 3 cards while it is in hand.
    CombatEngine limit = make_combat(CharacterId::IRONCLAD);
    DeckCard numbing(CardId::DEFEND);
    numbing.added_keywords = KEYWORD_LIMIT_PLAYS;
    limit.hand.clear();
    limit.hand.push_back(numbing);
    for (int i = 0; i < 4; ++i) limit.hand.push_back(CardId::ANGER); // free
    for (int i = 0; i < 3; ++i) assert(limit.play_card(1));
    assert(limit.play_blocker(1) == PlayBlocker::PLAY_LIMIT && limit.play_blocker(0) == PlayBlocker::PLAY_LIMIT);
    limit.hand.erase(0);
    assert(limit.play_blocker(0) == PlayBlocker::NONE);

    // The other reasons a card stays in hand.
    CombatEngine blocked = make_combat(CharacterId::REGENT);
    blocked.hand.clear();
    blocked.hand.push_back(CardId::STRIKE);
    blocked.hand.push_back(CardId::FALLING_STAR);
    blocked.player.energy = 0;
    assert(blocked.play_blocker(0) == PlayBlocker::ENERGY && blocked.play_blocker(1) == PlayBlocker::STARS);
    assert(blocked.play_blocker(5) == PlayBlocker::NOT_PLAYABLE);
    std::cout << "[PASS] Play conditions: Grand Finale, Attack-only hand, Battle Trance, play first, play limit" << std::endl;
}

void test_power_registry() {
    // A Power stays in play: it leaves the piles, and playing it again adds to the same power.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    play(combat, CardId::FEEL_NO_PAIN);
    play(combat, DeckCard(CardId::FEEL_NO_PAIN, true));
    assert(combat.powers.size() == 1 && combat.powers[0].id == PowerId::FEEL_NO_PAIN);
    assert(combat.powers[0].amount == 7 && combat.powers[0].stacks == 2);
    assert(combat.power_amount(PowerId::FEEL_NO_PAIN) == 7 && !combat.has_power(PowerId::BARRICADE));
    assert(combat.powers_in_play.size() == 2 && count_in_piles(combat, CardId::FEEL_NO_PAIN) == 0);

    // An effect "this turn" is a power that leaves at the end of the turn.
    play(combat, CardId::RAGE);
    assert(combat.powers.size() == 2 && combat.powers[1].turn_only && combat.powers_in_play.size() == 2);
    end_turn_idle(combat);
    assert(combat.powers.size() == 1 && combat.powers[0].turns == 1);
    end_turn_idle(combat);
    assert(combat.powers[0].turns == 2);

    // A new combat starts without any power.
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    assert(combat.powers.empty() && combat.powers_in_play.empty());

    // Rules read by the engine: Barricade keeps the Block, Well-Laid Plans keeps the hand.
    play(combat, CardId::BARRICADE);
    play(combat, CardId::DEFEND);
    end_turn_idle(combat);
    assert(combat.player.block == 5);
    assert(CardDef::get_cost(CardId::BARRICADE, false) == 3 && CardDef::get_cost(CardId::BARRICADE, true) == 2);

    CombatEngine plans = make_combat(CharacterId::SILENT);
    pad_draw_pile(plans, 10);
    play(plans, DeckCard(CardId::WELL_LAID_PLANS, true));
    assert(plans.player.energy == 2);
    plans.hand.push_back(CardId::STRIKE);
    plans.hand.push_back(CardId::DEFILE);
    end_turn_idle(plans);
    assert(plans.hand.size() == 6 && plans.hand[0] == CardId::STRIKE); // the Ethereal card is still exhausted
    assert(plans.exhaust_pile.size() == 1);

    // Accelerant: one more Poison tick on each enemy turn.
    CombatEngine poison = make_combat(CharacterId::SILENT);
    play(poison, CardId::ACCELERANT);
    poison.enemies[0].add_status(StatusType::POISON, 5);
    end_turn_idle(poison);
    assert(poison.enemies[0].hp == 42 - 5 - 4 && poison.enemies[0].get_status(StatusType::POISON) == 3);
    std::cout << "[PASS] Power registry: stacking, turn-only effects, Barricade, Well-Laid Plans, Accelerant" << std::endl;
}

void test_turn_powers() {
    // Energy, Strength, Stars and cards at the start of each turn.
    CombatEngine pyre = make_combat(CharacterId::IRONCLAD);
    play(pyre, DeckCard(CardId::PYRE, true));
    play(pyre, CardId::DEMON_FORM);
    assert(pyre.player.get_status(StatusType::STRENGTH) == 0);
    end_turn_idle(pyre);
    assert(pyre.player.energy == 5 && pyre.player.get_status(StatusType::STRENGTH) == 3);
    end_turn_idle(pyre);
    assert(pyre.player.energy == 5 && pyre.player.get_status(StatusType::STRENGTH) == 6);

    CombatEngine learning = make_combat(CharacterId::DEFECT);
    pad_draw_pile(learning, 10);
    play(learning, CardId::MACHINE_LEARNING);
    end_turn_idle(learning);
    assert(learning.hand.size() == 6 && learning.counters.cards_drawn == 0);
    assert(CardDef::has_keyword(CardId::MACHINE_LEARNING, true, KEYWORD_INNATE));

    CombatEngine genesis = make_combat(CharacterId::REGENT);
    play(genesis, DeckCard(CardId::GENESIS, true));
    end_turn_idle(genesis);
    assert(genesis.player.sovereign_stars == 3 && genesis.counters.stars_gained == 3);

    // Noxious Fumes: Poison on every enemy when the turn starts.
    CombatEngine fumes = make_pack(CharacterId::SILENT);
    play(fumes, CardId::NOXIOUS_FUMES);
    end_turn_idle(fumes);
    for (const Enemy& foe : fumes.enemies) assert(foe.get_status(StatusType::POISON) == 2);

    // Crimson Mantle: 1 HP for Block, each turn.
    CombatEngine mantle = make_combat(CharacterId::IRONCLAD);
    play(mantle, CardId::CRIMSON_MANTLE);
    end_turn_idle(mantle);
    assert(mantle.player.hp == 79 && mantle.player.block == 7 && mantle.counters.hp_lost == 1);
    mantle.player.hp = 1;
    end_turn_idle(mantle);
    assert(mantle.state == CombatState::DEFEAT);

    // Inferno: the HP it takes at the start of the turn already burns every enemy.
    CombatEngine inferno = make_pack(CharacterId::IRONCLAD);
    play(inferno, CardId::INFERNO);
    end_turn_idle(inferno);
    assert(inferno.player.hp == 79 && inferno.enemies[0].hp == 12 && inferno.enemies[2].hp == 36);
    play(inferno, CardId::BLOODLETTING);
    assert(inferno.enemies[0].hp == 6 && inferno.enemies[2].hp == 30);
    inferno.enemies[0].hp = 3;
    inferno.enemies[1].hp = 3;
    inferno.enemies[2].hp = 3;
    end_turn_idle(inferno);
    assert(inferno.state == CombatState::VICTORY);

    // Rupture: only the HP lost during the player's turn count.
    CombatEngine rupture = make_combat(CharacterId::IRONCLAD);
    play(rupture, DeckCard(CardId::RUPTURE, true));
    play(rupture, CardId::BLOODLETTING);
    assert(rupture.player.get_status(StatusType::STRENGTH) == 2);
    rupture.end_player_turn(); // Chomp
    assert(rupture.player.hp == 80 - 3 - 11 && rupture.player.get_status(StatusType::STRENGTH) == 2);

    // Aggression: a random Attack of the discard pile comes back upgraded.
    CombatEngine aggression = make_combat(CharacterId::IRONCLAD);
    play(aggression, CardId::AGGRESSION);
    aggression.hand.clear();
    aggression.draw_pile.clear();
    pad_draw_pile(aggression, 10);
    aggression.discard_pile.clear();
    aggression.discard_pile.push_back(CardId::DEFEND);
    aggression.discard_pile.push_back(CardId::BASH);
    end_turn_idle(aggression);
    assert(aggression.hand.size() == 6 && aggression.hand[0] == CardId::BASH && aggression.hand[0].upgraded);
    assert(aggression.discard_pile.size() == 1);
    assert(CardDef::has_keyword(CardId::AGGRESSION, true, KEYWORD_INNATE));

    // Friendship and Demesne.
    CombatEngine friendship = make_combat(CharacterId::NECROBINDER);
    pad_draw_pile(friendship, 10);
    play(friendship, CardId::FRIENDSHIP);
    assert(friendship.player.get_status(StatusType::STRENGTH) == -2);
    play(friendship, DeckCard(CardId::DEMESNE, true));
    assert(friendship.player.energy == 1 && friendship.powers.size() == 2);
    end_turn_idle(friendship);
    assert(friendship.player.energy == 5 && friendship.hand.size() == 6);
    assert(CardDef::has_keyword(CardId::DEMESNE, false, KEYWORD_ETHEREAL));

    // Pale Blue Dot: 5 cards in the turn, itself included.
    CombatEngine dot = make_combat(CharacterId::REGENT);
    pad_draw_pile(dot, 20);
    play(dot, CardId::PALE_BLUE_DOT);
    for (int i = 0; i < 3; ++i) play(dot, CardId::DEFEND);
    end_turn_idle(dot);
    assert(dot.hand.size() == 5);
    for (int i = 0; i < 5; ++i) play(dot, CardId::DEFEND);
    end_turn_idle(dot);
    assert(dot.hand.size() == 6);

    assert_in_pool(CharacterId::IRONCLAD, { CardId::FEEL_NO_PAIN, CardId::DARK_EMBRACE, CardId::DRUM_OF_BATTLE,
                                            CardId::BARRICADE, CardId::INFERNO, CardId::CRIMSON_MANTLE, CardId::RUPTURE,
                                            CardId::AGGRESSION, CardId::PYRE, CardId::DEMON_FORM, CardId::JUGGERNAUT,
                                            CardId::RAGE, CardId::VICIOUS });
    std::cout << "[PASS] Turn powers: Pyre, Demon Form, Machine Learning, Genesis, Noxious Fumes, Crimson Mantle, Inferno, Rupture, Aggression, Friendship, Demesne, Pale Blue Dot" << std::endl;
}

void test_reaction_powers() {
    // Cards exhausted.
    CombatEngine pain = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(pain, 10);
    play(pain, CardId::FEEL_NO_PAIN);
    play(pain, DeckCard(CardId::DARK_EMBRACE, true));
    assert(pain.player.energy == 2);
    play(pain, CardId::IMPERVIOUS);
    assert(pain.player.block == 33 && pain.hand.size() == 1);

    // Drum of Battle pays when something exhausts it.
    CombatEngine drum = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(drum, 10);
    play(drum, CardId::DRUM_OF_BATTLE);
    assert(drum.hand.size() == 2 && drum.player.energy == 2);
    drum.hand.clear();
    drum.hand.push_back(CardId::TRUE_GRIT);
    drum.hand.push_back(DeckCard(CardId::DRUM_OF_BATTLE, true));
    drum.player.energy = 3;
    assert(drum.play_card(0));
    assert(drum.exhaust_pile.size() == 1 && drum.player.energy == 3 - 1 + 3);

    // Block gained, Attacks played, Vulnerable applied.
    CombatEngine juggernaut = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(juggernaut, 10);
    play(juggernaut, CardId::JUGGERNAUT);
    play(juggernaut, CardId::DEFEND);
    assert(juggernaut.enemies[0].hp == 42 - 6);
    play(juggernaut, DeckCard(CardId::RAGE, true));
    play(juggernaut, CardId::STRIKE);
    assert(juggernaut.player.block == 5 + 5 && juggernaut.enemies[0].hp == 36 - 6 - 6);
    play(juggernaut, DeckCard(CardId::VICIOUS, true));
    play(juggernaut, CardId::BASH);
    assert(juggernaut.hand.size() == 2);
    end_turn_idle(juggernaut);
    int16_t worm_hp = juggernaut.enemies[0].hp;
    play(juggernaut, CardId::STRIKE); // Rage is over: no Block, so no Juggernaut hit either
    assert(juggernaut.player.block == 0 && juggernaut.enemies[0].hp == worm_hp - 9);
    std::cout << "[PASS] Ironclad powers: Feel No Pain, Dark Embrace, Drum of Battle, Juggernaut, Rage, Vicious" << std::endl;

    // Cards drawn: the opening hand does not count for Speedster.
    CombatEngine speedster = make_pack(CharacterId::SILENT);
    pad_draw_pile(speedster, 20);
    play(speedster, CardId::SPEEDSTER);
    end_turn_idle(speedster);
    assert(speedster.enemies[2].hp == 42);
    play(speedster, CardId::BACKFLIP);
    assert(speedster.enemies[0].hp == 14 && speedster.enemies[2].hp == 38);
    play(speedster, DeckCard(CardId::CORROSIVE_WAVE, true));
    play(speedster, CardId::BACKFLIP);
    for (const Enemy& foe : speedster.enemies) assert(foe.get_status(StatusType::POISON) == 6);
    end_turn_idle(speedster);
    assert(speedster.powers.size() == 1); // Corrosive Wave only lasted the turn

    // Envenom: only the hits that go through the Block.
    CombatEngine envenom = make_combat(CharacterId::SILENT);
    play(envenom, DeckCard(CardId::ENVENOM, true));
    play(envenom, CardId::STRIKE);
    assert(envenom.enemies[0].get_status(StatusType::POISON) == 2);
    envenom.enemies[0].block = 20;
    play(envenom, CardId::STRIKE);
    assert(envenom.enemies[0].get_status(StatusType::POISON) == 2);

    // Cards played. A power does not react to the card that brings it.
    CombatEngine serpent = make_combat(CharacterId::SILENT);
    play(serpent, CardId::SERPENT_FORM);
    assert(serpent.enemies[0].hp == 42);
    play(serpent, CardId::AFTERIMAGE);
    assert(serpent.enemies[0].hp == 38 && serpent.player.block == 0);
    play(serpent, CardId::DEFEND);
    assert(serpent.enemies[0].hp == 34 && serpent.player.block == 1 + 5);

    // Strangle follows its target, through its Block, for the turn.
    CombatEngine strangle = make_pack(CharacterId::SILENT);
    play(strangle, CardId::STRANGLE, 2);
    assert(strangle.enemies[2].hp == 42 - 8);
    strangle.enemies[2].block = 10;
    play(strangle, CardId::DEFEND);
    assert(strangle.enemies[2].hp == 32 && strangle.enemies[2].block == 10 && strangle.enemies[0].hp == 18);
    end_turn_idle(strangle);
    play(strangle, CardId::DEFEND);
    assert(strangle.enemies[2].hp == 32);
    assert_in_pool(CharacterId::SILENT, { CardId::ACCELERANT, CardId::SPEEDSTER, CardId::ENVENOM, CardId::SERPENT_FORM,
                                          CardId::AFTERIMAGE, CardId::WELL_LAID_PLANS, CardId::STRANGLE,
                                          CardId::CORROSIVE_WAVE, CardId::NOXIOUS_FUMES });
    std::cout << "[PASS] Silent powers: Speedster, Corrosive Wave, Envenom, Serpent Form, Afterimage, Strangle" << std::endl;

    // Powers played: Storm channels, Subroutine pays back.
    CombatEngine storm = make_combat(CharacterId::DEFECT);
    play(storm, CardId::STORM);
    assert(storm.player.orbs.empty());
    play(storm, CardId::SUBROUTINE);
    assert(storm.player.orbs.size() == 1 && storm.player.energy == 2);
    play(storm, DeckCard(CardId::STORM, true));
    assert(storm.player.orbs.size() == 2 && storm.player.energy == 3);
    assert(CardDef::get_cost(CardId::SUBROUTINE, true) == 0);

    // Feral: the first 0-cost Attack of each turn comes back.
    CombatEngine feral = make_combat(CharacterId::DEFECT);
    feral.draw_pile.clear();
    play(feral, DeckCard(CardId::FERAL, true));
    assert(feral.player.energy == 2);
    play(feral, CardId::GO_FOR_THE_EYES);
    assert(count_in_hand(feral, CardId::GO_FOR_THE_EYES) == 1 && feral.discard_pile.empty());
    assert(feral.play_card(0));
    assert(feral.hand.empty() && feral.discard_pile.size() == 1);
    feral.draw_pile.clear();
    pad_draw_pile(feral, 10);
    end_turn_idle(feral);
    play(feral, CardId::GO_FOR_THE_EYES);
    assert(count_in_hand(feral, CardId::GO_FOR_THE_EYES) == 1);
    assert_in_pool(CharacterId::DEFECT, { CardId::MACHINE_LEARNING, CardId::STORM, CardId::FERAL, CardId::SUBROUTINE });
    std::cout << "[PASS] Defect powers: Storm, Subroutine, Feral" << std::endl;

    // Stars gained and spent.
    CombatEngine stars = make_pack(CharacterId::REGENT);
    play(stars, DeckCard(CardId::CHILD_OF_THE_STARS, true));
    play(stars, CardId::BLACK_HOLE);
    stars.gain_stars(3);
    assert(stars.enemies[0].hp == 15 && stars.enemies[2].hp == 39);
    stars.player.block = 0;
    play(stars, CardId::CLOAK_OF_STARS);
    assert(stars.player.block == 3 + 7 && stars.enemies[0].hp == 12 && stars.enemies[2].hp == 36);

    // The Sealed Throne: a Star for each card, once it is in play.
    CombatEngine throne = make_combat(CharacterId::REGENT);
    throne.gain_stars(3);
    play(throne, CardId::THE_SEALED_THRONE);
    assert(throne.player.sovereign_stars == 0 && throne.player.energy == 2);
    play(throne, CardId::DEFEND);
    assert(throne.player.sovereign_stars == 1);
    assert(CardDef::get_star_cost(CardId::THE_SEALED_THRONE, false) == 3);

    // Make It So comes back from the piles on every third Skill of the turn.
    CombatEngine so = make_combat(CharacterId::REGENT);
    play(so, CardId::MAKE_IT_SO);
    assert(so.enemies[0].hp == 42 - 6 && so.discard_pile.back() == CardId::MAKE_IT_SO);
    play(so, CardId::DEFEND);
    play(so, CardId::DEFEND);
    assert(count_in_hand(so, CardId::MAKE_IT_SO) == 0);
    play(so, CardId::DEFEND);
    assert(count_in_hand(so, CardId::MAKE_IT_SO) == 1);

    // Cards created.
    CombatEngine arsenal = make_combat(CharacterId::REGENT);
    play(arsenal, CardId::ARSENAL);
    play(arsenal, DeckCard(CardId::PILLAR_OF_CREATION, true));
    play(arsenal, CardId::ANGER);
    assert(arsenal.player.get_status(StatusType::STRENGTH) == 1 && arsenal.player.block == 3);

    // Monologue: Strength for the turn with each card.
    CombatEngine monologue = make_combat(CharacterId::REGENT);
    play(monologue, CardId::MONOLOGUE);
    play(monologue, CardId::DEFEND);
    play(monologue, CardId::STRIKE);
    assert(monologue.enemies[0].hp == 42 - 8 && monologue.player.get_status(StatusType::STRENGTH) == 2);
    end_turn_idle(monologue);
    assert(monologue.player.get_status(StatusType::STRENGTH) == 0 && monologue.powers.empty());
    assert(CardDef::has_keyword(CardId::MONOLOGUE, true, KEYWORD_RETAIN));

    // Orbit: 1 energy back for every 4 spent after it.
    CombatEngine orbit = make_combat(CharacterId::REGENT);
    play(orbit, CardId::ORBIT);
    play(orbit, CardId::BLUDGEON);
    assert(orbit.player.energy == 0);
    play(orbit, CardId::STRIKE);
    assert(orbit.player.energy == 3 - 1 + 1 && orbit.powers[0].counter == 0);

    // Monarch's Gaze: each hit takes Strength for the enemy's next turn.
    CombatEngine gaze = make_combat(CharacterId::REGENT);
    play(gaze, DeckCard(CardId::MONARCHS_GAZE, true));
    assert(gaze.player.energy == 2);
    play(gaze, CardId::CELESTIAL_MIGHT);
    assert(gaze.enemies[0].get_status(StatusType::STRENGTH) == -3 && gaze.enemies[0].temp_strength_loss == 3);
    end_turn_idle(gaze);
    assert(gaze.enemies[0].get_status(StatusType::STRENGTH) == 0);
    assert_in_pool(CharacterId::REGENT, { CardId::CHILD_OF_THE_STARS, CardId::GENESIS, CardId::BLACK_HOLE, CardId::MAKE_IT_SO,
                                          CardId::PALE_BLUE_DOT, CardId::ARSENAL, CardId::MONOLOGUE, CardId::ORBIT,
                                          CardId::PILLAR_OF_CREATION, CardId::MONARCHS_GAZE });
    std::span<const CardId> regent_pool = RunManager::card_reward_pool(CharacterId::REGENT);
    assert(std::find(regent_pool.begin(), regent_pool.end(), CardId::THE_SEALED_THRONE) == regent_pool.end()); // Ancient
    std::cout << "[PASS] Regent powers: Child of the Stars, Black Hole, The Sealed Throne, Make It So, Arsenal, Pillar of Creation, Monologue, Orbit, Monarch's Gaze" << std::endl;

    // Cost of the card played, Ethereal cards played and drawn.
    CombatEngine danse = make_combat(CharacterId::NECROBINDER);
    play(danse, DeckCard(CardId::DANSE_MACABRE, true));
    play(danse, CardId::DEFEND);
    assert(danse.player.block == 5);
    play(danse, CardId::DELAY);
    assert(danse.player.block == 5 + 11 + 6);
    danse.player.block = 0;
    play(danse, CardId::SPIRIT_OF_ASH);
    play(danse, CardId::DEFILE);
    assert(danse.player.block == 4);

    CombatEngine pages = make_combat(CharacterId::NECROBINDER);
    play(pages, DeckCard(CardId::PAGESTORM, true));
    assert(pages.player.energy == 3);
    pages.draw_pile.push_back(CardId::STRIKE);
    pages.draw_pile.push_back(CardId::DEFILE);
    pages.draw_cards(1);
    assert(pages.hand.size() == 2 && pages.hand[0] == CardId::DEFILE && pages.hand[1] == CardId::STRIKE);

    // Sleight of Flesh: each debuff hurts.
    CombatEngine sleight = make_combat(CharacterId::NECROBINDER);
    play(sleight, CardId::SLEIGHT_OF_FLESH);
    play(sleight, CardId::PUTREFY);
    assert(sleight.enemies[0].hp == 42 - 2 * 9);
    assert_in_pool(CharacterId::NECROBINDER, { CardId::FRIENDSHIP, CardId::DANSE_MACABRE, CardId::DEMESNE,
                                               CardId::SPIRIT_OF_ASH, CardId::PAGESTORM, CardId::SLEIGHT_OF_FLESH });
    std::cout << "[PASS] Necrobinder powers: Danse Macabre, Spirit of Ash, Pagestorm, Sleight of Flesh" << std::endl;
}

// A fight opened the way RunManager does it: state reset, relic registered as a power, then the first turn.
struct RelicPower {
    PowerId id;
    int16_t amount = 1;
};

CombatEngine make_relic_combat(CharacterId hero, std::initializer_list<RelicPower> relics, bool pack = false) {
    CombatEngine combat = pack ? make_pack(hero) : make_combat(hero);
    while (!combat.hand.empty()) {
        combat.draw_pile.push_back(combat.hand.back());
        combat.hand.pop_back();
    }
    pad_draw_pile(combat, 30);
    combat.reset_combat_state();
    for (const RelicPower& relic : relics) combat.add_power(relic.id, relic.amount);
    combat.begin_first_turn();
    return combat;
}

CombatEngine make_relic_combat(CharacterId hero, PowerId relic, int16_t amount = 1, bool pack = false) {
    return make_relic_combat(hero, { RelicPower{ relic, amount } }, pack);
}

void test_turn_relics() {
    // Effects tied to a turn number.
    CombatEngine wheel = make_relic_combat(CharacterId::IRONCLAD, { { PowerId::CAPTAINS_WHEEL, 18 }, { PowerId::HORN_CLEAT, 14 } });
    assert(wheel.player.block == 0 && wheel.powers[0].turns == 1);
    end_turn_idle(wheel);
    assert(wheel.player.block == 14);
    end_turn_idle(wheel);
    assert(wheel.player.block == 18);
    end_turn_idle(wheel);
    assert(wheel.player.block == 0);

    CombatEngine candles = make_relic_combat(CharacterId::IRONCLAD, { { PowerId::CANDELABRA, 2 }, { PowerId::CHANDELIER, 3 },
                                                                      { PowerId::SPARKLING_ROUGE, 1 } });
    assert(candles.player.energy == 3);
    end_turn_idle(candles);
    assert(candles.player.energy == 5 && candles.player.get_status(StatusType::STRENGTH) == 0);
    end_turn_idle(candles);
    assert(candles.player.energy == 6);
    assert(candles.player.get_status(StatusType::STRENGTH) == 1 && candles.player.get_status(StatusType::DEXTERITY) == 1);
    end_turn_idle(candles);
    assert(candles.player.energy == 3 && candles.player.get_status(StatusType::STRENGTH) == 1);

    // Effects every 3 turns.
    CombatEngine flower = make_relic_combat(CharacterId::IRONCLAD, { { PowerId::HAPPY_FLOWER, 1 }, { PowerId::PENDULUM, 1 } });
    end_turn_idle(flower);
    assert(flower.player.energy == 3 && flower.hand.size() == 5);
    end_turn_idle(flower);
    assert(flower.player.energy == 4 && flower.hand.size() == 6);
    end_turn_idle(flower);
    assert(flower.player.energy == 3 && flower.hand.size() == 5);
    end_turn_idle(flower);
    end_turn_idle(flower);
    assert(flower.player.energy == 4 && flower.hand.size() == 6);

    // Bread costs the first turn and pays the next ones.
    CombatEngine bread = make_relic_combat(CharacterId::IRONCLAD, PowerId::BREAD, 1);
    assert(bread.player.energy == 1);
    end_turn_idle(bread);
    assert(bread.player.energy == 4);

    CombatEngine hourglass = make_relic_combat(CharacterId::IRONCLAD, PowerId::MERCURY_HOURGLASS, 3, true);
    assert(hourglass.enemies[0].hp == 15 && hourglass.enemies[2].hp == 39);
    end_turn_idle(hourglass);
    assert(hourglass.enemies[0].hp == 12 && hourglass.enemies[2].hp == 36);
    std::cout << "[PASS] Turn start relics: Captain's Wheel, Horn Cleat, Candelabra, Chandelier, Sparkling Rouge, Happy Flower, Pendulum, Bread, Mercury Hourglass" << std::endl;

    // End of turn: Block for the enemy's attack (Chomp: 11).
    CombatEngine clasp = make_relic_combat(CharacterId::IRONCLAD, PowerId::CLOAK_CLASP, 1);
    clasp.hand.pop_back(); // 4 cards left
    clasp.end_player_turn();
    assert(clasp.player.hp == 80 - (11 - 4));

    CombatEngine orichalcum = make_relic_combat(CharacterId::IRONCLAD, PowerId::ORICHALCUM, 6);
    orichalcum.end_player_turn();
    assert(orichalcum.player.hp == 80 - (11 - 6));
    play(orichalcum, CardId::DEFEND); // with Block already, nothing more
    orichalcum.enemies[0].current_intent.base_damage = 11;
    orichalcum.enemies[0].current_intent.type = EnemyIntentType::ATTACK;
    int16_t hp_before = orichalcum.player.hp;
    orichalcum.end_player_turn();
    assert(orichalcum.player.hp == hp_before - (11 - 5));

    CombatEngine parry = make_relic_combat(CharacterId::IRONCLAD, PowerId::PARRYING_SHIELD, 6);
    end_turn_idle(parry);
    assert(parry.enemies[0].hp == 42);
    play(parry, CardId::IMPERVIOUS);
    end_turn_idle(parry);
    assert(parry.enemies[0].hp == 42 - 6);

    // Stone Calendar: the end of turn 7, and only that one.
    CombatEngine calendar = make_relic_combat(CharacterId::IRONCLAD, PowerId::STONE_CALENDAR, 52);
    calendar.enemies[0].hp = 100;
    for (int turn = 1; turn <= 6; ++turn) end_turn_idle(calendar);
    assert(calendar.enemies[0].hp == 100);
    end_turn_idle(calendar);
    assert(calendar.enemies[0].hp == 48);
    end_turn_idle(calendar);
    assert(calendar.enemies[0].hp == 48);

    CombatEngine flagon = make_relic_combat(CharacterId::IRONCLAD, PowerId::SCREAMING_FLAGON, 20, true);
    for (Enemy& foe : flagon.enemies) foe.hp = 100;
    end_turn_idle(flagon);
    assert(flagon.enemies[0].hp == 100);
    flagon.hand.clear();
    end_turn_idle(flagon);
    for (const Enemy& foe : flagon.enemies) assert(foe.hp == 80);

    // What the turn was like decides the next one.
    CombatEngine war = make_relic_combat(CharacterId::IRONCLAD, PowerId::ART_OF_WAR, 1);
    play(war, CardId::DEFEND);
    end_turn_idle(war);
    assert(war.player.energy == 4);
    play(war, CardId::STRIKE);
    end_turn_idle(war);
    assert(war.player.energy == 3);

    CombatEngine watch = make_relic_combat(CharacterId::IRONCLAD, PowerId::POCKETWATCH, 3);
    for (int i = 0; i < 3; ++i) play(watch, CardId::DEFEND);
    end_turn_idle(watch);
    assert(watch.hand.size() == 8);
    for (int i = 0; i < 4; ++i) play(watch, CardId::DEFEND);
    end_turn_idle(watch);
    assert(watch.hand.size() == 5);

    // Kept from one turn to the next.
    CombatEngine cream = make_relic_combat(CharacterId::IRONCLAD, PowerId::ICE_CREAM);
    assert(cream.player.energy == 3);
    cream.player.energy = 2;
    end_turn_idle(cream);
    assert(cream.player.energy == 5);

    CombatEngine clamp = make_relic_combat(CharacterId::IRONCLAD, PowerId::STURDY_CLAMP, 10);
    play(clamp, CardId::IMPERVIOUS);
    end_turn_idle(clamp);
    assert(clamp.player.block == 10);
    clamp.player.block = 4;
    end_turn_idle(clamp);
    assert(clamp.player.block == 4);
    std::cout << "[PASS] Turn end relics: Cloak Clasp, Orichalcum, Parrying Shield, Stone Calendar, Screaming Flagon, Art of War, Pocketwatch, Ice Cream, Sturdy Clamp" << std::endl;
}

void test_event_relics() {
    // Enemy killed, by an attack or by anything else.
    CombatEngine horn = make_relic_combat(CharacterId::IRONCLAD, PowerId::GREMLIN_HORN, 1, true);
    horn.enemies[0].hp = 5;
    play(horn, CardId::STRIKE, 0);
    assert(!horn.enemies[0].is_alive() && horn.player.energy == 3 - 1 + 1 && horn.hand.size() == 1);
    horn.enemies[1].hp = 3;
    horn.enemies[1].add_status(StatusType::POISON, 3);
    end_turn_idle(horn); // Poison kills it during the enemy turn: the card is drawn on top of the new hand
    assert(!horn.enemies[1].is_alive() && horn.hand.size() == 6);

    // Razor Tooth upgrades what comes back to the piles; Powers never come back.
    CombatEngine tooth = make_relic_combat(CharacterId::IRONCLAD, PowerId::RAZOR_TOOTH);
    play(tooth, CardId::STRIKE);
    assert(tooth.enemies[0].hp == 42 - 6 && tooth.discard_pile.back() == CardId::STRIKE && tooth.discard_pile.back().upgraded);
    play(tooth, CardId::IMPERVIOUS);
    assert(tooth.exhaust_pile.back().upgraded && tooth.player.block == 30);

    // The Abacus: Block when the discard pile becomes the draw pile.
    CombatEngine abacus = make_relic_combat(CharacterId::IRONCLAD, PowerId::THE_ABACUS, 6);
    abacus.draw_pile.clear();
    abacus.draw_cards(1);
    assert(abacus.player.block == 0); // nothing to shuffle
    abacus.discard_pile.push_back(CardId::STRIKE);
    abacus.draw_cards(1);
    assert(abacus.player.block == 6);

    // Powers played.
    CombatEngine frost = make_relic_combat(CharacterId::IRONCLAD, PowerId::PERMAFROST, 7);
    frost.add_power(PowerId::GAME_PIECE, 1);
    play(frost, CardId::INFLAME);
    assert(frost.player.block == 7 && frost.hand.size() == 1 && frost.power_counter(PowerId::PERMAFROST) == 1);
    play(frost, CardId::INFLAME);
    assert(frost.player.block == 7 && frost.hand.size() == 1);

    // First HP loss of the combat.
    CombatEngine puzzle = make_relic_combat(CharacterId::IRONCLAD, PowerId::CENTENNIAL_PUZZLE, 3);
    puzzle.hand.clear();
    puzzle.lose_hp(1);
    assert(puzzle.hand.size() == 3);
    puzzle.lose_hp(1);
    assert(puzzle.hand.size() == 3);

    // Lizard Tail: once.
    CombatEngine tail = make_relic_combat(CharacterId::IRONCLAD, PowerId::LIZARD_TAIL, 50);
    tail.player.hp = 5;
    tail.lose_hp(10);
    assert(tail.player.hp == 40 && tail.state == CombatState::PLAYER_TURN && tail.power_counter(PowerId::LIZARD_TAIL) == 1);
    tail.lose_hp(60);
    assert(tail.player.hp == 0 && tail.state == CombatState::DEFEAT);

    // Unceasing Top: a card when the hand is empty.
    CombatEngine top = make_relic_combat(CharacterId::IRONCLAD, PowerId::UNCEASING_TOP);
    play(top, CardId::STRIKE);
    assert(top.hand.size() == 1);

    // Cards discarded during the turn (the end of turn discard does not count).
    CombatEngine tingsha = make_relic_combat(CharacterId::SILENT, PowerId::TINGSHA, 3);
    tingsha.add_power(PowerId::TOUGH_BANDAGES, 3);
    tingsha.hand.clear();
    tingsha.hand.push_back(CardId::CALCULATED_GAMBLE);
    tingsha.hand.push_back(CardId::STRIKE);
    tingsha.hand.push_back(CardId::DEFEND);
    assert(tingsha.play_card(0));
    assert(tingsha.enemies[0].hp == 42 - 6 && tingsha.player.block == 6);
    end_turn_idle(tingsha);
    assert(tingsha.enemies[0].hp == 36);
    std::cout << "[PASS] Event relics: Gremlin Horn, Razor Tooth, The Abacus, Permafrost, Game Piece, Centennial Puzzle, Lizard Tail, Unceasing Top, Tingsha, Tough Bandages" << std::endl;

    // RunManager registers the relics before the first turn.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    for (std::string_view relic : { "BREAD", "MERCURY_HOURGLASS", "BELLOWS", "LIZARD_TAIL" }) run.add_relic(relic);
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.combat.has_power(PowerId::BREAD) && run.combat.has_power(PowerId::LIZARD_TAIL));
    assert(!run.combat.has_power(PowerId::HAPPY_FLOWER) && run.combat.powers_in_play.empty());
    assert(run.combat.player.energy == 1); // Bread acted on the first turn
    for (const Enemy& foe : run.combat.enemies) assert(foe.hp == foe.max_hp - 3);
    for (const DeckCard& card : run.combat.hand) assert(card.upgraded); // Bellows
    run.combat.lose_hp(run.combat.player.hp);
    assert(run.combat.player.hp == run.max_hp / 2);
    run.proceed_after_combat();
    assert(!run.has_relic("LIZARD_TAIL") && run.has_relic("BREAD"));
    std::cout << "[PASS] Relics in a run: registered before the first turn, Bellows, Lizard Tail leaves once used" << std::endl;
}

void test_orb_operations() {
    // Lightning picks a random living enemy: with one left, it is that one.
    CombatEngine random = make_pack(CharacterId::DEFECT);
    random.enemies[0].hp = 0;
    random.enemies[1].hp = 0;
    random.channel_orb(OrbType::LIGHTNING);
    random.trigger_orb_passive(0);
    assert(random.enemies[2].hp == 42 - 3);
    random.evoke_orb(0);
    assert(random.enemies[2].hp == 39 - 8 && random.player.orbs.empty());
    assert(random.counters.combat_orbs_channeled == 1 && random.counters.combat_lightning_channeled == 1);

    // Each Dark orb keeps its own charge and hits the weakest enemy.
    CombatEngine dark = make_pack(CharacterId::DEFECT);
    play(dark, DeckCard(CardId::DARKNESS, true));
    assert(dark.player.orbs.size() == 1 && dark.player.orbs[0].charge == 6 + 2 * 6);
    dark.channel_orb(OrbType::DARK);
    assert(dark.player.orbs[1].charge == 6);
    dark.enemies[1].hp = 30;
    dark.enemies[0].hp = 25;
    dark.evoke_orb(0);
    assert(dark.enemies[0].hp == 25 - 18 && dark.enemies[1].hp == 30 && dark.enemies[2].hp == 42);

    // Focus is added to passives and evokes, and can make them worthless but never negative.
    CombatEngine focus = make_combat(CharacterId::DEFECT);
    make_enemy_idle(focus.enemies[0]);
    play(focus, DeckCard(CardId::DEFRAGMENT, true));
    assert(focus.player.get_status(StatusType::FOCUS) == 2 && focus.powers.empty());
    focus.channel_orb(OrbType::LIGHTNING);
    focus.channel_orb(OrbType::FROST);
    focus.channel_orb(OrbType::DARK);
    focus.end_player_turn();
    assert(focus.enemies[0].hp == 42 - 5 && focus.player.orbs[2].charge == 6 + 8);
    focus.player.block = 0;
    focus.evoke_orb(1);
    assert(focus.player.block == 7);
    focus.evoke_orb(0);
    assert(focus.enemies[0].hp == 37 - 10);
    focus.player.add_status(StatusType::FOCUS, -9);
    focus.channel_orb(OrbType::LIGHTNING);
    focus.trigger_orb_passive(1);
    assert(focus.enemies[0].hp == 27);

    // Focus "this turn" still counts for the passives of the end of the turn.
    CombatEngine hotfix = make_combat(CharacterId::DEFECT);
    make_enemy_idle(hotfix.enemies[0]);
    hotfix.channel_orb(OrbType::LIGHTNING);
    play(hotfix, CardId::HOTFIX);
    assert(hotfix.player.get_status(StatusType::FOCUS) == 2 && hotfix.exhaust_pile.size() == 1);
    play(hotfix, DeckCard(CardId::FOCUSED_STRIKE, true));
    assert(hotfix.enemies[0].hp == 42 - 11 && hotfix.player.get_status(StatusType::FOCUS) == 4);
    hotfix.end_player_turn();
    assert(hotfix.enemies[0].hp == 31 - 7 && hotfix.player.get_status(StatusType::FOCUS) == 0);
    play(hotfix, DeckCard(CardId::HOTFIX, true));
    assert(hotfix.exhaust_pile.size() == 1);

    CombatEngine beam = make_pack(CharacterId::DEFECT);
    play(beam, DeckCard(CardId::BIASED_COGNITION, true));
    assert(beam.player.get_status(StatusType::FOCUS) == 6);
    for (Enemy& foe : beam.enemies) foe.hp = 100;
    play(beam, CardId::HYPERBEAM);
    for (const Enemy& foe : beam.enemies) assert(foe.hp == 76);
    assert(beam.player.get_status(StatusType::FOCUS) == 3);
    end_turn_idle(beam);
    assert(beam.player.get_status(StatusType::FOCUS) == 2);
    std::cout << "[PASS] Orbs: random Lightning, Dark charge per orb, Focus, Defragment, Hotfix, Focused Strike, Biased Cognition, Hyperbeam" << std::endl;

    // Evoking: all the orbs, the oldest several times.
    CombatEngine shatter = make_pack(CharacterId::DEFECT);
    for (Enemy& foe : shatter.enemies) foe.hp = 100;
    shatter.enemies[0].hp = 0;
    shatter.enemies[1].hp = 0;
    shatter.channel_orb(OrbType::LIGHTNING);
    shatter.channel_orb(OrbType::FROST);
    shatter.player.block = 0;
    play(shatter, CardId::SHATTER);
    assert(shatter.enemies[2].hp == 100 - 7 - 2 * 8 && shatter.player.block == 10 && shatter.player.orbs.empty());
    assert(shatter.exhaust_pile.size() == 1);

    CombatEngine quad = make_combat(CharacterId::DEFECT);
    quad.channel_orb(OrbType::FROST);
    quad.channel_orb(OrbType::LIGHTNING);
    quad.player.block = 0;
    play(quad, DeckCard(CardId::QUADCAST, true));
    assert(quad.player.block == 20 && quad.player.energy == 3 && quad.player.orbs.size() == 1);
    play(quad, DeckCard(CardId::MULTI_CAST, true));
    assert(quad.enemies[0].hp == 42 - 4 * 8 && quad.player.orbs.empty() && quad.player.energy == 0);
    quad.channel_orb(OrbType::PLASMA);
    quad.hand.clear();
    quad.hand.push_back(CardId::MULTI_CAST);
    assert(quad.play_card(0)); // X = 0: nothing is evoked, the orb stays
    assert(quad.player.orbs.size() == 1 && quad.player.energy == 0);

    // Counting orbs.
    CombatEngine barrage = make_combat(CharacterId::DEFECT);
    barrage.channel_orb(OrbType::FROST);
    barrage.channel_orb(OrbType::FROST);
    barrage.channel_orb(OrbType::DARK);
    assert(barrage.unique_orbs() == 2);
    play(barrage, DeckCard(CardId::BARRAGE, true));
    assert(barrage.enemies[0].hp == 42 - 3 * 7);
    pad_draw_pile(barrage, 10);
    play(barrage, CardId::COMPILE_DRIVER);
    assert(barrage.enemies[0].hp == 21 - 7 && barrage.hand.size() == 2);

    // Voltaic channels again as many Lightning as the combat already saw.
    CombatEngine voltaic = make_combat(CharacterId::DEFECT);
    voltaic.channel_orb(OrbType::LIGHTNING);
    voltaic.channel_orb(OrbType::LIGHTNING);
    voltaic.player.orbs.clear();
    play(voltaic, CardId::VOLTAIC);
    assert(voltaic.player.orbs.size() == 2 && voltaic.counters.combat_lightning_channeled == 4);
    assert(voltaic.exhaust_pile.size() == 1 && !CardDef::has_keyword(CardId::VOLTAIC, true, KEYWORD_EXHAUST));

    // Tesla Coil points the Lightning passives at its target.
    CombatEngine tesla = make_pack(CharacterId::DEFECT);
    tesla.channel_orb(OrbType::LIGHTNING);
    tesla.channel_orb(OrbType::FROST);
    tesla.channel_orb(OrbType::LIGHTNING);
    play(tesla, DeckCard(CardId::TESLA_COIL, true), 2);
    assert(tesla.enemies[2].hp == 42 - 4 - 4 * 3 && tesla.enemies[0].hp == 18 && tesla.enemies[1].hp == 18);

    CombatEngine chaos = make_combat(CharacterId::DEFECT);
    play(chaos, DeckCard(CardId::CHAOS, true));
    assert(chaos.player.orbs.size() == 2 && chaos.player.orbs[0].type != OrbType::NONE);
    std::cout << "[PASS] Orb cards: Shatter, Quadcast, Multi-Cast, Barrage, Compile Driver, Voltaic, Tesla Coil, Chaos, Darkness" << std::endl;

    // Powers on orbs.
    CombatEngine loop = make_combat(CharacterId::DEFECT);
    loop.channel_orb(OrbType::PLASMA);
    loop.channel_orb(OrbType::FROST);
    play(loop, DeckCard(CardId::LOOP, true));
    play(loop, CardId::COOLANT);
    end_turn_idle(loop);
    // Frost passive at the end of the turn is gone with the turn; Plasma +1, Loop +2 on the oldest orb, Coolant 2 x 2.
    assert(loop.player.energy == 3 + 1 + 2 && loop.player.block == 4);

    CombatEngine shadow = make_combat(CharacterId::DEFECT);
    shadow.enemies[0].hp = 100;
    play(shadow, CardId::CONSUMING_SHADOW);
    assert(shadow.player.orbs.size() == 2);
    end_turn_idle(shadow);
    // The newest Dark orb is evoked with its starting charge; the other one charges.
    assert(shadow.enemies[0].hp == 100 - 6 && shadow.player.orbs.size() == 1 && shadow.player.orbs[0].charge == 12);

    CombatEngine hail = make_pack(CharacterId::DEFECT);
    play(hail, DeckCard(CardId::HAILSTORM, true));
    end_turn_idle(hail);
    assert(hail.enemies[2].hp == 42);
    hail.channel_orb(OrbType::FROST);
    end_turn_idle(hail);
    assert(hail.enemies[0].hp == 10 && hail.enemies[2].hp == 34);

    CombatEngine thunder = make_combat(CharacterId::DEFECT);
    play(thunder, CardId::THUNDER);
    thunder.channel_orb(OrbType::LIGHTNING);
    thunder.trigger_orb_passive(0);
    assert(thunder.enemies[0].hp == 42 - 3);
    thunder.evoke_orb(0);
    assert(thunder.enemies[0].hp == 39 - 8 - 8);

    assert_in_pool(CharacterId::DEFECT, { CardId::SHATTER, CardId::VOLTAIC, CardId::BARRAGE, CardId::TESLA_COIL, CardId::CHAOS,
                                          CardId::COMPILE_DRIVER, CardId::DARKNESS, CardId::MULTI_CAST, CardId::LOOP,
                                          CardId::CONSUMING_SHADOW, CardId::HAILSTORM, CardId::THUNDER, CardId::COOLANT,
                                          CardId::HOTFIX, CardId::DEFRAGMENT, CardId::FOCUSED_STRIKE, CardId::HYPERBEAM });
    std::span<const CardId> pool = RunManager::card_reward_pool(CharacterId::DEFECT);
    for (CardId ancient : { CardId::QUADCAST, CardId::BIASED_COGNITION }) {
        assert(std::find(pool.begin(), pool.end(), ancient) == pool.end());
    }
    std::cout << "[PASS] Orb powers: Loop, Coolant, Consuming Shadow, Hailstorm, Thunder" << std::endl;
}

void test_doom() {
    // An enemy with at least as much Doom as HP dies at the end of its turn, after acting.
    CombatEngine doom = make_combat(CharacterId::NECROBINDER);
    play(doom, CardId::SCOURGE);
    assert(doom.enemies[0].get_status(StatusType::DOOM) == 13 && doom.hand.size() == 1);
    assert(doom.counters.doom_applied == 1);
    doom.enemies[0].hp = 13;
    assert(doom.enemies[0].is_alive());
    int16_t hp_before = doom.player.hp;
    doom.end_player_turn();
    assert(doom.player.hp == hp_before - 11); // it still attacked
    assert(!doom.enemies[0].is_alive() && doom.state == CombatState::VICTORY);

    // Below its HP, Doom waits.
    CombatEngine waits = make_combat(CharacterId::NECROBINDER);
    play(waits, DeckCard(CardId::SCOURGE, true));
    end_turn_idle(waits);
    assert(waits.enemies[0].hp == 42 && waits.enemies[0].get_status(StatusType::DOOM) == 16);

    // On the player too, at the end of the player's turn.
    CombatEngine self = make_combat(CharacterId::NECROBINDER);
    pad_draw_pile(self, 10);
    play(self, DeckCard(CardId::NEUROSURGE, true));
    assert(self.player.energy == 3 + 4 && self.hand.size() == 2);
    end_turn_idle(self);
    assert(self.player.get_status(StatusType::DOOM) == 3 && self.state == CombatState::PLAYER_TURN);
    self.player.hp = 3;
    end_turn_idle(self);
    assert(self.state == CombatState::DEFEAT && self.player.hp == 0);
    std::cout << "[PASS] Doom: death at the end of the turn, enemy and player, Scourge, Neurosurge" << std::endl;

    CombatEngine pulse = make_pack(CharacterId::NECROBINDER);
    play(pulse, DeckCard(CardId::NEGATIVE_PULSE, true));
    assert(pulse.player.block == 6);
    for (const Enemy& foe : pulse.enemies) assert(foe.get_status(StatusType::DOOM) == 11);
    play(pulse, CardId::DEATHBRINGER);
    for (const Enemy& foe : pulse.enemies) {
        assert(foe.get_status(StatusType::DOOM) == 32 && foe.get_status(StatusType::WEAK) == 1);
    }
    // End of Days does not wait for the end of the turn.
    play(pulse, CardId::END_OF_DAYS);
    assert(!pulse.enemies[0].is_alive() && !pulse.enemies[1].is_alive() && !pulse.enemies[2].is_alive());
    assert(pulse.state == CombatState::VICTORY);

    // Blight Strike: as much Doom as HP taken. Time's Up turns the Doom into damage.
    CombatEngine blight = make_combat(CharacterId::NECROBINDER);
    blight.enemies[0].block = 3;
    play(blight, CardId::BLIGHT_STRIKE);
    assert(blight.enemies[0].hp == 42 - 5 && blight.enemies[0].get_status(StatusType::DOOM) == 5);
    play(blight, CardId::NO_ESCAPE);
    assert(blight.enemies[0].get_status(StatusType::DOOM) == 15);
    play(blight, DeckCard(CardId::NO_ESCAPE, true));
    assert(blight.enemies[0].get_status(StatusType::DOOM) == 15 + 15 + 5);
    play(blight, CardId::TIMES_UP);
    assert(blight.enemies[0].hp == 37 - 35);
    assert(CardDef::has_keyword(CardId::TIMES_UP, true, KEYWORD_RETAIN));

    // Death's Door: three times the Block once Doom was applied this turn.
    CombatEngine door = make_combat(CharacterId::NECROBINDER);
    play(door, CardId::DEATHS_DOOR);
    assert(door.player.block == 6);
    play(door, CardId::SCOURGE);
    door.player.block = 0;
    play(door, DeckCard(CardId::DEATHS_DOOR, true));
    assert(door.player.block == 21);
    end_turn_idle(door);
    play(door, CardId::DEATHS_DOOR);
    assert(door.player.block == 6);

    // Powers: Countdown, Reaper Form, Oblivion, Shroud.
    CombatEngine countdown = make_combat(CharacterId::NECROBINDER);
    play(countdown, DeckCard(CardId::COUNTDOWN, true));
    play(countdown, CardId::SHROUD);
    end_turn_idle(countdown);
    assert(countdown.enemies[0].get_status(StatusType::DOOM) == 9 && countdown.player.block == 3);

    CombatEngine reaper = make_combat(CharacterId::NECROBINDER);
    play(reaper, CardId::REAPER_FORM);
    reaper.enemies[0].block = 2;
    play(reaper, CardId::STRIKE);
    assert(reaper.enemies[0].hp == 42 - 4 && reaper.enemies[0].get_status(StatusType::DOOM) == 4);
    assert(CardDef::has_keyword(CardId::REAPER_FORM, true, KEYWORD_RETAIN));

    CombatEngine oblivion = make_pack(CharacterId::NECROBINDER);
    play(oblivion, DeckCard(CardId::OBLIVION, true), 2);
    assert(oblivion.enemies[2].get_status(StatusType::DOOM) == 0);
    play(oblivion, CardId::DEFEND);
    play(oblivion, CardId::DEFEND);
    assert(oblivion.enemies[2].get_status(StatusType::DOOM) == 8 && oblivion.enemies[0].get_status(StatusType::DOOM) == 0);
    end_turn_idle(oblivion);
    play(oblivion, CardId::DEFEND);
    assert(oblivion.enemies[2].get_status(StatusType::DOOM) == 8);

    assert_in_pool(CharacterId::NECROBINDER, { CardId::NEGATIVE_PULSE, CardId::END_OF_DAYS, CardId::BLIGHT_STRIKE,
                                               CardId::NO_ESCAPE, CardId::SCOURGE, CardId::DEATHBRINGER, CardId::DEATHS_DOOR,
                                               CardId::COUNTDOWN, CardId::REAPER_FORM, CardId::NEUROSURGE, CardId::TIMES_UP,
                                               CardId::OBLIVION, CardId::SHROUD });
    std::cout << "[PASS] Doom cards: Negative Pulse, Deathbringer, End of Days, Blight Strike, No Escape, Time's Up, Death's Door, Countdown, Shroud, Reaper Form, Oblivion" << std::endl;
}

void test_orb_slots() {
    // Capacitor: more slots, so more orbs before the oldest is evoked.
    CombatEngine capacitor = make_combat(CharacterId::DEFECT);
    assert(capacitor.player.orb_slots == 3);
    play(capacitor, CardId::CAPACITOR);
    assert(capacitor.player.orb_slots == 5);
    for (int i = 0; i < 5; ++i) capacitor.channel_orb(OrbType::LIGHTNING);
    assert(capacitor.player.orbs.size() == 5 && capacitor.enemies[0].hp == 42);
    capacitor.channel_orb(OrbType::FROST);
    assert(capacitor.player.orbs.size() == 5 && capacitor.enemies[0].hp == 34);

    // Bulk Up: one slot less; the orb that no longer fits leaves without being evoked.
    play(capacitor, DeckCard(CardId::BULK_UP, true));
    assert(capacitor.player.orb_slots == 4 && capacitor.player.orbs.size() == 4);
    assert(capacitor.player.block == 0 && capacitor.player.orbs[3] == OrbType::LIGHTNING);
    assert(capacitor.player.get_status(StatusType::STRENGTH) == 3 && capacitor.player.get_status(StatusType::DEXTERITY) == 3);

    // No slot left: nothing can be channeled. The count never goes below 0 nor above the maximum.
    capacitor.change_orb_slots(-10);
    assert(capacitor.player.orb_slots == 0 && capacitor.player.orbs.empty());
    capacitor.channel_orb(OrbType::DARK);
    assert(capacitor.player.orbs.empty());
    capacitor.change_orb_slots(50);
    assert(capacitor.player.orb_slots == MAX_ORB_SLOTS);

    // A new combat starts with 3 slots again.
    capacitor.start_combat(EnemyType::JAW_WORM, CharacterId::DEFECT);
    assert(capacitor.player.orb_slots == 3);

    // Synchronize: Focus for the turn, for each different orb.
    CombatEngine sync = make_combat(CharacterId::DEFECT);
    sync.channel_orb(OrbType::LIGHTNING);
    sync.channel_orb(OrbType::LIGHTNING);
    sync.channel_orb(OrbType::FROST);
    play(sync, DeckCard(CardId::SYNCHRONIZE, true));
    assert(sync.player.get_status(StatusType::FOCUS) == 4);
    end_turn_idle(sync);
    assert(sync.player.get_status(StatusType::FOCUS) == 0);
    assert_in_pool(CharacterId::DEFECT, { CardId::CAPACITOR, CardId::BULK_UP, CardId::SYNCHRONIZE });
    std::cout << "[PASS] Orb slots: Capacitor, Bulk Up, limits, Synchronize" << std::endl;
}

void test_hero_relics() {
    // Starter relics of the Regent and of the Necrobinder.
    RunManager regent;
    regent.start_run(CharacterId::REGENT, 0, PaletteVariant::CLASSIC, 777);
    assert(regent.has_relic("DIVINE_RIGHT"));
    enter_room_of_type(regent, RoomType::MONSTER);
    assert(regent.combat.player.sovereign_stars == 3);

    RunManager necro;
    necro.start_run(CharacterId::NECROBINDER, 0, PaletteVariant::CLASSIC, 777);
    assert(necro.has_relic("BOUND_PHYLACTERY"));
    enter_room_of_type(necro, RoomType::MONSTER);
    assert(necro.combat.player.minion_hp == 1);
    end_turn_idle(necro.combat);
    assert(necro.combat.player.minion_hp == 2);

    // Defect relics that act when the fight starts.
    RunManager defect;
    defect.start_run(CharacterId::DEFECT, 0, PaletteVariant::CLASSIC, 777);
    for (std::string_view relic : { "DATA_DISK", "SYMBIOTIC_VIRUS", "RUNIC_CAPACITOR", "POWER_CELL" }) defect.add_relic(relic);
    defect.master_deck.push_back(CardId::GO_FOR_THE_EYES);
    defect.master_deck.push_back(CardId::BEAM_CELL);
    enter_room_of_type(defect, RoomType::MONSTER);
    const Player& player = defect.combat.player;
    assert(player.get_status(StatusType::FOCUS) == 1 && player.orb_slots == 6);
    assert(player.orbs.size() == 2 && player.orbs[0] == OrbType::DARK && player.orbs[1] == OrbType::LIGHTNING);
    int free_cards = 0;
    for (const DeckCard& card : defect.combat.hand) free_cards += CardDef::get_cost(card.id, card.upgraded) == 0;
    assert(free_cards == 2 && defect.combat.hand.size() >= 5 && defect.combat.hand.size() <= 7);
    std::cout << "[PASS] Hero relics at combat start: Divine Right, Bound Phylactery, Data Disk, Symbiotic Virus, Runic Capacitor, Power Cell" << std::endl;

    // Gold-Plated Cables: the oldest orb's passive, once more.
    CombatEngine cables = make_relic_combat(CharacterId::DEFECT, PowerId::GOLD_PLATED_CABLES);
    cables.channel_orb(OrbType::LIGHTNING);
    cables.channel_orb(OrbType::FROST);
    end_turn_idle(cables);
    assert(cables.enemies[0].hp == 42 - 6);
    cables.player.orbs.clear();
    cables.channel_orb(OrbType::PLASMA);
    end_turn_idle(cables);
    assert(cables.player.energy == 5);

    // Metronome: the seventh orb of the combat, once.
    CombatEngine metronome = make_relic_combat(CharacterId::DEFECT, PowerId::METRONOME, 30);
    metronome.enemies[0].hp = 200;
    for (int i = 0; i < 6; ++i) metronome.channel_orb(OrbType::FROST);
    assert(metronome.enemies[0].hp == 200);
    metronome.channel_orb(OrbType::FROST);
    assert(metronome.enemies[0].hp == 170);
    metronome.channel_orb(OrbType::FROST);
    assert(metronome.enemies[0].hp == 170);

    // Emotion Chip: every passive again at the start of a turn that follows a HP loss.
    CombatEngine chip = make_relic_combat(CharacterId::DEFECT, PowerId::EMOTION_CHIP);
    chip.enemies[0].hp = 100;
    chip.channel_orb(OrbType::LIGHTNING);
    end_turn_idle(chip);
    assert(chip.enemies[0].hp == 97);
    chip.enemies[0].current_intent = EnemyIntent{};
    chip.enemies[0].current_intent.base_damage = 11;
    chip.end_player_turn(); // the hit hurts: the passive triggers at the end of the turn and at the start of the next
    assert(chip.player.hp == 75 - 11 && chip.enemies[0].hp == 91);

    // Book Repair Knife: HP back when Doom takes an enemy.
    CombatEngine knife = make_relic_combat(CharacterId::NECROBINDER, PowerId::BOOK_REPAIR_KNIFE, 3, true);
    knife.player.hp = 50;
    knife.enemies[0].add_status(StatusType::DOOM, 30);
    knife.enemies[1].add_status(StatusType::DOOM, 30);
    knife.enemies[1].minion = true;
    end_turn_idle(knife);
    assert(!knife.enemies[0].is_alive() && !knife.enemies[1].is_alive() && knife.player.hp == 53);
    std::cout << "[PASS] Hero relics in combat: Gold-Plated Cables, Metronome, Emotion Chip, Book Repair Knife" << std::endl;
}

// Puts `card` alone in hand with `energy` and plays it.
void play_with(CombatEngine& combat, DeckCard card, int energy, int target = -1) {
    combat.hand.clear();
    combat.hand.push_back(card);
    combat.player.energy = static_cast<int8_t>(energy);
    bool played = combat.play_card(0, target);
    assert(played);
}

int count_in(const auto& pile, CardId id) {
    int count = 0;
    for (const DeckCard& card : pile) count += card.id == id;
    return count;
}

void test_replays() {
    // One-Two Punch: the next Attack of the turn is played twice, and goes to the discard pile once.
    CombatEngine punch = make_combat(CharacterId::IRONCLAD);
    play(punch, CardId::ONE_TWO_PUNCH);
    play(punch, CardId::STRIKE);
    assert(punch.enemies[0].hp == 42 - 12 && count_in(punch.discard_pile, CardId::STRIKE) == 1);
    assert(punch.counters.attacks_played == 2 && !punch.has_power(PowerId::REPLAY_ATTACK));
    play(punch, CardId::STRIKE);
    assert(punch.enemies[0].hp == 24);
    // A Skill leaves it alone, and it is gone at the end of the turn.
    play(punch, DeckCard(CardId::ONE_TWO_PUNCH, true));
    play(punch, CardId::DEFEND);
    assert(punch.player.block == 5 && punch.power_amount(PowerId::REPLAY_ATTACK) == 2);
    end_turn_idle(punch);
    assert(!punch.has_power(PowerId::REPLAY_ATTACK));

    CombatEngine burst = make_combat(CharacterId::SILENT);
    play(burst, CardId::BURST);
    play(burst, CardId::DEFEND);
    assert(burst.player.block == 10 && !burst.has_power(PowerId::REPLAY_SKILL));
    play(burst, DeckCard(CardId::BURST, true));
    assert(burst.power_amount(PowerId::REPLAY_SKILL) == 2);

    // Echo Form: the first card of each turn, starting with the next one.
    CombatEngine echo = make_combat(CharacterId::DEFECT);
    assert(CardDef::has_keyword(CardId::ECHO_FORM, false, KEYWORD_ETHEREAL) && !CardDef::has_keyword(CardId::ECHO_FORM, true, KEYWORD_ETHEREAL));
    play(echo, CardId::ECHO_FORM);
    play(echo, CardId::STRIKE);
    assert(echo.enemies[0].hp == 36);
    end_turn_idle(echo);
    play(echo, CardId::STRIKE);
    assert(echo.enemies[0].hp == 24);
    play(echo, CardId::STRIKE);
    assert(echo.enemies[0].hp == 18);

    // Signal Boost: the next Power is gained twice, and stays in play once.
    CombatEngine boost = make_combat(CharacterId::DEFECT);
    play(boost, CardId::SIGNAL_BOOST);
    assert(count_in(boost.exhaust_pile, CardId::SIGNAL_BOOST) == 1);
    play(boost, CardId::DEFRAGMENT);
    assert(boost.player.get_status(StatusType::FOCUS) == 2 && boost.powers_in_play.size() == 1);
    assert(!boost.has_power(PowerId::REPLAY_POWER) && CardDef::get_cost(CardId::SIGNAL_BOOST, true) == 0);

    // Havoc: the top card of the draw pile is played for free and exhausted.
    CombatEngine havoc = make_combat(CharacterId::IRONCLAD);
    havoc.draw_pile.push_back(CardId::BASH);
    play(havoc, CardId::HAVOC);
    assert(havoc.enemies[0].hp == 34 && havoc.enemies[0].get_status(StatusType::VULNERABLE) == 2);
    assert(havoc.player.energy == 2 && count_in(havoc.exhaust_pile, CardId::BASH) == 1);
    assert(count_in(havoc.discard_pile, CardId::HAVOC) == 1 && CardDef::get_cost(CardId::HAVOC, true) == 0);
    // Nothing to play: nothing happens.
    havoc.draw_pile.clear();
    havoc.discard_pile.clear();
    play(havoc, CardId::HAVOC);
    assert(havoc.enemies[0].hp == 34);

    // Cascade: X cards from the top of the draw pile, X + 1 once upgraded.
    CombatEngine cascade = make_combat(CharacterId::IRONCLAD);
    cascade.draw_pile.push_back(CardId::DEFEND);
    cascade.draw_pile.push_back(CardId::STRIKE);
    cascade.draw_pile.push_back(CardId::STRIKE);
    play_with(cascade, CardId::CASCADE, 2);
    assert(cascade.enemies[0].hp == 30 && cascade.player.block == 0 && cascade.player.energy == 0);
    assert(count_in(cascade.discard_pile, CardId::STRIKE) == 2);
    play_with(cascade, DeckCard(CardId::CASCADE, true), 0);
    assert(cascade.player.block == 5);

    // Uproar: two hits, then a random Attack of the draw pile.
    CombatEngine uproar = make_combat(CharacterId::DEFECT);
    uproar.draw_pile.clear();
    uproar.draw_pile.push_back(CardId::DEFEND);
    uproar.draw_pile.push_back(CardId::STRIKE);
    uproar.draw_pile.push_back(CardId::DEFEND);
    play(uproar, CardId::UPROAR);
    assert(uproar.enemies[0].hp == 42 - 12 - 6 && uproar.draw_pile.size() == 2);
    assert(count_in(uproar.discard_pile, CardId::STRIKE) == 1 && uproar.player.block == 0);

    // Tear Asunder: one more hit for each time HP was lost in the combat.
    CombatEngine tear = make_combat(CharacterId::IRONCLAD);
    tear.counters.combat_hp_losses = 2;
    play(tear, CardId::TEAR_ASUNDER);
    assert(tear.enemies[0].hp == 42 - 15 && tear.counters.hits_dealt == 3);

    // Hellraiser: a Strike is played as soon as it is drawn; other cards are not.
    CombatEngine hell = make_combat(CharacterId::IRONCLAD);
    play(hell, CardId::HELLRAISER);
    hell.draw_pile.push_back(CardId::DEFEND);
    hell.draw_pile.push_back(CardId::TWIN_STRIKE);
    assert(hell.draw_card() && hell.enemies[0].hp == 42 - 10);
    assert(hell.hand.empty() && count_in(hell.discard_pile, CardId::TWIN_STRIKE) == 1 && hell.player.energy == 1);
    assert(hell.draw_card() && hell.hand.size() == 1 && hell.player.block == 0);
    assert(CardDef::get_cost(CardId::HELLRAISER, true) == 1);

    // Stampede: a random Attack of the hand at the end of the turn.
    CombatEngine stampede = make_combat(CharacterId::IRONCLAD);
    play(stampede, CardId::STAMPEDE);
    stampede.hand.push_back(CardId::DEFEND);
    stampede.hand.push_back(CardId::STRIKE);
    end_turn_idle(stampede);
    assert(stampede.enemies[0].hp == 36 && count_in(stampede.discard_pile, CardId::STRIKE) == 1);

    // Howl from Beyond: played from the exhaust pile at the end of each turn, where it stays.
    CombatEngine howl = make_combat(CharacterId::IRONCLAD);
    play(howl, CardId::HOWL_FROM_BEYOND);
    assert(howl.enemies[0].hp == 24 && count_in(howl.discard_pile, CardId::HOWL_FROM_BEYOND) == 1);
    howl.exhaust_pile.push_back(CardId::HOWL_FROM_BEYOND);
    end_turn_idle(howl);
    assert(howl.enemies[0].hp == 6 && count_in(howl.exhaust_pile, CardId::HOWL_FROM_BEYOND) == 1);
    end_turn_idle(howl);
    assert(howl.state == CombatState::VICTORY);

    // Bombardment: exhausted when played, then played again at the start of each turn.
    CombatEngine bombardment = make_combat(CharacterId::REGENT);
    play(bombardment, CardId::BOMBARDMENT);
    assert(bombardment.enemies[0].hp == 24 && count_in(bombardment.exhaust_pile, CardId::BOMBARDMENT) == 1);
    end_turn_idle(bombardment);
    assert(bombardment.enemies[0].hp == 6 && count_in(bombardment.exhaust_pile, CardId::BOMBARDMENT) == 1);
    assert(bombardment.player.energy == 3);
    end_turn_idle(bombardment);
    assert(bombardment.state == CombatState::VICTORY);

    // I Am Invincible: played from the top of the draw pile at the end of the turn, before the enemy acts.
    CombatEngine invincible = make_combat(CharacterId::REGENT);
    invincible.draw_pile.push_back(CardId::I_AM_INVINCIBLE);
    int16_t hp_before = invincible.player.hp;
    invincible.end_player_turn();
    assert(invincible.player.hp == hp_before - 1 && count_in(invincible.discard_pile, CardId::I_AM_INVINCIBLE) == 1);
    play(invincible, DeckCard(CardId::I_AM_INVINCIBLE, true));
    assert(invincible.player.block == 13);

    // Eidolon: the Ethereal cards of the exhaust pile are played and stay there.
    CombatEngine eidolon = make_combat(CharacterId::NECROBINDER);
    eidolon.exhaust_pile.push_back(CardId::DEFILE);
    eidolon.exhaust_pile.push_back(CardId::STRIKE);
    play(eidolon, CardId::EIDOLON);
    assert(eidolon.enemies[0].hp == 42 - 13 && eidolon.exhaust_pile.size() == 3);
    assert(eidolon.counters.combat_ethereal_played == 1 && CardDef::get_cost(CardId::EIDOLON, true) == 1);

    assert_in_pool(CharacterId::IRONCLAD, { CardId::HOWL_FROM_BEYOND, CardId::HELLRAISER, CardId::STAMPEDE, CardId::CASCADE,
                                            CardId::ONE_TWO_PUNCH, CardId::TEAR_ASUNDER, CardId::HAVOC });
    assert_in_pool(CharacterId::SILENT, { CardId::BURST });
    assert_in_pool(CharacterId::DEFECT, { CardId::ECHO_FORM, CardId::SIGNAL_BOOST, CardId::UPROAR });
    assert_in_pool(CharacterId::REGENT, { CardId::BOMBARDMENT, CardId::I_AM_INVINCIBLE });
    assert_in_pool(CharacterId::NECROBINDER, { CardId::EIDOLON });
    std::cout << "[PASS] Replays: One-Two Punch, Burst, Echo Form, Signal Boost, Havoc, Cascade, Uproar, Tear Asunder, Hellraiser, Stampede, Howl from Beyond, Bombardment, I Am Invincible, Eidolon" << std::endl;
}

void test_cost_changes() {
    // Corruption: Skills are free and exhausted; Attacks are not.
    CombatEngine corruption = make_combat(CharacterId::IRONCLAD);
    play(corruption, CardId::CORRUPTION);
    assert(corruption.player.energy == 0 && CardDef::get_cost(CardId::CORRUPTION, true) == 2);
    corruption.hand.push_back(CardId::DEFEND);
    corruption.hand.push_back(CardId::STRIKE);
    assert(corruption.effective_cost(corruption.hand[0]) == 0 && corruption.effective_cost(corruption.hand[1]) == 1);
    assert(!corruption.can_play_card(1) && corruption.play_card(0));
    assert(corruption.player.block == 5 && count_in(corruption.exhaust_pile, CardId::DEFEND) == 1);
    std::span<const CardId> ironclad_pool = RunManager::card_reward_pool(CharacterId::IRONCLAD);
    assert(std::find(ironclad_pool.begin(), ironclad_pool.end(), CardId::CORRUPTION) == ironclad_pool.end());

    // "The next ... costs 0": Unrelenting, Pounce, Synthesis, Veilpiercer.
    CombatEngine unrelenting = make_combat(CharacterId::IRONCLAD);
    play(unrelenting, CardId::UNRELENTING);
    assert(unrelenting.enemies[0].hp == 28 && unrelenting.player.energy == 1);
    unrelenting.hand.push_back(CardId::DEFEND);
    unrelenting.hand.push_back(CardId::BASH);
    unrelenting.hand.push_back(CardId::BASH);
    assert(unrelenting.effective_cost(unrelenting.hand[0]) == 1 && unrelenting.effective_cost(unrelenting.hand[1]) == 0);
    assert(unrelenting.play_card(1) && unrelenting.player.energy == 1);
    assert(!unrelenting.has_power(PowerId::FREE_ATTACK) && unrelenting.effective_cost(unrelenting.hand[1]) == 2);

    CombatEngine pounce = make_combat(CharacterId::SILENT);
    play(pounce, CardId::POUNCE);
    assert(pounce.enemies[0].hp == 28);
    pounce.hand.push_back(CardId::STRIKE);
    pounce.hand.push_back(CardId::DEFEND);
    assert(pounce.effective_cost(pounce.hand[0]) == 1 && pounce.effective_cost(pounce.hand[1]) == 0);

    CombatEngine synthesis = make_combat(CharacterId::DEFECT);
    play(synthesis, DeckCard(CardId::SYNTHESIS, true));
    assert(synthesis.enemies[0].hp == 22);
    synthesis.hand.push_back(CardId::ECHO_FORM);
    assert(synthesis.effective_cost(synthesis.hand[0]) == 0 && synthesis.play_card(0));
    assert(synthesis.player.energy == 1 && !synthesis.has_power(PowerId::FREE_POWER));

    CombatEngine veil = make_combat(CharacterId::NECROBINDER);
    play(veil, CardId::VEILPIERCER);
    assert(veil.enemies[0].hp == 32);
    veil.hand.push_back(CardId::STRIKE);
    veil.hand.push_back(CardId::DEFILE);
    assert(veil.effective_cost(veil.hand[0]) == 1 && veil.effective_cost(veil.hand[1]) == 0);
    assert(veil.play_card(0) && veil.has_power(PowerId::FREE_ETHEREAL)); // a card that is not Ethereal keeps it
    assert(veil.play_card(0) && !veil.has_power(PowerId::FREE_ETHEREAL) && veil.player.energy == 1);

    // Costs that go down with what happened: Midnight, Stomp, Pinpoint, Banshee's Cry.
    CombatEngine midnight = make_combat(CharacterId::IRONCLAD);
    midnight.enemies[0].hp = 200;
    midnight.hand.clear();
    midnight.hand.push_back(CardId::MIDNIGHT);
    assert(midnight.effective_cost(midnight.hand[0]) == 12 && midnight.play_blocker(0) == PlayBlocker::ENERGY);
    for (int i = 0; i < 10; ++i) midnight.exhaust_pile.push_back(CardId::DEFEND);
    assert(midnight.effective_cost(midnight.hand[0]) == 2 && midnight.play_card(0));
    assert(midnight.enemies[0].hp == 140 && midnight.player.energy == 1);
    for (int i = 0; i < 10; ++i) midnight.exhaust_pile.push_back(CardId::DEFEND);
    assert(midnight.effective_cost(DeckCard(CardId::MIDNIGHT, true)) == 0);

    CombatEngine stomp = make_pack(CharacterId::IRONCLAD);
    play(stomp, CardId::STRIKE, 2);
    play(stomp, CardId::STRIKE, 2);
    assert(stomp.effective_cost(CardId::STOMP) == 1);
    play_with(stomp, CardId::STOMP, 1);
    assert(stomp.enemies[0].hp == 6 && stomp.enemies[1].hp == 6 && stomp.enemies[2].hp == 18 && stomp.player.energy == 0);
    end_turn_idle(stomp);
    assert(stomp.effective_cost(CardId::STOMP) == 3);

    CombatEngine pinpoint = make_combat(CharacterId::SILENT);
    play(pinpoint, CardId::DEFEND);
    play(pinpoint, CardId::DEFEND);
    assert(pinpoint.effective_cost(CardId::PINPOINT) == 1);
    play_with(pinpoint, DeckCard(CardId::PINPOINT, true), 1);
    assert(pinpoint.enemies[0].hp == 42 - 19);

    CombatEngine banshee = make_pack(CharacterId::NECROBINDER);
    assert(banshee.effective_cost(CardId::BANSHEES_CRY) == 9 && banshee.effective_cost(DeckCard(CardId::BANSHEES_CRY, true)) == 7);
    banshee.counters.combat_ethereal_played = 3;
    play_with(banshee, CardId::BANSHEES_CRY, 3);
    assert(!banshee.enemies[0].is_alive() && !banshee.enemies[1].is_alive() && banshee.enemies[2].hp == 9);
    assert(banshee.player.energy == 0);

    // Bullet Time: the hand is free until the end of the turn, and nothing more is drawn.
    CombatEngine bullet = make_combat(CharacterId::SILENT);
    bullet.hand.clear();
    bullet.hand.push_back(CardId::BULLET_TIME);
    bullet.hand.push_back(CardId::PINPOINT);
    bullet.hand.push_back(CardId::DEFEND);
    assert(bullet.play_card(0) && bullet.player.energy == 0);
    assert(bullet.effective_cost(bullet.hand[0]) == 0 && bullet.effective_cost(bullet.hand[1]) == 0);
    assert(!bullet.draw_card() && bullet.play_card(0) && bullet.enemies[0].hp == 27);
    assert(!bullet.discard_pile.back().free_this_turn && CardDef::get_cost(CardId::BULLET_TIME, true) == 2);
    end_turn_idle(bullet);
    for (const DeckCard& card : bullet.discard_pile) assert(!card.free_this_turn);

    // Adaptive Strike: a copy that costs 0 goes to the discard pile.
    CombatEngine adaptive = make_combat(CharacterId::DEFECT);
    play(adaptive, CardId::ADAPTIVE_STRIKE);
    assert(adaptive.enemies[0].hp == 24 && count_in(adaptive.discard_pile, CardId::ADAPTIVE_STRIKE) == 2);
    int free_copies = 0;
    for (const DeckCard& card : adaptive.discard_pile) {
        free_copies += card.id == CardId::ADAPTIVE_STRIKE && adaptive.effective_cost(card) == 0;
    }
    assert(free_copies == 1);

    // Void Form: ends the turn it is played on, then the first two cards of each turn are free.
    CombatEngine void_form = make_combat(CharacterId::REGENT);
    assert(CardDef::has_keyword(CardId::VOID_FORM, false, KEYWORD_ETHEREAL) && !CardDef::has_keyword(CardId::VOID_FORM, true, KEYWORD_ETHEREAL));
    play(void_form, CardId::VOID_FORM);
    assert(void_form.end_turn_requested);
    void_form.hand.push_back(CardId::STRIKE);
    assert(!void_form.can_play_card(0) && void_form.play_blocker(0) == PlayBlocker::NOT_PLAYABLE);
    end_turn_idle(void_form);
    assert(!void_form.end_turn_requested);
    play(void_form, CardId::STRIKE);
    play(void_form, CardId::STRIKE);
    assert(void_form.player.energy == 3 && void_form.enemies[0].hp == 30);
    play(void_form, CardId::STRIKE);
    assert(void_form.player.energy == 2);
    end_turn_idle(void_form);
    assert(void_form.effective_cost(CardId::KINGLY_KICK) == 0);

    // Borrowed Time: more energy, and every card costs 1 more until the end of the turn.
    CombatEngine borrowed = make_combat(CharacterId::NECROBINDER);
    play(borrowed, CardId::BORROWED_TIME);
    assert(borrowed.player.energy == 3 - 1 + 4 && borrowed.effective_cost(CardId::STRIKE) == 2);
    end_turn_idle(borrowed);
    assert(borrowed.effective_cost(CardId::STRIKE) == 1);
    play(borrowed, DeckCard(CardId::BORROWED_TIME, true));
    assert(borrowed.player.energy == 3 - 1 + 6);

    assert_in_pool(CharacterId::IRONCLAD, { CardId::UNRELENTING, CardId::MIDNIGHT, CardId::STOMP });
    assert_in_pool(CharacterId::SILENT, { CardId::BULLET_TIME, CardId::PINPOINT, CardId::POUNCE });
    assert_in_pool(CharacterId::DEFECT, { CardId::ADAPTIVE_STRIKE, CardId::SYNTHESIS });
    assert_in_pool(CharacterId::REGENT, { CardId::VOID_FORM });
    assert_in_pool(CharacterId::NECROBINDER, { CardId::BANSHEES_CRY, CardId::VEILPIERCER, CardId::BORROWED_TIME });
    std::cout << "[PASS] Cost changes: Corruption, Unrelenting, Pounce, Synthesis, Veilpiercer, Midnight, Stomp, Pinpoint, Banshee's Cry, Bullet Time, Adaptive Strike, Void Form, Borrowed Time" << std::endl;
}

void test_evolving_cards() {
    // Rampage: the copy that was played hits harder each time.
    CombatEngine rampage = make_combat(CharacterId::IRONCLAD);
    rampage.enemies[0].hp = 200;
    play(rampage, CardId::RAMPAGE);
    assert(rampage.enemies[0].hp == 190 && rampage.discard_pile.back().bonus == 5);
    play(rampage, rampage.discard_pile.back());
    assert(rampage.enemies[0].hp == 175 && rampage.discard_pile.back().bonus == 10);
    play(rampage, DeckCard(CardId::RAMPAGE, true));
    assert(rampage.enemies[0].hp == 165 && rampage.discard_pile.back().bonus == 10);

    // Claw: every Claw of the combat grows, wherever it is.
    CombatEngine claw = make_combat(CharacterId::DEFECT);
    claw.draw_pile.push_back(CardId::CLAW);
    play(claw, CardId::CLAW);
    assert(claw.enemies[0].hp == 39 && claw.discard_pile.back().bonus == 2 && claw.draw_pile.back().bonus == 2);
    play(claw, claw.draw_pile.back());
    assert(claw.enemies[0].hp == 34 && claw.draw_pile.back().bonus == 4);

    // Momentum Strike costs 0 once played; Modded costs 1 more each time.
    CombatEngine momentum = make_combat(CharacterId::DEFECT);
    play(momentum, CardId::MOMENTUM_STRIKE);
    assert(momentum.enemies[0].hp == 31 && momentum.player.energy == 2);
    assert(momentum.effective_cost(momentum.discard_pile.back()) == 0);
    play(momentum, CardId::MODDED);
    assert(momentum.player.orb_slots == 4 && momentum.hand.size() == 1 && momentum.player.energy == 3);
    DeckCard modded = momentum.discard_pile.back();
    assert(modded.id == CardId::MODDED && momentum.effective_cost(modded) == 1);
    play(momentum, modded);
    assert(momentum.player.energy == 2 && momentum.effective_cost(momentum.discard_pile.back()) == 2);

    // Kingly Kick gets cheaper and Kingly Punch stronger each time they are drawn.
    CombatEngine kingly = make_combat(CharacterId::REGENT);
    kingly.hand.clear();
    kingly.draw_pile.push_back(CardId::KINGLY_PUNCH);
    kingly.draw_pile.push_back(CardId::KINGLY_KICK);
    assert(kingly.effective_cost(kingly.draw_pile.back()) == 4);
    assert(kingly.draw_card() && kingly.effective_cost(kingly.hand[0]) == 3);
    assert(kingly.draw_card() && kingly.hand[1].bonus == 4);
    kingly.player.energy = 4;
    assert(kingly.play_card(1) && kingly.enemies[0].hp == 42 - 12);
    assert(kingly.play_card(0) && kingly.enemies[0].hp == 30 - 27 && kingly.player.energy == 0);

    // Melancholy: 1 less each time someone dies.
    CombatEngine melancholy = make_pack(CharacterId::NECROBINDER);
    melancholy.discard_pile.push_back(CardId::MELANCHOLY);
    melancholy.enemies[0].hp = 1;
    play(melancholy, CardId::STRIKE, 0);
    assert(!melancholy.enemies[0].is_alive());
    DeckCard cheaper = melancholy.discard_pile[melancholy.discard_pile.size() - 2];
    assert(cheaper.id == CardId::MELANCHOLY && melancholy.effective_cost(cheaper) == 2);
    play_with(melancholy, DeckCard(CardId::MELANCHOLY, true), 3);
    assert(melancholy.player.block == 17);

    // Hang: each Hang doubles the next one on the same enemy.
    CombatEngine hang = make_pack(CharacterId::NECROBINDER);
    hang.enemies[2].hp = 200;
    play(hang, CardId::HANG, 2);
    play(hang, CardId::HANG, 2);
    play(hang, DeckCard(CardId::HANG, true), 2);
    assert(hang.enemies[2].hp == 200 - 10 - 20 - 52 && hang.enemies[2].hang_marks == 3);
    play(hang, CardId::HANG, 1);
    assert(hang.enemies[1].hp == 18 - 10);

    // Genetic Algorithm and The Scythe: what they gain stays on the card of the deck.
    RunManager run;
    run.start_run(CharacterId::DEFECT, 0, PaletteVariant::CLASSIC, 777);
    run.master_deck.push_back(CardId::GENETIC_ALGORITHM);
    run.master_deck.push_back(CardId::THE_SCYTHE);
    size_t deck_size = run.master_deck.size();
    enter_room_of_type(run, RoomType::MONSTER);
    auto take = [&run](CardId id) {
        for (const DeckCard& card : run.combat.hand) {
            if (card.id == id) return card;
        }
        for (const DeckCard& card : run.combat.draw_pile) {
            if (card.id == id) return card;
        }
        assert(false);
        return DeckCard();
    };
    DeckCard algorithm = take(CardId::GENETIC_ALGORITHM);
    DeckCard scythe = take(CardId::THE_SCYTHE);
    assert(algorithm.deck_index == deck_size - 2 && scythe.deck_index == deck_size - 1);
    run.combat.enemies[0].hp = 200;
    int16_t block_before = run.combat.player.block;
    play(run.combat, algorithm);
    assert(run.combat.player.block == block_before + 1 && run.combat.exhaust_pile.back().bonus == 3);
    int16_t foe_hp = run.combat.enemies[0].hp;
    play(run.combat, scythe);
    assert(run.combat.enemies[0].hp == foe_hp - 13);
    // A copy made during the fight belongs to no card of the deck.
    play(run.combat, DeckCard(CardId::THE_SCYTHE, true));
    assert(run.combat.permanent_gains.size() == 2);
    run.proceed_after_combat();
    assert(run.master_deck[deck_size - 2].bonus == 3 && run.master_deck[deck_size - 1].bonus == 5);
    assert(run.combat.permanent_gains.empty());
    // The next fight starts with the stronger cards.
    enter_room_of_type(run, RoomType::MONSTER);
    assert(take(CardId::GENETIC_ALGORITHM).bonus == 3 && take(CardId::THE_SCYTHE).bonus == 5);
    run.combat.enemies[0].hp = 200;
    play(run.combat, take(CardId::THE_SCYTHE));
    assert(run.combat.enemies[0].hp == 200 - 18);

    assert_in_pool(CharacterId::IRONCLAD, { CardId::RAMPAGE });
    assert_in_pool(CharacterId::DEFECT, { CardId::GENETIC_ALGORITHM, CardId::MOMENTUM_STRIKE, CardId::CLAW, CardId::MODDED });
    assert_in_pool(CharacterId::REGENT, { CardId::KINGLY_KICK, CardId::KINGLY_PUNCH });
    assert_in_pool(CharacterId::NECROBINDER, { CardId::THE_SCYTHE, CardId::MELANCHOLY, CardId::HANG });
    std::cout << "[PASS] Evolving cards: Rampage, Claw, Momentum Strike, Modded, Kingly Kick, Kingly Punch, Melancholy, Hang, Genetic Algorithm, The Scythe" << std::endl;
}

void test_status_cards() {
    // Unplayable: refused from the hand whatever the energy.
    CombatEngine wound = make_combat(CharacterId::IRONCLAD);
    wound.hand.clear();
    wound.hand.push_back(CardId::WOUND);
    assert(!wound.can_play_card(0) && wound.play_blocker(0) == PlayBlocker::NOT_PLAYABLE);
    assert(CardDef::get(CardId::WOUND).type == CardType::STATUS && CardDef::has_keyword(CardId::SOOT, false, KEYWORD_UNPLAYABLE));
    assert(!CardDef::has_keyword(CardId::SLIMED, false, KEYWORD_UNPLAYABLE));
    // Played by another card, it does nothing.
    wound.draw_pile.push_back(CardId::WOUND);
    play(wound, CardId::HAVOC);
    assert(wound.enemies[0].hp == 42 && count_in(wound.exhaust_pile, CardId::WOUND) == 1);

    // Dazed: Unplayable and Ethereal.
    CombatEngine dazed = make_combat(CharacterId::DEFECT);
    dazed.hand.clear();
    dazed.hand.push_back(CardId::DAZED);
    assert(!dazed.can_play_card(0));
    end_turn_idle(dazed);
    assert(count_in(dazed.exhaust_pile, CardId::DAZED) == 1);

    // Burn: 2 damage at the end of the turn for each one in hand; Block absorbs it.
    CombatEngine burn = make_combat(CharacterId::DEFECT);
    int16_t hp = burn.player.hp;
    burn.hand.clear();
    burn.hand.push_back(CardId::BURN);
    burn.hand.push_back(CardId::BURN);
    end_turn_idle(burn);
    assert(burn.player.hp == hp - 4 && count_in(burn.discard_pile, CardId::BURN) == 2);
    burn.hand.clear();
    burn.hand.push_back(CardId::BURN);
    burn.player.block = 5;
    end_turn_idle(burn);
    assert(burn.player.hp == hp - 4);
    burn.hand.clear();
    burn.hand.push_back(CardId::BURN);
    burn.player.hp = 2;
    end_turn_idle(burn);
    assert(burn.state == CombatState::DEFEAT);

    // Void: 1 energy lost when it is drawn, never below 0.
    CombatEngine void_card = make_combat(CharacterId::DEFECT);
    void_card.draw_pile.push_back(CardId::VOID_CARD);
    void_card.draw_pile.push_back(CardId::VOID_CARD);
    assert(void_card.draw_card() && void_card.player.energy == 2);
    void_card.player.energy = 0;
    assert(void_card.draw_card() && void_card.player.energy == 0);
    end_turn_idle(void_card);
    assert(count_in(void_card.exhaust_pile, CardId::VOID_CARD) == 2);

    // Slimed and Debris are played like any card, and counted as cards played.
    CombatEngine slimed = make_combat(CharacterId::DEFECT);
    play(slimed, CardId::SLIMED);
    assert(slimed.player.energy == 2 && slimed.hand.size() == 1 && count_in(slimed.exhaust_pile, CardId::SLIMED) == 1);
    assert(slimed.counters.statuses_played == 1 && slimed.counters.cards_played() == 1 && slimed.counters.skills_played == 0);
    play(slimed, CardId::DEBRIS);
    assert(slimed.player.energy == 2 && count_in(slimed.exhaust_pile, CardId::DEBRIS) == 1);

    // Fuel: a 0-cost Skill made from Status cards.
    play(slimed, CardId::FUEL);
    assert(slimed.player.energy == 4 && count_in(slimed.exhaust_pile, CardId::FUEL) == 1);
    play(slimed, DeckCard(CardId::FUEL, true));
    assert(slimed.player.energy == 5 && CardDef::get(CardId::FUEL).type == CardType::SKILL);

    // Cards made during the combat: a full hand overflows into the discard pile, the draw pile takes
    // the card anywhere.
    CombatEngine created = make_combat(CharacterId::DEFECT);
    created.hand.clear();
    created.create_card(CardId::WOUND, Pile::HAND);
    assert(created.hand.size() == 1 && created.counters.combat_cards_created == 1);
    while (created.hand.size() < created.hand.capacity()) created.hand.push_back(CardId::STRIKE);
    size_t discarded = created.discard_pile.size();
    created.create_card(CardId::WOUND, Pile::HAND);
    assert(created.hand.size() == created.hand.capacity() && created.discard_pile.size() == discarded + 1);
    size_t drawable = created.draw_pile.size();
    DeckCard top = created.draw_pile.back();
    for (int i = 0; i < 3; ++i) created.create_card(CardId::DAZED, Pile::DRAW);
    assert(created.draw_pile.size() == drawable + 3 && count_in(created.draw_pile, CardId::DAZED) == 3);
    assert(count_in(created.draw_pile, top.id) >= 1);

    // No Status card is offered as a reward.
    for (CharacterId hero : { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT, CharacterId::REGENT, CharacterId::NECROBINDER }) {
        for (CardId id : RunManager::card_reward_pool(hero)) {
            assert(CardDef::get(id).type != CardType::STATUS && id != CardId::FUEL);
        }
    }
    std::cout << "[PASS] Status cards: Unplayable, Dazed, Burn, Void, Slimed, Debris, Fuel, creation in each pile" << std::endl;
}

void test_status_card_users() {
    // Cards that pay for their effect with a Status card in the discard pile.
    CombatEngine boost = make_combat(CharacterId::DEFECT);
    play(boost, CardId::BOOST_AWAY);
    assert(boost.player.block == 6 && count_in(boost.discard_pile, CardId::DAZED) == 1 && boost.player.energy == 3);
    play(boost, DeckCard(CardId::FIGHT_THROUGH, true));
    assert(boost.player.block == 6 + 17 && count_in(boost.discard_pile, CardId::WOUND) == 2);
    play(boost, CardId::GUNK_UP);
    assert(boost.enemies[0].hp == 42 - 12 && count_in(boost.discard_pile, CardId::SLIMED) == 1);
    pad_draw_pile(boost, 3);
    play(boost, DeckCard(CardId::OVERCLOCK, true));
    assert(boost.hand.size() == 3 && count_in(boost.discard_pile, CardId::BURN) == 1);
    play(boost, CardId::TURBO);
    assert(boost.player.energy == 5 && count_in(boost.discard_pile, CardId::VOID_CARD) == 1);
    assert(boost.counters.combat_cards_created == 6);

    // Smokestack: damage to every enemy for each Status card created.
    CombatEngine smokestack = make_pack(CharacterId::DEFECT);
    play(smokestack, CardId::SMOKESTACK);
    play(smokestack, CardId::FIGHT_THROUGH);
    assert(smokestack.enemies[0].hp == 8 && smokestack.enemies[1].hp == 8 && smokestack.enemies[2].hp == 32);
    // Other created cards do not count.
    play(smokestack, CardId::ADAPTIVE_STRIKE, 2);
    assert(smokestack.enemies[0].hp == 8 && smokestack.enemies[2].hp == 14);

    // Trash to Treasure: a random orb for each Status card created.
    CombatEngine treasure = make_combat(CharacterId::DEFECT);
    play(treasure, CardId::TRASH_TO_TREASURE);
    play(treasure, CardId::FIGHT_THROUGH);
    assert(treasure.player.orbs.size() == 2 && CardDef::get_cost(CardId::TRASH_TO_TREASURE, true) == 0);

    // Iteration: the first Status card drawn each turn draws more.
    CombatEngine iteration = make_combat(CharacterId::DEFECT);
    play(iteration, CardId::ITERATION);
    iteration.draw_pile.push_back(CardId::WOUND);
    iteration.draw_pile.push_back(CardId::DEFEND);
    iteration.draw_pile.push_back(CardId::DEFEND);
    iteration.draw_pile.push_back(CardId::WOUND);
    assert(iteration.draw_card() && iteration.hand.size() == 3 && iteration.power_counter(PowerId::ITERATION) == 1);
    assert(iteration.draw_card() && iteration.hand.size() == 4);
    end_turn_idle(iteration);
    assert(iteration.power_counter(PowerId::ITERATION) == 0);

    // Rocket Punch: 1 less for each Status card created, until it is played.
    CombatEngine rocket = make_combat(CharacterId::DEFECT);
    rocket.discard_pile.push_back(CardId::ROCKET_PUNCH);
    play(rocket, CardId::FIGHT_THROUGH);
    DeckCard punch;
    for (const DeckCard& card : rocket.discard_pile) {
        if (card.id == CardId::ROCKET_PUNCH) punch = card;
    }
    assert(punch.id == CardId::ROCKET_PUNCH && rocket.effective_cost(punch) == 0);
    pad_draw_pile(rocket, 2);
    play(rocket, punch);
    assert(rocket.enemies[0].hp == 42 - 13 && rocket.player.energy == 3 && rocket.hand.size() == 1);
    assert(rocket.effective_cost(rocket.discard_pile.back()) == 2);

    // Compact: the Status cards of the hand become Fuel.
    CombatEngine compact = make_combat(CharacterId::DEFECT);
    compact.hand.clear();
    for (CardId id : { CardId::COMPACT, CardId::WOUND, CardId::BURN, CardId::STRIKE }) compact.hand.push_back(id);
    assert(compact.play_card(0) && compact.player.block == 6);
    assert(count_in(compact.hand, CardId::FUEL) == 2 && count_in(compact.hand, CardId::STRIKE) == 1 && !compact.hand[0].upgraded);
    compact.hand.clear();
    compact.hand.push_back(DeckCard(CardId::COMPACT, true));
    compact.hand.push_back(CardId::DAZED);
    assert(compact.play_card(0) && compact.hand[0] == DeckCard(CardId::FUEL, true));

    // Flak Cannon: every Status card of the three piles is exhausted, one hit for each.
    CombatEngine flak = make_combat(CharacterId::DEFECT);
    flak.hand.clear();
    flak.hand.push_back(CardId::FLAK_CANNON);
    flak.hand.push_back(CardId::WOUND);
    flak.draw_pile.push_back(CardId::DAZED);
    flak.discard_pile.push_back(CardId::BURN);
    assert(flak.play_card(0) && flak.enemies[0].hp == 42 - 3 * 8 && flak.counters.hits_dealt == 3);
    assert(flak.exhaust_pile.size() == 3 && flak.hand.empty() && count_in(flak.draw_pile, CardId::DAZED) == 0);
    play(flak, DeckCard(CardId::FLAK_CANNON, true));
    assert(flak.enemies[0].hp == 18);

    // Regent: Debris in hand.
    CombatEngine collision = make_combat(CharacterId::REGENT);
    play(collision, CardId::COLLISION_COURSE);
    assert(collision.enemies[0].hp == 32 && count_in(collision.hand, CardId::DEBRIS) == 1 && collision.player.energy == 3);
    CombatEngine crash = make_pack(CharacterId::REGENT);
    play(crash, CardId::CRASH_LANDING);
    assert(!crash.enemies[0].is_alive() && !crash.enemies[1].is_alive() && crash.enemies[2].hp == 21);
    assert(crash.hand.size() == crash.hand.capacity() && count_in(crash.hand, CardId::DEBRIS) == 10);

    // Blessed Antler: one more energy each turn, three Dazed in the draw pile, none left in the deck.
    RunManager run;
    run.start_run(CharacterId::DEFECT, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("BLESSED_ANTLER");
    size_t deck_size = run.master_deck.size();
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.combat.player.energy == 4);
    assert(count_in(run.combat.hand, CardId::DAZED) + count_in(run.combat.draw_pile, CardId::DAZED) == 3);
    end_turn_idle(run.combat);
    assert(run.combat.player.energy == 4);
    run.proceed_after_combat();
    assert(run.master_deck.size() == deck_size);

    assert_in_pool(CharacterId::DEFECT, { CardId::BOOST_AWAY, CardId::FLAK_CANNON, CardId::SMOKESTACK, CardId::COMPACT, CardId::GUNK_UP,
                                          CardId::ITERATION, CardId::FIGHT_THROUGH, CardId::OVERCLOCK, CardId::ROCKET_PUNCH,
                                          CardId::TRASH_TO_TREASURE, CardId::TURBO });
    assert_in_pool(CharacterId::REGENT, { CardId::COLLISION_COURSE, CardId::CRASH_LANDING });
    std::cout << "[PASS] Status card users: Boost Away, Fight Through, Gunk Up, Overclock, Turbo, Smokestack, Trash to Treasure, Iteration, Rocket Punch, Compact, Flak Cannon, Collision Course, Crash Landing, Blessed Antler" << std::endl;
}

void test_shivs() {
    // Shiv: a 0-cost Attack that exhausts.
    CombatEngine shiv = make_combat(CharacterId::SILENT);
    play(shiv, CardId::SHIV);
    assert(shiv.enemies[0].hp == 38 && shiv.player.energy == 3 && count_in(shiv.exhaust_pile, CardId::SHIV) == 1);
    play(shiv, DeckCard(CardId::SHIV, true));
    assert(shiv.enemies[0].hp == 32 && shiv.counters.shivs_played == 2 && shiv.counters.attacks_played == 2);
    // Added to the hand; what does not fit goes to the discard pile.
    shiv.hand.clear();
    for (int i = 0; i < 8; ++i) shiv.hand.push_back(CardId::DEFEND);
    size_t discarded = shiv.discard_pile.size();
    shiv.add_shivs(3);
    assert(count_in(shiv.hand, CardId::SHIV) == 2 && shiv.discard_pile.size() == discarded + 1);

    // Cards that add Shivs.
    CombatEngine dance = make_combat(CharacterId::SILENT);
    play(dance, CardId::BLADE_DANCE);
    assert(count_in(dance.hand, CardId::SHIV) == 3 && count_in(dance.exhaust_pile, CardId::BLADE_DANCE) == 1);
    play(dance, DeckCard(CardId::BLADE_DANCE, true));
    assert(count_in(dance.hand, CardId::SHIV) == 4);
    play(dance, CardId::CLOAK_AND_DAGGER);
    assert(dance.player.block == 6 && count_in(dance.hand, CardId::SHIV) == 1);
    play(dance, DeckCard(CardId::CLOAK_AND_DAGGER, true));
    assert(dance.player.block == 12 && count_in(dance.hand, CardId::SHIV) == 2);
    play(dance, CardId::LEADING_STRIKE);
    assert(dance.enemies[0].hp == 39 && count_in(dance.hand, CardId::SHIV) == 2);
    play(dance, DeckCard(CardId::LEADING_STRIKE, true));
    assert(dance.enemies[0].hp == 33);

    // Blade of Ink: Inky Shivs also apply Weak.
    CombatEngine ink = make_combat(CharacterId::SILENT);
    play(ink, CardId::BLADE_OF_INK);
    assert(ink.hand.size() == 2 && ink.hand[0].enchantment == Enchantment::INKY);
    assert(ink.play_card(0, 0) && ink.enemies[0].hp == 38 && ink.enemies[0].get_status(StatusType::WEAK) == INKY_WEAK);
    play(ink, DeckCard(CardId::BLADE_OF_INK, true));
    assert(count_in(ink.hand, CardId::SHIV) == 3);

    // Accuracy: more damage for every Shiv.
    CombatEngine accuracy = make_combat(CharacterId::SILENT);
    play(accuracy, CardId::ACCURACY);
    play(accuracy, CardId::SHIV);
    assert(accuracy.enemies[0].hp == 42 - 8);
    play(accuracy, DeckCard(CardId::ACCURACY, true));
    play(accuracy, CardId::STRIKE);
    play(accuracy, DeckCard(CardId::SHIV, true));
    assert(accuracy.enemies[0].hp == 34 - 6 - 16);

    // Phantom Blades: the first Shiv of each turn hits harder, and Shivs stay in hand.
    CombatEngine phantom = make_combat(CharacterId::SILENT);
    play(phantom, CardId::PHANTOM_BLADES);
    play(phantom, CardId::SHIV);
    play(phantom, CardId::SHIV);
    assert(phantom.enemies[0].hp == 42 - 13 - 4);
    phantom.hand.clear();
    phantom.hand.push_back(CardId::SHIV);
    phantom.hand.push_back(CardId::STRIKE);
    end_turn_idle(phantom);
    assert(count_in(phantom.hand, CardId::SHIV) == 1 && count_in(phantom.discard_pile, CardId::STRIKE) >= 1);
    play(phantom, CardId::SHIV);
    assert(phantom.enemies[0].hp == 25 - 13);
    // A Shiv played before the power was gained already was the first of the turn.
    CombatEngine late = make_combat(CharacterId::SILENT);
    play(late, CardId::SHIV);
    play(late, DeckCard(CardId::PHANTOM_BLADES, true));
    play(late, CardId::SHIV);
    assert(late.enemies[0].hp == 42 - 4 - 4);

    // Fan of Knives: Shivs hit every enemy.
    CombatEngine fan = make_pack(CharacterId::SILENT);
    play(fan, CardId::FAN_OF_KNIVES);
    assert(count_in(fan.hand, CardId::SHIV) == 4);
    assert(fan.play_card(0, 2));
    assert(fan.enemies[0].hp == 14 && fan.enemies[1].hp == 14 && fan.enemies[2].hp == 38);
    fan.hand.clear();
    fan.add_shivs(1, false, Enchantment::INKY);
    assert(fan.play_card(0, 0));
    for (const Enemy& foe : fan.enemies) assert(foe.get_status(StatusType::WEAK) == 1);
    play(fan, DeckCard(CardId::FAN_OF_KNIVES, true));
    assert(count_in(fan.hand, CardId::SHIV) == 5);

    // Infinite Blades: a Shiv at the start of each turn.
    CombatEngine infinite = make_combat(CharacterId::SILENT);
    play(infinite, CardId::INFINITE_BLADES);
    assert(count_in(infinite.hand, CardId::SHIV) == 0);
    end_turn_idle(infinite);
    assert(count_in(infinite.hand, CardId::SHIV) == 1 && infinite.hand.size() == 6);
    assert(CardDef::has_keyword(CardId::INFINITE_BLADES, true, KEYWORD_INNATE) && !CardDef::has_keyword(CardId::INFINITE_BLADES, false, KEYWORD_INNATE));

    // Storm of Steel: the hand becomes as many Shivs.
    CombatEngine storm = make_combat(CharacterId::SILENT);
    storm.hand.clear();
    for (CardId id : { CardId::STORM_OF_STEEL, CardId::STRIKE, CardId::DEFEND, CardId::DEFEND }) storm.hand.push_back(id);
    assert(storm.play_card(0));
    assert(storm.hand.size() == 3 && count_in(storm.hand, CardId::SHIV) == 3 && !storm.hand[0].upgraded);
    assert(storm.counters.cards_discarded == 3);
    storm.hand.clear();
    storm.hand.push_back(DeckCard(CardId::STORM_OF_STEEL, true));
    storm.hand.push_back(CardId::STRIKE);
    assert(storm.play_card(0) && storm.hand.size() == 1 && storm.hand[0] == DeckCard(CardId::SHIV, true));

    // Up My Sleeve: 1 cheaper each time it is played.
    CombatEngine sleeve = make_combat(CharacterId::SILENT);
    play(sleeve, CardId::UP_MY_SLEEVE);
    assert(sleeve.player.energy == 1 && count_in(sleeve.hand, CardId::SHIV) == 3);
    DeckCard cheaper = sleeve.discard_pile.back();
    assert(cheaper.id == CardId::UP_MY_SLEEVE && sleeve.effective_cost(cheaper) == 1);
    play(sleeve, cheaper);
    assert(sleeve.player.energy == 2 && sleeve.effective_cost(sleeve.discard_pile.back()) == 0);

    // Knife Trap: the Shivs of the exhaust pile are played on the enemy, and stay exhausted.
    CombatEngine trap = make_pack(CharacterId::SILENT);
    trap.exhaust_pile.push_back(CardId::SHIV);
    trap.exhaust_pile.push_back(CardId::STRIKE);
    trap.exhaust_pile.push_back(CardId::SHIV);
    play(trap, CardId::KNIFE_TRAP, 2);
    assert(trap.enemies[2].hp == 42 - 8 && trap.enemies[0].hp == 18 && trap.exhaust_pile.size() == 3);
    assert(count_in(trap.discard_pile, CardId::KNIFE_TRAP) == 1 && trap.counters.shivs_played == 2);
    play(trap, DeckCard(CardId::KNIFE_TRAP, true), 2);
    assert(trap.enemies[2].hp == 34 - 12 && trap.exhaust_pile[0].upgraded && !trap.exhaust_pile[1].upgraded);

    // Helical Dart: Dexterity until the end of the turn for each Shiv played.
    CombatEngine dart = make_relic_combat(CharacterId::SILENT, PowerId::HELICAL_DART);
    play(dart, CardId::SHIV);
    play(dart, CardId::DEFEND);
    assert(dart.player.block == 6);
    end_turn_idle(dart);
    assert(dart.player.get_status(StatusType::DEXTERITY) == 0);

    // Ninja Scroll: three Shivs in the first hand.
    RunManager run;
    run.start_run(CharacterId::SILENT, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("NINJA_SCROLL");
    run.add_relic("HELICAL_DART");
    enter_room_of_type(run, RoomType::MONSTER);
    assert(count_in(run.combat.hand, CardId::SHIV) == 3 && run.combat.has_power(PowerId::HELICAL_DART));

    // Cunning Potion: three upgraded Shivs.
    CombatEngine potion = make_combat(CharacterId::IRONCLAD);
    potion.hand.clear();
    assert(potion.use_potion("CUNNING_POTION"));
    assert(potion.hand.size() == 3 && potion.hand[2] == DeckCard(CardId::SHIV, true));

    assert_in_pool(CharacterId::SILENT, { CardId::BLADE_DANCE, CardId::CLOAK_AND_DAGGER, CardId::LEADING_STRIKE, CardId::BLADE_OF_INK,
                                          CardId::INFINITE_BLADES, CardId::STORM_OF_STEEL, CardId::FAN_OF_KNIVES, CardId::PHANTOM_BLADES,
                                          CardId::ACCURACY, CardId::UP_MY_SLEEVE, CardId::KNIFE_TRAP });
    std::span<const CardId> silent_pool = RunManager::card_reward_pool(CharacterId::SILENT);
    assert(std::find(silent_pool.begin(), silent_pool.end(), CardId::SHIV) == silent_pool.end());
    std::cout << "[PASS] Shivs: Shiv, Blade Dance, Cloak and Dagger, Leading Strike, Blade of Ink, Accuracy, Phantom Blades, Fan of Knives, Infinite Blades, Storm of Steel, Up My Sleeve, Knife Trap, Helical Dart, Ninja Scroll, Cunning Potion" << std::endl;
}

// Replaces the hand by `cards`, with full energy.
void deal(CombatEngine& combat, std::initializer_list<DeckCard> cards) {
    combat.hand.clear();
    for (const DeckCard& card : cards) combat.hand.push_back(card);
    combat.player.energy = combat.player.max_energy;
}

bool pick(CombatEngine& combat, std::initializer_list<uint8_t> hand_indices) {
    return combat.resolve_choice(std::span<const uint8_t>(hand_indices.begin(), hand_indices.size()));
}

void test_hand_choices() {
    // Survivor: the player picks the card to discard, and nothing else can be played meanwhile.
    CombatEngine survivor = make_combat(CharacterId::SILENT);
    deal(survivor, { CardId::SURVIVOR, CardId::STRIKE, CardId::DEFEND });
    assert(survivor.play_card(0) && survivor.player.block == 8);
    assert(survivor.choice_pending() && survivor.choice_size() == 1 && survivor.hand.size() == 2);
    assert(!survivor.can_play_card(0) && survivor.play_blocker(0) == PlayBlocker::CHOICE);
    // Picks that do not answer the choice are refused.
    assert(!pick(survivor, {}) && !pick(survivor, { 0, 1 }) && !pick(survivor, { 5 }) && survivor.choice_pending());
    assert(pick(survivor, { 1 }));
    assert(!survivor.choice_pending() && survivor.hand.size() == 1 && survivor.hand[0] == CardId::STRIKE);
    assert(survivor.discard_pile.back() == CardId::DEFEND && survivor.counters.cards_discarded == 1);
    assert(survivor.can_play_card(0));
    // Nothing to decide: a single card is discarded at once, an empty hand asks nothing.
    deal(survivor, { CardId::SURVIVOR, CardId::STRIKE });
    assert(survivor.play_card(0) && !survivor.choice_pending() && survivor.hand.empty());
    deal(survivor, { CardId::SURVIVOR });
    assert(survivor.play_card(0) && !survivor.choice_pending());
    // A choice left unanswered is answered when the turn ends.
    deal(survivor, { CardId::SURVIVOR, CardId::STRIKE, CardId::DEFEND });
    assert(survivor.play_card(0) && survivor.choice_pending());
    end_turn_idle(survivor);
    assert(!survivor.choice_pending() && survivor.state == CombatState::PLAYER_TURN);

    // Exhaust: True Grit+ lets the player pick, True Grit does not.
    CombatEngine grit = make_combat(CharacterId::IRONCLAD);
    deal(grit, { DeckCard(CardId::TRUE_GRIT, true), CardId::STRIKE, CardId::BASH });
    assert(grit.play_card(0) && grit.choice_pending() && grit.exhaust_pile.empty());
    assert(pick(grit, { 1 }) && grit.exhaust_pile.back() == CardId::BASH && grit.hand.size() == 1);
    deal(grit, { CardId::TRUE_GRIT, CardId::STRIKE, CardId::BASH });
    assert(grit.play_card(0) && !grit.choice_pending() && grit.exhaust_pile.size() == 2);

    // Burning Pact: the draw comes after the card is picked.
    CombatEngine pact = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(pact, 4);
    deal(pact, { CardId::BURNING_PACT, CardId::STRIKE, CardId::BASH });
    assert(pact.play_card(0) && pact.choice_pending() && pact.hand.size() == 2);
    assert(pick(pact, { 0 }) && pact.hand.size() == 3 && pact.exhaust_pile.back() == CardId::STRIKE);
    deal(pact, { DeckCard(CardId::BURNING_PACT, true) });
    assert(pact.play_card(0) && pact.hand.size() == 3);

    // Brand: HP and Strength at once, then the exhaust.
    CombatEngine brand = make_combat(CharacterId::IRONCLAD);
    int16_t hp = brand.player.hp;
    deal(brand, { DeckCard(CardId::BRAND, true), CardId::STRIKE, CardId::BASH });
    assert(brand.play_card(0) && brand.player.hp == hp - 1 && brand.player.get_status(StatusType::STRENGTH) == 2);
    assert(brand.choice_pending() && pick(brand, { 0 }) && brand.exhaust_pile.size() == 1);

    // Armaments: only a card that can be upgraded can be picked; upgraded, the whole hand is.
    CombatEngine armaments = make_combat(CharacterId::IRONCLAD);
    deal(armaments, { CardId::ARMAMENTS, DeckCard(CardId::STRIKE, true), CardId::DEFEND, CardId::BASH, CardId::WOUND });
    assert(armaments.play_card(0) && armaments.player.block == 5 && armaments.choice_pending());
    assert(!armaments.choice_allows(0) && armaments.choice_allows(1) && !armaments.choice_allows(3));
    assert(!pick(armaments, { 0 }) && pick(armaments, { 1 }));
    assert(armaments.hand[1].upgraded && !armaments.hand[2].upgraded && armaments.hand.size() == 4);
    deal(armaments, { DeckCard(CardId::ARMAMENTS, true), CardId::DEFEND, CardId::BASH, CardId::WOUND });
    assert(armaments.play_card(0) && !armaments.choice_pending());
    assert(armaments.hand[0].upgraded && armaments.hand[1].upgraded && !armaments.hand[2].upgraded);

    // Silent: draw, then discard.
    CombatEngine acrobatics = make_combat(CharacterId::SILENT);
    pad_draw_pile(acrobatics, 6);
    deal(acrobatics, { CardId::ACROBATICS });
    assert(acrobatics.play_card(0) && acrobatics.hand.size() == 3 && acrobatics.choice_pending());
    assert(pick(acrobatics, { 2 }) && acrobatics.hand.size() == 2);
    deal(acrobatics, { CardId::DAGGER_THROW, CardId::STRIKE });
    assert(acrobatics.play_card(0) && acrobatics.enemies[0].hp == 42 - 9 && acrobatics.hand.size() == 2);
    assert(acrobatics.choice_pending() && pick(acrobatics, { 0 }) && acrobatics.hand.size() == 1);
    // Prepared+: two cards to pick.
    pad_draw_pile(acrobatics, 4);
    deal(acrobatics, { DeckCard(CardId::PREPARED, true), CardId::STRIKE });
    assert(acrobatics.play_card(0) && acrobatics.hand.size() == 3 && acrobatics.choice_size() == 2);
    assert(!pick(acrobatics, { 0 }) && !pick(acrobatics, { 1, 1 }) && pick(acrobatics, { 0, 2 }));
    assert(acrobatics.hand.size() == 1 && acrobatics.player.energy == 3);

    // Hidden Daggers: the Shivs come once the cards are discarded.
    CombatEngine daggers = make_combat(CharacterId::SILENT);
    deal(daggers, { CardId::HIDDEN_DAGGERS, CardId::STRIKE, CardId::DEFEND, CardId::NEUTRALIZE });
    assert(daggers.play_card(0) && daggers.choice_size() == 2 && count_in(daggers.hand, CardId::SHIV) == 0);
    assert(pick(daggers, { 0, 1 }) && daggers.hand.size() == 3 && count_in(daggers.hand, CardId::SHIV) == 2);
    deal(daggers, { DeckCard(CardId::HIDDEN_DAGGERS, true), CardId::STRIKE });
    assert(daggers.play_card(0) && !daggers.choice_pending() && daggers.hand[0] == DeckCard(CardId::SHIV, true));

    // Scavenge: energy on the next turn.
    CombatEngine scavenge = make_combat(CharacterId::DEFECT);
    deal(scavenge, { CardId::SCAVENGE, CardId::STRIKE, CardId::DEFEND });
    assert(scavenge.play_card(0) && pick(scavenge, { 0 }) && scavenge.exhaust_pile.back() == CardId::STRIKE);
    end_turn_idle(scavenge);
    assert(scavenge.player.energy == 5);

    // Regent: a card goes back on top of the draw pile.
    CombatEngine photon = make_combat(CharacterId::REGENT);
    pad_draw_pile(photon, 6);
    deal(photon, { CardId::PHOTON_CUT, CardId::STRIKE });
    assert(photon.play_card(0) && photon.enemies[0].hp == 42 - 10 && photon.hand.size() == 2 && photon.choice_pending());
    assert(pick(photon, { 0 }) && photon.draw_pile.back() == CardId::STRIKE && photon.hand.size() == 1);
    deal(photon, { DeckCard(CardId::GLIMMER, true) });
    assert(photon.play_card(0) && photon.hand.size() == 4 && pick(photon, { 3 }) && photon.hand.size() == 3);

    // Tyranny: one more card each turn, and one card of the new hand to exhaust.
    CombatEngine tyranny = make_combat(CharacterId::REGENT);
    pad_draw_pile(tyranny, 8);
    play(tyranny, CardId::TYRANNY);
    end_turn_idle(tyranny);
    assert(tyranny.hand.size() == 6 && tyranny.choice_pending() && tyranny.pending_choices[0].kind == ChoiceKind::EXHAUST);
    size_t exhausted = tyranny.exhaust_pile.size();
    assert(pick(tyranny, { 5 }) && tyranny.hand.size() == 5 && tyranny.exhaust_pile.size() == exhausted + 1);
    assert(CardDef::has_keyword(CardId::TYRANNY, true, KEYWORD_INNATE) && !CardDef::has_keyword(CardId::TYRANNY, false, KEYWORD_INNATE));

    // Sculpting Strike: the picked card becomes Ethereal.
    CombatEngine sculpt = make_combat(CharacterId::NECROBINDER);
    deal(sculpt, { CardId::SCULPTING_STRIKE, CardId::STRIKE, CardId::DEFILE, CardId::DEFEND });
    assert(sculpt.play_card(0) && sculpt.enemies[0].hp == 42 - 9 && sculpt.choice_pending());
    assert(!sculpt.choice_allows(1) && pick(sculpt, { 2 }) && CardDef::has_keyword(sculpt.hand[2], KEYWORD_ETHEREAL));
    end_turn_idle(sculpt);
    assert(count_in(sculpt.exhaust_pile, CardId::DEFEND) == 1 && count_in(sculpt.exhaust_pile, CardId::DEFILE) == 1);

    // A card played twice asks twice, one choice after the other.
    CombatEngine twice = make_combat(CharacterId::SILENT);
    pad_draw_pile(twice, 8);
    play(twice, CardId::BURST);
    deal(twice, { CardId::ACROBATICS });
    assert(twice.play_card(0) && twice.hand.size() == 6 && twice.pending_choices.size() == 2);
    assert(pick(twice, { 0 }) && twice.choice_pending() && pick(twice, { 0 }) && !twice.choice_pending());
    assert(twice.hand.size() == 4);

    // Gambling Chip: any number of cards of the first hand, replaced by as many.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("GAMBLING_CHIP");
    enter_room_of_type(run, RoomType::MONSTER);
    CombatEngine& chip = run.combat;
    assert(chip.choice_pending() && chip.pending_choices[0].optional && chip.choice_size() == 5);
    size_t in_discard = chip.discard_pile.size();
    assert(pick(chip, { 0, 3 }) && chip.hand.size() == 5 && chip.discard_pile.size() == in_discard + 2);
    // Left alone, an optional choice picks nothing.
    enter_room_of_type(run, RoomType::MONSTER);
    assert(chip.choice_pending());
    chip.resolve_choice_default();
    assert(!chip.choice_pending() && chip.hand.size() == 5 && chip.discard_pile.empty());

    assert_in_pool(CharacterId::IRONCLAD, { CardId::ARMAMENTS, CardId::BRAND, CardId::BURNING_PACT });
    assert_in_pool(CharacterId::SILENT, { CardId::ACROBATICS, CardId::DAGGER_THROW, CardId::PREPARED, CardId::HIDDEN_DAGGERS });
    assert_in_pool(CharacterId::DEFECT, { CardId::SCAVENGE });
    assert_in_pool(CharacterId::REGENT, { CardId::PHOTON_CUT, CardId::GLIMMER, CardId::TYRANNY });
    assert_in_pool(CharacterId::NECROBINDER, { CardId::SCULPTING_STRIKE });
    std::cout << "[PASS] Hand choices: Survivor, True Grit+, Burning Pact, Brand, Armaments, Acrobatics, Dagger Throw, Prepared, Hidden Daggers, Scavenge, Photon Cut, Glimmer, Tyranny, Sculpting Strike, two choices in a row, Gambling Chip" << std::endl;
}

void test_pile_choices() {
    // Headbutt: a card of the discard pile goes on top of the draw pile; the Headbutt itself cannot be picked.
    CombatEngine headbutt = make_combat(CharacterId::IRONCLAD);
    headbutt.discard_pile.push_back(CardId::BASH);
    headbutt.discard_pile.push_back(CardId::DEFEND);
    deal(headbutt, { CardId::HEADBUTT });
    assert(headbutt.play_card(0) && headbutt.enemies[0].hp == 42 - 9 && headbutt.choice_pending());
    assert(headbutt.pending_choices[0].source == Pile::DISCARD && headbutt.choice_pool_size() == 2);
    assert(headbutt.discard_pile.size() == 3 && headbutt.discard_pile[2] == CardId::HEADBUTT);
    assert(!headbutt.choice_allows(2) && !pick(headbutt, { 2 }) && headbutt.choice_pending());
    assert(pick(headbutt, { 0 }) && headbutt.draw_pile.back() == CardId::BASH && headbutt.discard_pile.size() == 2);
    // A single card in the discard pile leaves nothing to decide; none, nothing happens.
    CombatEngine alone = make_combat(CharacterId::IRONCLAD);
    alone.discard_pile.push_back(CardId::BASH);
    deal(alone, { DeckCard(CardId::HEADBUTT, true) });
    assert(alone.play_card(0) && !alone.choice_pending() && alone.draw_pile.back() == CardId::BASH);
    assert(alone.enemies[0].hp == 42 - 12);
    deal(alone, { CardId::HEADBUTT });
    alone.discard_pile.clear();
    assert(alone.play_card(0) && !alone.choice_pending() && alone.discard_pile.size() == 1);

    // Hologram: from the discard pile to the hand.
    CombatEngine hologram = make_combat(CharacterId::DEFECT);
    hologram.discard_pile.push_back(CardId::ZAP);
    hologram.discard_pile.push_back(CardId::STRIKE);
    deal(hologram, { CardId::HOLOGRAM });
    assert(hologram.play_card(0) && hologram.player.block == 3 && hologram.choice_pending());
    assert(pick(hologram, { 0 }) && hologram.hand.size() == 1 && hologram.hand[0] == CardId::ZAP);
    assert(count_in(hologram.exhaust_pile, CardId::HOLOGRAM) == 1 && !CardDef::has_keyword(CardId::HOLOGRAM, true, KEYWORD_EXHAUST));

    // Cosmic Indifference, and Foregone Conclusion's pick in the draw pile once the next hand is drawn.
    CombatEngine regent = make_combat(CharacterId::REGENT);
    regent.discard_pile.push_back(CardId::STRIKE);
    regent.discard_pile.push_back(CardId::DEFEND);
    deal(regent, { DeckCard(CardId::COSMIC_INDIFFERENCE, true) });
    assert(regent.play_card(0) && regent.player.block == 9 && pick(regent, { 1 }) && regent.draw_pile.back() == CardId::DEFEND);
    pad_draw_pile(regent, 10);
    deal(regent, { CardId::FOREGONE_CONCLUSION });
    assert(regent.play_card(0) && !regent.choice_pending());
    end_turn_idle(regent);
    assert(regent.choice_pending() && regent.pending_choices[0].source == Pile::DRAW && regent.choice_size() == 2);
    size_t in_draw = regent.draw_pile.size();
    assert(regent.hand.size() == 5 && pick(regent, { 0, 1 }) && regent.hand.size() == 7 && regent.draw_pile.size() == in_draw - 2);
    end_turn_idle(regent);
    assert(!regent.choice_pending());

    // Graveblast and Dredge; a full hand leaves the cards where they are.
    CombatEngine grave = make_combat(CharacterId::NECROBINDER);
    for (CardId id : { CardId::STRIKE, CardId::DEFEND, CardId::DEFILE, CardId::HANG }) grave.discard_pile.push_back(id);
    deal(grave, { CardId::GRAVEBLAST });
    assert(grave.play_card(0) && grave.enemies[0].hp == 42 - 4 && pick(grave, { 3 }) && grave.hand[0] == CardId::HANG);
    assert(count_in(grave.exhaust_pile, CardId::GRAVEBLAST) == 1);
    deal(grave, { DeckCard(CardId::DREDGE, true) });
    assert(CardDef::has_keyword(CardId::DREDGE, true, KEYWORD_RETAIN) && CardDef::has_keyword(CardId::DREDGE, true, KEYWORD_EXHAUST));
    assert(grave.play_card(0) && !grave.choice_pending() && grave.hand.size() == 3 && grave.discard_pile.empty());
    for (CardId id : { CardId::STRIKE, CardId::DEFEND, CardId::DEFILE, CardId::HANG }) grave.discard_pile.push_back(id);
    grave.hand.clear();
    grave.hand.push_back(CardId::DREDGE);
    for (int i = 0; i < 8; ++i) grave.hand.push_back(CardId::STRIKE);
    grave.player.energy = 3;
    assert(grave.play_card(0) && grave.choice_size() == 3 && pick(grave, { 0, 1, 2 }));
    assert(grave.hand.size() == 10 && grave.discard_pile.size() == 2);

    // Cleanse: a card of the draw pile is exhausted.
    CombatEngine cleanse = make_combat(CharacterId::NECROBINDER);
    cleanse.draw_pile.clear();
    cleanse.draw_pile.push_back(CardId::STRIKE);
    cleanse.draw_pile.push_back(CardId::WOUND);
    deal(cleanse, { CardId::CLEANSE });
    int16_t minion = cleanse.player.minion_hp;
    assert(cleanse.play_card(0) && cleanse.player.minion_hp == minion + 3 && cleanse.choice_pending());
    assert(pick(cleanse, { 1 }) && cleanse.draw_pile.size() == 1 && cleanse.exhaust_pile.back() == CardId::WOUND);

    // Two choices on the discard pile in a row: the second one is about what the first one left.
    CombatEngine twice = make_combat(CharacterId::IRONCLAD);
    for (CardId id : { CardId::STRIKE, CardId::DEFEND, CardId::BASH }) twice.discard_pile.push_back(id);
    play(twice, CardId::ONE_TWO_PUNCH);
    deal(twice, { CardId::HEADBUTT });
    size_t before = twice.discard_pile.size();
    assert(twice.play_card(0) && twice.pending_choices.size() == 2);
    assert(pick(twice, { 0 }) && twice.choice_pending() && twice.choice_pool_size() == static_cast<int>(before) - 1);
    assert(!twice.choice_allows(before - 1) && pick(twice, { 0 }) && !twice.choice_pending());

    assert_in_pool(CharacterId::IRONCLAD, { CardId::HEADBUTT });
    assert_in_pool(CharacterId::DEFECT, { CardId::HOLOGRAM });
    assert_in_pool(CharacterId::REGENT, { CardId::COSMIC_INDIFFERENCE, CardId::FOREGONE_CONCLUSION });
    assert_in_pool(CharacterId::NECROBINDER, { CardId::GRAVEBLAST, CardId::DREDGE, CardId::CLEANSE });
    std::cout << "[PASS] Pile choices: Headbutt, Hologram, Cosmic Indifference, Foregone Conclusion, Graveblast, Dredge, Cleanse, two choices on the discard pile" << std::endl;
}

void test_ascension() {
    // A0: enemies as designed.
    RunManager normal;
    normal.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(normal, RoomType::ELITE);
    // The elite depends on the act zone the seed draws; its A0 HP is the one A3 keeps.
    const Enemy& elite = normal.combat.enemies[0];
    assert(elite.max_hp > 0 && elite.damage_percent == 100);

    // A3: elites hit 20 % harder but keep their HP until A8.
    RunManager a3;
    a3.start_run(CharacterId::IRONCLAD, 3, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(a3, RoomType::ELITE);
    assert(a3.combat.enemies[0].type == elite.type && a3.combat.enemies[0].max_hp == elite.max_hp);
    // An elite with its own Ascension values hits harder through them instead of the percentage.
    const Enemy& a3_elite = a3.combat.enemies[0];
    assert(a3_elite.ascension_hp() > 0 ? a3_elite.ascended_damage && a3_elite.damage_percent == 100
                                       : a3_elite.damage_percent == 120);

    // A10: every room is tougher; the intent shown is the damage really dealt (Chomp 11 x 1.2 = 13).
    RunManager a10;
    a10.start_run(CharacterId::IRONCLAD, 10, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(a10, RoomType::MONSTER);
    const Enemy& cultist = a10.combat.enemies[0];
    // The first fight depends on the seed (Corpse Slugs here, which have their own Ascension values).
    assert(cultist.type == EnemyType::CORPSE_SLUG && cultist.hp == cultist.max_hp);
    assert(cultist.ascended_damage && cultist.max_hp >= 27 && cultist.max_hp <= 29);

    CombatEngine fight = make_combat(CharacterId::IRONCLAD);
    fight.enemies[0].damage_percent = 120;
    assert(fight.enemies[0].intent_damage(fight.player) == 13);
    fight.end_player_turn();
    assert(fight.player.hp == 80 - 13);

    RunManager boss_a9;
    boss_a9.start_run(CharacterId::IRONCLAD, 9, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(boss_a9, RoomType::BOSS);
    // Each boss takes the Ascension HP of its sheet: the Lagavulin Matriarch, the Soul Fysh, the Waterfall Giant, the
    // Ceremonial Beast.
    const Enemy& boss = boss_a9.combat.enemies[0];
    if (boss.type == EnemyType::LAGAVULIN_MATRIARCH) assert(boss.max_hp == 233);
    else if (boss.type == EnemyType::SOUL_FYSH) assert(boss.max_hp == 221);
    else if (boss.type == EnemyType::WATERFALL_GIANT) assert(boss.max_hp == 250);
    else assert(boss.max_hp == 262);

    // After an act boss: full heal, only 75 % of the missing HP from A5.
    RunManager rested;
    rested.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(rested, RoomType::BOSS);
    rested.combat.player.hp = 30;
    rested.proceed_after_combat();
    // The heal comes with the rewards of the boss; the next act opens once they are taken or skipped.
    assert(rested.current_act == 1 && rested.hp == 80 && rested.phase == RunPhase::REWARD_SCREEN);
    rested.skip_card_rewards();
    assert(rested.current_act == 2 && rested.hp == 80);

    RunManager tired;
    tired.start_run(CharacterId::IRONCLAD, 5, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(tired, RoomType::BOSS);
    tired.combat.player.hp = 30;
    tired.proceed_after_combat();
    assert(tired.hp == 36 + (80 - 36) * 3 / 4); // Burning Blood heals 6 first
    std::cout << "[PASS] Ascension: elite/monster/boss damage (A2-A4) and HP (A7-A9) bonuses, intent damage, post-boss heal (A5)" << std::endl;
}

// A copy of `id` holding an enchantment (mechanic #107).
DeckCard enchanted(CardId id, Enchantment enchantment, uint8_t amount = 0) {
    DeckCard card(id);
    card.enchantment = enchantment;
    card.enchant_amount = amount;
    return card;
}

// A fight whose draw pile holds the starter deck and `card` on top of nothing in particular, opened afresh so the
// effects of the start of the combat (Innate, Imbued) apply to it.
CombatEngine open_with(CharacterId hero, DeckCard card) {
    CombatEngine combat = make_combat(hero);
    while (!combat.hand.empty()) {
        combat.draw_pile.push_back(combat.hand.back());
        combat.hand.pop_back();
    }
    // At the bottom of the draw pile (its front): drawn last.
    FixedVector<DeckCard, 50> pile;
    pile.push_back(card);
    for (const DeckCard& other : combat.draw_pile) pile.push_back(other);
    combat.draw_pile = pile;
    combat.reset_combat_state();
    combat.player.block = 0;
    combat.begin_first_turn();
    return combat;
}

int count_enchanted(const FixedVector<DeckCard, 10>& pile, Enchantment enchantment) {
    int count = 0;
    for (const DeckCard& card : pile) count += card.enchantment == enchantment;
    return count;
}

void test_enchantment_damage() {
    // Sharp (Tranchant): every hit of the card deals N more damage.
    CombatEngine sharp = make_combat(CharacterId::IRONCLAD);
    sharp.enemies[0].hp = 200;
    play(sharp, enchanted(CardId::STRIKE, Enchantment::SHARP, 3));
    assert(sharp.enemies[0].hp == 200 - 9);
    play(sharp, enchanted(CardId::TWIN_STRIKE, Enchantment::SHARP, 3));
    assert(sharp.enemies[0].hp == 200 - 9 - 2 * 8);
    // The enchantment belongs to the copy: a plain Strike deals its 6.
    play(sharp, CardId::STRIKE);
    assert(sharp.enemies[0].hp == 200 - 9 - 16 - 6);

    // Vigorous (Vigoureux): only the first play of the combat deals N more damage.
    CombatEngine vigorous = make_combat(CharacterId::IRONCLAD);
    vigorous.enemies[0].hp = 200;
    play(vigorous, enchanted(CardId::STRIKE, Enchantment::VIGOROUS, 8));
    assert(vigorous.enemies[0].hp == 200 - 14 && vigorous.discard_pile.back().enchant_combat == 1);
    play(vigorous, vigorous.discard_pile.back());
    assert(vigorous.enemies[0].hp == 200 - 14 - 6);
    assert(can_enchant(DeckCard{ CardId::STRIKE, false }, Enchantment::VIGOROUS));
    assert(!can_enchant(DeckCard{ CardId::DEFEND, false }, Enchantment::VIGOROUS));

    // Instinct: the damage of the card is doubled, Strength included.
    CombatEngine instinct = make_combat(CharacterId::IRONCLAD);
    instinct.enemies[0].hp = 200;
    instinct.player.add_status(StatusType::STRENGTH, 1);
    play(instinct, enchanted(CardId::STRIKE, Enchantment::INSTINCT));
    assert(instinct.enemies[0].hp == 200 - 14);

    // Tezcatara's Ember: costs 0, deals N more damage, and is Eternal.
    CombatEngine ember = make_combat(CharacterId::IRONCLAD);
    ember.enemies[0].hp = 200;
    const DeckCard bash = enchanted(CardId::BASH, Enchantment::TEZCATARAS_EMBER, 4);
    assert(ember.effective_cost(bash) == 0 && CardDef::get_cost(bash) == 0 && CardDef::is_eternal(bash));
    assert(!CardDef::is_eternal(DeckCard(CardId::BASH)));
    play(ember, bash);
    assert(ember.enemies[0].hp == 200 - 12 && ember.player.energy == ember.player.max_energy);

    // Momentum (Élan): each play adds N to the damage of that copy, for the rest of the combat only.
    CombatEngine momentum = make_combat(CharacterId::IRONCLAD);
    momentum.enemies[0].hp = 200;
    play(momentum, enchanted(CardId::STRIKE, Enchantment::MOMENTUM, 5));
    assert(momentum.enemies[0].hp == 200 - 6);
    DeckCard grown = momentum.discard_pile.back();
    assert(grown.enchant_combat == 5 && grown.enchant_amount == 5);
    play(momentum, grown);
    assert(momentum.enemies[0].hp == 200 - 6 - 11);
    assert(momentum.discard_pile.back().enchant_combat == 10);

    // Inky (Encrée): the card applies Weak to the enemies it hits, on any card.
    CombatEngine inky = make_combat(CharacterId::IRONCLAD);
    play(inky, enchanted(CardId::STRIKE, Enchantment::INKY));
    assert(inky.enemies[0].get_status(StatusType::WEAK) == INKY_WEAK);
    CombatEngine plain = make_combat(CharacterId::IRONCLAD);
    play(plain, CardId::STRIKE);
    assert(plain.enemies[0].get_status(StatusType::WEAK) == 0);

    // Corrupted (Corruption, Symbiote): the Attack deals 50% more damage and costs 2 HP each time it is played.
    CombatEngine corrupted = make_combat(CharacterId::IRONCLAD);
    corrupted.enemies[0].hp = 200;
    const int16_t hp_before = corrupted.player.hp;
    play(corrupted, enchanted(CardId::STRIKE, Enchantment::CORRUPTED));
    assert(corrupted.enemies[0].hp == 200 - 9 && corrupted.player.hp == hp_before - CORRUPTED_HP_LOSS);
    play(corrupted, CardId::STRIKE);
    assert(corrupted.enemies[0].hp == 200 - 9 - 6 && corrupted.player.hp == hp_before - CORRUPTED_HP_LOSS);
    assert(!can_enchant(DeckCard(CardId::DEFEND), Enchantment::CORRUPTED));
    // Perfect Fit (Ajustement parfait): once the draw pile is shuffled, the card is on top of it (the back).
    CombatEngine fit = make_combat(CharacterId::IRONCLAD);
    while (!fit.hand.empty()) {
        fit.discard_pile.push_back(fit.hand.back());
        fit.hand.pop_back();
    }
    fit.draw_pile.clear();
    for (int i = 0; i < 8; ++i) fit.discard_pile.push_back(DeckCard(CardId::DEFEND));
    fit.discard_pile.push_back(enchanted(CardId::BASH, Enchantment::PERFECT_FIT));
    fit.discard_pile.push_back(enchanted(CardId::STRIKE, Enchantment::PERFECT_FIT));
    for (int i = 0; i < 8; ++i) fit.discard_pile.push_back(DeckCard(CardId::DEFEND));
    fit.shuffle_discard_into_draw();
    const size_t top = fit.draw_pile.size();
    assert(fit.draw_pile[top - 1].enchantment == Enchantment::PERFECT_FIT && fit.draw_pile[top - 2].enchantment == Enchantment::PERFECT_FIT);
    assert(fit.draw_pile[top - 3].enchantment == Enchantment::NONE);
    std::cout << "[PASS] Enchantments on damage: Sharp, Instinct, Tezcatara's Ember, Momentum, Inky, Corrupted" << std::endl;
}

void test_enchantment_block() {
    // Adroit: the card also gains N Block, Dexterity applied.
    CombatEngine adroit = make_combat(CharacterId::IRONCLAD);
    play(adroit, enchanted(CardId::STRIKE, Enchantment::ADROIT, 4));
    assert(adroit.player.block == 4 && adroit.enemies[0].hp == 42 - 6);

    // Nimble (Agilité): the Block the card gives is raised by N.
    CombatEngine nimble = make_combat(CharacterId::IRONCLAD);
    play(nimble, enchanted(CardId::DEFEND, Enchantment::NIMBLE, 2));
    assert(nimble.player.block == 5 + 2);
    // A card that gives no Block gains none.
    play(nimble, enchanted(CardId::STRIKE, Enchantment::NIMBLE, 2));
    assert(nimble.player.block == 7);

    // Goopy (Viscosité): the card exhausts, and each play adds 1 to its Block for good.
    CombatEngine goopy = make_combat(CharacterId::IRONCLAD);
    DeckCard defend = enchanted(CardId::DEFEND, Enchantment::GOOPY);
    defend.deck_index = 4;
    assert(CardDef::has_keyword(defend, KEYWORD_EXHAUST));
    play(goopy, defend);
    assert(goopy.player.block == 5 && goopy.exhaust_pile.back().enchant_amount == 1);
    assert(goopy.permanent_gains.size() == 1 && goopy.permanent_gains[0].deck_index == 4 && goopy.permanent_gains[0].enchantment);
    play(goopy, goopy.exhaust_pile.back());
    assert(goopy.player.block == 5 + 6);

    // Soul's Power (Pouvoir de l'âme): an Exhaust card is discarded instead of exhausted, and only such a card can have it.
    CombatEngine soul = make_combat(CharacterId::IRONCLAD);
    DeckCard impervious = enchanted(CardId::IMPERVIOUS, Enchantment::SOULS_POWER);
    assert(CardDef::has_keyword(CardId::IMPERVIOUS, false, KEYWORD_EXHAUST) && !CardDef::has_keyword(impervious, KEYWORD_EXHAUST));
    assert(can_enchant(DeckCard(CardId::IMPERVIOUS), Enchantment::SOULS_POWER) && !can_enchant(DeckCard(CardId::STRIKE), Enchantment::SOULS_POWER));
    play(soul, impervious);
    assert(soul.player.block == 30 && soul.exhaust_pile.empty() && soul.discard_pile.back().id == CardId::IMPERVIOUS);
    std::cout << "[PASS] Enchantments on Block: Adroit, Nimble, Goopy" << std::endl;
}

void test_enchantment_plays() {
    // Swift (Célérité): the first play of the combat draws N cards, the next ones draw none.
    CombatEngine swift = make_combat(CharacterId::IRONCLAD);
    play(swift, enchanted(CardId::STRIKE, Enchantment::SWIFT, 2));
    assert(swift.hand.size() == 2 && swift.discard_pile.back().enchant_combat == 1);
    play(swift, swift.discard_pile.back());
    assert(swift.hand.empty());

    // Glam (Glamour): Echo 1 once per combat, the first time the card is played.
    CombatEngine glam = make_combat(CharacterId::IRONCLAD);
    glam.enemies[0].hp = 200;
    play(glam, enchanted(CardId::STRIKE, Enchantment::GLAM));
    assert(glam.enemies[0].hp == 200 - 12);
    play(glam, glam.discard_pile.back());
    assert(glam.enemies[0].hp == 200 - 18);

    // Royally Approved (Sceau royal): Innate and Retain. Drawn in the opening hand from the bottom of the pile,
    // and kept at the end of the turn.
    DeckCard royal = enchanted(CardId::STRIKE, Enchantment::ROYALLY_APPROVED);
    assert(CardDef::has_keyword(royal, KEYWORD_INNATE) && CardDef::has_keyword(royal, KEYWORD_RETAIN));
    CombatEngine stamp = open_with(CharacterId::IRONCLAD, royal);
    assert(count_enchanted(stamp.hand, Enchantment::ROYALLY_APPROVED) == 1);
    make_enemy_idle(stamp.enemies[0]);
    stamp.end_player_turn();
    assert(count_enchanted(stamp.hand, Enchantment::ROYALLY_APPROVED) == 1);

    // Imbued (Imprégnation): played for free at the start of the combat, without taking a place in the hand.
    CombatEngine imbued = open_with(CharacterId::IRONCLAD, enchanted(CardId::DEFEND, Enchantment::IMBUED));
    assert(imbued.player.block == 5 && imbued.hand.size() == 5 && imbued.player.energy == imbued.player.max_energy);
    assert(imbued.discard_pile.size() == 1 && imbued.discard_pile[0].enchantment == Enchantment::IMBUED);
    // An Imbued Attack hits an enemy.
    CombatEngine strike = open_with(CharacterId::IRONCLAD, enchanted(CardId::STRIKE, Enchantment::IMBUED));
    assert(strike.enemies[0].hp == strike.enemies[0].max_hp - 6);
    std::cout << "[PASS] Enchantments on plays: Swift, Glam, Royally Approved, Imbued" << std::endl;
}

void test_enchantments_in_the_deck() {
    // A card holds one enchantment, never on a Curse or a Status, and the damage ones only on an Attack.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.master_deck.clear();
    run.master_deck.push_back(CardId::STRIKE);
    run.master_deck.push_back(CardId::DEFEND);
    run.master_deck.push_back(CardId::REGRET);
    assert(!run.enchant_card_in_deck(1, Enchantment::SHARP, 3));
    assert(!run.enchant_card_in_deck(2, Enchantment::CLONE));
    assert(run.enchant_card_in_deck(0, Enchantment::SHARP, 3));
    assert(!run.enchant_card_in_deck(0, Enchantment::INSTINCT));
    assert(run.master_deck[0].enchantment == Enchantment::SHARP && run.master_deck[0].enchant_amount == 3);

    // Clone (Clonage): the rest site offers to duplicate the card, enchantment included, which uses it up.
    assert(run.campfire_actions().size() == 2);
    assert(run.enchant_card_in_deck(1, Enchantment::CLONE));
    auto actions = run.campfire_actions();
    assert(actions.size() == 3 && actions[2] == CampfireAction::CLONE);
    assert(!run.can_clone(0) && run.can_clone(1));
    run.phase = RunPhase::CAMPFIRE;
    assert(!run.clone_at_campfire(0));
    assert(run.clone_at_campfire(1));
    assert(run.master_deck.size() == 4 && run.master_deck[3].id == CardId::DEFEND);
    assert(run.master_deck[3].enchantment == Enchantment::CLONE && run.phase == RunPhase::MAP_NAVIGATION);

    // The enchantment follows the card into the fight; what Goopy adds stays on the card of the deck.
    run.master_deck[1].enchantment = Enchantment::GOOPY;
    enter_room_of_type(run, RoomType::MONSTER);
    bool found = false;
    for (const DeckCard& card : run.combat.draw_pile) found |= card.enchantment == Enchantment::SHARP;
    for (const DeckCard& card : run.combat.hand) found |= card.enchantment == Enchantment::SHARP;
    assert(found);
    run.combat.permanent_gains.push_back(PermanentGain{ 1, 1, true });
    run.proceed_after_combat();
    assert(run.master_deck[1].enchant_amount == 1 && run.master_deck[1].bonus == 0);
    std::cout << "[PASS] Enchantments in the deck: one per card, Clone at the rest site, Goopy kept by the deck" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Combat Rules Audit ===" << std::endl;
    test_attack_formula();
    test_turn_flow();
    test_cultist_ritual();
    test_ironclad();
    test_ironclad_commons();
    test_silent();
    test_silent_commons();
    test_defect();
    test_defect_commons();
    test_regent();
    test_regent_commons();
    test_necrobinder();
    test_necrobinder_commons();
    test_more_commons();
    test_ironclad_existing_mechanics();
    test_silent_existing_mechanics();
    test_defect_existing_mechanics();
    test_necrobinder_existing_mechanics();
    test_exhaust();
    test_ironclad_exhaust();
    test_other_heroes_exhaust();
    test_card_keywords();
    test_x_cost();
    test_delayed_effects();
    test_temporary_stats();
    test_hp_loss();
    test_counters();
    test_regent_stars();
    test_fatal();
    test_play_conditions();
    test_power_registry();
    test_turn_powers();
    test_reaction_powers();
    test_turn_relics();
    test_event_relics();
    test_orb_operations();
    test_doom();
    test_orb_slots();
    test_hero_relics();
    test_replays();
    test_cost_changes();
    test_evolving_cards();
    test_status_cards();
    test_status_card_users();
    test_shivs();
    test_hand_choices();
    test_pile_choices();
    test_ascension();
    test_enchantment_damage();
    test_enchantment_block();
    test_enchantment_plays();
    test_enchantments_in_the_deck();
    std::cout << "=== All Combat Rules Passed! ===" << std::endl;
    return 0;
}
