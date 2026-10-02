// Upgraded Silent cards: plays each upgraded version that no other test covers and checks what the upgrade changes.
// Expected values come from the CardDef lines of src/engine/card.cpp, cross-checked with bible/data/card_v0.111.0.json.
#include <algorithm>
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

// Jaw Worm opens with Chomp (11 damage, no block): a predictable punching bag.
CombatEngine make_combat(CharacterId hero, EnemyType enemy_type = EnemyType::JAW_WORM) {
    CombatEngine combat;
    combat.start_combat(enemy_type, hero);
    return combat;
}

// A fight against three enemies: two Lice (18 HP) and a Jaw Worm (42 HP).
CombatEngine make_pack(CharacterId hero) {
    CombatEngine combat;
    FixedVector<EnemyType, 3> pack;
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::LOUSE);
    pack.push_back(EnemyType::JAW_WORM);
    combat.start_combat_multi(pack, hero);
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

void pad_draw_pile(CombatEngine& combat, int count) {
    for (int i = 0; i < count; ++i) combat.draw_pile.push_back(CardId::DEFEND);
}

int count_in_hand(const CombatEngine& combat, CardId id) {
    int count = 0;
    for (const DeckCard& card : combat.hand) count += card.id == id;
    return count;
}

struct ExpectedDef {
    CardId id;
    uint8_t cost, value, secondary;
};

// Hero cards of RunManager::card_reward_pool(SILENT) and of the starting deck whose upgrade is tested here.
void test_upgraded_definitions() {
    static constexpr ExpectedDef BASE[] = {
        { CardId::NEUTRALIZE, 0, 3, 1 },     { CardId::SURVIVOR, 1, 8, 1 },       { CardId::DEADLY_POISON, 1, 0, 5 },
        { CardId::BACKFLIP, 1, 5, 2 },       { CardId::DAGGER_SPRAY, 1, 4, 2 },   { CardId::LEG_SWEEP, 2, 11, 2 },
        { CardId::PRECISE_CUT, 0, 13, 2 },   { CardId::FLECHETTES, 1, 5, 0 },     { CardId::FOOTWORK, 1, 2, 0 },
        { CardId::BUBBLE_BUBBLE, 1, 9, 0 },  { CardId::OUTBREAK, 3, 9, 0 },       { CardId::BACKSTAB, 0, 11, 0 },
        { CardId::BLUR, 1, 5, 0 },           { CardId::FINISHER, 1, 6, 0 },       { CardId::MEMENTO_MORI, 1, 9, 4 },
        { CardId::PREDATOR, 2, 15, 2 },      { CardId::EXPERTISE, 1, 0, 2 },      { CardId::ACCELERANT, 1, 1, 0 },
        { CardId::SPEEDSTER, 2, 2, 0 },      { CardId::SERPENT_FORM, 3, 4, 0 },   { CardId::AFTERIMAGE, 1, 1, 0 },
        { CardId::STRANGLE, 1, 8, 2 },       { CardId::NOXIOUS_FUMES, 1, 2, 0 }, { CardId::POUNCE, 2, 14, 0 },
    };
    static constexpr ExpectedDef UPGRADED[] = {
        { CardId::NEUTRALIZE, 0, 4, 2 },     { CardId::SURVIVOR, 1, 11, 1 },      { CardId::DEADLY_POISON, 1, 0, 7 },
        { CardId::BACKFLIP, 1, 8, 2 },       { CardId::DAGGER_SPRAY, 1, 6, 2 },   { CardId::LEG_SWEEP, 2, 14, 3 },
        { CardId::PRECISE_CUT, 0, 16, 2 },   { CardId::FLECHETTES, 1, 7, 0 },     { CardId::FOOTWORK, 1, 3, 0 },
        { CardId::BUBBLE_BUBBLE, 1, 12, 0 }, { CardId::OUTBREAK, 3, 12, 0 },      { CardId::BACKSTAB, 0, 15, 0 },
        { CardId::BLUR, 1, 8, 0 },           { CardId::FINISHER, 1, 8, 0 },       { CardId::MEMENTO_MORI, 1, 11, 5 },
        { CardId::PREDATOR, 2, 20, 2 },      { CardId::EXPERTISE, 1, 0, 3 },      { CardId::ACCELERANT, 1, 2, 0 },
        { CardId::SPEEDSTER, 2, 2, 0 },      { CardId::SERPENT_FORM, 3, 6, 0 },   { CardId::AFTERIMAGE, 1, 1, 0 },
        { CardId::STRANGLE, 1, 10, 3 },      { CardId::NOXIOUS_FUMES, 1, 3, 0 }, { CardId::POUNCE, 2, 20, 0 },
    };
    static_assert(std::size(BASE) == std::size(UPGRADED));
    std::span<const CardId> pool = RunManager::card_reward_pool(CharacterId::SILENT);
    for (size_t i = 0; i < std::size(BASE); ++i) {
        CardId id = BASE[i].id;
        assert(UPGRADED[i].id == id);
        // Neutralize and Survivor come from the starting deck, the others from the reward pool.
        if (id != CardId::NEUTRALIZE && id != CardId::SURVIVOR) {
            assert(std::find(pool.begin(), pool.end(), id) != pool.end());
        }
        assert(CardDef::get_cost(id, false) == BASE[i].cost);
        assert(CardDef::get_value(id, false) == BASE[i].value);
        assert(CardDef::get_secondary(id, false) == BASE[i].secondary);
        assert(CardDef::get_cost(id, true) == UPGRADED[i].cost);
        assert(CardDef::get_value(id, true) == UPGRADED[i].value);
        assert(CardDef::get_secondary(id, true) == UPGRADED[i].secondary);
    }

    // Keywords: no cost changes anywhere, Backstab+ keeps Innate and Exhaust, Speedster+ and Afterimage+ gain Innate.
    assert(CardDef::has_keyword(CardId::BACKSTAB, false, KEYWORD_INNATE) && CardDef::has_keyword(CardId::BACKSTAB, false, KEYWORD_EXHAUST));
    assert(CardDef::has_keyword(CardId::BACKSTAB, true, KEYWORD_INNATE) && CardDef::has_keyword(CardId::BACKSTAB, true, KEYWORD_EXHAUST));
    for (CardId id : { CardId::SPEEDSTER, CardId::AFTERIMAGE }) {
        assert(!CardDef::has_keyword(id, false, KEYWORD_INNATE));
        assert(CardDef::has_keyword(id, true, KEYWORD_INNATE));
        assert(!CardDef::has_keyword(id, true, KEYWORD_EXHAUST));
    }
    assert(!CardDef::has_keyword(CardId::BLUR, true, KEYWORD_INNATE));
    std::cout << "[PASS] Silent upgrades: cost, value and second value of 24 cards, Innate gained by Speedster+ and Afterimage+" << std::endl;
}

