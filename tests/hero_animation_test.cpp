// Animations of the playable hero in combat (#16): idle breathing, attack, hurt and guard poses and their timings.
#include <cassert>
#include <iostream>
#include "engine/hero_animation.h"

using namespace engine;

namespace {

// Plays an animation to its end from the given countdowns and returns the frames shown.
template<int N>
void play(AttackStyle style, HeroAnimation animation, HeroFrame (&frames)[N]) {
    for (int index = 0; index < N; ++index) {
        frames[index] = hero_frame(style, animation, 0);
        advance(animation);
    }
}

void test_idle_breathes_in_a_loop() {
    const HeroAnimation still;
    for (int frame = 0; frame < HERO_BREATH_PERIOD * 3; ++frame) {
        HeroFrame shown = hero_frame(AttackStyle::MELEE, still, frame);
        assert(shown.dx == 0 && shown.dy == 0);
        bool out = frame % HERO_BREATH_PERIOD >= HERO_BREATH_PERIOD / 2;
        assert(shown.pose == (out ? HeroPose::BREATH : HeroPose::IDLE));
    }
    std::cout << "[PASS] Idle: the hero alternates standing and breathing out, in a loop" << std::endl;
}

void test_each_hero_has_its_attack_style() {
    assert(hero_attack_style(CharacterId::IRONCLAD) == AttackStyle::MELEE);
    assert(hero_attack_style(CharacterId::SILENT) == AttackStyle::MELEE);
    assert(hero_attack_style(CharacterId::DEFECT) == AttackStyle::RANGED);
    assert(hero_attack_style(CharacterId::REGENT) == AttackStyle::RANGED);
    assert(hero_attack_style(CharacterId::NECROBINDER) == AttackStyle::SPELL);
    std::cout << "[PASS] Attack: melee for Ironclad and Silent, ranged for Defect and Regent, spell for Necrobinder"
              << std::endl;
}

void test_melee_attack_runs_in_peaks_on_impact_and_comes_back() {
    HeroAnimation animation;
    animation.attack = HERO_ATTACK_FRAMES;
    HeroFrame frames[HERO_ATTACK_FRAMES + 1];
    play(AttackStyle::MELEE, animation, frames);
    for (int index = 0; index < HERO_ATTACK_FRAMES; ++index) {
        assert(frames[index].pose == HeroPose::ATTACK);
        assert(frames[index].dy == 0);
        assert(frames[index].dx >= 0 && frames[index].dx <= HERO_MELEE_REACH);
        if (index > 0 && index <= HERO_ATTACK_PEAK) assert(frames[index].dx > frames[index - 1].dx);
        if (index > HERO_ATTACK_PEAK) assert(frames[index].dx <= frames[index - 1].dx);
    }
    assert(frames[0].dx == 0);
    assert(frames[HERO_ATTACK_PEAK].dx == HERO_MELEE_REACH);
    // The animation is over: back home, standing.
    assert(frames[HERO_ATTACK_FRAMES].pose == HeroPose::IDLE && frames[HERO_ATTACK_FRAMES].dx == 0);
    std::cout << "[PASS] Attack (melee): the hero runs in, reaches the enemy on the peak frame and comes back"
              << std::endl;
}

void test_ranged_attack_kicks_back_and_spell_rises() {
    HeroAnimation animation;
    animation.attack = HERO_ATTACK_FRAMES;
    HeroFrame shot[HERO_ATTACK_FRAMES + 1];
    HeroFrame cast[HERO_ATTACK_FRAMES + 1];
    play(AttackStyle::RANGED, animation, shot);
    play(AttackStyle::SPELL, animation, cast);
    assert(shot[HERO_ATTACK_PEAK].pose == HeroPose::ATTACK);
    assert(shot[HERO_ATTACK_PEAK].dx == -HERO_RANGED_KICK && shot[HERO_ATTACK_PEAK].dy == 0);
    assert(cast[HERO_ATTACK_PEAK].pose == HeroPose::ATTACK);
    assert(cast[HERO_ATTACK_PEAK].dy == -HERO_SPELL_RISE && cast[HERO_ATTACK_PEAK].dx == 0);
    assert(shot[HERO_ATTACK_FRAMES].dx == 0 && cast[HERO_ATTACK_FRAMES].dy == 0);
    std::cout << "[PASS] Attack (ranged, spell): the shot kicks the hero back, the spell lifts it, both return"
              << std::endl;
}

void test_hurt_recoils_and_wins_over_the_other_poses() {
    HeroAnimation animation;
    animation.hurt = HERO_HURT_FRAMES;
    animation.attack = HERO_ATTACK_FRAMES;
    animation.guard = HERO_GUARD_FRAMES;
    HeroFrame first = hero_frame(AttackStyle::RANGED, animation, 0);
    assert(first.pose == HeroPose::HURT);

    HeroAnimation hit;
    hit.hurt = HERO_HURT_FRAMES;
    HeroFrame frames[HERO_HURT_FRAMES + 1];
    play(AttackStyle::MELEE, hit, frames);
    for (int index = 0; index < HERO_HURT_FRAMES; ++index) {
        assert(frames[index].pose == HeroPose::HURT);
        assert(frames[index].dx <= 0 && frames[index].dy == 0);
    }
    assert(frames[0].dx <= -HERO_HURT_FRAMES / 5); // thrown furthest back on the hit itself
    assert(frames[HERO_HURT_FRAMES].pose == HeroPose::IDLE && frames[HERO_HURT_FRAMES].dx == 0);
    std::cout << "[PASS] Hurt: the hero is thrown back on the hit, then stands again" << std::endl;
}

void test_guard_crouches_while_the_block_lands() {
    HeroAnimation animation;
    animation.guard = HERO_GUARD_FRAMES;
    HeroFrame frames[HERO_GUARD_FRAMES + 1];
    play(AttackStyle::MELEE, animation, frames);
    for (int index = 0; index < HERO_GUARD_FRAMES; ++index) {
        assert(frames[index].pose == HeroPose::GUARD);
        assert(frames[index].dx == (index < HERO_GUARD_FRAMES / 2 ? -3 : -1));
    }
    assert(frames[HERO_GUARD_FRAMES].pose == HeroPose::IDLE);

    // Block gained by an Attack card (Iron Wave): the attack pose shows, the guard step still adds up.
    HeroAnimation both;
    both.attack = HERO_ATTACK_FRAMES;
    both.guard = HERO_GUARD_FRAMES;
    assert(hero_frame(AttackStyle::MELEE, both, 0).pose == HeroPose::ATTACK);
    std::cout << "[PASS] Guard: the hero crouches back while its Block lands, then stands again" << std::endl;
}

void test_dying_hero_keeps_the_hurt_pose() {
    HeroAnimation animation;
    animation.death = 32;
    assert(hero_frame(AttackStyle::SPELL, animation, 40).pose == HeroPose::HURT);
    std::cout << "[PASS] Death: the fading hero keeps its hurt pose" << std::endl;
}

static_assert(int(HeroPose::COUNT) == 5, "one frame per pose in graphics/hero_<hero>.bmp");

} // namespace

int main() {
    test_idle_breathes_in_a_loop();
    test_each_hero_has_its_attack_style();
    test_melee_attack_runs_in_peaks_on_impact_and_comes_back();
    test_ranged_attack_kicks_back_and_spell_rises();
    test_hurt_recoils_and_wins_over_the_other_poses();
    test_guard_crouches_while_the_block_lands();
    test_dying_hero_keeps_the_hurt_pose();
    return 0;
}
