#pragma once
#include <cstdint>
#include <string_view>
#include <span>

namespace engine {

struct MonsterMoveData {
    std::string_view id;
    std::string_view name;
    int16_t damage;
    int16_t damage_asc;
    int16_t block;
    uint8_t hits;
};

struct MonsterData {
    std::string_view id;
    std::string_view name;
    int16_t min_hp;
    int16_t max_hp;
    int16_t min_hp_asc;
    int16_t max_hp_asc;
    std::string_view encounter_type;
    std::span<const MonsterMoveData> moves;
};

const MonsterData* find_monster(std::string_view id);
std::span<const MonsterData> get_all_monsters();

} // namespace engine
