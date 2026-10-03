// The Necrobinder's cards that use Osty (#155, #156) and Sentry Mode with its token. One case per card, with the
// base and the upgraded numbers of bible/data/card_v0.111.0.json.
#include <cassert>
#include <algorithm>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

// A Necrobinder against `count` enemies with 200 HP each, empty piles, and Osty alive with `osty` HP (0: dead).
CombatEngine make_fight(int count = 1, int16_t osty = 10) {
    FixedVector<EnemyType, 3> pack;
    for (int i = 0; i < count; ++i) pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::NECROBINDER);
    for (Enemy& foe : combat.enemies) {
        foe.max_hp = 200;
        foe.hp = 200;
        foe.block = 0;
    }
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    if (osty > 0) combat.player.summon_minion(osty);
    return combat;
}

// Ends the turn with enemies that do not attack, or that all attack for `damage`.
void end_turn(CombatEngine& combat, int16_t damage = 0) {
    for (Enemy& foe : combat.enemies) {
        foe.current_intent = EnemyIntent();
        foe.current_intent.type = damage > 0 ? EnemyIntentType::ATTACK : EnemyIntentType::BUFF;
        foe.current_intent.base_damage = damage;
    }
    combat.end_player_turn();
}

// Adds `card` to the hand with full energy and plays it.
void play(CombatEngine& combat, DeckCard card, int target = 0) {
    combat.hand.push_back(card);
    combat.player.energy = combat.player.max_energy;
    bool played = combat.play_card(static_cast<int>(combat.hand.size()) - 1, target);
    assert(played);
}

// HP the first enemy loses when `card` is played on it.
int16_t damage_of(CombatEngine& combat, DeckCard card) {
    int16_t before = combat.enemies[0].hp;
    play(combat, card);
    return static_cast<int16_t>(before - combat.enemies[0].hp);
}

bool blocked_by_condition(CardId id) {
    CombatEngine combat = make_fight(1, 0);
    combat.hand.push_back(DeckCard(id));
    combat.player.energy = 9;
    return combat.play_blocker(0) == PlayBlocker::CONDITION && !combat.play_card(0, 0);
}

template <size_t Capacity>
int count_of(const FixedVector<DeckCard, Capacity>& pile, CardId id) {
    int count = 0;
    for (const DeckCard& card : pile) count += card.id == id;
    return count;
}

void test_poke() {
    CombatEngine combat = make_fight();
    assert(CardDef::get_cost(CardId::POKE, false) == 0 && CardDef::get_cost(CardId::POKE, true) == 0);
    assert(damage_of(combat, DeckCard(CardId::POKE)) == 6);
    assert(damage_of(combat, DeckCard(CardId::POKE, true)) == 9);
    assert(combat.counters.osty_attacks == 2);
    // "Osty deals N damage" cannot be played while he is dead: true of every such card.
    for (CardId id : { CardId::POKE, CardId::SNAP, CardId::RATTLE, CardId::SQUEEZE, CardId::FLATTEN, CardId::HIGH_FIVE,
                       CardId::FETCH, CardId::RIGHT_HAND_HAND, CardId::PROTECTOR, CardId::SIC_EM,
                       CardId::SWEEPING_GAZE }) {
        assert(blocked_by_condition(id));
    }
    std::cout << "[PASS] Poke: Osty deals 6 (9) damage; Osty's attacks need him alive" << std::endl;
}