void test_upgraded_starting_cards() {
    // Neutralize+: 4 damage and 2 Weak for 0 energy; the Weak cuts the Chomp by 25 %.
    CombatEngine neutralize = make_combat(CharacterId::SILENT);
    neutralize.hand.clear();
    neutralize.hand.push_back(DeckCard(CardId::NEUTRALIZE, true));
    neutralize.player.energy = 0;
    assert(neutralize.play_card(0, -1));
    assert(neutralize.enemies[0].hp == 42 - 4 && neutralize.enemies[0].get_status(StatusType::WEAK) == 2);
    neutralize.end_player_turn();
    assert(neutralize.player.hp == 70 - 8);
    assert(neutralize.enemies[0].get_status(StatusType::WEAK) == 1); // the 2nd turn of Weak is still ahead

    // Survivor+: 11 Block (Dexterity included) for 1 energy.
    CombatEngine survivor = make_combat(CharacterId::SILENT);
    survivor.player.block = 0;
    survivor.player.add_status(StatusType::DEXTERITY, 2);
    play(survivor, DeckCard(CardId::SURVIVOR, true));
    assert(survivor.player.block == 13 && survivor.player.energy == survivor.player.max_energy - 1);
    std::cout << "[PASS] Silent upgraded starting cards: Neutralize+, Survivor+" << std::endl;
}

