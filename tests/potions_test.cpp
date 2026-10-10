// Potions drunk through CombatEngine::use_potion: one test per potion, on hand-built situations.
#include <cassert>
#include <iostream>
#include <string_view>
#include "engine/combat.h"
#include "engine/potion.h"

using namespace engine;

namespace {

// Every potion the engine resolves, the five older ones included.
constexpr std::string_view ENGINE_POTIONS[] = {
    "FIRE_POTION", "BLOCK_POTION", "ENERGY_POTION", "BLOOD_POTION", "CUNNING_POTION",
    "EXPLOSIVE_AMPOULE", "WEAK_POTION", "SWIFT_POTION", "DEXTERITY_POTION", "STRENGTH_POTION", "POISON_POTION",
    "VULNERABLE_POTION", "STAR_POTION", "FOCUS_POTION", "FLEX_POTION", "SPEED_POTION", "POTION_OF_DOOM",
    "BLESSING_OF_THE_FORGE", "FORTIFIER", "FYSH_OIL", "BONE_BREW", "POTION_OF_BINDING", "CURE_ALL", "CLARITY",
    "RADIANT_TINCTURE", "POTION_OF_CAPACITY", "MAZALETHS_GIFT", "ESSENCE_OF_DARKNESS", "BOTTLED_POTENTIAL",
    "DISTILLED_CHAOS", "SHIP_IN_A_BOTTLE", "SHACKLING_POTION", "GLOWWATER_POTION", "GAMBLERS_BREW",
    "POWDERED_DEMISE", "DUPLICATOR", "DROPLET_OF_PRECOGNITION", "STABLE_SERUM",
    "ASHWATER", "OROBIC_ACID",
};

// Jaw Worm opens with Chomp (11 damage, no block): a predictable punching bag.
CombatEngine make_combat(CharacterId hero, EnemyType enemy_type = EnemyType::JAW_WORM) {
    CombatEngine combat;
    combat.start_combat(enemy_type, hero);
    return combat;
}

// Two Lice (18 HP, Bite 6) and a Jaw Worm (42 HP, Chomp 11).
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

void end_turn_idle(CombatEngine& combat) {
    for (Enemy& foe : combat.enemies) make_enemy_idle(foe);
    combat.end_player_turn();
}

void pad_draw_pile(CombatEngine& combat, int count) {
    for (int i = 0; i < count; ++i) combat.draw_pile.push_back(CardId::DEFEND);
}

// A potion thrown at an enemy lands on the target given, and on the first living enemy when there is none.
void assert_targeted(std::string_view potion_id, StatusType status, int16_t amount) {
    CombatEngine aimed = make_pack(CharacterId::IRONCLAD);
    assert(aimed.use_potion(potion_id, 2));
    assert(aimed.enemies[2].get_status(status) == amount);
    assert(aimed.enemies[0].statuses.empty() && aimed.enemies[1].statuses.empty());

    CombatEngine out_of_range = make_pack(CharacterId::IRONCLAD);
    assert(out_of_range.use_potion(potion_id, 7));
    assert(out_of_range.enemies[0].get_status(status) == amount);

    CombatEngine no_target = make_pack(CharacterId::IRONCLAD);
    assert(no_target.use_potion(potion_id));
    assert(no_target.enemies[0].get_status(status) == amount);

    CombatEngine dead_target = make_pack(CharacterId::IRONCLAD);
    dead_target.enemies[0].hp = 0;
    assert(dead_target.use_potion(potion_id, 0));
    assert(dead_target.enemies[0].statuses.empty());
    assert(dead_target.enemies[1].get_status(status) == amount && dead_target.enemies[2].statuses.empty());
}

void test_explosive_ampoule() {
    CombatEngine combat = make_pack(CharacterId::IRONCLAD);
    combat.enemies[1].block = 4;
    assert(combat.use_potion("EXPLOSIVE_AMPOULE"));
    assert(combat.enemies[0].hp == 18 - 10 && combat.enemies[2].hp == 42 - 10);
    assert(combat.enemies[1].hp == 18 - 6 && combat.enemies[1].block == 0); // Block absorbs it
    assert(combat.enemy.hp == combat.enemies[0].hp);

    // It is not an attack: Strength and Vulnerable change nothing.
    CombatEngine plain = make_combat(CharacterId::IRONCLAD);
    plain.player.add_status(StatusType::STRENGTH, 3);
    plain.enemies[0].add_status(StatusType::VULNERABLE, 2);
    assert(plain.use_potion("EXPLOSIVE_AMPOULE"));
    assert(plain.enemies[0].hp == 42 - 10);

    // Killing the last enemies wins the fight.
    CombatEngine lethal = make_pack(CharacterId::IRONCLAD);
    lethal.enemies[2].hp = 10;
    assert(lethal.use_potion("EXPLOSIVE_AMPOULE"));
    assert(lethal.enemies[0].hp == 8 && lethal.state == CombatState::PLAYER_TURN);
    assert(lethal.selected_target_index == 0);
    assert(lethal.use_potion("EXPLOSIVE_AMPOULE"));
    assert(lethal.state == CombatState::VICTORY);
    std::cout << "[PASS] Explosive Ampoule: 10 damage to every enemy, absorbed by Block, victory" << std::endl;
}

void test_weak_potion() {
    assert_targeted("WEAK_POTION", StatusType::WEAK, 3);

    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.use_potion("WEAK_POTION", 0));
    combat.end_player_turn();
    assert(combat.player.hp == 80 - 8); // Chomp 11 x 0.75
    assert(combat.enemies[0].get_status(StatusType::WEAK) == 2);
    std::cout << "[PASS] Weak Potion: 3 Weak on the target, first living enemy by default" << std::endl;
}

