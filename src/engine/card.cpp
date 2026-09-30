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
    { CardId::DRAIN_POWER, "Drain Power", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 10, 2, "Deal 10 dmg. Upgrade 2 discarded cards.", "Drain Power+", 1, 12, 3, "Deal 12 dmg. Upgrade 3 discarded cards." }
};

const CardDef& CardDef::get(CardId id) {
    uint8_t index = static_cast<uint8_t>(id);
    if (index >= static_cast<uint8_t>(CardId::COUNT)) {
        return CARDS[0];
    }
    return CARDS[index];
}

} // namespace engine
