// Regent upgraded cards: what the upgrade changes, played in a combat.
// Covers the Regent cards (card reward pool + starter deck) whose upgraded version appears in no other test.
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

// Two Lice (18 HP) and a Jaw Worm (42 HP).
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

// The cost, Star cost and keywords the upgrade leaves alone or changes, straight from the card definition.
void assert_costs(CardId id, int cost, int upgraded_cost, int stars, int upgraded_stars) {
    assert(CardDef::get_cost(id, false) == cost && CardDef::get_cost(id, true) == upgraded_cost);
    assert(CardDef::get_star_cost(id, false) == stars && CardDef::get_star_cost(id, true) == upgraded_stars);
}

// Venerate, Gather Light, Glitterstream, Hidden Cache, Cloak of Stars, Particle Wall: Stars and Block.
void test_upgraded_stars_and_block() {
    // Venerate: 2 Stars, 3 once upgraded.
    CombatEngine venerate = make_combat(CharacterId::REGENT);
    play(venerate, CardId::VENERATE);
    assert(venerate.player.sovereign_stars == 2);
    CombatEngine venerate_up = make_combat(CharacterId::REGENT);
    play(venerate_up, DeckCard(CardId::VENERATE, true));
    assert(venerate_up.player.sovereign_stars == 3 && venerate_up.player.energy == 2);
    assert_costs(CardId::VENERATE, 1, 1, 0, 0);

    // Gather Light: 8 Block becomes 11, the Star gained stays 1.
    CombatEngine light = make_combat(CharacterId::REGENT);
    play(light, DeckCard(CardId::GATHER_LIGHT, true));
    assert(light.player.block == 11 && light.player.sovereign_stars == 1 && light.player.energy == 2);
    assert_costs(CardId::GATHER_LIGHT, 1, 1, 0, 0);

    // Glitterstream: 11 + 5 Block becomes 13 + 7, for the same 2 energy.
    CombatEngine stream = make_combat(CharacterId::REGENT);
    play(stream, DeckCard(CardId::GLITTERSTREAM, true));
    assert(stream.player.block == 13 && stream.player.energy == 1 && stream.delayed_effects.size() == 1);
    end_turn_idle(stream);
    assert(stream.player.block == 7 && stream.delayed_effects.empty());
    assert_costs(CardId::GLITTERSTREAM, 2, 2, 0, 0);

    // Hidden Cache: 1 Star now, and 4 (instead of 3) next turn.
    CombatEngine cache = make_combat(CharacterId::REGENT);
    play(cache, DeckCard(CardId::HIDDEN_CACHE, true));
    assert(cache.player.sovereign_stars == 1 && cache.delayed_effects.size() == 1);
    end_turn_idle(cache);
    assert(cache.player.sovereign_stars == 1 + 4 && cache.counters.stars_gained == 4);
    assert_costs(CardId::HIDDEN_CACHE, 1, 1, 0, 0);

    // Cloak of Stars: 10 Block instead of 7, for the same 1 Star and no energy.
    CombatEngine cloak = make_combat(CharacterId::REGENT);
    cloak.gain_stars(1);
    play(cloak, DeckCard(CardId::CLOAK_OF_STARS, true));
    assert(cloak.player.block == 10 && cloak.player.sovereign_stars == 0 && cloak.player.energy == 3);
    assert_costs(CardId::CLOAK_OF_STARS, 0, 0, 1, 1);

    // Particle Wall: 12 Block instead of 9, still back in the hand for 2 Stars.
    CombatEngine wall = make_combat(CharacterId::REGENT);
    wall.gain_stars(4);
    play(wall, DeckCard(CardId::PARTICLE_WALL, true));
    assert(wall.player.block == 12 && wall.player.sovereign_stars == 2);
    assert(wall.hand.size() == 1 && wall.hand[0] == CardId::PARTICLE_WALL && wall.hand[0].upgraded);
    assert(wall.discard_pile.empty() && wall.play_card(0));
    assert(wall.player.block == 24 && wall.player.sovereign_stars == 0 && !wall.can_play_card(0));
    assert_costs(CardId::PARTICLE_WALL, 0, 0, 2, 2);
    std::cout << "[PASS] Upgraded Stars and Block: Venerate+, Gather Light+, Glitterstream+, Hidden Cache+, Cloak of Stars+, Particle Wall+" << std::endl;
}

