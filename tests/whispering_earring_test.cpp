// Whispering Earring (Ancient relic): 1 more energy each turn, and Vakuu plays the first turn of each combat.
// He plays the hand from left to right while he can pay, aims at the leftmost living enemy, leaves the cards
// that would ask the player to pick cards and the ones that cannot be played, then ends the turn.
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <string_view>
#include "engine/card.h"
#include "engine/character.h"
#include "engine/combat.h"
#include "engine/entity.h"
#include "engine/fixed_vector.h"
#include "engine/map.h"
#include "engine/power.h"
#include "engine/run.h"

using namespace engine;

namespace {

void make_idle(CombatEngine& combat) {
    for (Enemy& foe : combat.enemies) {
        foe.current_intent = EnemyIntent{};
        foe.current_intent.type = EnemyIntentType::DEFEND;
    }
}

// A fight whose first turn is Vakuu's, as RunManager opens it for the owner of the relic: the powers of the
// relic are registered between the two halves of open_first_turn. The piles are emptied for the test to fill.
CombatEngine make_fight(int enemy_count, bool with_relic = true) {
    FixedVector<EnemyType, 3> pack;
    for (int i = 0; i < enemy_count; ++i) pack.push_back(EnemyType::CULTIST);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.reset_combat_state();
    if (with_relic) {
        combat.add_power(PowerId::TURN_ENERGY, 1);
        combat.add_power(PowerId::WHISPERING_EARRING, 1);
    }
    combat.begin_first_turn();
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.exhaust_pile.clear();
    combat.player.energy = 3;
    combat.player.block = 0;
    for (Enemy& foe : combat.enemies) {
        foe.max_hp = 100;
        foe.hp = 100;
        foe.block = 0;
    }
    make_idle(combat);
    return combat;
}

void fill_hand(CombatEngine& combat, std::initializer_list<DeckCard> cards) {
    combat.hand.clear();
    for (const DeckCard& card : cards) combat.hand.push_back(card);
}

bool hand_is(const CombatEngine& combat, std::initializer_list<CardId> ids) {
    if (combat.hand.size() != ids.size()) return false;
    size_t index = 0;
    for (CardId id : ids) {
        if (combat.hand[index++].id != id) return false;
    }
    return true;
}

void enter_monster_room(RunManager& run) {
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    for (Enemy& foe : run.combat.enemies) {
        foe.max_hp = 200;
        foe.hp = 200;
    }
    make_idle(run.combat);
}

void test_energy_each_turn() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("WHISPERING_EARRING");
    assert(run.has_relic("WHISPERING_EARRING"));
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);

    enter_monster_room(run);
    enter_monster_room(plain);
    assert(run.combat.has_power(PowerId::WHISPERING_EARRING));
    assert(run.combat.vakuu_turn_active);
    assert(run.combat.player.energy == plain.combat.player.energy + 1);

    // Nothing is played before the first step: the scene shows each card.
    assert(run.combat.hand.size() == 5 && run.combat.counters.cards_played() == 0);
    assert(run.combat.vakuu_play_turn() > 0);
    plain.combat.end_player_turn();
    assert(run.combat.turn_number == 2 && plain.combat.turn_number == 2);
    assert(run.combat.player.energy == plain.combat.player.energy + 1);

    make_idle(run.combat);
    make_idle(plain.combat);
    run.combat.end_player_turn();
    plain.combat.end_player_turn();
    assert(run.combat.turn_number == 3);
    assert(run.combat.player.energy == plain.combat.player.energy + 1);
    std::cout << "[PASS] 1 more energy at the start of each turn" << std::endl;
}