void test_snap() {
    CombatEngine combat = make_fight();
    assert(CardDef::get_cost(CardId::SNAP, false) == 1);
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    combat.hand.push_back(DeckCard(CardId::DEFEND));
    assert(damage_of(combat, DeckCard(CardId::SNAP)) == 7);
    // Two cards in hand: the player picks the one that gains Retain.
    assert(combat.choice_pending() && combat.pending_choices[0].kind == ChoiceKind::MAKE_RETAIN);
    assert(combat.choice_size() == 1);
    const uint8_t pick[] = { 1 };
    assert(combat.resolve_choice(pick));
    assert(!combat.choice_pending());
    assert(!CardDef::has_keyword(combat.hand[0], KEYWORD_RETAIN) && CardDef::has_keyword(combat.hand[1], KEYWORD_RETAIN));
    // The card stays in hand at the end of the turn, and the turn after: Retain is added for the combat.
    for (int i = 0; i < 12; ++i) combat.draw_pile.push_back(DeckCard(CardId::WISP)); // no reshuffle of the discard pile
    end_turn(combat);
    assert(count_of(combat.hand, CardId::DEFEND) == 1 && count_of(combat.hand, CardId::STRIKE) == 0);
    end_turn(combat);
    assert(count_of(combat.hand, CardId::DEFEND) == 1);

    // A single card that can gain Retain gains it without a choice; a card that already has it cannot be picked.
    CombatEngine single = make_fight();
    single.hand.push_back(DeckCard(CardId::SPUR));
    single.hand.push_back(DeckCard(CardId::STRIKE));
    assert(damage_of(single, DeckCard(CardId::SNAP, true)) == 10);
    assert(!single.choice_pending());
    assert((single.hand[1].added_keywords & KEYWORD_RETAIN) != 0 && single.hand[0].added_keywords == 0);

    // An empty hand: nothing to pick, the card still deals its damage.
    CombatEngine empty = make_fight();
    assert(damage_of(empty, DeckCard(CardId::SNAP)) == 7);
    assert(!empty.choice_pending());
    std::cout << "[PASS] Snap: Osty deals 7 (10) damage, a card of the hand gains Retain" << std::endl;
}

void test_rattle() {
    CombatEngine combat = make_fight();
    assert(CardDef::get_cost(CardId::RATTLE, false) == 1);
    // Osty has not attacked yet: one hit.
    assert(damage_of(combat, DeckCard(CardId::RATTLE)) == 7);
    assert(combat.counters.osty_attacks == 1);
    // One attack earlier this turn: two hits, and the card still counts as one attack.
    assert(damage_of(combat, DeckCard(CardId::RATTLE)) == 14);
    assert(combat.counters.osty_attacks == 2);
    assert(damage_of(combat, DeckCard(CardId::RATTLE, true)) == 27);
    // A new turn: the tally restarts.
    end_turn(combat);
    assert(damage_of(combat, DeckCard(CardId::RATTLE, true)) == 9);
    std::cout << "[PASS] Rattle: 7 (9) damage, one more hit for each earlier attack of Osty this turn" << std::endl;
}

void test_bone_shards() {
    CombatEngine combat = make_fight(2, 10);
    assert(CardDef::get_cost(CardId::BONE_SHARDS, false) == 1);
    play(combat, DeckCard(CardId::BONE_SHARDS));
    assert(combat.enemies[0].hp == 191 && combat.enemies[1].hp == 191);
    assert(combat.player.block == 9);
    assert(!combat.player.minion_alive() && combat.player.minion_max_hp == 0);
    assert(combat.counters.osty_attacks == 1);

    CombatEngine upgraded = make_fight(2, 10);
    play(upgraded, DeckCard(CardId::BONE_SHARDS, true));
    assert(upgraded.enemies[0].hp == 188 && upgraded.enemies[1].hp == 188 && upgraded.player.block == 12);

    // Osty dead: the card can be played and does nothing.
    CombatEngine dead = make_fight(2, 0);
    play(dead, DeckCard(CardId::BONE_SHARDS));
    assert(dead.enemies[0].hp == 200 && dead.player.block == 0 && dead.player.energy == dead.player.max_energy - 1);
    std::cout << "[PASS] Bone Shards: 9 (12) damage to all, 9 (12) Block, Osty dies; nothing when he is dead" << std::endl;
}

void test_squeeze() {
    CombatEngine combat = make_fight();
    assert(CardDef::get_cost(CardId::SQUEEZE, false) == 3 && CardDef::get_cost(CardId::SQUEEZE, true) == 3);
    // No other Osty Attack.
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    assert(damage_of(combat, DeckCard(CardId::SQUEEZE)) == 25);
    // The Squeeze just played is in the discard pile now: with it, three other Osty Attacks in the hand and the
    // draw and discard piles. The exhaust pile and the cards that are not Osty Attacks do not count.
    combat.hand.push_back(DeckCard(CardId::POKE));
    combat.draw_pile.push_back(DeckCard(CardId::UNLEASH));
    combat.draw_pile.push_back(DeckCard(CardId::BODYGUARD));
    combat.exhaust_pile.push_back(DeckCard(CardId::FETCH));
    assert(damage_of(combat, DeckCard(CardId::SQUEEZE)) == 25 + 5 * 3);
    assert(damage_of(combat, DeckCard(CardId::SQUEEZE, true)) == 30 + 6 * 4);
    std::cout << "[PASS] Squeeze: 25 (30) damage, 5 (6) more for each other Osty Attack" << std::endl;
}