void test_swift_potion() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.hand.size() == 5 && combat.draw_pile.size() == 5);
    assert(combat.use_potion("SWIFT_POTION"));
    assert(combat.hand.size() == 8 && combat.draw_pile.size() == 2);
    assert(combat.counters.cards_drawn == 3);
    std::cout << "[PASS] Swift Potion: draw 3 cards" << std::endl;
}

void test_dexterity_potion() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.use_potion("DEXTERITY_POTION"));
    assert(combat.player.get_status(StatusType::DEXTERITY) == 2);
    play(combat, CardId::DEFEND);
    assert(combat.player.block == 5 + 2);
    end_turn_idle(combat);
    assert(combat.player.get_status(StatusType::DEXTERITY) == 2); // for the rest of the combat
    std::cout << "[PASS] Dexterity Potion: 2 Dexterity for the combat" << std::endl;
}

void test_strength_potion() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.use_potion("STRENGTH_POTION"));
    assert(combat.player.get_status(StatusType::STRENGTH) == 2);
    play(combat, CardId::STRIKE);
    assert(combat.enemies[0].hp == 42 - 8);
    end_turn_idle(combat);
    assert(combat.player.get_status(StatusType::STRENGTH) == 2); // for the rest of the combat
    std::cout << "[PASS] Strength Potion: 2 Strength for the combat" << std::endl;
}

void test_poison_potion() {
    assert_targeted("POISON_POTION", StatusType::POISON, 6);

    CombatEngine combat = make_combat(CharacterId::SILENT);
    assert(combat.use_potion("POISON_POTION", 0));
    end_turn_idle(combat);
    assert(combat.enemies[0].hp == 42 - 6 && combat.enemies[0].get_status(StatusType::POISON) == 5);
    std::cout << "[PASS] Poison Potion: 6 Poison on the target, first living enemy by default" << std::endl;
}

void test_vulnerable_potion() {
    assert_targeted("VULNERABLE_POTION", StatusType::VULNERABLE, 3);

    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.use_potion("VULNERABLE_POTION", 0));
    play(combat, CardId::STRIKE);
    assert(combat.enemies[0].hp == 42 - 9); // Strike 6 x 1.5
    std::cout << "[PASS] Vulnerable Potion: 3 Vulnerable on the target, first living enemy by default" << std::endl;
}

void test_star_potion() {
    CombatEngine combat = make_combat(CharacterId::REGENT);
    assert(combat.player.sovereign_stars == 0);
    assert(combat.use_potion("STAR_POTION"));
    assert(combat.player.sovereign_stars == 3 && combat.counters.stars_gained == 3);
    std::cout << "[PASS] Star Potion: 3 Stars" << std::endl;
}

void test_focus_potion() {
    CombatEngine combat = make_combat(CharacterId::DEFECT);
    assert(combat.use_potion("FOCUS_POTION"));
    assert(combat.player.get_status(StatusType::FOCUS) == 2);
    combat.channel_orb(OrbType::LIGHTNING);
    end_turn_idle(combat);
    assert(combat.enemies[0].hp == 42 - (LIGHTNING_PASSIVE + 2));
    assert(combat.player.get_status(StatusType::FOCUS) == 2); // for the rest of the combat
    std::cout << "[PASS] Focus Potion: 2 Focus for the combat" << std::endl;
}

void test_flex_potion() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    combat.player.add_status(StatusType::STRENGTH, 1);
    assert(combat.use_potion("FLEX_POTION"));
    assert(combat.player.get_status(StatusType::STRENGTH) == 6);
    play(combat, CardId::STRIKE);
    assert(combat.enemies[0].hp == 42 - 12);
    end_turn_idle(combat);
    assert(combat.player.get_status(StatusType::STRENGTH) == 1); // only what the potion gave is lost
    std::cout << "[PASS] Flex Potion: 5 Strength until the end of the turn" << std::endl;
}

void test_speed_potion() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.use_potion("SPEED_POTION"));
    assert(combat.player.get_status(StatusType::DEXTERITY) == 5);
    play(combat, CardId::DEFEND);
    assert(combat.player.block == 5 + 5);
    end_turn_idle(combat);
    assert(combat.player.get_status(StatusType::DEXTERITY) == 0);
    std::cout << "[PASS] Speed Potion: 5 Dexterity until the end of the turn" << std::endl;
}

void test_potion_of_doom() {
    assert_targeted("POTION_OF_DOOM", StatusType::DOOM, 33);

    // Below its HP, Doom waits; the enemy dies at the end of the turn where its HP are within reach.
    CombatEngine combat = make_combat(CharacterId::NECROBINDER);
    assert(combat.use_potion("POTION_OF_DOOM", 0));
    assert(combat.counters.doom_applied == 1);
    end_turn_idle(combat);
    assert(combat.enemies[0].hp == 42 && combat.state == CombatState::PLAYER_TURN);
    combat.enemies[0].hp = 33;
    end_turn_idle(combat);
    assert(!combat.enemies[0].is_alive() && combat.state == CombatState::VICTORY);
    std::cout << "[PASS] Potion of Doom: 33 Doom on the target, first living enemy by default" << std::endl;
}