void test_left_to_right() {
    CombatEngine combat = make_fight(1);
    assert(combat.vakuu_turn_active);
    fill_hand(combat, { CardId::DEFEND, CardId::STRIKE, CardId::BASH, CardId::STRIKE });

    // Defend (1), Strike (1), Bash (2) cannot be paid with the 1 energy left, Strike (1).
    assert(combat.vakuu_next_card() == 0);
    assert(combat.vakuu_step());
    assert(combat.player.block == 5 && combat.player.energy == 2);
    assert(hand_is(combat, { CardId::STRIKE, CardId::BASH, CardId::STRIKE }));

    assert(combat.vakuu_next_card() == 0);
    assert(combat.vakuu_step());
    assert(combat.enemies[0].hp == 94 && combat.player.energy == 1);
    assert(hand_is(combat, { CardId::BASH, CardId::STRIKE }));

    assert(combat.vakuu_next_card() == 1);
    assert(combat.vakuu_step());
    assert(combat.enemies[0].hp == 88 && combat.player.energy == 0);
    assert(hand_is(combat, { CardId::BASH }));

    // Nothing left to play: the turn must end, and it is still Vakuu's until it does.
    assert(combat.vakuu_next_card() == -1);
    assert(!combat.vakuu_step());
    assert(combat.vakuu_turn_active && combat.state == CombatState::PLAYER_TURN && combat.turn_number == 1);
    assert(combat.counters.energy_spent == 3 && combat.counters.cards_played() == 3);
    std::cout << "[PASS] First turn played from left to right, energy paid for each card" << std::endl;
}

void test_targets_leftmost_living_enemy() {
    CombatEngine combat = make_fight(3);
    combat.enemies[0].hp = 6;
    fill_hand(combat, { CardId::STRIKE, CardId::STRIKE, CardId::STRIKE });
    // The player's own cursor is not followed.
    combat.selected_target_index = 2;

    assert(combat.vakuu_step());
    assert(!combat.enemies[0].is_alive());
    assert(combat.enemies[1].hp == 100 && combat.enemies[2].hp == 100);
    assert(combat.vakuu_step());
    assert(combat.vakuu_step());
    assert(combat.enemies[1].hp == 88 && combat.enemies[2].hp == 100);
    assert(!combat.vakuu_step());
    std::cout << "[PASS] Attacks aim at the leftmost enemy still alive" << std::endl;
}

void test_zero_cost_after_energy() {
    CombatEngine combat = make_fight(1);
    combat.player.energy = 2;
    fill_hand(combat, { CardId::BASH, CardId::STRIKE, CardId::ANGER, CardId::DEFEND });

    // Bash takes the 2 energy; Strike and Defend stay, Anger costs 0 and is still played.
    assert(combat.vakuu_step());
    assert(combat.player.energy == 0 && combat.enemies[0].hp == 92);
    assert(combat.vakuu_next_card() == 1);
    assert(combat.vakuu_step());
    assert(combat.player.energy == 0);
    assert(combat.enemies[0].hp == 83); // 6, Vulnerable
    assert(!combat.vakuu_step());
    assert(hand_is(combat, { CardId::STRIKE, CardId::DEFEND }));
    assert(combat.player.block == 0);
    std::cout << "[PASS] Cards that cost 0 are played once the energy is spent" << std::endl;
}