void test_flatten() {
    CombatEngine combat = make_fight();
    DeckCard flatten(CardId::FLATTEN);
    assert(combat.effective_cost(flatten) == 2 && combat.effective_cost(DeckCard(CardId::FLATTEN, true)) == 2);
    assert(damage_of(combat, flatten) == 12);
    assert(combat.player.energy == combat.player.max_energy - 2);
    // Osty has attacked this turn: it costs 0.
    assert(combat.effective_cost(flatten) == 0);
    combat.hand.push_back(DeckCard(CardId::FLATTEN, true));
    combat.player.energy = 0;
    int16_t before = combat.enemies[0].hp;
    assert(combat.play_card(0, 0));
    assert(before - combat.enemies[0].hp == 16 && combat.player.energy == 0);
    // The next turn it costs 2 again.
    end_turn(combat);
    assert(combat.effective_cost(flatten) == 2);
    std::cout << "[PASS] Flatten: 12 (16) damage, costs 0 once Osty has attacked this turn" << std::endl;
}

void test_high_five() {
    CombatEngine combat = make_fight(2, 10);
    assert(CardDef::get_cost(CardId::HIGH_FIVE, false) == 2);
    play(combat, DeckCard(CardId::HIGH_FIVE));
    for (const Enemy& foe : combat.enemies) {
        assert(foe.hp == 189 && foe.get_status(StatusType::VULNERABLE) == 2);
    }
    assert(combat.counters.osty_attacks == 1);

    CombatEngine upgraded = make_fight(2, 10);
    play(upgraded, DeckCard(CardId::HIGH_FIVE, true));
    for (const Enemy& foe : upgraded.enemies) {
        assert(foe.hp == 187 && foe.get_status(StatusType::VULNERABLE) == 3);
    }
    std::cout << "[PASS] High Five: 11 (13) damage and 2 (3) Vulnerable to all enemies" << std::endl;
}

void test_fetch() {
    CombatEngine combat = make_fight();
    assert(CardDef::get_cost(CardId::FETCH, false) == 0);
    for (int i = 0; i < 6; ++i) combat.draw_pile.push_back(DeckCard(CardId::DEFEND));
    // The first Fetch of the turn draws 1 card, the second does not.
    assert(damage_of(combat, DeckCard(CardId::FETCH)) == 3);
    assert(combat.hand.size() == 1 && combat.hand[0].id == CardId::DEFEND);
    assert(damage_of(combat, DeckCard(CardId::FETCH, true)) == 6);
    assert(combat.hand.size() == 1);
    // A new turn: it draws again.
    end_turn(combat);
    size_t held = combat.hand.size();
    play(combat, DeckCard(CardId::FETCH, true));
    assert(combat.hand.size() == held + 1);
    std::cout << "[PASS] Fetch: 3 (6) damage, draws 1 card the first time it is played each turn" << std::endl;
}

void test_calcify() {
    CombatEngine combat = make_fight(2, 10);
    assert(CardDef::get_cost(CardId::CALCIFY, false) == 1 && CardDef::get_cost(CardId::CALCIFY, true) == 1);
    play(combat, DeckCard(CardId::CALCIFY));
    assert(combat.power_amount(PowerId::CALCIFY) == 4);
    assert(damage_of(combat, DeckCard(CardId::POKE)) == 10);
    play(combat, DeckCard(CardId::CALCIFY, true));
    assert(combat.power_amount(PowerId::CALCIFY) == 10);
    assert(damage_of(combat, DeckCard(CardId::POKE)) == 16);
    // Every hit of an attack on all enemies gets it; the player's own attacks do not.
    play(combat, DeckCard(CardId::HIGH_FIVE));
    assert(combat.enemies[1].hp == 200 - 21);
    CombatEngine own = make_fight();
    play(own, DeckCard(CardId::CALCIFY));
    assert(damage_of(own, DeckCard(CardId::STRIKE)) == 6);
    std::cout << "[PASS] Calcify: Osty's attacks deal 4 (6) more damage" << std::endl;
}

