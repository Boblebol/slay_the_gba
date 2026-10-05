// Relics that act outside the fights: the hooks of RunManager, the campfire actions a relic gives and the
// counters kept from room to room.
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include "engine/run.h"
#include "engine/save_system.h"

using namespace engine;

namespace {

class MemoryStorage final : public SaveStorage {
public:
    std::vector<uint8_t> bytes = std::vector<uint8_t>(2 * SAVE_BANK_SIZE, 0xFF);

    void read(void* destination, size_t size, size_t offset) override {
        std::memcpy(destination, bytes.data() + offset, size);
    }
    void write(const void* source, size_t size, size_t offset) override {
        std::memcpy(bytes.data() + offset, source, size);
    }
};

RunManager make_run(uint32_t seed = 2024) {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
    return run;
}

// Enters the first map room after forcing its type, like a player walking into it.
void enter_room_of_type(RunManager& run, RoomType type) {
    int index = type == RoomType::BOSS ? run.map.boss_node_index : 0;
    run.map.nodes[index].type = type;
    run.enter_room(index);
}

void test_room_hook() {
    RunManager run = make_run();
    assert(run.hook_count(RunHook::ROOM_ENTERED) == 0);
    int rooms = 0;
    for (RoomType type : { RoomType::MONSTER, RoomType::REST_SITE, RoomType::SHOP, RoomType::TREASURE, RoomType::EVENT,
                           RoomType::ELITE }) {
        enter_room_of_type(run, type);
        assert(run.hook_count(RunHook::ROOM_ENTERED) == ++rooms);
    }
    // A new run starts counting again.
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 1);
    assert(run.hook_count(RunHook::ROOM_ENTERED) == 0);
    std::cout << "[PASS] Hook: entering a room, whatever its type" << std::endl;
}

void test_rest_hook_and_campfire_actions() {
    RunManager run = make_run();
    enter_room_of_type(run, RoomType::REST_SITE);
    assert(run.phase == RunPhase::CAMPFIRE && run.hook_count(RunHook::RESTED) == 0);
    run.hp = 10;
    run.rest_at_campfire();
    assert(run.hook_count(RunHook::RESTED) == 1 && run.hp == 10 + run.max_hp * 3 / 10);
    // Smithing is not resting.
    enter_room_of_type(run, RoomType::REST_SITE);
    assert(run.smith_at_campfire(0) && run.hook_count(RunHook::RESTED) == 1);

    // Rest and Smith by default; a relic adds its action, in the order of the menu.
    FixedVector<CampfireAction, 5> base = run.campfire_actions();
    assert(base.size() == 2 && base[0] == CampfireAction::REST && base[1] == CampfireAction::SMITH);
    run.add_relic("MEAT_CLEAVER");
    run.add_relic("ANCHOR"); // a relic with no campfire action
    FixedVector<CampfireAction, 5> cook = run.campfire_actions();
    assert(cook.size() == 3 && cook[2] == CampfireAction::COOK);
    run.add_relic("SHOVEL");
    run.add_relic("GIRYA");
    FixedVector<CampfireAction, 5> all = run.campfire_actions();
    assert(all.size() == 5 && all[2] == CampfireAction::DIG && all[3] == CampfireAction::LIFT &&
           all[4] == CampfireAction::COOK);
    std::cout << "[PASS] Hook: resting; campfire actions given by Shovel, Girya and Meat Cleaver" << std::endl;
}

void test_shop_hook() {
    RunManager run = make_run();
    run.open_shop();
    assert(run.hook_count(RunHook::SHOP_OPENED) == 1);
    run.leave_shop();
    enter_room_of_type(run, RoomType::SHOP);
    assert(run.hook_count(RunHook::SHOP_OPENED) == 2 && run.phase == RunPhase::SHOP);

    // The hook runs once the shelves are filled: a discount applies to every price.
    ShopInventory shop = run.current_shop;
    run.current_shop.apply_discount(20);
    assert(run.current_shop.card_removal_cost == 60);
    for (size_t i = 0; i < shop.relics.size(); ++i) {
        assert(run.current_shop.relics[i].cost == (shop.relics[i].cost * 80 + 50) / 100);
    }
    std::cout << "[PASS] Hook: the merchant's stock is drawn, then relics change its prices" << std::endl;
}

void test_combat_and_reward_hooks() {
    RunManager run = make_run();
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.hook_count(RunHook::COMBAT_WON) == 0 && run.hook_count(RunHook::REWARDS_GENERATED) == 0);
    run.proceed_after_combat();
    assert(run.phase == RunPhase::REWARD_SCREEN);
    assert(run.hook_count(RunHook::COMBAT_WON) == 1 && run.hook_count(RunHook::REWARDS_GENERATED) == 1);

    // The gold of the fight is gained when the rewards are left, and the card picked joins the deck.
    int16_t gold = run.gold;
    int16_t reward = run.gold_reward;
    size_t deck = run.master_deck.size();
    assert(reward > 0 && run.hook_count(RunHook::GOLD_GAINED) == 0 && run.hook_count(RunHook::CARD_ADDED) == 0);
    run.pick_card_reward(0);
    assert(run.gold == gold + reward && run.hook_count(RunHook::GOLD_GAINED) == 1);
    assert(run.master_deck.size() == deck + 1 && run.hook_count(RunHook::CARD_ADDED) == 1);

    // Skipping the cards still pays the gold, and adds nothing to the deck.
    enter_room_of_type(run, RoomType::ELITE);
    run.proceed_after_combat();
    run.skip_card_rewards();
    assert(run.hook_count(RunHook::COMBAT_WON) == 2 && run.hook_count(RunHook::REWARDS_GENERATED) == 2);
    assert(run.hook_count(RunHook::GOLD_GAINED) == 2 && run.hook_count(RunHook::CARD_ADDED) == 1);
    std::cout << "[PASS] Hook: fight won, rewards drawn" << std::endl;
}

