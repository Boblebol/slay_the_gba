#include "engine/map.h"

namespace engine {

void MapGraph::generate(uint8_t act, uint32_t seed) {
    (void)act;
    nodes.clear();
    uint32_t rng = seed ? seed : 777;
    auto rnd = [&rng]() -> uint32_t {
        rng ^= rng << 13;
        rng ^= rng >> 17;
        rng ^= rng << 5;
        return rng;
    };

    // 15 floors (0 to 14) + Boss (floor 15)
    // 3 lanes (0: Left, 1: Center, 2: Right)
    for (uint8_t f = 0; f < 15; ++f) {
        for (uint8_t lane = 0; lane < 3; ++lane) {
            MapNode node;
            node.floor = f;
            node.lane = lane;
            node.children_count = 0;

            if (f == 0) {
                node.type = RoomType::MONSTER; // Floor 1 always monster
            } else if (f == 6) {
                node.type = (lane == 1) ? RoomType::ELITE : RoomType::REST_SITE;
            } else if (f == 7) {
                node.type = RoomType::TREASURE; // Mid-act chest
            } else if (f == 14) {
                node.type = RoomType::REST_SITE; // Campfire before boss
            } else {
                uint32_t r = rnd() % 100;
                if (f >= 5 && r < 20) {
                    node.type = RoomType::ELITE;
                } else if (r < 55) {
                    node.type = RoomType::MONSTER;
                } else if (r < 75) {
                    node.type = RoomType::EVENT;
                } else if (r < 90) {
                    node.type = RoomType::REST_SITE;
                } else {
                    node.type = RoomType::SHOP;
                }
            }

            nodes.push_back(node);
        }
    }

    // Add Boss node (Floor 15, lane 1)
    MapNode boss;
    boss.floor = 15;
    boss.lane = 1;
    boss.type = RoomType::BOSS;
    boss.children_count = 0;
    boss_node_index = static_cast<int>(nodes.size());
    nodes.push_back(boss);

    // Build connections between floors
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].floor == 14) {
            // Floor 14 connects to Boss
            nodes[i].children[0] = boss_node_index;
            nodes[i].children_count = 1;
        } else if (nodes[i].floor < 14) {
            uint8_t next_f = nodes[i].floor + 1;
            uint8_t cur_lane = nodes[i].lane;

            // Connect to same lane on next floor
            for (size_t j = 0; j < nodes.size(); ++j) {
                if (nodes[j].floor == next_f) {
                    if (nodes[j].lane == cur_lane || (cur_lane > 0 && nodes[j].lane == cur_lane - 1)) {
                        if (nodes[i].children_count < 3) {
                            nodes[i].children[nodes[i].children_count++] = static_cast<uint8_t>(j);
                        }
                    }
                }
            }
        }
    }
}

FixedVector<int, 3> MapGraph::get_starting_nodes() const {
    FixedVector<int, 3> starts;
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].floor == 0) {
            starts.push_back(static_cast<int>(i));
        }
    }
    return starts;
}

FixedVector<int, 3> MapGraph::get_next_nodes(int current_node_index) const {
    FixedVector<int, 3> nexts;
    if (current_node_index < 0 || static_cast<size_t>(current_node_index) >= nodes.size()) {
        return get_starting_nodes();
    }
    const auto& node = nodes[current_node_index];
    for (uint8_t i = 0; i < node.children_count; ++i) {
        nexts.push_back(node.children[i]);
    }
    return nexts;
}

std::string_view MapGraph::get_room_icon(RoomType type) const {
    switch (type) {
        case RoomType::MONSTER:   return "[Combat]";
        case RoomType::ELITE:     return "[ELITE]";
        case RoomType::REST_SITE: return "[Feu]";
        case RoomType::SHOP:      return "[Shop]";
        case RoomType::TREASURE:  return "[Coffre]";
        case RoomType::EVENT:     return "[Event ?]";
        case RoomType::BOSS:      return "[BOSS]";
        default:                  break;
    }
    return "[?]";
}

std::string_view MapGraph::get_room_name(RoomType type) const {
    switch (type) {
        case RoomType::MONSTER:   return "Combat de Monstre";
        case RoomType::ELITE:     return "Combat d'Élite";
        case RoomType::REST_SITE: return "Feu de Camp";
        case RoomType::SHOP:      return "Marchand de la Tour";
        case RoomType::TREASURE:  return "Salle au Trésor";
        case RoomType::EVENT:     return "Événement Mystère";
        case RoomType::BOSS:      return "Gardien de l'Étage (Boss)";
        default:                  break;
    }
    return "Inconnu";
}

} // namespace engine