void test_right_hand_hand() {
    CombatEngine combat = make_fight();
    assert(CardDef::get_cost(CardId::RIGHT_HAND_HAND, false) == 0);
    assert(damage_of(combat, DeckCard(CardId::RIGHT_HAND_HAND)) == 4);
    assert(damage_of(combat, DeckCard(CardId::RIGHT_HAND_HAND, true)) == 6);
    assert(count_of(combat.discard_pile, CardId::RIGHT_HAND_HAND) == 2 && combat.hand.empty());
    // Cards that cost less than 2 leave it in the discard pile.
    play(combat, DeckCard(CardId::POKE));
    play(combat, DeckCard(CardId::SPUR));
    assert(count_of(combat.discard_pile, CardId::RIGHT_HAND_HAND) == 2);
    // A card that costs 2: every copy comes back to the hand, the upgraded one still upgraded.
    play(combat, DeckCard(CardId::HIGH_FIVE));
    assert(count_of(combat.discard_pile, CardId::RIGHT_HAND_HAND) == 0);
    assert(combat.hand.size() == 2 && count_of(combat.hand, CardId::RIGHT_HAND_HAND) == 2);
    assert(combat.hand[0].upgraded != combat.hand[1].upgraded);
    // A copy in the draw pile stays there.
    CombatEngine drawn = make_fight();
    drawn.draw_pile.push_back(DeckCard(CardId::RIGHT_HAND_HAND));
    play(drawn, DeckCard(CardId::SQUEEZE));
    assert(drawn.hand.empty() && drawn.draw_pile.size() == 1);
    // A card that costs 2 played for free does not bring it back: what counts is the energy paid.
    CombatEngine free_card = make_fight();
    free_card.discard_pile.push_back(DeckCard(CardId::RIGHT_HAND_HAND));
    play(free_card, DeckCard(CardId::POKE));
    play(free_card, DeckCard(CardId::FLATTEN)); // costs 0: Osty has attacked
    assert(free_card.hand.empty());
    std::cout << "[PASS] Right Hand Hand: 4 (6) damage, back from the discard pile when a card costing 2+ is played" << std::endl;
}

void test_necro_mastery() {
    CombatEngine combat = make_fight(2, 0);
    assert(CardDef::get_cost(CardId::NECRO_MASTERY, false) == 2 && CardDef::get_cost(CardId::NECRO_MASTERY, true) == 2);
    play(combat, DeckCard(CardId::NECRO_MASTERY));
    assert(combat.player.minion_hp == 5 && combat.player.minion_max_hp == 5);
    assert(combat.has_power(PowerId::NECRO_MASTERY));
    // Each enemy hits Osty for 2: he loses 4 HP in two hits, and every enemy loses 2 twice.
    end_turn(combat, 2);
    assert(combat.player.minion_hp == 1);
    assert(combat.enemies[0].hp == 196 && combat.enemies[1].hp == 196);

    CombatEngine upgraded = make_fight(1, 0);
    play(upgraded, DeckCard(CardId::NECRO_MASTERY, true));
    assert(upgraded.player.minion_hp == 8 && upgraded.player.minion_max_hp == 8);
    // Made to die, Osty loses the HP he had left.
    play(upgraded, DeckCard(CardId::SACRIFICE));
    assert(upgraded.enemies[0].hp == 192);
    std::cout << "[PASS] Necro Mastery: Summon 5 (8); every enemy loses the HP Osty loses" << std::endl;
}

void test_protector() {
    CombatEngine combat = make_fight(1, 10);
    assert(CardDef::get_cost(CardId::PROTECTOR, false) == 1 && CardDef::get_cost(CardId::PROTECTOR, true) == 0);
    // Max HP, not current HP: a wounded Osty hits as hard.
    end_turn(combat, 4);
    assert(combat.player.minion_hp == 6 && combat.player.minion_max_hp == 10);
    assert(damage_of(combat, DeckCard(CardId::PROTECTOR)) == 20);
    assert(damage_of(combat, DeckCard(CardId::PROTECTOR, true)) == 25);
    std::cout << "[PASS] Protector: 10 (15) damage plus Osty's Max HP; the upgrade costs 0" << std::endl;
}

