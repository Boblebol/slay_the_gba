// Eternal Armor (#377), Fasten (#351), Rebound (#402), Panic Button (#385), Gold Axe (#380), Anointed (#390),
// Fisticuffs (#391), Rend (#397), The Bomb (#382), Rolling Boulder (#360), Panache (#359), Jackpot (#364).
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

CombatEngine make_fight(int enemy_count = 1) {
    FixedVector<EnemyType, 3> pack;
    for (int i = 0; i < enemy_count; ++i) pack.push_back(EnemyType::CULTIST);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.exhaust_pile.clear();
    combat.returning_cards.clear();
    combat.player.energy = 9;
    combat.player.block = 0;
    for (size_t i = 0; i < combat.enemies.size(); ++i) {
        combat.enemies[i].max_hp = 300;
        combat.enemies[i].hp = 300;
        combat.enemies[i].block = 0;
    }
    for (int i = 0; i < 6; ++i) combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    return combat;
}

void test_eternal_armor(bool upgraded, int amount) {
    assert(CardDef::get_cost(CardId::ETERNAL_ARMOR, upgraded) == 3);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::ETERNAL_ARMOR, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.player.get_status(StatusType::PLATED_ARMOR) == amount);
}

void test_fasten(bool upgraded, int amount) {
    assert(CardDef::get_cost(CardId::FASTEN, upgraded) == 1);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::FASTEN, upgraded));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    assert(combat.play_card(0, 0));
    assert(combat.play_card(0, 0));
    assert(combat.player.block == 5 + amount);
}

void test_rebound(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::REBOUND, upgraded) == 1);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::REBOUND, upgraded));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300 - damage);
    assert(combat.discard_pile.size() == 1);
    assert(combat.play_card(0, 0)); // Defend goes back on top of the draw pile
    assert(combat.draw_pile.size() == 7);
    assert(combat.draw_pile.back().id == CardId::DEFEND);
    assert(combat.discard_pile.size() == 1);
    assert(combat.play_card(0, 0)); // only the next card is moved
    assert(combat.discard_pile.size() == 2);
}

void test_panic_button(bool upgraded, int block) {
    assert(CardDef::get_cost(CardId::PANIC_BUTTON, upgraded) == 0);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::PANIC_BUTTON, upgraded));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    assert(combat.play_card(0, 0));
    assert(combat.player.block == block);
    assert(combat.exhaust_pile.size() == 1);
    assert(combat.play_card(0, 0));
    assert(combat.player.block == block); // this turn
    combat.end_player_turn();
    assert(combat.state == CombatState::PLAYER_TURN);
    combat.hand.clear();
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    combat.player.block = 0;
    assert(combat.play_card(0, 0));
    assert(combat.player.block == 0); // next turn
    combat.end_player_turn();
    combat.hand.clear();
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    combat.player.block = 0;
    assert(combat.play_card(0, 0));
    assert(combat.player.block == 5); // the turn after
}

void test_gold_axe(bool upgraded) {
    assert(CardDef::get_cost(CardId::GOLD_AXE, upgraded) == 1);
    assert(CardDef::has_keyword(DeckCard(CardId::GOLD_AXE, upgraded), KEYWORD_RETAIN) == upgraded);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    combat.hand.push_back(DeckCard(CardId::GOLD_AXE, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.play_card(0, 0));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300 - 3); // three cards played, itself included
}

void test_anointed(bool upgraded) {
    assert(CardDef::get_cost(CardId::ANOINTED, upgraded) == 1);
    assert(CardDef::has_keyword(DeckCard(CardId::ANOINTED, upgraded), KEYWORD_RETAIN) == upgraded);
    CombatEngine combat = make_fight();
    combat.draw_pile.clear();
    combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    combat.draw_pile.push_back(DeckCard(CardId::BARRICADE)); // Rare
    combat.draw_pile.push_back(DeckCard(CardId::STRIKE));
    combat.draw_pile.push_back(DeckCard(CardId::DEMON_FORM)); // Rare
    combat.hand.push_back(DeckCard(CardId::ANOINTED, upgraded));
    assert(card_meta(CardId::BARRICADE).rarity == CardRarity::RARE);
    assert(card_meta(CardId::DEMON_FORM).rarity == CardRarity::RARE);
    assert(combat.play_card(0, 0));
    assert(combat.hand.size() == 2);
    assert(combat.draw_pile.size() == 2);
    assert(combat.exhaust_pile.size() == 1);
}

