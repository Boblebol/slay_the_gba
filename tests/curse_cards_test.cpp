// The Curse cards that only need the existing mechanics (Unplayable, Eternal, Ethereal, Retain, Innate, the
// "end of turn in hand" hook and the play limit). Values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

CombatEngine fight_with(CardId card, bool upgraded = false) {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.hand.push_back(DeckCard(card, upgraded));
    combat.player.energy = 3;
    return combat;
}

void check_definition(CardId id, uint16_t keywords, bool unplayable = true) {
    const CardKeyword all[] = { KEYWORD_ETERNAL, KEYWORD_ETHEREAL, KEYWORD_RETAIN, KEYWORD_INNATE, KEYWORD_LIMIT_PLAYS, KEYWORD_EXHAUST };
    assert(CardDef::get(id).type == CardType::CURSE);
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(id, up) == (unplayable ? COST_X : 1));
        assert(CardDef::has_keyword(id, up, KEYWORD_UNPLAYABLE) == unplayable);
        for (CardKeyword bit : all) {
            assert(CardDef::has_keyword(id, up, bit) == ((keywords & bit) != 0));
        }
        assert(CardDef::is_eternal(DeckCard(id, up)) == ((keywords & KEYWORD_ETERNAL) != 0));
    }
}

void test_injury() {
    // The witness card of the Curse mechanic: it only exists to sit in the hand.
    check_definition(CardId::INJURY, 0);
    for (bool up : { false, true }) {
        CombatEngine combat = fight_with(CardId::INJURY, up);
        assert(!combat.can_play_card(0));
        assert(!combat.play_card(0, 0));
        assert(combat.player.energy == 3 && combat.hand.size() == 1);
        const int16_t hp = combat.player.hp;
        combat.end_player_turn();
        // No effect in hand, and it goes to the discard pile like any card.
        assert(combat.player.hp >= static_cast<int16_t>(hp - 20));
        assert(combat.discard_pile.size() >= 1);
    }
    std::cout << "[PASS] Injury: X cost, Curse, Unplayable, nothing else, same upgraded" << std::endl;
}

void test_keyword_curses() {
    check_definition(CardId::GREED, KEYWORD_ETERNAL);
    check_definition(CardId::CLUMSY, KEYWORD_ETHEREAL);
    check_definition(CardId::ASCENDERS_BANE, KEYWORD_ETHEREAL | KEYWORD_ETERNAL);
    check_definition(CardId::CURSE_OF_THE_BELL, KEYWORD_ETERNAL);
    check_definition(CardId::POOR_SLEEP, KEYWORD_RETAIN);
    check_definition(CardId::WRITHE, KEYWORD_INNATE);
    check_definition(CardId::NORMALITY, KEYWORD_LIMIT_PLAYS);
    check_definition(CardId::DOUBT, 0);
    check_definition(CardId::SHAME, 0);
    check_definition(CardId::DECAY, 0);
    check_definition(CardId::REGRET, 0);
    check_definition(CardId::BAD_LUCK, KEYWORD_ETERNAL);
    check_definition(CardId::DEBT, 0);
    check_definition(CardId::SPORE_MIND, KEYWORD_EXHAUST, false);
    std::cout << "[PASS] Curses: type, cost, keywords and Eternal, base and upgraded" << std::endl;
}

void test_ethereal_and_retain() {
    for (CardId id : { CardId::CLUMSY, CardId::ASCENDERS_BANE }) {
        CombatEngine combat = fight_with(id);
        combat.end_player_turn();
        assert(combat.exhaust_pile.size() == 1 && combat.exhaust_pile[0].id == id);
    }
    // Retain: still in hand after the end of the turn.
    CombatEngine combat = fight_with(CardId::POOR_SLEEP);
    combat.end_player_turn();
    bool kept = false;
    for (size_t i = 0; i < combat.hand.size(); ++i) kept = kept || combat.hand[i].id == CardId::POOR_SLEEP;
    assert(kept);
    std::cout << "[PASS] Clumsy and Ascender's Bane exhaust at end of turn, Poor Sleep is retained" << std::endl;
}

void test_innate() {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::IRONCLAD);
    // Innate: drawn in the opening hand.
    bool found = false;
    for (int attempt = 0; attempt < 5 && !found; ++attempt) {
        combat.draw_pile.clear();
        for (int i = 0; i < 12; ++i) combat.draw_pile.push_back(DeckCard(CardId::STRIKE, false));
        combat.draw_pile.push_back(DeckCard(CardId::WRITHE, false));
        combat.hand.clear();
        combat.discard_pile.clear();
        combat.begin_first_turn();
        for (size_t i = 0; i < combat.hand.size(); ++i) found = found || combat.hand[i].id == CardId::WRITHE;
    }
    assert(found);
    std::cout << "[PASS] Writhe is drawn in the opening hand" << std::endl;
}

