// Quest cards (Neow's relics, events). One test per card, with the values of bible/data/card_v0.111.0.json.
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

// An Ironclad against one Jaw Worm, empty piles and 3 energy.
CombatEngine make_fight() {
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.player.energy = 3;
    return combat;
}

// Enters the first room of the map as a room of `type`.
void enter(RunManager& run, RoomType type) {
    run.map.nodes[0].type = type;
    run.enter_room(0);
}

} // namespace

void test_dowsing() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::DOWSING, up) == COST_X);
        assert(CardDef::get(CardId::DOWSING).type == CardType::QUEST);
        assert(CardDef::has_keyword(CardId::DOWSING, up, KEYWORD_UNPLAYABLE));
        assert(CardDef::get_value(CardId::DOWSING, up) == 5);

        // Unplayable.
        CombatEngine combat = make_fight();
        combat.hand.push_back(DeckCard(CardId::DOWSING, up));
        assert(!combat.can_play_card(0));
        assert(!combat.play_card(0, 0));
        assert(combat.player.energy == 3 && combat.hand.size() == 1);

        // After 5 ? rooms, it becomes an Abundance (upgraded as it was); other rooms do not count.
        RunManager run;
        run.start_run(CharacterId::IRONCLAD);
        const size_t size = run.master_deck.size();
        run.master_deck.push_back(DeckCard(CardId::DOWSING, up));
        enter(run, RoomType::REST_SITE);
        enter(run, RoomType::SHOP);
        assert(run.master_deck.back().id == CardId::DOWSING && run.master_deck.back().bonus == 0);
        for (int room = 1; room <= 5; ++room) {
            enter(run, RoomType::EVENT);
            assert(run.master_deck.size() == size + 1);
            assert(run.master_deck.back().id == (room < 5 ? CardId::DOWSING : CardId::ABUNDANCE));
        }
        assert(run.master_deck.back().upgraded == up && run.master_deck.back().bonus == 0);
    }
    std::cout << "[PASS] Dowsing: X cost, Quest, Unplayable, an Abundance after 5 ? rooms, same upgraded" << std::endl;
}

int main() {
    test_dowsing();
    std::cout << "All quest card tests passed." << std::endl;
    return 0;
}
