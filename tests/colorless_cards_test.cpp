// Colorless cards, and the choice of 1 card among N offered (Discovery mechanic, #72): Seeker Strike offers
// cards of the draw pile, offer_random_cards offers cards made for the offer.
#include <algorithm>
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

constexpr int JAW_WORM = 2; // the third enemy of make_pack, 42 HP

// A fight against three enemies: two Lice (18 HP) and a Jaw Worm (42 HP).
CombatEngine make_pack() {
    CombatEngine combat;
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::JAW_WORM);
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    return combat;
}

// Six different cards in the draw pile, nothing in hand nor in the discard pile.
void fill_draw_pile(CombatEngine& combat) {
    combat.hand.clear();
    combat.discard_pile.clear();
    combat.draw_pile.clear();
    for (CardId id : { CardId::STRIKE, CardId::DEFEND, CardId::BASH, CardId::ANGER,
                       CardId::CLEAVE, CardId::ARMAMENTS }) {
        combat.draw_pile.push_back(DeckCard(id));
    }
}

int count_in(const auto& pile, CardId id) {
    int count = 0;
    for (const DeckCard& card : pile) count += card.id == id;
    return count;
}

void test_seeker_strike() {
    const CardDef& def = CardDef::get(CardId::SEEKER_STRIKE);
    assert(def.type == CardType::ATTACK);
    assert(card_meta(CardId::SEEKER_STRIKE).rarity == CardRarity::UNCOMMON);
    assert(card_meta(CardId::SEEKER_STRIKE).pool == CardPool::COLORLESS);
    assert(CombatEngine::opens_choice(DeckCard(CardId::SEEKER_STRIKE)));

    for (bool upgraded : { false, true }) {
        CombatEngine combat = make_pack();
        fill_draw_pile(combat);
        combat.hand.push_back(DeckCard(CardId::SEEKER_STRIKE, upgraded));
        combat.player.energy = 3;
        const int hp = combat.enemies[JAW_WORM].hp;
        assert(combat.play_card(0, JAW_WORM));

        // Cost 1, 9 damage (12 upgraded).
        assert(combat.player.energy == 2);
        assert(combat.enemies[JAW_WORM].hp == hp - (upgraded ? 12 : 9));

        // 3 different cards of the draw pile are offered; the draw pile is untouched until one is picked.
        assert(combat.choice_pending());
        assert(combat.pending_choices[0].source == Pile::OFFERS);
        assert(combat.pending_choices[0].kind == ChoiceKind::TO_HAND);
        assert(combat.offers.size() == 3 && combat.choice_pool_size() == 3 && combat.choice_size() == 1);
        assert(combat.draw_pile.size() == 6 && combat.hand.empty());
        for (size_t i = 0; i < combat.offers.size(); ++i) {
            assert(combat.offer_origins[i] < combat.draw_pile.size());
            assert(combat.draw_pile[combat.offer_origins[i]] == combat.offers[i]);
            for (size_t j = 0; j < i; ++j) assert(combat.offer_origins[j] != combat.offer_origins[i]);
        }

        // The picked card goes from the draw pile to the hand; the 2 others stay in the draw pile.
        const DeckCard picked = combat.offers[1];
        const DeckCard left_a = combat.offers[0];
        const DeckCard left_b = combat.offers[2];
        const uint8_t picks[] = { 1 };
        assert(combat.resolve_choice(picks));
        assert(!combat.choice_pending() && combat.offers.empty() && combat.offer_origins.empty());
        assert(combat.hand.size() == 1 && combat.hand[0] == picked);
        assert(combat.draw_pile.size() == 5);
        assert(count_in(combat.draw_pile, picked.id) == 0);
        assert(count_in(combat.draw_pile, left_a.id) == 1);
        assert(count_in(combat.draw_pile, left_b.id) == 1);
        assert(count_in(combat.discard_pile, CardId::SEEKER_STRIKE) == 1);
    }
}

