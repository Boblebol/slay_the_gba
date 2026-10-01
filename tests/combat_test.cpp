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

    // Frost passive block (2, Dexterity ignored) protects during the enemy turn.
    CombatEngine frost = make_combat(CharacterId::DEFECT);
    frost.player.add_status(StatusType::DEXTERITY, 5);
    frost.player.channel_orb(OrbType::FROST);
    frost.end_player_turn();
    assert(frost.player.hp == 75 - (11 - 2));

    // Dualcast evokes the same orb twice.
    CombatEngine dual = make_combat(CharacterId::DEFECT);
    dual.player.channel_orb(OrbType::LIGHTNING);
    dual.player.channel_orb(OrbType::FROST);
    play(dual, CardId::DUALCAST);
    assert(dual.enemies[0].hp == 42 - 16);
    assert(dual.player.orbs.size() == 1 && dual.player.orbs[0] == OrbType::FROST);
    assert(dual.player.block == 0);

    // Dark charges +6 per turn and deals its charge on evoke.
    CombatEngine dark = make_combat(CharacterId::DEFECT);
    make_enemy_idle(dark.enemies[0]);
    dark.player.channel_orb(OrbType::DARK);
    assert(dark.player.dark_orb_charge == 6);
    dark.end_player_turn();
    assert(dark.player.dark_orb_charge == 12);
    play(dark, CardId::DUALCAST);
    assert(dark.enemies[0].hp == 42 - 24);

    // Channeling into full slots evokes the oldest orb first.
    CombatEngine full = make_combat(CharacterId::DEFECT);
    Enemy& worm = full.enemies[0];
    for (int i = 0; i < 3; ++i) full.player.channel_orb(OrbType::LIGHTNING, &worm);
    assert(worm.hp == 42);
    full.player.channel_orb(OrbType::FROST, &worm);
    assert(worm.hp == 34);
    assert(full.player.orbs.size() == 3 && full.player.orbs[2] == OrbType::FROST);

    // Plasma: +1 energy at the start of each turn, +2 on evoke.
    CombatEngine plasma = make_combat(CharacterId::DEFECT);
    plasma.player.channel_orb(OrbType::PLASMA);
    plasma.end_player_turn();
    assert(plasma.player.energy == 4);
    plasma.player.evoke_oldest_orb();
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
    CombatEngine combat = make_combat(CharacterId::REGENT);
    Enemy& worm = combat.enemies[0];
    play(combat, CardId::FALLING_STAR);
    assert(worm.hp == 35 && combat.player.sovereign_stars == 1);

    worm.add_status(StatusType::VULNERABLE, 1);
    play(combat, CardId::FALLING_STAR);
    assert(worm.hp == 35 - 10); // Vulnerable now applies to Falling Star too

    play(combat, CardId::VENERATE);
    assert(combat.player.block == 6 && combat.player.sovereign_stars == 4);
    combat.player.add_stars(10);
    assert(combat.player.sovereign_stars == 5);

    // Stars empower the basic Strike and Defend.
    CombatEngine stars = make_combat(CharacterId::REGENT);
    stars.player.add_stars(2);
    play(stars, CardId::STRIKE);
    play(stars, CardId::DEFEND);
    assert(stars.enemies[0].hp == 42 - 8 && stars.player.block == 7);
    std::cout << "[PASS] Regent: Falling Star + Vulnerable, Venerate, Stars cap 5, Stars on Strike/Defend" << std::endl;
}

void test_regent_commons() {
    // Solar Strike hits and gains Stars (capped at 5 like the other Star cards).
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
    play(combat, CardId::UNLEASH);
    assert(combat.enemies[0].hp == 42 - 12); // Vulnerable now applies to Unleash too

    // Bodyguard: Osty soaks the hit first, then the block, then the Necrobinder.
    CombatEngine guard = make_combat(CharacterId::NECROBINDER);
    play(guard, CardId::BODYGUARD);
    assert(guard.player.block == 7 && guard.player.minion_hp == 1);
    guard.end_player_turn();
    assert(guard.player.minion_hp == 0);
    assert(guard.player.hp == 65 - (11 - 1 - 7));
    std::cout << "[PASS] Necrobinder: Unleash + Vulnerable, Bodyguard block and Osty absorbing damage" << std::endl;
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
    assert(bury.enemies[0].hp == 150 - 52);

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

void test_ascension() {
    // A0: enemies as designed.
    RunManager normal;
    normal.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(normal, RoomType::ELITE);
    assert(normal.combat.enemies[0].max_hp == 82 && normal.combat.enemies[0].damage_percent == 100);

    // A3: elites hit 20 % harder but keep their HP until A8.
    RunManager a3;
    a3.start_run(CharacterId::IRONCLAD, 3, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(a3, RoomType::ELITE);
    assert(a3.combat.enemies[0].max_hp == 82 && a3.combat.enemies[0].damage_percent == 120);

    // A10: every room is tougher; the intent shown is the damage really dealt (Chomp 11 x 1.2 = 13).
    RunManager a10;
    a10.start_run(CharacterId::IRONCLAD, 10, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(a10, RoomType::MONSTER);
    const Enemy& cultist = a10.combat.enemies[0];
    assert(cultist.max_hp == 50 * 115 / 100 && cultist.hp == cultist.max_hp && cultist.damage_percent == 120);

    CombatEngine fight = make_combat(CharacterId::IRONCLAD);
    fight.enemies[0].damage_percent = 120;
    assert(fight.enemies[0].intent_damage(fight.player) == 13);
    fight.end_player_turn();
    assert(fight.player.hp == 80 - 13);

    RunManager boss_a9;
    boss_a9.start_run(CharacterId::IRONCLAD, 9, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(boss_a9, RoomType::BOSS);
    assert(boss_a9.combat.enemies[0].max_hp == 150 * 115 / 100);

    // After an act boss: full heal, only 75 % of the missing HP from A5.
    RunManager rested;
    rested.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(rested, RoomType::BOSS);
    rested.combat.player.hp = 30;
    rested.proceed_after_combat();
    assert(rested.current_act == 2 && rested.hp == 80);

    RunManager tired;
    tired.start_run(CharacterId::IRONCLAD, 5, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(tired, RoomType::BOSS);
    tired.combat.player.hp = 30;
    tired.proceed_after_combat();
    assert(tired.hp == 36 + (80 - 36) * 3 / 4); // Burning Blood heals 6 first
    std::cout << "[PASS] Ascension: elite/monster/boss damage (A2-A4) and HP (A7-A9) bonuses, intent damage, post-boss heal (A5)" << std::endl;
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
    test_ascension();
    std::cout << "=== All Combat Rules Passed! ===" << std::endl;
    return 0;
}
