#include "engine/map.h"

namespace engine {

namespace {

constexpr uint8_t FLOOR_COUNT = 15;  // floors 0 to 14, the boss sits on floor 15
constexpr uint8_t LANE_COUNT = 3;    // 0: Left, 1: Center, 2: Right
constexpr uint8_t PATH_COUNT = 4;    // random walks traced from the bottom to the top
constexpr uint8_t TREASURE_FLOOR = 7; // mid-act chest
constexpr uint8_t PRE_BOSS_FLOOR = 14; // single campfire every path funnels into
constexpr uint8_t MIN_ELITE_REST_FLOOR = 5;

struct Xorshift {
    uint32_t state;

    uint32_t next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    uint32_t below(uint32_t bound) { return next() % bound; }
};

// Room types that must never repeat along a path nor among the choices offered by one room.
bool is_unique_along_path(RoomType type) {
    return type == RoomType::ELITE || type == RoomType::REST_SITE || type == RoomType::SHOP;
}

struct Layout {
    bool used[FLOOR_COUNT][LANE_COUNT] = {};
    // edge[f][a][b]: room (f, a) leads to room (f + 1, b).
    bool edge[FLOOR_COUNT - 1][LANE_COUNT][LANE_COUNT] = {};
    RoomType type[FLOOR_COUNT][LANE_COUNT] = {};

    // Two edges between the same floors cross when they swap sides (a < c but b > d).
    bool crosses(uint8_t floor, uint8_t from, uint8_t to) const {
        for (uint8_t a = 0; a < LANE_COUNT; ++a) {
            for (uint8_t b = 0; b < LANE_COUNT; ++b) {
                if (edge[floor][a][b] && ((a < from && b > to) || (a > from && b < to))) {
                    return true;
                }
            }
        }
        return false;
    }

    void trace_path(Xorshift& rng, uint8_t start_lane) {
        uint8_t lane = start_lane;
        used[0][lane] = true;

        for (uint8_t floor = 0; floor + 1 < FLOOR_COUNT; ++floor) {
            uint8_t next;
            if (floor + 1 == PRE_BOSS_FLOOR) {
                next = 1; // every lane is adjacent to the center: the funnel is always reachable
            } else {
                int lo = lane > 0 ? lane - 1 : 0;
                int hi = lane + 1 < LANE_COUNT ? lane + 1 : LANE_COUNT - 1;
                next = static_cast<uint8_t>(lo + static_cast<int>(rng.below(static_cast<uint32_t>(hi - lo + 1))));
                // Going straight up never crosses (edges only span one lane).
                if (crosses(floor, lane, next)) next = lane;
            }
            edge[floor][lane][next] = true;
            used[floor + 1][next] = true;
            lane = next;
        }
    }

    bool allows(uint8_t floor, uint8_t lane, RoomType candidate) const {
        if ((candidate == RoomType::ELITE || candidate == RoomType::REST_SITE) && floor < MIN_ELITE_REST_FLOOR) {
            return false;
        }
        // The pre-boss campfire already follows: no back-to-back rest.
        if (candidate == RoomType::REST_SITE && floor + 1 == PRE_BOSS_FLOOR) {
            return false;
        }
        if (!is_unique_along_path(candidate) || floor == 0) {
            return true;
        }
        for (uint8_t parent = 0; parent < LANE_COUNT; ++parent) {
            if (!edge[floor - 1][parent][lane]) continue;
            if (type[floor - 1][parent] == candidate) return false;
            // Siblings are assigned left to right: only the ones before this room are known.
            for (uint8_t sibling = 0; sibling < lane; ++sibling) {
                if (edge[floor - 1][parent][sibling] && type[floor][sibling] == candidate) return false;
            }
        }
        return true;
    }

