// Engine fixes: rules that used to break silently, one test per fix.
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

// Jaw Worm opens with Chomp (11 damage, no block): a predictable punching bag.
CombatEngine make_combat(CharacterId hero, EnemyType enemy_type = EnemyType::JAW_WORM) {
    CombatEngine combat;
    combat.start_combat(enemy_type, hero);
    return combat;
}

int count_in(const FixedVector<DeckCard, 10>& pile, CardId id) {
    int count = 0;
    for (const DeckCard& card : pile) count += card.id == id;
    return count;
}

void test_enemy_buff_does_not_leak() {
    // Each move sets the whole intent: Strength only comes with a buff move, never with the attacks after it.
    for (EnemyType type : { EnemyType::JAW_WORM, EnemyType::LOUSE, EnemyType::GREMLIN_NOB, EnemyType::CEREMONIAL_BEAST }) {
        CombatEngine combat = make_combat(CharacterId::IRONCLAD, type);
        combat.player.max_hp = 999;
        combat.player.hp = 999;
        Enemy& foe = combat.enemies[0];
        int16_t expected = 0;
        for (int turn = 1; turn <= 6; ++turn) {
            const EnemyIntent intent = foe.current_intent;
            if (intent.type == EnemyIntentType::BUFF) {
                assert(intent.status_type == StatusType::STRENGTH && intent.status_amount > 0);
                expected = static_cast<int16_t>(expected + intent.status_amount);
            } else {
                assert(intent.status_type == StatusType::NONE && intent.status_amount == 0);
            }
            combat.hand.clear();
            combat.end_player_turn();
            assert(combat.state == CombatState::PLAYER_TURN);
            assert(foe.get_status(StatusType::STRENGTH) == expected);
        }
        // Gremlin Nob and the Beast roar once, the Jaw Worm bellows on turns 3 and 5, the Louse never buffs.
        int16_t final_strength = type == EnemyType::JAW_WORM ? 6 : type == EnemyType::LOUSE ? 0 : 2;
        assert(expected == final_strength);
    }
    std::cout << "[PASS] Enemy intents: Jaw Worm, Louse, Gremlin Nob and the Beast gain Strength only on their buff moves" << std::endl;
}

void test_fiend_fire_with_dark_embrace() {
    // Fiend Fire burns the 3 cards held when it is played; what Dark Embrace draws stays in hand.
    CombatEngine fiend = make_combat(CharacterId::IRONCLAD);
    fiend.add_power(PowerId::DARK_EMBRACE, 1);
    fiend.hand.clear();
    fiend.hand.push_back(CardId::FIEND_FIRE);
    fiend.hand.push_back(CardId::STRIKE);
    fiend.hand.push_back(CardId::DEFEND);
    fiend.hand.push_back(CardId::DEFEND);
    fiend.draw_pile.clear();
    fiend.discard_pile.clear();
    fiend.exhaust_pile.clear();
    for (int i = 0; i < 20; ++i) fiend.draw_pile.push_back(CardId::BASH);
    uint8_t hits_before = fiend.counters.hits_dealt;
    assert(fiend.play_card(0));

    assert(fiend.counters.hits_dealt - hits_before == 3);
    assert(fiend.enemies[0].hp == 42 - 3 * 7);
    // The 3 cards and Fiend Fire itself are exhausted, each one drawing a card.
    assert(fiend.exhaust_pile.size() == 4);
    for (const DeckCard& card : fiend.exhaust_pile) assert(card.id != CardId::BASH);
    assert(fiend.hand.size() == 4 && count_in(fiend.hand, CardId::BASH) == 4);
    assert(fiend.draw_pile.size() == 16);
    std::cout << "[PASS] Fiend Fire + Dark Embrace: 3 cards burnt, 3 hits, the 4 cards drawn stay in hand" << std::endl;
}

void test_hand_discards_with_a_draw_reaction() {
    // Tingsha kills the enemy on the first discard and Gremlin Horn draws a card: that card was not held when the
    // card was played, so it is not discarded with the hand.
    for (CardId played : { CardId::STORM_OF_STEEL, CardId::CALCULATED_GAMBLE }) {
        CombatEngine combat = make_combat(CharacterId::SILENT);
        combat.add_power(PowerId::TINGSHA, 3);
        combat.add_power(PowerId::GREMLIN_HORN, 1);
        combat.enemies[0].hp = 3;
        combat.hand.clear();
        combat.hand.push_back(played);
        combat.hand.push_back(CardId::STRIKE);
        combat.hand.push_back(CardId::DEFEND);
        combat.draw_pile.clear();
        combat.discard_pile.clear();
        for (int i = 0; i < 10; ++i) combat.draw_pile.push_back(CardId::NEUTRALIZE);
        assert(combat.play_card(0));

        assert(!combat.enemies[0].is_alive());
        int discarded = 0;
        for (const DeckCard& card : combat.discard_pile) {
            assert(card.id != CardId::NEUTRALIZE);
            discarded += card.id == CardId::STRIKE || card.id == CardId::DEFEND;
        }
        assert(discarded == 2);
        if (played == CardId::STORM_OF_STEEL) {
            // One Shiv per card held, plus the card Gremlin Horn drew.
            assert(count_in(combat.hand, CardId::SHIV) == 2 && count_in(combat.hand, CardId::NEUTRALIZE) == 1);
        } else {
            // As many cards drawn as held, plus the card Gremlin Horn drew.
            assert(combat.hand.size() == 3 && count_in(combat.hand, CardId::NEUTRALIZE) == 3);
        }
    }
    std::cout << "[PASS] Storm of Steel, Calculated Gamble: a card drawn while the hand is discarded stays in hand" << std::endl;
}

} // namespace

int main() {
    test_enemy_buff_does_not_leak();
    test_fiend_fire_with_dark_embrace();
    test_hand_discards_with_a_draw_reaction();
    std::cout << "All engine fix tests passed." << std::endl;
    return 0;
}
