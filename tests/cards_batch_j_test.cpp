// Enlightenment (#414), Dual Wield (#404), Call of the Void (#345), Heirloom Hammer (#343), Bundle of Joy (#342),
// Spectrum Shift (#340), Decisions, Decisions (#338), White Noise (#334), Creative AI (#333), Nightmare (#323),
// Tools of the Trade (#322), Infernal Blade (#317), Stoke (#316).
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <span>
#include "engine/card.h"
#include "engine/card_meta.h"
#include "engine/character.h"
#include "engine/combat.h"
#include "engine/power.h"

using namespace engine;

namespace {

CombatEngine make_fight(CharacterId hero = CharacterId::IRONCLAD) {
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::CULTIST);
    CombatEngine combat;
    combat.start_combat_multi(pack, hero);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.exhaust_pile.clear();
    combat.returning_cards.clear();
    combat.player.energy = 9;
    combat.player.block = 0;
    combat.enemies[0].max_hp = 300;
    combat.enemies[0].hp = 300;
    combat.enemies[0].block = 0;
    return combat;
}

bool pick(CombatEngine& combat, std::initializer_list<uint8_t> indices) {
    return combat.resolve_choice(std::span<const uint8_t>(indices.begin(), indices.size()));
}

void fill_draw_pile(CombatEngine& combat) {
    for (int i = 0; i < 20; ++i) combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
}

size_t count_of(const CombatEngine& combat, CardId id) {
    size_t count = 0;
    for (const DeckCard& card : combat.hand) count += card.id == id ? 1 : 0;
    return count;
}

void test_enlightenment(bool upgraded) {
    assert(CardDef::get_cost(CardId::ENLIGHTENMENT, upgraded) == 0);
    assert(CardDef::has_keyword(CardId::ENLIGHTENMENT, upgraded, KEYWORD_EXHAUST));
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::ENLIGHTENMENT, upgraded));
    combat.hand.push_back(DeckCard(CardId::BASH));     // 2
    combat.hand.push_back(DeckCard(CardId::ANGER));    // 0
    combat.hand.push_back(DeckCard(CardId::BLUDGEON)); // 3
    assert(combat.play_card(0, 0));
    assert(combat.exhaust_pile.size() == 1);
    assert(combat.effective_cost(combat.hand[0]) == 1);
    assert(combat.effective_cost(combat.hand[1]) == 0);
    assert(combat.effective_cost(combat.hand[2]) == 1);
    // Ends with the turn, unless upgraded: the cap holds for the combat.
    combat.retain_hand = true;
    fill_draw_pile(combat);
    combat.end_player_turn();
    assert(count_of(combat, CardId::BASH) == 1);
    for (const DeckCard& card : combat.hand) {
        if (card.id == CardId::BASH) assert(combat.effective_cost(card) == (upgraded ? 1 : 2));
        if (card.id == CardId::BLUDGEON) assert(combat.effective_cost(card) == (upgraded ? 1 : 3));
    }
}

void test_dual_wield(bool upgraded) {
    assert(CardDef::get_cost(CardId::DUAL_WIELD, upgraded) == 1);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::DUAL_WIELD, upgraded));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::METALLICIZE));
    assert(combat.play_card(0, 0));
    assert(combat.choice_pending());
    assert(!combat.choice_allows(0) && combat.choice_allows(1) && combat.choice_allows(2));
    assert(!pick(combat, { 0 }));
    assert(pick(combat, { 2 }));
    assert(count_of(combat, CardId::METALLICIZE) == (upgraded ? 3u : 2u));
    assert(count_of(combat, CardId::STRIKE) == 1);
}

