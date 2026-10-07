// Enthralled (Assuétude), a Curse: cost 2, must be played before the other cards of the hand, Eternal. Same upgraded.
// Values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

void test_enthralled_definition() {
    assert(CardDef::get(CardId::ENTHRALLED).type == CardType::CURSE);
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::ENTHRALLED, up) == 2);
        assert(CardDef::has_keyword(CardId::ENTHRALLED, up, KEYWORD_PLAY_FIRST));
        assert(CardDef::has_keyword(CardId::ENTHRALLED, up, KEYWORD_ETERNAL));
        assert(!CardDef::has_keyword(CardId::ENTHRALLED, up, KEYWORD_UNPLAYABLE));
        assert(CardDef::is_eternal(DeckCard(CardId::ENTHRALLED, up)));
    }
    std::cout << "[PASS] Enthralled: cost 2, Curse, play first, Eternal, same upgraded" << std::endl;
}

void test_enthralled_must_be_played_first() {
    for (bool up : { false, true }) {
        CombatEngine combat;
        combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
        combat.hand.clear();
        combat.hand.push_back(DeckCard(CardId::STRIKE));
        combat.hand.push_back(DeckCard(CardId::ENTHRALLED, up));
        combat.player.energy = 3;
        assert(combat.play_blocker(0) == PlayBlocker::PLAY_FIRST);
        assert(!combat.can_play_card(0));
        assert(combat.can_play_card(1));
        assert(combat.play_card(1, 0));
        assert(combat.player.energy == 1);
        // Once it is gone, the other cards are free again.
        assert(combat.hand.size() == 1 && combat.hand[0].id == CardId::STRIKE);
        assert(combat.can_play_card(0));
    }
    std::cout << "[PASS] Enthralled blocks the other cards until it is played" << std::endl;
}

void test_enthralled_needs_energy() {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.hand.push_back(DeckCard(CardId::ENTHRALLED));
    combat.player.energy = 1;
    assert(!combat.can_play_card(0));
    std::cout << "[PASS] Enthralled costs 2 energy" << std::endl;
}

void test_enthralled_is_eternal() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 7);
    assert(run.add_card_to_deck(DeckCard(CardId::ENTHRALLED)));
    const int index = static_cast<int>(run.master_deck.size()) - 1;
    assert(!run.remove_card_from_deck(index));
    assert(!run.transform_card_in_deck(index));
    run.request_deck_choice(DeckChoiceKind::REMOVE, 1);
    assert(!run.deck_choice_allows(index));
    assert(run.master_deck[index].id == CardId::ENTHRALLED);
    std::cout << "[PASS] Enthralled cannot be removed or transformed" << std::endl;
}

} // namespace

int main() {
    test_enthralled_definition();
    test_enthralled_must_be_played_first();
    test_enthralled_needs_energy();
    test_enthralled_is_eternal();
    return 0;
}