void test_sacrifice() {
    CombatEngine combat = make_fight(1, 10);
    assert(CardDef::get_cost(CardId::SACRIFICE, false) == 1 && CardDef::get_cost(CardId::SACRIFICE, true) == 0);
    assert(CardDef::has_keyword(CardId::SACRIFICE, false, KEYWORD_RETAIN));
    assert(CardDef::has_keyword(CardId::SACRIFICE, true, KEYWORD_RETAIN));
    end_turn(combat, 4); // Osty: 6 HP out of 10
    play(combat, DeckCard(CardId::SACRIFICE));
    assert(combat.player.block == 30);
    assert(!combat.player.minion_alive() && combat.player.minion_max_hp == 0);
    // Osty dead: nothing.
    play(combat, DeckCard(CardId::SACRIFICE, true));
    assert(combat.player.block == 30);
    // Retain: it stays in hand at the end of the turn.
    combat.discard_pile.clear();
    combat.hand.push_back(DeckCard(CardId::SACRIFICE));
    end_turn(combat);
    assert(count_of(combat.hand, CardId::SACRIFICE) == 1);
    std::cout << "[PASS] Sacrifice: Retain; Osty dies, Block for three times his Max HP; the upgrade costs 0" << std::endl;
}

void test_spur() {
    CombatEngine combat = make_fight(1, 10);
    assert(CardDef::get_cost(CardId::SPUR, false) == 1 && CardDef::get_cost(CardId::SPUR, true) == 1);
    assert(CardDef::has_keyword(CardId::SPUR, false, KEYWORD_RETAIN) && CardDef::has_keyword(CardId::SPUR, true, KEYWORD_RETAIN));
    end_turn(combat, 8); // Osty: 2 HP out of 10
    play(combat, DeckCard(CardId::SPUR));
    // Summon 3: 5 HP out of 13. Then 5 HP healed.
    assert(combat.player.minion_hp == 10 && combat.player.minion_max_hp == 13);
    play(combat, DeckCard(CardId::SPUR, true));
    // Summon 5: 15 out of 18. Then 7 HP healed, up to his Max HP.
    assert(combat.player.minion_hp == 18 && combat.player.minion_max_hp == 18);

    // Osty dead: Summon brings him, and there is nothing left to heal.
    CombatEngine dead = make_fight(1, 0);
    play(dead, DeckCard(CardId::SPUR));
    assert(dead.player.minion_hp == 3 && dead.player.minion_max_hp == 3);
    std::cout << "[PASS] Spur: Retain; Summon 3 (5), Osty heals 5 (7) HP" << std::endl;
}

void test_sic_em() {
    CombatEngine combat = make_fight(2, 10);
    assert(CardDef::get_cost(CardId::SIC_EM, false) == 1 && CardDef::get_cost(CardId::SIC_EM, true) == 1);
    // The hit of the card comes before its mark: no Summon yet.
    assert(damage_of(combat, DeckCard(CardId::SIC_EM)) == 5);
    assert(combat.player.minion_max_hp == 10);
    // Osty hits the marked enemy: Summon 3. The other enemy is not marked.
    play(combat, DeckCard(CardId::POKE), 0);
    assert(combat.player.minion_hp == 13 && combat.player.minion_max_hp == 13);
    play(combat, DeckCard(CardId::POKE), 1);
    assert(combat.player.minion_max_hp == 13);
    // The player's own attacks are not Osty's.
    play(combat, DeckCard(CardId::STRIKE), 0);
    assert(combat.player.minion_max_hp == 13);
    // Each hit counts: Rattle hits 4 times here (3 earlier attacks of Osty).
    play(combat, DeckCard(CardId::RATTLE), 0);
    assert(combat.player.minion_max_hp == 13 + 4 * 3);
    // A second, upgraded, Sic 'Em on the same enemy: its own hit gives the 3 of the first one, then 3 + 4 a hit.
    int16_t before = combat.enemies[0].hp;
    play(combat, DeckCard(CardId::SIC_EM, true), 0);
    assert(before - combat.enemies[0].hp == 6);
    assert(combat.player.minion_max_hp == 28);
    play(combat, DeckCard(CardId::POKE), 0);
    assert(combat.player.minion_max_hp == 35);
    // An attack on all enemies hits the marked one once.
    play(combat, DeckCard(CardId::HIGH_FIVE));
    assert(combat.player.minion_max_hp == 42);
    // The mark is gone at the end of the turn.
    end_turn(combat);
    play(combat, DeckCard(CardId::POKE), 0);
    assert(combat.player.minion_max_hp == 42 && !combat.has_power(PowerId::SIC_EM));
    std::cout << "[PASS] Sic 'Em: 5 (6) damage; this turn, Summon 3 (4) each time Osty hits that enemy" << std::endl;
}

