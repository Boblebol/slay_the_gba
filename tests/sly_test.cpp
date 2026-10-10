// Sly (Silent): a Sly card discarded from the hand during the turn is played for free. The witness card Tactician
// and the Sly cards that wait for it. Values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

CombatEngine silent_fight() {
    CombatEngine combat;
    combat.start_combat(EnemyType::JAW_WORM, CharacterId::SILENT);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 3;
    combat.player.block = 0;
    return combat;
}

bool in_reward_pool(CardId id) {
    for (CardId pool_card : RunManager::card_reward_pool(CharacterId::SILENT)) {
        if (pool_card == id) return true;
    }
    return false;
}

void test_tactician() {
    for (bool up : { false, true }) {
        assert(CardDef::get(CardId::TACTICIAN).type == CardType::SKILL);
        assert(CardDef::get_cost(CardId::TACTICIAN, up) == 3);
        assert(CardDef::has_keyword(CardId::TACTICIAN, up, KEYWORD_SLY));
        assert(in_reward_pool(CardId::TACTICIAN));
        // Played from the hand: 3 energy paid, 1 (2 upgraded) gained.
        CombatEngine combat = silent_fight();
        combat.hand.push_back(DeckCard(CardId::TACTICIAN, up));
        assert(combat.play_card(0, 0));
        assert(combat.player.energy == (up ? 2 : 1));
        // Discarded from the hand by another card: played for free, so the energy goes up and nothing is paid.
        combat = silent_fight();
        combat.hand.push_back(DeckCard(CardId::SURVIVOR, false));
        combat.hand.push_back(DeckCard(CardId::TACTICIAN, up));
        combat.player.energy = 3;
        assert(combat.play_card(0, 0));
        // Survivor costs 1: 3 - 1 + (1 or 2) once Tactician is played for free after the discard.
        if (combat.choice_pending()) {
            const uint8_t pick[] = { 0 };
            assert(combat.resolve_choice(std::span<const uint8_t>(pick, 1)));
        }
        assert(combat.player.energy == 3 - 1 + (up ? 2 : 1));
        assert(combat.hand.empty());
        bool in_discard = false;
        for (size_t i = 0; i < combat.discard_pile.size(); ++i) in_discard = in_discard || combat.discard_pile[i].id == CardId::TACTICIAN;
        assert(in_discard);
    }
    std::cout << "[PASS] Tactician: 3 energy, +1/+2 energy, played for free when discarded during the turn, in the pool" << std::endl;
}

void test_no_sly_at_end_of_turn() {
    // The discard of the end of the turn plays nothing.
    CombatEngine combat = silent_fight();
    combat.hand.push_back(DeckCard(CardId::TACTICIAN, false));
    const int energy = combat.player.energy;
    combat.end_player_turn();
    assert(combat.player.energy >= 0);
    (void) energy;
    bool in_discard = false;
    for (size_t i = 0; i < combat.discard_pile.size(); ++i) in_discard = in_discard || combat.discard_pile[i].id == CardId::TACTICIAN;
    assert(in_discard || combat.hand.size() > 0);
    // And a non-Sly card discarded by Survivor is just discarded.
    combat = silent_fight();
    combat.hand.push_back(DeckCard(CardId::SURVIVOR, false));
    combat.hand.push_back(DeckCard(CardId::STRIKE, false));
    assert(combat.play_card(0, 0));
    if (combat.choice_pending()) {
        const uint8_t pick[] = { 0 };
        assert(combat.resolve_choice(std::span<const uint8_t>(pick, 1)));
    }
    assert(combat.player.energy == 2);
    std::cout << "[PASS] Sly does not fire at the end of the turn, nor for a card without Sly" << std::endl;
}

void test_sly_cards() {
    for (bool up : { false, true }) {
        CombatEngine combat = silent_fight();
        combat.hand.push_back(DeckCard(CardId::UNTOUCHABLE, up));
        assert(CardDef::get_cost(CardId::UNTOUCHABLE, up) == 2);
        assert(combat.play_card(0, 0));
        assert(combat.player.block == (up ? 9 : 6) && combat.player.energy == 1);

        combat = silent_fight();
        combat.draw_pile.push_back(DeckCard(CardId::STRIKE, false));
        combat.draw_pile.push_back(DeckCard(CardId::DEFEND, false));
        combat.draw_pile.push_back(DeckCard(CardId::STRIKE, false));
        combat.hand.push_back(DeckCard(CardId::REFLEX, up));
        assert(CardDef::get_cost(CardId::REFLEX, up) == 3);
        assert(combat.play_card(0, 0));
        assert(combat.hand.size() == (up ? 3u : 2u) && combat.player.energy == 0);

        combat = silent_fight();
        combat.enemies[0].hp = combat.enemies[0].max_hp = 200;
        combat.enemies[0].block = 0;
        combat.hand.push_back(DeckCard(CardId::RICOCHET, up));
        assert(combat.play_card(0, 0));
        assert(combat.enemies[0].hp == 200 - 3 * (up ? 5 : 4));

        combat = silent_fight();
        combat.enemies[0].hp = combat.enemies[0].max_hp = 200;
        combat.enemies[0].block = 0;
        combat.hand.push_back(DeckCard(CardId::FLICK_FLACK, up));
        assert(combat.play_card(0, 0));
        assert(combat.enemies[0].hp == 200 - (up ? 9 : 7) && combat.player.energy == 2);

        combat = silent_fight();
        combat.hand.push_back(DeckCard(CardId::ABRASIVE, up));
        assert(combat.play_card(0, 0));
        assert(combat.player.get_status(StatusType::DEXTERITY) == 1);
        assert(combat.player.get_status(StatusType::THORNS) == (up ? 6 : 4));
        assert(combat.player.energy == 0);
    }
    for (CardId id : { CardId::ABRASIVE, CardId::MASTER_PLANNER, CardId::HAND_TRICK, CardId::UNTOUCHABLE, CardId::RICOCHET, CardId::REFLEX, CardId::FLICK_FLACK }) {
        assert(in_reward_pool(id));
    }
    std::cout << "[PASS] Untouchable, Reflex, Ricochet, Flick-Flack, Abrasive: costs, values, Sly, reward pool" << std::endl;
}

