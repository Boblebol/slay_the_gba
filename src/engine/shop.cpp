#include "engine/shop.h"
#include "engine/run.h"

namespace engine {

void ShopInventory::generate(CharacterId char_id, uint32_t seed, uint8_t previous_removals) {
    (void)seed;
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

    // 3 Relics
    relics.push_back({ "VAJRA", 160, false });
    relics.push_back({ "ANCHOR", 150, false });
    relics.push_back({ "SLING_OF_COURAGE", 150, false });

    // 2 Potions
    potions.push_back({ "FIRE_POTION", 50, false });
    potions.push_back({ "BLOCK_POTION", 50, false });
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

    run.gold -= item.cost;
    item.purchased = true;
    run.add_relic(item.relic_id);
    return true;
}

bool ShopInventory::buy_potion(RunManager& run, int index) {
    if (index < 0 || static_cast<size_t>(index) >= potions.size()) return false;
    auto& item = potions[index];
    if (item.purchased || run.gold < item.cost) return false;
    if (run.potions.size() >= run.potions.capacity()) return false;

    run.gold -= item.cost;
    item.purchased = true;
    run.potions.push_back(item.potion_id);
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
