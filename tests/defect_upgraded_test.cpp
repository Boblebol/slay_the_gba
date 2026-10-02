// Defect upgrades: each card of the hero whose upgraded version is played by no other test, played upgraded on a
// hand-built situation. Expected values come from the CardDef rows of src/engine/card.cpp, which match
// bible/data/card_v0.111.0.json.
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

// Puts `card` alone in hand with full energy and plays it.
void play(CombatEngine& combat, DeckCard card, int target = -1) {
    combat.hand.clear();
    combat.hand.push_back(card);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, target);
    assert(played);
}

// Same, with the given amount of energy instead of the full pool.
void play_with_energy(CombatEngine& combat, DeckCard card, int energy) {
    combat.hand.clear();
    combat.hand.push_back(card);
    combat.player.energy = static_cast<int8_t>(energy);
    bool played = combat.play_card(0);
    assert(played);
}

// Whether the card can be played with exactly that much energy; the hand is left as it was if not.
bool playable_with_energy(CombatEngine& combat, DeckCard card, int energy) {
    combat.hand.clear();
    combat.hand.push_back(card);
    combat.player.energy = static_cast<int8_t>(energy);
    return combat.play_card(0);
}

// Energy a play took, for a play() that started from the full pool.
int spent(const CombatEngine& combat) {
    return combat.player.max_energy - combat.player.energy;
}

