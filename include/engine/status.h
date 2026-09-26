#pragma once

#include <cstdint>
#include <string_view>

namespace engine {

enum class StatusType : uint8_t {
    NONE = 0,
    VULNERABLE,   // Takes 50% more attack damage
    WEAK,         // Deals 25% less attack damage
    STRENGTH,     // Adds X damage to attacks
    DEXTERITY,    // Adds X block to skill cards
    POISON,       // Takes X direct damage each turn, decreases by 1
    METALLICIZE,  // Gains X block at end of turn
    PLATED_ARMOR, // Gains X block at end of turn, -1 when damaged
    ARTIFACT,     // Blocks next debuff
    RITUAL        // Gains X strength at end of turn
};

enum class OrbType : uint8_t {
    NONE = 0,
    LIGHTNING, // Passive: 3 dmg / Evoke: 8 dmg
    FROST,     // Passive: 2 block / Evoke: 5 block
    DARK,      // Passive: +6 charge / Evoke: deal accumulated charge
    PLASMA     // Passive: +1 Energy / Evoke: +2 Energy
};

struct StatusEffect {
    StatusType type = StatusType::NONE;
    int16_t amount = 0;

    static std::string_view name(StatusType t) {
        switch (t) {
            case StatusType::VULNERABLE:   return "Vulnerable";
            case StatusType::WEAK:         return "Weak";
            case StatusType::STRENGTH:     return "Strength";
            case StatusType::DEXTERITY:    return "Dexterity";
            case StatusType::POISON:       return "Poison";
            case StatusType::METALLICIZE:  return "Metallicize";
            case StatusType::PLATED_ARMOR: return "Plated Armor";
            case StatusType::ARTIFACT:     return "Artifact";
            case StatusType::RITUAL:       return "Ritual";
            default:                       return "None";
        }
    }
};

} // namespace engine
