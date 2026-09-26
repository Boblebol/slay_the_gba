#pragma once
#include <cstdint>
#include <string_view>
#include <span>

namespace engine {

struct CardRecord {
    std::string_view id;
    std::string_view name;
    int8_t energy; // -1 for X or special
    std::string_view type;
    std::string_view rarity;
    std::string_view pool;
    std::string_view desc_base;
    std::string_view desc_upgraded;
};

const CardRecord* find_card_record(std::string_view id);
std::span<const CardRecord> get_all_card_records();
size_t count_cards_by_pool(std::string_view pool);
size_t count_cards_by_pool_and_rarity(std::string_view pool, std::string_view rarity);

} // namespace engine