void test_blessing_of_the_forge() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.hand.push_back(CardId::STRIKE);
    combat.hand.push_back(CardId::DEFEND);
    combat.hand.push_back(DeckCard(CardId::BASH, true));
    combat.hand.push_back(CardId::BURN);
    assert(combat.use_potion("BLESSING_OF_THE_FORGE"));
    assert(combat.hand.size() == 4);
    assert(combat.hand[0].upgraded && combat.hand[1].upgraded && combat.hand[2].upgraded);
    assert(!combat.hand[3].upgraded); // Status cards have no upgrade
    for (const DeckCard& card : combat.draw_pile) assert(!card.upgraded); // only the hand

    // The upgrade follows the card for the rest of the combat.
    combat.player.energy = combat.player.max_energy;
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 42 - 9);
    assert(combat.discard_pile.back() == DeckCard(CardId::STRIKE, true));
    std::cout << "[PASS] Blessing of the Forge: every card in hand upgraded for the combat, Status cards excepted" << std::endl;
}

void test_fortifier() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.use_potion("FORTIFIER"));
    assert(combat.player.block == 0); // nothing to triple
    combat.player.block = 7;
    combat.player.add_status(StatusType::DEXTERITY, 3);
    assert(combat.use_potion("FORTIFIER"));
    assert(combat.player.block == 21); // Dexterity does not apply
    std::cout << "[PASS] Fortifier: Block tripled" << std::endl;
}

void test_fysh_oil() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.use_potion("FYSH_OIL"));
    assert(combat.player.get_status(StatusType::STRENGTH) == 1 && combat.player.get_status(StatusType::DEXTERITY) == 1);
    end_turn_idle(combat);
    assert(combat.player.get_status(StatusType::STRENGTH) == 1 && combat.player.get_status(StatusType::DEXTERITY) == 1);
    std::cout << "[PASS] Fysh Oil: 1 Strength and 1 Dexterity for the combat" << std::endl;
}

void test_bone_brew() {
    CombatEngine combat = make_combat(CharacterId::NECROBINDER);
    int16_t hp_before = combat.player.hp;
    assert(combat.use_potion("BONE_BREW"));
    assert(combat.player.minion_hp == 15);
    combat.end_player_turn(); // Chomp 11 lands on the minion
    assert(combat.player.hp == hp_before && combat.player.minion_hp == 4);
    assert(combat.use_potion("BONE_BREW"));
    assert(combat.player.minion_hp == 19);
    std::cout << "[PASS] Bone Brew: Summon 15" << std::endl;
}

void test_potion_of_binding() {
    CombatEngine combat = make_pack(CharacterId::IRONCLAD);
    combat.enemies[1].hp = 0;
    assert(combat.use_potion("POTION_OF_BINDING"));
    for (size_t i : { size_t(0), size_t(2) }) {
        assert(combat.enemies[i].get_status(StatusType::WEAK) == 1);
        assert(combat.enemies[i].get_status(StatusType::VULNERABLE) == 1);
    }
    assert(combat.enemies[1].statuses.empty()); // the dead are left alone
    std::cout << "[PASS] Potion of Binding: 1 Weak and 1 Vulnerable on every living enemy" << std::endl;
}

void test_cure_all() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.use_potion("CURE_ALL"));
    assert(combat.player.energy == 3 + 1 && combat.hand.size() == 5 + 2);
    end_turn_idle(combat);
    assert(combat.player.energy == 3); // this turn only
    std::cout << "[PASS] Cure All: 1 energy and 2 cards" << std::endl;
}

void test_clarity() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(combat, 20);
    assert(combat.use_potion("CLARITY"));
    assert(combat.hand.size() == 5 + 1 && combat.delayed_effects.size() == 1);
    for (int turn = 0; turn < 3; ++turn) {
        end_turn_idle(combat);
        assert(combat.hand.size() == 5 + 1);
    }
    assert(combat.delayed_effects.empty());
    end_turn_idle(combat);
    assert(combat.hand.size() == 5);
    std::cout << "[PASS] Clarity: 1 card now, 1 more card at the start of the next 3 turns" << std::endl;
}

void test_radiant_tincture() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.use_potion("RADIANT_TINCTURE"));
    assert(combat.player.energy == 3 + 1 && combat.delayed_effects.size() == 1);
    for (int turn = 0; turn < 3; ++turn) {
        end_turn_idle(combat);
        assert(combat.player.energy == 3 + 1);
    }
    assert(combat.delayed_effects.empty());
    end_turn_idle(combat);
    assert(combat.player.energy == 3);
    std::cout << "[PASS] Radiant Tincture: 1 energy now, 1 more at the start of the next 3 turns" << std::endl;
}

void test_potion_of_capacity() {
    CombatEngine combat = make_combat(CharacterId::DEFECT);
    assert(combat.player.orb_slots == STARTING_ORB_SLOTS);
    assert(combat.use_potion("POTION_OF_CAPACITY"));
    assert(combat.player.orb_slots == STARTING_ORB_SLOTS + 2);
    for (int orb = 0; orb < 5; ++orb) combat.channel_orb(OrbType::FROST);
    assert(combat.player.orbs.size() == 5 && combat.player.block == 0); // room for all: none was evoked

    // Never more than MAX_ORB_SLOTS.
    for (int potion = 0; potion < 4; ++potion) assert(combat.use_potion("POTION_OF_CAPACITY"));
    assert(combat.player.orb_slots == MAX_ORB_SLOTS);
    std::cout << "[PASS] Potion of Capacity: 2 Orb slots, up to the maximum" << std::endl;
}

