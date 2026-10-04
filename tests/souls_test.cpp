// Souls of the Necrobinder (#106). In bible/data a Soul is a card, not a gauge: a token that costs 0, "Draw 2
// cards. Exhaust.", which other cards add to the draw pile, the hand or the discard pile. One case per rule.
// Then the cards, the relic and the potion that make Souls or count them (#154), with the numbers of bible/data.
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

// A Necrobinder against `count` enemies with 100 HP each, hand emptied: the draw pile holds the starter deck.
CombatEngine make_fight(int count = 1) {
    FixedVector<EnemyType, 3> pack;
    for (int i = 0; i < count; ++i) pack.push_back(EnemyType::JAW_WORM);
    CombatEngine combat;
    combat.start_combat_multi(pack, CharacterId::NECROBINDER);
    for (Enemy& foe : combat.enemies) {
        foe.max_hp = 100;
        foe.hp = 100;
    }
    while (!combat.hand.empty()) {
        combat.draw_pile.push_back(combat.hand.back());
        combat.hand.pop_back();
    }
    return combat;
}

// Puts `card` alone in hand and plays it. The energy is left as it is: a Soul costs nothing.
void play(CombatEngine& combat, DeckCard card, int target = -1) {
    combat.hand.clear();
    combat.hand.push_back(card);
    bool played = combat.play_card(0, target);
    assert(played);
}

template <size_t Capacity>
int souls_in(const FixedVector<DeckCard, Capacity>& pile, bool upgraded_only = false) {
    int count = 0;
    for (const DeckCard& card : pile) count += is_soul(card.id) && (card.upgraded || !upgraded_only);
    return count;
}

int total_hp(const CombatEngine& combat) {
    int total = 0;
    for (const Enemy& foe : combat.enemies) total += foe.hp;
    return total;
}

// ---- Souls are cards ----------------------------------------------------------------------------------------

void test_a_combat_starts_without_souls() {
    CombatEngine combat = make_fight();
    assert(souls_in(combat.draw_pile) == 0 && souls_in(combat.hand) == 0 && souls_in(combat.discard_pile) == 0);
    assert(combat.exhausted_souls() == 0);
    assert(!is_soul(CardId::NONE) && !is_soul(CardId::STRIKE));
}

void test_adding_souls_makes_cards() {
    CombatEngine combat = make_fight();
    const size_t before = combat.draw_pile.size();
    // "Add 3 Souls into your Draw Pile": three more cards.
    combat.add_souls(3);
    assert(combat.draw_pile.size() == before + 3 && souls_in(combat.draw_pile) == 3);
    combat.add_souls(0);
    combat.add_souls(-2);
    assert(combat.draw_pile.size() == before + 3);
}

void test_other_cards_are_not_souls() {
    CombatEngine combat = make_fight();
    combat.add_power(PowerId::HAUNT, 7);
    combat.add_power(PowerId::DEVOUR_LIFE, 1);
    combat.player.energy = 3;
    play(combat, CardId::DEFEND);
    // Neither "whenever you play a Soul" power heard it.
    assert(combat.enemies[0].hp == 100 && !combat.player.minion_alive());
    // A card that is exhausted is not a Soul in the exhaust pile.
    combat.exhaust_card(CardId::DEFEND);
    assert(combat.exhaust_pile.size() == 1 && combat.exhausted_souls() == 0);
}

// ---- Rules of the Soul card -------------------------------------------------------------------------------

void test_soul_costs_nothing_draws_two_and_is_exhausted() {
    CombatEngine combat = make_fight();
    assert(CardDef::get(SOUL_CARD).type == CardType::SKILL);
    assert(CardDef::get_cost(SOUL_CARD, false) == 0 && CardDef::get_cost(SOUL_CARD, true) == 0);
    assert(CardDef::has_keyword(SOUL_CARD, false, KEYWORD_EXHAUST) && CardDef::has_keyword(SOUL_CARD, true, KEYWORD_EXHAUST));

    combat.player.energy = 0;
    const size_t deck = combat.draw_pile.size();
    play(combat, SOUL_CARD);
    assert(combat.hand.size() == 2 && combat.draw_pile.size() == deck - 2 && combat.player.energy == 0);
    assert(combat.exhaust_pile.size() == 1 && combat.discard_pile.empty());
    assert(combat.counters.skills_played == 1);
}