// Prophesize, Guiding Star: the upgrade draws more cards.
void test_upgraded_draw() {
    // Prophesize: 6 cards become 9, for the same 2 energy.
    CombatEngine prophesize = make_combat(CharacterId::REGENT);
    pad_draw_pile(prophesize, 12);
    play(prophesize, CardId::PROPHESIZE);
    assert(prophesize.hand.size() == 6);
    CombatEngine prophesize_up = make_combat(CharacterId::REGENT);
    pad_draw_pile(prophesize_up, 12);
    play(prophesize_up, DeckCard(CardId::PROPHESIZE, true));
    assert(prophesize_up.hand.size() == 9 && prophesize_up.player.energy == 1);
    assert_costs(CardId::PROPHESIZE, 2, 2, 0, 0);

    // Guiding Star: 13 damage (instead of 12) and 3 cards (instead of 2).
    CombatEngine star = make_combat(CharacterId::REGENT);
    pad_draw_pile(star, 6);
    star.gain_stars(1);
    play(star, DeckCard(CardId::GUIDING_STAR, true));
    assert(star.enemies[0].hp == 42 - 13 && star.hand.size() == 3);
    assert(star.player.sovereign_stars == 0 && star.player.energy == 2);
    assert_costs(CardId::GUIDING_STAR, 1, 1, 1, 1);
    std::cout << "[PASS] Upgraded draw: Prophesize+ draws 9, Guiding Star+ deals 13 and draws 3" << std::endl;
}

// Heavenly Drill, Lunar Blast, Radiate: the upgrade raises the damage of each hit.
void test_upgraded_scaling_attacks() {
    // Heavenly Drill: X hits of 10 (instead of 8), doubled from X = 4 on.
    CombatEngine drill = make_combat(CharacterId::REGENT);
    drill.enemies[0].hp = 200;
    drill.player.max_energy = 3;
    play(drill, DeckCard(CardId::HEAVENLY_DRILL, true));
    assert(drill.enemies[0].hp == 200 - 3 * 10 && drill.player.energy == 0);
    drill.enemies[0].hp = 200;
    drill.player.max_energy = 4;
    play(drill, DeckCard(CardId::HEAVENLY_DRILL, true));
    assert(drill.enemies[0].hp == 200 - 8 * 10 && drill.player.energy == 0);
    assert(CardDef::is_x_cost(CardId::HEAVENLY_DRILL, false) && CardDef::is_x_cost(CardId::HEAVENLY_DRILL, true));

    // Lunar Blast: 5 (instead of 4) per Skill played this turn, for free.
    CombatEngine lunar = make_combat(CharacterId::REGENT);
    play(lunar, CardId::DEFEND);
    play(lunar, CardId::DEFEND);
    play(lunar, DeckCard(CardId::LUNAR_BLAST, true));
    assert(lunar.enemies[0].hp == 42 - 2 * 5 && lunar.player.energy == 3);
    assert_costs(CardId::LUNAR_BLAST, 0, 0, 0, 0);

    // Radiate: 4 (instead of 3) to every enemy per Star gained this turn.
    CombatEngine radiate = make_pack(CharacterId::REGENT);
    radiate.gain_stars(2);
    radiate.gain_stars(1);
    play(radiate, DeckCard(CardId::RADIATE, true));
    assert(radiate.enemies[0].hp == 18 - 3 * 4 && radiate.enemies[1].hp == 18 - 3 * 4 && radiate.enemies[2].hp == 42 - 3 * 4);
    assert(radiate.player.sovereign_stars == 3 && radiate.player.energy == 3);
    assert_costs(CardId::RADIATE, 0, 0, 0, 0);
    std::cout << "[PASS] Upgraded scaling attacks: Heavenly Drill+ 10 per hit, Lunar Blast+ 5 per Skill, Radiate+ 4 per Star" << std::endl;
}

