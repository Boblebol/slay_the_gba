#include "engine/shop.h"
#include "engine/run.h"
#include "engine/potion.h"
#include "engine/relic.h"
#include "engine/rewards.h"

namespace engine {

namespace {

// Rarity of a card of the merchant, drawn like the rewards: 60 % Common, 30 % Uncommon, 10 % Rare.
CardRarity roll_card_rarity(RewardRng& rng, bool colorless) {
    uint32_t roll = rng.below(100);
    if (colorless) return roll < 70 ? CardRarity::UNCOMMON : CardRarity::RARE; // no Common Colorless card
    return roll < 60 ? CardRarity::COMMON : (roll < 90 ? CardRarity::UNCOMMON : CardRarity::RARE);
}

int16_t card_base_price(CardRarity rarity, bool colorless) {
    int16_t base = rarity == CardRarity::COMMON ? 50 : (rarity == CardRarity::UNCOMMON ? 75 : 150);
    return colorless ? static_cast<int16_t>(base * 115 / 100) : base;
}

bool already_stocked(const FixedVector<ShopCard, 7>& cards, CardId id) {
    for (const ShopCard& item : cards) {
        if (item.card.id == id) return true;
    }
    return false;
}

// A card of `rarity` among `candidates` not stocked yet; another rarity (the next one, then the previous)
// when none is left. CardId::COUNT when nothing can be drawn.
CardId draw_stock_card(RewardRng& rng, std::span<const CardId> candidates, CardRarity rarity,
                       const FixedVector<ShopCard, 7>& cards, CardRarity& drawn_rarity) {
    static constexpr CardRarity ORDER[] = { CardRarity::COMMON, CardRarity::UNCOMMON, CardRarity::RARE };
    int start = rarity == CardRarity::COMMON ? 0 : (rarity == CardRarity::UNCOMMON ? 1 : 2);
    for (int step = 0; step < 5; ++step) {
        static constexpr int SEARCH[] = { 0, 1, -1, 2, -2 };
        int at = start + SEARCH[step];
        if (at < 0 || at > 2) continue;
        FixedVector<CardId, 512> pool;
        for (CardId id : candidates) {
            if (card_meta(id).rarity == ORDER[at] && !already_stocked(cards, id)) pool.push_back(id);
        }
        if (pool.empty()) continue;
        drawn_rarity = ORDER[at];
        return pool[rng.below(static_cast<uint32_t>(pool.size()))];
    }
    return CardId::COUNT;
}

} // namespace

void ShopInventory::generate(CharacterId char_id, uint32_t seed, uint8_t previous_removals,
                             std::span<const std::string_view> owned_relics) {
    cards.clear();
    relics.clear();
    potions.clear();
    card_removal_cost = 75 + (previous_removals * 25);
    card_removal_used = false;

    // The stock of relics and potions has its own stream, seeded by the draw the run made for this visit.
    RewardRng rng(seed);

    // 3 relics the hero does not own: two drawn by rarity, and one of the merchant's own (Shop rarity).
    FixedVector<std::string_view, 3> stocked_relics;
    for (int slot = 0; slot < 3; ++slot) {
        RelicSource source = slot == 2 ? RelicSource::SHOP : RelicSource::ELITE;
        std::string_view id = draw_relic(rng, char_id, source, owned_relics,
                                         std::span<const std::string_view>(stocked_relics.begin(), stocked_relics.size()));
        if (id.empty()) break;
        stocked_relics.push_back(id);
        relics.push_back({ id, vary_price(rng, relic_base_price(find_relic(id)->rarity)), false });
    }

    // 3 different potions the engine resolves, drawn by rarity.
    FixedVector<std::string_view, 3> stocked_potions;
    for (int slot = 0; slot < 3; ++slot) {
        std::string_view id = draw_potion(rng, char_id,
                                          std::span<const std::string_view>(stocked_potions.begin(), stocked_potions.size()));
        if (id.empty()) break;
        stocked_potions.push_back(id);
        potions.push_back({ id, vary_price(rng, potion_base_price(find_potion(id)->rarity)), false });
    }

    // 5 cards of the hero, then 2 Colorless cards, drawn by rarity after the relics and the potions.
    std::span<const CardId> hero_pool = RunManager::card_reward_pool(char_id);
    for (int slot = 0; slot < 5; ++slot) {
        CardRarity rarity = CardRarity::COMMON;
        CardId id = draw_stock_card(rng, hero_pool, roll_card_rarity(rng, false), cards, rarity);
        if (id == CardId::COUNT) break;
        cards.push_back({ DeckCard(id, false), vary_price(rng, card_base_price(rarity, false)), false, false });
    }
    FixedVector<CardId, 128> colorless_pool;
    for (uint16_t raw = 0; raw < static_cast<uint16_t>(CardId::COUNT); ++raw) {
        CardId id = static_cast<CardId>(raw);
        if (card_meta(id).pool == CardPool::COLORLESS) colorless_pool.push_back(id);
    }
    for (int slot = 0; slot < 2; ++slot) {
        CardRarity rarity = CardRarity::UNCOMMON;
        CardId id = draw_stock_card(rng, std::span<const CardId>(colorless_pool.begin(), colorless_pool.size()),
                                    roll_card_rarity(rng, true), cards, rarity);
        if (id == CardId::COUNT) break;
        cards.push_back({ DeckCard(id, false), vary_price(rng, card_base_price(rarity, true)), false, false });
    }
    // One card of the hero is on sale at half price.
    size_t hero_cards = cards.size() < 5 ? cards.size() : 5;
    if (hero_cards > 0) {
        ShopCard& sale = cards[rng.below(static_cast<uint32_t>(hero_cards))];
        sale.cost = static_cast<int16_t>((sale.cost + 1) / 2);
        sale.on_sale = true;
    }
}

bool ShopInventory::buy_card(RunManager& run, int index) {
    if (index < 0 || static_cast<size_t>(index) >= cards.size()) return false;
    auto& item = cards[index];
    if (item.purchased || run.gold < item.cost) return false;

    run.gold -= item.cost;
    item.purchased = true;
    run.add_card_to_deck(item.card);
    return true;
}

bool ShopInventory::buy_relic(RunManager& run, int index) {
    if (index < 0 || static_cast<size_t>(index) >= relics.size()) return false;
    auto& item = relics[index];
    if (item.purchased || run.gold < item.cost) return false;
    if (run.has_relic(item.relic_id) || run.relics.size() >= run.relics.capacity()) return false;

    run.gold -= item.cost;
    item.purchased = true;
    run.add_relic(item.relic_id);
    return true;
}

bool ShopInventory::buy_potion(RunManager& run, int index, int replace_index) {
    if (index < 0 || static_cast<size_t>(index) >= potions.size()) return false;
    auto& item = potions[index];
    if (item.purchased || run.gold < item.cost || run.has_relic("SOZU")) return false;
    bool full = run.potion_belt_full();
    if (full && (replace_index < 0 || static_cast<size_t>(replace_index) >= run.potions.size())) return false;

    run.gold -= item.cost;
    item.purchased = true;
    if (full) {
        run.potions[replace_index] = item.potion_id;
    } else {
        run.potions.push_back(item.potion_id);
    }
    return true;
}

void ShopInventory::apply_discount(int percent) {
    auto lowered = [percent](int16_t cost) {
        return static_cast<int16_t>((cost * (100 - percent) + 50) / 100);
    };
    for (ShopCard& item : cards) item.cost = lowered(item.cost);
    for (ShopRelic& item : relics) item.cost = lowered(item.cost);
    for (ShopPotion& item : potions) item.cost = lowered(item.cost);
    card_removal_cost = lowered(card_removal_cost);
}

bool ShopInventory::remove_card(RunManager& run, int deck_index) {
    if (card_removal_used || run.gold < card_removal_cost) return false;
    if (deck_index < 0 || static_cast<size_t>(deck_index) >= run.master_deck.size()) return false;
    if (CardDef::is_eternal(run.master_deck[deck_index])) return false;

    run.gold -= card_removal_cost;
    card_removal_used = true;
    run.card_removals_count++;
    run.master_deck.erase(deck_index);
    return true;
}

} // namespace engine
