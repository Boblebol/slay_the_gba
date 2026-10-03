// Souls of the Necrobinder (#106). In bible/data a Soul is a card, not a gauge: a token that costs 0, "Draw 2
// cards. Exhaust.", which other cards add to the draw pile, the hand or the discard pile. One case per rule.
//
// The card itself is engine::SOUL_CARD. While it is CardId::NONE (the card is not in the card table yet), the
// cases that need a Soul to be drawn or played are skipped and say so; the others run either way.
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

constexpr bool SOUL_EXISTS = SOUL_CARD != CardId::NONE;

// A Necrobinder against `count` enemies with 100 HP each, hand emptied: the draw pile holds the starter deck.
CombatEngine make_fight(int count = 1) {
    FixedVector<EnemyType, 3> pack;
    for (int i = 0; i < count; ++i) pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::NECROBINDER);
    for (Enemy& foe : combat.enemies) {
        foe.max_hp = 100;
        foe.hp = 100;
    }
    while (!combat.hand.empty()) {
        combat.draw_pile.push_back(combat.hand.back());
        combat.hand.pop_back();
    }
    return combat;
}

// Puts `card` alone in hand and plays it. The energy is left as it is: a Soul costs nothing.
void play(CombatEngine& combat, DeckCard card, int target = -1) {
    combat.hand.clear();
    combat.hand.push_back(card);
    bool played = combat.play_card(0, target);
    assert(played);
}

template <size_t Capacity>
int souls_in(const FixedVector<DeckCard, Capacity>& pile, bool upgraded_only = false) {
    int count = 0;
    for (const DeckCard& card : pile) count += is_soul(card.id) && (card.upgraded || !upgraded_only);
    return count;
}

int total_hp(const CombatEngine& combat) {
    int total = 0;
    for (const Enemy& foe : combat.enemies) total += foe.hp;
    return total;
}

// ---- Rules that hold with or without the card -------------------------------------------------------------

void test_a_combat_starts_without_souls() {
    CombatEngine combat = make_fight();
    assert(combat.player.souls == 0);
    assert(souls_in(combat.draw_pile) == 0 && souls_in(combat.hand) == 0 && souls_in(combat.discard_pile) == 0);
    assert(combat.exhausted_souls() == 0);
    assert(!is_soul(CardId::NONE) && !is_soul(CardId::STRIKE));
}

void test_adding_souls_makes_cards_or_counts_them() {
    CombatEngine combat = make_fight();
    const size_t before = combat.draw_pile.size();
    combat.add_souls(3);
    if (SOUL_EXISTS) {
        // "Add 3 Souls into your Draw Pile": three more cards, and nothing is counted on the side.
        assert(combat.draw_pile.size() == before + 3 && souls_in(combat.draw_pile) == 3 && combat.player.souls == 0);
    } else {
        // Stand-in: the piles are left alone, the Souls are counted, 99 at most.
        assert(combat.draw_pile.size() == before && combat.player.souls == 3);
        combat.add_souls(200, Pile::HAND);
        assert(combat.player.souls == 99 && combat.hand.empty());
    }
    combat.add_souls(0);
    combat.add_souls(-2);
    assert(combat.draw_pile.size() == before + (SOUL_EXISTS ? 3 : 0));
}

void test_other_cards_are_not_souls() {
    CombatEngine combat = make_fight();
    combat.add_power(PowerId::HAUNT, 7);
    combat.add_power(PowerId::DEVOUR_LIFE, 1);
    combat.player.energy = 3;
    play(combat, CardId::DEFEND);
    // Neither "whenever you play a Soul" power heard it.
    assert(combat.enemies[0].hp == 100 && !combat.player.minion_alive());
    // A card that is exhausted is not a Soul in the exhaust pile.
    combat.exhaust_card(CardId::DEFEND);
    assert(combat.exhaust_pile.size() == 1 && combat.exhausted_souls() == 0);
}

// ---- Rules of the Soul card -------------------------------------------------------------------------------