// Astral Pulse, Comet, Crescent Spear, Dying Star, Shining Strike: Star-cost attacks.
void test_upgraded_star_attacks() {
    // Astral Pulse: two waves of 8 (instead of 6) to all, for the same 3 Stars.
    CombatEngine pulse = make_pack(CharacterId::REGENT);
    pulse.gain_stars(3);
    play(pulse, DeckCard(CardId::ASTRAL_PULSE, true));
    assert(pulse.enemies[0].hp == 18 - 16 && pulse.enemies[1].hp == 18 - 16 && pulse.enemies[2].hp == 42 - 16);
    assert(pulse.player.sovereign_stars == 0 && pulse.player.energy == 3);
    assert_costs(CardId::ASTRAL_PULSE, 0, 0, 3, 3);

    // Comet: 44 damage (instead of 33), still 3 Weak and 3 Vulnerable, for the same 5 Stars.
    CombatEngine comet = make_combat(CharacterId::REGENT);
    comet.enemies[0].hp = 100;
    comet.gain_stars(7);
    play(comet, DeckCard(CardId::COMET, true));
    assert(comet.enemies[0].hp == 100 - 44 && comet.player.sovereign_stars == 2);
    assert(comet.enemies[0].get_status(StatusType::WEAK) == 3 && comet.enemies[0].get_status(StatusType::VULNERABLE) == 3);
    assert_costs(CardId::COMET, 0, 0, 5, 5);

    // Crescent Spear: +3 (instead of +2) per card with a Star cost in the piles, itself included.
    CombatEngine spear = make_combat(CharacterId::REGENT);
    spear.draw_pile.clear();
    spear.discard_pile.clear();
    spear.draw_pile.push_back(CardId::FALLING_STAR);
    spear.draw_pile.push_back(CardId::STRIKE);
    spear.discard_pile.push_back(CardId::COMET);
    spear.gain_stars(1);
    play(spear, DeckCard(CardId::CRESCENT_SPEAR, true));
    assert(spear.enemies[0].hp == 42 - (8 + 3 * 3) && spear.player.sovereign_stars == 0);
    assert_costs(CardId::CRESCENT_SPEAR, 1, 1, 1, 1);

    // Dying Star: 11 damage and 11 Strength lost (instead of 9), still Ethereal.
    CombatEngine dying = make_pack(CharacterId::REGENT);
    dying.gain_stars(3);
    play(dying, DeckCard(CardId::DYING_STAR, true));
    assert(dying.enemies[0].hp == 18 - 11 && dying.enemies[1].hp == 18 - 11 && dying.enemies[2].hp == 42 - 11);
    for (const Enemy& foe : dying.enemies) assert(foe.get_status(StatusType::STRENGTH) == -11);
    assert(CardDef::has_keyword(CardId::DYING_STAR, false, KEYWORD_ETHEREAL));
    assert(CardDef::has_keyword(CardId::DYING_STAR, true, KEYWORD_ETHEREAL));
    assert_costs(CardId::DYING_STAR, 1, 1, 3, 3);

    // Shining Strike: 11 damage (instead of 8), 2 Stars, back on top of the draw pile still upgraded.
    CombatEngine shining = make_combat(CharacterId::REGENT);
    play(shining, DeckCard(CardId::SHINING_STRIKE, true));
    assert(shining.enemies[0].hp == 42 - 11 && shining.player.sovereign_stars == 2 && shining.player.energy == 2);
    assert(shining.draw_pile.back() == CardId::SHINING_STRIKE && shining.draw_pile.back().upgraded);
    assert_costs(CardId::SHINING_STRIKE, 1, 1, 0, 0);
    std::cout << "[PASS] Upgraded Star attacks: Astral Pulse+, Comet+, Crescent Spear+, Dying Star+, Shining Strike+" << std::endl;
}

// Make It So, Bombardment: the upgrade keeps the recall and the replay.
void test_upgraded_recurring_attacks() {
    // Make It So: 9 damage (instead of 6), back in hand after 3 Skills, still upgraded.
    CombatEngine so = make_combat(CharacterId::REGENT);
    play(so, DeckCard(CardId::MAKE_IT_SO, true));
    assert(so.enemies[0].hp == 42 - 9 && so.player.energy == 3 && so.discard_pile.back() == CardId::MAKE_IT_SO);
    play(so, CardId::DEFEND);
    play(so, CardId::DEFEND);
    assert(count_in_hand(so, CardId::MAKE_IT_SO) == 0);
    play(so, CardId::DEFEND);
    assert(count_in_hand(so, CardId::MAKE_IT_SO) == 1 && so.hand.back().upgraded);
    assert(so.play_card(count_in_hand(so, CardId::DEFEND)));
    assert(so.enemies[0].hp == 42 - 9 - 9);
    assert_costs(CardId::MAKE_IT_SO, 0, 0, 0, 0);

    // Bombardment: 24 damage (instead of 18), Exhaust kept; it plays itself at each start of turn, upgraded.
    CombatEngine bombardment = make_combat(CharacterId::REGENT);
    bombardment.enemies[0].hp = 100;
    play(bombardment, DeckCard(CardId::BOMBARDMENT, true));
    assert(bombardment.enemies[0].hp == 100 - 24 && bombardment.player.energy == 0);
    assert(bombardment.exhaust_pile.size() == 1 && bombardment.exhaust_pile[0].upgraded);
    end_turn_idle(bombardment);
    assert(bombardment.enemies[0].hp == 100 - 24 - 24 && bombardment.exhaust_pile.size() == 1);
    assert(CardDef::has_keyword(CardId::BOMBARDMENT, false, KEYWORD_EXHAUST));
    assert(CardDef::has_keyword(CardId::BOMBARDMENT, true, KEYWORD_EXHAUST));
    assert_costs(CardId::BOMBARDMENT, 3, 3, 0, 0);
    std::cout << "[PASS] Upgraded recurring attacks: Make It So+ (9, recalled upgraded), Bombardment+ (24, replays itself)" << std::endl;
}

