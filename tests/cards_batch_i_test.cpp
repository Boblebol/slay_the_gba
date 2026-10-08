// Hello World (#401), Dark Shackles (#379), Equilibrium (#375), Purity (#374), Secret Technique (#373),
// Dramatic Entrance (#370), Secret Weapon (#367), Mayhem (#357), Prep Time (#355), Stratagem (#354),
// Calamity (#352), Automation (#350).
#include <cassert>
#include <cstdint>
#include <iostream>
#include <span>
#include "engine/card_meta.h"
#include "engine/combat.h"

using namespace engine;

namespace {

CombatEngine make_fight() {
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::CULTIST);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
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

void test_hello_world(bool upgraded) {
    assert(CardDef::get_cost(CardId::HELLO_WORLD, upgraded) == 1);
    assert(CardDef::has_keyword(CardId::HELLO_WORLD, upgraded, KEYWORD_INNATE) == upgraded);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::HELLO_WORLD, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.has_power(PowerId::HELLO_WORLD));
    for (int i = 0; i < 10; ++i) combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    combat.end_player_turn();
    // 5 drawn cards plus one random Common card.
    assert(combat.hand.size() == 6);
    int commons = 0;
    for (const DeckCard& card : combat.hand) commons += card_meta(card.id).rarity == CardRarity::COMMON ? 1 : 0;
    assert(commons >= 1);
}

void test_dark_shackles(bool upgraded, int loss) {
    assert(CardDef::get_cost(CardId::DARK_SHACKLES, upgraded) == 0);
    CombatEngine combat = make_fight();
    const int before = combat.enemies[0].get_status(StatusType::STRENGTH);
    combat.hand.push_back(DeckCard(CardId::DARK_SHACKLES, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].get_status(StatusType::STRENGTH) == before - loss);
    assert(combat.exhaust_pile.size() == 1);
}

void test_equilibrium(bool upgraded, int block) {
    assert(CardDef::get_cost(CardId::EQUILIBRIUM, upgraded) == 2);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::EQUILIBRIUM, upgraded));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    assert(!combat.retain_hand);
    assert(combat.play_card(0, 0));
    assert(combat.player.block == block);
    assert(combat.retain_hand);
}

void test_purity(bool upgraded, int cards) {
    assert(CardDef::get_cost(CardId::PURITY, upgraded) == 0);
    assert(CardDef::has_keyword(CardId::PURITY, upgraded, KEYWORD_RETAIN));
    assert(CardDef::has_keyword(CardId::PURITY, upgraded, KEYWORD_EXHAUST));
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::PURITY, upgraded));
    for (int i = 0; i < 6; ++i) combat.hand.push_back(DeckCard(CardId::STRIKE));
    assert(combat.play_card(0, 0));
    assert(combat.choice_pending());
    if (cards == 3) {
        assert(!pick(combat, { 0, 1, 2, 3 })); // at most 3
        assert(pick(combat, { 0, 1, 2 }));
    } else {
        assert(pick(combat, { 0, 1, 2, 3, 4 }));
    }
    assert(combat.hand.size() == static_cast<size_t>(6 - cards));
    assert(combat.exhaust_pile.size() == static_cast<size_t>(cards + 1)); // Purity exhausts itself too
}

void test_secret(CardId id, CardType wanted, CardType other, bool upgraded) {
    assert(CardDef::get_cost(id, upgraded) == 0);
    assert(CardDef::has_keyword(id, upgraded, KEYWORD_EXHAUST) == !upgraded);
    CombatEngine combat = make_fight();
    // Draw pile, bottom first: index 0 is an unwanted card, 1 and 3 are of the wanted type.
    const CardId wanted_id = wanted == CardType::SKILL ? CardId::DEFEND : CardId::STRIKE;
    const CardId other_id = other == CardType::SKILL ? CardId::DEFEND : CardId::STRIKE;
    combat.draw_pile.push_back(DeckCard(other_id));
    combat.draw_pile.push_back(DeckCard(wanted_id));
    combat.draw_pile.push_back(DeckCard(other_id));
    combat.draw_pile.push_back(DeckCard(wanted_id));
    combat.hand.push_back(DeckCard(id, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.choice_pending());
    assert(!combat.choice_allows(0) && combat.choice_allows(1) && !combat.choice_allows(2) && combat.choice_allows(3));
    assert(!pick(combat, { 0 }));
    assert(pick(combat, { 1 }));
    assert(combat.hand.size() == 1 && combat.hand[0].id == wanted_id);
    assert(combat.draw_pile.size() == 3);
}

void test_dramatic_entrance(bool upgraded, int damage) {
    assert(CardDef::get_cost(CardId::DRAMATIC_ENTRANCE, upgraded) == 0);
    assert(CardDef::has_keyword(CardId::DRAMATIC_ENTRANCE, upgraded, KEYWORD_INNATE));
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::DRAMATIC_ENTRANCE, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.enemies[0].hp == 300 - damage);
    assert(combat.exhaust_pile.size() == 1);
}

