// Random combat simulation: plays seeded, reproducible fights for every hero and checks the invariants that must
// hold after each card and each end of turn. It does not check what a card does, only that the combat stays sane.
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <set>
#include <utility>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

constexpr int SEEDS_PER_HERO = 40;
constexpr int MAX_TURNS = 30;
constexpr int DRAW_PILE_SIZE = 20;
constexpr size_t MAX_HAND = 10;
constexpr double TIME_BUDGET_SECONDS = 5.0;

// Seeds (hero, seed) known to break an invariant, kept out of the run. Each one is a bug to report, not to hide.
const std::set<std::pair<int, uint32_t>> EXCLUDED_SEEDS = {};

// xorshift32, written here so the test owns its randomness: the same seed always gives the same fight.
class Rng {
public:
    explicit Rng(uint32_t seed) : state_(seed * 2654435761u + 0x9E3779B9u) {
        if (state_ == 0) state_ = 0x6D2B79F5u;
        for (int i = 0; i < 4; ++i) next();
    }
    uint32_t next() {
        state_ ^= state_ << 13;
        state_ ^= state_ >> 17;
        state_ ^= state_ << 5;
        return state_;
    }
    // Uniform enough for a test; `bound` must be positive.
    size_t below(size_t bound) { return next() % bound; }

private:
    uint32_t state_;
};

const char* hero_name(CharacterId hero) {
    switch (hero) {
        case CharacterId::IRONCLAD: return "IRONCLAD";
        case CharacterId::SILENT: return "SILENT";
        case CharacterId::DEFECT: return "DEFECT";
        case CharacterId::REGENT: return "REGENT";
        case CharacterId::NECROBINDER: return "NECROBINDER";
        default: return "?";
    }
}

struct Failure {
    bool failed = false;
    char message[160] = {0};
};

// Records the first broken invariant of a fight.
void fail(Failure& failure, const char* what, const char* where, int turn) {
    if (failure.failed) return;
    failure.failed = true;
    std::snprintf(failure.message, sizeof(failure.message), "%s (%s, turn %d)", what, where, turn);
}

template <size_t N>
bool has_none_card(const FixedVector<DeckCard, N>& pile) {
    for (const DeckCard& card : pile) {
        if (card.id == CardId::NONE) return true;
    }
    return false;
}

void check_invariants(const CombatEngine& combat, Failure& failure, const char* where) {
    const int turn = combat.turn_number;
    const Player& player = combat.player;

    if (player.hp < 0 || player.hp > player.max_hp) fail(failure, "HP outside 0..max HP", where, turn);
    if (player.block < 0) fail(failure, "negative Block", where, turn);
    if (combat.hand.size() > MAX_HAND) fail(failure, "hand over 10 cards", where, turn);
    if (has_none_card(combat.hand)) fail(failure, "NONE card in hand", where, turn);
    if (has_none_card(combat.draw_pile)) fail(failure, "NONE card in draw pile", where, turn);
    if (has_none_card(combat.discard_pile)) fail(failure, "NONE card in discard pile", where, turn);
    if (has_none_card(combat.exhaust_pile)) fail(failure, "NONE card in exhaust pile", where, turn);
    if (combat.state == CombatState::VICTORY && combat.has_alive_enemies()) {
        fail(failure, "VICTORY with a living enemy", where, turn);
    }
    if (combat.state == CombatState::DEFEAT && player.hp != 0) fail(failure, "DEFEAT with HP left", where, turn);
    if (player.orbs.size() > player.orb_slots) fail(failure, "more orbs than slots", where, turn);
}

// Replaces the draw pile with DRAW_PILE_SIZE cards of the hero's reward pool, half of them upgraded.
void fill_draw_pile(CombatEngine& combat, CharacterId hero, Rng& rng) {
    const auto pool = RunManager::card_reward_pool(hero);
    combat.draw_pile.clear();
    for (int i = 0; i < DRAW_PILE_SIZE; ++i) {
        CardId id = pool[rng.below(pool.size())];
        combat.draw_pile.push_back(DeckCard(id, i % 2 == 0));
    }
    // Upgrades alternate along the pile as built, so shuffle to spread them.
    for (size_t i = combat.draw_pile.size(); i > 1; --i) {
        std::swap(combat.draw_pile[i - 1], combat.draw_pile[rng.below(i)]);
    }
}

int random_living_enemy(const CombatEngine& combat, Rng& rng) {
    int living[3];
    int count = 0;
    for (size_t i = 0; i < combat.enemies.size(); ++i) {
        if (combat.enemies[i].is_alive()) living[count++] = static_cast<int>(i);
    }
    return count == 0 ? 0 : living[rng.below(static_cast<size_t>(count))];
}