void test_mazaleths_gift() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.use_potion("MAZALETHS_GIFT"));
    assert(combat.has_power(PowerId::TURN_STRENGTH));
    assert(combat.player.get_status(StatusType::STRENGTH) == 0); // nothing before the turn ends
    end_turn_idle(combat);
    assert(combat.player.get_status(StatusType::STRENGTH) == 1);
    play(combat, CardId::STRIKE);
    assert(combat.enemies[0].hp == 42 - 7);
    end_turn_idle(combat);
    assert(combat.player.get_status(StatusType::STRENGTH) == 2);

    // Two potions add up.
    assert(combat.use_potion("MAZALETHS_GIFT"));
    end_turn_idle(combat);
    assert(combat.player.get_status(StatusType::STRENGTH) == 4);
    std::cout << "[PASS] Mazaleth's Gift: 1 Ritual, 1 more Strength each turn" << std::endl;
}

void test_essence_of_darkness() {
    CombatEngine combat = make_combat(CharacterId::DEFECT);
    assert(combat.use_potion("ESSENCE_OF_DARKNESS"));
    assert(combat.player.orbs.size() == STARTING_ORB_SLOTS);
    for (const Orb& orb : combat.player.orbs) assert(orb == OrbType::DARK && orb.charge == DARK_PASSIVE);

    // One Dark orb for each slot, whatever their number; the orbs already there are evoked to make room.
    CombatEngine full = make_combat(CharacterId::DEFECT);
    full.change_orb_slots(2);
    for (int orb = 0; orb < 5; ++orb) full.channel_orb(OrbType::FROST);
    assert(full.use_potion("ESSENCE_OF_DARKNESS"));
    assert(full.player.block == 5 * FROST_EVOKE && full.player.orbs.size() == 5);
    for (const Orb& orb : full.player.orbs) assert(orb == OrbType::DARK);
    assert(full.counters.combat_orbs_channeled == 10);
    std::cout << "[PASS] Essence of Darkness: a Dark orb channeled for each Orb slot" << std::endl;
}

void test_bottled_potential() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    for (int i = 0; i < 3; ++i) combat.discard_pile.push_back(CardId::BASH);
    combat.exhaust_pile.push_back(CardId::BURN);
    assert(combat.hand.size() == 5 && combat.draw_pile.size() == 5);
    assert(combat.use_potion("BOTTLED_POTENTIAL"));
    assert(combat.hand.size() == 5 && combat.draw_pile.size() == 8 && combat.discard_pile.empty());
    assert(combat.exhaust_pile.size() == 1); // exhausted cards do not come back

    // The 13 cards are all still there: 5 Strikes, 4 Defends, 4 Bashes.
    int strikes = 0, defends = 0, bashes = 0;
    for (const DeckCard& card : combat.hand) {
        strikes += card.id == CardId::STRIKE;
        defends += card.id == CardId::DEFEND;
        bashes += card.id == CardId::BASH;
    }
    for (const DeckCard& card : combat.draw_pile) {
        strikes += card.id == CardId::STRIKE;
        defends += card.id == CardId::DEFEND;
        bashes += card.id == CardId::BASH;
    }
    assert(strikes == 5 && defends == 4 && bashes == 4);

    // Fewer than 5 cards in all: every one of them is drawn.
    CombatEngine small = make_combat(CharacterId::IRONCLAD);
    small.draw_pile.clear();
    small.hand.clear();
    small.hand.push_back(CardId::STRIKE);
    small.discard_pile.push_back(CardId::DEFEND);
    assert(small.use_potion("BOTTLED_POTENTIAL"));
    assert(small.hand.size() == 2 && small.draw_pile.empty() && small.discard_pile.empty());
    std::cout << "[PASS] Bottled Potential: hand and discard pile shuffled into the draw pile, 5 cards drawn" << std::endl;
}

void test_distilled_chaos() {
    // The top of the draw pile is its back: Strike, Strike, Defend are played, Bash stays.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    combat.draw_pile.clear();
    combat.draw_pile.push_back(CardId::BASH);
    combat.draw_pile.push_back(CardId::DEFEND);
    combat.draw_pile.push_back(CardId::STRIKE);
    combat.draw_pile.push_back(CardId::STRIKE);
    assert(combat.use_potion("DISTILLED_CHAOS"));
    assert(combat.enemies[0].hp == 42 - 12 && combat.player.block == 5);
    assert(combat.player.energy == 3 && combat.hand.size() == 5); // played for free, not from the hand
    assert(combat.draw_pile.size() == 1 && combat.draw_pile[0] == CardId::BASH);
    assert(combat.discard_pile.size() == 3);
    assert(combat.counters.attacks_played == 2 && combat.counters.skills_played == 1);

    // Nothing to play: the potion is drunk for nothing.
    CombatEngine empty = make_combat(CharacterId::IRONCLAD);
    empty.draw_pile.clear();
    assert(empty.use_potion("DISTILLED_CHAOS"));
    assert(empty.enemies[0].hp == 42 && empty.counters.cards_played() == 0);

    // A card played this way can win the fight...
    CombatEngine lethal = make_combat(CharacterId::IRONCLAD);
    lethal.enemies[0].hp = 6;
    lethal.draw_pile.push_back(CardId::STRIKE);
    assert(lethal.use_potion("DISTILLED_CHAOS"));
    assert(lethal.state == CombatState::VICTORY);

    // ... or cost the player's last HP.
    CombatEngine fatal = make_combat(CharacterId::IRONCLAD);
    fatal.player.hp = 3;
    fatal.draw_pile.push_back(CardId::BLOODLETTING);
    assert(fatal.use_potion("DISTILLED_CHAOS"));
    assert(fatal.player.hp == 0 && fatal.state == CombatState::DEFEAT);
    std::cout << "[PASS] Distilled Chaos: the top 3 cards of the draw pile played for free, victory, defeat" << std::endl;
}

