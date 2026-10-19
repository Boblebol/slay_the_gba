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

void test_spoils_map() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::SPOILS_MAP, up) == COST_X);
        assert(CardDef::get(CardId::SPOILS_MAP).type == CardType::QUEST);
        assert(CardDef::has_keyword(CardId::SPOILS_MAP, up, KEYWORD_UNPLAYABLE));
        assert(SPOILS_MAP_GOLD == 600);

        // Unplayable.
        CombatEngine combat = make_fight();
        combat.hand.push_back(DeckCard(CardId::SPOILS_MAP, up));
        assert(!combat.can_play_card(0));
        assert(!combat.play_card(0, 0));
        assert(combat.player.energy == 3 && combat.hand.size() == 1);

        // Nothing is marked in the act where the map is found.
        RunManager run;
        run.start_run(CharacterId::IRONCLAD);
        const size_t size = run.master_deck.size();
        run.master_deck.push_back(DeckCard(CardId::SPOILS_MAP, up));
        assert(run.spoils_map_node() == -1);

        // The next act marks one of its rooms (never the boss), the same for the same map.
        run.advance_act();
        const int site = run.spoils_map_node();
        assert(site >= 0 && site < static_cast<int>(run.map.nodes.size()) && site != run.map.boss_node_index);
        RunManager again;
        again.start_run(CharacterId::IRONCLAD);
        again.master_deck.push_back(DeckCard(CardId::SPOILS_MAP, up));
        again.advance_act();
        assert(again.map_seed == run.map_seed && again.spoils_map_node() == site);

        // Another room gives nothing; the marked one gives 600 Gold, and the card leaves the deck.
        const int other = site == 0 ? 1 : 0;
        run.map.nodes[other].type = RoomType::REST_SITE;
        int16_t gold = run.gold;
        run.enter_room(other);
        assert(run.gold == gold && run.master_deck.size() == size + 1);
        run.map.nodes[site].type = RoomType::REST_SITE;
        run.enter_room(site);
        assert(run.gold == gold + 600);
        assert(run.master_deck.size() == size && run.spoils_map_node() == -1);
    }
    std::cout << "[PASS] Spoils Map: X cost, Quest, Unplayable, 600 Gold in a room of the next act" << std::endl;
}

void test_byrdonis_egg() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::BYRDONIS_EGG, up) == COST_X);
        assert(CardDef::get(CardId::BYRDONIS_EGG).type == CardType::QUEST);
        assert(CardDef::has_keyword(CardId::BYRDONIS_EGG, up, KEYWORD_UNPLAYABLE));

        // Unplayable.
        CombatEngine combat = make_fight();
        combat.hand.push_back(DeckCard(CardId::BYRDONIS_EGG, up));
        assert(!combat.can_play_card(0));
        assert(!combat.play_card(0, 0));
        assert(combat.player.energy == 3 && combat.hand.size() == 1);

        // Without an egg, the Rest Site has no Hatch action and nothing hatches.
        RunManager run;
        run.start_run(CharacterId::IRONCLAD);
        enter(run, RoomType::REST_SITE);
        const auto has_hatch = [&run] {
            for (CampfireAction action : run.campfire_actions()) {
                if (action == CampfireAction::HATCH) return true;
            }
            return false;
        };
        assert(!has_hatch() && !run.hatch_at_campfire());

        // With one, the Rest Site offers to hatch it: the egg leaves the deck for the Byrdpip, and the site is used up.
        const size_t size = run.master_deck.size();
        run.master_deck.push_back(DeckCard(CardId::BYRDONIS_EGG, up));
        assert(has_hatch() && !run.has_relic("BYRDPIP"));
        assert(run.hatch_at_campfire());
        // The Byrdpip comes with its Byrd Swoop card.
        assert(run.master_deck.size() == size + 1 && run.master_deck.back().id == CardId::BYRD_SWOOP && run.has_relic("BYRDPIP"));
        for (const DeckCard& card : run.master_deck) assert(card.id != CardId::BYRDONIS_EGG);
        assert(run.phase == RunPhase::MAP_NAVIGATION && !has_hatch());
    }
    std::cout << "[PASS] Byrdonis Egg: X cost, Quest, Unplayable, hatches into the Byrdpip at a Rest Site" << std::endl;
}

void test_lantern_key() {
    for (bool up : { false, true }) {
        assert(CardDef::get_cost(CardId::LANTERN_KEY, up) == COST_X);
        assert(CardDef::get(CardId::LANTERN_KEY).type == CardType::QUEST);
        assert(CardDef::has_keyword(CardId::LANTERN_KEY, up, KEYWORD_UNPLAYABLE));

        // Unplayable.
        CombatEngine combat = make_fight();
        combat.hand.push_back(DeckCard(CardId::LANTERN_KEY, up));
        assert(!combat.can_play_card(0));
        assert(!combat.play_card(0, 0));
        assert(combat.player.energy == 3 && combat.hand.size() == 1);

        // In the act where it is found, a ? room is an ordinary event and the key stays.
        RunManager run;
        run.start_run(CharacterId::IRONCLAD);
        const size_t size = run.master_deck.size();
        run.master_deck.push_back(DeckCard(CardId::LANTERN_KEY, up));
        enter(run, RoomType::EVENT);
        assert(run.current_event.id != "THE_LANTERN_KEY" && run.master_deck.size() == size + 1);

        // In the next act, the first ? room is the special event, and the key is used up.
        run.advance_act();
        assert(run.master_deck.size() == size + 1 && run.master_deck.back().id == CardId::LANTERN_KEY);
        enter(run, RoomType::EVENT);
        assert(run.current_event.id == "THE_LANTERN_KEY" && run.phase == RunPhase::EVENT);
        assert(run.master_deck.size() == size);
        for (const DeckCard& card : run.master_deck) assert(card.id != CardId::LANTERN_KEY);
        const int16_t gold = run.gold;
        char msg[128];
        assert(run.choose_event_choice(0, msg, sizeof(msg)));
        assert(run.gold == gold + LANTERN_KEY_GOLD);

        // The next ? room is ordinary again.
        enter(run, RoomType::EVENT);
        assert(run.current_event.id != "THE_LANTERN_KEY");
    }
    std::cout << "[PASS] Lantern Key: X cost, Quest, Unplayable, unlocks its special event in the next act" << std::endl;
}

int main() {
    test_dowsing();
    test_spoils_map();
    test_byrdonis_egg();
    test_lantern_key();
    std::cout << "All quest card tests passed." << std::endl;
    return 0;
}
