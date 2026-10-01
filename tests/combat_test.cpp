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
    test_exhaust();
    test_ironclad_exhaust();
    test_other_heroes_exhaust();
    test_card_keywords();
    test_x_cost();
    test_delayed_effects();
    test_temporary_stats();
    test_hp_loss();
    test_counters();
    test_ascension();
    std::cout << "=== All Combat Rules Passed! ===" << std::endl;
    return 0;
}
