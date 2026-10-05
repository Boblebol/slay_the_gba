// What a relic does when it is obtained: the hook of RunManager::add_relic and each effect it can call.
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include "engine/potion.h"
#include "engine/relic.h"
#include "engine/rewards.h"
#include "engine/run.h"
#include "engine/save_system.h"
#include "engine/shop.h"

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

RunManager make_run(CharacterId hero = CharacterId::IRONCLAD, uint32_t seed = 2024) {
    RunManager run;
    run.start_run(hero, 0, PaletteVariant::CLASSIC, seed);
    return run;
}

int count_upgraded(const RunManager& run) {
    int count = 0;
    for (const DeckCard& card : run.master_deck) count += card.upgraded;
    return count;
}

int count_card(const RunManager& run, CardId id) {
    int count = 0;
    for (const DeckCard& card : run.master_deck) count += card.id == id;
    return count;
}

// The hook runs wherever a relic comes from: chest, fight reward, merchant, event.
void test_pickup_hook() {
    RunManager reward = make_run();
    int16_t max_hp = reward.max_hp;
    reward.phase = RunPhase::REWARD_SCREEN;
    reward.relic_reward = "STRAWBERRY";
    assert(reward.take_relic_reward());
    assert(reward.max_hp == max_hp + 7);
    // Already owned: refused, and nothing is paid twice.
    reward.relic_reward = "STRAWBERRY";
    assert(!reward.take_relic_reward() && reward.max_hp == max_hp + 7);

    RunManager shop = make_run();
    shop.gold = 999;
    shop.open_shop();
    shop.current_shop.relics[0] = ShopRelic{ "STRAWBERRY", 100, false };
    assert(shop.buy_shop_relic(0));
    assert(shop.max_hp == max_hp + 7 && shop.gold == 899);
    // Not enough gold: no relic, no effect.
    RunManager poor = make_run();
    poor.gold = 10;
    poor.open_shop();
    poor.current_shop.relics[0] = ShopRelic{ "STRAWBERRY", 100, false };
    assert(!poor.buy_shop_relic(0) && poor.max_hp == max_hp);

    // The starting relic and a restored run do not go through the hook.
    MemoryStorage storage;
    SaveManager& store = SaveManager::instance();
    store.reset_to_defaults();
    store.set_storage(&storage);
    store.autosave_run(reward);
    RunManager loaded;
    assert(store.restore_run(loaded));
    assert(loaded.has_relic("STRAWBERRY") && loaded.max_hp == max_hp + 7);
    store.set_storage(nullptr);
    store.reset_to_defaults();
    std::cout << "[PASS] Pickup hook: fight reward, merchant, once per relic, not replayed by a restored run" << std::endl;
}

void test_hp_and_gold_effects() {
    RunManager run = make_run();
    run.hp = 30;
    int16_t max_hp = run.max_hp;

    run.gain_max_hp(10);
    assert(run.max_hp == max_hp + 10 && run.hp == 40);
    run.gain_max_hp(0);
    run.gain_max_hp(-5);
    assert(run.max_hp == max_hp + 10 && run.hp == 40);

    run.heal(15);
    assert(run.hp == 55);
    run.heal(1000);
    assert(run.hp == run.max_hp);
    run.heal(-20);
    assert(run.hp == run.max_hp);

    int16_t gold = run.gold;
    run.gain_gold(300);
    assert(run.gold == gold + 300);
    run.gain_gold(-50);
    assert(run.gold == gold + 300);
    run.gain_gold(30000);
    run.gain_gold(30000);
    assert(run.gold == 9999);
    std::cout << "[PASS] Pickup effects: Max HP (current HP follow), heal capped at Max HP, gold capped" << std::endl;
}

void test_random_upgrades() {
    RunManager run = make_run();
    size_t deck = run.master_deck.size();
    assert(count_upgraded(run) == 0);
    assert(run.upgrade_random_cards(2) == 2 && count_upgraded(run) == 2);

    // By type: only Attacks (War Paint and Whetstone upgrade Skills and Attacks).
    RunManager attacks = make_run();
    assert(attacks.upgrade_random_cards(2, CardType::ATTACK) == 2);
    for (const DeckCard& card : attacks.master_deck) {
        assert(!card.upgraded || CardDef::get(card.id).type == CardType::ATTACK);
    }
    assert(attacks.upgrade_random_cards(3, CardType::POWER) == 0 && count_upgraded(attacks) == 2);

    // Never more than the cards left to upgrade, and never the same card twice.
    assert(run.upgrade_random_cards(100) == static_cast<int>(deck) - 2);
    assert(count_upgraded(run) == static_cast<int>(deck));
    assert(run.upgrade_random_cards(1) == 0);

    // Same seed, same cards.
    RunManager first = make_run(CharacterId::SILENT, 9);
    RunManager second = make_run(CharacterId::SILENT, 9);
    first.upgrade_random_cards(3);
    second.upgrade_random_cards(3);
    for (size_t i = 0; i < first.master_deck.size(); ++i) {
        assert(first.master_deck[i].upgraded == second.master_deck[i].upgraded);
    }
    std::cout << "[PASS] Pickup effects: random upgrades, by type, bounded by the deck, same seed same cards" << std::endl;
}

