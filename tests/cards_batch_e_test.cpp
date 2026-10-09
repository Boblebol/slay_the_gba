// Stack (#426), Rip and Tear (#424), Scrawl (#386), Mind Blast (#369).
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

CombatEngine make_fight() {
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.exhaust_pile.clear();
    combat.player.energy = 3;
    combat.player.block = 0;
    combat.enemies[0].max_hp = 200;
    combat.enemies[0].hp = 200;
    combat.enemies[0].block = 0;
    return combat;
}

void test_stack(bool upgraded, int bonus) {
    assert(CardDef::get_cost(CardId::STACK, upgraded) == 1);
    assert(CardDef::get(CardId::STACK).type == CardType::SKILL);
    CombatEngine combat = make_fight();
    for (int i = 0; i < 4; ++i) combat.discard_pile.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::STACK, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.block == 4 + bonus);
    assert(combat.player.energy == 2);
}

void test_rip_and_tear(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::RIP_AND_TEAR, upgraded) == 1);
    assert(CardDef::get(CardId::RIP_AND_TEAR).type == CardType::ATTACK);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::RIP_AND_TEAR, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 200 - 2 * damage);
    assert(combat.player.energy == 2);
}

void test_scrawl(bool upgraded) {
    assert(CardDef::get_cost(CardId::SCRAWL, upgraded) == 1);
    assert(CardDef::has_keyword(CardId::SCRAWL, upgraded, KEYWORD_EXHAUST));
    assert(CardDef::has_keyword(CardId::SCRAWL, upgraded, KEYWORD_RETAIN) == upgraded);
    CombatEngine combat = make_fight();
    for (int i = 0; i < 20; ++i) combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::SCRAWL, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.hand.size() == combat.hand.capacity());
    assert(combat.exhaust_pile.size() == 1);
    // A short draw pile stops the draw without looping.
    CombatEngine few = make_fight();
    for (int i = 0; i < 3; ++i) few.draw_pile.push_back(DeckCard(CardId::STRIKE));
    few.hand.push_back(DeckCard(CardId::SCRAWL, upgraded));
    assert(few.play_card(0, 0));
    assert(few.hand.size() == 3);
}

void test_mind_blast(bool upgraded) {
    assert(CardDef::get_cost(CardId::MIND_BLAST, upgraded) == (upgraded ? 0 : 1));
    assert(CardDef::get(CardId::MIND_BLAST).type == CardType::ATTACK);
    assert(CardDef::has_keyword(CardId::MIND_BLAST, upgraded, KEYWORD_INNATE));
    CombatEngine combat = make_fight();
    for (int i = 0; i < 7; ++i) combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::MIND_BLAST, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 200 - 7);
    assert(combat.player.energy == (upgraded ? 3 : 2));
}

} // namespace

int main() {
    test_stack(false, 0);
    test_stack(true, 3);
    test_rip_and_tear(false, 7);
    test_rip_and_tear(true, 9);
    test_scrawl(false);
    test_scrawl(true);
    test_mind_blast(false);
    test_mind_blast(true);
    std::cout << "[PASS] Cards batch E: Stack, Rip and Tear, Scrawl, Mind Blast" << std::endl;
    return 0;
}