void test_mayhem(bool upgraded) {
    assert(CardDef::get_cost(CardId::MAYHEM, upgraded) == (upgraded ? 1 : 2));
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::MAYHEM, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.has_power(PowerId::MAYHEM));
    for (int i = 0; i < 10; ++i) combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    combat.draw_pile.push_back(DeckCard(CardId::STRIKE)); // top card
    combat.end_player_turn();
    // The Strike was played at the start of the turn: it hurt the Cultist and is not in the hand.
    int strikes = 0;
    for (const DeckCard& card : combat.hand) strikes += card.id == CardId::STRIKE ? 1 : 0;
    assert(strikes == 0);
    assert(combat.enemies[0].hp < 300);
    assert(combat.hand.size() == 5);
}

void test_prep_time(bool upgraded, int vigor) {
    assert(CardDef::get_cost(CardId::PREP_TIME, upgraded) == 1);
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::PREP_TIME, upgraded));
    assert(combat.play_card(0, 0));
    for (int i = 0; i < 10; ++i) combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    combat.end_player_turn();
    assert(combat.player.get_status(StatusType::VIGOR) == vigor);
}

void test_stratagem(bool upgraded) {
    assert(CardDef::get_cost(CardId::STRATAGEM, upgraded) == (upgraded ? 0 : 1));
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::STRATAGEM, upgraded));
    assert(combat.play_card(0, 0));
    assert(combat.has_power(PowerId::STRATAGEM));
    combat.discard_pile.push_back(DeckCard(CardId::BASH));
    combat.discard_pile.push_back(DeckCard(CardId::STRIKE));
    combat.discard_pile.push_back(DeckCard(CardId::DEFEND));
    combat.shuffle_discard_into_draw();
    assert(combat.choice_pending());
    assert(pick(combat, { 0 }));
    assert(combat.hand.size() == 1);
    assert(combat.draw_pile.size() == 2);
}

void test_calamity(bool upgraded) {
    assert(CardDef::get_cost(CardId::CALAMITY, upgraded) == (upgraded ? 2 : 3));
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::CALAMITY, upgraded));
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    assert(combat.play_card(0, 0));
    assert(combat.has_power(PowerId::CALAMITY));
    assert(combat.play_card(1, 0)); // Defend: nothing
    assert(combat.hand.size() == 1);
    assert(combat.play_card(0, 0)); // Strike: a random Attack comes to the hand
    assert(combat.hand.size() == 1);
    assert(CardDef::get(combat.hand[0].id).type == CardType::ATTACK);
}

void test_automation(bool upgraded) {
    assert(CardDef::get_cost(CardId::AUTOMATION, upgraded) == (upgraded ? 0 : 1));
    CombatEngine combat = make_fight();
    combat.hand.push_back(DeckCard(CardId::AUTOMATION, upgraded));
    for (int i = 0; i < 30; ++i) combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    assert(combat.play_card(0, 0));
    const int energy = combat.player.energy;
    combat.draw_cards(9);
    assert(combat.player.energy == energy);
    combat.draw_cards(1);
    assert(combat.player.energy == energy + 1);
    combat.hand.clear();
    combat.draw_cards(10);
    assert(combat.player.energy == energy + 2);
}

} // namespace

int main() {
    for (bool up : { false, true }) {
        test_hello_world(up);
        test_dark_shackles(up, up ? 15 : 9);
        test_equilibrium(up, up ? 16 : 13);
        test_purity(up, up ? 5 : 3);
        test_secret(CardId::SECRET_TECHNIQUE, CardType::SKILL, CardType::ATTACK, up);
        test_dramatic_entrance(up, up ? 15 : 11);
        test_secret(CardId::SECRET_WEAPON, CardType::ATTACK, CardType::SKILL, up);
        test_mayhem(up);
        test_prep_time(up, up ? 6 : 4);
        test_stratagem(up);
        test_calamity(up);
        test_automation(up);
    }
    std::cout << "[PASS] Cards batch I: Hello World, Dark Shackles, Equilibrium, Purity, Secret Technique, "
                 "Dramatic Entrance, Secret Weapon, Mayhem, Prep Time, Stratagem, Calamity, Automation"
              << std::endl;
    return 0;
}