void test_cards_added_to_the_deck() {
    RunManager run = make_run();
    size_t deck = run.master_deck.size();
    assert(run.add_card_to_deck(DeckCard(CardId::BLUDGEON, true)));
    assert(run.master_deck.size() == deck + 1);
    assert(run.master_deck.back().id == CardId::BLUDGEON && run.master_deck.back().upgraded);
    // A card that weighs on the deck is added the same way: Unplayable, drawn in the next fight.
    assert(run.add_card_to_deck(CardId::WOUND));
    assert(count_card(run, CardId::WOUND) == 1);

    while (run.master_deck.size() < run.master_deck.capacity()) assert(run.add_card_to_deck(CardId::STRIKE));
    assert(!run.add_card_to_deck(CardId::STRIKE));
    assert(run.master_deck.size() == run.master_deck.capacity());
    std::cout << "[PASS] Pickup effects: a card or a dead weight added to the deck, refused when the deck is full" << std::endl;
}

void test_transform() {
    for (CharacterId hero : { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT, CharacterId::REGENT,
                              CharacterId::NECROBINDER }) {
        std::span<const CardId> pool = RunManager::card_reward_pool(hero);
        for (uint32_t seed = 1; seed < 40; ++seed) {
            RunManager run = make_run(hero, seed);
            size_t deck = run.master_deck.size();
            int index = static_cast<int>(seed % deck);
            run.master_deck[index].upgraded = true;
            CardId before = run.master_deck[index].id;
            assert(run.transform_card_in_deck(index));
            const DeckCard& after = run.master_deck[index];
            // Another card of the hero, at the same place, not upgraded.
            assert(after.id != before && !after.upgraded && run.master_deck.size() == deck);
            assert(std::find(pool.begin(), pool.end(), after.id) != pool.end());
        }
    }
    RunManager run = make_run();
    assert(run.transform_card_in_deck(0, true) && run.master_deck[0].upgraded);
    assert(!run.transform_card_in_deck(-1) && !run.transform_card_in_deck(50));
    std::cout << "[PASS] Pickup effects: a transformed card becomes another card of the hero, upgraded on request" << std::endl;
}

void test_extra_relics_and_potions() {
    RunManager run = make_run();
    size_t relics = run.relics.size();
    assert(run.gain_random_relics(3) == 3 && run.relics.size() == relics + 3);
    for (size_t i = 0; i < run.relics.size(); ++i) {
        for (size_t j = i + 1; j < run.relics.size(); ++j) assert(run.relics[i] != run.relics[j]);
    }
    // A relic obtained this way pays its own pickup effect.
    bool strawberry = false;
    for (uint32_t seed = 1; seed < 400 && !strawberry; ++seed) {
        RunManager chained = make_run(CharacterId::IRONCLAD, seed);
        int16_t max_hp = chained.max_hp;
        chained.gain_random_relics(2);
        strawberry = chained.has_relic("STRAWBERRY");
        assert(chained.max_hp == max_hp + (strawberry ? 7 : 0));
    }
    assert(strawberry);
    // Never beyond the bag.
    while (run.relics.size() < run.relics.capacity() - 1) run.relics.push_back("CIRCLET");
    assert(run.gain_random_relics(5) == 1 && run.relics.size() == run.relics.capacity());

    // Potions fill the free slots of the belt, with potions the hero can drink.
    RunManager brewer = make_run(CharacterId::SILENT);
    assert(brewer.gain_random_potions(2) == 2 && brewer.potions.size() == 2);
    assert(brewer.gain_random_potions(5) == 1 && brewer.potion_belt_full());
    for (std::string_view id : brewer.potions) assert(find_potion(id) && potion_fits_hero(id, CharacterId::SILENT));
    assert(brewer.gain_random_potions(1) == 0);
    std::cout << "[PASS] Pickup effects: extra relics (with their own pickup effect) and potions, within the bag and the belt"
              << std::endl;
}

