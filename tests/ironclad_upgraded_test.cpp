// Ironclad upgraded cards: plays the "+" version of every card of the hero whose upgrade no other test covers,
// and checks what the upgrade changes (value, cost, keyword, number of hits). Expected values: the CardDef rows of
// src/engine/card.cpp, cross-checked with bible/data/card_v0.111.0.json.
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

// The printed cost and the cost the engine charges for the upgraded card.
void assert_cost(CardId id, int base, int upgraded) {
    assert(CardDef::get_cost(id, false) == base);
    assert(CardDef::get_cost(id, true) == upgraded);
    assert(CombatEngine{}.effective_cost(DeckCard(id, true)) == upgraded);
}

void test_upgraded_single_target_attacks() {
    // Bash+: 10 damage and 3 Vulnerable for the same 2 energy; the 3 stacks last one more enemy turn.
    CombatEngine bash = make_combat(CharacterId::IRONCLAD);
    play(bash, DeckCard(CardId::BASH, true));
    assert(bash.enemies[0].hp == 42 - 10 && bash.enemies[0].get_status(StatusType::VULNERABLE) == 3);
    assert(bash.player.energy == 1);
    bash.end_player_turn();
    assert(bash.enemies[0].get_status(StatusType::VULNERABLE) == 2);
    assert_cost(CardId::BASH, 2, 2);

    // Anger+: 8 damage for free, and the copy it leaves in the discard pile is upgraded too.
    CombatEngine anger = make_combat(CharacterId::IRONCLAD);
    anger.discard_pile.clear();
    play(anger, DeckCard(CardId::ANGER, true));
    assert(anger.enemies[0].hp == 42 - 8 && anger.player.energy == 3);
    assert(count_in(anger.discard_pile, CardId::ANGER) == 2);
    for (const DeckCard& card : anger.discard_pile) assert(card.upgraded);
    assert_cost(CardId::ANGER, 0, 0);

    // Iron Wave+: 7 Block and 7 damage.
    CombatEngine wave = make_combat(CharacterId::IRONCLAD);
    wave.player.block = 0;
    play(wave, DeckCard(CardId::IRON_WAVE, true));
    assert(wave.player.block == 7 && wave.enemies[0].hp == 42 - 7 && wave.player.energy == 2);
    assert_cost(CardId::IRON_WAVE, 1, 1);

    // Ashen Strike+: 6 damage, +4 per exhausted card (+3 before).
    CombatEngine ashen = make_combat(CharacterId::IRONCLAD);
    for (int n = 0; n < 2; ++n) ashen.exhaust_card(CardId::DEFEND);
    play(ashen, DeckCard(CardId::ASHEN_STRIKE, true));
    assert(ashen.enemies[0].hp == 42 - (6 + 4 * 2) && ashen.player.energy == 2);
    assert_cost(CardId::ASHEN_STRIKE, 1, 1);

    // Bully+: 4 damage, +3 per Vulnerable stack, then Vulnerable applies: (4 + 3 x 2) x 1.5 = 15.
    CombatEngine bully = make_combat(CharacterId::IRONCLAD);
    bully.enemies[0].add_status(StatusType::VULNERABLE, 2);
    play(bully, DeckCard(CardId::BULLY, true));
    assert(bully.enemies[0].hp == 42 - 15 && bully.player.energy == 3);
    assert_cost(CardId::BULLY, 0, 0);

    // Cinder+: 24 damage, and it still exhausts a card of the hand at random.
    CombatEngine cinder = make_combat(CharacterId::IRONCLAD);
    cinder.hand.clear();
    cinder.hand.push_back(DeckCard(CardId::CINDER, true));
    cinder.hand.push_back(CardId::DEFEND);
    cinder.player.energy = 3;
    assert(cinder.play_card(0));
    assert(cinder.enemies[0].hp == 42 - 24 && cinder.player.energy == 1);
    assert(cinder.hand.empty() && cinder.exhaust_pile.size() == 1 && cinder.exhaust_pile[0].id == CardId::DEFEND);
    assert_cost(CardId::CINDER, 2, 2);

    // Dismantle+: 10 damage, and twice that once the enemy is Vulnerable (10 x 1.5 = 15 per hit).
    CombatEngine dismantle = make_combat(CharacterId::IRONCLAD);
    play(dismantle, DeckCard(CardId::DISMANTLE, true));
    assert(dismantle.enemies[0].hp == 42 - 10);
    dismantle.enemies[0].add_status(StatusType::VULNERABLE, 1);
    play(dismantle, DeckCard(CardId::DISMANTLE, true));
    assert(dismantle.enemies[0].hp == 32 - 30);
    assert_cost(CardId::DISMANTLE, 1, 1);

    // Molten Fist+: 14 damage (21 on a Vulnerable enemy), doubles its Vulnerable and exhausts.
    CombatEngine fist = make_combat(CharacterId::IRONCLAD);
    fist.enemies[0].add_status(StatusType::VULNERABLE, 2);
    play(fist, DeckCard(CardId::MOLTEN_FIST, true));
    assert(fist.enemies[0].hp == 42 - 21 && fist.enemies[0].get_status(StatusType::VULNERABLE) == 4);
    assert(count_in(fist.exhaust_pile, CardId::MOLTEN_FIST) == 1);
    assert(CardDef::has_keyword(CardId::MOLTEN_FIST, true, KEYWORD_EXHAUST));
    assert_cost(CardId::MOLTEN_FIST, 1, 1);

    // Pillage+: 9 damage, then draws until a card that is not an Attack comes up.
    CombatEngine pillage = make_combat(CharacterId::IRONCLAD);
    pillage.draw_pile.clear();
    pillage.discard_pile.clear();
    pillage.draw_pile.push_back(CardId::STRIKE);  // never reached
    pillage.draw_pile.push_back(CardId::DEFEND);  // third draw: stops here
    pillage.draw_pile.push_back(CardId::STRIKE);
    pillage.draw_pile.push_back(CardId::BASH);    // top of the pile
    play(pillage, DeckCard(CardId::PILLAGE, true));
    assert(pillage.enemies[0].hp == 42 - 9);
    assert(pillage.hand.size() == 3 && pillage.hand[2].id == CardId::DEFEND && pillage.draw_pile.size() == 1);
    assert_cost(CardId::PILLAGE, 1, 1);

    // Setup Strike+: 9 damage and 4 Strength until the end of the turn (a Strike then hits for 10).
    CombatEngine setup = make_combat(CharacterId::IRONCLAD);
    play(setup, DeckCard(CardId::SETUP_STRIKE, true));
    assert(setup.enemies[0].hp == 42 - 9 && setup.player.get_status(StatusType::STRENGTH) == 4);
    play(setup, CardId::STRIKE);
    assert(setup.enemies[0].hp == 33 - 10);
    end_turn_idle(setup);
    assert(setup.player.get_status(StatusType::STRENGTH) == 0 && setup.temp_strength == 0);
    assert_cost(CardId::SETUP_STRIKE, 1, 1);

    // Tear Asunder+: 7 damage, one more hit for each time HP was lost this combat (3 hits here).
    CombatEngine tear = make_combat(CharacterId::IRONCLAD);
    tear.counters.combat_hp_losses = 2;
    play(tear, DeckCard(CardId::TEAR_ASUNDER, true));
    assert(tear.enemies[0].hp == 42 - 21 && tear.counters.hits_dealt == 3 && tear.player.energy == 1);
    assert_cost(CardId::TEAR_ASUNDER, 2, 2);

    // Unrelenting+: 20 damage, and the next Attack still costs 0.
    CombatEngine unrelenting = make_combat(CharacterId::IRONCLAD);
    play(unrelenting, DeckCard(CardId::UNRELENTING, true));
    assert(unrelenting.enemies[0].hp == 42 - 20 && unrelenting.player.energy == 1);
    unrelenting.hand.push_back(CardId::DEFEND);
    unrelenting.hand.push_back(CardId::BASH);
    assert(unrelenting.effective_cost(unrelenting.hand[0]) == 1 && unrelenting.effective_cost(unrelenting.hand[1]) == 0);
    assert(unrelenting.play_card(1) && unrelenting.player.energy == 1);
    assert(!unrelenting.has_power(PowerId::FREE_ATTACK));
    assert_cost(CardId::UNRELENTING, 2, 2);

    // Thrash+: two hits of 6, then a random Attack of the hand is exhausted and its damage kept by this copy.
    CombatEngine thrash = make_combat(CharacterId::IRONCLAD);
    thrash.hand.clear();
    thrash.hand.push_back(DeckCard(CardId::THRASH, true));
    thrash.hand.push_back(CardId::DEFEND);
    thrash.hand.push_back(CardId::BLUDGEON);
    thrash.discard_pile.clear();
    assert(thrash.play_card(0));
    assert(thrash.enemies[0].hp == 42 - 12);
    assert(thrash.exhaust_pile.size() == 1 && thrash.exhaust_pile[0].id == CardId::BLUDGEON);
    assert(thrash.hand.size() == 1 && thrash.discard_pile[0].id == CardId::THRASH);
    assert(thrash.discard_pile[0].upgraded && thrash.discard_pile[0].bonus == 32);
    thrash.enemies[0].hp = 200;
    thrash.hand.clear();
    thrash.hand.push_back(thrash.discard_pile[0]);
    thrash.player.energy = 3;
    assert(thrash.play_card(0));
    assert(thrash.enemies[0].hp == 200 - 2 * (6 + 32));
    assert_cost(CardId::THRASH, 1, 1);

    // Mangle+: 26 damage and 15 Strength taken from the enemy for the turn, which comes back afterwards.
    CombatEngine mangle = make_combat(CharacterId::IRONCLAD);
    mangle.enemies[0].add_status(StatusType::STRENGTH, 3);
    play(mangle, DeckCard(CardId::MANGLE, true));
    assert(mangle.enemies[0].hp == 42 - 26 && mangle.enemies[0].get_status(StatusType::STRENGTH) == -12);
    assert(mangle.enemies[0].intent_damage(mangle.player) == 0 && mangle.player.energy == 0);
    int16_t hp_before = mangle.player.hp;
    mangle.end_player_turn();
    assert(mangle.player.hp == hp_before);
    assert(mangle.enemies[0].get_status(StatusType::STRENGTH) == 3 && mangle.enemies[0].temp_strength_loss == 0);
    assert_cost(CardId::MANGLE, 3, 3);

    // Fight Me!+: two hits of 6, then 4 Strength for the hero and still 1 for the enemy.
    CombatEngine fight = make_combat(CharacterId::IRONCLAD);
    play(fight, DeckCard(CardId::FIGHT_ME, true));
    assert(fight.enemies[0].hp == 42 - 12);
    assert(fight.player.get_status(StatusType::STRENGTH) == 4);
    assert(fight.enemies[0].get_status(StatusType::STRENGTH) == 1 && fight.player.energy == 1);
    assert_cost(CardId::FIGHT_ME, 2, 2);

    std::cout << "[PASS] Upgraded single-target Attacks: Bash, Anger, Iron Wave, Ashen Strike, Bully, Cinder, Dismantle, "
                 "Molten Fist, Pillage, Setup Strike, Tear Asunder, Unrelenting, Thrash, Mangle, Fight Me!"
              << std::endl;
}

