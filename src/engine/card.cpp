#include "engine/card.h"

namespace engine {

static const CardDef CARDS[] = {
    { CardId::NONE, "None", 0, CardType::SKILL, TargetType::SELF, 0, 0, "" },
    { CardId::STRIKE, "Strike", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 6, 0, "Deal 6 dmg." },
    { CardId::DEFEND, "Defend", 1, CardType::SKILL, TargetType::SELF, 5, 0, "Gain 5 block." },
    { CardId::BASH, "Bash", 2, CardType::ATTACK, TargetType::SINGLE_ENEMY, 8, 2, "Deal 8 dmg. Apply 2 Vuln." },
    { CardId::IRON_WAVE, "Iron Wave", 1, CardType::ATTACK, TargetType::SINGLE_ENEMY, 5, 5, "Gain 5 blk. Deal 5 dmg." },
    { CardId::CLEAVE, "Cleave", 1, CardType::ATTACK, TargetType::ALL_ENEMIES, 8, 0, "Deal 8 dmg to all." },
    { CardId::ANGER, "Anger", 0, CardType::ATTACK, TargetType::SINGLE_ENEMY, 6, 0, "Deal 6 dmg. Copy to discard." }
};

const CardDef& CardDef::get(CardId id) {
    uint8_t index = static_cast<uint8_t>(id);
    if (index >= static_cast<uint8_t>(CardId::COUNT)) {
        return CARDS[0];
    }
    return CARDS[index];
}

} // namespace engine