void test_potion_slots() {
    RunManager run = make_run();
    assert(run.potion_slots == BASE_POTION_SLOTS && !run.potion_belt_full());
    while (!run.potion_belt_full()) run.potions.push_back("FIRE_POTION");
    assert(run.potions.size() == 3);

    // Potion Belt: 2 more slots.
    run.add_potion_slots(2);
    assert(run.potion_slots == 5 && !run.potion_belt_full());
    // The fight reward and the merchant use the new slots before asking which potion to throw away.
    run.phase = RunPhase::REWARD_SCREEN;
    run.potion_reward = "BLOCK_POTION";
    assert(run.take_potion_reward() && run.potions.size() == 4 && run.potions[3] == "BLOCK_POTION");
    run.gold = 999;
    run.open_shop();
    assert(run.buy_shop_potion(0) && run.potions.size() == 5 && run.potion_belt_full());
    assert(!run.buy_shop_potion(1));
    assert(run.buy_shop_potion(1, 4) && run.potions.size() == 5);

    // The belt never grows past its limit, and a new run starts with 3 slots again.
    run.add_potion_slots(100);
    assert(run.potion_slots == MAX_POTION_SLOTS);
    run.add_potion_slots(-1);
    assert(run.potion_slots == MAX_POTION_SLOTS);
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 5);
    assert(run.potion_slots == BASE_POTION_SLOTS && run.potions.empty());

    // The size of the belt and the potions past the third one are saved.
    RunManager saved = make_run();
    saved.add_potion_slots(2);
    for (std::string_view id : { "FIRE_POTION", "BLOCK_POTION", "FIRE_POTION", "BLOCK_POTION", "FIRE_POTION" }) {
        saved.potions.push_back(id);
    }
    MemoryStorage storage;
    SaveManager& store = SaveManager::instance();
    store.reset_to_defaults();
    store.set_storage(&storage);
    store.autosave_run(saved);
    store.reset_to_defaults();
    assert(store.load_from_storage());
    RunManager loaded;
    assert(store.restore_run(loaded));
    assert(loaded.potion_slots == 5 && loaded.potions.size() == 5 && loaded.potions[4] == "FIRE_POTION");
    store.set_storage(nullptr);
    store.reset_to_defaults();
    std::cout << "[PASS] Pickup effects: potion slots, used by rewards and merchant, capped, saved with the run" << std::endl;
}

void test_deck_choice() {
    // Upgrade: only the cards not upgraded yet can be picked.
    RunManager run = make_run();
    assert(!run.has_deck_choice() && !run.resolve_deck_choice(0));
    run.master_deck[0].upgraded = true;
    run.request_deck_choice(DeckChoiceKind::UPGRADE, 2);
    assert(run.has_deck_choice() && run.deck_choice.remaining == 2);
    assert(!run.deck_choice_allows(0) && run.deck_choice_allows(1) && !run.deck_choice_allows(-1));
    assert(!run.resolve_deck_choice(0) && run.deck_choice.remaining == 2);
    assert(!run.skip_deck_choice()); // it has to be made
    assert(run.resolve_deck_choice(1) && run.master_deck[1].upgraded && run.deck_choice.remaining == 1);
    assert(run.resolve_deck_choice(2) && run.master_deck[2].upgraded);
    assert(!run.has_deck_choice() && count_upgraded(run) == 3);

    // Remove: the deck loses the chosen cards, one pick at a time.
    RunManager remover = make_run();
    size_t deck = remover.master_deck.size();
    CardId last = remover.master_deck.back().id;
    remover.request_deck_choice(DeckChoiceKind::REMOVE, 2);
    assert(remover.resolve_deck_choice(0) && remover.resolve_deck_choice(0));
    assert(!remover.has_deck_choice() && remover.master_deck.size() == deck - 2);
    assert(remover.master_deck.back().id == last);

    // Transform, then upgrade (Astrolabe).
    RunManager astrolabe = make_run();
    CardId before = astrolabe.master_deck[4].id;
    astrolabe.request_deck_choice(DeckChoiceKind::TRANSFORM, 1, false, true);
    assert(astrolabe.resolve_deck_choice(4));
    assert(astrolabe.master_deck[4].id != before && astrolabe.master_deck[4].upgraded);
    assert(!astrolabe.has_deck_choice() && astrolabe.master_deck.size() == deck);

    // "Up to N": the choice can be left at any time.
    RunManager optional = make_run();
    optional.request_deck_choice(DeckChoiceKind::REMOVE, 3, true);
    assert(optional.resolve_deck_choice(0) && optional.deck_choice.remaining == 2);
    assert(optional.skip_deck_choice() && !optional.has_deck_choice());
    assert(optional.master_deck.size() == deck - 1);

    // Nothing is asked when no card fits, and no more cards than the deck can give.
    RunManager done = make_run();
    for (DeckCard& card : done.master_deck) card.upgraded = true;
    done.request_deck_choice(DeckChoiceKind::UPGRADE, 1);
    assert(!done.has_deck_choice());
    done.master_deck[0].upgraded = false;
    done.request_deck_choice(DeckChoiceKind::UPGRADE, 3);
    assert(done.deck_choice.remaining == 1);
    assert(done.resolve_deck_choice(0) && !done.has_deck_choice());
    done.master_deck.clear();
    done.request_deck_choice(DeckChoiceKind::REMOVE, 1);
    assert(!done.has_deck_choice());
    done.request_deck_choice(DeckChoiceKind::NONE, 2);
    assert(!done.has_deck_choice());

    // A new run forgets the choice of the previous one.
    RunManager restarted = make_run();
    restarted.request_deck_choice(DeckChoiceKind::REMOVE, 1);
    restarted.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 3);
    assert(!restarted.has_deck_choice());
    std::cout << "[PASS] Deck choice: upgrade, remove, transform (upgraded on request), optional, bounded by the deck"
              << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Relic Pickup Effects ===" << std::endl;
    test_pickup_hook();
    test_hp_and_gold_effects();
    test_random_upgrades();
    test_cards_added_to_the_deck();
    test_transform();
    test_extra_relics_and_potions();
    test_potion_slots();
    test_deck_choice();
    std::cout << "All relic pickup tests passed." << std::endl;
    return 0;
}
