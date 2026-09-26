#pragma once
#include <cstdint>
#include <string_view>
#include <span>

namespace engine {

enum class RelicRarity : uint8_t {
    STARTER = 0,
    COMMON,
    UNCOMMON,
    RARE,
    BOSS,
    SHOP,
    EVENT,
    ANCIENT,
    UNKNOWN
};

struct RelicData {
    std::string_view id;
    std::string_view name;
    RelicRarity rarity;
    std::string_view pool;
    std::string_view description;
};

const RelicData* find_relic(std::string_view id);
std::span<const RelicData> get_all_relics();

} // namespace engine