void make_enemy_idle(Enemy& enemy) {
    enemy.current_intent = EnemyIntent{};
    enemy.current_intent.type = EnemyIntentType::DEFEND;
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

void assert_in_pool(std::initializer_list<CardId> cards) {
    std::span<const CardId> pool = RunManager::card_reward_pool(CharacterId::DEFECT);
    for (CardId id : cards) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
}

void test_upgrade_costs() {
    // Zap and Dualcast come with the starting deck.
    RunManager run;
    run.start_run(CharacterId::DEFECT, 0, PaletteVariant::CLASSIC, 1);
    int zaps = 0, dualcasts = 0;
    for (const DeckCard& card : run.master_deck) {
        zaps += card.id == CardId::ZAP;
        dualcasts += card.id == CardId::DUALCAST;
    }
    assert(zaps == 1 && dualcasts == 1);

    // Zap costs 1, Zap+ is free; both channel one Lightning.
    CombatEngine zap = make_combat(CharacterId::DEFECT);
    play(zap, CardId::ZAP);
    assert(spent(zap) == 1 && zap.player.orbs.size() == 1 && zap.player.orbs[0] == OrbType::LIGHTNING);
    CombatEngine zap_plus = make_combat(CharacterId::DEFECT);
    play(zap_plus, DeckCard(CardId::ZAP, true));
    assert(spent(zap_plus) == 0 && zap_plus.player.orbs.size() == 1 && zap_plus.player.orbs[0] == OrbType::LIGHTNING);
    assert(!playable_with_energy(zap, CardId::ZAP, 0) && zap.hand.size() == 1);
    assert(playable_with_energy(zap_plus, DeckCard(CardId::ZAP, true), 0));

    // Dualcast+ is free and still evokes the oldest orb twice (Lightning: 8 + 8), leaving the Frost orb.
    CombatEngine dual = make_combat(CharacterId::DEFECT);
    dual.channel_orb(OrbType::LIGHTNING);
    dual.channel_orb(OrbType::FROST);
    play(dual, DeckCard(CardId::DUALCAST, true));
    assert(spent(dual) == 0);
    assert(dual.enemies[0].hp == 42 - 16);
    assert(dual.player.orbs.size() == 1 && dual.player.orbs[0] == OrbType::FROST);
    CombatEngine dual_base = make_combat(CharacterId::DEFECT);
    dual_base.channel_orb(OrbType::LIGHTNING);
    play(dual_base, CardId::DUALCAST);
    assert(spent(dual_base) == 1 && dual_base.enemies[0].hp == 42 - 16);
    assert(!playable_with_energy(dual_base, CardId::DUALCAST, 0));
    assert(playable_with_energy(dual, DeckCard(CardId::DUALCAST, true), 0));

    assert_in_pool({ CardId::ZAP, CardId::DUALCAST });
    std::cout << "[PASS] Defect upgrades, cost: Zap+ and Dualcast+ are free" << std::endl;
}

void test_upgraded_free_attacks() {
    // Beam Cell+: 4 damage and 2 Vulnerable for free.
    CombatEngine beam = make_combat(CharacterId::DEFECT);
    play_with_energy(beam, DeckCard(CardId::BEAM_CELL, true), 0);
    assert(beam.enemies[0].hp == 42 - 4);
    assert(beam.enemies[0].get_status(StatusType::VULNERABLE) == 2);
    assert(beam.player.energy == 0);

    // Go for the Eyes+: 4 damage, and 2 Weak only when the enemy is about to attack.
    CombatEngine eyes = make_combat(CharacterId::DEFECT);
    play_with_energy(eyes, DeckCard(CardId::GO_FOR_THE_EYES, true), 0);
    assert(eyes.enemies[0].hp == 42 - 4 && eyes.enemies[0].get_status(StatusType::WEAK) == 2);
    CombatEngine calm = make_combat(CharacterId::DEFECT);
    make_enemy_idle(calm.enemies[0]);
    play_with_energy(calm, DeckCard(CardId::GO_FOR_THE_EYES, true), 0);
    assert(calm.enemies[0].hp == 42 - 4 && calm.enemies[0].get_status(StatusType::WEAK) == 0);

    // Claw+: 4 damage; every other Claw gains 3 damage for the combat, and the played one keeps it too.
    CombatEngine claw = make_combat(CharacterId::DEFECT);
    claw.draw_pile.clear();
    claw.discard_pile.clear();
    claw.draw_pile.push_back(CardId::CLAW);
    play_with_energy(claw, DeckCard(CardId::CLAW, true), 0);
    assert(claw.enemies[0].hp == 42 - 4);
    assert(claw.draw_pile[0].bonus == 3 && claw.discard_pile[0].bonus == 3);
    DeckCard grown = claw.draw_pile[0];
    claw.draw_pile.clear();
    play_with_energy(claw, grown, 0);
    assert(claw.enemies[0].hp == 38 - (3 + 3));
    // The plain Claw played in between raised the Claw+ waiting in the discard pile by its own 2: 3 + 2.
    assert(claw.discard_pile[0].bonus == 5);
    play_with_energy(claw, claw.discard_pile[0], 0);
    assert(claw.enemies[0].hp == 32 - (4 + 5));

    assert_in_pool({ CardId::BEAM_CELL, CardId::GO_FOR_THE_EYES, CardId::CLAW });
    std::cout << "[PASS] Defect upgrades, free attacks: Beam Cell+, Go for the Eyes+, Claw+" << std::endl;
}

void test_upgraded_orb_attacks() {
    // Ball Lightning+: 10 damage, one Lightning orb, still 1 energy.
    CombatEngine ball = make_combat(CharacterId::DEFECT);
    play(ball, DeckCard(CardId::BALL_LIGHTNING, true));
    assert(ball.enemies[0].hp == 42 - 10 && spent(ball) == 1);
    assert(ball.player.orbs.size() == 1 && ball.player.orbs[0] == OrbType::LIGHTNING);

    // Sweeping Beam+: 9 damage to every enemy and still one card drawn.
    CombatEngine sweep = make_pack(CharacterId::DEFECT);
    pad_draw_pile(sweep, 5);
    play(sweep, DeckCard(CardId::SWEEPING_BEAM, true));
    assert(sweep.enemies[0].hp == 18 - 9 && sweep.enemies[1].hp == 18 - 9 && sweep.enemies[2].hp == 42 - 9);
    assert(sweep.hand.size() == 1 && spent(sweep) == 1);

    // Meteor Strike+: still 5 energy, now 30 damage, and 3 Plasma orbs.
    CombatEngine meteor = make_combat(CharacterId::DEFECT);
    assert(!playable_with_energy(meteor, DeckCard(CardId::METEOR_STRIKE, true), 4));
    assert(meteor.hand.size() == 1);
    play_with_energy(meteor, DeckCard(CardId::METEOR_STRIKE, true), 5);
    assert(meteor.enemies[0].hp == 42 - 30 && meteor.player.energy == 0);
    assert(meteor.player.orbs.size() == 3);
    for (OrbType orb : meteor.player.orbs) assert(orb == OrbType::PLASMA);

    // Ice Lance+: still 3 energy, now 24 damage, and 3 Frost orbs.
    CombatEngine lance = make_combat(CharacterId::DEFECT);
    assert(!playable_with_energy(lance, DeckCard(CardId::ICE_LANCE, true), 2));
    play_with_energy(lance, DeckCard(CardId::ICE_LANCE, true), 3);
    assert(lance.enemies[0].hp == 42 - 24 && lance.player.energy == 0);
    assert(lance.player.orbs.size() == 3);
    for (OrbType orb : lance.player.orbs) assert(orb == OrbType::FROST);

    // Compile Driver+: 10 damage, then one card per orb type held (Lightning and Frost: 2).
    CombatEngine driver = make_combat(CharacterId::DEFECT);
    pad_draw_pile(driver, 5);
    driver.channel_orb(OrbType::LIGHTNING);
    driver.channel_orb(OrbType::FROST);
    play(driver, DeckCard(CardId::COMPILE_DRIVER, true));
    assert(driver.enemies[0].hp == 42 - 10 && driver.hand.size() == 2 && spent(driver) == 1);

    // Shadow Shield+: 15 Block and a Dark orb for 2 energy.
    CombatEngine shield = make_combat(CharacterId::DEFECT);
    shield.player.block = 0;
    play(shield, DeckCard(CardId::SHADOW_SHIELD, true));
    assert(shield.player.block == 15 && spent(shield) == 2);
    assert(shield.player.orbs.size() == 1 && shield.player.orbs[0] == OrbType::DARK);

    assert_in_pool({ CardId::BALL_LIGHTNING, CardId::SWEEPING_BEAM, CardId::METEOR_STRIKE, CardId::ICE_LANCE,
                     CardId::COMPILE_DRIVER, CardId::SHADOW_SHIELD });
    std::cout << "[PASS] Defect upgrades, orb cards: Ball Lightning+, Sweeping Beam+, Meteor Strike+, Ice Lance+, "
                 "Compile Driver+, Shadow Shield+" << std::endl;
}

void test_upgraded_pile_cards() {
    // All for One+: 14 damage, then every card costing 0 comes back, an upgraded Zap included (it is free).
    CombatEngine all = make_combat(CharacterId::DEFECT);
    all.discard_pile.clear();
    all.discard_pile.push_back(CardId::BEAM_CELL);
    all.discard_pile.push_back(CardId::STRIKE);
    all.discard_pile.push_back(CardId::ZAP);
    all.discard_pile.push_back(DeckCard(CardId::ZAP, true));
    play(all, DeckCard(CardId::ALL_FOR_ONE, true));
    assert(all.enemies[0].hp == 42 - 14 && spent(all) == 2);
    assert(all.hand.size() == 2 && count_in_hand(all, CardId::BEAM_CELL) == 1 && count_in_hand(all, CardId::ZAP) == 1);
    assert(all.hand[1].upgraded);
    assert(all.discard_pile.size() == 3); // the Strike, the plain Zap and All for One+ itself

    // Scrape+: 10 damage, draws 5 and keeps only the cards costing 0.
    CombatEngine scrape = make_combat(CharacterId::DEFECT);
    scrape.draw_pile.clear();
    scrape.discard_pile.clear();
    scrape.draw_pile.push_back(CardId::STRIKE);
    scrape.draw_pile.push_back(CardId::BEAM_CELL);
    scrape.draw_pile.push_back(CardId::DEFEND);
    scrape.draw_pile.push_back(CardId::GO_FOR_THE_EYES);
    scrape.draw_pile.push_back(CardId::SUPERCRITICAL);
    play(scrape, DeckCard(CardId::SCRAPE, true));
    assert(scrape.enemies[0].hp == 42 - 10 && spent(scrape) == 1);
    assert(scrape.draw_pile.empty()); // the 5 cards were all drawn
    assert(scrape.hand.size() == 3);
    assert(scrape.discard_pile.size() == 3); // Strike, Defend and Scrape+ itself

    // Reboot+: free, draws 6 instead of 4 out of the whole deck, and still exhausts.
    CombatEngine reboot = make_combat(CharacterId::DEFECT);
    reboot.draw_pile.clear();
    reboot.discard_pile.clear();
    for (int i = 0; i < 5; ++i) reboot.draw_pile.push_back(CardId::STRIKE);
    for (int i = 0; i < 5; ++i) reboot.discard_pile.push_back(CardId::DEFEND);
    play(reboot, DeckCard(CardId::REBOOT, true));
    assert(spent(reboot) == 0);
    assert(reboot.hand.size() == 6 && reboot.draw_pile.size() == 4 && reboot.discard_pile.empty());
    assert(reboot.exhaust_pile.size() == 1 && reboot.exhaust_pile[0].id == CardId::REBOOT);

    // Adaptive Strike+: 23 damage; the copy it leaves in the discard pile is upgraded and costs 0.
    CombatEngine adaptive = make_combat(CharacterId::DEFECT);
    adaptive.discard_pile.clear();
    play(adaptive, DeckCard(CardId::ADAPTIVE_STRIKE, true));
    assert(adaptive.enemies[0].hp == 42 - 23 && spent(adaptive) == 2);
    assert(adaptive.discard_pile.size() == 2);
    int free_copies = 0;
    for (const DeckCard& card : adaptive.discard_pile) {
        assert(card.id == CardId::ADAPTIVE_STRIKE && card.upgraded);
        free_copies += adaptive.effective_cost(card) == 0;
    }
    assert(free_copies == 1);

    // Momentum Strike+: 15 damage for 1 energy, then the card costs 0.
    CombatEngine momentum = make_combat(CharacterId::DEFECT);
    momentum.discard_pile.clear();
    play(momentum, DeckCard(CardId::MOMENTUM_STRIKE, true));
    assert(momentum.enemies[0].hp == 42 - 15 && spent(momentum) == 1);
    assert(momentum.discard_pile.size() == 1 && momentum.effective_cost(momentum.discard_pile[0]) == 0);
    play_with_energy(momentum, momentum.discard_pile[0], 0);
    assert(momentum.enemies[0].hp == 27 - 15 && momentum.player.energy == 0);

    // Uproar+: 8 damage twice, then a random Attack from the draw pile (the only one: a Strike).
    CombatEngine uproar = make_combat(CharacterId::DEFECT);
    uproar.draw_pile.clear();
    uproar.draw_pile.push_back(CardId::STRIKE);
    play(uproar, DeckCard(CardId::UPROAR, true));
    assert(uproar.enemies[0].hp == 42 - 8 - 8 - 6 && spent(uproar) == 2);
    assert(uproar.draw_pile.empty());

    // Modded+: one orb slot and 2 cards drawn; the copy costs 1 more afterwards.
    CombatEngine modded = make_combat(CharacterId::DEFECT);
    pad_draw_pile(modded, 5);
    modded.discard_pile.clear();
    uint8_t slots = modded.player.orb_slots;
    play_with_energy(modded, DeckCard(CardId::MODDED, true), 0);
    assert(modded.player.orb_slots == slots + 1 && modded.hand.size() == 2);
    assert(modded.discard_pile.size() == 1 && modded.effective_cost(modded.discard_pile[0]) == 1);

    assert_in_pool({ CardId::ALL_FOR_ONE, CardId::SCRAPE, CardId::REBOOT, CardId::ADAPTIVE_STRIKE,
                     CardId::MOMENTUM_STRIKE, CardId::UPROAR, CardId::MODDED });
    std::cout << "[PASS] Defect upgrades, piles: All for One+, Scrape+, Reboot+, Adaptive Strike+, Momentum Strike+, "
                 "Uproar+, Modded+" << std::endl;
}

void test_upgraded_keywords() {
    // Rainbow+ no longer exhausts: it goes to the discard pile, the plain card to the exhaust pile.
    CombatEngine rainbow = make_combat(CharacterId::DEFECT);
    rainbow.discard_pile.clear();
    play(rainbow, DeckCard(CardId::RAINBOW, true));
    assert(spent(rainbow) == 2 && rainbow.player.orbs.size() == 3);
    assert(rainbow.player.orbs[0] == OrbType::LIGHTNING && rainbow.player.orbs[1] == OrbType::FROST &&
           rainbow.player.orbs[2] == OrbType::DARK);
    assert(rainbow.exhaust_pile.empty() && rainbow.discard_pile.size() == 1 && rainbow.discard_pile[0].upgraded);
    CombatEngine rainbow_base = make_combat(CharacterId::DEFECT);
    play(rainbow_base, CardId::RAINBOW);
    assert(rainbow_base.exhaust_pile.size() == 1);

    // Fusion+: same thing for one Plasma orb.
    CombatEngine fusion = make_combat(CharacterId::DEFECT);
    fusion.discard_pile.clear();
    play(fusion, DeckCard(CardId::FUSION, true));
    assert(spent(fusion) == 1 && fusion.player.orbs.size() == 1 && fusion.player.orbs[0] == OrbType::PLASMA);
    assert(fusion.exhaust_pile.empty() && fusion.discard_pile.size() == 1);
    CombatEngine fusion_base = make_combat(CharacterId::DEFECT);
    play(fusion_base, CardId::FUSION);
    assert(fusion_base.exhaust_pile.size() == 1);

    // Supercritical+ gives 6 energy instead of 4, for free, and exhausts.
    CombatEngine super = make_combat(CharacterId::DEFECT);
    play(super, DeckCard(CardId::SUPERCRITICAL, true));
    assert(super.player.energy == 3 + 6 && super.exhaust_pile.size() == 1);
    CombatEngine super_base = make_combat(CharacterId::DEFECT);
    play(super_base, CardId::SUPERCRITICAL);
    assert(super_base.player.energy == 3 + 4);

    // Boot Sequence+: 13 Block, free, exhausts, and is still Innate.
    assert(CardDef::has_keyword(CardId::BOOT_SEQUENCE, true, KEYWORD_INNATE));
    assert(CardDef::has_keyword(CardId::BOOT_SEQUENCE, true, KEYWORD_EXHAUST));
    CombatEngine boot = make_combat(CharacterId::DEFECT);
    boot.hand.clear();
    boot.discard_pile.clear();
    boot.draw_pile.clear();
    boot.draw_pile.push_back(DeckCard(CardId::BOOT_SEQUENCE, true));
    for (int i = 0; i < 11; ++i) boot.draw_pile.push_back(CardId::STRIKE);
    boot.open_first_turn();
    assert(boot.hand.size() == 5 && count_in_hand(boot, CardId::BOOT_SEQUENCE) == 1);
    boot.player.block = 0;
    play(boot, DeckCard(CardId::BOOT_SEQUENCE, true));
    assert(boot.player.block == 13 && boot.player.energy == 3 && boot.exhaust_pile.size() == 1);

    // Genetic Algorithm+: 1 Block, then +4 for good instead of +3, and it exhausts.
    CombatEngine genetic = make_combat(CharacterId::DEFECT);
    genetic.player.block = 0;
    DeckCard algorithm(CardId::GENETIC_ALGORITHM, true);
    algorithm.deck_index = 3;
    play(genetic, algorithm);
    assert(genetic.player.block == 1 && spent(genetic) == 1);
    assert(genetic.exhaust_pile.size() == 1 && genetic.exhaust_pile[0].bonus == 4);
    assert(genetic.permanent_gains.size() == 1 && genetic.permanent_gains[0].deck_index == 3 &&
           genetic.permanent_gains[0].amount == 4);
    genetic.player.block = 0;
    play(genetic, genetic.exhaust_pile[0]);
    assert(genetic.player.block == 1 + 4);

    assert_in_pool({ CardId::RAINBOW, CardId::FUSION, CardId::SUPERCRITICAL, CardId::BOOT_SEQUENCE,
                     CardId::GENETIC_ALGORITHM });
    std::cout << "[PASS] Defect upgrades, keywords: Rainbow+ and Fusion+ lose Exhaust, Supercritical+, Boot Sequence+, "
                 "Genetic Algorithm+" << std::endl;
}

void test_upgraded_effects_over_turns() {
    // Lightning Rod+: 7 Block, then one Lightning at the start of each of the next 2 turns.
    CombatEngine rod = make_combat(CharacterId::DEFECT);
    rod.player.block = 0;
    play(rod, DeckCard(CardId::LIGHTNING_ROD, true));
    assert(rod.player.block == 7 && spent(rod) == 1 && rod.player.orbs.empty());
    end_turn_idle(rod);
    assert(rod.player.orbs.size() == 1 && rod.enemies[0].hp == 42);
    end_turn_idle(rod); // the first Lightning hits for 3 at the end of the turn
    assert(rod.player.orbs.size() == 2 && rod.enemies[0].hp == 42 - 3);
    end_turn_idle(rod);
    assert(rod.player.orbs.size() == 2 && rod.enemies[0].hp == 39 - 6); // no third Lightning

    // Shatter+: 11 damage to all and every orb evoked twice (Frost: 5 Block each time), then it exhausts.
    CombatEngine shatter = make_pack(CharacterId::DEFECT);
    shatter.player.block = 0;
    shatter.channel_orb(OrbType::FROST);
    play(shatter, DeckCard(CardId::SHATTER, true));
    assert(shatter.enemies[0].hp == 18 - 11 && shatter.enemies[1].hp == 18 - 11 && shatter.enemies[2].hp == 42 - 11);
    assert(shatter.player.block == 10 && shatter.player.orbs.empty() && spent(shatter) == 1);
    assert(shatter.exhaust_pile.size() == 1);
    CombatEngine bolt = make_combat(CharacterId::DEFECT);
    bolt.channel_orb(OrbType::LIGHTNING);
    play(bolt, DeckCard(CardId::SHATTER, true));
    assert(bolt.enemies[0].hp == 42 - 11 - 8 - 8);

    // Hyperbeam+: 30 damage to all for 2 energy, and still 3 Focus lost.
    CombatEngine beam = make_pack(CharacterId::DEFECT);
    play(beam, DeckCard(CardId::HYPERBEAM, true));
    assert(!beam.enemies[0].is_alive() && !beam.enemies[1].is_alive() && beam.enemies[2].hp == 42 - 30);
    assert(beam.player.get_status(StatusType::FOCUS) == -3 && spent(beam) == 2);

    assert_in_pool({ CardId::LIGHTNING_ROD, CardId::SHATTER, CardId::HYPERBEAM });
    std::cout << "[PASS] Defect upgrades, effects: Lightning Rod+, Shatter+, Hyperbeam+" << std::endl;
}

void test_upgraded_powers() {
    // Capacitor+: 3 orb slots instead of 2.
    CombatEngine capacitor = make_combat(CharacterId::DEFECT);
    uint8_t slots = capacitor.player.orb_slots;
    play(capacitor, DeckCard(CardId::CAPACITOR, true));
    assert(capacitor.player.orb_slots == slots + 3 && spent(capacitor) == 1);

    // Thunder+: a Lightning orb evoked hits for 8 + 11.
    CombatEngine thunder = make_combat(CharacterId::DEFECT);
    play(thunder, DeckCard(CardId::THUNDER, true));
    assert(spent(thunder) == 1 && thunder.power_amount(PowerId::THUNDER) == 11);
    thunder.channel_orb(OrbType::LIGHTNING);
    thunder.evoke_orb(0);
    assert(thunder.enemies[0].hp == 42 - 8 - 11);

    // Coolant+: 3 Block per orb type at the start of each turn (Lightning and Dark: 6).
    CombatEngine coolant = make_combat(CharacterId::DEFECT);
    play(coolant, DeckCard(CardId::COOLANT, true));
    assert(spent(coolant) == 1 && coolant.power_amount(PowerId::COOLANT) == 3);
    coolant.channel_orb(OrbType::LIGHTNING);
    coolant.channel_orb(OrbType::DARK);
    end_turn_idle(coolant);
    assert(coolant.player.block == 6);

    // Consuming Shadow+: 3 Dark orbs for 2 energy; at the end of the turn the newest one is evoked (charge 6).
    CombatEngine shadow = make_combat(CharacterId::DEFECT);
    play(shadow, DeckCard(CardId::CONSUMING_SHADOW, true));
    assert(spent(shadow) == 2 && shadow.player.orbs.size() == 3);
    for (const Orb& orb : shadow.player.orbs) assert(orb == OrbType::DARK);
    end_turn_idle(shadow);
    assert(shadow.player.orbs.size() == 2 && shadow.enemies[0].hp == 42 - 6);
    CombatEngine shadow_base = make_combat(CharacterId::DEFECT);
    play(shadow_base, CardId::CONSUMING_SHADOW);
    assert(shadow_base.player.orbs.size() == 2);

    assert_in_pool({ CardId::CAPACITOR, CardId::THUNDER, CardId::COOLANT, CardId::CONSUMING_SHADOW });
    std::cout << "[PASS] Defect upgrades, powers: Capacitor+, Thunder+, Coolant+, Consuming Shadow+" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Defect Upgraded Cards ===" << std::endl;
    test_upgrade_costs();
    test_upgraded_free_attacks();
    test_upgraded_orb_attacks();
    test_upgraded_pile_cards();
    test_upgraded_keywords();
    test_upgraded_effects_over_turns();
    test_upgraded_powers();
    std::cout << "=== All Defect upgraded card tests passed ===" << std::endl;
    return 0;
}