void test_upgraded_soul_draws_three() {
    CombatEngine combat = make_fight();
    const size_t deck = combat.draw_pile.size();
    play(combat, DeckCard(SOUL_CARD, true));
    assert(combat.hand.size() == 3 && combat.draw_pile.size() == deck - 3);
}

void test_souls_go_to_the_pile_they_are_added_to() {
    CombatEngine combat = make_fight();
    const size_t deck = combat.draw_pile.size();
    const uint8_t created = combat.counters.combat_cards_created;

    // Severance: one in the draw pile, one in the hand, one in the discard pile.
    combat.add_souls(1, Pile::DRAW);
    combat.add_souls(1, Pile::HAND);
    combat.add_souls(1, Pile::DISCARD);
    assert(combat.draw_pile.size() == deck + 1 && souls_in(combat.draw_pile) == 1);
    assert(combat.hand.size() == 1 && souls_in(combat.hand) == 1);
    assert(combat.discard_pile.size() == 1 && souls_in(combat.discard_pile) == 1);
    // They are cards made during the combat, like any other.
    assert(combat.counters.combat_cards_created == created + 3);

    // Reave+, Dirge+: upgraded Souls.
    combat.add_souls(2, Pile::DRAW, true);
    assert(souls_in(combat.draw_pile) == 3 && souls_in(combat.draw_pile, true) == 2);

    // A full hand: the Souls that do not fit go to the discard pile.
    while (combat.hand.size() < combat.hand.capacity()) combat.hand.push_back(CardId::DEFEND);
    combat.add_souls(2, Pile::HAND);
    assert(souls_in(combat.hand) == 1 && souls_in(combat.discard_pile) == 3);
}

void test_souls_in_the_draw_pile_are_drawn_and_played() {
    CombatEngine combat = make_fight();
    combat.draw_pile.clear();
    combat.add_souls(3);
    combat.draw_cards(3);
    assert(souls_in(combat.hand) == 3 && combat.draw_pile.empty());
    combat.player.energy = 0;
    // Nothing left to draw: the Soul is played all the same, and exhausted.
    assert(combat.can_play_card(0) && combat.play_card(0));
    assert(souls_in(combat.hand) == 2 && combat.exhausted_souls() == 1);
}

void test_exhausted_souls_are_counted() {
    CombatEngine combat = make_fight();
    play(combat, SOUL_CARD);
    play(combat, DeckCard(SOUL_CARD, true));
    // Soul Storm counts the Souls of the exhaust pile, upgraded or not, and no other card or pile.
    combat.exhaust_card(CardId::DEFEND);
    combat.add_souls(1, Pile::DISCARD);
    combat.add_souls(1, Pile::DRAW);
    assert(combat.exhausted_souls() == 2);
    // A Soul exhausted without being played is in the exhaust pile too.
    combat.exhaust_card(SOUL_CARD);
    assert(combat.exhausted_souls() == 3);
}

void test_haunt_makes_a_random_enemy_lose_hp() {
    CombatEngine combat = make_fight(3);
    for (Enemy& foe : combat.enemies) foe.block = 20;
    combat.add_power(PowerId::HAUNT, 7);
    play(combat, SOUL_CARD);
    // One enemy lost 7 HP: an HP loss, its Block is untouched.
    assert(total_hp(combat) == 293);
    for (const Enemy& foe : combat.enemies) assert(foe.block == 20 && (foe.hp == 100 || foe.hp == 93));
    // Once per Soul played.
    play(combat, SOUL_CARD);
    assert(total_hp(combat) == 286);
    // Haunt+ is 9; a second Haunt adds up.
    combat.add_power(PowerId::HAUNT, 9);
    play(combat, SOUL_CARD);
    assert(total_hp(combat) == 270);

    // The HP loss can kill, and the combat is won.
    CombatEngine last = make_fight();
    last.enemies[0].hp = 7;
    last.add_power(PowerId::HAUNT, 7);
    play(last, SOUL_CARD);
    assert(!last.enemies[0].is_alive() && last.state == CombatState::VICTORY);
}

void test_devour_life_summons_when_a_soul_is_played() {
    CombatEngine combat = make_fight();
    combat.add_power(PowerId::DEVOUR_LIFE, 1);
    play(combat, SOUL_CARD);
    assert(combat.player.minion_alive() && combat.player.minion_hp == 1 && combat.player.minion_max_hp == 1);
    play(combat, SOUL_CARD);
    assert(combat.player.minion_hp == 2 && combat.player.minion_max_hp == 2);
    // Devour Life+ summons 2.
    CombatEngine upgraded = make_fight();
    upgraded.add_power(PowerId::DEVOUR_LIFE, 2);
    play(upgraded, DeckCard(SOUL_CARD, true));
    assert(upgraded.player.minion_hp == 2);
}