void test_upgraded_area_attacks() {
    // Breakthrough+: 13 damage to everyone for 1 HP.
    CombatEngine breakthrough = make_pack(CharacterId::IRONCLAD);
    play(breakthrough, DeckCard(CardId::BREAKTHROUGH, true));
    assert(breakthrough.player.hp == 79 && breakthrough.enemies[0].hp == 18 - 13 && breakthrough.enemies[2].hp == 42 - 13);
    assert_cost(CardId::BREAKTHROUGH, 1, 1);

    // Conflagration+: 2 damage to every enemy, 5 times instead of 4.
    CombatEngine fire = make_pack(CharacterId::IRONCLAD);
    play(fire, DeckCard(CardId::CONFLAGRATION, true));
    assert(fire.enemies[0].hp == 18 - 10 && fire.enemies[1].hp == 18 - 10 && fire.enemies[2].hp == 42 - 10);
    assert_cost(CardId::CONFLAGRATION, 1, 1);

    // Fiend Fire+: exhausts the hand, one hit of 10 per card (7 before), then itself.
    CombatEngine fiend = make_combat(CharacterId::IRONCLAD);
    fiend.hand.clear();
    fiend.hand.push_back(DeckCard(CardId::FIEND_FIRE, true));
    fiend.hand.push_back(CardId::STRIKE);
    fiend.hand.push_back(CardId::DEFEND);
    fiend.hand.push_back(CardId::DEFEND);
    assert(fiend.play_card(0));
    assert(fiend.enemies[0].hp == 42 - 30 && fiend.hand.empty() && fiend.exhaust_pile.size() == 4);
    assert(CardDef::has_keyword(CardId::FIEND_FIRE, true, KEYWORD_EXHAUST));
    assert_cost(CardId::FIEND_FIRE, 2, 2);

    // Pact's End+: 24 damage to everyone, but only from 3 exhausted cards (the threshold does not move).
    CombatEngine pact = make_pack(CharacterId::IRONCLAD);
    for (int n = 0; n < 2; ++n) pact.exhaust_card(CardId::DEFEND);
    play(pact, DeckCard(CardId::PACTS_END, true));
    assert(pact.enemies[0].hp == 18 && pact.enemies[2].hp == 42);
    pact.exhaust_card(CardId::DEFEND);
    play(pact, DeckCard(CardId::PACTS_END, true));
    assert(pact.enemies[0].hp == 0 && pact.enemies[1].hp == 0 && pact.enemies[2].hp == 42 - 24);
    assert_cost(CardId::PACTS_END, 0, 0);

    // Stomp+: 15 damage to everyone; it costs 3 and still gets 1 cheaper for each Attack played this turn.
    CombatEngine stomp = make_pack(CharacterId::IRONCLAD);
    assert(stomp.effective_cost(DeckCard(CardId::STOMP, true)) == 3);
    play(stomp, CardId::STRIKE, 2);
    play(stomp, CardId::STRIKE, 2);
    assert(stomp.effective_cost(DeckCard(CardId::STOMP, true)) == 1);
    play_with(stomp, DeckCard(CardId::STOMP, true), 1);
    assert(stomp.enemies[0].hp == 18 - 15 && stomp.enemies[1].hp == 18 - 15 && stomp.enemies[2].hp == 42 - 12 - 15);
    assert(stomp.player.energy == 0);
    assert_cost(CardId::STOMP, 3, 3);

    // Whirlwind+: still X energy, but 8 damage to everyone per energy spent (5 before).
    CombatEngine whirlwind = make_pack(CharacterId::IRONCLAD);
    whirlwind.hand.clear();
    whirlwind.hand.push_back(DeckCard(CardId::WHIRLWIND, true));
    whirlwind.player.energy = 2;
    assert(whirlwind.play_card(0));
    assert(whirlwind.enemies[0].hp == 18 - 16 && whirlwind.enemies[1].hp == 18 - 16 && whirlwind.enemies[2].hp == 42 - 16);
    assert(whirlwind.player.energy == 0);
    assert(CardDef::is_x_cost(CardId::WHIRLWIND, false) && CardDef::is_x_cost(CardId::WHIRLWIND, true));

    // Howl from Beyond+: 24 damage to everyone when played, and again from the exhaust pile at the end of each turn.
    CombatEngine howl = make_combat(CharacterId::IRONCLAD);
    howl.enemies[0].hp = 100;
    play(howl, DeckCard(CardId::HOWL_FROM_BEYOND, true));
    assert(howl.enemies[0].hp == 100 - 24 && howl.player.energy == 0);
    assert(count_in(howl.discard_pile, CardId::HOWL_FROM_BEYOND) == 1);
    howl.exhaust_pile.push_back(DeckCard(CardId::HOWL_FROM_BEYOND, true));
    end_turn_idle(howl);
    assert(howl.enemies[0].hp == 76 - 24 && count_in(howl.exhaust_pile, CardId::HOWL_FROM_BEYOND) == 1);
    assert_cost(CardId::HOWL_FROM_BEYOND, 3, 3);

    std::cout << "[PASS] Upgraded area Attacks: Breakthrough, Conflagration, Fiend Fire, Pact's End, Stomp, Whirlwind, "
                 "Howl from Beyond"
              << std::endl;
}

