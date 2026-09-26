#pragma once
#include <cstdint>
#include <string_view>
#include <span>

namespace engine {

struct PotionData {
    std::string_view id;
    std::string_view name;
    std::string_view rarity;
    std::string_view target;
    std::string_view description;
};

const PotionData* find_potion(std::string_view id);
std::span<const PotionData> get_all_potions();

} // namespace engine
