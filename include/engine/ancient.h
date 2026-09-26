#pragma once

#include <cstdint>
#include <string_view>
#include <span>

namespace engine {

enum class AncientId : uint8_t {
    NEOW = 0,
    DARV,
    OROBAS,
    PAEL,
    TEZCATARA,
    NONUPEIPE,
    TANX,
    VAKUU,
    COUNT
};

enum class RelicTier : uint8_t {
    TIER_S = 0,
    TIER_A,
    TIER_B,
    TIER_C,
    TIER_D,
    UNTIERED
};

struct AncientRelic {
    std::string_view id;
    std::string_view name;
    RelicTier tier;
    std::string_view description;
};

struct AncientSlot {
    std::string_view group_name;
    uint8_t picks_allowed;
    std::span<const AncientRelic> options;
};

struct Ancient {
    AncientId id;
    std::string_view slug;
    std::string_view name;
    std::string_view epithet;
    uint8_t act_mask; // Bit 0: Act 1, Bit 1: Act 2, Bit 2: Act 3
    std::span<const AncientSlot> slots;
};

// Accessor functions
const Ancient& get_ancient(AncientId id);
std::span<const Ancient> get_all_ancients();

} // namespace engine
