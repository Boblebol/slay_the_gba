#pragma once
#include <cstdint>
#include <string_view>
#include <span>

namespace engine {

struct EventData {
    std::string_view id;
    std::string_view name;
    uint8_t option_count;
    std::string_view description;
};

const EventData* find_event(std::string_view id);
std::span<const EventData> get_all_events();

} // namespace engine
