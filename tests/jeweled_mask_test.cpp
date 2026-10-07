// Jeweled Mask (ancient): a random Power of the draw pile goes to the hand at the start of the combat, free.
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

// Enters a fight with a deck of one Inflame (a Power that costs 1) and strikes.
void enter_fight(RunManager& run, uint32_t seed, bool with_mask) {
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
    if (with_mask) run.add_relic("JEWELED_MASK");
    run.master_deck.clear();
    for (int n = 0; n < 16; ++n) {
        // The shuffle does not depend on the seed: the Power moves along the deck instead.
        run.master_deck.push_back(DeckCard(n == static_cast<int>(seed % 16) ? CardId::INFLAME : CardId::STRIKE));
    }
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
}

template <typename Pile>
const DeckCard* find_power(const Pile& pile) {
    for (const DeckCard& card : pile) {
        if (card.id == CardId::INFLAME) return &card;
    }
    return nullptr;
}

void test_jeweled_mask() {
    int picked = 0;
    for (uint32_t seed = 1; seed <= 20; ++seed) {
        RunManager without;
        enter_fight(without, seed, false);
        const DeckCard* plain = find_power(without.combat.hand);
        if (plain) assert(without.combat.effective_cost(*plain) == 1);

        RunManager with;
        enter_fight(with, seed, true);
        const DeckCard* power = find_power(with.combat.hand);
        // The only Power is in the hand either way: drawn, or picked from the draw pile and free.
        assert(power != nullptr);
        if (!plain) {
            assert(with.combat.effective_cost(*power) == 0);
            assert(find_power(with.combat.draw_pile) == nullptr);
            ++picked;
        }
    }
    assert(picked > 0);
}

}  // namespace

int main() {
    test_jeweled_mask();
    std::cout << "jeweled_mask_test: ok\n";
    return 0;
}