void test_upgraded_attacks() {
    // Dagger Spray+: 6 damage twice to every enemy.
    CombatEngine spray = make_pack(CharacterId::SILENT);
    play(spray, DeckCard(CardId::DAGGER_SPRAY, true));
    assert(spray.enemies[0].hp == 18 - 12 && spray.enemies[1].hp == 18 - 12 && spray.enemies[2].hp == 42 - 12);

    // Precise Cut+: 16 damage, still 2 less per other card in hand, never below 0.
    CombatEngine cut = make_combat(CharacterId::SILENT);
    cut.hand.clear();
    cut.hand.push_back(DeckCard(CardId::PRECISE_CUT, true));
    cut.hand.push_back(CardId::DEFEND);
    cut.hand.push_back(CardId::DEFEND);
    assert(cut.play_card(0));
    assert(cut.enemies[0].hp == 42 - 12 && cut.player.energy == 3);
    while (cut.hand.size() < 9) cut.hand.push_back(CardId::DEFEND);
    cut.hand.push_back(DeckCard(CardId::PRECISE_CUT, true));
    assert(cut.play_card(9));
    assert(cut.enemies[0].hp == 30); // 16 - 2 x 9 is negative: no damage, no healing

    // Flechettes+: one hit of 7 per Skill left in hand.
    CombatEngine darts = make_combat(CharacterId::SILENT);
    darts.hand.clear();
    darts.hand.push_back(DeckCard(CardId::FLECHETTES, true));
    darts.hand.push_back(CardId::DEFEND);
    darts.hand.push_back(CardId::SURVIVOR);
    darts.hand.push_back(CardId::STRIKE);
    assert(darts.play_card(0));
    assert(darts.enemies[0].hp == 42 - 14 && darts.counters.hits_dealt == 2);

    // Finisher+: 8 per Attack played before it this turn (the two Strikes).
    CombatEngine finisher = make_combat(CharacterId::SILENT);
    play(finisher, CardId::STRIKE);
    play(finisher, CardId::STRIKE);
    assert(finisher.enemies[0].hp == 42 - 12);
    play(finisher, DeckCard(CardId::FINISHER, true));
    assert(finisher.enemies[0].hp == 30 - 2 * 8 && finisher.counters.hits_dealt == 4); // the Finisher itself is not counted

    // Memento Mori+: 11 damage, 5 more per card discarded this turn.
    CombatEngine memento = make_combat(CharacterId::SILENT);
    memento.hand.clear();
    memento.hand.push_back(CardId::CALCULATED_GAMBLE);
    memento.hand.push_back(CardId::STRIKE);
    memento.hand.push_back(CardId::DEFEND);
    assert(memento.play_card(0));
    assert(memento.counters.cards_discarded == 2);
    play(memento, DeckCard(CardId::MEMENTO_MORI, true));
    assert(memento.enemies[0].hp == 42 - (11 + 2 * 5));

    // Predator+: 20 damage, still two more cards in the next opening draw.
    CombatEngine predator = make_combat(CharacterId::SILENT);
    pad_draw_pile(predator, 4);
    play(predator, DeckCard(CardId::PREDATOR, true));
    assert(predator.enemies[0].hp == 42 - 20 && predator.player.energy == predator.player.max_energy - 2);
    end_turn_idle(predator);
    assert(predator.hand.size() == 7);

    // Strangle+: 10 damage, then 3 HP per card played this turn, through Block.
    CombatEngine strangle = make_pack(CharacterId::SILENT);
    play(strangle, DeckCard(CardId::STRANGLE, true), 2);
    assert(strangle.enemies[2].hp == 42 - 10);
    strangle.enemies[2].block = 10;
    play(strangle, CardId::DEFEND);
    assert(strangle.enemies[2].hp == 32 - 3 && strangle.enemies[2].block == 10);
    play(strangle, CardId::DEFEND);
    assert(strangle.enemies[2].hp == 29 - 3);
    end_turn_idle(strangle);
    play(strangle, CardId::DEFEND);
    assert(strangle.enemies[2].hp == 26); // the effect only lasted the turn

    // Pounce+: 20 damage, and the next Skill costs 0 as before.
    CombatEngine pounce = make_combat(CharacterId::SILENT);
    play(pounce, DeckCard(CardId::POUNCE, true));
    assert(pounce.enemies[0].hp == 42 - 20 && pounce.player.energy == pounce.player.max_energy - 2);
    pounce.hand.push_back(CardId::STRIKE);
    pounce.hand.push_back(CardId::DEFEND);
    assert(pounce.effective_cost(pounce.hand[0]) == 1 && pounce.effective_cost(pounce.hand[1]) == 0);
    std::cout << "[PASS] Silent upgraded attacks: Dagger Spray+, Precise Cut+, Flechettes+, Finisher+, Memento Mori+, Predator+, Strangle+, Pounce+"
              << std::endl;
}

void test_upgraded_backstab() {
    // Backstab+: 15 damage for 0 energy, Exhausted after the play.
    CombatEngine backstab = make_combat(CharacterId::SILENT);
    backstab.hand.clear();
    backstab.hand.push_back(DeckCard(CardId::BACKSTAB, true));
    backstab.player.energy = 0;
    assert(backstab.play_card(0));
    assert(backstab.enemies[0].hp == 42 - 15 && backstab.exhaust_pile.size() == 1);
    assert(backstab.exhaust_pile[0].upgraded && backstab.discard_pile.empty());

    // ... and Innate: drawn in the opening hand wherever the shuffle put it.
    CombatEngine innate = make_combat(CharacterId::SILENT);
    innate.hand.clear();
    innate.discard_pile.clear();
    innate.draw_pile.clear();
    innate.draw_pile.push_back(DeckCard(CardId::BACKSTAB, true));
    for (int i = 0; i < 12; ++i) innate.draw_pile.push_back(CardId::STRIKE);
    innate.open_first_turn();
    assert(innate.hand.size() == 5 && count_in_hand(innate, CardId::BACKSTAB) == 1);
    std::cout << "[PASS] Silent upgraded Backstab+: 15 damage, free, Exhaust and Innate kept" << std::endl;
}

