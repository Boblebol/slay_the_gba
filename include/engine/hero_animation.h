#pragma once

#include <cstdint>

#include "engine/character.h"

// Animations of the playable hero in combat (#16): which pose frame of graphics/hero_<hero>.bmp to show and how
// far to move the sprite, from the countdowns the combat scene starts. Kept here, away from Butano, so that the
// timings are tested on the host; the GBA scene only copies the result onto the sprite.
namespace engine
{

// Frames of graphics/hero_<hero>.bmp, top to bottom (HERO_POSES in scripts/build_gba_art.py).
enum class HeroPose : uint8_t
{
    IDLE,   // standing
    BREATH, // breathing out, alternates with IDLE while nothing happens
    ATTACK, // leaning towards the enemies
    HURT,   // thrown back by a hit
    GUARD,  // crouched behind fresh Block
    COUNT
};

// How the hero moves when it plays an Attack.
enum class AttackStyle : uint8_t
{
    MELEE,  // runs in and strikes: Ironclad's sword, Silent's daggers
    RANGED, // stays put and kicks back on the shot: Defect's orbs, Regent's stars
    SPELL   // rises while casting: Necrobinder's staff
};

constexpr AttackStyle hero_attack_style(CharacterId hero)
{
    switch(hero)
    {
        case CharacterId::DEFECT:
        case CharacterId::REGENT:
            return AttackStyle::RANGED;
        case CharacterId::NECROBINDER:
            return AttackStyle::SPELL;
        default:
            return AttackStyle::MELEE;
    }
}

// Lengths in frames (60 per second). The attack starts when the played card leaves the stage and reaches its
// furthest point (frame ATTACK_PEAK) as the card hits the enemy, 6 frames later in the combat scene.
constexpr int HERO_ATTACK_FRAMES = 16;
constexpr int HERO_ATTACK_PEAK = 6;
constexpr int HERO_HURT_FRAMES = 20;
constexpr int HERO_GUARD_FRAMES = 14;
constexpr int HERO_BREATH_PERIOD = 64; // one breath in and out
constexpr int HERO_MELEE_REACH = 30;   // pixels run towards the enemies
constexpr int HERO_RANGED_KICK = 6;    // pixels kicked back by the shot
constexpr int HERO_SPELL_RISE = 6;     // pixels risen while casting

// Countdowns of the running animations, in frames left (0 = not running). The scene sets one to its length
// when the event happens and the animation counts it down.
struct HeroAnimation
{
    int attack = 0;
    int hurt = 0;
    int guard = 0;
    int death = 0;
};

struct HeroFrame
{
    HeroPose pose = HeroPose::IDLE;
    int dx = 0; // pixels, positive towards the enemies
    int dy = 0; // pixels, positive downwards
};

// Distance travelled at `elapsed` frames into an attack: out to `reach` by the peak, back to 0 at the end.
constexpr int hero_attack_reach(int elapsed, int reach)
{
    if(elapsed <= 0 || elapsed >= HERO_ATTACK_FRAMES)
    {
        return 0;
    }

    if(elapsed <= HERO_ATTACK_PEAK)
    {
        // Fast start, slowing down on the target.
        int left = HERO_ATTACK_PEAK - elapsed;
        constexpr int span = HERO_ATTACK_PEAK * HERO_ATTACK_PEAK;
        return reach - (reach * left * left + span / 2) / span;
    }

    // Eased way back.
    int back = HERO_ATTACK_FRAMES - HERO_ATTACK_PEAK;
    int done = elapsed - HERO_ATTACK_PEAK;
    int eased = done * done * (3 * back - 2 * done); // smoothstep times back^3
    return reach - reach * eased / (back * back * back);
}

// Pose and offset of the hero this frame. `frame` is the scene's frame counter, for the breathing.
// A hit wins over an attack, an attack over a guard: the latest, most violent event shows.
constexpr HeroFrame hero_frame(AttackStyle style, const HeroAnimation& animation, int frame)
{
    HeroFrame result;
    result.pose = (frame % HERO_BREATH_PERIOD) < HERO_BREATH_PERIOD / 2 ? HeroPose::IDLE : HeroPose::BREATH;

    if(animation.guard > 0)
    {
        result.pose = HeroPose::GUARD;
        result.dx -= animation.guard > HERO_GUARD_FRAMES / 2 ? 3 : 1;
    }

    if(animation.attack > 0)
    {
        result.pose = HeroPose::ATTACK;
        int elapsed = HERO_ATTACK_FRAMES - animation.attack;

        switch(style)
        {
            case AttackStyle::MELEE:
                result.dx += hero_attack_reach(elapsed, HERO_MELEE_REACH);
                break;
            case AttackStyle::RANGED:
                result.dx -= hero_attack_reach(elapsed, HERO_RANGED_KICK);
                break;
            case AttackStyle::SPELL:
                result.dy -= hero_attack_reach(elapsed, HERO_SPELL_RISE);
                break;
        }
    }

    if(animation.hurt > 0)
    {
        result.pose = HeroPose::HURT;
        result.dx -= (animation.hurt % 4 < 2 ? 3 : 0) + animation.hurt / 5;
    }

    if(animation.death > 0)
    {
        result.pose = HeroPose::HURT;
    }

    return result;
}

// One frame later: every running countdown goes down by one.
constexpr void advance(HeroAnimation& animation)
{
    if(animation.attack > 0) --animation.attack;
    if(animation.hurt > 0) --animation.hurt;
    if(animation.guard > 0) --animation.guard;
    if(animation.death > 0) --animation.death;
}

} // namespace engine
