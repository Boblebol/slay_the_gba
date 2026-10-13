// Monsters read from the monster database (src/engine/monster.cpp): the name, the HP (lowest, range, Ascension)
// and the numbers of the moves of every EnemyType come from its sheet.
#include <cassert>
#include <iostream>
#include <string_view>
#include "engine/entity.h"
#include "engine/monster.h"

using namespace engine;

namespace {

constexpr int ENEMY_TYPES = static_cast<int>(EnemyType::TEST_SUBJECT) + 1;

bool is_hand_written(EnemyType type) {
    return type == EnemyType::CULTIST || type == EnemyType::JAW_WORM || type == EnemyType::LOUSE ||
           type == EnemyType::GREMLIN_NOB;
}

// The monsters whose attacks keep their usual damage at Ascension, while their sheet gives more.
bool keeps_damage_at_ascension(EnemyType type) {
    switch (type) {
        case EnemyType::FUZZY_WURM_CRAWLER:
        case EnemyType::CROSSBOW_RUBY_RAIDER:
        case EnemyType::BRUTE_RUBY_RAIDER:
        case EnemyType::ASSASSIN_RUBY_RAIDER:
        case EnemyType::AXE_RUBY_RAIDER:
        case EnemyType::CALCIFIED_CULTIST:
        case EnemyType::DAMP_CULTIST:
        case EnemyType::SEWER_CLAM: return true;
        default: return false;
    }
}

// The Forgotten's Dread deals 13 damage (15 at Ascension) plus its Dexterity: its sheet gives it no damage.
bool has_unlisted_damage(EnemyType type) {
    return type == EnemyType::THE_FORGOTTEN;
}

bool damage_in_sheet(const MonsterData& sheet, int16_t damage, bool ascended) {
    for (const MonsterMoveData& move : sheet.moves) {
        if ((ascended ? move.damage_asc : move.damage) == damage) return true;
    }
    return false;
}

void test_every_type_has_a_sheet_but_the_hand_written_ones() {
    for (int i = 0; i < ENEMY_TYPES; ++i) {
        const EnemyType type = static_cast<EnemyType>(i);
        assert((monster_sheet(type) == nullptr) == is_hand_written(type));
    }
    assert(monster_sheet(EnemyType::CROSSBOW_RUBY_RAIDER) == find_monster("CROSSBOW_RUBY_RAIDER"));
    assert(monster_sheet(static_cast<EnemyType>(ENEMY_TYPES)) == nullptr);
    std::cout << "  every EnemyType has its sheet, but the four hand-written ones" << std::endl;
}

void test_name_and_hp_come_from_the_sheet() {
    for (int i = 0; i < ENEMY_TYPES; ++i) {
        const EnemyType type = static_cast<EnemyType>(i);
        const MonsterData* sheet = monster_sheet(type);
        if (sheet == nullptr) continue;
        Enemy enemy;
        enemy.init(type);
        assert(enemy.name == sheet->name);
        assert(enemy.max_hp == sheet->min_hp && enemy.hp == sheet->min_hp);
        // A monster whose HP vary draws them between the lowest and the highest of its sheet.
        const uint8_t spread = enemy.hp_spread();
        assert(spread == 0 || spread == sheet->max_hp - sheet->min_hp);
        for (uint32_t seed = 1; seed < 200; seed += 7) {
            enemy.roll_hp(seed);
            assert(enemy.max_hp >= sheet->min_hp && enemy.max_hp <= sheet->min_hp + spread);
            assert(enemy.hp == enemy.max_hp);
        }
        // A monster with its own Ascension HP takes the lowest of the sheet.
        Enemy fresh;
        fresh.init(type);
        const int16_t ascension = fresh.ascension_hp();
        assert(ascension == 0 || ascension == sheet->min_hp_asc);
        if (ascension > 0) {
            fresh.ascend(true, false);
            assert(fresh.max_hp == sheet->min_hp_asc && fresh.hp == sheet->min_hp_asc);
        }
    }
    std::cout << "  name, HP, HP range and Ascension HP come from the sheet" << std::endl;
}

void test_hand_written_monsters_keep_their_values() {
    Enemy enemy;
    enemy.init(EnemyType::JAW_WORM);
    assert(enemy.name == "Jaw Worm" && enemy.max_hp == 42 && enemy.hp_spread() == 0 && enemy.ascension_hp() == 0);
    std::cout << "  the hand-written monsters keep their values" << std::endl;
}

void test_sheet_moves() {
    Enemy enemy;
    enemy.init(EnemyType::CROSSBOW_RUBY_RAIDER);
    const MonsterMoveData& fire = enemy.sheet_move("FIRE");
    assert(fire.id == "FIRE" && fire.damage == 14 && fire.damage_asc == 16 && fire.hits == 1);
    assert(enemy.sheet_move("RELOAD").block == 3);
    assert(enemy.move_damage("FIRE") == 14);
    enemy.ascend(false, true);
    assert(enemy.move_damage("FIRE") == 16);
    // A move the sheet does not have: no damage, no Block, one hit.
    const MonsterMoveData& none = enemy.sheet_move("NOT_A_MOVE");
    assert(none.damage == 0 && none.damage_asc == 0 && none.block == 0 && none.hits == 1);
    enemy.init(EnemyType::JAW_WORM);
    assert(enemy.sheet_move("CHOMP").damage == 0);
    std::cout << "  moves are read by their identifier in the sheet" << std::endl;
}

// Every attack a monster announces over its first turns, with and without the damage of Ascension, deals the
// damage of one of the moves of its sheet.
void test_attack_damage_comes_from_the_sheet() {
    int checked = 0;
    for (int i = 0; i < ENEMY_TYPES; ++i) {
        const EnemyType type = static_cast<EnemyType>(i);
        const MonsterData* sheet = monster_sheet(type);
        if (sheet == nullptr) continue;
        for (bool ascended : { false, true }) {
            Enemy enemy;
            enemy.init(type);
            if (ascended) enemy.ascend(false, true);
            for (int turn = 1; turn <= 12; ++turn) {
                if (turn > 1) enemy.plan_turn(turn);
                const int16_t damage = enemy.current_intent.base_damage;
                if (damage <= 0 || has_unlisted_damage(type)) continue;
                const bool asc_values = ascended && !keeps_damage_at_ascension(type);
                if (!damage_in_sheet(*sheet, damage, asc_values)) {
                    std::cerr << "  " << sheet->id << " turn " << turn << (ascended ? " (Ascension)" : "")
                              << ": " << damage << " damage is not in its sheet" << std::endl;
                    assert(false);
                }
                ++checked;
            }
        }
    }
    assert(checked > 500);
    std::cout << "  attack damage of every monster comes from its sheet (" << checked << " intents)" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Monster sheets ===" << std::endl;
    test_every_type_has_a_sheet_but_the_hand_written_ones();
    test_name_and_hp_come_from_the_sheet();
    test_hand_written_monsters_keep_their_values();
    test_sheet_moves();
    test_attack_damage_comes_from_the_sheet();
    std::cout << "All monster sheet tests passed." << std::endl;
    return 0;
}