void test_ship_in_a_bottle() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    combat.player.add_status(StatusType::DEXTERITY, 3);
    assert(combat.use_potion("SHIP_IN_A_BOTTLE"));
    assert(combat.player.block == 10 && combat.delayed_effects.size() == 1); // Dexterity does not apply
    end_turn_idle(combat);
    assert(combat.player.block == 10 && combat.delayed_effects.empty());
    end_turn_idle(combat);
    assert(combat.player.block == 0);
    std::cout << "[PASS] Ship in a Bottle: 10 Block now, 10 Block next turn" << std::endl;
}

void test_shackling_potion() {
    CombatEngine combat = make_pack(CharacterId::IRONCLAD);
    combat.enemies[1].add_status(StatusType::STRENGTH, 2);
    assert(combat.use_potion("SHACKLING_POTION"));
    assert(combat.enemies[0].get_status(StatusType::STRENGTH) == -7);
    assert(combat.enemies[1].get_status(StatusType::STRENGTH) == -5);
    assert(combat.enemies[2].get_status(StatusType::STRENGTH) == -7);
    combat.end_player_turn();
    assert(combat.player.hp == 80 - 0 - 1 - 4); // Bite 6 - 7, Bite 6 - 5, Chomp 11 - 7
    // The Strength comes back once each enemy has acted.
    assert(combat.enemies[0].get_status(StatusType::STRENGTH) == 0);
    assert(combat.enemies[1].get_status(StatusType::STRENGTH) == 2);
    assert(combat.enemies[2].get_status(StatusType::STRENGTH) == 0);
    std::cout << "[PASS] Shackling Potion: every enemy loses 7 Strength until the end of its turn" << std::endl;
}

void test_glowwater_potion() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    for (int i = 0; i < 4; ++i) combat.hand.push_back(CardId::STRIKE);
    pad_draw_pile(combat, 12);
    assert(combat.use_potion("GLOWWATER_POTION"));
    assert(combat.exhaust_pile.size() == 4 && combat.exhausted_this_turn == 4);
    for (const DeckCard& card : combat.exhaust_pile) assert(card.id == CardId::STRIKE);
    assert(combat.hand.size() == 10 && combat.draw_pile.size() == 2);
    for (const DeckCard& card : combat.hand) assert(card.id == CardId::DEFEND);

    // Cards drawn because a card was exhausted (Dark Embrace) are not exhausted with the hand.
    CombatEngine embrace = make_combat(CharacterId::IRONCLAD);
    embrace.add_power(PowerId::DARK_EMBRACE, 1);
    embrace.hand.clear();
    embrace.draw_pile.clear();
    for (int i = 0; i < 4; ++i) embrace.hand.push_back(CardId::STRIKE);
    pad_draw_pile(embrace, 20);
    assert(embrace.use_potion("GLOWWATER_POTION"));
    assert(embrace.exhaust_pile.size() == 4);
    for (const DeckCard& card : embrace.exhaust_pile) assert(card.id == CardId::STRIKE);
    assert(embrace.hand.size() == 10 && embrace.draw_pile.size() == 10); // 4 from Dark Embrace, 6 of the 10

    // An empty hand: the 10 cards are still drawn.
    CombatEngine bare = make_combat(CharacterId::IRONCLAD);
    bare.hand.clear();
    pad_draw_pile(bare, 10);
    assert(bare.use_potion("GLOWWATER_POTION"));
    assert(bare.exhaust_pile.empty() && bare.hand.size() == 10);
    std::cout << "[PASS] Glowwater Potion: hand exhausted, 10 cards drawn" << std::endl;
}

void test_gamblers_brew() {
    // Discard any number of cards, then draw that many.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.hand.size() == 5 && combat.draw_pile.size() == 5);
    assert(combat.use_potion("GAMBLERS_BREW"));
    assert(combat.choice_pending());
    assert(combat.pending_choices[0].kind == ChoiceKind::DISCARD);
    assert(combat.pending_choices[0].optional);
    assert(combat.pending_choices[0].then == ChoiceThen::DRAW_PER_CARD);
    assert(combat.choice_size() == 5);
    assert(std::string_view(combat.combat_log) == "Gambler's Brew!");

    // Pick 2 cards to discard: 2 are discarded, 2 are drawn.
    uint8_t picks[] = { 1, 3 };
    assert(combat.resolve_choice(picks));
    assert(!combat.choice_pending());
    assert(combat.hand.size() == 5);
    assert(combat.discard_pile.size() == 2);
    assert(combat.draw_pile.size() == 3);
    assert(combat.counters.cards_drawn == 2);

    // Discarding no cards: hand stays intact.
    CombatEngine keep = make_combat(CharacterId::IRONCLAD);
    assert(keep.use_potion("GAMBLERS_BREW"));
    assert(keep.choice_pending());
    keep.resolve_choice_default();
    assert(!keep.choice_pending());
    assert(keep.hand.size() == 5);
    assert(keep.discard_pile.empty());
    assert(keep.draw_pile.size() == 5);

    // Discarding the entire hand.
    CombatEngine all = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(all, 5);
    assert(all.use_potion("GAMBLERS_BREW"));
    uint8_t all_picks[] = { 0, 1, 2, 3, 4 };
    assert(all.resolve_choice(all_picks));
    assert(all.hand.size() == 5);
    assert(all.discard_pile.size() == 5);
    assert(all.draw_pile.size() == 5);

    // Empty hand: nothing discarded, nothing drawn, no choice asked.
    CombatEngine empty = make_combat(CharacterId::IRONCLAD);
    empty.hand.clear();
    assert(empty.use_potion("GAMBLERS_BREW"));
    assert(!empty.choice_pending());
    assert(empty.hand.empty());
    assert(empty.discard_pile.empty());

    std::cout << "[PASS] Gambler's Brew: discard any number of cards, draw that many" << std::endl;
}