void test_card_added_hook() {
    // Every way a card joins the deck: the reward (above), the merchant, a relic or an event.
    RunManager run = make_run();
    run.gold = 999;
    run.open_shop();
    assert(run.buy_shop_card(0) && run.hook_count(RunHook::CARD_ADDED) == 1);
    assert(run.add_card_to_deck(CardId::BLUDGEON) && run.hook_count(RunHook::CARD_ADDED) == 2);
    assert(run.master_deck.back().id == CardId::BLUDGEON);

    // A card the full deck refuses is not announced. Transforming or upgrading a card adds none.
    while (run.master_deck.size() < run.master_deck.capacity()) run.master_deck.push_back(CardId::STRIKE);
    assert(!run.add_card_to_deck(CardId::BLUDGEON) && run.hook_count(RunHook::CARD_ADDED) == 2);
    assert(run.transform_card_in_deck(0) && run.upgrade_card_in_deck(1));
    assert(run.hook_count(RunHook::CARD_ADDED) == 2);
    std::cout << "[PASS] Hook: a card added to the deck by the merchant or a relic, not by a full deck" << std::endl;
}

void test_gold_gained_hook() {
    RunManager run = make_run();
    int16_t gold = run.gold;
    // A chest gives 50 gold.
    enter_room_of_type(run, RoomType::TREASURE);
    assert(run.gold == gold + 50 && run.hook_count(RunHook::GOLD_GAINED) == 1);
    run.gain_gold(10);
    assert(run.hook_count(RunHook::GOLD_GAINED) == 2);
    // Nothing gained, nothing announced; spending gold is not gaining it.
    run.gain_gold(0);
    run.gold = 999;
    run.open_shop();
    assert(run.buy_shop_card(0));
    assert(run.hook_count(RunHook::GOLD_GAINED) == 2);
    std::cout << "[PASS] Hook: gold gained from a chest or an effect, never when it is spent" << std::endl;
}

void test_relic_counters() {
    RunManager run = make_run();
    assert(run.relic_counter("GIRYA") == 0);

    // "3 times max": a plain number.
    assert(run.set_relic_counter("GIRYA", 2) && run.relic_counter("GIRYA") == 2);
    assert(run.set_relic_counter("GIRYA", 3) && run.relic_counter("GIRYA") == 3);
    assert(run.relic_counters.size() == 1);

    // "Every 3 combats": true on the third, sixth... call.
    for (int round = 0; round < 3; ++round) {
        assert(!run.tick_relic_counter("FISHING_ROD", 3) && run.relic_counter("FISHING_ROD") == 1);
        assert(!run.tick_relic_counter("FISHING_ROD", 3) && run.relic_counter("FISHING_ROD") == 2);
        assert(run.tick_relic_counter("FISHING_ROD", 3) && run.relic_counter("FISHING_ROD") == 0);
    }
    assert(run.tick_relic_counter("LASTING_CANDY", 1));
    assert(run.relic_counter("GIRYA") == 3); // each relic has its own

    // Saved with the run, and read back.
    run.tick_relic_counter("FISHING_ROD", 3);
    MemoryStorage storage;
    SaveManager& store = SaveManager::instance();
    store.reset_to_defaults();
    store.set_storage(&storage);
    store.autosave_run(run);
    store.reset_to_defaults();
    assert(store.load_from_storage());
    RunManager loaded;
    assert(store.restore_run(loaded));
    assert(loaded.relic_counters.size() == 3);
    assert(loaded.relic_counter("GIRYA") == 3 && loaded.relic_counter("FISHING_ROD") == 1 &&
           loaded.relic_counter("LASTING_CANDY") == 0);
    assert(!loaded.tick_relic_counter("FISHING_ROD", 3) && loaded.tick_relic_counter("FISHING_ROD", 3));
    store.set_storage(nullptr);
    store.reset_to_defaults();

    // No more counters than the save can hold, and a new run starts without any.
    RunManager full = make_run();
    std::string_view ids[] = { "ANCHOR", "VAJRA", "BREAD", "KUNAI", "GORGET", "AKABEKO", "LANTERN", "PENDULUM", "SHURIKEN" };
    for (size_t i = 0; i < MAX_RELIC_COUNTERS; ++i) assert(full.set_relic_counter(ids[i], 1));
    assert(!full.set_relic_counter(ids[MAX_RELIC_COUNTERS], 1) && full.relic_counter(ids[MAX_RELIC_COUNTERS]) == 0);
    assert(!full.tick_relic_counter(ids[MAX_RELIC_COUNTERS], 1));
    assert(full.set_relic_counter(ids[0], 9) && full.relic_counter(ids[0]) == 9);
    full.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 3);
    assert(full.relic_counters.empty());
    std::cout << "[PASS] Relic counters: plain number, every N, one per relic, saved with the run" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Relics Outside Combat ===" << std::endl;
    test_room_hook();
    test_rest_hook_and_campfire_actions();
    test_shop_hook();
    test_combat_and_reward_hooks();
    test_card_added_hook();
    test_gold_gained_hook();
    test_relic_counters();
    std::cout << "All out-of-combat relic tests passed." << std::endl;
    return 0;
}