// Kingly Kick, Kingly Punch: the upgrade changes what a draw does to the card.
void test_upgraded_kingly_cards() {
    // Kingly Kick: 35 damage (instead of 27), and the cost of 4 still drops by 1 each time it is drawn.
    CombatEngine kick = make_combat(CharacterId::REGENT);
    kick.hand.clear();
    kick.draw_pile.clear();
    kick.draw_pile.push_back(DeckCard(CardId::KINGLY_KICK, true));
    assert(kick.draw_card());
    assert(kick.effective_cost(kick.hand[0]) == 3 && kick.hand[0].upgraded);
    kick.player.energy = 3;
    assert(kick.play_card(0));
    assert(kick.enemies[0].hp == 42 - 35 && kick.player.energy == 0);
    // Drawn again, it is cheaper still.
    DeckCard again = kick.discard_pile.back();
    kick.discard_pile.clear();
    kick.draw_pile.push_back(again);
    assert(kick.draw_card());
    assert(kick.effective_cost(kick.hand[0]) == 2 && kick.hand[0].upgraded);
    // Not enough energy at its printed cost: the upgrade does not make it cheaper.
    CombatEngine undrawn = make_combat(CharacterId::REGENT);
    undrawn.hand.clear();
    undrawn.hand.push_back(DeckCard(CardId::KINGLY_KICK, true));
    undrawn.player.energy = 3;
    assert(!undrawn.can_play_card(0));
    assert_costs(CardId::KINGLY_KICK, 4, 4, 0, 0);

    // Kingly Punch: 10 damage (instead of 8), and +6 (instead of +4) each time it is drawn.
    CombatEngine punch = make_combat(CharacterId::REGENT);
    punch.hand.clear();
    punch.draw_pile.clear();
    punch.draw_pile.push_back(DeckCard(CardId::KINGLY_PUNCH, true));
    assert(punch.draw_card() && punch.hand[0].bonus == 6);
    play(punch, punch.hand[0]);
    assert(punch.enemies[0].hp == 42 - (10 + 6) && punch.player.energy == 2);
    DeckCard drawn_twice = punch.discard_pile.back();
    punch.discard_pile.clear();
    punch.draw_pile.push_back(drawn_twice);
    punch.hand.clear();
    assert(punch.draw_card() && punch.hand[0].bonus == 12);
    punch.player.energy = 3;
    assert(punch.play_card(0));
    assert(punch.enemies[0].hp == 42 - 16 - (10 + 12));
    CombatEngine base_punch = make_combat(CharacterId::REGENT);
    base_punch.hand.clear();
    base_punch.draw_pile.clear();
    base_punch.draw_pile.push_back(CardId::KINGLY_PUNCH);
    assert(base_punch.draw_card() && base_punch.hand[0].bonus == 4);
    assert_costs(CardId::KINGLY_PUNCH, 1, 1, 0, 0);
    std::cout << "[PASS] Upgraded Kingly cards: Kingly Kick+ (35, cost drops per draw), Kingly Punch+ (10, +6 per draw)" << std::endl;
}