void test_fisticuffs(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::FISTICUFFS, upgraded) == 1);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::FISTICUFFS, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300 - damage);
    assert(combat.player.block == damage);
}

void test_rend(bool upgraded, int base, int extra) {
    assert(CardDef::get_cost(CardId::REND, upgraded) == 1);
    CombatEngine plain = make_fight();
    plain.hand.push_back(DeckCard(CardId::REND, upgraded));
    assert(plain.play_card(0, 0));
    assert(plain.enemies[0].hp == 300 - base);

    CombatEngine debuffed = make_fight();
    debuffed.enemies[0].add_status(StatusType::WEAK, 2);
    debuffed.enemies[0].add_status(StatusType::POISON, 3);
    debuffed.enemies[0].add_status(StatusType::STRENGTH, 4); // not a debuff
    debuffed.hand.push_back(DeckCard(CardId::REND, upgraded));
    assert(debuffed.play_card(0, 0));
    assert(debuffed.enemies[0].hp == 300 - base - 2 * extra);
}

void test_the_bomb(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::THE_BOMB, upgraded) == 2);
    CombatEngine combat = make_fight(2);
    combat.hand.push_back(DeckCard(CardId::THE_BOMB, upgraded));
    assert(combat.play_card(0, 0));
    for (int turn = 0; turn < 3; ++turn) {
        assert(combat.enemies[0].hp == 300 && combat.enemies[1].hp == 300);
        combat.end_player_turn();
        assert(combat.state == CombatState::PLAYER_TURN);
        combat.player.hp = combat.player.max_hp;
    }
    assert(combat.enemies[0].hp == 300 - damage);
    assert(combat.enemies[1].hp == 300 - damage);
}

void test_rolling_boulder(bool upgraded, int first) {
    assert(CardDef::get_cost(CardId::ROLLING_BOULDER, upgraded) == 3);
    CombatEngine combat = make_fight(2);
    combat.hand.push_back(DeckCard(CardId::ROLLING_BOULDER, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300);
    for (int turn = 0; turn < 2; ++turn) {
        combat.end_player_turn();
        combat.player.hp = combat.player.max_hp;
        const int expected = 300 - first - (turn == 1 ? first + 5 : 0);
        assert(combat.enemies[0].hp == expected);
        assert(combat.enemies[1].hp == expected);
    }
}

void test_panache(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::PANACHE, upgraded) == 0);
    CombatEngine combat = make_fight(2);
    combat.hand.push_back(DeckCard(CardId::PANACHE, upgraded));
    for (int i = 0; i < 5; ++i) combat.hand.push_back(DeckCard(CardId::DEFEND));
    assert(combat.play_card(0, 0)); // Panache does not count itself
    for (int i = 0; i < 4; ++i) assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300 && combat.enemies[1].hp == 300);
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300 - damage);
    assert(combat.enemies[1].hp == 300 - damage);
}

void test_jackpot(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::JACKPOT, upgraded) == 3);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::JACKPOT, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300 - damage);
    assert(combat.hand.size() == 3);
    for (const DeckCard& card : combat.hand) {
        assert(card.upgraded == upgraded);
        assert(CardDef::get_cost(card.id, upgraded) == 0);
    }
}

} // namespace

int main() {
    for (bool up : { false, true }) {
        test_eternal_armor(up, up ? 12 : 9);
        test_fasten(up, up ? 6 : 4);
        test_rebound(up, up ? 12 : 9);
        test_panic_button(up, up ? 40 : 30);
        test_gold_axe(up);
        test_anointed(up);
        test_fisticuffs(up, up ? 9 : 7);
        test_rend(up, up ? 12 : 10, up ? 8 : 5);
        test_the_bomb(up, up ? 50 : 40);
        test_rolling_boulder(up, up ? 10 : 5);
        test_panache(up, up ? 14 : 10);
        test_jackpot(up, up ? 30 : 25);
    }
    std::cout << "[PASS] Cards batch G: Eternal Armor, Fasten, Rebound, Panic Button, Gold Axe, Anointed, Fisticuffs, "
                 "Rend, The Bomb, Rolling Boulder, Panache, Jackpot"
              << std::endl;
    return 0;
}
