#include "engine/navigation.h"
#include <algorithm>

namespace engine {

MapNavigator::MapNavigator(RunManager& r) : run(r) {
    reset_to_player_floor();
}

void MapNavigator::reset_to_player_floor() {
    if (run.current_node_index >= 0 && static_cast<size_t>(run.current_node_index) < run.map.nodes.size()) {
        scroll_floor = run.map.nodes[run.current_node_index].floor;
    } else {
        scroll_floor = 0;
    }
    selected_route_index = 0;
}

void MapNavigator::scroll_up() {
    if (scroll_floor < 15) {
        ++scroll_floor;
    }
}

void MapNavigator::scroll_down() {
    if (scroll_floor > 0) {
        --scroll_floor;
    }
}

FixedVector<int, 3> MapNavigator::get_available_routes() const {
    return run.get_available_rooms();
}

void MapNavigator::next_route() {
    auto routes = get_available_routes();
    if (!routes.empty()) {
        selected_route_index = (selected_route_index + 1) % static_cast<int>(routes.size());
    }
}

void MapNavigator::prev_route() {
    auto routes = get_available_routes();
    if (!routes.empty()) {
        selected_route_index = (selected_route_index - 1 + static_cast<int>(routes.size())) % static_cast<int>(routes.size());
    }
}

int MapNavigator::get_selected_target_node() const {
    auto routes = get_available_routes();
    if (routes.empty()) return -1;
    int idx = std::clamp(selected_route_index, 0, static_cast<int>(routes.size()) - 1);
    return routes[idx];
}

bool MapNavigator::select_current_route() {
    int target = get_selected_target_node();
    if (target >= 0) {
        run.enter_room(target);
        reset_to_player_floor();
        return true;
    }
    return false;
}

void MapNavigator::toggle_deck_view() {
    deck_view_active = !deck_view_active;
    deck_scroll_index = 0;
}

void MapNavigator::deck_scroll_up() {
    if (deck_scroll_index > 0) {
        --deck_scroll_index;
    }
}

void MapNavigator::deck_scroll_down() {
    if (deck_scroll_index + 1 < static_cast<int>(run.master_deck.size())) {
        ++deck_scroll_index;
    }
}

int MapNavigator::get_current_floor() const {
    if (run.current_node_index >= 0 && static_cast<size_t>(run.current_node_index) < run.map.nodes.size()) {
        return run.map.nodes[run.current_node_index].floor + 1;
    }
    return 1;
}

std::string_view MapNavigator::get_room_icon(RoomType type) const {
    return run.map.get_room_icon(type);
}

std::string_view MapNavigator::get_room_label(RoomType type) const {
    return run.map.get_room_name(type);
}

} // namespace engine