void test_upgraded_skills() {
    // Deadly Poison+: 7 Poison, which kills at the start of the enemy turn.
    CombatEngine deadly = make_combat(CharacterId::SILENT);
    deadly.enemies[0].hp = 6;
    play(deadly, DeckCard(CardId::DEADLY_POISON, true));
    assert(deadly.enemies[0].get_status(StatusType::POISON) == 7);
    deadly.end_player_turn();
    assert(!deadly.enemies[0].is_alive() && deadly.player.hp == 70 && deadly.state == CombatState::VICTORY);

    // Backflip+: 8 Block (Dexterity included) and still 2 cards.
    CombatEngine flip = make_combat(CharacterId::SILENT);
    flip.player.block = 0;
    flip.player.add_status(StatusType::DEXTERITY, 1);
    play(flip, DeckCard(CardId::BACKFLIP, true));
    assert(flip.player.block == 9 && flip.hand.size() == 2);

    // Leg Sweep+: 3 Weak and 14 Block for 2 energy.
    CombatEngine sweep = make_combat(CharacterId::SILENT);
    sweep.player.block = 0;
    play(sweep, DeckCard(CardId::LEG_SWEEP, true));
    assert(sweep.enemies[0].get_status(StatusType::WEAK) == 3 && sweep.player.block == 14);
    assert(sweep.player.energy == sweep.player.max_energy - 2);

    // Bubble Bubble+: needs a Poisoned enemy, then applies 12.
    CombatEngine bubble = make_combat(CharacterId::SILENT);
    play(bubble, DeckCard(CardId::BUBBLE_BUBBLE, true));
    assert(bubble.enemies[0].get_status(StatusType::POISON) == 0);
    bubble.enemies[0].add_status(StatusType::POISON, 1);
    play(bubble, DeckCard(CardId::BUBBLE_BUBBLE, true));
    assert(bubble.enemies[0].get_status(StatusType::POISON) == 13);

    // Outbreak+: 12 Poison on every enemy, ticking at once (12 damage, 11 Poison left).
    CombatEngine outbreak = make_pack(CharacterId::SILENT);
    play(outbreak, DeckCard(CardId::OUTBREAK, true));
    assert(outbreak.player.energy == outbreak.player.max_energy - 3);
    assert(outbreak.enemies[0].hp == 18 - 12 && outbreak.enemies[1].hp == 18 - 12 && outbreak.enemies[2].hp == 42 - 12);
    for (const Enemy& foe : outbreak.enemies) assert(foe.get_status(StatusType::POISON) == 11);

    // Blur+: 8 Block, kept through one start of turn.
    CombatEngine blur = make_combat(CharacterId::SILENT);
    play(blur, DeckCard(CardId::BLUR, true));
    assert(blur.player.block == 8);
    end_turn_idle(blur);
    assert(blur.player.block == 8 && blur.keep_block_turns == 0);
    end_turn_idle(blur);
    assert(blur.player.block == 0);

    // Expertise+: draws 3 cards instead of 2, each Retained for one end of turn.
    CombatEngine expertise = make_combat(CharacterId::SILENT);
    pad_draw_pile(expertise, 12);
    expertise.hand.clear();
    expertise.hand.push_back(DeckCard(CardId::EXPERTISE, true));
    expertise.hand.push_back(CardId::STRIKE);
    assert(expertise.play_card(0));
    assert(expertise.hand.size() == 1 + 3);
    for (size_t i = 1; i < expertise.hand.size(); ++i) assert(expertise.hand[i].added_keywords == KEYWORD_RETAIN_ONCE);
    assert(expertise.hand[0].added_keywords == 0);
    end_turn_idle(expertise);
    assert(expertise.hand.size() == 3 + 5); // the 3 retained cards, then a full opening hand
    std::cout << "[PASS] Silent upgraded skills: Deadly Poison+, Backflip+, Leg Sweep+, Bubble Bubble+, Outbreak+, Blur+, Expertise+" << std::endl;
}