// Black Hole, Pale Blue Dot, Arsenal, Orbit: the upgraded Powers.
void test_upgraded_powers() {
    // Black Hole: 4 (instead of 3) damage to all whenever Stars are gained or spent.
    CombatEngine hole = make_pack(CharacterId::REGENT);
    play(hole, DeckCard(CardId::BLACK_HOLE, true));
    assert(hole.enemies[0].hp == 18 && hole.player.energy == 2);
    hole.gain_stars(2);
    assert(hole.enemies[0].hp == 18 - 4 && hole.enemies[1].hp == 18 - 4 && hole.enemies[2].hp == 42 - 4);
    play(hole, CardId::CLOAK_OF_STARS);
    assert(hole.player.sovereign_stars == 1 && hole.enemies[2].hp == 42 - 8);
    assert_costs(CardId::BLACK_HOLE, 1, 1, 0, 0);

    // Pale Blue Dot: after 5 cards played in a turn, 2 cards (instead of 1) more next turn.
    CombatEngine dot = make_combat(CharacterId::REGENT);
    pad_draw_pile(dot, 12);
    play(dot, DeckCard(CardId::PALE_BLUE_DOT, true));
    for (int i = 0; i < 4; ++i) play(dot, CardId::DEFEND);
    end_turn_idle(dot);
    assert(dot.hand.size() == 5 + 2);
    // Four cards are not enough.
    CombatEngine short_turn = make_combat(CharacterId::REGENT);
    pad_draw_pile(short_turn, 12);
    play(short_turn, DeckCard(CardId::PALE_BLUE_DOT, true));
    for (int i = 0; i < 3; ++i) play(short_turn, CardId::DEFEND);
    end_turn_idle(short_turn);
    assert(short_turn.hand.size() == 5);
    assert_costs(CardId::PALE_BLUE_DOT, 1, 1, 0, 0);

    // Arsenal: the upgrade adds Innate, and a created card still gives 1 Strength.
    assert(!CardDef::has_keyword(CardId::ARSENAL, false, KEYWORD_INNATE));
    assert(CardDef::has_keyword(CardId::ARSENAL, true, KEYWORD_INNATE));
    CombatEngine innate = make_combat(CharacterId::REGENT);
    innate.hand.clear();
    innate.discard_pile.clear();
    innate.draw_pile.clear();
    innate.draw_pile.push_back(DeckCard(CardId::ARSENAL, true));
    for (int i = 0; i < 10; ++i) innate.draw_pile.push_back(CardId::STRIKE);
    innate.open_first_turn();
    assert(innate.hand.size() == 5 && count_in_hand(innate, CardId::ARSENAL) == 1);
    CombatEngine plain = make_combat(CharacterId::REGENT);
    plain.hand.clear();
    plain.discard_pile.clear();
    plain.draw_pile.clear();
    plain.draw_pile.push_back(CardId::ARSENAL);
    for (int i = 0; i < 10; ++i) plain.draw_pile.push_back(CardId::STRIKE);
    plain.open_first_turn();
    assert(plain.hand.size() == 5 && count_in_hand(plain, CardId::ARSENAL) == 0);
    CombatEngine arsenal = make_combat(CharacterId::REGENT);
    play(arsenal, DeckCard(CardId::ARSENAL, true));
    arsenal.create_card_in_discard(CardId::STRIKE);
    assert(arsenal.player.get_status(StatusType::STRENGTH) == 1);
    assert_costs(CardId::ARSENAL, 1, 1, 0, 0);

    // Orbit: the cost falls from 2 to 1, and 1 energy still comes back for every 4 spent after it.
    assert_costs(CardId::ORBIT, 2, 1, 0, 0);
    CombatEngine orbit = make_combat(CharacterId::REGENT);
    play(orbit, DeckCard(CardId::ORBIT, true));
    assert(orbit.player.energy == 3 - 1);
    play(orbit, CardId::BLUDGEON);
    assert(orbit.player.energy == 0);
    play(orbit, CardId::STRIKE);
    assert(orbit.player.energy == 3 - 1 + 1 && orbit.powers[0].counter == 0);
    std::cout << "[PASS] Upgraded Powers: Black Hole+ (4), Pale Blue Dot+ (2 cards), Arsenal+ (Innate), Orbit+ (cost 1)" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Regent Upgraded Cards ===" << std::endl;
    test_upgraded_stars_and_block();
    test_upgraded_draw();
    test_upgraded_scaling_attacks();
    test_upgraded_star_attacks();
    test_upgraded_recurring_attacks();
    test_upgraded_kingly_cards();
    test_upgraded_powers();
    std::cout << "All Regent upgraded card tests passed." << std::endl;
    return 0;
}