void test_powdered_demise() {
    // Basic effect: Jaw Worm (42 HP) loses 9 HP at the end of each of its turns.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.enemies[0].hp == 42);
    assert(combat.use_potion("POWDERED_DEMISE"));
    assert(std::string_view(combat.combat_log) == "Powdered Demise!");
    assert(combat.enemies[0].hp == 42); // no immediate damage

    // Turn 1 end: loses 9 HP.
    end_turn_idle(combat);
    assert(combat.enemies[0].hp == 42 - 9);

    // Turn 2 end: loses 9 HP again.
    end_turn_idle(combat);
    assert(combat.enemies[0].hp == 42 - 18);

    // HP loss is not absorbed by Block.
    make_enemy_idle(combat.enemies[0]);
    combat.enemies[0].current_intent.block = 15;
    combat.end_player_turn();
    assert(combat.enemies[0].hp == 42 - 27);
    assert(combat.enemies[0].block == 15);

    // Multi-enemy: targeting a specific enemy (e.g. enemy 1 in a pack).
    CombatEngine pack = make_pack(CharacterId::IRONCLAD);
    assert(pack.use_potion("POWDERED_DEMISE", 1));
    end_turn_idle(pack);
    assert(pack.enemies[0].hp == 18);      // untouched
    assert(pack.enemies[1].hp == 18 - 9);  // targeted
    assert(pack.enemies[2].hp == 42);      // untouched

    // Stacking: using two Powdered Demise on the same enemy.
    CombatEngine stacked = make_combat(CharacterId::IRONCLAD);
    assert(stacked.use_potion("POWDERED_DEMISE"));
    assert(stacked.use_potion("POWDERED_DEMISE"));
    end_turn_idle(stacked);
    assert(stacked.enemies[0].hp == 42 - 18);

    // Lethal: kills the enemy at the end of its turn and achieves victory.
    CombatEngine lethal = make_combat(CharacterId::IRONCLAD);
    lethal.enemies[0].hp = 5;
    assert(lethal.use_potion("POWDERED_DEMISE"));
    end_turn_idle(lethal);
    assert(lethal.enemies[0].hp == 0 && lethal.state == CombatState::VICTORY);

    // Buffer cancels the HP loss.
    CombatEngine buffed = make_combat(CharacterId::IRONCLAD);
    buffed.enemies[0].add_status(StatusType::BUFFER, 1);
    assert(buffed.use_potion("POWDERED_DEMISE"));
    end_turn_idle(buffed);
    assert(buffed.enemies[0].hp == 42 && buffed.enemies[0].get_status(StatusType::BUFFER) == 0);

    // Intangible reduces the HP loss to 1.
    CombatEngine intang = make_combat(CharacterId::IRONCLAD);
    intang.enemies[0].add_status(StatusType::INTANGIBLE, 2);
    assert(intang.use_potion("POWDERED_DEMISE"));
    end_turn_idle(intang);
    assert(intang.enemies[0].hp == 41);

    std::cout << "[PASS] Powdered Demise: enemy loses 9 HP at the end of each of its turns" << std::endl;
}