void test_funerary_mask_adds_three_souls_to_the_draw_pile() {
    RunManager run;
    run.start_run(CharacterId::NECROBINDER, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("FUNERARY_MASK");
    const size_t deck = run.master_deck.size();
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    // Shuffled in before the first hand: the three Souls are in the draw pile or already drawn.
    assert(souls_in(run.combat.draw_pile) + souls_in(run.combat.hand) == 3);
    assert(run.combat.draw_pile.size() + run.combat.hand.size() == deck + 3);
    // Each combat, and the deck of the run does not keep them.
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    assert(souls_in(run.combat.draw_pile) + souls_in(run.combat.hand) == 3 && run.master_deck.size() == deck);
    bool drops = false;
    for (std::string_view id : RunManager::active_relics()) drops = drops || id == "FUNERARY_MASK";
    assert(drops);
}

void test_pot_of_ghouls_adds_two_souls_to_the_hand() {
    CombatEngine combat = make_fight();
    assert(combat.use_potion("POT_OF_GHOULS"));
    assert(combat.hand.size() == 2 && souls_in(combat.hand) == 2 && souls_in(combat.hand, true) == 0);
}

// ---- Cards that make Souls or count them (#154) -------------------------------------------------------------

// Plays `card` with `energy` and returns what it cost.
int play_with_energy(CombatEngine& combat, DeckCard card, int target = -1, int energy = 3) {
    combat.player.energy = static_cast<int8_t>(energy);
    play(combat, card, target);
    return energy - combat.player.energy;
}

void test_reave_deals_damage_and_adds_a_soul() {
    CombatEngine combat = make_fight();
    const size_t deck = combat.draw_pile.size();
    assert(play_with_energy(combat, CardId::REAVE, 0) == 1);
    assert(combat.enemies[0].hp == 90);
    assert(combat.draw_pile.size() == deck + 1 && souls_in(combat.draw_pile) == 1 && souls_in(combat.draw_pile, true) == 0);
    assert(combat.discard_pile.size() == 1 && combat.hand.empty());
    // Reave+: 13 damage and a Soul+.
    CombatEngine upgraded = make_fight();
    play_with_energy(upgraded, DeckCard(CardId::REAVE, true), 0);
    assert(upgraded.enemies[0].hp == 87);
    assert(souls_in(upgraded.draw_pile) == 1 && souls_in(upgraded.draw_pile, true) == 1);
}

void test_grave_warden_gains_block_and_adds_a_soul() {
    CombatEngine combat = make_fight();
    assert(play_with_energy(combat, CardId::GRAVE_WARDEN) == 1);
    assert(combat.player.block == 8 && souls_in(combat.draw_pile) == 1);
    // Grave Warden+: 11 Block, and the Soul is not upgraded.
    CombatEngine upgraded = make_fight();
    play_with_energy(upgraded, DeckCard(CardId::GRAVE_WARDEN, true));
    assert(upgraded.player.block == 11);
    assert(souls_in(upgraded.draw_pile) == 1 && souls_in(upgraded.draw_pile, true) == 0);
}

void test_capture_spirit_makes_the_enemy_lose_hp_and_adds_souls() {
    CombatEngine combat = make_fight(2);
    for (Enemy& foe : combat.enemies) foe.block = 20;
    assert(CardDef::get(CardId::CAPTURE_SPIRIT).type == CardType::SKILL);
    assert(play_with_energy(combat, CardId::CAPTURE_SPIRIT, 1) == 1);
    // An HP loss on the chosen enemy: its Block is untouched.
    assert(combat.enemies[1].hp == 97 && combat.enemies[1].block == 20 && combat.enemies[0].hp == 100);
    assert(souls_in(combat.draw_pile) == 3);
    // Capture Spirit+: 4 HP and 4 Souls.
    CombatEngine upgraded = make_fight();
    play_with_energy(upgraded, DeckCard(CardId::CAPTURE_SPIRIT, true), 0);
    assert(upgraded.enemies[0].hp == 96 && souls_in(upgraded.draw_pile) == 4 && souls_in(upgraded.draw_pile, true) == 0);
    // The HP loss can kill.
    CombatEngine last = make_fight();
    last.enemies[0].hp = 3;
    play_with_energy(last, CardId::CAPTURE_SPIRIT, 0);
    assert(!last.enemies[0].is_alive() && last.state == CombatState::VICTORY);
}

void test_severance_adds_a_soul_to_each_pile() {
    CombatEngine combat = make_fight();
    assert(play_with_energy(combat, CardId::SEVERANCE, 0) == 2);
    assert(combat.enemies[0].hp == 87);
    assert(souls_in(combat.draw_pile) == 1 && souls_in(combat.hand) == 1 && souls_in(combat.discard_pile) == 1);
    // Severance itself and the Soul are in the discard pile.
    assert(combat.hand.size() == 1 && combat.discard_pile.size() == 2);
    // Severance+: 18 damage, the Souls are not upgraded.
    CombatEngine upgraded = make_fight();
    play_with_energy(upgraded, DeckCard(CardId::SEVERANCE, true), 0);
    assert(upgraded.enemies[0].hp == 82);
    assert(souls_in(upgraded.draw_pile) == 1 && souls_in(upgraded.hand) == 1 && souls_in(upgraded.discard_pile) == 1);
    assert(souls_in(upgraded.draw_pile, true) + souls_in(upgraded.hand, true) + souls_in(upgraded.discard_pile, true) == 0);
}

void test_dirge_summons_and_adds_souls_x_times() {
    CombatEngine combat = make_fight();
    assert(CardDef::is_x_cost(CardId::DIRGE, false) && CardDef::is_x_cost(CardId::DIRGE, true));
    // X = 3: Summon 3 three times, 3 Souls, all the energy spent, and Dirge is exhausted.
    assert(play_with_energy(combat, CardId::DIRGE) == 3);
    assert(combat.player.minion_hp == 9 && combat.player.minion_max_hp == 9);
    assert(souls_in(combat.draw_pile) == 3 && souls_in(combat.draw_pile, true) == 0);
    assert(combat.exhaust_pile.size() == 1 && combat.discard_pile.empty() && combat.exhausted_souls() == 0);
    // Dirge+ with X = 2: Summon 4 twice, 2 Souls+.
    CombatEngine upgraded = make_fight();
    assert(play_with_energy(upgraded, DeckCard(CardId::DIRGE, true), -1, 2) == 2);
    assert(upgraded.player.minion_hp == 8);
    assert(souls_in(upgraded.draw_pile) == 2 && souls_in(upgraded.draw_pile, true) == 2);
    // X = 0: played for nothing.
    CombatEngine empty = make_fight();
    assert(play_with_energy(empty, CardId::DIRGE, -1, 0) == 0);
    assert(!empty.player.minion_alive() && souls_in(empty.draw_pile) == 0 && empty.exhaust_pile.size() == 1);
}

void test_haunt_card_gives_the_power() {
    CombatEngine combat = make_fight();
    assert(CardDef::get(CardId::HAUNT).type == CardType::POWER);
    assert(play_with_energy(combat, CardId::HAUNT) == 1);
    assert(combat.enemies[0].hp == 100);
    play(combat, SOUL_CARD);
    assert(combat.enemies[0].hp == 93);
    // Haunt+ is 9, on top of the first one.
    play_with_energy(combat, DeckCard(CardId::HAUNT, true));
    play(combat, SOUL_CARD);
    assert(combat.enemies[0].hp == 77);
    // A power card is neither discarded nor exhausted: only the two Souls are in the exhaust pile.
    assert(combat.discard_pile.empty() && combat.exhaust_pile.size() == 2 && combat.exhausted_souls() == 2);
}

void test_devour_life_card_gives_the_power() {
    CombatEngine combat = make_fight();
    assert(CardDef::get(CardId::DEVOUR_LIFE).type == CardType::POWER);
    assert(play_with_energy(combat, CardId::DEVOUR_LIFE) == 1);
    assert(!combat.player.minion_alive());
    play(combat, SOUL_CARD);
    assert(combat.player.minion_hp == 1);
    // Devour Life+ summons 2, on top of the first one.
    play_with_energy(combat, DeckCard(CardId::DEVOUR_LIFE, true));
    play(combat, SOUL_CARD);
    assert(combat.player.minion_hp == 4 && combat.player.minion_max_hp == 4);
}

void test_soul_storm_counts_the_exhausted_souls() {
    CombatEngine combat = make_fight();
    assert(play_with_energy(combat, CardId::SOUL_STORM, 0) == 1);
    assert(combat.enemies[0].hp == 91);
    // Two Souls played, one exhausted unplayed, one still in the draw pile: 9 + 4 x 3.
    combat.draw_pile.clear();
    play(combat, SOUL_CARD);
    play(combat, DeckCard(SOUL_CARD, true));
    combat.exhaust_card(SOUL_CARD);
    combat.exhaust_card(CardId::DEFEND);
    combat.add_souls(1);
    play_with_energy(combat, CardId::SOUL_STORM, 0);
    assert(combat.enemies[0].hp == 91 - 21);
    // Soul Storm+: 9 + 6 x 3.
    play_with_energy(combat, DeckCard(CardId::SOUL_STORM, true), 0);
    assert(combat.enemies[0].hp == 91 - 21 - 27);
    // One attack: Strength is added once, not once per Soul.
    combat.player.add_status(StatusType::STRENGTH, 2);
    play_with_energy(combat, CardId::SOUL_STORM, 0);
    assert(combat.enemies[0].hp == 91 - 21 - 27 - 23);
}

void test_glimpse_beyond_adds_souls_and_is_exhausted() {
    CombatEngine combat = make_fight();
    assert(play_with_energy(combat, CardId::GLIMPSE_BEYOND) == 1);
    assert(souls_in(combat.draw_pile) == 3 && souls_in(combat.draw_pile, true) == 0);
    assert(combat.exhaust_pile.size() == 1 && combat.discard_pile.empty());
    // Glimpse Beyond+: 4 Souls.
    CombatEngine upgraded = make_fight();
    play_with_energy(upgraded, DeckCard(CardId::GLIMPSE_BEYOND, true));
    assert(souls_in(upgraded.draw_pile) == 4 && upgraded.exhaust_pile.size() == 1);
}

void test_souls_cards_are_offered_to_the_necrobinder_only() {
    constexpr CardId SOULS_CARDS[] = { CardId::REAVE, CardId::GRAVE_WARDEN, CardId::CAPTURE_SPIRIT,
                                       CardId::SEVERANCE, CardId::DIRGE, CardId::HAUNT, CardId::DEVOUR_LIFE,
                                       CardId::SOUL_STORM, CardId::GLIMPSE_BEYOND };
    auto offers = [](CharacterId hero, CardId wanted) {
        for (CardId id : RunManager::card_reward_pool(hero)) {
            if (id == wanted) return true;
        }
        return false;
    };
    for (CardId id : SOULS_CARDS) {
        assert(offers(CharacterId::NECROBINDER, id));
        assert(!offers(CharacterId::IRONCLAD, id) && !offers(CharacterId::SILENT, id));
    }
    // The Soul is a token: no reward offers it.
    assert(!offers(CharacterId::NECROBINDER, SOUL_CARD));
}

} // namespace

