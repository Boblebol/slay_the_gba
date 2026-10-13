// Ancient met between the acts (#261): which Ancient comes, what it offers, the relic taken, and the save.
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include "engine/ancient.h"
#include "engine/run.h"
#include "engine/save_system.h"

using namespace engine;

namespace {

class MemoryStorage final : public SaveStorage {
public:
    std::vector<uint8_t> bytes = std::vector<uint8_t>(32 * 1024, 0xFF);

    void read(void* destination, size_t size, size_t offset) override {
        std::memcpy(destination, bytes.data() + offset, size);
    }

    void write(const void* source, size_t size, size_t offset) override {
        std::memcpy(bytes.data() + offset, source, size);
    }
};

RunManager new_run(uint32_t seed = 777) {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
    return run;
}

bool in_act(AncientId id, uint8_t act) {
    return (get_ancient(id).act_mask & (1u << (act - 1))) != 0;
}

// Option index of `relic_id` in `slot` of the Ancient.
uint8_t option_of(AncientId id, size_t slot, std::string_view relic_id) {
    const AncientSlot& group = get_ancient(id).slots[slot];
    for (size_t i = 0; i < group.options.size(); ++i) {
        if (group.options[i].id == relic_id) return static_cast<uint8_t>(i);
    }
    assert(false);
    return 0;
}

bool pending(std::string_view relic_id) {
    for (std::string_view id : { "MASSIVE_SCROLL", "SCROLL_BOXES", "SEA_GLASS",
                                 "TOY_BOX" }) {
        if (id == relic_id) return true;
    }
    return false;
}

void test_ancient_of_the_act() {
    // Acts 2 and 3 draw among their own Ancients, Darv at both; Neow only ever opens act 1.
    for (uint8_t act : { 2, 3 }) {
        bool seen[static_cast<size_t>(AncientId::COUNT)] = {};
        for (uint32_t seed = 1; seed <= 400; ++seed) {
            AncientId id = RunManager::draw_ancient(act, seed * 2654435761u);
            assert(id < AncientId::COUNT && in_act(id, act) && id != AncientId::NEOW);
            seen[static_cast<size_t>(id)] = true;
        }
        for (const Ancient& ancient : get_all_ancients()) {
            assert(seen[static_cast<size_t>(ancient.id)] == in_act(ancient.id, act));
        }
        assert(seen[static_cast<size_t>(AncientId::DARV)]);
    }
    for (uint32_t seed = 1; seed <= 50; ++seed) {
        AncientId id = RunManager::draw_ancient(1, seed);
        assert(id == AncientId::NEOW || id == AncientId::DARV);
    }
    assert(RunManager::draw_ancient(0, 5) == AncientId::COUNT && RunManager::draw_ancient(4, 5) == AncientId::COUNT);

    // The next act opens on an Ancient of that act, the same one for the same run.
    for (uint32_t seed : { 1u, 777u, 4242u }) {
        RunManager run = new_run(seed);
        const uint32_t before = run.rng_state();
        run.advance_act();
        assert(run.phase == RunPhase::ANCIENT_BLESSING && run.current_act == 2);
        assert(run.current_ancient < AncientId::COUNT && in_act(run.current_ancient, 2));
        assert(!run.ancient_offers.empty());
        // The Ancient has a stream of its own: the run's generator only gave the seed of the new map.
        assert(run.map_seed == run.rng_state() && run.rng_state() != before);
        RunManager again = new_run(seed);
        again.advance_act();
        assert(again.current_ancient == run.current_ancient && again.ancient_offers.size() == run.ancient_offers.size());
        for (size_t i = 0; i < run.ancient_offers.size(); ++i) {
            assert(again.ancient_offer(static_cast<int>(i)).id == run.ancient_offer(static_cast<int>(i)).id);
        }
        run.choose_ancient_relic(0);
        run.phase = RunPhase::MAP_NAVIGATION;
        run.deck_choice = {};
        run.advance_act();
        assert(run.phase == RunPhase::ANCIENT_BLESSING && run.current_act == 3 && in_act(run.current_ancient, 3));
    }
    std::cout << "[PASS] Ancient: drawn among the Ancients of the act, Darv at every act" << std::endl;
}

void test_offer_follows_the_slots() {
    // Each slot offers as many relics as it says, never twice the same, never one the engine cannot resolve.
    for (const Ancient& ancient : get_all_ancients()) {
        for (uint32_t seed = 1; seed <= 30; ++seed) {
            RunManager run = new_run(seed);
            run.meet_ancient(ancient.id);
            assert(run.phase == RunPhase::ANCIENT_BLESSING && run.current_ancient == ancient.id);
            size_t expected = 0;
            for (size_t slot = 0; slot < ancient.slots.size(); ++slot) {
                size_t resolvable = 0;
                for (const AncientRelic& option : ancient.slots[slot].options) resolvable += pending(option.id) ? 0 : 1;
                const size_t wanted = std::min<size_t>(ancient.slots[slot].picks_allowed, resolvable);
                size_t got = 0;
                for (const AncientOffer& offer : run.ancient_offers) got += offer.slot == slot ? 1 : 0;
                assert(got == wanted);
                expected += wanted;
            }
            assert(run.ancient_offers.size() == expected);
            for (size_t i = 0; i < run.ancient_offers.size(); ++i) {
                const AncientRelic& relic = run.ancient_offer(static_cast<int>(i));
                assert(!pending(relic.id) && !run.has_relic(relic.id));
                for (size_t j = i + 1; j < run.ancient_offers.size(); ++j) {
                    assert(run.ancient_offer(static_cast<int>(j)).id != relic.id);
                }
            }
        }
    }
    // The usual offer: 2 blessings and 1 curse for Neow, 3 relics for Darv, 1 per slot for Pael.
    RunManager neow = new_run();
    neow.meet_ancient(AncientId::NEOW);
    assert(neow.ancient_offers.size() == 3 && neow.ancient_offers[0].slot == 0 && neow.ancient_offers[1].slot == 0 &&
           neow.ancient_offers[2].slot == 1);
    RunManager darv = new_run();
    darv.meet_ancient(AncientId::DARV);
    assert(darv.ancient_offers.size() == 3);
    RunManager pael = new_run();
    pael.meet_ancient(AncientId::PAEL);
    assert(pael.ancient_offers.size() == 3);
    for (uint8_t i = 0; i < 3; ++i) assert(pael.ancient_offers[i].slot == i);

    // A relic already owned is never offered: with all of Darv's but three, those three are the offer.
    RunManager owned = new_run();
    const Ancient& ancient = get_ancient(AncientId::DARV);
    for (size_t i = 3; i < ancient.slots[0].options.size(); ++i) owned.relics.push_back(ancient.slots[0].options[i].id);
    owned.meet_ancient(AncientId::DARV);
    assert(owned.ancient_offers.size() == 3);
    for (const AncientOffer& offer : owned.ancient_offers) assert(offer.slot == 0 && offer.option < 3);
    std::cout << "[PASS] Ancient: each slot offers its number of relics, none owned or unresolved" << std::endl;
}

void test_choosing_the_relic() {
    // The relic chosen is obtained and the run is back on the map.
    RunManager run = new_run();
    run.meet_ancient(AncientId::DARV);
    const std::string_view chosen = run.ancient_offer(1).id;
    const size_t relics = run.relics.size();
    assert(!run.choose_ancient_relic(-1) && !run.choose_ancient_relic(3));
    assert(!run.leave_ancient()); // a relic has to be taken
    assert(run.choose_ancient_relic(1));
    assert(run.has_relic(chosen) && run.relics.size() >= relics + 1);
    assert(run.current_ancient == AncientId::COUNT && run.ancient_offers.empty());
    assert(run.phase == RunPhase::MAP_NAVIGATION || run.phase == RunPhase::REWARD_SCREEN);
    assert(!run.choose_ancient_relic(0)); // only one relic

    // Its effect on pickup still runs: a card reward to take, or cards of the deck to choose.
    RunManager coffer = new_run();
    coffer.meet_ancient(AncientId::NEOW);
    coffer.ancient_offers[0] = AncientOffer{ 0, option_of(AncientId::NEOW, 0, "LOST_COFFER") };
    assert(coffer.choose_ancient_relic(0));
    assert(coffer.has_relic("LOST_COFFER") && coffer.phase == RunPhase::REWARD_SCREEN);
    assert(!coffer.current_card_rewards.empty());

    RunManager scissors = new_run();
    scissors.meet_ancient(AncientId::NEOW);
    scissors.ancient_offers[2] = AncientOffer{ 0, option_of(AncientId::NEOW, 0, "PRECISE_SCISSORS") };
    assert(scissors.choose_ancient_relic(2));
    assert(scissors.phase == RunPhase::MAP_NAVIGATION && scissors.has_deck_choice());
    assert(scissors.deck_choice.kind == DeckChoiceKind::REMOVE);

    RunManager oyster = new_run();
    oyster.meet_ancient(AncientId::NEOW);
    oyster.ancient_offers[0] = AncientOffer{ 0, option_of(AncientId::NEOW, 0, "NUTRITIOUS_OYSTER") };
    const int16_t max_hp = oyster.max_hp;
    assert(oyster.choose_ancient_relic(0));
    assert(oyster.max_hp == max_hp + 11 && oyster.phase == RunPhase::MAP_NAVIGATION);

    // With no room for one more relic, the Ancient has nothing to give and can be left.
    RunManager full = new_run();
    while (full.relics.size() < full.relics.capacity()) full.relics.push_back("ANCHOR");
    full.meet_ancient(AncientId::TANX);
    assert(full.phase == RunPhase::ANCIENT_BLESSING && full.ancient_offers.empty());
    assert(full.leave_ancient());
    assert(full.phase == RunPhase::MAP_NAVIGATION && full.current_ancient == AncientId::COUNT);
    assert(!full.leave_ancient());
    std::cout << "[PASS] Ancient: one relic taken, its pickup effect resolved" << std::endl;
}

void test_saved_with_the_run() {
    MemoryStorage storage;
    SaveManager& store = SaveManager::instance();
    store.reset_to_defaults();
    store.set_storage(&storage);

    // Saved before the relic is taken: the same Ancient offers the same relics after a reload.
    RunManager run = new_run(4242);
    run.advance_act();
    store.autosave_run(run);
    store.reset_to_defaults();
    assert(store.load_from_storage());
    RunManager loaded;
    assert(store.restore_run(loaded));
    assert(loaded.phase == RunPhase::ANCIENT_BLESSING && loaded.current_act == 2);
    assert(loaded.current_ancient == run.current_ancient);
    assert(loaded.ancient_offers.size() == run.ancient_offers.size());
    for (size_t i = 0; i < run.ancient_offers.size(); ++i) {
        assert(loaded.ancient_offer(static_cast<int>(i)).id == run.ancient_offer(static_cast<int>(i)).id);
    }
    const std::string_view chosen = loaded.ancient_offer(0).id;
    assert(loaded.choose_ancient_relic(0) && loaded.has_relic(chosen));

    // Once it is taken, the run resumes on the map.
    loaded.phase = RunPhase::MAP_NAVIGATION;
    loaded.deck_choice = {};
    store.autosave_run(loaded);
    store.reset_to_defaults();
    assert(store.load_from_storage());
    RunManager after;
    assert(store.restore_run(after));
    assert(after.phase == RunPhase::MAP_NAVIGATION && after.current_ancient == AncientId::COUNT);
    assert(after.ancient_offers.empty() && after.has_relic(chosen));

    // A save written before the Ancient existed (its bytes at 0) never resumes on one.
    store.autosave_run(run);
    store.get_active_profile().active_run.ancient = 0;
    RunManager older;
    assert(store.restore_run(older));
    assert(older.phase == RunPhase::MAP_NAVIGATION && older.current_ancient == AncientId::COUNT);

    store.set_storage(nullptr);
    store.reset_to_defaults();
    std::cout << "[PASS] Ancient: the phase and the offer are kept in the save" << std::endl;
}

} // namespace

int main() {
    test_ancient_of_the_act();
    test_offer_follows_the_slots();
    test_choosing_the_relic();
    test_saved_with_the_run();
    return 0;
}
