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
    test_ascension();
    std::cout << "=== All Combat Rules Passed! ===" << std::endl;
    return 0;
}