// Fewer cards in the draw pile than offered: all of them are offered; a single one goes to the hand at once,
// none offers nothing.
void test_offer_from_short_draw_pile() {
    CombatEngine two = make_pack();
    fill_draw_pile(two);
    two.draw_pile.erase(0);
    two.draw_pile.erase(0);
    two.draw_pile.erase(0);
    two.draw_pile.erase(0);
    two.offer_from_draw(3);
    assert(two.choice_pending() && two.offers.size() == 2 && two.choice_size() == 1);

    CombatEngine one = make_pack();
    fill_draw_pile(one);
    while (one.draw_pile.size() > 1) one.draw_pile.erase(0);
    one.offer_from_draw(3);
    assert(!one.choice_pending() && one.offers.empty());
    assert(one.hand.size() == 1 && one.hand[0] == CardId::ARMAMENTS && one.draw_pile.empty());

    CombatEngine none = make_pack();
    fill_draw_pile(none);
    none.draw_pile.clear();
    none.offer_from_draw(3);
    assert(!none.choice_pending() && none.offers.empty() && none.hand.empty());
}

// With a full hand, the picked card of the draw pile stays there.
void test_offer_from_draw_with_full_hand() {
    CombatEngine combat = make_pack();
    fill_draw_pile(combat);
    while (combat.hand.size() < combat.hand.capacity()) combat.hand.push_back(DeckCard(CardId::STRIKE));
    combat.offer_from_draw(3);
    assert(combat.choice_pending());
    const uint8_t picks[] = { 0 };
    assert(combat.resolve_choice(picks));
    assert(combat.draw_pile.size() == 6 && combat.hand.size() == combat.hand.capacity());
    assert(combat.offers.empty());
}

// Two offers asked in a row: the second is dealt once the first is answered, from the draw pile as it is then.
void test_offers_in_a_row() {
    CombatEngine combat = make_pack();
    fill_draw_pile(combat);
    combat.offer_from_draw(3);
    combat.offer_from_draw(3);
    assert(combat.pending_choices.size() == 2 && combat.offers.size() == 3);
    const DeckCard first = combat.offers[2];
    const uint8_t picks[] = { 2 };
    assert(combat.resolve_choice(picks));
    assert(combat.choice_pending() && combat.offers.size() == 3 && combat.draw_pile.size() == 5);
    for (const DeckCard& offered : combat.offers) assert(offered.id != first.id);
    combat.resolve_choice_default();
    assert(!combat.choice_pending() && combat.offers.empty());
    assert(combat.hand.size() == 2 && combat.hand[0] == first && combat.draw_pile.size() == 4);
}

// Random cards made for the offer: different, of the filter, upgraded and free when asked. The picked one goes
// to the hand (to the discard pile when the hand is full), the others vanish.
void test_offer_random_cards() {
    for (bool full_hand : { false, true }) {
        CombatEngine combat = make_pack();
        fill_draw_pile(combat);
        if (full_hand) {
            while (combat.hand.size() < combat.hand.capacity()) combat.hand.push_back(DeckCard(CardId::STRIKE));
        }
        const size_t hand_before = combat.hand.size();
        combat.offer_random_cards(CardFilter(CardType::ATTACK), 3, true, true);
        assert(combat.choice_pending() && combat.pending_choices[0].source == Pile::OFFERS);
        assert(combat.offers.size() == 3 && combat.choice_size() == 1);
        for (size_t i = 0; i < combat.offers.size(); ++i) {
            const DeckCard& offered = combat.offers[i];
            assert(CardDef::get(offered.id).type == CardType::ATTACK);
            assert(offered.upgraded && offered.free_this_turn);
            assert(combat.offer_origins[i] == NOT_IN_PILE);
            for (size_t j = 0; j < i; ++j) assert(combat.offers[j].id != offered.id);
        }
        const DeckCard picked = combat.offers[0];
        const uint8_t picks[] = { 0 };
        assert(combat.resolve_choice(picks));
        assert(!combat.choice_pending() && combat.offers.empty());
        assert(combat.draw_pile.size() == 6);
        if (full_hand) {
            assert(combat.hand.size() == hand_before);
            assert(combat.discard_pile.size() == 1 && combat.discard_pile[0] == picked);
        } else {
            assert(combat.hand.size() == 1 && combat.hand[0] == picked && combat.hand[0].free_this_turn);
            assert(combat.discard_pile.empty());
        }
    }
}

// A new combat forgets an offer left unanswered.
void test_offers_cleared_with_the_combat() {
    CombatEngine combat = make_pack();
    fill_draw_pile(combat);
    combat.offer_from_draw(3);
    assert(!combat.offers.empty());
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::JAW_WORM);
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    assert(!combat.choice_pending() && combat.offers.empty() && combat.offer_origins.empty());
}

