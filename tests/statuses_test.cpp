// Statuses of #91, #92 and #93: one case per rule, on hand-built situations.
#include <cassert>
#include <iostream>
#include "engine/combat.h"

using namespace engine;

namespace {

// Jaw Worm opens with Chomp (11 damage, no block): a predictable punching bag.
CombatEngine make_combat(CharacterId hero, EnemyType enemy_type = EnemyType::JAW_WORM) {
    CombatEngine combat;
    combat.start_combat(enemy_type, hero);
    return combat;
}

// Puts `card` alone in hand with full energy and plays it.
void play(CombatEngine& combat, DeckCard card, int target = -1) {
    combat.hand.clear();
    combat.hand.push_back(card);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(0, target);
    assert(played);
}

void make_enemy_idle(Enemy& enemy) {
    enemy.current_intent = EnemyIntent{};
    enemy.current_intent.type = EnemyIntentType::DEFEND;
}

// Ends the turn with every enemy doing nothing, so only the player's own effects show.
void end_turn_idle(CombatEngine& combat) {
    for (Enemy& foe : combat.enemies) make_enemy_idle(foe);
    combat.end_player_turn();
}

// Ends the turn with the first enemy attacking for `damage`.
void end_turn_attacked(CombatEngine& combat, int16_t damage) {
    for (Enemy& foe : combat.enemies) make_enemy_idle(foe);
    combat.enemies[0].current_intent.type = EnemyIntentType::ATTACK;
    combat.enemies[0].current_intent.base_damage = damage;
    combat.end_player_turn();
}

void pad_draw_pile(CombatEngine& combat, int count, CardId id = CardId::DEFEND) {
    for (int i = 0; i < count; ++i) combat.draw_pile.push_back(id);
}

void test_plating() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(combat, 20);
    combat.player.add_status(StatusType::PLATED_ARMOR, 4);
    assert(combat.player.block == 0);

    // End of turn: 4 Block, which absorbs part of the attack. Start of the next turn: Plating 3.
    int16_t hp = combat.player.hp;
    end_turn_attacked(combat, 10);
    assert(combat.player.hp == hp - 6);
    assert(combat.player.get_status(StatusType::PLATED_ARMOR) == 3);
    assert(combat.player.block == 0);

    // Dexterity does not change it; it wears off one point a turn and leaves the status list.
    combat.player.add_status(StatusType::DEXTERITY, 5);
    hp = combat.player.hp;
    end_turn_attacked(combat, 10);
    assert(combat.player.hp == hp - 7);
    end_turn_idle(combat);
    end_turn_idle(combat);
    assert(combat.player.get_status(StatusType::PLATED_ARMOR) == 0);
    assert(combat.player.statuses.size() == 1);

    // Juggernaut hears the Block it gives.
    CombatEngine heard = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(heard, 20);
    heard.add_power(PowerId::JUGGERNAUT, 5);
    heard.player.add_status(StatusType::PLATED_ARMOR, 2);
    int16_t foe_hp = heard.enemies[0].hp;
    end_turn_idle(heard);
    assert(heard.enemies[0].hp == foe_hp - 5);

    // An enemy: one point lost at the start of its turn, Block at its end, kept during the player's turn.
    CombatEngine armored = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(armored, 20);
    armored.enemies[0].add_status(StatusType::PLATED_ARMOR, 5);
    end_turn_idle(armored);
    assert(armored.enemies[0].get_status(StatusType::PLATED_ARMOR) == 4 && armored.enemies[0].block == 4);
    std::cout << "[PASS] Plating: Block at the end of the turn, one point lost at the start of the turn" << std::endl;
}

void test_thorns() {
    // The player's Thorns hurt the attacker, through its Block first.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(combat, 20);
    combat.player.add_status(StatusType::THORNS, 3);
    int16_t foe_hp = combat.enemies[0].hp;
    end_turn_attacked(combat, 5);
    assert(combat.enemies[0].hp == foe_hp - 3);
    assert(combat.player.get_status(StatusType::THORNS) == 3);

    // An enemy that does not attack is not hurt.
    end_turn_idle(combat);
    assert(combat.enemies[0].hp == foe_hp - 3);

    // Thorns can kill the attacker: the fight is won.
    combat.enemies[0].hp = 2;
    end_turn_attacked(combat, 5);
    assert(combat.state == CombatState::VICTORY);

    // An enemy's Thorns hurt the player once per hit, Block absorbing them.
    CombatEngine spiky = make_combat(CharacterId::IRONCLAD);
    spiky.enemies[0].add_status(StatusType::THORNS, 2);
    int16_t hp = spiky.player.hp;
    play(spiky, CardId::TWIN_STRIKE);
    assert(spiky.player.hp == hp - 4);
    assert(spiky.counters.hp_lost == 4);
    spiky.player.block = 10;
    play(spiky, CardId::STRIKE);
    assert(spiky.player.hp == hp - 4 && spiky.player.block == 8);
    // A Skill is not an attack.
    play(spiky, CardId::DEFEND);
    assert(spiky.player.hp == hp - 4);
    std::cout << "[PASS] Thorns: damage back for each attack hit, for the player and for the enemies" << std::endl;
}

