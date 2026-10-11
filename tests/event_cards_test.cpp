// Event cards (offered by events). One test per card, with the values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

// An Ironclad against one Jaw Worm, empty piles and 3 energy.
CombatEngine make_fight() {
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 3;
    return combat;
}

void test_apparition() {
    assert(CardDef::get_cost(CardId::APPARITION, false) == 1);
    assert(CardDef::get_cost(CardId::APPARITION, true) == 1);
    assert(CardDef::get(CardId::APPARITION).type == CardType::SKILL);
    assert(CardDef::has_keyword(CardId::APPARITION, false, KEYWORD_ETHEREAL));
    assert(CardDef::has_keyword(CardId::APPARITION, false, KEYWORD_EXHAUST));

    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::APPARITION));
    assert(combat.play_card(0, 0));
    assert(combat.player.get_status(StatusType::INTANGIBLE) == 1);
    assert(combat.player.energy == 2);
    assert(combat.hand.empty() && combat.exhaust_pile.size() == 1);

    // Upgraded: still Intangible 1 and Exhaust, but no longer Ethereal.
    assert(!CardDef::has_keyword(CardId::APPARITION, true, KEYWORD_ETHEREAL));
    assert(CardDef::has_keyword(CardId::APPARITION, true, KEYWORD_EXHAUST));
    CombatEngine upgraded = make_fight();
    upgraded.hand.push_back(DeckCard(CardId::APPARITION, true));
    assert(upgraded.play_card(0, 0));
    assert(upgraded.player.get_status(StatusType::INTANGIBLE) == 1);
    assert(upgraded.player.energy == 2);
    assert(upgraded.hand.empty() && upgraded.exhaust_pile.size() == 1);
    std::cout << "[PASS] Apparition: 1 energy, Intangible 1, Ethereal (base only), Exhaust" << std::endl;
}

// A fight against a Jaw Worm with 200 HP, so that the damage can be read from its HP.
CombatEngine make_big_fight() {
    CombatEngine combat = make_fight();
    combat.enemies[0].max_hp = 200;
    combat.enemies[0].hp = 200;
    combat.enemies[0].block = 0;
    return combat;
}

void test_maul() {
    assert(CardDef::get_cost(CardId::MAUL, false) == 1);
    assert(CardDef::get(CardId::MAUL).type == CardType::ATTACK);
    // Base: 5 damage twice, then every Maul gains 2.
    CombatEngine combat = make_big_fight();
    combat.hand.push_back(DeckCard(CardId::MAUL));
    combat.hand.push_back(DeckCard(CardId::MAUL));
    combat.draw_pile.push_back(DeckCard(CardId::MAUL));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 190);
    assert(combat.player.energy == 2);
    assert(combat.hand[0].bonus == 2 && combat.draw_pile[0].bonus == 2);
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 190 - 14);
    assert(combat.draw_pile[0].bonus == 4);
    // Upgraded: 6 damage twice, then every Maul gains 3.
    CombatEngine up = make_big_fight();
    up.hand.push_back(DeckCard(CardId::MAUL, true));
    up.hand.push_back(DeckCard(CardId::MAUL, true));
    assert(CardDef::get_cost(CardId::MAUL, true) == 1);
    assert(up.play_card(0, 0));
    assert(up.enemies[0].hp == 188);
    assert(up.hand[0].bonus == 3);
    assert(up.play_card(0, 0));
    assert(up.enemies[0].hp == 188 - 18);
    std::cout << "[PASS] Maul: 5/6 damage twice, all Maul +2/+3 for the combat" << std::endl;
}