// The Gambit: 0 cost Skill, Rare, 50 Block (75 upgraded); unblocked attack damage this combat is fatal.
void test_the_gambit() {
    const CardDef& def = CardDef::get(CardId::THE_GAMBIT);
    assert(def.type == CardType::SKILL);
    assert(card_meta(CardId::THE_GAMBIT).rarity == CardRarity::RARE);
    assert(card_meta(CardId::THE_GAMBIT).pool == CardPool::COLORLESS);

    for (bool upgraded : { false, true }) {
        assert(CardDef::get_cost(CardId::THE_GAMBIT, upgraded) == 0);
        CombatEngine combat = make_pack();
        combat.hand.clear();
        combat.player.energy = 3;
        combat.player.block = 0;
        combat.hand.push_back(DeckCard(CardId::THE_GAMBIT, upgraded));
        assert(combat.play_card(0, 0));

        // Block: 50 (75 upgraded). Still alive: the enemies' attacks this turn are blocked.
        assert(combat.player.block == (upgraded ? 75 : 50));
        assert(combat.gambit_active);
        combat.end_player_turn();
        assert(combat.player.is_alive() && combat.state != CombatState::DEFEAT);

        // Without Block, the next unblocked attack damage ends the run.
        combat.player.block = 0;
        combat.end_player_turn();
        assert(combat.state == CombatState::DEFEAT || !combat.player.is_alive());
    }
    std::cout << "[PASS] The Gambit: 0 cost Skill, 50/75 Block, unblocked attack damage kills\n";
}

// Discovery: 1 cost Skill, Uncommon; choose 1 of 3 random cards to add to the hand, free this turn.
// Exhaust, which the upgrade removes.
void test_discovery() {
    const CardDef& def = CardDef::get(CardId::DISCOVERY);
    assert(def.type == CardType::SKILL);
    assert(card_meta(CardId::DISCOVERY).rarity == CardRarity::UNCOMMON);
    assert(card_meta(CardId::DISCOVERY).pool == CardPool::COLORLESS);
    assert(CardDef::has_keyword(CardId::DISCOVERY, false, KEYWORD_EXHAUST));
    assert(!CardDef::has_keyword(CardId::DISCOVERY, true, KEYWORD_EXHAUST));
    assert(CombatEngine::opens_choice(DeckCard(CardId::DISCOVERY)));

    for (bool upgraded : { false, true }) {
        assert(CardDef::get_cost(CardId::DISCOVERY, upgraded) == 1);
        CombatEngine combat = make_pack();
        fill_draw_pile(combat);
        combat.hand.push_back(DeckCard(CardId::DISCOVERY, upgraded));
        combat.player.energy = 3;
        assert(combat.play_card(0, -1));
        assert(combat.player.energy == 2);

        assert(combat.choice_pending());
        assert(combat.pending_choices[0].source == Pile::OFFERS);
        assert(combat.offers.size() == 3 && combat.choice_size() == 1);
        for (size_t i = 0; i < combat.offers.size(); ++i) {
            assert(combat.offers[i].free_this_turn && !combat.offers[i].upgraded);
            for (size_t j = 0; j < i; ++j) assert(combat.offers[j].id != combat.offers[i].id);
        }
        assert(combat.draw_pile.size() == 6);

        const DeckCard picked = combat.offers[2];
        const uint8_t picks[] = { 2 };
        assert(combat.resolve_choice(picks));
        assert(!combat.choice_pending() && combat.offers.empty());
        assert(combat.hand.size() == 1 && combat.hand[0] == picked && combat.hand[0].free_this_turn);
        assert(combat.draw_pile.size() == 6);

        // Base version is exhausted, upgraded goes to the discard pile.
        assert(count_in(combat.exhaust_pile, CardId::DISCOVERY) == (upgraded ? 0 : 1));
        assert(count_in(combat.discard_pile, CardId::DISCOVERY) == (upgraded ? 1 : 0));
    }
    std::cout << "[PASS] Discovery: 1 of 3 random cards, free this turn, Exhaust (not upgraded)\n";
}

} // namespace

int main() {
    test_seeker_strike();
    test_offer_from_short_draw_pile();
    test_offer_from_draw_with_full_hand();
    test_offers_in_a_row();
    test_offer_random_cards();
    test_offers_cleared_with_the_combat();
    test_the_gambit();
    test_discovery();
    std::cout << "colorless_cards_test: all tests passed\n";
    return 0;
}
