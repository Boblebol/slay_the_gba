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
    RITUAL,       // Gains X strength at end of turn
    FOCUS,        // Added to what the orbs do (Defect), can be negative
    DOOM          // Whoever has at least as much Doom as HP dies at the end of its turn
};

enum class OrbType : uint8_t {
    NONE = 0,
    LIGHTNING, // Passive: 3 dmg / Evoke: 8 dmg
    FROST,     // Passive: 2 block / Evoke: 5 block
    DARK,      // Passive: +6 charge / Evoke: deal accumulated charge
    PLASMA     // Passive: +1 Energy / Evoke: +2 Energy
};

constexpr size_t MAX_ORB_SLOTS = 10;
constexpr uint8_t STARTING_ORB_SLOTS = 3;

// Orb values before Focus.
constexpr int16_t LIGHTNING_PASSIVE = 3;
constexpr int16_t LIGHTNING_EVOKE = 8;
constexpr int16_t FROST_PASSIVE = 2;
constexpr int16_t FROST_EVOKE = 5;
constexpr int16_t DARK_PASSIVE = 6; // also the charge a Dark orb starts with
constexpr int16_t PLASMA_PASSIVE = 1;
constexpr int16_t PLASMA_EVOKE = 2;

// An orb in one of the Defect's slots. Slot 0 is the oldest orb: the "rightmost" one of the card texts, evoked
// first when no slot is free. The last slot is the newest, the "leftmost".
struct Orb {
    OrbType type = OrbType::NONE;
    int16_t charge = 0; // damage stored by a Dark orb

    constexpr Orb() = default;
    constexpr Orb(OrbType t) : type(t) {}
    constexpr operator OrbType() const { return type; }
    constexpr bool operator==(OrbType other) const { return type == other; }
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
            case StatusType::FOCUS:        return "Focus";
            case StatusType::DOOM:         return "Doom";
            default:                       return "None";
        }
    }
};

} // namespace engine
