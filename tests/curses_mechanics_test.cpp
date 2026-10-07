// Curse and Quest mechanics: card types, the Eternal keyword, end-of-turn effects of cards left in hand.
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

void test_card_types_and_keyword_bits() {
    // Curse and Quest are distinct card types, after Status.
    static_assert(CardType::CURSE != CardType::STATUS && CardType::QUEST != CardType::CURSE);
    // Eternal takes a free bit of the 16-bit keyword mask.
    static_assert((KEYWORD_ETERNAL & (KEYWORD_EXHAUST | KEYWORD_RETAIN | KEYWORD_ETHEREAL | KEYWORD_INNATE |
                                      KEYWORD_RETURN | KEYWORD_RETAIN_ONCE | KEYWORD_PLAY_FIRST |
                                      KEYWORD_LIMIT_PLAYS | KEYWORD_UNPLAYABLE)) == 0);
    static_assert(KEYWORD_ETERNAL <= 0xFFFF);
    assert(!CardDef::is_eternal(DeckCard(CardId::STRIKE)));
    std::cout << "[PASS] Curse/Quest types and the Eternal keyword bit" << std::endl;
}

void test_non_eternal_cards_still_leave_the_deck() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 7);
    size_t size = run.master_deck.size();
    assert(run.remove_card_from_deck(0));
    assert(run.master_deck.size() == size - 1);
    assert(run.transform_card_in_deck(0));
    run.request_deck_choice(DeckChoiceKind::REMOVE, 1);
    assert(run.deck_choice_allows(0));
    std::cout << "[PASS] Non-Eternal cards can still be removed and transformed" << std::endl;
}

void test_burn_goes_through_end_of_turn_hook() {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    int16_t hp = combat.player.hp;
    combat.player.block = 0;
    combat.trigger_end_of_turn_in_hand(DeckCard(CardId::BURN));
    assert(combat.player.hp == hp - 2);
    // A card without an end-of-turn effect does nothing.
    combat.trigger_end_of_turn_in_hand(DeckCard(CardId::STRIKE));
    assert(combat.player.hp == hp - 2);
    std::cout << "[PASS] End-of-turn in-hand hook (Burn)" << std::endl;
}

void test_gold_lost_in_combat_is_paid_by_the_run() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 7);
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    run.gold = 50;
    run.combat.lose_gold(15);
    run.combat.lose_gold(0);
    run.proceed_after_combat();
    assert(run.combat.gold_lost == 0);
    assert(run.gold == 35);

    // Gold never goes below zero.
    run.map.nodes[1].type = RoomType::MONSTER;
    run.enter_room(1);
    run.gold = 10;
    run.combat.lose_gold(40);
    run.proceed_after_combat();
    assert(run.gold == 0);
    std::cout << "[PASS] Gold lost in combat is taken from the run" << std::endl;
}

} // namespace

int main() {
    test_card_types_and_keyword_bits();
    test_non_eternal_cards_still_leave_the_deck();
    test_burn_goes_through_end_of_turn_hook();
    test_gold_lost_in_combat_is_paid_by_the_run();
    return 0;
}