void test_upgraded_skills() {
    // Blood Wall+: 20 Block (16 before) for the same 2 HP.
    CombatEngine wall = make_combat(CharacterId::IRONCLAD);
    wall.player.block = 0;
    play(wall, DeckCard(CardId::BLOOD_WALL, true));
    assert(wall.player.hp == 78 && wall.player.block == 20 && wall.player.energy == 1);
    assert_cost(CardId::BLOOD_WALL, 2, 2);

    // Shrug It Off+: 11 Block and still 1 card.
    CombatEngine shrug = make_combat(CharacterId::IRONCLAD);
    shrug.player.block = 0;
    pad_draw_pile(shrug, 10);
    play(shrug, DeckCard(CardId::SHRUG_IT_OFF, true));
    assert(shrug.player.block == 11 && shrug.hand.size() == 1);
    assert_cost(CardId::SHRUG_IT_OFF, 1, 1);

    // Impervious+: 40 Block (30 before) and it still exhausts.
    CombatEngine impervious = make_combat(CharacterId::IRONCLAD);
    impervious.player.block = 0;
    play(impervious, DeckCard(CardId::IMPERVIOUS, true));
    assert(impervious.player.block == 40 && count_in(impervious.exhaust_pile, CardId::IMPERVIOUS) == 1);
    assert(impervious.player.energy == 1 && CardDef::has_keyword(CardId::IMPERVIOUS, true, KEYWORD_EXHAUST));
    assert_cost(CardId::IMPERVIOUS, 2, 2);

    // Second Wind+: exhausts the non-Attacks of the hand for 7 Block each (5 before).
    CombatEngine wind = make_combat(CharacterId::IRONCLAD);
    wind.player.block = 0;
    wind.hand.clear();
    wind.hand.push_back(DeckCard(CardId::SECOND_WIND, true));
    wind.hand.push_back(CardId::DEFEND);
    wind.hand.push_back(CardId::STRIKE);
    wind.hand.push_back(CardId::SHRUG_IT_OFF);
    wind.player.energy = 3;
    assert(wind.play_card(0));
    assert(wind.player.block == 14 && wind.exhaust_pile.size() == 2);
    assert(wind.hand.size() == 1 && wind.hand[0].id == CardId::STRIKE);
    assert_cost(CardId::SECOND_WIND, 1, 1);

    // Dominate+: 2 Vulnerable instead of 1, then 1 Strength per Vulnerable on the enemy; still exhausts.
    CombatEngine dominate = make_combat(CharacterId::IRONCLAD);
    play(dominate, DeckCard(CardId::DOMINATE, true));
    assert(dominate.enemies[0].get_status(StatusType::VULNERABLE) == 2 && dominate.player.get_status(StatusType::STRENGTH) == 2);
    assert(count_in(dominate.exhaust_pile, CardId::DOMINATE) == 1);
    CombatEngine dominate_base = make_combat(CharacterId::IRONCLAD);
    play(dominate_base, CardId::DOMINATE);
    assert(dominate_base.enemies[0].get_status(StatusType::VULNERABLE) == 1);
    assert(CardDef::has_keyword(CardId::DOMINATE, true, KEYWORD_EXHAUST));
    assert_cost(CardId::DOMINATE, 1, 1);

    // Offering+: 6 HP for 2 energy and 5 cards (3 before); it still exhausts and costs nothing.
    CombatEngine offering = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(offering, 10);
    play(offering, DeckCard(CardId::OFFERING, true));
    assert(offering.player.hp == 74 && offering.player.energy == 5 && offering.hand.size() == 5);
    assert(count_in(offering.exhaust_pile, CardId::OFFERING) == 1);
    assert(CardDef::has_keyword(CardId::OFFERING, true, KEYWORD_EXHAUST));
    assert_cost(CardId::OFFERING, 0, 0);

    std::cout << "[PASS] Upgraded Skills: Blood Wall, Shrug It Off, Impervious, Second Wind, Dominate, Offering" << std::endl;
}