void test_duplicator() {
    // Basic effect: next card (Attack) is played twice.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.use_potion("DUPLICATOR"));
    assert(std::string_view(combat.combat_log) == "Duplicator!");
    assert(combat.has_power(PowerId::DUPLICATION));
    assert(combat.power_amount(PowerId::DUPLICATION) == 1);

    // Play Strike: 6 * 2 = 12 damage, 1 energy spent, goes to discard once.
    play(combat, CardId::STRIKE);
    assert(combat.enemies[0].hp == 42 - 12);
    assert(combat.player.energy == combat.player.max_energy - 1);
    assert(combat.discard_pile.size() == 1 && combat.discard_pile[0].id == CardId::STRIKE);
    assert(!combat.has_power(PowerId::DUPLICATION));

    // Next Strike is played only once (Duplication was consumed).
    play(combat, CardId::STRIKE);
    assert(combat.enemies[0].hp == 42 - 18);

    // Skill duplication: next Skill gives double block.
    CombatEngine skill_combat = make_combat(CharacterId::IRONCLAD);
    assert(skill_combat.use_potion("DUPLICATOR"));
    play(skill_combat, CardId::DEFEND);
    assert(skill_combat.player.block == 10);
    assert(!skill_combat.has_power(PowerId::DUPLICATION));

    // Power duplication: next Power is applied twice and stays in play once.
    CombatEngine power_combat = make_combat(CharacterId::IRONCLAD);
    assert(power_combat.use_potion("DUPLICATOR"));
    play(power_combat, CardId::INFLAME);
    assert(power_combat.player.get_status(StatusType::STRENGTH) == 4);
    assert(power_combat.powers_in_play.size() == 1);
    assert(!power_combat.has_power(PowerId::DUPLICATION));

    // Expires at the end of the turn if no card is played.
    CombatEngine expire_combat = make_combat(CharacterId::IRONCLAD);
    assert(expire_combat.use_potion("DUPLICATOR"));
    assert(expire_combat.has_power(PowerId::DUPLICATION));
    end_turn_idle(expire_combat);
    assert(!expire_combat.has_power(PowerId::DUPLICATION));
    play(expire_combat, CardId::STRIKE);
    assert(expire_combat.enemies[0].hp == 42 - 6);

    // Stacking: two Duplicator potions duplicate the next 2 cards.
    CombatEngine stacked = make_combat(CharacterId::IRONCLAD);
    assert(stacked.use_potion("DUPLICATOR"));
    assert(stacked.use_potion("DUPLICATOR"));
    assert(stacked.power_amount(PowerId::DUPLICATION) == 2);
    play(stacked, CardId::STRIKE);
    assert(stacked.enemies[0].hp == 42 - 12);
    assert(stacked.power_amount(PowerId::DUPLICATION) == 1);
    play(stacked, CardId::DEFEND);
    assert(stacked.player.block == 10);
    assert(!stacked.has_power(PowerId::DUPLICATION));
    play(stacked, CardId::STRIKE);
    assert(stacked.enemies[0].hp == 42 - 18);

    // Target consistency: pack combat, target specific enemy.
    CombatEngine pack = make_pack(CharacterId::IRONCLAD);
    assert(pack.use_potion("DUPLICATOR"));
    play(pack, CardId::STRIKE, 1);
    assert(pack.enemies[0].hp == 18);     // untouched
    assert(pack.enemies[1].hp == 18 - 12); // hit twice
    assert(pack.enemies[2].hp == 42);     // untouched

    std::cout << "[PASS] Duplicator: next card this turn is played an extra time" << std::endl;
}

void test_droplet_of_precognition() {
    // Choose 1 card of the draw pile and add it to the hand: the hand grows by one, the draw pile loses it.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.hand.size() == 5 && combat.draw_pile.size() == 5);
    assert(combat.use_potion("DROPLET_OF_PRECOGNITION"));
    assert(combat.choice_pending());
    assert(combat.pending_choices[0].kind == ChoiceKind::TO_HAND);
    assert(combat.pending_choices[0].source == Pile::DRAW);
    assert(combat.pending_choices[0].count == 1);
    assert(!combat.pending_choices[0].optional);
    assert(combat.choice_size() == 1);
    assert(std::string_view(combat.combat_log) == "Droplet of Precognition!");
    assert(combat.player.energy == 3 && combat.hand.size() == 5);

    const CardId picked = combat.draw_pile[1].id;
    const uint8_t pick[] = { 1 };
    assert(combat.resolve_choice(pick));
    assert(!combat.choice_pending());
    assert(combat.hand.size() == 6 && combat.hand[5].id == picked);
    assert(combat.draw_pile.size() == 4);
    assert(combat.discard_pile.empty() && combat.exhaust_pile.empty());

    // Empty draw pile: nothing to choose from, the discard pile is not used.
    CombatEngine empty = make_combat(CharacterId::IRONCLAD);
    empty.draw_pile.clear();
    empty.discard_pile.push_back(DeckCard(CardId::BASH));
    assert(empty.use_potion("DROPLET_OF_PRECOGNITION"));
    assert(!empty.choice_pending());
    assert(empty.hand.size() == 5 && empty.discard_pile.size() == 1);

    std::cout << "[PASS] Droplet of Precognition: add a card of the draw pile to the hand" << std::endl;
}

void test_orobic_acid() {
    // One Attack, one Skill and one Power are added to the hand, free this turn.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.hand.size() == 5);
    assert(combat.use_potion("OROBIC_ACID"));
    assert(combat.hand.size() == 8);
    assert(CardDef::get(combat.hand[5].id).type == CardType::ATTACK);
    assert(CardDef::get(combat.hand[6].id).type == CardType::SKILL);
    assert(CardDef::get(combat.hand[7].id).type == CardType::POWER);
    for (size_t i = 5; i < 8; ++i) assert(combat.hand[i].free_this_turn);
    for (size_t i = 0; i < 5; ++i) assert(!combat.hand[i].free_this_turn);
    assert(combat.player.energy == 3);
    assert(std::string_view(combat.combat_log) == "Orobic Acid!");

    // The added cards cost 0: playing one spends no energy.
    combat.hand[5] = DeckCard(CardId::STRIKE);
    combat.hand[5].free_this_turn = true;
    assert(combat.play_card(5, 0));
    assert(combat.player.energy == 3);

    std::cout << "[PASS] Orobic Acid: an Attack, a Skill and a Power added to the hand, free this turn" << std::endl;
}

void test_unknown_potion() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(!combat.use_potion("NOT_A_POTION"));
    assert(!combat.use_potion(""));
    assert(!combat.use_potion("fire_potion")); // ids are matched exactly
    assert(combat.player.hp == 80 && combat.player.block == 0 && combat.player.energy == 3);
    assert(combat.enemies[0].hp == 42 && combat.hand.size() == 5);
    assert(combat.state == CombatState::PLAYER_TURN);

    // Every potion the engine resolves is one of the game's.
    for (std::string_view potion_id : ENGINE_POTIONS) assert(find_potion(potion_id) != nullptr);
    std::cout << "[PASS] Unknown potion: refused, nothing happens" << std::endl;
}

