#include "engine/rewards.h"
#include "engine/potion.h"
#include "engine/run.h"

namespace engine {

namespace {

using namespace std::string_view_literals;

// Avalanche of a 32-bit value: close seeds (two floors of the same map) give unrelated streams.
constexpr uint32_t mix(uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

constexpr ActivePotion ACTIVE_POTIONS[] = {
    { "ASHWATER" },
    { "BLESSING_OF_THE_FORGE" }, { "BLOCK_POTION" }, { "BLOOD_POTION" },
    { "BONE_BREW", false, CharacterId::NECROBINDER },
    { "BOTTLED_POTENTIAL" }, { "CLARITY" }, { "CUNNING_POTION" }, { "CURE_ALL" }, { "DEXTERITY_POTION" },
    { "DISTILLED_CHAOS" }, { "DROPLET_OF_PRECOGNITION" }, { "DUPLICATOR" }, { "ENERGY_POTION" },
    { "ESSENCE_OF_DARKNESS", false, CharacterId::DEFECT },
    { "EXPLOSIVE_AMPOULE" }, { "FIRE_POTION" }, { "FLEX_POTION" },
    { "FOCUS_POTION", false, CharacterId::DEFECT },
    { "FORTIFIER" }, { "FYSH_OIL" }, { "GAMBLERS_BREW" }, { "GHOST_IN_A_JAR" }, { "GLOWWATER_POTION" }, { "HEART_OF_IRON" },
    { "KINGS_COURAGE", false, CharacterId::REGENT },
    { "LIQUID_BRONZE" }, { "LUCKY_TONIC" }, { "MAZALETHS_GIFT" }, { "POISON_POTION" }, { "POTION_OF_BINDING" },
    { "POTION_OF_CAPACITY", false, CharacterId::DEFECT },
    { "POTION_OF_DOOM" },
    { "POT_OF_GHOULS", false, CharacterId::NECROBINDER },
    { "POWDERED_DEMISE" },
    { "RADIANT_TINCTURE" }, { "REGEN_POTION" }, { "SHACKLING_POTION" },
    { "SHIP_IN_A_BOTTLE" }, { "SPEED_POTION" },
    { "STAR_POTION", false, CharacterId::REGENT },
    { "STRENGTH_POTION" }, { "SWIFT_POTION" }, { "VULNERABLE_POTION" }, { "WEAK_POTION" },
};

// Rarities a draw falls back on, in this order, when the one it rolled has no candidate left.
constexpr std::string_view POTION_RARITIES[] = { "Common"sv, "Uncommon"sv, "Rare"sv };
constexpr RelicRarity RELIC_RARITIES[] = { RelicRarity::COMMON, RelicRarity::UNCOMMON, RelicRarity::RARE };

bool contains(std::span<const std::string_view> ids, std::string_view id) {
    for (std::string_view other : ids) {
        if (other == id) return true;
    }
    return false;
}

} // namespace

RewardRng::RewardRng(uint32_t seed) : state_(mix(seed)) {
    if (state_ == 0) state_ = 0x9E3779B9u; // xorshift never leaves 0
}

uint32_t RewardRng::next() {
    state_ ^= state_ << 13;
    state_ ^= state_ >> 17;
    state_ ^= state_ << 5;
    return mix(state_);
}

std::span<const ActivePotion> active_potions() {
    return ACTIVE_POTIONS;
}

bool potion_fits_hero(std::string_view potion_id, CharacterId hero) {
    for (const ActivePotion& potion : ACTIVE_POTIONS) {
        if (potion.id == potion_id) return potion.any_hero || potion.hero == hero;
    }
    return false;
}

std::string_view draw_potion(RewardRng& rng, CharacterId hero, std::span<const std::string_view> excluded) {
    uint32_t roll = rng.below(100);
    size_t rolled = roll < POTION_COMMON_CHANCE ? 0 : (roll < POTION_COMMON_CHANCE + POTION_UNCOMMON_CHANCE ? 1 : 2);

    // -1: the rarity rolled; then Common, Uncommon and Rare for a rarity that has nothing left.
    for (int attempt = -1; attempt < 3; ++attempt) {
        std::string_view rarity = POTION_RARITIES[attempt < 0 ? rolled : static_cast<size_t>(attempt)];
        auto is_candidate = [&](const ActivePotion& potion) {
            if (!(potion.any_hero || potion.hero == hero) || contains(excluded, potion.id)) return false;
            const PotionData* data = find_potion(potion.id);
            return data && data->rarity == rarity;
        };
        uint32_t count = 0;
        for (const ActivePotion& potion : ACTIVE_POTIONS) {
            if (is_candidate(potion)) ++count;
        }
        if (count == 0) continue;

        uint32_t pick = rng.below(count);
        for (const ActivePotion& potion : ACTIVE_POTIONS) {
            if (is_candidate(potion) && pick-- == 0) return potion.id;
        }
    }
    return {};
}

std::string_view draw_relic(RewardRng& rng, CharacterId hero, RelicSource source,
                            std::span<const std::string_view> owned, std::span<const std::string_view> excluded) {
    uint32_t roll = rng.below(100);
    RelicRarity rolled = roll < RELIC_COMMON_CHANCE
                             ? RelicRarity::COMMON
                             : (roll < RELIC_COMMON_CHANCE + RELIC_UNCOMMON_CHANCE ? RelicRarity::UNCOMMON
                                                                                    : RelicRarity::RARE);
    if (source == RelicSource::BOSS) rolled = RelicRarity::RARE;
    if (source == RelicSource::SHOP) rolled = RelicRarity::SHOP;

    std::string_view hero_pool = get_character(hero).english_name;
    for (int attempt = -1; attempt < 3; ++attempt) {
        // A boss falls back on the rarest relic left, the others on the most common one.
        RelicRarity rarity = attempt < 0 ? rolled : RELIC_RARITIES[source == RelicSource::BOSS ? 2 - attempt : attempt];
        auto is_candidate = [&](std::string_view id) {
            const RelicData* relic = find_relic(id);
            return relic && relic->rarity == rarity && (relic->pool == "Shared" || relic->pool == hero_pool) &&
                   !contains(owned, id) && !contains(excluded, id);
        };
        uint32_t count = 0;
        for (std::string_view id : RunManager::active_relics()) {
            if (is_candidate(id)) ++count;
        }
        if (count == 0) continue;

        uint32_t pick = rng.below(count);
        for (std::string_view id : RunManager::active_relics()) {
            if (is_candidate(id) && pick-- == 0) return id;
        }
    }
    return {};
}

int16_t relic_base_price(RelicRarity rarity) {
    switch (rarity) {
        case RelicRarity::COMMON: return 150;
        case RelicRarity::UNCOMMON: return 250;
        case RelicRarity::RARE: return 300;
        case RelicRarity::SHOP: return 150;
        default: return 0;
    }
}

int16_t potion_base_price(std::string_view rarity) {
    if (rarity == "Common") return 50;
    if (rarity == "Uncommon") return 75;
    if (rarity == "Rare") return 100;
    return 0;
}

int16_t vary_price(RewardRng& rng, int16_t base) {
    return static_cast<int16_t>(base * static_cast<int32_t>(95 + rng.below(11)) / 100);
}

} // namespace engine