void test_upgraded_powers() {
    // Crimson Mantle+: 1 HP for 10 Block (7 before) at the start of each turn.
    CombatEngine mantle = make_combat(CharacterId::IRONCLAD);
    play(mantle, DeckCard(CardId::CRIMSON_MANTLE, true));
    assert(count_in(mantle.discard_pile, CardId::CRIMSON_MANTLE) == 0 && mantle.player.energy == 2);
    end_turn_idle(mantle);
    assert(mantle.player.hp == 79 && mantle.player.block == 10 && mantle.counters.hp_lost == 1);
    assert_cost(CardId::CRIMSON_MANTLE, 1, 1);

    // Demon Form+: 4 Strength at the start of each turn (3 before).
    CombatEngine demon = make_combat(CharacterId::IRONCLAD);
    play(demon, DeckCard(CardId::DEMON_FORM, true));
    assert(demon.player.get_status(StatusType::STRENGTH) == 0 && demon.player.energy == 0);
    end_turn_idle(demon);
    assert(demon.player.get_status(StatusType::STRENGTH) == 4);
    end_turn_idle(demon);
    assert(demon.player.get_status(StatusType::STRENGTH) == 8);
    assert_cost(CardId::DEMON_FORM, 3, 3);

    // Inferno+: the HP it takes at the start of the turn burns every enemy for 9 (6 before).
    CombatEngine inferno = make_pack(CharacterId::IRONCLAD);
    play(inferno, DeckCard(CardId::INFERNO, true));
    end_turn_idle(inferno);
    assert(inferno.player.hp == 79 && inferno.enemies[0].hp == 18 - 9 && inferno.enemies[2].hp == 42 - 9);
    play(inferno, CardId::BLOODLETTING);
    assert(inferno.enemies[0].hp == 0 && inferno.enemies[2].hp == 42 - 18);
    assert_cost(CardId::INFERNO, 1, 1);

    // Juggernaut+: each time Block is gained, 8 damage to a random enemy (6 before).
    CombatEngine juggernaut = make_combat(CharacterId::IRONCLAD);
    play(juggernaut, DeckCard(CardId::JUGGERNAUT, true));
    assert(juggernaut.player.energy == 1 && juggernaut.enemies[0].hp == 42);
    play(juggernaut, CardId::DEFEND);
    assert(juggernaut.enemies[0].hp == 42 - 8);
    assert_cost(CardId::JUGGERNAUT, 2, 2);

    // Stampede+: costs 1 instead of 2, and still plays a random Attack of the hand at the end of the turn.
    CombatEngine stampede = make_combat(CharacterId::IRONCLAD);
    assert(stampede.effective_cost(DeckCard(CardId::STAMPEDE, false)) == 2);
    assert(stampede.effective_cost(DeckCard(CardId::STAMPEDE, true)) == 1);
    play(stampede, DeckCard(CardId::STAMPEDE, true));
    assert(stampede.player.energy == 2 && count_in(stampede.discard_pile, CardId::STAMPEDE) == 0);
    stampede.hand.push_back(CardId::DEFEND);
    stampede.hand.push_back(CardId::STRIKE);
    end_turn_idle(stampede);
    assert(stampede.enemies[0].hp == 36 && count_in(stampede.discard_pile, CardId::STRIKE) == 1);
    assert_cost(CardId::STAMPEDE, 2, 1);

    std::cout << "[PASS] Upgraded Powers: Crimson Mantle, Demon Form, Inferno, Juggernaut, Stampede" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Ironclad Upgraded Cards ===" << std::endl;
    test_upgraded_single_target_attacks();
    test_upgraded_area_attacks();
    test_upgraded_skills();
    test_upgraded_powers();
    std::cout << "=== All Ironclad Upgraded Cards Passed! ===" << std::endl;
    return 0;
}
