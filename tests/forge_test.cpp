// Forge and the Sovereign Blade of the Regent (#102): one case per rule and per card group, on hand-built situations.
#include <algorithm>
#include <cassert>
#include <initializer_list>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

constexpr int16_t LOTS_OF_HP = 500;

// Enemies with enough HP to take any Blade, and nothing in the piles but what the test puts there.
CombatEngine make_combat(int enemy_count = 1) {
    CombatEngine combat;
    FixedVector<EnemyType, 3> pack;
    for (int i = 0; i < enemy_count; ++i) pack.push_back(EnemyType::JAW_WORM);
    combat.start_combat_multi(pack, CharacterId::REGENT);
    for (Enemy& foe : combat.enemies) {
        foe.max_hp = LOTS_OF_HP;
        foe.hp = LOTS_OF_HP;
        foe.block = 0;
    }
    combat.hand.clear();
    combat.draw_pile.clear();
    combat.discard_pile.clear();
    combat.exhaust_pile.clear();
    combat.player.block = 0;
    combat.player.energy = combat.player.max_energy;
    return combat;
}

template <size_t Capacity>
int count_in(const FixedVector<DeckCard, Capacity>& pile, CardId id) {
    int count = 0;
    for (const DeckCard& card : pile) count += card.id == id;
    return count;
}