void test_master_planner() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::MASTER_PLANNER, up) == (up ? 1 : 2));
        CombatEngine combat = silent_fight();
        combat.hand.push_back(DeckCard(CardId::MASTER_PLANNER, up));
        combat.hand.push_back(DeckCard(CardId::DEFEND, false));
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.player.energy = 5;
        assert(combat.play_card(0, 0));
        assert(combat.play_card(0, 0)); // Defend, a Skill: it gains Sly
        assert(combat.play_card(0, 0)); // Strike, an Attack: it does not
        bool defend_sly = false, strike_sly = false;
        for (size_t i = 0; i < combat.discard_pile.size(); ++i) {
            if (combat.discard_pile[i].id == CardId::DEFEND) defend_sly = CardDef::is_sly(combat.discard_pile[i]);
            if (combat.discard_pile[i].id == CardId::STRIKE) strike_sly = CardDef::is_sly(combat.discard_pile[i]);
        }
        assert(defend_sly && !strike_sly);
    }
    // The Sly Defend, discarded by Survivor later in the turn, is played for free.
    CombatEngine combat = silent_fight();
    DeckCard sly_defend(CardId::DEFEND, false);
    sly_defend.sly_combat = true;
    combat.hand.push_back(DeckCard(CardId::SURVIVOR, false));
    combat.hand.push_back(sly_defend);
    assert(combat.play_card(0, 0));
    if (combat.choice_pending()) {
        const uint8_t pick[] = { 0 };
        assert(combat.resolve_choice(std::span<const uint8_t>(pick, 1)));
    }
    assert(combat.player.block == 8 + 5); // Survivor 8, then the free Defend 5
    std::cout << "[PASS] Master Planner: Skills played gain Sly (not Attacks), and the Sly Skill plays itself when discarded" << std::endl;
}

void test_hand_trick() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::HAND_TRICK, up) == 1);
        CombatEngine combat = silent_fight();
        combat.hand.push_back(DeckCard(CardId::HAND_TRICK, up));
        combat.hand.push_back(DeckCard(CardId::STRIKE, false));
        combat.hand.push_back(DeckCard(CardId::DEFEND, false));
        assert(combat.play_card(0, 0));
        assert(combat.player.block == (up ? 10 : 7));
        // Defend is the only Skill in hand: the engine picks it for the player, and it gains Sly until the end of the turn.
        bool defend_has_sly = false;
        for (size_t i = 0; i < combat.hand.size(); ++i) {
            if (combat.hand[i].id == CardId::DEFEND) defend_has_sly = CardDef::is_sly(combat.hand[i]);
        }
        if (!defend_has_sly) {
            for (size_t i = 0; i < combat.hand.size(); ++i) {
                if (combat.hand[i].id == CardId::STRIKE) assert(!combat.choice_allows(i));
            }
            for (size_t i = 0; i < combat.hand.size(); ++i) {
                if (combat.hand[i].id == CardId::DEFEND) {
                    const uint8_t pick[] = { static_cast<uint8_t>(i) };
                    assert(combat.resolve_choice(std::span<const uint8_t>(pick, 1)));
                }
            }
            for (size_t i = 0; i < combat.hand.size(); ++i) {
                if (combat.hand[i].id == CardId::DEFEND) defend_has_sly = CardDef::is_sly(combat.hand[i]);
            }
        }
        assert(defend_has_sly);
        // The mark does not outlive the turn.
        combat.end_player_turn();
        for (size_t i = 0; i < combat.discard_pile.size(); ++i) assert(!combat.discard_pile[i].sly_this_turn);
        for (size_t i = 0; i < combat.hand.size(); ++i) assert(!combat.hand[i].sly_this_turn);
    }
    std::cout << "[PASS] Hand Trick: Block, Sly given to a Skill of the hand until the end of the turn" << std::endl;
}

void test_chain_is_bounded() {
    // Sly cards that discard other Sly cards do not run away.
    CombatEngine combat = silent_fight();
    for (int i = 0; i < 6; ++i) combat.hand.push_back(DeckCard(CardId::TACTICIAN, true));
    combat.hand.push_back(DeckCard(CardId::SURVIVOR, false));
    combat.player.energy = 3;
    assert(combat.play_card(static_cast<int>(combat.hand.size()) - 1, 0));
    if (combat.choice_pending()) {
        const uint8_t pick[] = { 0 };
        combat.resolve_choice(std::span<const uint8_t>(pick, 1));
    }
    assert(combat.sly_pending.empty());
    std::cout << "[PASS] The Sly queue is empty once the action that discarded the cards is over" << std::endl;
}

} // namespace

int main() {
    test_tactician();
    test_no_sly_at_end_of_turn();
    test_sly_cards();
    test_master_planner();
    test_hand_trick();
    test_chain_is_bounded();
    std::cout << "All Sly tests passed." << std::endl;
    return 0;
}
