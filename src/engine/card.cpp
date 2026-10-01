#include "engine/card.h"

namespace engine {

static const CardDef CARDS[] = {
    { CardId::NONE, "None", 0, CardType::SKILL, TargetType::SELF, 0, 0, "", "None+", 0, 0, 0, "" },
    { CardId::STRIKE, "Strike", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 6, 0, "Deal 6 dmg.", "Strike+", 1, 9, 0, "Deal 9 dmg." },
    { CardId::DEFEND, "Defend", 1, CardType::SKILL, TargetType::SELF, 5, 0, "Gain 5 block.", "Defend+", 1, 8, 0, "Gain 8 block." },
    { CardId::BASH, "Bash", 2, CardType::ATTACK, TargetType::SINGLE_ENEMY, 8, 2, "Deal 8 dmg. Apply 2 Vuln.", "Bash+", 2, 10, 3, "Deal 10 dmg. Apply 3 Vuln." },
    { CardId::IRON_WAVE, "Iron Wave", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 5, 5, "Gain 5 blk. Deal 5 dmg.", "Iron Wave+", 1, 7, 7, "Gain 7 blk. Deal 7 dmg." },
    { CardId::CLEAVE, "Cleave", 1, CardType::ATTACK, TargetType::ALL_ENEMIES, 8, 0, "Deal 8 dmg to all.", "Cleave+", 1, 11, 0, "Deal 11 dmg to all." },
    { CardId::ANGER, "Anger", 0, CardType::ATTACK, TargetType::SINGLE_ENEMY, 6, 0, "Deal 6 dmg. Copy to discard.", "Anger+", 0, 8, 0, "Deal 8 dmg. Copy to discard." },
    { CardId::METALLICIZE, "Metallicize", 1, CardType::POWER, TargetType::SELF, 3, 0, "Gain 3 block at end of turn.", "Metallicize+", 1, 4, 0, "Gain 4 block at end of turn." },
    // Silent
    { CardId::NEUTRALIZE, "Neutralize", 0, CardType::ATTACK, TargetType::SINGLE_ENEMY, 3, 1, "Deal 3 dmg. Apply 1 Weak.", "Neutralize+", 0, 4, 2, "Deal 4 dmg. Apply 2 Weak." },
    { CardId::SURVIVOR, "Survivor", 1, CardType::SKILL, TargetType::SELF, 8, 1, "Gain 8 block. Discard 1 card.", "Survivor+", 1, 11, 1, "Gain 11 block. Discard 1 card." },
    { CardId::POISON_STAB, "Poison Stab", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 6, 3, "Deal 6 dmg. Apply 3 Poison.", "Poison Stab+", 1, 8, 4, "Deal 8 dmg. Apply 4 Poison." },
    { CardId::DEADLY_POISON, "Deadly Poison", 1, CardType::SKILL, TargetType::SINGLE_ENEMY, 0, 5, "Apply 5 Poison.", "Deadly Poison+", 1, 0, 7, "Apply 7 Poison." },
    // Defect
    { CardId::ZAP, "Zap", 1, CardType::SKILL, TargetType::SELF, 0, 1, "Channel 1 Lightning.", "Zap+", 0, 0, 1, "Channel 1 Lightning." },
    { CardId::DUALCAST, "Dualcast", 1, CardType::SKILL, TargetType::SELF, 0, 2, "Trigger oldest Orb twice.", "Dualcast+", 0, 0, 2, "Trigger oldest Orb twice." },
    // Regent
    { CardId::FALLING_STAR, "Falling Star", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 7, 1, "Deal 7 dmg. Gain 1 Star.", "Falling Star+", 1, 10, 2, "Deal 10 dmg. Gain 2 Stars." },
    { CardId::VENERATE, "Venerate", 1, CardType::SKILL, TargetType::SELF, 6, 2, "Gain 6 block. Gain 2 Stars.", "Venerate+", 1, 9, 3, "Gain 9 block. Gain 3 Stars." },
    // Necrobinder
    { CardId::UNLEASH, "Unleash", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 8, 0, "Deal 8 necrotic dmg.", "Unleash+", 1, 12, 0, "Deal 12 necrotic dmg." },
    { CardId::BODYGUARD, "Bodyguard", 1, CardType::SKILL, TargetType::SELF, 7, 1, "Gain 7 block. Summon 1 minion.", "Bodyguard+", 1, 10, 1, "Gain 10 block. Summon 1 minion." },
    // Ironclad commons
    { CardId::TWIN_STRIKE, "Twin Strike", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 5, 2, "Deal 5 dmg twice.", "Twin Strike+", 1, 7, 2, "Deal 7 dmg twice." },
    { CardId::POMMEL_STRIKE, "Pommel Strike", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 9, 1, "Deal 9 dmg. Draw 1 card.", "Pommel Strike+", 1, 10, 2, "Deal 10 dmg. Draw 2 cards." },
    { CardId::SHRUG_IT_OFF, "Shrug It Off", 1, CardType::SKILL, TargetType::SELF, 8, 1, "Gain 8 block. Draw 1 card.", "Shrug It Off+", 1, 11, 1, "Gain 11 block. Draw 1 card." },
    { CardId::TAUNT, "Taunt", 1, CardType::SKILL, TargetType::SINGLE_ENEMY, 6, 1, "Gain 6 block. Apply 1 Vuln.", "Taunt+", 1, 7, 2, "Gain 7 block. Apply 2 Vuln." },
    // Silent commons
    { CardId::SLICE, "Slice", 0, CardType::ATTACK, TargetType::SINGLE_ENEMY, 6, 0, "Deal 6 dmg.", "Slice+", 0, 9, 0, "Deal 9 dmg." },
    { CardId::BACKFLIP, "Backflip", 1, CardType::SKILL, TargetType::SELF, 5, 2, "Gain 5 block. Draw 2 cards.", "Backflip+", 1, 8, 2, "Gain 8 block. Draw 2 cards." },
    { CardId::SUCKER_PUNCH, "Sucker Punch", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 8, 1, "Deal 8 dmg. Apply 1 Weak.", "Sucker Punch+", 1, 10, 2, "Deal 10 dmg. Apply 2 Weak." },
    { CardId::DAGGER_SPRAY, "Dagger Spray", 1, CardType::ATTACK, TargetType::ALL_ENEMIES, 4, 2, "Deal 4 dmg to all twice.", "Dagger Spray+", 1, 6, 2, "Deal 6 dmg to all twice." },
    // Defect commons
    { CardId::BEAM_CELL, "Beam Cell", 0, CardType::ATTACK, TargetType::SINGLE_ENEMY, 3, 1, "Deal 3 dmg. Apply 1 Vuln.", "Beam Cell+", 0, 4, 2, "Deal 4 dmg. Apply 2 Vuln." },
    { CardId::BALL_LIGHTNING, "Ball Lightning", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 7, 1, "Deal 7 dmg. Channel 1 Lightning.", "Ball Lightning+", 1, 10, 1, "Deal 10 dmg. Channel 1 Lightning." },
    { CardId::COLD_SNAP, "Cold Snap", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 6, 1, "Deal 6 dmg. Channel 1 Frost.", "Cold Snap+", 1, 9, 1, "Deal 9 dmg. Channel 1 Frost." },
    { CardId::LEAP, "Leap", 1, CardType::SKILL, TargetType::SELF, 9, 0, "Gain 9 block.", "Leap+", 1, 12, 0, "Gain 12 block." },
    // Regent commons
    { CardId::SOLAR_STRIKE, "Solar Strike", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 9, 1, "Deal 9 dmg. Gain 1 Star.", "Solar Strike+", 1, 10, 2, "Deal 10 dmg. Gain 2 Stars." },
    { CardId::GATHER_LIGHT, "Gather Light", 1, CardType::SKILL, TargetType::SELF, 8, 1, "Gain 8 block. Gain 1 Star.", "Gather Light+", 1, 11, 1, "Gain 11 block. Gain 1 Star." },
    { CardId::CELESTIAL_MIGHT, "Celestial Might", 2, CardType::ATTACK, TargetType::SINGLE_ENEMY, 6, 3, "Deal 6 dmg 3 times.", "Celestial Might+", 2, 6, 4, "Deal 6 dmg 4 times." },
    // Necrobinder commons
    { CardId::PULL_AGGRO, "Pull Aggro", 2, CardType::SKILL, TargetType::SELF, 7, 4, "Summon 4. Gain 7 block.", "Pull Aggro+", 2, 9, 5, "Summon 5. Gain 9 block." },
    { CardId::DRAIN_POWER, "Drain Power", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 10, 2, "Deal 10 dmg. Upgrade 2 discarded cards.", "Drain Power+", 1, 12, 3, "Deal 12 dmg. Upgrade 3 discarded cards." },
    // More Ironclad commons
    { CardId::BODY_SLAM, "Body Slam", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 0, 0, "Deal dmg equal to your block.", "Body Slam+", 0, 0, 0, "Deal dmg equal to your block." },
    // More Silent commons
    { CardId::DEFLECT, "Deflect", 0, CardType::SKILL, TargetType::SELF, 4, 0, "Gain 4 block.", "Deflect+", 0, 7, 0, "Gain 7 block." },
    // More Defect commons
    { CardId::COOLHEADED, "Coolheaded", 1, CardType::SKILL, TargetType::SELF, 0, 1, "Channel 1 Frost. Draw 1 card.", "Coolheaded+", 1, 0, 2, "Channel 1 Frost. Draw 2 cards." },
    { CardId::SWEEPING_BEAM, "Sweeping Beam", 1, CardType::ATTACK, TargetType::ALL_ENEMIES, 6, 1, "Deal 6 dmg to all. Draw 1 card.", "Sweeping Beam+", 1, 9, 1, "Deal 9 dmg to all. Draw 1 card." },
    // Ironclad cards on existing mechanics
    { CardId::THUNDERCLAP, "Thunderclap", 1, CardType::ATTACK, TargetType::ALL_ENEMIES, 4, 1, "Deal 4 dmg and apply 1 Vuln to all.", "Thunderclap+", 1, 7, 1, "Deal 7 dmg and apply 1 Vuln to all." },
    { CardId::DISMANTLE, "Dismantle", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 8, 0, "Deal 8 dmg, twice if Vulnerable.", "Dismantle+", 1, 10, 0, "Deal 10 dmg, twice if Vulnerable." },
    { CardId::INFLAME, "Inflame", 1, CardType::POWER, TargetType::SELF, 2, 0, "Gain 2 Strength.", "Inflame+", 1, 3, 0, "Gain 3 Strength." },
    { CardId::PERFECTED_STRIKE, "Perfected Strike", 2, CardType::ATTACK, TargetType::SINGLE_ENEMY, 6, 2, "Deal 6 dmg, +2 per Strike card.", "Perfected Strike+", 2, 6, 3, "Deal 6 dmg, +3 per Strike card." },
    { CardId::BLUDGEON, "Bludgeon", 3, CardType::ATTACK, TargetType::SINGLE_ENEMY, 32, 0, "Deal 32 dmg.", "Bludgeon+", 3, 42, 0, "Deal 42 dmg." },
    { CardId::BULLY, "Bully", 0, CardType::ATTACK, TargetType::SINGLE_ENEMY, 4, 2, "Deal 4 dmg, +2 per Vulnerable.", "Bully+", 0, 4, 3, "Deal 4 dmg, +3 per Vulnerable." },
    { CardId::SWORD_BOOMERANG, "Sword Boomerang", 1, CardType::ATTACK, TargetType::RANDOM_ENEMY, 3, 3, "Deal 3 dmg to a random enemy 3 times.", "Sword Boomerang+", 1, 3, 4, "Deal 3 dmg to a random enemy 4 times." },
    { CardId::BREAK, "Break", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 20, 5, "Deal 20 dmg. Apply 5 Vuln.", "Break+", 1, 30, 7, "Deal 30 dmg. Apply 7 Vuln." },
    { CardId::CONFLAGRATION, "Conflagration", 1, CardType::ATTACK, TargetType::ALL_ENEMIES, 2, 4, "Deal 2 dmg to all 4 times.", "Conflagration+", 1, 2, 5, "Deal 2 dmg to all 5 times." },
    { CardId::EXPECT_A_FIGHT, "Expect a Fight", 3, CardType::SKILL, TargetType::SELF, 15, 5, "Gain 15 block, +5 per Strength.", "Expect a Fight+", 3, 16, 8, "Gain 16 block, +8 per Strength." },
    { CardId::PILLAGE, "Pillage", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 6, 0, "Deal 6 dmg. Draw until a non-Attack.", "Pillage+", 1, 9, 0, "Deal 9 dmg. Draw until a non-Attack." },
    { CardId::UPPERCUT, "Uppercut", 2, CardType::ATTACK, TargetType::SINGLE_ENEMY, 13, 1, "Deal 13 dmg. Apply 1 Weak and 1 Vuln.", "Uppercut+", 2, 13, 2, "Deal 13 dmg. Apply 2 Weak and 2 Vuln." },
    { CardId::FIGHT_ME, "Fight Me", 2, CardType::ATTACK, TargetType::SINGLE_ENEMY, 5, 3, "Deal 5 dmg twice. Gain 3 Str, enemy 1.", "Fight Me+", 2, 6, 4, "Deal 6 dmg twice. Gain 4 Str, enemy 1." },
    // Silent cards on existing mechanics
    { CardId::LEG_SWEEP, "Leg Sweep", 2, CardType::SKILL, TargetType::SINGLE_ENEMY, 11, 2, "Apply 2 Weak. Gain 11 block.", "Leg Sweep+", 2, 14, 3, "Apply 3 Weak. Gain 14 block." },
    { CardId::HAZE, "Haze", 2, CardType::SKILL, TargetType::ALL_ENEMIES, 4, 1, "Apply 4 Poison and 1 Weak to all.", "Haze+", 2, 6, 2, "Apply 6 Poison and 2 Weak to all." },
    { CardId::PRECISE_CUT, "Precise Cut", 0, CardType::ATTACK, TargetType::SINGLE_ENEMY, 13, 2, "Deal 13 dmg, -2 per other card in hand.", "Precise Cut+", 0, 16, 2, "Deal 16 dmg, -2 per other card in hand." },
    { CardId::BOUNCING_FLASK, "Bouncing Flask", 2, CardType::SKILL, TargetType::RANDOM_ENEMY, 3, 3, "Apply 3 Poison to a random enemy 3 times.", "Bouncing Flask+", 2, 3, 4, "Apply 3 Poison to a random enemy 4 times." },
    { CardId::FLECHETTES, "Flechettes", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 5, 0, "Deal 5 dmg per Skill in hand.", "Flechettes+", 1, 7, 0, "Deal 7 dmg per Skill in hand." },
    { CardId::FOOTWORK, "Footwork", 1, CardType::POWER, TargetType::SELF, 2, 0, "Gain 2 Dexterity.", "Footwork+", 1, 3, 0, "Gain 3 Dexterity." },
    { CardId::ESCAPE_PLAN, "Escape Plan", 0, CardType::SKILL, TargetType::SELF, 3, 0, "Draw 1 card. If it is a Skill, gain 3 block.", "Escape Plan+", 0, 5, 0, "Draw 1 card. If it is a Skill, gain 5 block." },
    { CardId::DASH, "Dash", 2, CardType::ATTACK, TargetType::SINGLE_ENEMY, 10, 10, "Gain 10 block. Deal 10 dmg.", "Dash+", 2, 13, 13, "Gain 13 block. Deal 13 dmg." },
    { CardId::BUBBLE_BUBBLE, "Bubble Bubble", 1, CardType::SKILL, TargetType::SINGLE_ENEMY, 9, 0, "If the enemy is Poisoned, apply 9 Poison.", "Bubble Bubble+", 1, 12, 0, "If the enemy is Poisoned, apply 12 Poison." },
    { CardId::OUTBREAK, "Outbreak", 3, CardType::SKILL, TargetType::ALL_ENEMIES, 9, 0, "Apply 9 Poison to all, it triggers at once.", "Outbreak+", 3, 12, 0, "Apply 12 Poison to all, it triggers at once." },
    // Defect cards on existing mechanics
    { CardId::SHADOW_SHIELD, "Shadow Shield", 2, CardType::SKILL, TargetType::SELF, 11, 0, "Gain 11 block. Channel 1 Dark.", "Shadow Shield+", 2, 15, 0, "Gain 15 block. Channel 1 Dark." },
    { CardId::METEOR_STRIKE, "Meteor Strike", 5, CardType::ATTACK, TargetType::SINGLE_ENEMY, 24, 3, "Deal 24 dmg. Channel 3 Plasma.", "Meteor Strike+", 5, 30, 3, "Deal 30 dmg. Channel 3 Plasma." },
    { CardId::GLACIER, "Glacier", 2, CardType::SKILL, TargetType::SELF, 6, 2, "Gain 6 block. Channel 2 Frost.", "Glacier+", 2, 9, 2, "Gain 9 block. Channel 2 Frost." },
    { CardId::ICE_LANCE, "Ice Lance", 3, CardType::ATTACK, TargetType::SINGLE_ENEMY, 19, 3, "Deal 19 dmg. Channel 3 Frost.", "Ice Lance+", 3, 24, 3, "Deal 24 dmg. Channel 3 Frost." },
    { CardId::SKIM, "Skim", 1, CardType::SKILL, TargetType::SELF, 0, 3, "Draw 3 cards.", "Skim+", 1, 0, 4, "Draw 4 cards." },
    { CardId::ALL_FOR_ONE, "All for One", 2, CardType::ATTACK, TargetType::SINGLE_ENEMY, 10, 0, "Deal 10 dmg. Return 0-cost cards from discard.", "All for One+", 2, 14, 0, "Deal 14 dmg. Return 0-cost cards from discard." },
    { CardId::NULL_CARD, "Null", 2, CardType::ATTACK, TargetType::SINGLE_ENEMY, 10, 1, "Deal 10 dmg. Apply 1 Weak. Channel 1 Dark.", "Null+", 2, 13, 2, "Deal 13 dmg. Apply 2 Weak. Channel 1 Dark." },
    { CardId::GO_FOR_THE_EYES, "Go for the Eyes", 0, CardType::ATTACK, TargetType::SINGLE_ENEMY, 3, 1, "Deal 3 dmg. If the enemy attacks, 1 Weak.", "Go for the Eyes+", 0, 4, 2, "Deal 4 dmg. If the enemy attacks, 2 Weak." },
    { CardId::SCRAPE, "Scrape", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 7, 4, "Deal 7 dmg. Draw 4, discard those not costing 0.", "Scrape+", 1, 10, 5, "Deal 10 dmg. Draw 5, discard those not costing 0." },
    // Necrobinder cards on existing mechanics
    { CardId::BURY, "Bury", 4, CardType::ATTACK, TargetType::SINGLE_ENEMY, 52, 0, "Deal 52 dmg.", "Bury+", 4, 63, 0, "Deal 63 dmg." },
    { CardId::MISERY, "Misery", 0, CardType::ATTACK, TargetType::SINGLE_ENEMY, 7, 0, "Deal 7 dmg. Spread its debuffs to the others.", "Misery+", 0, 9, 0, "Retain. Deal 9 dmg. Spread its debuffs to the others." },
    { CardId::UNDEATH, "Undeath", 0, CardType::SKILL, TargetType::SELF, 7, 0, "Gain 7 block. Copy to discard.", "Undeath+", 0, 9, 0, "Gain 9 block. Copy to discard." }
};

const CardDef& CardDef::get(CardId id) {
    uint8_t index = static_cast<uint8_t>(id);
    if (index >= static_cast<uint8_t>(CardId::COUNT)) {
        return CARDS[0];
    }
    return CARDS[index];
}

} // namespace engine