void test_skipped_cards() {
    CombatEngine combat = make_fight(1);
    for (CardId id : { CardId::DEFEND, CardId::BASH, CardId::STRIKE }) {
        combat.draw_pile.push_back(DeckCard(id));
        combat.discard_pile.push_back(DeckCard(id));
    }
    // Wish, Survivor and Dual Wield ask for a pick; Wound is Unplayable; Clash needs a hand of Attacks only.
    fill_hand(combat, { CardId::WISH, CardId::WOUND, CardId::SURVIVOR, CardId::DUAL_WIELD, CardId::CLASH,
                        CardId::STRIKE });
    assert(CombatEngine::opens_choice(DeckCard(CardId::WISH)));
    assert(CombatEngine::opens_choice(DeckCard(CardId::SECRET_TECHNIQUE)));
    assert(!CombatEngine::opens_choice(DeckCard(CardId::STRIKE)));
    assert(combat.can_play_card(0) && combat.can_play_card(2) && combat.can_play_card(3));

    assert(combat.vakuu_next_card() == 5);
    assert(combat.vakuu_step());
    assert(!combat.vakuu_step());
    assert(!combat.choice_pending());
    assert(hand_is(combat, { CardId::WISH, CardId::WOUND, CardId::SURVIVOR, CardId::DUAL_WIELD, CardId::CLASH }));
    assert(combat.player.energy == 2 && combat.enemies[0].hp == 94 && combat.player.block == 0);
    assert(combat.draw_pile.size() == 3 && combat.exhaust_pile.empty());

    // An X card with nothing left to spend is not thrown away.
    CombatEngine whirl = make_fight(1);
    whirl.player.energy = 1;
    fill_hand(whirl, { CardId::STRIKE, CardId::WHIRLWIND });
    assert(whirl.vakuu_step());
    assert(!whirl.vakuu_step());
    assert(hand_is(whirl, { CardId::WHIRLWIND }));
    std::cout << "[PASS] Choice cards, Unplayable cards and failed conditions are skipped" << std::endl;
}

// Whatever the card, Vakuu never leaves the player in front of a choice: every card that asks one is known
// to opens_choice.
void test_no_card_played_opens_a_choice() {
    for (int raw = 1; raw < static_cast<int>(CardId::COUNT); ++raw) {
        for (bool upgraded : { false, true }) {
            const DeckCard card(static_cast<CardId>(raw), upgraded);
            if (CombatEngine::opens_choice(card)) continue;
            CombatEngine combat = make_fight(1);
            combat.enemies[0].max_hp = 999;
            combat.enemies[0].hp = 999;
            combat.player.energy = 9;
            combat.player.sovereign_stars = 9;
            combat.player.summon_minion(10, 0);
            for (int copy = 0; copy < 3; ++copy) {
                for (CardId id : { CardId::DEFEND, CardId::BASH, CardId::STRIKE }) {
                    combat.draw_pile.push_back(DeckCard(id));
                    combat.discard_pile.push_back(DeckCard(id));
                }
            }
            fill_hand(combat, { card, CardId::STRIKE, CardId::DEFEND, CardId::BASH, CardId::INFLAME });
            if (!combat.can_play_card(0)) continue;
            combat.play_card(0, 0);
            if (combat.choice_pending()) {
                std::cout << "Card " << raw << (upgraded ? "+" : "") << " asks a choice" << std::endl;
                assert(false);
            }
        }
    }
    std::cout << "[PASS] Every card that asks a choice is one Vakuu skips" << std::endl;
}

void test_cards_drawn_and_made_are_played() {
    CombatEngine combat = make_fight(1);
    combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    fill_hand(combat, { CardId::POMMEL_STRIKE });
    assert(combat.vakuu_step());
    assert(hand_is(combat, { CardId::STRIKE }));
    assert(combat.vakuu_step());
    assert(!combat.vakuu_step());
    assert(combat.enemies[0].hp == 100 - 9 - 6 && combat.player.energy == 1);

    // A hand that never runs out: he stops by himself.
    CombatEngine endless = make_fight(1);
    endless.enemies[0].max_hp = 999;
    endless.enemies[0].hp = 999;
    fill_hand(endless, { CardId::ANGER });
    int played = 0;
    while (endless.vakuu_step()) {
        ++played;
        endless.discard_pile.clear();
        endless.hand.push_back(DeckCard(CardId::ANGER));
        assert(played <= VAKUU_MAX_PLAYS);
    }
    assert(played == VAKUU_MAX_PLAYS);
    assert(endless.vakuu_next_card() == -1);
    std::cout << "[PASS] Cards drawn or made during the turn are played, up to the cap" << std::endl;
}