void check_apotheosis(bool upgraded) {
    CombatEngine combat = make_fight();
    combat.exhaust_pile.clear();
    assert(CardDef::get_cost(CardId::APOTHEOSIS, upgraded) == (upgraded ? 1 : 2));
    assert(CardDef::get(CardId::APOTHEOSIS).type == CardType::SKILL);
    assert(CardDef::get(CardId::APOTHEOSIS).keywords & KEYWORD_INNATE);
    assert(CardDef::get(CardId::APOTHEOSIS).keywords & KEYWORD_EXHAUST);
    combat.hand.push_back(DeckCard(CardId::APOTHEOSIS, upgraded));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    combat.discard_pile.push_back(DeckCard(CardId::BASH));
    combat.exhaust_pile.push_back(DeckCard(CardId::STRIKE));
    assert(combat.play_card(0, 0));
    assert(combat.player.energy == (upgraded ? 2 : 1));
    assert(combat.exhaust_pile.size() == 2);
    assert(combat.hand.size() == 1 && combat.hand[0].upgraded);
    assert(combat.draw_pile[0].upgraded);
    assert(combat.discard_pile[0].upgraded);
    assert(combat.exhaust_pile[0].upgraded);
}

void test_apotheosis() {
    check_apotheosis(false);
    check_apotheosis(true);
    std::cout << "[PASS] Apotheosis: 2 energy (1 upgraded), Innate, upgrades all cards, Exhaust" << std::endl;
}

void test_mad_science() {
    assert(CardDef::get_cost(CardId::MAD_SCIENCE, false) == 1);
    assert(CardDef::get_cost(CardId::MAD_SCIENCE, true) == 1);
    assert(CardDef::get(CardId::MAD_SCIENCE).type == CardType::ATTACK);
    assert(!CardDef::has_keyword(CardId::MAD_SCIENCE, false, KEYWORD_INNATE));
    assert(CardDef::has_keyword(CardId::MAD_SCIENCE, true, KEYWORD_INNATE));
    // Base: 1 energy, 12 damage, then the card goes to the discard pile.
    CombatEngine combat = make_big_fight();
    combat.hand.push_back(DeckCard(CardId::MAD_SCIENCE));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 188);
    assert(combat.player.energy == 2);
    assert(combat.hand.empty() && combat.discard_pile.size() == 1);
    // Upgraded: still 1 energy and 12 damage, now Innate.
    CombatEngine up = make_big_fight();
    up.hand.push_back(DeckCard(CardId::MAD_SCIENCE, true));
    assert(up.play_card(0, 0));
    assert(up.enemies[0].hp == 188);
    assert(up.player.energy == 2);
    assert(up.hand.empty() && up.discard_pile.size() == 1);
    std::cout << "[PASS] Mad Science: 1 energy, 12 damage, Innate (upgraded only)" << std::endl;
}

void check_abundance(bool upgraded) {
    const int cost = upgraded ? 0 : 1;
    assert(CardDef::get_cost(CardId::ABUNDANCE, upgraded) == cost);
    assert(CardDef::get(CardId::ABUNDANCE).type == CardType::SKILL);
    assert(CardDef::has_keyword(CardId::ABUNDANCE, upgraded, KEYWORD_EXHAUST));
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::ABUNDANCE, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.energy == 3 - cost);
    // The card is exhausted; 3 different upgraded Powers are offered, free this turn once picked.
    assert(combat.hand.empty() && combat.exhaust_pile.size() == 1);
    assert(combat.choice_pending());
    assert(combat.offers.size() == 3);
    for (const DeckCard& offer : combat.offers) {
        assert(CardDef::get(offer.id).type == CardType::POWER);
        assert(offer.upgraded && offer.free_this_turn);
    }
    assert(combat.offers[0].id != combat.offers[1].id && combat.offers[1].id != combat.offers[2].id
           && combat.offers[0].id != combat.offers[2].id);
    const DeckCard picked = combat.offers[1];
    const uint8_t picks[] = { 1 };
    assert(combat.resolve_choice(picks));
    assert(!combat.choice_pending());
    assert(combat.hand.size() == 1 && combat.hand[0].id == picked.id);
    assert(combat.hand[0].upgraded && combat.hand[0].free_this_turn);
}

void test_abundance() {
    check_abundance(false);
    check_abundance(true);
    std::cout << "[PASS] Abundance: 1 energy (0 upgraded), 1 of 3 upgraded Powers free this turn, Exhaust" << std::endl;
}

} // namespace

int main() {
    test_apparition();
    test_maul();
    test_apotheosis();
    test_mad_science();
    test_abundance();
    return 0;
}
