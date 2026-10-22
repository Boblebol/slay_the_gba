// Procedural animation of enemies and bosses: rest, attack, hit taken, death (#17).
#include <cassert>
#include <iostream>
#include "engine/enemy_anim.h"

using namespace engine;

namespace {

constexpr EnemyMoveStyle ALL_STYLES[] = { EnemyMoveStyle::STRIKE, EnemyMoveStyle::GUARD, EnemyMoveStyle::POWER_UP,
                                          EnemyMoveStyle::CURSE, EnemyMoveStyle::OTHER };

bool within_box(const EnemyPose& pose) {
    return pose.sx > 0 && pose.sx <= ANIM_SCALE_ONE && pose.sy > 0 && pose.sy <= ANIM_SCALE_ONE;
}

// Each intent maps to the move it plays: every attack variant strikes.
void test_move_style_follows_intent() {
    assert(move_style(EnemyIntentType::ATTACK) == EnemyMoveStyle::STRIKE);
    assert(move_style(EnemyIntentType::ATTACK_DEFEND) == EnemyMoveStyle::STRIKE);
    assert(move_style(EnemyIntentType::ATTACK_DEBUFF) == EnemyMoveStyle::STRIKE);
    assert(move_style(EnemyIntentType::ATTACK_BUFF) == EnemyMoveStyle::STRIKE);
    assert(move_style(EnemyIntentType::DEFEND) == EnemyMoveStyle::GUARD);
    assert(move_style(EnemyIntentType::BUFF) == EnemyMoveStyle::POWER_UP);
    assert(move_style(EnemyIntentType::HEAL) == EnemyMoveStyle::POWER_UP);
    assert(move_style(EnemyIntentType::DEBUFF) == EnemyMoveStyle::CURSE);
    assert(move_style(EnemyIntentType::STATUS) == EnemyMoveStyle::CURSE);
    assert(move_style(EnemyIntentType::STUN) == EnemyMoveStyle::OTHER);
    assert(move_style(EnemyIntentType::SUMMON) == EnemyMoveStyle::OTHER);
    std::cout << "[PASS] Enemy animation: the move played follows the intent" << std::endl;
}

// Idle: the body breathes (it moves and squashes over a cycle), a little, and an attack intent breathes
// faster and deeper than a plain idle.
void test_idle_breathes_and_telegraphs() {
    for (EnemyMoveStyle style : ALL_STYLES) {
        int min_dy = 100, max_dy = -100, min_sy = 1000, max_sy = 0;

        for (int frame = 0; frame < 240; ++frame) {
            EnemyPose pose = idle_pose(style, frame);
            assert(within_box(pose));
            assert(pose.dx >= -3 && pose.dx <= 3 && pose.dy >= -3 && pose.dy <= 3);
            min_dy = pose.dy < min_dy ? pose.dy : min_dy;
            max_dy = pose.dy > max_dy ? pose.dy : max_dy;
            min_sy = pose.sy < min_sy ? pose.sy : min_sy;
            max_sy = pose.sy > max_sy ? pose.sy : max_sy;
        }

        assert(max_dy > min_dy);
        assert(style == EnemyMoveStyle::POWER_UP || max_sy > min_sy);
    }

    int strike_low = ANIM_SCALE_ONE, other_low = ANIM_SCALE_ONE;

    for (int frame = 0; frame < 144; ++frame) {
        int a = idle_pose(EnemyMoveStyle::STRIKE, frame).sy;
        int b = idle_pose(EnemyMoveStyle::OTHER, frame).sy;
        strike_low = a < strike_low ? a : strike_low;
        other_low = b < other_low ? b : other_low;
    }

    assert(strike_low < other_low);
    // A buff about to come stands tall: narrower, never squashed.
    assert(idle_pose(EnemyMoveStyle::POWER_UP, 0).sx < ANIM_SCALE_ONE);
    // A crowded row keeps the breath but drops the squash (no affine sprite).
    for (int frame = 0; frame < 100; ++frame) {
        assert(!idle_pose(EnemyMoveStyle::STRIKE, frame, false).scaled());
    }
    assert(idle_squash_allowed(1) && idle_squash_allowed(3) && !idle_squash_allowed(4) && !idle_squash_allowed(6));
    std::cout << "[PASS] Enemy animation: idle breathes and telegraphs its intent" << std::endl;
}

// Attack: a strike leans back first, reaches the hero (left) at the hit frame, and comes back to its place.
void test_strike_lunges_at_the_hit_frame() {
    EnemyPose windup = attack_pose(EnemyMoveStyle::STRIKE, 3);
    assert(windup.dx > 0 && windup.sy < ANIM_SCALE_ONE);
    EnemyPose hit = attack_pose(EnemyMoveStyle::STRIKE, ENEMY_ATTACK_FRAMES / 2);
    assert(hit.dx <= -24);

    for (int frame = 0; frame <= ENEMY_ATTACK_FRAMES; ++frame) {
        assert(attack_pose(EnemyMoveStyle::STRIKE, frame).dx >= hit.dx);
    }

    EnemyPose done = attack_pose(EnemyMoveStyle::STRIKE, ENEMY_ATTACK_FRAMES);
    assert(done.dx == 0 && done.dy == 0 && !done.scaled());
    std::cout << "[PASS] Enemy animation: a strike lunges at the hero on the hit frame" << std::endl;
}

// The other moves stay distinct from a strike and all end at rest.
void test_other_moves_are_distinct() {
    int guard_dx = 0, guard_sy = ANIM_SCALE_ONE, buff_dy = 0, curse_dx = 0;

    for (EnemyMoveStyle style : ALL_STYLES) {
        for (int frame = 0; frame <= ENEMY_ATTACK_FRAMES; ++frame) {
            EnemyPose pose = attack_pose(style, frame);
            assert(within_box(pose));

            if (style == EnemyMoveStyle::GUARD) {
                guard_dx = pose.dx > guard_dx ? pose.dx : guard_dx;
                guard_sy = pose.sy < guard_sy ? pose.sy : guard_sy;
                assert(pose.dx >= 0); // never moves towards the hero
            } else if (style == EnemyMoveStyle::POWER_UP) {
                buff_dy = pose.dy < buff_dy ? pose.dy : buff_dy;
            } else if (style == EnemyMoveStyle::CURSE) {
                curse_dx = pose.dx < curse_dx ? pose.dx : curse_dx;
            }
        }

        EnemyPose done = attack_pose(style, ENEMY_ATTACK_FRAMES);
        assert(done.dx == 0 && done.dy == 0 && !done.scaled());
    }

    assert(guard_dx >= 4 && guard_sy <= 232);
    assert(buff_dy <= -7);
    assert(curse_dx <= -10 && curse_dx > attack_pose(EnemyMoveStyle::STRIKE, ENEMY_ATTACK_FRAMES / 2).dx);
    std::cout << "[PASS] Enemy animation: guard, buff and curse moves differ from a strike" << std::endl;
}

// A hit taken: flashes white first, knocked back away from the hero, squashed, then back to normal.
void test_hurt_flashes_and_recoils() {
    EnemyPose first = hurt_pose(16, 16);
    assert(first.flash > 0 && first.sy < ANIM_SCALE_ONE);
    int recoil = 0;

    for (int remaining = 16; remaining > 10; --remaining) {
        recoil += hurt_pose(remaining, 16).dx;
    }

    assert(recoil > 0);
    assert(hurt_pose(6, 16).flash == 0);
    EnemyPose over = hurt_pose(0, 16);
    assert(over.dx == 0 && over.flash == 0 && !over.scaled());
    std::cout << "[PASS] Enemy animation: a hit flashes and knocks the enemy back" << std::endl;
}

// Death: the body collapses onto its feet, more and more.
void test_death_collapses() {
    int previous = ANIM_SCALE_ONE + 1;

    for (int frame = 0; frame <= 32; ++frame) {
        EnemyPose pose = death_pose(frame, 32);
        assert(within_box(pose));
        assert(pose.sy <= previous);
        previous = pose.sy;
    }

    assert(previous <= 110);
    // The feet stay on the ground: the bottom edge of a 64 px picture does not move.
    EnemyPose end = death_pose(32, 32);
    PartOffset offset = part_offset(end, 0, 0, 32);
    int bottom = offset.y + 32 * end.sy / ANIM_SCALE_ONE;
    assert(bottom >= 31 && bottom <= 33);
    std::cout << "[PASS] Enemy animation: death collapses the enemy onto its feet" << std::endl;
}

// Boss: its four 64x64 quarters stay joined when squashed (no gap between them), with the feet on the ground.
void test_boss_quarters_stay_joined() {
    EnemyPose pose;
    pose.sx = 230;
    pose.sy = 200;
    PartOffset top_left = part_offset(pose, -32, -32, 64);
    PartOffset top_right = part_offset(pose, 32, -32, 64);
    PartOffset bottom_left = part_offset(pose, -32, 32, 64);
    int width = 64 * pose.sx / ANIM_SCALE_ONE;
    int height = 64 * pose.sy / ANIM_SCALE_ONE;
    assert(top_right.x - top_left.x >= width - 1 && top_right.x - top_left.x <= width + 1);
    assert(bottom_left.y - top_left.y >= height - 1 && bottom_left.y - top_left.y <= height + 1);
    int bottom = bottom_left.y + height / 2;
    assert(bottom >= 63 && bottom <= 65);
    std::cout << "[PASS] Enemy animation: a boss's four quarters stay joined" << std::endl;
}

// Poses add up: an idle plus a hit keeps the hit's flash and multiplies the squash.
void test_poses_combine() {
    EnemyPose a;
    a.dx = 2;
    a.sy = 128;
    EnemyPose b;
    b.dx = -5;
    b.dy = 3;
    b.sy = 128;
    b.flash = 192;
    EnemyPose both = combine(a, b);
    assert(both.dx == -3 && both.dy == 3 && both.sy == 64 && both.sx == ANIM_SCALE_ONE && both.flash == 192);
    std::cout << "[PASS] Enemy animation: poses combine" << std::endl;
}

} // namespace

int main() {
    test_move_style_follows_intent();
    test_idle_breathes_and_telegraphs();
    test_strike_lunges_at_the_hit_frame();
    test_other_moves_are_distinct();
    test_hurt_flashes_and_recoils();
    test_death_collapses();
    test_boss_quarters_stay_joined();
    test_poses_combine();
    return 0;
}