void test_soul_costs_nothing_draws_two_and_is_exhausted() {
    CombatEngine combat = make_fight();
    assert(CardDef::get(SOUL_CARD).type == CardType::SKILL);
    assert(CardDef::get_cost(SOUL_CARD, false) == 0 && CardDef::get_cost(SOUL_CARD, true) == 0);
    assert(CardDef::has_keyword(SOUL_CARD, false, KEYWORD_EXHAUST) && CardDef::has_keyword(SOUL_CARD, true, KEYWORD_EXHAUST));

    combat.player.energy = 0;
    const size_t deck = combat.draw_pile.size();
    play(combat, SOUL_CARD);
    assert(combat.hand.size() == 2 && combat.draw_pile.size() == deck - 2 && combat.player.energy == 0);
    assert(combat.exhaust_pile.size() == 1 && combat.discard_pile.empty());
    assert(combat.counters.skills_played == 1);
}

void test_upgraded_soul_draws_three() {
    CombatEngine combat = make_fight();
    const size_t deck = combat.draw_pile.size();
    play(combat, DeckCard(SOUL_CARD, true));
    assert(combat.hand.size() == 3 && combat.draw_pile.size() == deck - 3);
}

void test_souls_go_to_the_pile_they_are_added_to() {
    CombatEngine combat = make_fight();
    const size_t deck = combat.draw_pile.size();
    const uint8_t created = combat.counters.combat_cards_created;

    // Severance: one in the draw pile, one in the hand, one in the discard pile.
    combat.add_souls(1, Pile::DRAW);
    combat.add_souls(1, Pile::HAND);
    combat.add_souls(1, Pile::DISCARD);
    assert(combat.draw_pile.size() == deck + 1 && souls_in(combat.draw_pile) == 1);
    assert(combat.hand.size() == 1 && souls_in(combat.hand) == 1);
    assert(combat.discard_pile.size() == 1 && souls_in(combat.discard_pile) == 1);
    // They are cards made during the combat, like any other.
    assert(combat.counters.combat_cards_created == created + 3);

    // Reave+, Dirge+: upgraded Souls.
    combat.add_souls(2, Pile::DRAW, true);
    assert(souls_in(combat.draw_pile) == 3 && souls_in(combat.draw_pile, true) == 2);

    // A full hand: the Souls that do not fit go to the discard pile.
    while (combat.hand.size() < combat.hand.capacity()) combat.hand.push_back(CardId::DEFEND);
    combat.add_souls(2, Pile::HAND);
    assert(souls_in(combat.hand) == 1 && souls_in(combat.discard_pile) == 3);
}

void test_souls_in_the_draw_pile_are_drawn_and_played() {
    CombatEngine combat = make_fight();
    combat.draw_pile.clear();
    combat.add_souls(3);
    combat.draw_cards(3);
    assert(souls_in(combat.hand) == 3 && combat.draw_pile.empty());
    combat.player.energy = 0;
    // Nothing left to draw: the Soul is played all the same, and exhausted.
    assert(combat.can_play_card(0) && combat.play_card(0));
    assert(souls_in(combat.hand) == 2 && combat.exhausted_souls() == 1);
}

void test_exhausted_souls_are_counted() {
    CombatEngine combat = make_fight();
    play(combat, SOUL_CARD);
    play(combat, DeckCard(SOUL_CARD, true));
    // Soul Storm counts the Souls of the exhaust pile, upgraded or not, and no other card or pile.
    combat.exhaust_card(CardId::DEFEND);
    combat.add_souls(1, Pile::DISCARD);
    combat.add_souls(1, Pile::DRAW);
    assert(combat.exhausted_souls() == 2);
    // A Soul exhausted without being played is in the exhaust pile too.
    combat.exhaust_card(SOUL_CARD);
    assert(combat.exhausted_souls() == 3);
}

