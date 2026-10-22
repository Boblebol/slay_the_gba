#pragma once

// Procedural animation of enemies and bosses in a fight (#17): rest, attack, hit taken and death are drawn from
// the monster's one existing picture, only moved and squashed. No extra frame, so no extra VRAM tile: a boss keeps
// its four 64x64 quarters (256 of the 1024 sprite tiles), and an animated enemy borrows one affine matrix.
//
// Pure integer maths, free of Butano so that the host tests can check it. Positions are in pixels (y grows
// downwards, the hero stands on the left), scales in 1/256.

#include <cstdint>

#include "engine/entity.h"

namespace engine {

constexpr int ANIM_SCALE_ONE = 256;

// How a move looks: the attack it plays and the idle it telegraphs while its intent is shown.
enum class EnemyMoveStyle : uint8_t {
    STRIKE,   // any move that hits the hero
    GUARD,    // only gains block
    POWER_UP, // buffs or heals itself
    CURSE,    // statuses or cards for the hero, without a hit
    OTHER     // stunned, escaping, summoning
};

[[nodiscard]] constexpr EnemyMoveStyle move_style(EnemyIntentType type) {
    switch (type) {
    case EnemyIntentType::ATTACK:
    case EnemyIntentType::ATTACK_DEFEND:
    case EnemyIntentType::ATTACK_DEBUFF:
    case EnemyIntentType::ATTACK_BUFF:
        return EnemyMoveStyle::STRIKE;
    case EnemyIntentType::DEFEND:
        return EnemyMoveStyle::GUARD;
    case EnemyIntentType::BUFF:
    case EnemyIntentType::HEAL:
        return EnemyMoveStyle::POWER_UP;
    case EnemyIntentType::DEBUFF:
    case EnemyIntentType::STATUS:
        return EnemyMoveStyle::CURSE;
    default:
        return EnemyMoveStyle::OTHER;
    }
}

// Offset and squash of an enemy picture for one frame. Scales never go above 1: a squashed 64x64 sprite still fits
// its own box, so the affine sprite needs no double size mode (no larger box, no extra scanline cost).
struct EnemyPose {
    int dx = 0;
    int dy = 0;
    int sx = ANIM_SCALE_ONE;
    int sy = ANIM_SCALE_ONE;
    int flash = 0; // white fade, in 1/256