// Plays randomly chosen playable cards until none is left, the fight is over, or a card ended the turn.
void play_hand(CombatEngine& combat, Rng& rng, Failure& failure) {
    while (combat.state == CombatState::PLAYER_TURN && !combat.end_turn_requested && !failure.failed) {
        int playable[MAX_HAND];
        int count = 0;
        for (size_t i = 0; i < combat.hand.size(); ++i) {
            if (combat.can_play_card(static_cast<int>(i))) playable[count++] = static_cast<int>(i);
        }
        if (count == 0) return;

        int index = playable[rng.below(static_cast<size_t>(count))];
        if (!combat.play_card(index, random_living_enemy(combat, rng))) {
            fail(failure, "play_card refused a playable card", "after can_play_card", combat.turn_number);
            return;
        }
        check_invariants(combat, failure, "after card");
    }
}

struct Outcome {
    Failure failure;
    CombatState state = CombatState::PLAYER_TURN;
    int turns = 0;
    int player_hp = 0;
    int cards_played = 0;
};

Outcome simulate(CharacterId hero, uint32_t seed) {
    Rng rng(seed);
    Outcome outcome;

    FixedVector<EnemyType, 3> enemies;
    const size_t enemy_count = 1 + rng.below(3);
    for (size_t i = 0; i < enemy_count; ++i) {
        enemies.push_back(static_cast<EnemyType>(rng.below(static_cast<size_t>(EnemyType::CEREMONIAL_BEAST) + 1)));
    }

    CombatEngine combat;
    combat.start_combat_multi(enemies, hero);
    fill_draw_pile(combat, hero, rng);
    check_invariants(combat, outcome.failure, "after start");

    for (int turn = 0; turn < MAX_TURNS && !outcome.failure.failed; ++turn) {
        if (combat.state != CombatState::PLAYER_TURN) break;
        play_hand(combat, rng, outcome.failure);
        if (outcome.failure.failed || combat.state != CombatState::PLAYER_TURN) break;

        combat.end_turn_requested = false;
        combat.end_player_turn();
        check_invariants(combat, outcome.failure, "after end turn");
    }

    outcome.state = combat.state;
    outcome.turns = combat.turn_number;
    outcome.player_hp = combat.player.hp;
    outcome.cards_played = combat.counters.combat_cards_played;
    return outcome;
}

bool same_outcome(const Outcome& a, const Outcome& b) {
    return a.failure.failed == b.failure.failed && a.state == b.state && a.turns == b.turns &&
           a.player_hp == b.player_hp && a.cards_played == b.cards_played;
}

} // namespace

int main() {
    const auto start = std::chrono::steady_clock::now();
    int fights = 0;
    int victories = 0;
    int defeats = 0;
    int unfinished = 0;
    int failures = 0;

    for (int h = 0; h < static_cast<int>(CharacterId::COUNT); ++h) {
        const CharacterId hero = static_cast<CharacterId>(h);
        for (uint32_t seed = 1; seed <= SEEDS_PER_HERO; ++seed) {
            if (EXCLUDED_SEEDS.count({h, seed})) continue;

            const Outcome outcome = simulate(hero, seed);
            ++fights;
            if (outcome.failure.failed) {
                ++failures;
                std::cout << "[FAIL] " << hero_name(hero) << " seed " << seed << ": " << outcome.failure.message << std::endl;
                continue;
            }

            // Same seed, same fight.
            const Outcome again = simulate(hero, seed);
            if (!same_outcome(outcome, again)) {
                ++failures;
                std::cout << "[FAIL] " << hero_name(hero) << " seed " << seed << ": not reproducible" << std::endl;
                continue;
            }

            if (outcome.state == CombatState::VICTORY) ++victories;
            else if (outcome.state == CombatState::DEFEAT) ++defeats;
            else ++unfinished;
        }
    }

    const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "[INFO] " << fights << " fights: " << victories << " victories, " << defeats << " defeats, "
              << unfinished << " unfinished after " << MAX_TURNS << " turns (" << elapsed << " s)" << std::endl;

    if (failures > 0) {
        std::cout << "[FAIL] " << failures << " fight(s) broke an invariant" << std::endl;
        return EXIT_FAILURE;
    }
    if (elapsed >= TIME_BUDGET_SECONDS) {
        std::cout << "[FAIL] simulation took " << elapsed << " s, budget is " << TIME_BUDGET_SECONDS << " s" << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "[PASS] Combat simulation: invariants hold over " << fights << " seeded fights" << std::endl;
    return EXIT_SUCCESS;
}
