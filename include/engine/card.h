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
    METALLICIZE,
    // Silent
    NEUTRALIZE,
    SURVIVOR,
    POISON_STAB,
    DEADLY_POISON,
    // Defect
    ZAP,
    DUALCAST,
    // Regent
    FALLING_STAR,
    VENERATE,
    // Necrobinder
    UNLEASH,
    BODYGUARD,
    COUNT
};

struct DeckCard {
    CardId id = CardId::NONE;
    bool upgraded = false;

    constexpr DeckCard() = default;
    constexpr DeckCard(CardId i, bool up = false) : id(i), upgraded(up) {}
    constexpr operator CardId() const { return id; }
    constexpr bool operator==(CardId other) const { return id == other; }
    constexpr bool operator==(const DeckCard& other) const { return id == other.id && upgraded == other.upgraded; }
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

    // Upgraded stats
    std::string_view upgraded_name;
    uint8_t upgraded_cost;
    uint8_t upgraded_base_value;
    uint8_t upgraded_secondary_value;
    std::string_view upgraded_description;

    static const CardDef& get(CardId id);
    static std::string_view get_display_name(CardId id, bool upgraded) {
        const auto& c = get(id);
        return upgraded ? c.upgraded_name : c.name;
    }
    static uint8_t get_cost(CardId id, bool upgraded) {
        const auto& c = get(id);
        return upgraded ? c.upgraded_cost : c.cost;
    }
    static uint8_t get_value(CardId id, bool upgraded) {
        const auto& c = get(id);
        return upgraded ? c.upgraded_base_value : c.base_value;
    }
    static uint8_t get_secondary(CardId id, bool upgraded) {
        const auto& c = get(id);
        return upgraded ? c.upgraded_secondary_value : c.secondary_value;
    }
    static std::string_view get_desc(CardId id, bool upgraded) {
        const auto& c = get(id);
        return upgraded ? c.upgraded_description : c.description;
    }
};

} // namespace engine