    void assign_types(Xorshift& rng, uint8_t act) {
        uint8_t assignable = 0;
        for (uint8_t floor = 0; floor < FLOOR_COUNT; ++floor) {
            for (uint8_t lane = 0; lane < LANE_COUNT; ++lane) {
                if (!used[floor][lane]) continue;
                if (floor == 0) {
                    type[floor][lane] = RoomType::MONSTER;
                } else if (floor == TREASURE_FLOOR) {
                    type[floor][lane] = RoomType::TREASURE;
                } else if (floor == PRE_BOSS_FLOOR) {
                    type[floor][lane] = RoomType::REST_SITE;
                } else {
                    ++assignable;
                }
            }
        }

        // Shuffled room pool (Slay the Spire style): fixed shares guarantee events, shops and
        // campfires on every map, elites grow with the act. Remaining slots are monsters.
        auto share = [assignable](int percent) { return static_cast<uint8_t>((assignable * percent + 50) / 100); };
        FixedVector<RoomType, FLOOR_COUNT * LANE_COUNT> pool;
        auto add = [&pool](RoomType room, uint8_t count) {
            for (uint8_t i = 0; i < count; ++i) pool.push_back(room);
        };
        add(RoomType::EVENT, share(22));
        add(RoomType::REST_SITE, share(12));
        add(RoomType::ELITE, share(8 + 2 * (act > 0 ? act - 1 : 0)));
        add(RoomType::SHOP, share(8));
        while (pool.size() < assignable) pool.push_back(RoomType::MONSTER);

        for (size_t i = pool.size(); i > 1; --i) {
            size_t j = rng.below(static_cast<uint32_t>(i));
            RoomType tmp = pool[i - 1];
            pool[i - 1] = pool[j];
            pool[j] = tmp;
        }

        for (uint8_t floor = 1; floor < FLOOR_COUNT; ++floor) {
            if (floor == TREASURE_FLOOR || floor == PRE_BOSS_FLOOR) continue;
            for (uint8_t lane = 0; lane < LANE_COUNT; ++lane) {
                if (!used[floor][lane]) continue;
                type[floor][lane] = RoomType::MONSTER; // fallback when no pooled room fits here
                for (size_t i = 0; i < pool.size(); ++i) {
                    if (allows(floor, lane, pool[i])) {
                        type[floor][lane] = pool[i];
                        pool.erase(i);
                        break;
                    }
                }
            }
        }
    }
};

} // namespace

void MapGraph::generate(uint8_t act, uint32_t seed) {
    nodes.clear();
    Xorshift rng{ (seed ? seed : 777u) ^ (act * 0x9E3779B9u) };
    if (rng.state == 0) rng.state = 777;

    Layout layout;
    // The first two paths start on different lanes so the run always opens on a real choice.
    uint8_t first_lane = static_cast<uint8_t>(rng.below(LANE_COUNT));
    uint8_t second_lane = static_cast<uint8_t>((first_lane + 1 + rng.below(LANE_COUNT - 1)) % LANE_COUNT);
    layout.trace_path(rng, first_lane);
    layout.trace_path(rng, second_lane);
    for (uint8_t p = 2; p < PATH_COUNT; ++p) {
        layout.trace_path(rng, static_cast<uint8_t>(rng.below(LANE_COUNT)));
    }
    layout.assign_types(rng, act);

    // Flatten the grid floor by floor, left to right: children indices then follow the screen order.
    uint8_t index_of[FLOOR_COUNT][LANE_COUNT] = {};
    for (uint8_t floor = 0; floor < FLOOR_COUNT; ++floor) {
        for (uint8_t lane = 0; lane < LANE_COUNT; ++lane) {
            if (!layout.used[floor][lane]) continue;
            MapNode node{};
            node.floor = floor;
            node.lane = lane;
            node.type = layout.type[floor][lane];
            index_of[floor][lane] = static_cast<uint8_t>(nodes.size());
            nodes.push_back(node);
        }
    }

    MapNode boss{};
    boss.floor = FLOOR_COUNT;
    boss.lane = 1;
    boss.type = RoomType::BOSS;
    boss_node_index = static_cast<int>(nodes.size());
    nodes.push_back(boss);

    for (MapNode& node : nodes) {
        if (node.floor == PRE_BOSS_FLOOR) {
            node.children[node.children_count++] = static_cast<uint8_t>(boss_node_index);
        } else if (node.floor < PRE_BOSS_FLOOR) {
            for (uint8_t next = 0; next < LANE_COUNT; ++next) {
                if (layout.edge[node.floor][node.lane][next]) {
                    node.children[node.children_count++] = index_of[node.floor + 1][next];
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