void test_call_of_the_void(bool upgraded) {
    assert(CardDef::get_cost(CardId::CALL_OF_THE_VOID, upgraded) == 1);
    assert(CardDef::has_keyword(CardId::CALL_OF_THE_VOID, upgraded, KEYWORD_INNATE) == upgraded);
    assert(card_meta(CardId::CALL_OF_THE_VOID).pool == CardPool::NECROBINDER);
    CombatEngine combat = make_fight(CharacterId::NECROBINDER);
    combat.hand.push_back(DeckCard(CardId::CALL_OF_THE_VOID, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.has_power(PowerId::CALL_OF_THE_VOID));
    fill_draw_pile(combat);
    combat.end_player_turn();
    assert(combat.hand.size() == 6);
    size_t ethereal = 0;
    for (const DeckCard& card : combat.hand) ethereal += CardDef::has_keyword(card, KEYWORD_ETHEREAL) ? 1 : 0;
    assert(ethereal >= 1);
}

void test_heirloom_hammer(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::HEIRLOOM_HAMMER, upgraded) == 2);
    assert(card_meta(CardId::HEIRLOOM_HAMMER).pool == CardPool::REGENT);
    CombatEngine combat = make_fight(CharacterId::REGENT);
    combat.hand.push_back(DeckCard(CardId::HEIRLOOM_HAMMER, upgraded));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::FINESSE));
    combat.hand.push_back(DeckCard(CardId::FLASH_OF_STEEL));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300 - damage);
    assert(combat.choice_pending());
    assert(!combat.choice_allows(0) && combat.choice_allows(1) && combat.choice_allows(2));
    assert(pick(combat, { 1 }));
    assert(count_of(combat, CardId::FINESSE) == 2);
    assert(combat.hand.size() == 4);
}

