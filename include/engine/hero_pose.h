#pragma once
#include <cstdint>

namespace engine {

// Pose drawn on the hero's battle sprite; the value is the frame of its sprite sheet
// (HERO_POSES in scripts/build_gba_art.py).
enum class HeroPose : uint8_t {
    IDLE = 0,
    ATTACK = 1,
    HURT = 2,
};

// Pose for the frame counters of the hero's battle animations (each counts down to 0).
// Being hit or dying wins over an attack in progress: the hit is what the player must notice.
constexpr HeroPose hero_pose(int lunge_frames, int hurt_frames, int death_frames) {
    if(hurt_frames > 0 || death_frames > 0) {
        return HeroPose::HURT;
    }
    return lunge_frames > 0 ? HeroPose::ATTACK : HeroPose::IDLE;
}

} // namespace engine