void test_haunt_makes_a_random_enemy_lose_hp() {
    CombatEngine combat = make_fight(3);
    for (Enemy& foe : combat.enemies) foe.block = 20;
    combat.add_power(PowerId::HAUNT, 7);
    play(combat, SOUL_CARD);
    // One enemy lost 7 HP: an HP loss, its Block is untouched.
    assert(total_hp(combat) == 293);
    for (const Enemy& foe : combat.enemies) assert(foe.block == 20 && (foe.hp == 100 || foe.hp == 93));
    // Once per Soul played.
    play(combat, SOUL_CARD);
    assert(total_hp(combat) == 286);
    // Haunt+ is 9; a second Haunt adds up.
    combat.add_power(PowerId::HAUNT, 9);
    play(combat, SOUL_CARD);
    assert(total_hp(combat) == 270);

    // The HP loss can kill, and the combat is won.
    CombatEngine last = make_fight();
    last.enemies[0].hp = 7;
    last.add_power(PowerId::HAUNT, 7);
    play(last, SOUL_CARD);
    assert(!last.enemies[0].is_alive() && last.state == CombatState::VICTORY);
}

void test_devour_life_summons_when_a_soul_is_played() {
    CombatEngine combat = make_fight();
    combat.add_power(PowerId::DEVOUR_LIFE, 1);
    play(combat, SOUL_CARD);
    assert(combat.player.minion_alive() && combat.player.minion_hp == 1 && combat.player.minion_max_hp == 1);
    play(combat, SOUL_CARD);
    assert(combat.player.minion_hp == 2 && combat.player.minion_max_hp == 2);
    // Devour Life+ summons 2.
    CombatEngine upgraded = make_fight();
    upgraded.add_power(PowerId::DEVOUR_LIFE, 2);
    play(upgraded, DeckCard(SOUL_CARD, true));
    assert(upgraded.player.minion_hp == 2);
}

void test_funerary_mask_adds_three_souls_to_the_draw_pile() {
    RunManager run;
    run.start_run(CharacterId::NECROBINDER, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("FUNERARY_MASK");
    const size_t deck = run.master_deck.size();
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    // Shuffled in before the first hand: the three Souls are in the draw pile or already drawn.
    assert(souls_in(run.combat.draw_pile) + souls_in(run.combat.hand) == 3);
    assert(run.combat.draw_pile.size() + run.combat.hand.size() == deck + 3);
    // Each combat, and the deck of the run does not keep them.
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    assert(souls_in(run.combat.draw_pile) + souls_in(run.combat.hand) == 3 && run.master_deck.size() == deck);
    bool drops = false;
    for (std::string_view id : RunManager::active_relics()) drops = drops || id == "FUNERARY_MASK";
    assert(drops);
}

void test_pot_of_ghouls_adds_two_souls_to_the_hand() {
    CombatEngine combat = make_fight();
    assert(combat.use_potion("POT_OF_GHOULS"));
    assert(combat.hand.size() == 2 && souls_in(combat.hand) == 2 && souls_in(combat.hand, true) == 0);
}

} // namespace

int main() {
    test_a_combat_starts_without_souls();
    test_adding_souls_makes_cards_or_counts_them();
    test_other_cards_are_not_souls();
    std::cout << "[PASS] Souls: none at the start, add_souls, other cards are not Souls" << std::endl;

    if (!SOUL_EXISTS) {
        std::cout << "[SKIP] Souls: 7 cases need the Soul card (engine::SOUL_CARD is CardId::NONE)" << std::endl;
        return 0;
    }
    test_soul_costs_nothing_draws_two_and_is_exhausted();
    test_upgraded_soul_draws_three();
    test_souls_go_to_the_pile_they_are_added_to();
    test_souls_in_the_draw_pile_are_drawn_and_played();
    test_exhausted_souls_are_counted();
    test_haunt_makes_a_random_enemy_lose_hp();
    test_devour_life_summons_when_a_soul_is_played();
    test_funerary_mask_adds_three_souls_to_the_draw_pile();
    test_pot_of_ghouls_adds_two_souls_to_the_hand();
    std::cout << "[PASS] Souls: the card, the three piles, the exhaust count, Haunt and Devour Life" << std::endl;
    return 0;
}