int main() {
    test_a_combat_starts_without_souls();
    test_adding_souls_makes_cards();
    test_other_cards_are_not_souls();
    test_soul_costs_nothing_draws_two_and_is_exhausted();
    test_upgraded_soul_draws_three();
    test_souls_go_to_the_pile_they_are_added_to();
    test_souls_in_the_draw_pile_are_drawn_and_played();
    test_exhausted_souls_are_counted();
    test_haunt_makes_a_random_enemy_lose_hp();
    test_devour_life_summons_when_a_soul_is_played();
    test_funerary_mask_adds_three_souls_to_the_draw_pile();
    test_pot_of_ghouls_adds_two_souls_to_the_hand();
    std::cout << "[PASS] Souls: the card, the three piles, the exhaust count, Haunt and Devour Life" << std::endl;

    test_reave_deals_damage_and_adds_a_soul();
    test_grave_warden_gains_block_and_adds_a_soul();
    test_capture_spirit_makes_the_enemy_lose_hp_and_adds_souls();
    test_severance_adds_a_soul_to_each_pile();
    test_dirge_summons_and_adds_souls_x_times();
    test_haunt_card_gives_the_power();
    test_devour_life_card_gives_the_power();
    test_soul_storm_counts_the_exhausted_souls();
    test_glimpse_beyond_adds_souls_and_is_exhausted();
    test_souls_cards_are_offered_to_the_necrobinder_only();
    std::cout << "[PASS] Souls cards: Reave, Grave Warden, Capture Spirit, Severance, Dirge, Haunt, Devour Life, "
                 "Soul Storm, Glimpse Beyond" << std::endl;
    return 0;
}
