#pragma once
#include <cstdint>
#include <string_view>
#include "engine/fixed_vector.h"

namespace engine {

enum class RoomType : uint8_t {
    MONSTER = 0,
    EVENT,
    SHOP,
    REST_SITE,
    TREASURE,
    ELITE,
    BOSS
};

struct MapNode {
    uint8_t floor;      // 0 to 15 (15 = Boss)
    uint8_t lane;       // 0 to 2 (Left, Middle, Right)
    RoomType type;
    uint8_t children[3];
    uint8_t children_count;
};

constexpr size_t MAX_MAP_NODES = 50;

class MapGraph {
public:
    FixedVector<MapNode, MAX_MAP_NODES> nodes;
    int boss_node_index = -1;

    void generate(uint8_t act, uint32_t seed);
    FixedVector<int, 3> get_starting_nodes() const;
    FixedVector<int, 3> get_next_nodes(int current_node_index) const;
    std::string_view get_room_icon(RoomType type) const;
    std::string_view get_room_name(RoomType type) const;
};

} // namespace engine
