// Ironclad cards added on top of the existing mechanics: each one is played in its base and upgraded versions,
// and must be offered in the hero's card pool.
#include <algorithm>
#include <cassert>
#include <iostream>
#include <span>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

// Jaw Worm opens with Chomp (11 damage, no block): a predictable punching bag.
CombatEngine make_combat(CharacterId hero, EnemyType enemy_type = EnemyType::JAW_WORM) {
    CombatEngine combat;
    combat.start_combat(enemy_type, hero);
    return combat;
}

// Puts `card` alone in hand with full energy and plays it.
void play(CombatEngine& combat, DeckCard card, int target = -1) {
    combat.hand.clear();
    combat.hand.push_back(card);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, target);
    assert(played);
}

void make_enemy_idle(Enemy& enemy) {
    enemy.current_intent = EnemyIntent{};
    enemy.current_intent.type = EnemyIntentType::DEFEND;
}

// Ends the turn with every enemy doing nothing, so only the player's own effects show.
void end_turn_idle(CombatEngine& combat) {
    for (Enemy& foe : combat.enemies) make_enemy_idle(foe);
    combat.end_player_turn();
}

void assert_in_pool(CharacterId hero, std::initializer_list<CardId> cards) {
    std::span<const CardId> pool = RunManager::card_reward_pool(hero);
    for (CardId id : cards) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
}

int count_in(const auto& pile, CardId id) {
    int count = 0;
    for (const DeckCard& card : pile) count += card.id == id;
    return count;
}

// Plays the Attack at the start of the hand, on the enemy, with energy to spare.
void play_attack(CombatEngine& combat) {
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, 0);
    assert(played);
}

void test_juggling() {
    for (bool upgraded : { false, true }) {
        // Juggling: a Power that costs 1, innate once upgraded.
        CombatEngine juggling = make_combat(CharacterId::IRONCLAD);
        play(juggling, DeckCard(CardId::JUGGLING, upgraded));
        assert(juggling.has_power(PowerId::JUGGLING));
        assert(juggling.player.energy == juggling.player.max_energy - 1);
        assert(CardDef::has_keyword(CardId::JUGGLING, upgraded, KEYWORD_INNATE) == upgraded);

        // The 3rd Attack of the turn comes back to the hand: the first two and the Skills do not count.
        juggling.hand.clear();
        juggling.hand.push_back(CardId::STRIKE);
        juggling.hand.push_back(CardId::DEFEND);
        juggling.hand.push_back(CardId::STRIKE);
        juggling.hand.push_back(CardId::BASH);
        play_attack(juggling);
        assert(juggling.hand.size() == 3);
        bool defended = juggling.play_card(0);
        assert(defended);
        play_attack(juggling);
        assert(juggling.hand.size() == 1 && juggling.hand[0] == CardId::BASH);
        assert(juggling.counters.attacks_played == 2);
        play_attack(juggling);
        assert(juggling.hand.size() == 1 && juggling.hand[0] == CardId::BASH);
        assert(juggling.enemies[0].hp == 42 - 6 - 6 - 8);
        // A 4th Attack is not copied.
        juggling.hand.clear();
        juggling.hand.push_back(CardId::STRIKE);
        play_attack(juggling);
        assert(juggling.hand.empty());

        // The tally restarts with the next turn.
        end_turn_idle(juggling);
        juggling.enemies[0].hp = 42;
        juggling.hand.clear();
        juggling.hand.push_back(CardId::STRIKE);
        juggling.hand.push_back(CardId::STRIKE);
        juggling.hand.push_back(CardId::STRIKE);
        play_attack(juggling);
        play_attack(juggling);
        assert(juggling.hand.size() == 1);
        play_attack(juggling);
        assert(juggling.hand.size() == 1 && juggling.hand[0] == CardId::STRIKE);
        assert(count_in(juggling.discard_pile, CardId::STRIKE) >= 3);
    }

    assert_in_pool(CharacterId::IRONCLAD, { CardId::JUGGLING });
    std::cout << "[PASS] Ironclad: Juggling" << std::endl;
}

void test_unmovable() {
    for (bool upgraded : { false, true }) {
        // Unmovable: a Power, 2 energy (1 upgraded).
        CombatEngine combat = make_combat(CharacterId::IRONCLAD);
        assert(CardDef::get_cost(CardId::UNMOVABLE, upgraded) == (upgraded ? 1 : 2));
        play(combat, DeckCard(CardId::UNMOVABLE, upgraded));
        assert(combat.has_power(PowerId::UNMOVABLE));
        assert(combat.player.energy == combat.player.max_energy - (upgraded ? 1 : 2));

        // The first Block a card gives each turn is doubled, the next ones are not.
        combat.player.block = 0;
        play(combat, CardId::DEFEND);
        assert(combat.player.block == 10);
        play(combat, CardId::DEFEND);
        assert(combat.player.block == 15);

        // It applies again the next turn.
        end_turn_idle(combat);
        combat.player.block = 0;
        play(combat, CardId::DEFEND);
        assert(combat.player.block == 10);
    }

    assert_in_pool(CharacterId::IRONCLAD, { CardId::UNMOVABLE });
    std::cout << "[PASS] Ironclad: Unmovable" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Ironclad Cards ===" << std::endl;
    test_juggling();
    test_unmovable();
    std::cout << "=== All Ironclad Cards Passed! ===" << std::endl;
    return 0;
}