void test_intangible() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(combat, 20);
    combat.player.add_status(StatusType::INTANGIBLE, 1);
    // The intent already shows 1.
    combat.enemies[0].current_intent.base_damage = 30;
    assert(combat.enemies[0].intent_damage(combat.player) == 1);
    // HP lost from a card: 1.
    int16_t hp = combat.player.hp;
    combat.lose_hp(6);
    assert(combat.player.hp == hp - 1);
    // An attack: 1, then the status wears off with the round.
    end_turn_attacked(combat, 30);
    assert(combat.player.hp == hp - 2);
    assert(combat.player.get_status(StatusType::INTANGIBLE) == 0);
    end_turn_attacked(combat, 30);
    assert(combat.player.hp == hp - 32);

    // Block still absorbs the single point; Poison is an HP loss like another.
    CombatEngine shielded = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(shielded, 20);
    shielded.player.add_status(StatusType::INTANGIBLE, 2);
    shielded.player.add_status(StatusType::PLATED_ARMOR, 3);
    shielded.player.add_status(StatusType::POISON, 5);
    hp = shielded.player.hp;
    end_turn_attacked(shielded, 30);
    assert(shielded.player.hp == hp - 1); // the attack was blocked, Poison took 1
    assert(shielded.player.get_status(StatusType::POISON) == 4);
    assert(shielded.player.get_status(StatusType::INTANGIBLE) == 1);

    // An Intangible enemy takes 1 from each hit and from damage that is not an attack.
    CombatEngine ghost = make_combat(CharacterId::IRONCLAD);
    ghost.enemies[0].add_status(StatusType::INTANGIBLE, 1);
    int16_t foe_hp = ghost.enemies[0].hp;
    play(ghost, CardId::TWIN_STRIKE);
    assert(ghost.enemies[0].hp == foe_hp - 2);
    ghost.damage_enemy(ghost.enemies[0], 20);
    assert(ghost.enemies[0].hp == foe_hp - 3);
    std::cout << "[PASS] Intangible: hits and HP losses reduced to 1 until the end of the round" << std::endl;
}

void test_buffer() {
    CombatEngine combat = make_combat(CharacterId::DEFECT);
    pad_draw_pile(combat, 20);
    combat.player.add_status(StatusType::BUFFER, 2);
    int16_t hp = combat.player.hp;

    // An attack fully blocked does not use it.
    combat.player.add_status(StatusType::PLATED_ARMOR, 20);
    end_turn_attacked(combat, 10);
    assert(combat.player.hp == hp && combat.player.get_status(StatusType::BUFFER) == 2);
    combat.player.clear_status(StatusType::PLATED_ARMOR);

    // An attack, then an HP loss: both cancelled, nothing counted as lost.
    end_turn_attacked(combat, 10);
    assert(combat.player.hp == hp && combat.player.get_status(StatusType::BUFFER) == 1);
    combat.lose_hp(6);
    assert(combat.player.hp == hp && combat.player.get_status(StatusType::BUFFER) == 0);
    assert(combat.counters.hp_lost == 0 && combat.counters.combat_hp_losses == 0);

    // Spent: the next one goes through.
    combat.lose_hp(6);
    assert(combat.player.hp == hp - 6);
    std::cout << "[PASS] Buffer: the next HP loss is cancelled, one stack each" << std::endl;
}

