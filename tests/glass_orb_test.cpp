// Glass orb (#99): passive damage to ALL enemies that drops by 1 each turn, evoke damage to ALL enemies, and the
// cards that channel it: Glasswork, Refract, Spinner. The orb values are an assumption (see GLASS_PASSIVE).
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

bool in_pool(CardId id) {
    for (CardId pool_card : RunManager::card_reward_pool(CharacterId::DEFECT)) {
        if (pool_card == id) return true;
    }
    return false;
}

CombatEngine fight() {
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::JAW_WORM);
    pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::DEFECT);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.orbs.clear();
    combat.player.energy = 9;
    combat.player.block = 0;
    for (Enemy& foe : combat.enemies) foe.hp = foe.max_hp = 500;
    return combat;
}

void test_glass_orb() {
    CombatEngine combat = fight();
    combat.channel_orb(OrbType::GLASS);
    assert(combat.player.orbs.size() == 1 && combat.player.orbs[0] == OrbType::GLASS);
    // The passive hits every enemy at the end of the turn and weakens by 1 each time.
    combat.end_player_turn();
    assert(combat.enemies[0].hp == 500 - 4 && combat.enemies[1].hp == 500 - 4);
    combat.end_player_turn();
    assert(combat.enemies[0].hp == 500 - 4 - 3 && combat.enemies[1].hp == 500 - 4 - 3);
    // Evoked: damage to ALL enemies.
    combat = fight();
    combat.channel_orb(OrbType::GLASS);
    combat.evoke_orb(0);
    assert(combat.enemies[0].hp == 500 - 6 && combat.enemies[1].hp == 500 - 6 && combat.player.orbs.empty());
    // Never below 0.
    combat = fight();
    combat.channel_orb(OrbType::GLASS);
    for (int turn = 0; turn < 6; ++turn) combat.end_player_turn();
    assert(combat.player.orbs[0].charge == 0);
    std::cout << "[PASS] Glass orb: passive damage to ALL enemies, 1 less each turn, evoke damage to ALL enemies" << std::endl;
}

void test_glasswork() {
    assert(in_pool(CardId::GLASSWORK));
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::GLASSWORK, up) == 1);
        CombatEngine combat = fight();
        combat.hand.push_back(DeckCard(CardId::GLASSWORK, up));
        assert(combat.play_card(0, 0));
        assert(combat.player.block == (up ? 8 : 5));
        assert(combat.player.orbs.size() == 1 && combat.player.orbs[0] == OrbType::GLASS);
    }
    std::cout << "[PASS] Glasswork: 5 (8) Block and 1 Glass orb" << std::endl;
}

void test_refract() {
    assert(in_pool(CardId::REFRACT));
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::REFRACT, up) == 3);
        CombatEngine combat = fight();
        combat.hand.push_back(DeckCard(CardId::REFRACT, up));
        assert(combat.play_card(0, 0));
        assert(combat.enemies[0].hp == 500 - 2 * (up ? 13 : 10));
        assert(combat.player.orbs.size() == 2 && combat.player.orbs[0] == OrbType::GLASS && combat.player.orbs[1] == OrbType::GLASS);
    }
    std::cout << "[PASS] Refract: 10 (13) damage twice and 2 Glass orbs" << std::endl;
}

void test_spinner() {
    assert(in_pool(CardId::SPINNER));
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::SPINNER, up) == 1);
        CombatEngine combat = fight();
        combat.hand.push_back(DeckCard(CardId::SPINNER, up));
        assert(combat.play_card(0, 0));
        assert(combat.player.orbs.size() == (up ? 1u : 0u));
        combat.player.orbs.clear();
        combat.end_player_turn();
        // The next turn begins with a Glass orb channeled.
        assert(combat.player.orbs.size() == 1 && combat.player.orbs[0] == OrbType::GLASS);
    }
    std::cout << "[PASS] Spinner: a Glass orb at the start of each turn, one more when played upgraded" << std::endl;
}

} // namespace

int main() {
    test_glass_orb();
    test_glasswork();
    test_refract();
    test_spinner();
    std::cout << "All Glass orb tests passed." << std::endl;
    return 0;
}
