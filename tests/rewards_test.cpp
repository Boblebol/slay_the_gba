// Rewards of a fight and stock of the merchant: potions and relics drawn by rarity, from a stream of their own.
#include <algorithm>
#include <cassert>
#include <iostream>
#include <set>
#include <string>
#include <string_view>

#include "engine/potion.h"
#include "engine/relic.h"
#include "engine/rewards.h"
#include "engine/run.h"

using namespace engine;

namespace {

constexpr CharacterId HEROES[] = { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT,
                                   CharacterId::REGENT, CharacterId::NECROBINDER };
constexpr uint32_t SEEDS = 2000;

// A run whose hero just won a fight in a room of the given type.
RunManager run_after_fight(CharacterId hero, uint32_t seed, RoomType room) {
    RunManager run;
    run.start_run(hero, 0, PaletteVariant::CLASSIC, seed);
    run.current_node_index = run.get_available_rooms()[0];
    run.map.nodes[run.current_node_index].type = room;
    run.current_floor = static_cast<uint8_t>(run.map.nodes[run.current_node_index].floor + 1);
    run.phase = RunPhase::IN_COMBAT;
    return run;
}

bool is_active_relic(std::string_view id) {
    auto active = RunManager::active_relics();
    return std::find(active.begin(), active.end(), id) != active.end();
}

bool relic_fits_hero(std::string_view id, CharacterId hero) {
    const RelicData* relic = find_relic(id);
    return relic && (relic->pool == "Shared" || relic->pool == get_character(hero).english_name);
}

bool is_drop_rarity(RelicRarity rarity) {
    return rarity == RelicRarity::COMMON || rarity == RelicRarity::UNCOMMON || rarity == RelicRarity::RARE;
}

// `count` out of `total` is within `tolerance` points of `percent`.
bool about(uint32_t count, uint32_t total, uint32_t percent, uint32_t tolerance) {
    uint32_t tenths = count * 1000 / total;
    return tenths + tolerance * 10 >= percent * 10 && tenths <= (percent + tolerance) * 10;
}

void test_active_potions_are_the_resolved_ones() {
    // The engine tells a potion it resolves from one it does not: use_potion refuses the second kind.
    size_t resolved = 0;
    for (const PotionData& potion : get_all_potions()) {
        bool listed = false;
        CharacterId hero = CharacterId::IRONCLAD;
        for (const ActivePotion& active : active_potions()) {
            if (active.id == potion.id) {
                listed = true;
                if (!active.any_hero) hero = active.hero;
            }
        }
        CombatEngine combat;
        combat.start_combat(EnemyType::CULTIST, hero);
        bool works = combat.use_potion(potion.id, 0);
        if (works != listed) {
            std::cout << "[FAIL] " << potion.id << (works ? " is resolved by the engine but missing from"
                                                          : " is not resolved by the engine but listed in")
                      << " active_potions() (src/engine/rewards.cpp)" << std::endl;
        }
        assert(works == listed);
        resolved += works;
    }
    assert(resolved == active_potions().size()); // no id twice, none unknown to the database
    for (const ActivePotion& active : active_potions()) {
        assert(find_potion(active.id) != nullptr);
        for (CharacterId hero : HEROES) {
            assert(potion_fits_hero(active.id, hero) == (active.any_hero || active.hero == hero));
        }
    }
    assert(!potion_fits_hero("SNECKO_OIL", CharacterId::IRONCLAD) && !potion_fits_hero("NOT_A_POTION", CharacterId::DEFECT));
    assert(potion_fits_hero("FOCUS_POTION", CharacterId::DEFECT) && !potion_fits_hero("FOCUS_POTION", CharacterId::SILENT));
    std::cout << "[PASS] Potions: " << resolved << " resolved by the engine, the only ones on offer" << std::endl;
}

void test_potion_rewards() {
    uint32_t fights = 0, drops = 0, common = 0, uncommon = 0, rare = 0;
    for (CharacterId hero : HEROES) {
        for (uint32_t seed = 1; seed <= SEEDS; ++seed) {
            RunManager run = run_after_fight(hero, seed, RoomType::MONSTER);
            run.generate_rewards();
            ++fights;
            assert(run.relic_reward.empty()); // a normal fight gives no relic
            if (run.potion_reward.empty()) continue;
            ++drops;
            assert(potion_fits_hero(run.potion_reward, hero));
            std::string_view rarity = find_potion(run.potion_reward)->rarity;
            assert(rarity == "Common" || rarity == "Uncommon" || rarity == "Rare");
            common += rarity == "Common";
            uncommon += rarity == "Uncommon";
            rare += rarity == "Rare";
        }
    }
    assert(about(drops, fights, POTION_REWARD_CHANCE, 2));
    assert(about(common, drops, 65, 3) && about(uncommon, drops, 25, 3) && about(rare, drops, 10, 3));
    std::cout << "[PASS] Potion rewards: " << drops << " in " << fights << " fights (" << drops * 100 / fights
              << " %), Common " << common * 100 / drops << " %, Uncommon " << uncommon * 100 / drops << " %, Rare "
              << rare * 100 / drops << " %" << std::endl;
}

void test_relic_rewards() {
    uint32_t common = 0, uncommon = 0, rare = 0, total = 0;
    for (CharacterId hero : HEROES) {
        for (uint32_t seed = 1; seed <= SEEDS; ++seed) {
            RunManager run = run_after_fight(hero, seed, RoomType::ELITE);
            run.generate_rewards();
            std::string_view relic = run.relic_reward;
            assert(!relic.empty() && is_active_relic(relic) && relic_fits_hero(relic, hero) && !run.has_relic(relic));
            RelicRarity rarity = find_relic(relic)->rarity;
            assert(is_drop_rarity(rarity)); // the merchant's own relics are not dropped
            ++total;
            common += rarity == RelicRarity::COMMON;
            uncommon += rarity == RelicRarity::UNCOMMON;
            rare += rarity == RelicRarity::RARE;

            // A boss gives a Rare relic.
            RunManager boss = run_after_fight(hero, seed, RoomType::BOSS);
            boss.generate_rewards();
            assert(!boss.relic_reward.empty() && relic_fits_hero(boss.relic_reward, hero));
            assert(find_relic(boss.relic_reward)->rarity == RelicRarity::RARE);
            assert(boss.gold_reward >= 95 && boss.gold_reward <= 105 && boss.current_card_rewards.size() == 3);
        }
    }
    assert(about(common, total, RELIC_COMMON_CHANCE, 3) && about(uncommon, total, RELIC_UNCOMMON_CHANCE, 3) &&
           about(rare, total, 17, 3));
    std::cout << "[PASS] Relic rewards: one after each of " << total << " elites, Common " << common * 100 / total
              << " %, Uncommon " << uncommon * 100 / total << " %, Rare " << rare * 100 / total
              << " %; a Rare one after a boss" << std::endl;
}

void test_relics_never_twice() {
    for (CharacterId hero : HEROES) {
        size_t available = 0;
        for (std::string_view id : RunManager::active_relics()) {
            available += relic_fits_hero(id, hero) && is_drop_rarity(find_relic(id)->rarity);
        }
        RunManager run = run_after_fight(hero, 31, RoomType::ELITE);
        std::set<std::string_view> seen;
        size_t taken = 0;
        // Elite after elite, until the bag is full or nothing is left to give.
        for (uint8_t floor = 1; floor < 120; ++floor) {
            run.current_floor = floor;
            run.phase = RunPhase::IN_COMBAT;
            run.generate_rewards();
            if (run.relic_reward.empty()) break;
            std::string_view relic = run.relic_reward;
            assert(!run.has_relic(relic) && seen.insert(relic).second && relic_fits_hero(relic, hero));
            assert(run.take_relic_reward() && run.has_relic(relic) && run.relic_reward.empty());
            assert(!run.take_relic_reward()); // taken once
            ++taken;
        }
        // The starter relic takes one of the 25 places.
        assert(taken == std::min(available, run.relics.capacity() - 1));
        for (size_t i = 0; i < run.relics.size(); ++i) {
            for (size_t j = 0; j < i; ++j) assert(run.relics[i] != run.relics[j]);
        }
    }

    // Nothing is offered once every relic the hero can hold is owned.
    RunManager owner = run_after_fight(CharacterId::IRONCLAD, 8, RoomType::ELITE);
    owner.relics.clear();
    size_t owned = 0;
    for (std::string_view id : RunManager::active_relics()) {
        if (!relic_fits_hero(id, CharacterId::IRONCLAD) || !is_drop_rarity(find_relic(id)->rarity)) continue;
        if (owner.relics.size() + 1 < owner.relics.capacity()) {
            owner.add_relic(id);
            ++owned;
        }
    }
    RewardRng rng(5);
    std::span<const std::string_view> bag(owner.relics.begin(), owner.relics.size());
    for (int draw = 0; draw < 200; ++draw) {
        std::string_view id = draw_relic(rng, CharacterId::IRONCLAD, RelicSource::ELITE, bag);
        assert(id.empty() || !owner.has_relic(id));
    }
    assert(owned > 0);
    std::cout << "[PASS] Relic rewards: never a relic already owned, never another hero's" << std::endl;
}

void test_same_seed_same_rewards() {
    for (CharacterId hero : HEROES) {
        for (RoomType room : { RoomType::MONSTER, RoomType::ELITE, RoomType::BOSS }) {
            for (uint32_t seed : { 42u, 777u, 4242u, 90001u }) {
                RunManager first = run_after_fight(hero, seed, room);
                RunManager second = run_after_fight(hero, seed, room);
                first.generate_rewards();
                second.generate_rewards();
                assert(first.potion_reward == second.potion_reward && first.relic_reward == second.relic_reward);
                assert(first.gold_reward == second.gold_reward);
                for (size_t i = 0; i < 3; ++i) {
                    assert(first.current_card_rewards[i].id == second.current_card_rewards[i].id);
                }
                assert(first.rng_state() == second.rng_state());

                // The potion and the relic are drawn apart: the run's generator ends where a normal fight leaves
                // it, whatever the room gives.
                RunManager monster = run_after_fight(hero, seed, RoomType::MONSTER);
                RunManager elite = run_after_fight(hero, seed, RoomType::ELITE);
                monster.generate_rewards();
                elite.generate_rewards();
                assert(monster.rng_state() == elite.rng_state());
                for (size_t i = 0; i < 3; ++i) {
                    assert(monster.current_card_rewards[i].id == elite.current_card_rewards[i].id);
                }
                assert(monster.gold_reward == elite.gold_reward);
            }
        }
    }

    // A boss draws everything from the stream of its room: the run's generator does not move until the next
    // act draws its map, as before bosses gave rewards.
    RunManager boss = run_after_fight(CharacterId::SILENT, 777, RoomType::BOSS);
    boss.combat.player.hp = 30;
    boss.combat.player.max_hp = boss.max_hp;
    const uint32_t before = boss.rng_state();
    boss.proceed_after_combat();
    assert(boss.phase == RunPhase::REWARD_SCREEN && boss.rng_state() == before && boss.current_act == 1);
    assert(boss.take_relic_reward());
    boss.skip_card_rewards();
    assert(boss.phase == RunPhase::ANCIENT_BLESSING && boss.current_act == 2);
    assert(boss.rng_state() != before && boss.map_seed == boss.rng_state()); // one draw: the seed of the new map
    assert(boss.potion_reward.empty() && boss.relic_reward.empty());

    // Two rooms of the same run do not give the same thing over and over.
    std::set<std::string_view> relics;
    RunManager run = run_after_fight(CharacterId::IRONCLAD, 42, RoomType::ELITE);
    for (uint8_t floor = 1; floor <= 15; ++floor) {
        run.current_floor = floor;
        run.generate_rewards();
        relics.insert(run.relic_reward);
    }
    assert(relics.size() > 5);
    std::cout << "[PASS] Rewards: same seed, same rewards; the run's own random sequence is left as it was" << std::endl;
}

void test_potion_belt() {
    // Find a fight that gives a potion.
    uint32_t seed = 1;
    RunManager run = run_after_fight(CharacterId::IRONCLAD, seed, RoomType::MONSTER);
    run.generate_rewards();
    while (run.potion_reward.empty()) {
        run = run_after_fight(CharacterId::IRONCLAD, ++seed, RoomType::MONSTER);
        run.generate_rewards();
    }
    const std::string_view offered = run.potion_reward;

    // Room in the belt: the potion goes in.
    RunManager taker = run;
    assert(taker.take_potion_reward() && taker.potions.size() == 1 && taker.potions[0] == offered);
    assert(taker.potion_reward.empty() && !taker.take_potion_reward());
    assert(taker.phase == RunPhase::REWARD_SCREEN); // the cards are still to choose

    // Full belt: the potion stays on offer until one is thrown away for it, or it is left behind.
    RunManager full = run;
    full.potions.push_back("FIRE_POTION");
    full.potions.push_back("BLOCK_POTION");
    full.potions.push_back("SWIFT_POTION");
    assert(!full.take_potion_reward() && !full.take_potion_reward(3) && !full.take_potion_reward(-2));
    assert(full.potion_reward == offered && full.potions.size() == 3);
    assert(full.take_potion_reward(1));
    assert(full.potions.size() == 3 && full.potions[0] == "FIRE_POTION" && full.potions[1] == offered &&
           full.potions[2] == "SWIFT_POTION");

    RunManager skipper = run;
    skipper.potions.push_back("FIRE_POTION");
    skipper.potions.push_back("BLOCK_POTION");
    skipper.potions.push_back("SWIFT_POTION");
    const int16_t gold = skipper.gold;
    const int16_t reward = skipper.gold_reward;
    skipper.skip_card_rewards();
    assert(skipper.potions.size() == 3 && skipper.potions[1] == "BLOCK_POTION" && skipper.potion_reward.empty());
    assert(skipper.gold == gold + reward && skipper.phase == RunPhase::MAP_NAVIGATION);
    assert(!skipper.take_potion_reward(0)); // the reward screen is closed

    // The potion taken is one the hero can drink in the next fight.
    taker.skip_card_rewards();
    taker.enter_room(taker.get_available_rooms()[0]);
    if (taker.phase == RunPhase::IN_COMBAT) {
        assert(taker.use_potion(0, 0) && taker.potions.empty());
    }
    std::cout << "[PASS] Potion rewards: taken with room in the belt, replace or leave with a full belt" << std::endl;
}

void test_shop_stock() {
    for (CharacterId hero : HEROES) {
        for (uint32_t seed = 1; seed <= 400; ++seed) {
            RunManager run;
            run.start_run(hero, 0, PaletteVariant::CLASSIC, seed);
            run.add_relic("VAJRA");
            run.add_relic("BREAD");
            run.open_shop();
            const ShopInventory& shop = run.current_shop;

            assert(shop.relics.size() == 3);
            for (size_t i = 0; i < shop.relics.size(); ++i) {
                const ShopRelic& item = shop.relics[i];
                const RelicData* relic = find_relic(item.relic_id);
                assert(relic && is_active_relic(item.relic_id) && relic_fits_hero(item.relic_id, hero));
                assert(!run.has_relic(item.relic_id) && !item.purchased);
                for (size_t j = 0; j < i; ++j) assert(shop.relics[j].relic_id != item.relic_id);
                // Two relics drawn by rarity, then one of the merchant's own.
                assert(i == 2 ? relic->rarity == RelicRarity::SHOP : is_drop_rarity(relic->rarity));
                int base = relic->rarity == RelicRarity::UNCOMMON ? 250 : (relic->rarity == RelicRarity::RARE ? 300 : 150);
                assert(relic_base_price(relic->rarity) == base);
                assert(item.cost * 100 >= base * 95 - 99 && item.cost * 100 <= base * 105);
            }

            assert(shop.potions.size() == 3);
            for (size_t i = 0; i < shop.potions.size(); ++i) {
                const ShopPotion& item = shop.potions[i];
                assert(potion_fits_hero(item.potion_id, hero) && !item.purchased);
                for (size_t j = 0; j < i; ++j) assert(shop.potions[j].potion_id != item.potion_id);
                std::string_view rarity = find_potion(item.potion_id)->rarity;
                int base = rarity == "Common" ? 50 : (rarity == "Uncommon" ? 75 : 100);
                assert(rarity == "Common" || rarity == "Uncommon" || rarity == "Rare");
                assert(potion_base_price(rarity) == base);
                assert(item.cost * 100 >= base * 95 - 99 && item.cost * 100 <= base * 105);
            }

            // Same seed, same stock.
            RunManager again;
            again.start_run(hero, 0, PaletteVariant::CLASSIC, seed);
            again.add_relic("VAJRA");
            again.add_relic("BREAD");
            again.open_shop();
            for (size_t i = 0; i < 3; ++i) {
                assert(again.current_shop.relics[i].relic_id == shop.relics[i].relic_id);
                assert(again.current_shop.relics[i].cost == shop.relics[i].cost);
                assert(again.current_shop.potions[i].potion_id == shop.potions[i].potion_id);
                assert(again.current_shop.potions[i].cost == shop.potions[i].cost);
            }
            assert(again.rng_state() == run.rng_state());
        }
    }
    assert(relic_base_price(RelicRarity::SHOP) == 150 && relic_base_price(RelicRarity::ANCIENT) == 0);
    assert(potion_base_price("Event") == 0);

    // Prices move with the visit: the same rarity is not always sold at the same price.
    std::set<int16_t> prices;
    for (uint32_t seed = 1; seed <= 60; ++seed) {
        ShopInventory shop;
        shop.generate(CharacterId::IRONCLAD, seed);
        prices.insert(shop.relics[2].cost);
        assert(shop.relics[2].cost >= 142 && shop.relics[2].cost <= 157);
    }
    assert(prices.size() > 5);
    std::cout << "[PASS] Shop: 3 relics and 3 potions drawn by rarity, priced by rarity within 5 %, same seed same stock"
              << std::endl;
}

void test_shop_cards() {
    std::set<int16_t> common_prices;
    int stocks = 0;
    for (CharacterId hero : HEROES) {
        std::set<CardId> seen;
        for (uint32_t seed = 1; seed <= 300; ++seed) {
            ShopInventory shop;
            shop.generate(hero, seed);
            // 5 cards of the hero then 2 Colorless cards, all different.
            assert(shop.cards.size() == 7);
            int on_sale = 0;
            for (size_t i = 0; i < shop.cards.size(); ++i) {
                const ShopCard& item = shop.cards[i];
                bool colorless = i >= 5;
                const CardMeta& meta = card_meta(item.card.id);
                assert(!item.purchased && !item.card.upgraded);
                assert(meta.pool == (colorless ? CardPool::COLORLESS : static_cast<CardPool>(1 + static_cast<int>(hero))));
                assert(meta.rarity == CardRarity::COMMON || meta.rarity == CardRarity::UNCOMMON || meta.rarity == CardRarity::RARE);
                for (size_t j = 0; j < i; ++j) assert(shop.cards[j].card.id != item.card.id);
                seen.insert(item.card.id);
                // Price by rarity (Colorless 15 % dearer), within 5 %, halved on the card on sale.
                int base = meta.rarity == CardRarity::COMMON ? 50 : (meta.rarity == CardRarity::UNCOMMON ? 75 : 150);
                if (colorless) base = base * 115 / 100;
                int low = base * 95 / 100, high = base * 105 / 100;
                if (item.on_sale) {
                    assert(!colorless);
                    ++on_sale;
                    low = (low + 1) / 2;
                    high = (high + 1) / 2;
                }
                assert(item.cost >= low && item.cost <= high);
                if (meta.rarity == CardRarity::COMMON && !colorless && !item.on_sale) common_prices.insert(item.cost);
            }
            assert(on_sale == 1);
            ++stocks;

            // Same seed, same stock.
            ShopInventory again;
            again.generate(hero, seed);
            for (size_t i = 0; i < shop.cards.size(); ++i) {
                assert(again.cards[i].card == shop.cards[i].card && again.cards[i].cost == shop.cards[i].cost);
                assert(again.cards[i].on_sale == shop.cards[i].on_sale);
            }
        }
        // The stock changes with the seed.
        assert(seen.size() > 20);
    }
    assert(stocks == 5 * 300 && common_prices.size() > 3);

    // The stock of a run follows the run's seed, and every visit draws its own.
    RunManager first, second;
    first.start_run(CharacterId::SILENT, 0, PaletteVariant::CLASSIC, 777);
    second.start_run(CharacterId::SILENT, 0, PaletteVariant::CLASSIC, 777);
    first.open_shop();
    second.open_shop();
    for (size_t i = 0; i < 7; ++i) assert(first.current_shop.cards[i].card == second.current_shop.cards[i].card);
    first.open_shop();
    bool differs = false;
    for (size_t i = 0; i < 7; ++i) differs |= !(first.current_shop.cards[i].card == second.current_shop.cards[i].card);
    assert(differs);
    std::cout << "[PASS] Shop: 5 hero cards and 2 Colorless cards drawn by rarity, one on sale, same seed same stock"
              << std::endl;
}

void test_shop_potion_with_full_belt() {
    RunManager run;
    run.start_run(CharacterId::REGENT, 0, PaletteVariant::CLASSIC, 12);
    run.open_shop();
    run.gold = 999;
    run.potions.push_back("FIRE_POTION");
    run.potions.push_back("BLOCK_POTION");
    run.potions.push_back("SWIFT_POTION");
    const ShopPotion first = run.current_shop.potions[0];

    // No room and no potion to throw away: nothing is sold.
    assert(!run.buy_shop_potion(0) && !run.buy_shop_potion(0, 3));
    assert(run.gold == 999 && !run.current_shop.potions[0].purchased && run.potions.size() == 3);

    assert(run.buy_shop_potion(0, 2));
    assert(run.gold == 999 - first.cost && run.current_shop.potions[0].purchased);
    assert(run.potions.size() == 3 && run.potions[2] == first.potion_id && run.potions[0] == "FIRE_POTION");
    assert(!run.buy_shop_potion(0, 1)); // sold

    // With room, the index is not needed.
    run.potions.erase(0);
    const ShopPotion second = run.current_shop.potions[1];
    assert(run.buy_shop_potion(1));
    assert(run.potions.size() == 3 && run.potions.back() == second.potion_id);
    std::cout << "[PASS] Shop: a potion bought with a full belt replaces the one thrown away" << std::endl;
}

} // namespace

int main() {
    test_active_potions_are_the_resolved_ones();
    test_potion_rewards();
    test_relic_rewards();
    test_relics_never_twice();
    test_same_seed_same_rewards();
    test_potion_belt();
    test_shop_stock();
    test_shop_cards();
    test_shop_potion_with_full_belt();
    std::cout << "[PASS] Rewards: all groups validated" << std::endl;
    return 0;
}