void test_artifact() {
    // On an enemy: the debuff is cancelled, Artifact goes down, no reaction is raised.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    combat.add_power(PowerId::SLEIGHT_OF_FLESH, 9);
    Enemy& foe = combat.enemies[0];
    foe.add_status(StatusType::ARTIFACT, 2);
    int16_t foe_hp = foe.hp;
    combat.afflict(foe, StatusType::VULNERABLE, 2);
    assert(foe.get_status(StatusType::VULNERABLE) == 0 && foe.get_status(StatusType::ARTIFACT) == 1);
    assert(foe.hp == foe_hp);
    // A Strength loss is a debuff too, a Strength gain is not.
    combat.afflict(foe, StatusType::STRENGTH, 3);
    assert(foe.get_status(StatusType::STRENGTH) == 3 && foe.get_status(StatusType::ARTIFACT) == 1);
    combat.sap_strength(foe, 2);
    assert(foe.get_status(StatusType::STRENGTH) == 3 && foe.get_status(StatusType::ARTIFACT) == 0);
    assert(foe.temp_strength_loss == 0);
    // Spent: the next debuff lands and is heard.
    combat.afflict(foe, StatusType::WEAK, 1);
    assert(foe.get_status(StatusType::WEAK) == 1 && foe.hp == foe_hp - 9);

    // Doom cancelled is not counted as applied.
    CombatEngine doomed = make_combat(CharacterId::NECROBINDER);
    doomed.enemies[0].add_status(StatusType::ARTIFACT, 1);
    doomed.afflict(doomed.enemies[0], StatusType::DOOM, 5);
    assert(doomed.counters.doom_applied == 0 && doomed.enemies[0].get_status(StatusType::DOOM) == 0);

    // On the player: the debuffs of their own cards and powers are cancelled as well.
    CombatEngine self = make_combat(CharacterId::DEFECT);
    pad_draw_pile(self, 20);
    self.player.add_status(StatusType::ARTIFACT, 2);
    play(self, CardId::BIASED_COGNITION);
    assert(self.player.get_status(StatusType::FOCUS) == 5);
    end_turn_idle(self);
    assert(self.player.get_status(StatusType::FOCUS) == 5 && self.player.get_status(StatusType::ARTIFACT) == 1);
    assert(!self.player.receive_status(StatusType::FRAIL, 2));
    assert(self.player.get_status(StatusType::FRAIL) == 0 && self.player.get_status(StatusType::ARTIFACT) == 0);
    assert(self.player.receive_status(StatusType::FRAIL, 2));
    // A buff never uses it.
    self.player.add_status(StatusType::ARTIFACT, 1);
    assert(self.player.receive_status(StatusType::THORNS, 3));
    assert(self.player.get_status(StatusType::ARTIFACT) == 1);
    std::cout << "[PASS] Artifact: the next debuff is cancelled, for the enemies and for the player" << std::endl;
}

void test_frail() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(combat, 20);
    combat.player.add_status(StatusType::FRAIL, 2);
    // Defend: 5 Block, 3 when Frail (rounded down).
    play(combat, CardId::DEFEND);
    assert(combat.player.block == 3);
    // Dexterity is added before the cut: (5 + 3) * 0.75.
    combat.player.block = 0;
    combat.player.add_status(StatusType::DEXTERITY, 3);
    play(combat, CardId::DEFEND);
    assert(combat.player.block == 6);
    // Block that does not come from a card is not cut.
    combat.player.block = 0;
    combat.add_block(12);
    assert(combat.player.block == 12);

    // It wears off with the round, like Weak.
    end_turn_idle(combat);
    assert(combat.player.get_status(StatusType::FRAIL) == 1);
    end_turn_idle(combat);
    assert(combat.player.get_status(StatusType::FRAIL) == 0);
    combat.player.block = 0;
    play(combat, CardId::DEFEND);
    assert(combat.player.block == 8);
    std::cout << "[PASS] Frail: a quarter less Block from cards, until it wears off" << std::endl;
}

void test_vigor() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    combat.enemies[0].hp = 100;
    combat.player.add_status(StatusType::VIGOR, 5);
    // A Skill does not spend it.
    play(combat, CardId::DEFEND);
    assert(combat.player.get_status(StatusType::VIGOR) == 5);
    // Twin Strike: both hits get it (5 + 5 twice), then it is gone.
    play(combat, CardId::TWIN_STRIKE);
    assert(combat.enemies[0].hp == 100 - 20);
    assert(combat.player.get_status(StatusType::VIGOR) == 0);
    play(combat, CardId::STRIKE);
    assert(combat.enemies[0].hp == 100 - 26);

    // Added like Strength: before Weak and Vulnerable.
    CombatEngine weak = make_combat(CharacterId::IRONCLAD);
    weak.enemies[0].hp = 100;
    weak.player.add_status(StatusType::VIGOR, 6);
    weak.player.add_status(StatusType::WEAK, 1);
    play(weak, CardId::STRIKE);
    assert(weak.enemies[0].hp == 100 - 9); // (6 + 6) * 0.75

    // An enemy's Vigor is spent by its attack.
    CombatEngine brute = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(brute, 20);
    brute.enemies[0].add_status(StatusType::VIGOR, 4);
    int16_t hp = brute.player.hp;
    end_turn_attacked(brute, 5);
    assert(brute.player.hp == hp - 9 && brute.enemies[0].get_status(StatusType::VIGOR) == 0);
    std::cout << "[PASS] Vigor: added to every hit of the next Attack, then spent" << std::endl;
}

