#include "engine/shop.h"
#include "engine/run.h"
#include "engine/potion.h"
#include "engine/relic.h"
#include "engine/rewards.h"

namespace engine {

void ShopInventory::generate(CharacterId char_id, uint32_t seed, uint8_t previous_removals,
                             std::span<const std::string_view> owned_relics) {
    cards.clear();
    relics.clear();
    potions.clear();
    card_removal_cost = 75 + (previous_removals * 25);
    card_removal_used = false;

    // Character-specific card catalog
    switch (char_id) {
        case CharacterId::SILENT:
            cards.push_back({ DeckCard(CardId::POISON_STAB, false), 50, false });
            cards.push_back({ DeckCard(CardId::DEADLY_POISON, false), 75, false });
            cards.push_back({ DeckCard(CardId::SURVIVOR, true), 120, false });
            break;
        case CharacterId::DEFECT:
            cards.push_back({ DeckCard(CardId::ZAP, true), 80, false });
            cards.push_back({ DeckCard(CardId::DUALCAST, true), 80, false });
            cards.push_back({ DeckCard(CardId::BALL_LIGHTNING, false), 50, false });
            break;
        case CharacterId::REGENT:
            cards.push_back({ DeckCard(CardId::FALLING_STAR, false), 60, false });
            cards.push_back({ DeckCard(CardId::VENERATE, true), 110, false });
            cards.push_back({ DeckCard(CardId::GATHER_LIGHT, false), 50, false });
            break;
        case CharacterId::NECROBINDER:
            cards.push_back({ DeckCard(CardId::UNLEASH, false), 65, false });
            cards.push_back({ DeckCard(CardId::BODYGUARD, true), 115, false });
            cards.push_back({ DeckCard(CardId::DRAIN_POWER, false), 50, false });
            break;
        default: // IRONCLAD
            cards.push_back({ DeckCard(CardId::IRON_WAVE, false), 50, false });
            cards.push_back({ DeckCard(CardId::THUNDERCLAP, false), 75, false });
            cards.push_back({ DeckCard(CardId::METALLICIZE, false), 150, false });
            break;
    }

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
}

bool ShopInventory::buy_card(RunManager& run, int index) {
    if (index < 0 || static_cast<size_t>(index) >= cards.size()) return false;
    auto& item = cards[index];
    if (item.purchased || run.gold < item.cost) return false;

    run.gold -= item.cost;
    item.purchased = true;
    run.master_deck.push_back(item.card);
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
    if (item.purchased || run.gold < item.cost) return false;
    bool full = run.potions.size() >= run.potions.capacity();
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

bool ShopInventory::remove_card(RunManager& run, int deck_index) {
    if (card_removal_used || run.gold < card_removal_cost) return false;
    if (deck_index < 0 || static_cast<size_t>(deck_index) >= run.master_deck.size()) return false;

    run.gold -= card_removal_cost;
    card_removal_used = true;
    run.card_removals_count++;
    run.master_deck.erase(deck_index);
    return true;
}

} // namespace engine