// The first copy of `id` in the hand, -1 when there is none.
int find_in_hand(const CombatEngine& combat, CardId id) {
    for (size_t i = 0; i < combat.hand.size(); ++i) {
        if (combat.hand[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

// Adds `card` to the hand, the other cards staying there, and plays it with full energy.
void play(CombatEngine& combat, DeckCard card, int target = 0) {
    combat.player.energy = combat.player.max_energy;
    bool added = combat.hand.push_back(card);
    assert(added);
    bool played = combat.play_card(static_cast<int>(combat.hand.size()) - 1, target);
    assert(played);
}

// Plays the Sovereign Blade held in hand, with full energy. Returns the HP the target lost.
int play_blade(CombatEngine& combat, int target = 0) {
    int index = find_in_hand(combat, CardId::SOVEREIGN_BLADE);
    assert(index >= 0);
    combat.player.energy = combat.player.max_energy;
    int16_t before = combat.enemies[target].hp;
    bool played = combat.play_card(index, target);
    assert(played);
    return before - combat.enemies[target].hp;
}

// Takes the Blade back from the discard pile, as a draw would.
void take_blade_back(CombatEngine& combat) {
    for (size_t i = 0; i < combat.discard_pile.size(); ++i) {
        if (combat.discard_pile[i].id != CardId::SOVEREIGN_BLADE) continue;
        combat.hand.push_back(combat.discard_pile[i]);
        combat.discard_pile.erase(i);
        return;
    }
    assert(false);
}

void end_turn_idle(CombatEngine& combat) {
    for (Enemy& foe : combat.enemies) {
        foe.current_intent = EnemyIntent{};
        foe.current_intent.type = EnemyIntentType::DEFEND;
    }
    combat.end_player_turn();
    for (Enemy& foe : combat.enemies) foe.block = 0;
}

void test_forge_rule() {
    // The first Forge of the combat adds the Sovereign Blade to the hand, with what was forged.
    CombatEngine combat = make_combat();
    assert(combat.forged == 0 && count_in(combat.hand, CardId::SOVEREIGN_BLADE) == 0);
    combat.forge(6);
    assert(combat.hand.size() == 1 && combat.hand[0].id == CardId::SOVEREIGN_BLADE && combat.hand[0].bonus == 6);
    assert(combat.forged == 6 && combat.counters.combat_cards_created == 1);

    // The next ones only raise its damage: there is still one Blade.
    combat.forge(4);
    assert(combat.hand.size() == 1 && combat.hand[0].bonus == 10 && combat.forged == 10);
    combat.forge(0);
    combat.forge(-3);
    assert(combat.hand[0].bonus == 10 && combat.forged == 10);

    // The Blade: 2 energy, 10 damage plus what was forged, then to the discard pile.
    const CardDef& blade = CardDef::get(CardId::SOVEREIGN_BLADE);
    assert(blade.cost == 2 && blade.type == CardType::ATTACK && blade.base_value == 10);
    assert(CardDef::has_keyword(CardId::SOVEREIGN_BLADE, false, KEYWORD_RETAIN));
    assert(play_blade(combat) == 20);
    assert(combat.player.energy == combat.player.max_energy - 2);
    assert(count_in(combat.discard_pile, CardId::SOVEREIGN_BLADE) == 1 && combat.hand.empty());

    // It is forged wherever it is: discard pile, draw pile, exhaust pile.
    combat.forge(5);
    assert(combat.discard_pile[0].bonus == 15 && combat.hand.empty());
    combat.draw_pile.push_back(combat.discard_pile[0]);
    combat.discard_pile.clear();
    combat.forge(1);
    assert(combat.draw_pile[0].bonus == 16);
    combat.exhaust_pile.push_back(combat.draw_pile[0]);
    combat.draw_pile.clear();
    combat.forge(1);
    assert(combat.exhaust_pile[0].bonus == 17 && combat.forged == 17);

    // Strength, Weak and Vulnerable apply to the Blade as to any attack.
    CombatEngine strong = make_combat();
    strong.forge(2);
    strong.player.add_status(StatusType::STRENGTH, 3);
    strong.enemies[0].add_status(StatusType::VULNERABLE, 2);
    assert(play_blade(strong) == (10 + 2 + 3) * 3 / 2);

    // Retain: the Blade stays in hand at the end of the turn.
    CombatEngine kept = make_combat();
    for (int i = 0; i < 10; ++i) kept.draw_pile.push_back(CardId::DEFEND);
    kept.forge(3);
    kept.hand.push_back(CardId::DEFEND);
    end_turn_idle(kept);
    assert(count_in(kept.hand, CardId::SOVEREIGN_BLADE) == 1 && kept.hand.size() == 6);
    assert(kept.hand[find_in_hand(kept, CardId::SOVEREIGN_BLADE)].bonus == 3);

    // A full hand sends the new Blade to the discard pile, already forged.
    CombatEngine full = make_combat();
    for (int i = 0; i < 10; ++i) full.hand.push_back(CardId::DEFEND);
    full.forge(9);
    assert(full.hand.size() == 10 && full.discard_pile.size() == 1 && full.discard_pile[0].bonus == 9);

    // A Blade that left the combat is not made again: only the first Forge makes one.
    CombatEngine gone = make_combat();
    gone.forge(5);
    gone.hand.clear();
    gone.forge(5);
    assert(gone.hand.empty() && gone.forged == 10);

    // What was forged does not outlive the combat.
    gone.start_combat(EnemyType::JAW_WORM, CharacterId::REGENT);
    assert(gone.forged == 0);
    gone.hand.clear();
    gone.forge(2);
    assert(gone.hand.size() == 1 && gone.hand[0].bonus == 2);
    std::cout << "[PASS] Forge: the first one makes the Sovereign Blade, each one raises its damage for the combat" << std::endl;
}

void test_commons() {
    // Refine Blade: Forge 8 (12), 1 energy next turn.
    CombatEngine refine = make_combat();
    for (int i = 0; i < 10; ++i) refine.draw_pile.push_back(CardId::DEFEND);
    play(refine, CardId::REFINE_BLADE);
    assert(refine.forged == 8 && refine.player.energy == refine.player.max_energy - 1);
    play(refine, DeckCard(CardId::REFINE_BLADE, true));
    assert(refine.forged == 20 && count_in(refine.hand, CardId::SOVEREIGN_BLADE) == 1);
    end_turn_idle(refine);
    assert(refine.player.energy == refine.player.max_energy + 2);

    // Spoils of Battle: Forge 6 (9), draw 2.
    CombatEngine spoils = make_combat();
    for (int i = 0; i < 10; ++i) spoils.draw_pile.push_back(CardId::DEFEND);
    play(spoils, CardId::SPOILS_OF_BATTLE);
    assert(spoils.forged == 6 && spoils.hand.size() == 3 && count_in(spoils.hand, CardId::DEFEND) == 2);
    play(spoils, DeckCard(CardId::SPOILS_OF_BATTLE, true));
    assert(spoils.forged == 15 && spoils.hand.size() == 5);

    // Wrought in War: 7 damage and Forge 7 (9 and 9).
    CombatEngine wrought = make_combat();
    play(wrought, CardId::WROUGHT_IN_WAR);
    assert(wrought.enemies[0].hp == LOTS_OF_HP - 7 && wrought.forged == 7);
    play(wrought, DeckCard(CardId::WROUGHT_IN_WAR, true));
    assert(wrought.enemies[0].hp == LOTS_OF_HP - 16 && wrought.forged == 16);
    assert(play_blade(wrought) == 26);
    std::cout << "[PASS] Commons: Refine Blade, Spoils of Battle, Wrought in War" << std::endl;
}

void test_uncommons() {
    // Bulwark: 12 Block and Forge 10 (15 and 13), for 2 energy.
    CombatEngine bulwark = make_combat();
    play(bulwark, CardId::BULWARK);
    assert(bulwark.player.block == 12 && bulwark.forged == 10 && bulwark.player.energy == bulwark.player.max_energy - 2);
    play(bulwark, DeckCard(CardId::BULWARK, true));
    assert(bulwark.player.block == 27 && bulwark.forged == 23);

    // Conqueror: Forge 3 (5); the Blade deals double damage to that enemy, this turn only.
    CombatEngine conqueror = make_combat(2);
    for (int i = 0; i < 10; ++i) conqueror.draw_pile.push_back(CardId::DEFEND);
    play(conqueror, CardId::CONQUEROR, 1);
    assert(conqueror.forged == 3);
    assert(play_blade(conqueror, 0) == 13);
    take_blade_back(conqueror);
    assert(play_blade(conqueror, 1) == 26);
    // Strength is doubled with the rest; a second Conqueror on the same enemy does not double again.
    take_blade_back(conqueror);
    play(conqueror, DeckCard(CardId::CONQUEROR, true), 1);
    assert(conqueror.forged == 8);
    conqueror.player.add_status(StatusType::STRENGTH, 2);
    assert(play_blade(conqueror, 1) == (10 + 8 + 2) * 2);
    take_blade_back(conqueror);
    end_turn_idle(conqueror);
    conqueror.player.clear_status(StatusType::STRENGTH);
    assert(play_blade(conqueror, 1) == 18);

    // Furnace: Forge 5 (7) at the start of each turn.
    CombatEngine furnace = make_combat();
    for (int i = 0; i < 20; ++i) furnace.draw_pile.push_back(CardId::DEFEND);
    play(furnace, CardId::FURNACE);
    assert(furnace.forged == 0 && furnace.hand.empty());
    end_turn_idle(furnace);
    assert(furnace.forged == 5 && count_in(furnace.hand, CardId::SOVEREIGN_BLADE) == 1 && furnace.hand.size() == 6);
    play(furnace, DeckCard(CardId::FURNACE, true));
    end_turn_idle(furnace);
    assert(furnace.forged == 17 && count_in(furnace.hand, CardId::SOVEREIGN_BLADE) == 1);

    // Parry: the Blade gives 10 (14) Block when it is played, whatever the Dexterity.
    CombatEngine parry = make_combat();
    parry.forge(1);
    play(parry, CardId::PARRY);
    parry.player.add_status(StatusType::DEXTERITY, 3);
    assert(parry.player.block == 0);
    assert(play_blade(parry) == 11 && parry.player.block == 10);
    take_blade_back(parry);
    play(parry, DeckCard(CardId::PARRY, true));
    play_blade(parry);
    assert(parry.player.block == 34);
    // No other attack gives that Block.
    play(parry, CardId::WROUGHT_IN_WAR);
    assert(parry.player.block == 34);

    // Summon Forth: the Blade comes to the hand from the draw, discard or exhaust pile, then Forge 8 (11).
    for (int place = 0; place < 3; ++place) {
        CombatEngine summon = make_combat();
        summon.forge(2);
        DeckCard blade = summon.hand[0];
        summon.hand.clear();
        if (place == 0) summon.draw_pile.push_back(blade);
        if (place == 1) summon.discard_pile.push_back(blade);
        if (place == 2) summon.exhaust_pile.push_back(blade);
        play(summon, place == 2 ? DeckCard(CardId::SUMMON_FORTH, true) : DeckCard(CardId::SUMMON_FORTH));
        assert(summon.hand.size() == 1 && summon.hand[0].id == CardId::SOVEREIGN_BLADE);
        assert(summon.hand[0].bonus == (place == 2 ? 13 : 10));
        assert(summon.draw_pile.empty() && summon.exhaust_pile.empty() && summon.discard_pile.size() == 1);
    }
    // Without a Blade yet, its Forge makes it.
    CombatEngine first = make_combat();
    play(first, CardId::SUMMON_FORTH);
    assert(first.hand.size() == 1 && first.hand[0].id == CardId::SOVEREIGN_BLADE && first.hand[0].bonus == 8);
    std::cout << "[PASS] Uncommons: Bulwark, Conqueror, Furnace, Parry, Summon Forth" << std::endl;
}

void test_rares() {
    // Beat Into Shape: 5 damage, Forge 5, and 5 more for each earlier hit on that enemy this turn.
    CombatEngine beat = make_combat(2);
    for (int i = 0; i < 10; ++i) beat.draw_pile.push_back(CardId::DEFEND);
    play(beat, CardId::BEAT_INTO_SHAPE, 0);
    assert(beat.enemies[0].hp == LOTS_OF_HP - 5 && beat.forged == 5);
    play(beat, CardId::BEAT_INTO_SHAPE, 0); // one earlier hit
    assert(beat.forged == 15);
    play(beat, CardId::STRIKE, 1); // a hit on the other enemy does not count
    play(beat, DeckCard(CardId::BEAT_INTO_SHAPE, true), 0); // two earlier hits, 7 each
    assert(beat.enemies[0].hp == LOTS_OF_HP - 17 && beat.forged == 15 + 21);
    play(beat, CardId::BEAT_INTO_SHAPE, 1); // one earlier hit on this one
    assert(beat.forged == 36 + 10);
    // The hits of the previous turn are forgotten.
    end_turn_idle(beat);
    int16_t before = beat.forged;
    play(beat, CardId::BEAT_INTO_SHAPE, 0);
    assert(beat.forged == before + 5);

    // Big Bang: draw 1, 1 energy, 1 Star, Forge 5, Exhaust; Innate once upgraded.
    CombatEngine bang = make_combat();
    for (int i = 0; i < 10; ++i) bang.draw_pile.push_back(CardId::DEFEND);
    uint8_t stars = bang.player.sovereign_stars;
    play(bang, CardId::BIG_BANG);
    assert(bang.player.energy == bang.player.max_energy + 1 && bang.player.sovereign_stars == stars + 1);
    assert(bang.forged == 5 && count_in(bang.hand, CardId::DEFEND) == 1 && count_in(bang.hand, CardId::SOVEREIGN_BLADE) == 1);
    assert(count_in(bang.exhaust_pile, CardId::BIG_BANG) == 1);
    assert(!CardDef::has_keyword(CardId::BIG_BANG, false, KEYWORD_INNATE));
    assert(CardDef::has_keyword(CardId::BIG_BANG, true, KEYWORD_INNATE) && CardDef::has_keyword(CardId::BIG_BANG, true, KEYWORD_EXHAUST));
    assert(CardDef::get_value(CardId::BIG_BANG, true) == 5);

    // Seeking Edge: Forge 7 (11); the Blade hits every enemy.
    CombatEngine edge = make_combat(3);
    play(edge, CardId::SEEKING_EDGE);
    assert(edge.forged == 7);
    assert(play_blade(edge, 1) == 17);
    assert(edge.enemies[0].hp == LOTS_OF_HP - 17 && edge.enemies[2].hp == LOTS_OF_HP - 17);
    take_blade_back(edge);
    play(edge, DeckCard(CardId::SEEKING_EDGE, true));
    assert(edge.forged == 18);

    // Sword Sage: the Blade is played once more, each time; it costs 2 (1 upgraded).
    CombatEngine sage = make_combat();
    sage.forge(4);
    assert(CardDef::get_cost(CardId::SWORD_SAGE, false) == 2 && CardDef::get_cost(CardId::SWORD_SAGE, true) == 1);
    play(sage, CardId::SWORD_SAGE);
    assert(play_blade(sage) == 28 && sage.player.energy == sage.player.max_energy - 2);
    assert(count_in(sage.discard_pile, CardId::SOVEREIGN_BLADE) == 1);
    take_blade_back(sage);
    assert(play_blade(sage) == 28);
    // A second Sword Sage: three plays. Other attacks are not replayed.
    take_blade_back(sage);
    play(sage, DeckCard(CardId::SWORD_SAGE, true));
    assert(play_blade(sage) == 42);
    int16_t hp = sage.enemies[0].hp;
    play(sage, CardId::STRIKE);
    assert(sage.enemies[0].hp == hp - 6);

    // The Smith: 1 energy and 4 Stars, Forge 30 (40).
    CombatEngine smith = make_combat();
    smith.player.sovereign_stars = 3;
    smith.hand.push_back(CardId::THE_SMITH);
    assert(smith.play_blocker(0) == PlayBlocker::STARS);
    smith.hand.clear();
    smith.player.sovereign_stars = 9;
    play(smith, CardId::THE_SMITH);
    assert(smith.forged == 30 && smith.player.sovereign_stars == 5 && smith.player.energy == smith.player.max_energy - 1);
    play(smith, DeckCard(CardId::THE_SMITH, true));
    assert(smith.forged == 70 && smith.player.sovereign_stars == 1);
    assert(play_blade(smith) == 80);
    std::cout << "[PASS] Rares: Beat Into Shape, Big Bang, Seeking Edge, Sword Sage, The Smith" << std::endl;
}

void test_modifiers_together() {
    // Seeking Edge, Sword Sage, Parry and Conqueror on the same Blade.
    CombatEngine combat = make_combat(2);
    play(combat, CardId::SEEKING_EDGE);         // Forge 7
    play(combat, CardId::SWORD_SAGE);
    play(combat, CardId::PARRY);
    play(combat, CardId::CONQUEROR, 1);         // Forge 3: the Blade deals 20
    assert(combat.forged == 10);
    play_blade(combat, 0);
    // Two plays, each on both enemies, doubled on the second one; Parry gives its Block for each play.
    assert(combat.enemies[0].hp == LOTS_OF_HP - 40);
    assert(combat.enemies[1].hp == LOTS_OF_HP - 80);
    assert(combat.player.block == 20);
    assert(combat.counters.attacks_played == 2);
    std::cout << "[PASS] Blade modifiers: every enemy, double damage, Block and Replay add up" << std::endl;
}

void test_relic_and_potion() {
    // Fencing Manual: Forge 10 at the start of each combat.
    RunManager run;
    run.start_run(CharacterId::REGENT, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("FENCING_MANUAL");
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    assert(run.combat.forged == 10 && run.combat.hand.size() == 6);
    int blade = find_in_hand(run.combat, CardId::SOVEREIGN_BLADE);
    assert(blade >= 0 && run.combat.hand[blade].bonus == 10);
    std::span<const std::string_view> droppable = RunManager::active_relics();
    assert(std::find(droppable.begin(), droppable.end(), "FENCING_MANUAL") != droppable.end());

    // King's Courage: Forge 15.
    CombatEngine potion = make_combat();
    assert(potion.use_potion("KINGS_COURAGE"));
    assert(potion.forged == 15 && potion.hand.size() == 1 && potion.hand[0].bonus == 15);
    assert(play_blade(potion) == 25);

    // The cards are in the Regent's rewards; the Blade is a token and is never offered.
    std::span<const CardId> pool = RunManager::card_reward_pool(CharacterId::REGENT);
    for (CardId id : { CardId::REFINE_BLADE, CardId::SPOILS_OF_BATTLE, CardId::WROUGHT_IN_WAR, CardId::BULWARK,
                       CardId::CONQUEROR, CardId::FURNACE, CardId::PARRY, CardId::SUMMON_FORTH, CardId::BEAT_INTO_SHAPE,
                       CardId::BIG_BANG, CardId::SEEKING_EDGE, CardId::SWORD_SAGE, CardId::THE_SMITH }) {
        assert(std::count(pool.begin(), pool.end(), id) == 1);
    }
    assert(std::find(pool.begin(), pool.end(), CardId::SOVEREIGN_BLADE) == pool.end());
    std::cout << "[PASS] Fencing Manual, King's Courage, and the Regent's reward pool" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Forge ===" << std::endl;
    test_forge_rule();
    test_commons();
    test_uncommons();
    test_rares();
    test_modifiers_together();
    test_relic_and_potion();
    std::cout << "All Forge tests passed." << std::endl;
    return 0;
}