void test_upgraded_powers() {
    // Footwork+: 3 Dexterity.
    CombatEngine footwork = make_combat(CharacterId::SILENT);
    play(footwork, DeckCard(CardId::FOOTWORK, true));
    assert(footwork.player.get_status(StatusType::DEXTERITY) == 3);
    footwork.player.block = 0;
    play(footwork, CardId::DEFEND);
    assert(footwork.player.block == 8);

    // Accelerant+: two more Poison ticks per enemy turn (5 + 4 + 3 damage, 2 Poison left).
    CombatEngine poison = make_combat(CharacterId::SILENT);
    play(poison, DeckCard(CardId::ACCELERANT, true));
    poison.enemies[0].add_status(StatusType::POISON, 5);
    end_turn_idle(poison);
    assert(poison.enemies[0].hp == 42 - 5 - 4 - 3 && poison.enemies[0].get_status(StatusType::POISON) == 2);

    // Serpent Form+: 6 damage to a random enemy per card played; the card that brings it does not count.
    CombatEngine serpent = make_combat(CharacterId::SILENT);
    play(serpent, DeckCard(CardId::SERPENT_FORM, true));
    assert(serpent.enemies[0].hp == 42 && serpent.player.energy == serpent.player.max_energy - 3);
    play(serpent, CardId::DEFEND);
    assert(serpent.enemies[0].hp == 42 - 6);

    // Noxious Fumes+: 3 Poison on every enemy at the start of each turn.
    CombatEngine fumes = make_pack(CharacterId::SILENT);
    play(fumes, DeckCard(CardId::NOXIOUS_FUMES, true));
    for (const Enemy& foe : fumes.enemies) assert(foe.get_status(StatusType::POISON) == 0);
    end_turn_idle(fumes);
    for (const Enemy& foe : fumes.enemies) assert(foe.get_status(StatusType::POISON) == 3);
    end_turn_idle(fumes);
    for (const Enemy& foe : fumes.enemies) assert(foe.get_status(StatusType::POISON) == 3 - 1 + 3);

    // Speedster+: same 2 damage per card drawn, and now Innate.
    CombatEngine speedster = make_pack(CharacterId::SILENT);
    pad_draw_pile(speedster, 20);
    play(speedster, DeckCard(CardId::SPEEDSTER, true));
    assert(speedster.player.energy == speedster.player.max_energy - 2);
    end_turn_idle(speedster);
    assert(speedster.enemies[2].hp == 42); // the opening draw does not trigger it
    play(speedster, CardId::BACKFLIP);
    assert(speedster.enemies[0].hp == 18 - 4 && speedster.enemies[2].hp == 42 - 4);

    CombatEngine speedster_innate = make_combat(CharacterId::SILENT);
    speedster_innate.hand.clear();
    speedster_innate.discard_pile.clear();
    speedster_innate.draw_pile.clear();
    speedster_innate.draw_pile.push_back(DeckCard(CardId::SPEEDSTER, true));
    for (int i = 0; i < 12; ++i) speedster_innate.draw_pile.push_back(CardId::STRIKE);
    speedster_innate.open_first_turn();
    assert(count_in_hand(speedster_innate, CardId::SPEEDSTER) == 1);

    // Afterimage+: same 1 Block per card played, and now Innate.
    CombatEngine afterimage = make_combat(CharacterId::SILENT);
    play(afterimage, DeckCard(CardId::AFTERIMAGE, true));
    assert(afterimage.player.block == 0 && afterimage.player.energy == afterimage.player.max_energy - 1);
    play(afterimage, CardId::DEFEND);
    assert(afterimage.player.block == 5 + 1);

    CombatEngine afterimage_innate = make_combat(CharacterId::SILENT);
    afterimage_innate.hand.clear();
    afterimage_innate.discard_pile.clear();
    afterimage_innate.draw_pile.clear();
    afterimage_innate.draw_pile.push_back(DeckCard(CardId::AFTERIMAGE, true));
    for (int i = 0; i < 12; ++i) afterimage_innate.draw_pile.push_back(CardId::STRIKE);
    afterimage_innate.open_first_turn();
    assert(count_in_hand(afterimage_innate, CardId::AFTERIMAGE) == 1);
    std::cout << "[PASS] Silent upgraded powers: Footwork+, Accelerant+, Serpent Form+, Noxious Fumes+, Speedster+, Afterimage+" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Silent Upgraded Cards ===" << std::endl;
    test_upgraded_definitions();
    test_upgraded_starting_cards();
    test_upgraded_attacks();
    test_upgraded_backstab();
    test_upgraded_skills();
    test_upgraded_powers();
    std::cout << "All Silent upgraded card tests passed." << std::endl;
    return 0;
}