void test_bundle_of_joy(bool upgraded, size_t cards) {
    assert(CardDef::get_cost(CardId::BUNDLE_OF_JOY, upgraded) == 1);
    CombatEngine combat = make_fight(CharacterId::REGENT);
    combat.hand.push_back(DeckCard(CardId::BUNDLE_OF_JOY, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.hand.size() == cards);
    for (const DeckCard& card : combat.hand) assert(card_meta(card.id).pool == CardPool::COLORLESS);
    assert(combat.exhaust_pile.size() == 1);
}

void test_spectrum_shift(bool upgraded) {
    assert(CardDef::get_cost(CardId::SPECTRUM_SHIFT, upgraded) == (upgraded ? 1 : 2));
    CombatEngine combat = make_fight(CharacterId::REGENT);
    combat.hand.push_back(DeckCard(CardId::SPECTRUM_SHIFT, upgraded));
    assert(combat.play_card(0, 0));
    fill_draw_pile(combat);
    combat.end_player_turn();
    assert(combat.hand.size() == 6);
    size_t colorless = 0;
    for (const DeckCard& card : combat.hand) colorless += card_meta(card.id).pool == CardPool::COLORLESS ? 1 : 0;
    assert(colorless == 1);
}

void test_decisions_decisions(bool upgraded, size_t drawn) {
    assert(CardDef::get_cost(CardId::DECISIONS_DECISIONS, upgraded) == 0);
    assert(CardDef::get_star_cost(CardId::DECISIONS_DECISIONS, upgraded) == 6);
    CombatEngine combat = make_fight(CharacterId::REGENT);
    combat.player.sovereign_stars = 5;
    combat.hand.push_back(DeckCard(CardId::DECISIONS_DECISIONS, upgraded));
    assert(!combat.play_card(0, 0)); // 6 Stars needed
    combat.player.sovereign_stars = 6;
    for (int i = 0; i < 10; ++i) combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    assert(combat.play_card(0, 0));
    assert(combat.player.sovereign_stars == 0);
    assert(combat.hand.size() == drawn);
    assert(combat.exhaust_pile.size() == 1);
    // Only the Defend is a Skill: it is played three times without being picked, then discarded.
    assert(combat.player.block == 15);
    assert(count_of(combat, CardId::DEFEND) == 0);
    assert(combat.discard_pile.size() == 1 && combat.discard_pile[0].id == CardId::DEFEND);
}

void test_card_maker(CardId id, CardType made, bool upgraded) {
    assert(CardDef::get_cost(id, upgraded) == (upgraded ? 0 : 1));
    assert(CardDef::has_keyword(id, upgraded, KEYWORD_EXHAUST));
    CombatEngine combat = make_fight(id == CardId::WHITE_NOISE ? CharacterId::DEFECT : CharacterId::IRONCLAD);
    combat.hand.push_back(DeckCard(id, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.hand.size() == 1);
    assert(CardDef::get(combat.hand[0].id).type == made);
    assert(combat.effective_cost(combat.hand[0]) == 0 || CardDef::is_x_cost(combat.hand[0].id, false));
    assert(combat.exhaust_pile.size() == 1);
}

void test_creative_ai(bool upgraded) {
    assert(CardDef::get_cost(CardId::CREATIVE_AI, upgraded) == (upgraded ? 2 : 3));
    CombatEngine combat = make_fight(CharacterId::DEFECT);
    combat.hand.push_back(DeckCard(CardId::CREATIVE_AI, upgraded));
    assert(combat.play_card(0, 0));
    fill_draw_pile(combat);
    combat.end_player_turn();
    assert(combat.hand.size() == 6);
    size_t powers = 0;
    for (const DeckCard& card : combat.hand) powers += CardDef::get(card.id).type == CardType::POWER ? 1 : 0;
    assert(powers == 1);
}

void test_nightmare(bool upgraded) {
    assert(CardDef::get_cost(CardId::NIGHTMARE, upgraded) == (upgraded ? 2 : 3));
    CombatEngine combat = make_fight(CharacterId::SILENT);
    combat.hand.push_back(DeckCard(CardId::NIGHTMARE, upgraded));
    combat.hand.push_back(DeckCard(CardId::NEUTRALIZE, true));
    combat.hand.push_back(DeckCard(CardId::SURVIVOR));
    assert(combat.play_card(0, 0));
    assert(combat.choice_pending());
    assert(pick(combat, { 0 }));
    assert(combat.exhaust_pile.size() == 1);
    assert(count_of(combat, CardId::NEUTRALIZE) == 1); // the picked card stays
    fill_draw_pile(combat);
    combat.end_player_turn();
    assert(count_of(combat, CardId::NEUTRALIZE) == 3);
    for (const DeckCard& card : combat.hand) {
        if (card.id == CardId::NEUTRALIZE) assert(card.upgraded);
    }
    assert(combat.hand.size() == 8);
}

void test_tools_of_the_trade(bool upgraded) {
    assert(CardDef::get_cost(CardId::TOOLS_OF_THE_TRADE, upgraded) == (upgraded ? 0 : 1));
    CombatEngine combat = make_fight(CharacterId::SILENT);
    combat.hand.push_back(DeckCard(CardId::TOOLS_OF_THE_TRADE, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.has_power(PowerId::TOOLS_OF_THE_TRADE));
    fill_draw_pile(combat);
    combat.end_player_turn();
    // Six cards drawn, then one of them to discard.
    assert(combat.hand.size() == 6);
    assert(combat.choice_pending());
    assert(pick(combat, { 0 }));
    assert(combat.hand.size() == 5);
    assert(combat.discard_pile.size() == 1);
}

void test_stoke(bool upgraded) {
    assert(CardDef::get_cost(CardId::STOKE, upgraded) == 1);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::STOKE, upgraded));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    combat.hand.push_back(DeckCard(CardId::BASH));
    assert(combat.play_card(0, 0));
    assert(combat.exhaust_pile.size() == 3);
    assert(combat.hand.size() == 3);
    for (const DeckCard& card : combat.hand) {
        assert(card.upgraded == upgraded);
        assert(card_meta(card.id).pool == CardPool::IRONCLAD);
    }
    assert(combat.discard_pile.size() == 1 && combat.discard_pile[0].id == CardId::STOKE);
}

} // namespace

int main() {
    for (bool up : { false, true }) {
        test_enlightenment(up);
        test_dual_wield(up);
        test_call_of_the_void(up);
        test_heirloom_hammer(up, up ? 25 : 20);
        test_bundle_of_joy(up, up ? 4 : 3);
        test_spectrum_shift(up);
        test_decisions_decisions(up, up ? 5 : 3);
        test_card_maker(CardId::WHITE_NOISE, CardType::POWER, up);
        test_creative_ai(up);
        test_nightmare(up);
        test_tools_of_the_trade(up);
        test_card_maker(CardId::INFERNAL_BLADE, CardType::ATTACK, up);
        test_stoke(up);
    }
    std::cout << "[PASS] Cards batch J: Enlightenment, Dual Wield, Call of the Void, Heirloom Hammer, Bundle of Joy, "
                 "Spectrum Shift, Decisions Decisions, White Noise, Creative AI, Nightmare, Tools of the Trade, "
                 "Infernal Blade, Stoke"
              << std::endl;
    return 0;
}