void test_second_turn_is_the_players() {
    CombatEngine combat = make_fight(1);
    for (int copy = 0; copy < 8; ++copy) combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    fill_hand(combat, { CardId::STRIKE, CardId::DEFEND, CardId::WOUND });

    // The whole turn at once: two cards, then the end of the turn and the enemies' turn.
    assert(combat.vakuu_play_turn() == 2);
    assert(combat.state == CombatState::PLAYER_TURN && combat.turn_number == 2);
    assert(!combat.vakuu_turn_active);
    assert(combat.enemies[0].hp == 94);
    assert(combat.player.energy == 4); // 3 and the relic's 1
    assert(combat.hand.size() == 5);

    assert(combat.vakuu_next_card() == -1);
    assert(!combat.vakuu_step());
    assert(combat.vakuu_play_turn() == 0);
    assert(combat.turn_number == 2 && combat.hand.size() == 5 && combat.player.energy == 4);
    assert(combat.counters.cards_played() == 0);
    // The player plays as usual.
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 88);
    std::cout << "[PASS] The second turn is the player's" << std::endl;
}

void test_victory_stops_the_turn() {
    CombatEngine combat = make_fight(1);
    combat.enemies[0].hp = 6;
    fill_hand(combat, { CardId::STRIKE, CardId::STRIKE });
    assert(combat.vakuu_play_turn() == 1);
    assert(combat.state == CombatState::VICTORY);
    assert(!combat.vakuu_turn_active);
    assert(combat.hand.size() == 1 && combat.turn_number == 1);
    std::cout << "[PASS] Vakuu stops when the fight is won" << std::endl;
}

void test_nothing_without_the_relic() {
    CombatEngine combat = make_fight(1, false);
    fill_hand(combat, { CardId::STRIKE, CardId::DEFEND });
    assert(!combat.vakuu_turn_active);
    assert(combat.vakuu_next_card() == -1);
    assert(!combat.vakuu_step());
    assert(combat.vakuu_play_turn() == 0);
    assert(combat.hand.size() == 2 && combat.player.energy == 3 && combat.turn_number == 1);
    assert(combat.enemies[0].hp == 100 && combat.state == CombatState::PLAYER_TURN);

    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("BLOOD_SOAKED_ROSE");
    enter_monster_room(plain);
    assert(!plain.combat.has_power(PowerId::WHISPERING_EARRING));
    assert(!plain.combat.vakuu_turn_active);
    assert(plain.combat.vakuu_play_turn() == 0);
    assert(plain.combat.hand.size() == 5 && plain.combat.turn_number == 1);
    assert(plain.combat.counters.cards_played() == 0);

    // The relic is an Ancient's: it is never drawn among the relics of the rewards.
    for (std::string_view id : RunManager::active_relics()) assert(id != "WHISPERING_EARRING");
    std::cout << "[PASS] Without the relic nothing is played for the player" << std::endl;
}

// Every combat of the run starts with a turn of Vakuu.
void test_every_combat() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("WHISPERING_EARRING");
    enter_monster_room(run);
    run.combat.vakuu_play_turn();
    assert(!run.combat.vakuu_turn_active);
    enter_monster_room(run);
    assert(run.combat.vakuu_turn_active && run.combat.turn_number == 1);
    std::cout << "[PASS] Vakuu plays the first turn of each combat" << std::endl;
}

}  // namespace

int main() {
    std::cout << "=== Whispering Earring ===" << std::endl;
    test_energy_each_turn();
    test_left_to_right();
    test_targets_leftmost_living_enemy();
    test_zero_cost_after_energy();
    test_skipped_cards();
    test_no_card_played_opens_a_choice();
    test_cards_drawn_and_made_are_played();
    test_second_turn_is_the_players();
    test_victory_stops_the_turn();
    test_nothing_without_the_relic();
    test_every_combat();
    std::cout << "All tests passed" << std::endl;
    return 0;
}
