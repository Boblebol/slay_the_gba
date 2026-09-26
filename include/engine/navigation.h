#pragma once

#include <cstdint>
#include <string_view>
#include "engine/fixed_vector.h"
#include "engine/run.h"

namespace engine {

struct MapViewNode {
    uint8_t floor;
    uint8_t lane;
    RoomType type;
    bool is_current;
    bool is_reachable;
    bool is_selected;
};

class MapNavigator {
public:
    RunManager& run;
    int scroll_floor = 0;       // Floor currently in view (0 to 15)
    int selected_route_index = 0; // Index in run.get_available_rooms()
    bool deck_view_active = false;
    int deck_scroll_index = 0;

    explicit MapNavigator(RunManager& r);

    // Map navigation
    void scroll_up();
    void scroll_down();
    void next_route();
    void prev_route();
    bool select_current_route();
    void reset_to_player_floor();

    // Available routes from current position
    FixedVector<int, 3> get_available_routes() const;
    int get_selected_target_node() const;

    // Deck viewing
    void toggle_deck_view();
    void deck_scroll_up();
    void deck_scroll_down();

    // Information helpers
    int get_current_floor() const;
    int get_max_floors() const { return 16; }
    std::string_view get_room_icon(RoomType type) const;
    std::string_view get_room_label(RoomType type) const;
};

} // namespace engine