void test_regen() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    pad_draw_pile(combat, 20);
    combat.player.hp = 40;
    combat.player.add_status(StatusType::REGEN, 3);
    end_turn_idle(combat);
    assert(combat.player.hp == 43 && combat.player.get_status(StatusType::REGEN) == 2);
    end_turn_idle(combat);
    end_turn_idle(combat);
    assert(combat.player.hp == 46 && combat.player.get_status(StatusType::REGEN) == 0);
    end_turn_idle(combat);
    assert(combat.player.hp == 46);

    // Never above Max HP, and it still goes down.
    combat.player.hp = static_cast<int16_t>(combat.player.max_hp - 1);
    combat.player.add_status(StatusType::REGEN, 5);
    end_turn_idle(combat);
    assert(combat.player.hp == combat.player.max_hp && combat.player.get_status(StatusType::REGEN) == 4);

    // An enemy heals at the end of its own turn.
    combat.enemies[0].hp = 10;
    combat.enemies[0].add_status(StatusType::REGEN, 4);
    end_turn_idle(combat);
    assert(combat.enemies[0].hp == 14 && combat.enemies[0].get_status(StatusType::REGEN) == 3);
    std::cout << "[PASS] Regen: heals at the end of the turn, then goes down by 1" << std::endl;
}

void test_confused() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    // Bludgeon costs 3, Whirlwind costs X, Wound cannot be played.
    pad_draw_pile(combat, 40, CardId::BLUDGEON);
    combat.player.add_status(StatusType::CONFUSED, 1);

    bool seen[4] = { false, false, false, false };
    for (int i = 0; i < 40; ++i) {
        combat.hand.clear();
        assert(combat.draw_card());
        int cost = combat.effective_cost(combat.hand[0]);
        assert(cost >= 0 && cost <= 3);
        seen[cost] = true;
    }
    assert(seen[0] && seen[1] && seen[2] && seen[3]);

    combat.hand.clear();
    combat.draw_pile.push_back(CardId::WOUND);
    combat.draw_pile.push_back(CardId::WHIRLWIND);
    combat.draw_cards(2);
    assert(combat.hand[0].cost_delta == 0 && combat.effective_cost(combat.hand[0]) == COST_X);
    assert(combat.hand[1].cost_delta == 0);

    // Without the status, the cost is the printed one.
    CombatEngine clear = make_combat(CharacterId::IRONCLAD);
    clear.hand.clear();
    clear.draw_pile.clear();
    pad_draw_pile(clear, 5, CardId::BLUDGEON);
    clear.draw_cards(5);
    for (const DeckCard& card : clear.hand) assert(clear.effective_cost(card) == 3);
    std::cout << "[PASS] Confused: cards drawn cost 0 to 3 at random, X and Unplayable cards excepted" << std::endl;
}

// An entity holds every status at the same time: none is dropped for lack of room.
void test_every_status_at_once() {
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    const int last = static_cast<int>(StatusType::CONFUSED);
    for (int type = 1; type <= last; ++type) {
        combat.player.add_status(static_cast<StatusType>(type), 2);
        combat.enemies[0].add_status(static_cast<StatusType>(type), 2);
    }
    assert(combat.player.statuses.size() == static_cast<size_t>(last));
    for (int type = 1; type <= last; ++type) {
        assert(combat.player.get_status(static_cast<StatusType>(type)) == 2);
        assert(combat.enemies[0].get_status(static_cast<StatusType>(type)) == 2);
    }
    std::cout << "[PASS] Statuses: an entity holds every status at once" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Statuses ===" << std::endl;
    test_plating();
    test_thorns();
    test_intangible();
    test_buffer();
    test_artifact();
    test_frail();
    test_vigor();
    test_regen();
    test_confused();
    test_every_status_at_once();
    std::cout << "All status tests passed." << std::endl;
    return 0;
}
