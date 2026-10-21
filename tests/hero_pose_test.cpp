// Battle poses of the hero sprite: idle, attack while lunging, hurt when hit or dying (#2).
#include <cassert>
#include <iostream>
#include "engine/hero_pose.h"

using namespace engine;

void test_the_hero_rests_when_no_animation_runs() {
    assert(hero_pose(0, 0, 0) == HeroPose::IDLE);
    std::cout << "[PASS] Hero pose: idle when no animation runs" << std::endl;
}

void test_the_hero_strikes_while_lunging() {
    assert(hero_pose(16, 0, 0) == HeroPose::ATTACK);
    assert(hero_pose(1, 0, 0) == HeroPose::ATTACK);
    std::cout << "[PASS] Hero pose: attack while lunging at the enemy" << std::endl;
}

void test_the_hero_flinches_when_hit_or_dying() {
    assert(hero_pose(0, 20, 0) == HeroPose::HURT);
    assert(hero_pose(0, 0, 40) == HeroPose::HURT);
    assert(hero_pose(8, 5, 0) == HeroPose::HURT); // a hit interrupts the attack pose
    std::cout << "[PASS] Hero pose: hurt when hit or dying, even mid-attack" << std::endl;
}

void test_poses_are_the_frames_of_the_sprite_sheet() {
    static_assert(static_cast<int>(HeroPose::IDLE) == 0);
    static_assert(static_cast<int>(HeroPose::ATTACK) == 1);
    static_assert(static_cast<int>(HeroPose::HURT) == 2);
    std::cout << "[PASS] Hero pose: idle, attack, hurt are frames 0, 1, 2" << std::endl;
}

int main() {
    test_the_hero_rests_when_no_animation_runs();
    test_the_hero_strikes_while_lunging();
    test_the_hero_flinches_when_hit_or_dying();
    test_poses_are_the_frames_of_the_sprite_sheet();
    return 0;
}
