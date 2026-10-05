#pragma once
#include <cstdint>
#include <string_view>
#include "engine/fixed_vector.h"
#include "engine/card.h"
#include "engine/character.h"
#include <span>

namespace engine {

class RunManager;

struct ShopCard {
    DeckCard card;
    int16_t cost = 50;
    bool purchased = false;
};

struct ShopRelic {
    std::string_view relic_id;
    int16_t cost = 150;
    bool purchased = false;
};

struct ShopPotion {
    std::string_view potion_id;
    int16_t cost = 50;
    bool purchased = false;
};

class ShopInventory {
public:
    FixedVector<ShopCard, 5> cards;
    FixedVector<ShopRelic, 3> relics;
    FixedVector<ShopPotion, 3> potions;
    int16_t card_removal_cost = 75;
    bool card_removal_used = false;

    // Relics and potions are drawn from `seed`, by rarity and priced by rarity; a relic of `owned_relics` is
    // never put on sale.
    void generate(CharacterId char_id, uint32_t seed, uint8_t previous_removals = 0,
                  std::span<const std::string_view> owned_relics = {});
    bool buy_card(RunManager& run, int index);
    bool buy_relic(RunManager& run, int index);
    // With a full belt, `replace_index` is the potion of the belt thrown away to make room.
    bool buy_potion(RunManager& run, int index, int replace_index = -1);
    bool remove_card(RunManager& run, int deck_index);
    // Lowers every price still to pay by `percent`, rounded to the nearest gold: cards, relics, potions and
    // the card removal.
    void apply_discount(int percent);
};

} // namespace engine