void test_refused_outside_player_turn() {
    for (CombatState other : { CombatState::ENEMY_TURN, CombatState::VICTORY, CombatState::DEFEAT }) {
        for (std::string_view potion_id : ENGINE_POTIONS) {
            CombatEngine combat = make_pack(CharacterId::DEFECT);
            combat.state = other;
            assert(!combat.use_potion(potion_id, 0));
            assert(combat.state == other);
            assert(combat.player.hp == combat.player.max_hp && combat.player.block == 0 && combat.player.energy == 3);
            assert(combat.player.statuses.empty() && combat.player.orbs.empty() && combat.powers.empty());
            assert(combat.hand.size() == 5 && combat.exhaust_pile.empty() && combat.delayed_effects.empty());
            assert(combat.enemies[0].hp == 18 && combat.enemies[0].statuses.empty());
        }
    }

    // During the player's turn, each of them is drunk.
    for (std::string_view potion_id : ENGINE_POTIONS) {
        CombatEngine combat = make_pack(CharacterId::DEFECT);
        assert(combat.use_potion(potion_id, 0));
    }
    std::cout << "[PASS] Potions are refused outside the player's turn: enemy turn, victory, defeat" << std::endl;
}

} // namespace

void test_stable_serum() {
    // The hand is kept at the end of the next 2 turns, then discarded again.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(combat, 30);
    assert(combat.use_potion("STABLE_SERUM"));
    assert(combat.has_power(PowerId::STABLE_SERUM));
    assert(combat.power_amount(PowerId::STABLE_SERUM) == 2);
    assert(std::string_view(combat.combat_log) == "Stable Serum!");
    assert(combat.player.energy == 3 && combat.hand.size() == 5);

    end_turn_idle(combat);
    // Turn 2: the 5 cards stayed, 5 more were drawn, nothing was discarded.
    assert(combat.discard_pile.empty());
    assert(combat.hand.size() == 10);
    assert(combat.power_amount(PowerId::STABLE_SERUM) == 1);

    end_turn_idle(combat);
    assert(combat.discard_pile.empty());
    assert(combat.hand.size() == 10);
    assert(!combat.has_power(PowerId::STABLE_SERUM));

    end_turn_idle(combat);
    assert(combat.discard_pile.size() == 10);
    assert(combat.hand.size() == 5);

    std::cout << "[PASS] Stable Serum: retain the hand for 2 turns" << std::endl;
}

void test_ashwater() {
    // Exhaust any number of cards of the hand: the choice is optional and capped by the hand size.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    assert(combat.hand.size() == 5);
    assert(combat.use_potion("ASHWATER"));
    assert(combat.choice_pending());
    assert(combat.pending_choices[0].kind == ChoiceKind::EXHAUST);
    assert(combat.pending_choices[0].source == Pile::HAND);
    assert(combat.pending_choices[0].optional);
    assert(combat.choice_size() == 5);
    assert(std::string_view(combat.combat_log) == "Ashwater!");
    assert(combat.player.energy == 3);

    const CardId first = combat.hand[0].id;
    const CardId third = combat.hand[2].id;
    const uint8_t pick[] = { 0, 2 };
    assert(combat.resolve_choice(pick));
    assert(!combat.choice_pending());
    assert(combat.hand.size() == 3);
    assert(combat.exhaust_pile.size() == 2);
    assert((combat.exhaust_pile[0].id == first && combat.exhaust_pile[1].id == third)
        || (combat.exhaust_pile[0].id == third && combat.exhaust_pile[1].id == first));
    assert(combat.discard_pile.empty() && combat.draw_pile.size() == 5);

    // Picking nothing is allowed.
    CombatEngine none = make_combat(CharacterId::IRONCLAD);
    assert(none.use_potion("ASHWATER"));
    assert(none.resolve_choice(std::span<const uint8_t>{}));
    assert(none.hand.size() == 5 && none.exhaust_pile.empty());

    std::cout << "[PASS] Ashwater: exhaust any number of cards of the hand" << std::endl;
}

int main() {
    std::cout << "=== Potions ===" << std::endl;
    test_explosive_ampoule();
    test_weak_potion();
    test_swift_potion();
    test_dexterity_potion();
    test_strength_potion();
    test_poison_potion();
    test_vulnerable_potion();
    test_star_potion();
    test_focus_potion();
    test_flex_potion();
    test_speed_potion();
    test_potion_of_doom();
    test_blessing_of_the_forge();
    test_fortifier();
    test_fysh_oil();
    test_bone_brew();
    test_potion_of_binding();
    test_cure_all();
    test_clarity();
    test_radiant_tincture();
    test_potion_of_capacity();
    test_mazaleths_gift();
    test_essence_of_darkness();
    test_bottled_potential();
    test_distilled_chaos();
    test_ship_in_a_bottle();
    test_shackling_potion();
    test_glowwater_potion();
    test_gamblers_brew();
    test_powdered_demise();
    test_duplicator();
    test_droplet_of_precognition();
    test_orobic_acid();
    test_unknown_potion();
    test_refused_outside_player_turn();
    std::cout << "=== All Potions Passed! ===" << std::endl;
    test_stable_serum();
    test_ashwater();
    return 0;
}