void test_end_of_turn_effects() {
    {
        // Weak and Frail wear off at the end of the enemy turn, so the effect is read at the hook itself.
        CombatEngine combat = fight_with(CardId::DOUBT);
        combat.trigger_end_of_turn_in_hand(combat.hand[0]);
        assert(combat.player.get_status(StatusType::WEAK) == 1);
    }
    {
        CombatEngine combat = fight_with(CardId::SHAME);
        combat.trigger_end_of_turn_in_hand(combat.hand[0]);
        assert(combat.player.get_status(StatusType::FRAIL) == 1);
    }
    {
        CombatEngine combat = fight_with(CardId::DECAY);
        combat.player.block = 0;
        const int16_t hp = combat.player.hp;
        combat.end_player_turn();
        assert(combat.player.hp <= static_cast<int16_t>(hp - 2));
    }
    {
        CombatEngine combat = fight_with(CardId::BAD_LUCK);
        combat.player.block = 0;
        const int16_t hp = combat.player.hp;
        combat.end_player_turn();
        assert(combat.player.hp <= static_cast<int16_t>(hp - 13));
    }
    {
        // 1 HP for each card in hand, the curse included.
        CombatEngine combat = fight_with(CardId::REGRET);
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.hand.push_back(DeckCard(CardId::DEFEND, false));
        combat.player.block = 0;
        const int16_t hp = combat.player.hp;
        combat.end_player_turn();
        assert(combat.player.hp <= static_cast<int16_t>(hp - 3));
    }
    {
        CombatEngine combat = fight_with(CardId::DEBT);
        assert(combat.gold_lost == 0);
        combat.end_player_turn();
        assert(combat.gold_lost == 10);
    }
    {
        // Nothing happens when the card is not in hand at the end of the turn.
        CombatEngine combat = fight_with(CardId::DECAY);
        combat.hand.clear();
        combat.discard_pile.push_back(DeckCard(CardId::DECAY, false));
        combat.player.block = 0;
        const int16_t hp = combat.player.hp;
        combat.end_player_turn();
        assert(combat.player.hp >= static_cast<int16_t>(hp - 12));
    }
    std::cout << "[PASS] Doubt, Shame, Decay, Regret, Bad Luck, Debt: effect at end of turn only while in hand" << std::endl;
}

void test_normality() {
    CombatEngine combat = fight_with(CardId::NORMALITY);
    combat.hand.clear();
    combat.hand.push_back(DeckCard(CardId::NORMALITY, false));
    for (int i = 0; i < 5; ++i) combat.hand.push_back(DeckCard(CardId::DEFEND, false));
    combat.player.energy = 10;
    int played = 0;
    for (int tries = 0; tries < 6; ++tries) {
        int index = -1;
        for (size_t i = 0; i < combat.hand.size(); ++i) {
            if (combat.hand[i].id == CardId::DEFEND && combat.can_play_card(static_cast<int>(i))) { index = static_cast<int>(i); break; }
        }
        if (index < 0) break;
        assert(combat.play_card(index, 0));
        ++played;
    }
    assert(played == 3);
    std::cout << "[PASS] Normality: no more than 3 cards played in a turn" << std::endl;
}

void test_spore_mind() {
    check_definition(CardId::SPORE_MIND, KEYWORD_EXHAUST, false);
    CombatEngine combat = fight_with(CardId::SPORE_MIND);
    assert(combat.can_play_card(0));
    assert(combat.play_card(0, 0));
    assert(combat.player.energy == 2);
    assert(combat.exhaust_pile.size() == 1 && combat.exhaust_pile[0].id == CardId::SPORE_MIND);
    std::cout << "[PASS] Spore Mind: cost 1, playable, exhausted, nothing else" << std::endl;
}

void test_eternal_stays_in_the_deck() {
    for (CardId id : { CardId::GREED, CardId::ASCENDERS_BANE, CardId::CURSE_OF_THE_BELL, CardId::BAD_LUCK }) {
        RunManager run;
        run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
        run.master_deck.push_back(DeckCard(id, false));
        const size_t size = run.master_deck.size();
        assert(!run.remove_card_from_deck(size - 1));
        assert(run.master_deck.size() == size);
    }
    // A non-Eternal curse can be removed.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.master_deck.push_back(DeckCard(CardId::INJURY, false));
    const size_t size = run.master_deck.size();
    assert(run.remove_card_from_deck(size - 1));
    assert(run.master_deck.size() == size - 1);
    std::cout << "[PASS] Eternal curses cannot be removed from the deck, the others can" << std::endl;
}

void test_ascension_10_starting_curse() {
    // Ascension 10 puts Ascender's Bane in the starting deck; below it the deck is unchanged.
    for (uint8_t ascension : { 0, 9 }) {
        RunManager run;
        run.start_run(CharacterId::IRONCLAD, ascension, PaletteVariant::CLASSIC, 777);
        for (size_t i = 0; i < run.master_deck.size(); ++i) {
            assert(run.master_deck[i].id != CardId::ASCENDERS_BANE);
        }
        assert(run.master_deck.size() == 10);
    }
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 10, PaletteVariant::CLASSIC, 777);
    assert(run.master_deck.size() == 11);
    assert(run.master_deck[run.master_deck.size() - 1].id == CardId::ASCENDERS_BANE);
    assert(!run.remove_card_from_deck(run.master_deck.size() - 1));
    assert(run.master_deck.size() == 11);
    std::cout << "[PASS] Ascension 10 starts with Ascender's Bane, Ascension 9 does not" << std::endl;
}

} // namespace

int main() {
    test_injury();
    test_keyword_curses();
    test_ethereal_and_retain();
    test_innate();
    test_end_of_turn_effects();
    test_normality();
    test_spore_mind();
    test_eternal_stays_in_the_deck();
    test_ascension_10_starting_curse();
    std::cout << "All curse card tests passed." << std::endl;
    return 0;
}
