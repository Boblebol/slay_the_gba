#pragma once

#include <cstdint>
#include <string_view>

namespace engine {

enum class StatusType : uint8_t {
    NONE = 0,
    VULNERABLE, // Takes 50% more attack damage
    WEAK,       // Deals 25% less attack damage
    STRENGTH,   // Adds X damage to attacks
    RITUAL      // Gains X strength at end of turn
};

struct StatusEffect {
    StatusType type = StatusType::NONE;
    int16_t amount = 0;

    static std::string_view name(StatusType t) {
        switch (t) {
            case StatusType::VULNERABLE: return "Vulnerable";
            case StatusType::WEAK:       return "Weak";
            case StatusType::STRENGTH:   return "Strength";
            case StatusType::RITUAL:     return "Ritual";
            default:                     return "None";
        }
    }
};

} // namespace engine