void test_sentry_mode_and_sweeping_gaze() {
    CombatEngine combat = make_fight(2, 10);
    assert(CardDef::get_cost(CardId::SENTRY_MODE, false) == 2 && CardDef::get_cost(CardId::SENTRY_MODE, true) == 1);
    play(combat, DeckCard(CardId::SENTRY_MODE));
    assert(combat.power_amount(PowerId::SENTRY_MODE) == 1);
    assert(count_of(combat.hand, CardId::SWEEPING_GAZE) == 0);
    // Start of the next turn: one Sweeping Gaze in hand.
    end_turn(combat);
    assert(count_of(combat.hand, CardId::SWEEPING_GAZE) == 1);
    assert(!combat.hand[0].upgraded);

    // The token: 0 energy, Ethereal, Exhaust, 10 damage from Osty to a random enemy.
    assert(CardDef::get_cost(CardId::SWEEPING_GAZE, false) == 0);
    assert(CardDef::get(CardId::SWEEPING_GAZE).type == CardType::ATTACK);
    assert(CardDef::get(CardId::SWEEPING_GAZE).target == TargetType::RANDOM_ENEMY);
    assert(CardDef::has_keyword(CardId::SWEEPING_GAZE, false, KEYWORD_ETHEREAL));
    assert(CardDef::has_keyword(CardId::SWEEPING_GAZE, false, KEYWORD_EXHAUST));
    combat.player.energy = 0;
    assert(combat.play_card(0, -1));
    assert(combat.enemies[0].hp + combat.enemies[1].hp == 390);
    assert(std::min(combat.enemies[0].hp, combat.enemies[1].hp) == 190);
    assert(combat.counters.osty_attacks == 1);
    assert(count_of(combat.exhaust_pile, CardId::SWEEPING_GAZE) == 1 && combat.discard_pile.empty());

    // Not played, it is exhausted at the end of the turn, and another one comes. A second Sentry Mode: two.
    play(combat, DeckCard(CardId::SENTRY_MODE, true));
    end_turn(combat);
    assert(count_of(combat.hand, CardId::SWEEPING_GAZE) == 2);
    end_turn(combat);
    assert(count_of(combat.hand, CardId::SWEEPING_GAZE) == 2);
    assert(count_of(combat.exhaust_pile, CardId::SWEEPING_GAZE) == 3);
    std::cout << "[PASS] Sentry Mode: a Sweeping Gaze each turn (Ethereal, Exhaust, Osty deals 10 to a random enemy)" << std::endl;
}

void test_reward_pool() {
    auto pool = RunManager::card_reward_pool(CharacterId::NECROBINDER);
    auto offered = [&pool](CardId id) { return std::find(pool.begin(), pool.end(), id) != pool.end(); };
    for (CardId id : { CardId::POKE, CardId::SNAP, CardId::RATTLE, CardId::BONE_SHARDS, CardId::SQUEEZE,
                       CardId::FLATTEN, CardId::HIGH_FIVE, CardId::FETCH, CardId::CALCIFY, CardId::RIGHT_HAND_HAND,
                       CardId::NECRO_MASTERY, CardId::SACRIFICE, CardId::SPUR, CardId::SIC_EM, CardId::SENTRY_MODE }) {
        assert(offered(id));
    }
    // The Ancient card and the token are not rewards.
    assert(!offered(CardId::PROTECTOR) && !offered(CardId::SWEEPING_GAZE));
    for (CharacterId other : { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT, CharacterId::REGENT }) {
        auto other_pool = RunManager::card_reward_pool(other);
        assert(std::find(other_pool.begin(), other_pool.end(), CardId::POKE) == other_pool.end());
    }
    std::cout << "[PASS] Osty's cards are offered to the Necrobinder, Protector and Sweeping Gaze are not" << std::endl;
}

} // namespace

int main() {
    test_poke();
    test_snap();
    test_rattle();
    test_bone_shards();
    test_squeeze();
    test_flatten();
    test_high_five();
    test_fetch();
    test_calcify();
    test_right_hand_hand();
    test_necro_mastery();
    test_protector();
    test_sacrifice();
    test_spur();
    test_sic_em();
    test_sentry_mode_and_sweeping_gaze();
    test_reward_pool();
    std::cout << "All Osty card tests passed." << std::endl;
    return 0;
}