    [[nodiscard]] constexpr bool scaled() const { return sx != ANIM_SCALE_ONE || sy != ANIM_SCALE_ONE; }
};

// Two poses played at once (an idle and a hit taken): offsets add up, scales multiply, the brighter flash wins.
[[nodiscard]] constexpr EnemyPose combine(const EnemyPose& a, const EnemyPose& b) {
    EnemyPose pose;
    pose.dx = a.dx + b.dx;
    pose.dy = a.dy + b.dy;
    pose.sx = a.sx * b.sx / ANIM_SCALE_ONE;
    pose.sy = a.sy * b.sy / ANIM_SCALE_ONE;
    pose.flash = a.flash > b.flash ? a.flash : b.flash;
    return pose;
}

namespace anim_detail {

// Sine-like wave in [-256, 256] (two parabolas), period in frames.
[[nodiscard]] constexpr int wave(int frame, int period) {
    if (period <= 0) {
        return 0;
    }

    int phase = frame % period;

    if (phase < 0) {
        phase += period;
    }

    int x = phase * 512 / period;
    int h = x % 256;
    int value = 4 * h * (256 - h) / 256;
    return x < 256 ? value : -value;
}

// t in [0, 256] -> [0, 256], cubic ease-out.
[[nodiscard]] constexpr int ease_out(int t) {
    int inv = 256 - t;
    return 256 - inv * inv / 256 * inv / 256;
}

// t in [0, 256] -> [0, 256], smoothstep.
[[nodiscard]] constexpr int ease_in_out(int t) {
    return t * t / 256 * (768 - 2 * t) / 256;
}

// 0 at t = 0 and t = 256, 256 at t = 128.
[[nodiscard]] constexpr int hop(int t) {
    return 4 * t * (256 - t) / 256;
}

[[nodiscard]] constexpr int clamp_t(int t) {
    return t < 0 ? 0 : (t > 256 ? 256 : t);
}

} // namespace anim_detail

// Breathing at rest, telegraphing the intent shown above the head: a coiled, faster breath before an attack,
// hunkered before a block, rising tall before a buff, swaying before a curse. `squash` off keeps only the offset
// (a crowded row, where every affine sprite would cost too much scanline time).
[[nodiscard]] constexpr EnemyPose idle_pose(EnemyMoveStyle telegraph, int frame, bool squash = true) {
    int period = 72;
    int bob = 1;
    int squash_amount = 6;
    int stretch_amount = 0;
    int sway = 0;
    int base_sy = ANIM_SCALE_ONE;

    switch (telegraph) {
    case EnemyMoveStyle::STRIKE:
        period = 44;
        bob = 2;
        squash_amount = 16;
        break;
    case EnemyMoveStyle::GUARD:
        period = 96;
        squash_amount = 6;
        base_sy = 246;
        break;
    case EnemyMoveStyle::POWER_UP:
        period = 60;
        bob = 3;
        squash_amount = 0;
        stretch_amount = 12;
        break;
    case EnemyMoveStyle::CURSE:
        period = 80;
        bob = 1;
        squash_amount = 4;
        sway = 2;
        break;
    default: // OTHER
        break;
    }

    int s = anim_detail::wave(frame, period);
    EnemyPose pose;
    // Lowest point (s = 256) is the most squashed: the body settles, then breathes up.
    pose.dy = bob * s / 256;
    pose.dx = sway * anim_detail::wave(frame + period / 4, period * 2) / 256;

    if (squash) {
        pose.sy = base_sy - squash_amount * (s + 256) / 512;
        // Taller when up: the width narrows instead, since a scale never grows past 1.
        pose.sx = ANIM_SCALE_ONE - stretch_amount * (256 - s) / 512;
    }

    return pose;
}

// Length of a move played during the enemy turn; its peak (the hit) falls on frame ENEMY_ATTACK_FRAMES / 2.
constexpr int ENEMY_ATTACK_FRAMES = 18;

// The move itself, `elapsed` frames in. A strike winds up (leans back, squashes), lunges at the hero, comes back.
[[nodiscard]] constexpr EnemyPose attack_pose(EnemyMoveStyle style, int elapsed) {
    using namespace anim_detail;
    int t = clamp_t(elapsed * 256 / ENEMY_ATTACK_FRAMES);
    EnemyPose pose;

    switch (style) {
    case EnemyMoveStyle::STRIKE:
        if (t < 64) {
            int u = t * 4;
            pose.dx = 4 * u / 256;
            pose.sy = 256 - 26 * u / 256;
        } else if (t < 128) {
            int u = ease_out((t - 64) * 4);
            pose.dx = 4 - 30 * u / 256;
            pose.dy = -6 * hop(u) / 256 / 2;
            pose.sy = 230 + 12 * u / 256;
        } else {
            int u = ease_in_out((t - 128) * 2);
            pose.dx = -26 + 26 * u / 256;
            pose.sy = 242 + 14 * u / 256;
        }
        break;
    case EnemyMoveStyle::GUARD: {
        // Steps back and braces, low behind its guard.
        int u = t < 128 ? ease_out(t * 2) : 256 - ease_in_out((t - 128) * 2);
        pose.dx = 5 * u / 256;
        pose.sy = 256 - 30 * u / 256;
        break;
    }
    case EnemyMoveStyle::POWER_UP: {
        // Two quick hops, tall in the air, squashed on landing.
        int half = t < 128 ? t * 2 : (t - 128) * 2;
        int h = hop(half);
        pose.dy = -8 * h / 256;
        pose.sx = 256 - 24 * h / 256;
        int land = half > 192 ? hop((half - 192) * 4) : 0;
        pose.sy = 256 - 20 * land / 256;
        break;
    }
    case EnemyMoveStyle::CURSE: {
        // Leans towards the hero, quivering.
        int h = hop(t);
        int quiver = wave(elapsed, 6);
        pose.dx = -12 * h / 256;
        pose.sx = 256 - 14 * h / 256 * (quiver < 0 ? -quiver : quiver) / 256;
        break;
    }
    default: { // OTHER
        int h = hop(t);
        pose.dy = -4 * h / 256;
        pose.sy = 256 - 12 * h / 256;
        break;
    }
    }

    return pose;
}

// A hit taken, `remaining` frames before it ends (counted down from `total`): knocked back away from the hero,
// squashed, shaking, flashing white for the first frames.
[[nodiscard]] constexpr EnemyPose hurt_pose(int remaining, int total) {
    EnemyPose pose;

    if (remaining <= 0 || total <= 0) {
        return pose;
    }

    int k = remaining * 256 / total;
    int jitter = (remaining % 4 < 2 ? 1 : -1) * (remaining / 4 + 1);
    pose.dx = 5 * k / 256 + jitter;
    pose.sy = 256 - 22 * k / 256;
    pose.flash = remaining * 8 > total * 5 ? 192 : 0;
    return pose;
}

// Death, `elapsed` frames in out of `total`: the body sags and collapses onto its feet as it fades out.
[[nodiscard]] constexpr EnemyPose death_pose(int elapsed, int total) {
    EnemyPose pose;

    if (total <= 0) {
        return pose;
    }

    int t = anim_detail::clamp_t(elapsed * 256 / total);
    int u = anim_detail::ease_out(t);
    pose.sy = 256 - 160 * u / 256;
    pose.sx = 256 - 24 * u / 256;
    pose.dx = 3 * anim_detail::hop(t) / 256;
    return pose;
}

struct PartOffset {
    int x = 0;
    int y = 0;
};

// Where a part (one 64x64 quarter of a boss, or the whole sprite at 0, 0) goes for a pose, relative to the
// enemy's place. The squash keeps the feet on the ground: `half_height` is half the picture's height.
[[nodiscard]] constexpr PartOffset part_offset(const EnemyPose& pose, int part_x, int part_y, int half_height) {
    PartOffset offset;
    offset.x = pose.dx + part_x * pose.sx / ANIM_SCALE_ONE;
    offset.y = pose.dy + part_y * pose.sy / ANIM_SCALE_ONE + half_height * (ANIM_SCALE_ONE - pose.sy) / ANIM_SCALE_ONE;
    return offset;
}

// Squash at rest for every enemy of a row only up to three of them: each squashed sprite is drawn affine, which
// costs twice the scanline time of a plain one, and a crowded row of six would drop sprites.
[[nodiscard]] constexpr bool idle_squash_allowed(int enemies_shown) {
    return enemies_shown <= 3;
}

} // namespace engine
