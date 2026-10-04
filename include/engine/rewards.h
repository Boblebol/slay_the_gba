#pragma once
#include <cstdint>
#include <span>
#include <string_view>
#include "engine/character.h"
#include "engine/relic.h"

namespace engine {

// Random stream of the rewards and of the merchant's stock, apart from the run's own generator: what a room
// gives is drawn from a seed of its own (the map seed, the act, the floor and the room), so offering a potion
// or a relic never moves the sequence that draws the encounters, the shuffles and the cards.
class RewardRng {
public:
    explicit RewardRng(uint32_t seed);
    uint32_t next();
    // Uniform draw in [0, bound). `bound` must not be 0.
    uint32_t below(uint32_t bound) { return next() % bound; }

private:
    uint32_t state_;
};

// A potion the combat engine resolves. `any_hero` is false for a potion that only acts on the mechanic of one
// hero (orbs, Stars, Forge, Osty): another hero would drink it for nothing.
struct ActivePotion {
    std::string_view id;
    bool any_hero = true;
    CharacterId hero = CharacterId::IRONCLAD;
};

// Every potion CombatEngine::use_potion resolves: the only ones a fight or the merchant can give.
std::span<const ActivePotion> active_potions();
// True when `hero` can be offered the potion: resolved by the engine and useful to that hero.
bool potion_fits_hero(std::string_view potion_id, CharacterId hero);

// Drop rates, in percent. The reference gives the 40 % of the potion; the rest are the first game's values.
constexpr uint8_t POTION_REWARD_CHANCE = 40;
constexpr uint8_t POTION_COMMON_CHANCE = 65;
constexpr uint8_t POTION_UNCOMMON_CHANCE = 25; // the remaining 10 % are Rare
constexpr uint8_t RELIC_COMMON_CHANCE = 50;
constexpr uint8_t RELIC_UNCOMMON_CHANCE = 33;  // the remaining 17 % are Rare

// Where a relic comes from, which decides its rarity.
enum class RelicSource : uint8_t {
    ELITE, // Common, Uncommon or Rare, drawn with the rates above
    BOSS,  // Rare
    SHOP   // Shop rarity
};

// A potion drawn by rarity (Common, Uncommon, Rare) among the ones `hero` can be offered, never one of
// `excluded`. A rarity with no candidate left gives way to the next one. Empty when nothing can be drawn.
std::string_view draw_potion(RewardRng& rng, CharacterId hero, std::span<const std::string_view> excluded = {});

// A relic drawn by rarity among RunManager::active_relics(): shared or of `hero`, never one of `owned` nor of
// `excluded`. A rarity with no candidate left gives way to the next one. Empty when nothing can be drawn.
std::string_view draw_relic(RewardRng& rng, CharacterId hero, RelicSource source,
                            std::span<const std::string_view> owned,
                            std::span<const std::string_view> excluded = {});

// Prices of the merchant before their variation of 5 % up or down. 0 for a rarity that is never sold.
int16_t relic_base_price(RelicRarity rarity);
int16_t potion_base_price(std::string_view rarity);
// `base` moved by -5 % to +5 %.
int16_t vary_price(RewardRng& rng, int16_t base);

} // namespace engine
