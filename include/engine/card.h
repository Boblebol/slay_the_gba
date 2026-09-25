#pragma once

#include <cstdint>
#include <string_view>

namespace engine {

enum class CardId : uint8_t {
    NONE = 0,
    STRIKE,
    DEFEND,
    BASH,
    IRON_WAVE,
    CLEAVE,
    ANGER,
    COUNT
};

enum class CardType : uint8_t {
    ATTACK,
    SKILL,
    POWER
};

enum class TargetType : uint8_t {
    SINGLE_ENEMY,
    SELF,
    ALL_ENEMIES
};

struct CardDef {
    CardId id;
    std::string_view name;
    uint8_t cost;
    CardType type;
    TargetType target;
    uint8_t base_value; // Damage or Block
    uint8_t secondary_value; // e.g. Vulnerable turns
    std::string_view description;

    static const CardDef& get(CardId id);
};

} // namespace engine
