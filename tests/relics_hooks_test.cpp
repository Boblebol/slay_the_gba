// Relics that act on the combat events: one case per relic, on hand-built situations.
#include <algorithm>
#include <cassert>
#include <iostream>
#include "engine/combat.h"
#include "engine/run.h"
#include "engine/ancient.h"
#include "engine/card_meta.h"

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

// Ends the turn with the first enemy attacking for `damage`.
void end_turn_attacked(CombatEngine& combat, int16_t damage) {
    for (Enemy& foe : combat.enemies) make_enemy_idle(foe);
    combat.enemies[0].current_intent.type = EnemyIntentType::ATTACK;
    combat.enemies[0].current_intent.base_damage = damage;
    combat.end_player_turn();
}

void pad_draw_pile(CombatEngine& combat, int count) {
    for (int i = 0; i < count; ++i) combat.draw_pile.push_back(CardId::DEFEND);
}

// A fight opened the way RunManager does it: state reset, relic registered as a power, then the first turn.
struct RelicPower {
    PowerId id;
    int16_t amount = 1;
};

CombatEngine make_relic_combat(CharacterId hero, std::initializer_list<RelicPower> relics, bool pack = false) {
    CombatEngine combat = pack ? make_pack(hero) : make_combat(hero);
    while (!combat.hand.empty()) {
        combat.draw_pile.push_back(combat.hand.back());
        combat.hand.pop_back();
    }
    pad_draw_pile(combat, 30);
    combat.reset_combat_state();
    for (const RelicPower& relic : relics) combat.add_power(relic.id, relic.amount);
    combat.begin_first_turn();
    return combat;
}

CombatEngine make_relic_combat(CharacterId hero, PowerId relic, int16_t amount = 1, bool pack = false) {
    return make_relic_combat(hero, { RelicPower{ relic, amount } }, pack);
}

// Enters the first map room after forcing its type, like a player walking into it.
void enter_room_of_type(RunManager& run, RoomType type) {
    int index = type == RoomType::BOSS ? run.map.boss_node_index : 0;
    run.map.nodes[index].type = type;
    run.enter_room(index);
}

// A run of `hero` holding `relics`, standing in its first fight.
RunManager make_relic_run(CharacterId hero, std::initializer_list<std::string_view> relics) {
    RunManager run;
    run.start_run(hero, 0, PaletteVariant::CLASSIC, 777);
    for (std::string_view relic : relics) run.add_relic(relic);
    enter_room_of_type(run, RoomType::MONSTER);
    return run;
}

bool in_pool(CharacterId hero, CardId id) {
    std::span<const CardId> pool = RunManager::card_reward_pool(hero);
    return std::find(pool.begin(), pool.end(), id) != pool.end();
}

void test_attack_relics() {
    // Three Attacks in a turn. Like every power, the relic acts before the card it hears: the third Strike
    // already hits with Shuriken's Strength.
    CombatEngine shuriken = make_relic_combat(CharacterId::IRONCLAD, PowerId::SHURIKEN, 1);
    shuriken.enemies[0].hp = 100;
    play(shuriken, CardId::STRIKE);
    play(shuriken, CardId::STRIKE);
    assert(shuriken.player.get_status(StatusType::STRENGTH) == 0 && shuriken.enemies[0].hp == 100 - 12);
    play(shuriken, CardId::STRIKE);
    assert(shuriken.player.get_status(StatusType::STRENGTH) == 1 && shuriken.enemies[0].hp == 100 - 12 - 7);
    for (int i = 0; i < 3; ++i) play(shuriken, CardId::STRIKE);
    assert(shuriken.player.get_status(StatusType::STRENGTH) == 2);
    // The tally restarts with the turn, and Skills are not counted.
    end_turn_idle(shuriken);
    play(shuriken, CardId::STRIKE);
    play(shuriken, CardId::DEFEND);
    play(shuriken, CardId::STRIKE);
    assert(shuriken.player.get_status(StatusType::STRENGTH) == 2);
    play(shuriken, CardId::STRIKE);
    assert(shuriken.player.get_status(StatusType::STRENGTH) == 3);

    CombatEngine kunai = make_relic_combat(CharacterId::IRONCLAD, PowerId::KUNAI, 1);
    for (int i = 0; i < 2; ++i) play(kunai, CardId::STRIKE);
    assert(kunai.player.get_status(StatusType::DEXTERITY) == 0);
    play(kunai, CardId::STRIKE);
    assert(kunai.player.get_status(StatusType::DEXTERITY) == 1);
    play(kunai, CardId::DEFEND);
    assert(kunai.player.block == 6);

    CombatEngine fan = make_relic_combat(CharacterId::IRONCLAD, PowerId::ORNAMENTAL_FAN, 4);
    fan.enemies[0].hp = 100;
    for (int i = 0; i < 2; ++i) play(fan, CardId::STRIKE);
    assert(fan.player.block == 0);
    play(fan, CardId::STRIKE);
    assert(fan.player.block == 4);
    for (int i = 0; i < 3; ++i) play(fan, CardId::STRIKE);
    assert(fan.player.block == 8);

    CombatEngine kusarigama = make_relic_combat(CharacterId::IRONCLAD, PowerId::KUSARIGAMA, 6);
    for (int i = 0; i < 2; ++i) play(kusarigama, CardId::STRIKE);
    assert(kusarigama.enemies[0].hp == 42 - 12);
    play(kusarigama, CardId::STRIKE);
    assert(kusarigama.enemies[0].hp == 42 - 18 - 6);

    // Nunchaku counts the Attacks of the whole combat.
    CombatEngine nunchaku = make_relic_combat(CharacterId::IRONCLAD, PowerId::NUNCHAKU, 1);
    nunchaku.enemies[0].hp = 100;
    for (int i = 0; i < 6; ++i) play(nunchaku, CardId::STRIKE);
    end_turn_idle(nunchaku);
    for (int i = 0; i < 3; ++i) play(nunchaku, CardId::STRIKE);
    assert(nunchaku.player.energy == 3 - 1 && nunchaku.power_counter(PowerId::NUNCHAKU) == 9);
    play(nunchaku, CardId::DEFEND);
    assert(nunchaku.player.energy == 3 - 1);
    play(nunchaku, CardId::STRIKE);
    assert(nunchaku.player.energy == 3 - 1 + 1 && nunchaku.power_counter(PowerId::NUNCHAKU) == 0);
    std::cout << "[PASS] Attack relics: Shuriken, Kunai, Ornamental Fan, Kusarigama, Nunchaku" << std::endl;
}

void test_skill_and_power_relics() {
    // Letter Opener: every third Skill of the turn hits every enemy.
    CombatEngine opener = make_relic_combat(CharacterId::IRONCLAD, PowerId::LETTER_OPENER, 5, true);
    for (int i = 0; i < 2; ++i) play(opener, CardId::DEFEND);
    play(opener, CardId::STRIKE, 2);
    assert(opener.enemies[0].hp == 18 && opener.enemies[1].hp == 18 && opener.enemies[2].hp == 42 - 6);
    play(opener, CardId::DEFEND);
    assert(opener.enemies[0].hp == 13 && opener.enemies[1].hp == 13 && opener.enemies[2].hp == 36 - 5);
    for (int i = 0; i < 3; ++i) play(opener, CardId::DEFEND);
    assert(opener.enemies[0].hp == 8 && opener.enemies[2].hp == 26);
    end_turn_idle(opener);
    for (int i = 0; i < 2; ++i) play(opener, CardId::DEFEND);
    assert(opener.enemies[0].hp == 8 && opener.enemies[2].hp == 26);

    // Tuning Fork counts the Skills of the whole combat.
    CombatEngine fork = make_relic_combat(CharacterId::IRONCLAD, PowerId::TUNING_FORK, 7);
    for (int i = 0; i < 6; ++i) play(fork, CardId::DEFEND);
    end_turn_idle(fork);
    for (int i = 0; i < 3; ++i) play(fork, CardId::DEFEND);
    play(fork, CardId::STRIKE);
    assert(fork.player.block == 15 && fork.power_counter(PowerId::TUNING_FORK) == 9);
    play(fork, CardId::DEFEND);
    assert(fork.player.block == 20 + 7 && fork.power_counter(PowerId::TUNING_FORK) == 0);

    // Mummified Hand: a card that still costs something becomes free until the end of the turn.
    CombatEngine mummy = make_relic_combat(CharacterId::IRONCLAD, PowerId::MUMMIFIED_HAND);
    mummy.hand.clear();
    mummy.hand.push_back(CardId::INFLAME);
    mummy.hand.push_back(CardId::SHIV);     // costs 0 already
    mummy.hand.push_back(CardId::BURN);     // cannot be played
    mummy.hand.push_back(CardId::BLUDGEON);
    mummy.player.energy = 3;
    assert(mummy.play_card(0));
    assert(!mummy.hand[0].free_this_turn && !mummy.hand[1].free_this_turn && mummy.hand[2].free_this_turn);
    assert(mummy.effective_cost(mummy.hand[2]) == 0 && mummy.player.energy == 2);
    assert(mummy.play_card(2, 0));
    assert(mummy.player.energy == 2 && mummy.enemies[0].hp == 42 - 34);
    mummy.hand.clear();
    mummy.hand.push_back(CardId::INFLAME);
    mummy.hand.push_back(CardId::DEFEND);
    assert(mummy.play_card(0));
    assert(mummy.hand[0].free_this_turn);
    end_turn_idle(mummy);
    for (const DeckCard& card : mummy.discard_pile) assert(!card.free_this_turn);
    // A Skill gives nothing.
    mummy.hand.clear();
    mummy.hand.push_back(CardId::DEFEND);
    mummy.hand.push_back(CardId::BLUDGEON);
    assert(mummy.play_card(0));
    assert(!mummy.hand[0].free_this_turn);

    // Rainbow Ring: an Attack, a Skill and a Power in the same turn, once a turn.
    CombatEngine ring = make_relic_combat(CharacterId::IRONCLAD, PowerId::RAINBOW_RING, 1);
    ring.enemies[0].hp = 100;
    play(ring, CardId::STRIKE);
    play(ring, CardId::DEFEND);
    assert(ring.player.get_status(StatusType::DEXTERITY) == 0);
    play(ring, CardId::FOOTWORK);
    assert(ring.player.get_status(StatusType::STRENGTH) == 1 && ring.player.get_status(StatusType::DEXTERITY) == 1 + 2);
    play(ring, CardId::FOOTWORK);
    play(ring, CardId::STRIKE);
    assert(ring.player.get_status(StatusType::STRENGTH) == 1 && ring.player.get_status(StatusType::DEXTERITY) == 1 + 4);
    end_turn_idle(ring);
    play(ring, CardId::FOOTWORK);
    play(ring, CardId::STRIKE);
    assert(ring.player.get_status(StatusType::STRENGTH) == 1);
    play(ring, CardId::DEFEND);
    assert(ring.player.get_status(StatusType::STRENGTH) == 2 && ring.player.get_status(StatusType::DEXTERITY) == 2 + 6);

    // Ivory Tile: energy back for a card that costs 3 or more.
    CombatEngine tile = make_relic_combat(CharacterId::NECROBINDER, PowerId::IVORY_TILE, 1);
    tile.enemies[0].hp = 100;
    play(tile, CardId::BASH);
    assert(tile.player.energy == 3 - 2);
    play(tile, CardId::BLUDGEON);
    assert(tile.player.energy == 3 - 3 + 1);
    std::cout << "[PASS] Skill and Power relics: Letter Opener, Tuning Fork, Mummified Hand, Rainbow Ring, Ivory Tile" << std::endl;
}

void test_exhaust_and_creation_relics() {
    // Joss Paper: a card for every 5 cards exhausted.
    CombatEngine joss = make_relic_combat(CharacterId::IRONCLAD, PowerId::JOSS_PAPER, 1);
    joss.hand.clear();
    for (int i = 0; i < 4; ++i) joss.exhaust_card(CardId::STRIKE);
    assert(joss.hand.empty() && joss.power_counter(PowerId::JOSS_PAPER) == 4);
    joss.exhaust_card(CardId::STRIKE);
    assert(joss.hand.size() == 1 && joss.power_counter(PowerId::JOSS_PAPER) == 0);
    for (int i = 0; i < 5; ++i) joss.exhaust_card(CardId::STRIKE);
    assert(joss.hand.size() == 2);

    // Charon's Ashes: every enemy is hit, whatever exhausted the card.
    CombatEngine ashes = make_relic_combat(CharacterId::IRONCLAD, PowerId::CHARONS_ASHES, 3, true);
    ashes.exhaust_card(CardId::STRIKE);
    assert(ashes.enemies[0].hp == 15 && ashes.enemies[1].hp == 15 && ashes.enemies[2].hp == 39);
    play(ashes, CardId::IMPERVIOUS);
    assert(ashes.enemies[0].hp == 12 && ashes.enemies[1].hp == 12 && ashes.enemies[2].hp == 36);

    // Burning Sticks: the first Skill exhausted, and only that one, is copied.
    CombatEngine sticks = make_relic_combat(CharacterId::IRONCLAD, PowerId::BURNING_STICKS);
    sticks.hand.clear();
    sticks.exhaust_card(CardId::STRIKE);
    assert(sticks.hand.empty() && sticks.power_counter(PowerId::BURNING_STICKS) == 0);
    DeckCard impervious(CardId::IMPERVIOUS, true);
    impervious.deck_index = 3;
    play(sticks, impervious);
    assert(sticks.hand.size() == 1 && sticks.hand[0] == CardId::IMPERVIOUS && sticks.hand[0].upgraded);
    assert(sticks.hand[0].deck_index == NOT_IN_DECK && sticks.exhaust_pile.back().deck_index == 3);
    sticks.player.energy = 3;
    assert(sticks.play_card(0));
    assert(sticks.hand.empty() && sticks.player.block == 80);

    // Regalite: the first card created each turn.
    CombatEngine regalite = make_relic_combat(CharacterId::REGENT, PowerId::REGALITE, 4);
    assert(regalite.player.block == 0);
    regalite.create_card(CardId::SHIV, Pile::HAND);
    assert(regalite.player.block == 4);
    regalite.create_card(CardId::DAZED, Pile::DRAW);
    assert(regalite.player.block == 4);
    end_turn_idle(regalite);
    assert(regalite.player.block == 0);
    regalite.create_card(CardId::DAZED, Pile::DISCARD);
    assert(regalite.player.block == 4);
    std::cout << "[PASS] Exhaust and creation relics: Joss Paper, Charon's Ashes, Burning Sticks, Regalite" << std::endl;
}

void test_turn_relics() {
    // Ripple Basin: Block at the end of a turn without an Attack (Chomp: 11).
    CombatEngine basin = make_relic_combat(CharacterId::IRONCLAD, PowerId::RIPPLE_BASIN, 4);
    play(basin, CardId::INFLAME);
    basin.end_player_turn();
    assert(basin.player.hp == 80 - (11 - 4));
    play(basin, CardId::STRIKE);
    end_turn_attacked(basin, 11);
    assert(basin.player.hp == 73 - 11);

    // Brimstone: Strength for both sides, turn after turn.
    CombatEngine brimstone = make_relic_combat(CharacterId::IRONCLAD, PowerId::BRIMSTONE, 2, true);
    assert(brimstone.player.get_status(StatusType::STRENGTH) == 2);
    for (const Enemy& foe : brimstone.enemies) assert(foe.get_status(StatusType::STRENGTH) == 1);
    brimstone.enemies[0].hp = 0;
    end_turn_idle(brimstone);
    assert(brimstone.player.get_status(StatusType::STRENGTH) == 4);
    assert(brimstone.enemies[0].get_status(StatusType::STRENGTH) == 1); // dead
    assert(brimstone.enemies[1].get_status(StatusType::STRENGTH) == 2 && brimstone.enemies[2].get_status(StatusType::STRENGTH) == 2);

    // Lunar Pastry: a Star at the end of each turn.
    CombatEngine pastry = make_relic_combat(CharacterId::REGENT, PowerId::LUNAR_PASTRY, 1);
    assert(pastry.player.sovereign_stars == 0);
    end_turn_idle(pastry);
    assert(pastry.player.sovereign_stars == 1);
    end_turn_idle(pastry);
    assert(pastry.player.sovereign_stars == 2);
    std::cout << "[PASS] Turn relics: Ripple Basin, Brimstone, Lunar Pastry" << std::endl;
}

void test_paels_tears() {
    // Unspent energy at the end of the turn: 2 more energy next turn.
    CombatEngine tears = make_relic_combat(CharacterId::IRONCLAD, PowerId::PAELS_TEARS, 2);
    end_turn_idle(tears);
    assert(tears.player.energy == 5);
    // Everything spent: nothing.
    tears.player.energy = 0;
    end_turn_idle(tears);
    assert(tears.player.energy == 3);

    // Without the relic: no extra energy.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, {});
    end_turn_idle(plain);
    assert(plain.player.energy == 3);
    std::cout << "[PASS] Pael's Tears: 2 energy next turn after a turn with unspent energy" << std::endl;
}

void test_hp_relics() {
    // Self-Forming Clay: Block at the start of the turn that follows each HP loss.
    CombatEngine clay = make_relic_combat(CharacterId::IRONCLAD, PowerId::SELF_FORMING_CLAY, 3);
    clay.lose_hp(2);
    clay.lose_hp(1);
    assert(clay.player.block == 0);
    end_turn_idle(clay);
    assert(clay.player.block == 6);
    end_turn_idle(clay);
    assert(clay.player.block == 0);
    end_turn_attacked(clay, 11);
    assert(clay.player.hp == 80 - 3 - 11 && clay.player.block == 3);
    // An attack that Block absorbs takes no HP.
    play(clay, CardId::IMPERVIOUS);
    end_turn_attacked(clay, 11);
    assert(clay.player.hp == 66 && clay.player.block == 0);

    // Red Skull: Strength as long as the HP are at half or less. A heal shows when the next card is played.
    CombatEngine skull = make_relic_combat(CharacterId::IRONCLAD, PowerId::RED_SKULL, 3);
    skull.lose_hp(39);
    assert(skull.player.hp == 41 && skull.player.get_status(StatusType::STRENGTH) == 0);
    skull.lose_hp(1);
    assert(skull.player.get_status(StatusType::STRENGTH) == 3);
    skull.lose_hp(5);
    assert(skull.player.get_status(StatusType::STRENGTH) == 3);
    play(skull, CardId::STRIKE);
    assert(skull.enemies[0].hp == 42 - 9);
    skull.player.heal(20);
    play(skull, CardId::STRIKE);
    assert(skull.player.get_status(StatusType::STRENGTH) == 0 && skull.enemies[0].hp == 33 - 6);
    // The Strength of the other sources is left alone.
    play(skull, CardId::INFLAME);
    skull.lose_hp(30);
    assert(skull.player.get_status(StatusType::STRENGTH) == 2 + 3);

    // Demon Tongue: the first HP loss of each of the player's turns is healed.
    CombatEngine tongue = make_relic_combat(CharacterId::IRONCLAD, PowerId::DEMON_TONGUE);
    tongue.lose_hp(5);
    assert(tongue.player.hp == 80);
    tongue.lose_hp(5);
    assert(tongue.player.hp == 75);
    tongue.end_player_turn(); // Chomp, during the enemy's turn: 11
    assert(tongue.player.hp == 75 - 11);
    play(tongue, CardId::BLOODLETTING);
    assert(tongue.player.hp == 64);
    play(tongue, CardId::BLOODLETTING);
    assert(tongue.player.hp == 64 - 3);
    // Poison ticks before the turn is announced: it is that turn's first loss.
    tongue.player.add_status(StatusType::POISON, 4);
    end_turn_idle(tongue);
    assert(tongue.player.hp == 61);
    tongue.lose_hp(2);
    assert(tongue.player.hp == 59);
    std::cout << "[PASS] HP relics: Self-Forming Clay, Red Skull, Demon Tongue" << std::endl;
}

void test_poison_and_star_relics() {
    // Snecko Skull: 1 more Poison each time Poison is applied, to each enemy it is applied to.
    CombatEngine snecko = make_relic_combat(CharacterId::SILENT, PowerId::SNECKO_SKULL, 1, true);
    play(snecko, CardId::DEADLY_POISON, 2);
    assert(snecko.enemies[2].get_status(StatusType::POISON) == 5 + 1 && snecko.enemies[0].get_status(StatusType::POISON) == 0);
    play(snecko, CardId::POISON_STAB, 0);
    assert(snecko.enemies[0].get_status(StatusType::POISON) == 3 + 1);
    play(snecko, CardId::BASH, 1); // Vulnerable is not Poison
    assert(snecko.enemies[1].get_status(StatusType::POISON) == 0);

    // Mini Regent: the first Stars spent each turn.
    CombatEngine mini = make_relic_combat(CharacterId::REGENT, PowerId::MINI_REGENT, 1);
    mini.gain_stars(5);
    play(mini, CardId::DEFEND);
    assert(mini.player.get_status(StatusType::STRENGTH) == 0);
    play(mini, CardId::CLOAK_OF_STARS);
    assert(mini.player.get_status(StatusType::STRENGTH) == 1 && mini.player.sovereign_stars == 4);
    play(mini, CardId::CLOAK_OF_STARS);
    assert(mini.player.get_status(StatusType::STRENGTH) == 1);
    end_turn_idle(mini);
    play(mini, CardId::CLOAK_OF_STARS);
    assert(mini.player.get_status(StatusType::STRENGTH) == 2);

    // Galactic Dust: every 10 Stars spent, across cards and turns.
    CombatEngine dust = make_relic_combat(CharacterId::REGENT, PowerId::GALACTIC_DUST, 10);
    dust.enemies[0].hp = 100;
    dust.gain_stars(40);
    play(dust, CardId::COMET); // 5 Stars
    play(dust, CardId::GAMMA_BLAST); // 3 Stars
    assert(dust.player.block == 0 && dust.power_counter(PowerId::GALACTIC_DUST) == 8);
    play(dust, CardId::GAMMA_BLAST);
    assert(dust.player.block == 10 && dust.power_counter(PowerId::GALACTIC_DUST) == 1);
    end_turn_idle(dust);
    dust.spend_stars(29);
    assert(dust.player.block == 30 && dust.power_counter(PowerId::GALACTIC_DUST) == 0);
    std::cout << "[PASS] Poison and Star relics: Snecko Skull, Mini Regent, Galactic Dust" << std::endl;
}

void test_paels_blood() {
    // Paël's Blood: 1 more card on the first turn and on each following one.
    RunManager blood = make_relic_run(CharacterId::IRONCLAD, { "PAELS_BLOOD" });
    assert(blood.combat.hand.size() == 6);
    end_turn_idle(blood.combat);
    assert(blood.combat.hand.size() == 6);

    // Without the relic: the usual 5 cards.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(plain.combat.hand.size() == 5);
    end_turn_idle(plain.combat);
    assert(plain.combat.hand.size() == 5);
    std::cout << "[PASS] Paël's Blood" << std::endl;
}

void test_combat_start_relics() {
    // Bag of Preparation: 2 more cards in the first hand, not in the next ones.
    RunManager bag = make_relic_run(CharacterId::IRONCLAD, { "BAG_OF_PREPARATION" });
    assert(bag.combat.hand.size() == 7);
    end_turn_idle(bag.combat);
    assert(bag.combat.hand.size() == 5);

    // Vexing Puzzlebox: one more card, from the hero's pool, free for the turn.
    RunManager puzzlebox = make_relic_run(CharacterId::IRONCLAD, { "VEXING_PUZZLEBOX" });
    assert(puzzlebox.combat.hand.size() == 6);
    const DeckCard gift = puzzlebox.combat.hand.back();
    assert(gift.free_this_turn && gift.deck_index == NOT_IN_DECK && in_pool(CharacterId::IRONCLAD, gift.id));
    for (size_t i = 0; i + 1 < puzzlebox.combat.hand.size(); ++i) assert(!puzzlebox.combat.hand[i].free_this_turn);
    end_turn_idle(puzzlebox.combat);
    for (const DeckCard& card : puzzlebox.combat.hand) assert(!card.free_this_turn);
    for (const DeckCard& card : puzzlebox.combat.discard_pile) assert(!card.free_this_turn);

    // Big Hat: 2 Ethereal cards of the hero's pool.
    RunManager hat = make_relic_run(CharacterId::NECROBINDER, { "BIG_HAT" });
    assert(hat.combat.hand.size() == 7);
    for (size_t i = 5; i < 7; ++i) {
        const DeckCard& card = hat.combat.hand[i];
        assert(CardDef::has_keyword(card, KEYWORD_ETHEREAL) && in_pool(CharacterId::NECROBINDER, card.id));
        assert(card.deck_index == NOT_IN_DECK);
    }

    // Twisted Funnel: 4 Poison to every enemy, 5 with Snecko Skull.
    RunManager funnel = make_relic_run(CharacterId::SILENT, { "TWISTED_FUNNEL" });
    for (const Enemy& foe : funnel.combat.enemies) assert(foe.get_status(StatusType::POISON) == 4);
    RunManager skull = make_relic_run(CharacterId::SILENT, { "TWISTED_FUNNEL", "SNECKO_SKULL" });
    for (const Enemy& foe : skull.combat.enemies) assert(foe.get_status(StatusType::POISON) == 5);
    assert(skull.combat.enemy.get_status(StatusType::POISON) == 5);
    std::cout << "[PASS] Combat start relics: Bag of Preparation, Vexing Puzzlebox, Big Hat, Twisted Funnel" << std::endl;
}

void test_relics_in_a_run() {
    // Meat on the Bone: 12 HP at the end of a fight left with half the HP or less.
    RunManager meat = make_relic_run(CharacterId::SILENT, { "MEAT_ON_THE_BONE" });
    meat.combat.player.hp = meat.max_hp / 2;
    meat.proceed_after_combat();
    assert(meat.hp == meat.max_hp / 2 + 12);
    RunManager full = make_relic_run(CharacterId::SILENT, { "MEAT_ON_THE_BONE" });
    full.combat.player.hp = full.max_hp / 2 + 1;
    full.proceed_after_combat();
    assert(full.hp == full.max_hp / 2 + 1);
    // Burning Blood heals first: its 6 HP can bring the Ironclad above half.
    RunManager blood = make_relic_run(CharacterId::IRONCLAD, { "MEAT_ON_THE_BONE" });
    blood.combat.player.hp = 40;
    blood.proceed_after_combat();
    assert(blood.hp == 46);
    RunManager both = make_relic_run(CharacterId::IRONCLAD, { "MEAT_ON_THE_BONE" });
    both.combat.player.hp = 30;
    both.proceed_after_combat();
    assert(both.hp == 30 + 6 + 12);

    // Intimidating Helmet is registered as Danse Macabre's power: Block for a card that costs 2 or more.
    RunManager helmet = make_relic_run(CharacterId::IRONCLAD, { "INTIMIDATING_HELMET" });
    assert(helmet.combat.power_amount(PowerId::DANSE_MACABRE) == 4);
    play(helmet.combat, CardId::STRIKE);
    assert(helmet.combat.player.block == 0);
    play(helmet.combat, CardId::BASH);
    assert(helmet.combat.player.block == 4);

    // Red Skull already counts when the fight starts with half the HP.
    RunManager wounded;
    wounded.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    wounded.add_relic("RED_SKULL");
    wounded.hp = wounded.max_hp / 2;
    enter_room_of_type(wounded, RoomType::MONSTER);
    assert(wounded.combat.player.get_status(StatusType::STRENGTH) == 3);

    // RunManager registers every relic with the amount of its text.
    RunManager shared = make_relic_run(CharacterId::IRONCLAD, { "JOSS_PAPER", "LETTER_OPENER", "TUNING_FORK", "KUNAI",
                                                                "KUSARIGAMA", "MUMMIFIED_HAND", "NUNCHAKU",
                                                                "RAINBOW_RING", "RIPPLE_BASIN", "BURNING_STICKS",
                                                                "SHURIKEN", "ORNAMENTAL_FAN" });
    const CombatEngine& fight = shared.combat;
    assert(fight.power_amount(PowerId::JOSS_PAPER) == 1 && fight.power_amount(PowerId::LETTER_OPENER) == 5);
    assert(fight.power_amount(PowerId::TUNING_FORK) == 7 && fight.power_amount(PowerId::KUNAI) == 1);
    assert(fight.power_amount(PowerId::KUSARIGAMA) == 6 && fight.has_power(PowerId::MUMMIFIED_HAND));
    assert(fight.power_amount(PowerId::NUNCHAKU) == 1 && fight.power_amount(PowerId::RAINBOW_RING) == 1);
    assert(fight.power_amount(PowerId::RIPPLE_BASIN) == 4 && fight.has_power(PowerId::BURNING_STICKS));
    assert(fight.power_amount(PowerId::SHURIKEN) == 1 && fight.power_amount(PowerId::ORNAMENTAL_FAN) == 4);
    assert(fight.powers.size() == 12 && fight.powers_in_play.empty());

    RunManager heroes = make_relic_run(CharacterId::IRONCLAD, { "SELF_FORMING_CLAY", "CHARONS_ASHES", "RED_SKULL",
                                                                "DEMON_TONGUE", "BRIMSTONE", "SNECKO_SKULL",
                                                                "MINI_REGENT", "GALACTIC_DUST", "LUNAR_PASTRY",
                                                                "REGALITE", "IVORY_TILE" });
    const CombatEngine& brawl = heroes.combat;
    assert(brawl.power_amount(PowerId::SELF_FORMING_CLAY) == 3 && brawl.power_amount(PowerId::CHARONS_ASHES) == 3);
    assert(brawl.power_amount(PowerId::RED_SKULL) == 3 && brawl.has_power(PowerId::DEMON_TONGUE));
    assert(brawl.power_amount(PowerId::BRIMSTONE) == 2 && brawl.power_amount(PowerId::SNECKO_SKULL) == 1);
    assert(brawl.power_amount(PowerId::MINI_REGENT) == 1 && brawl.power_amount(PowerId::GALACTIC_DUST) == 10);
    assert(brawl.power_amount(PowerId::LUNAR_PASTRY) == 1 && brawl.power_amount(PowerId::REGALITE) == 4);
    assert(brawl.power_amount(PowerId::IVORY_TILE) == 1 && brawl.powers.size() == 11);
    // Brimstone acted on the first turn.
    assert(brawl.player.get_status(StatusType::STRENGTH) == 2);
    for (const Enemy& foe : brawl.enemies) assert(foe.get_status(StatusType::STRENGTH) == 1);

    // A run without them registers nothing.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(plain.combat.powers.empty() && plain.combat.hand.size() == 5);
    std::cout << "[PASS] Relics in a run: Meat on the Bone, Intimidating Helmet, every relic registered with its amount" << std::endl;
}

} // namespace

void test_very_hot_cocoa() {
    // The first turn starts with 4 more energy; the next ones are back to normal.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    RunManager cocoa = make_relic_run(CharacterId::IRONCLAD, { "VERY_HOT_COCOA" });
    assert(cocoa.combat.player.energy == plain.combat.player.energy + 4);
    end_turn_idle(cocoa.combat);
    end_turn_idle(plain.combat);
    assert(cocoa.combat.player.energy == plain.combat.player.energy);
}

void test_strawberry() {
    // Obtained: 7 more maximum HP, and the current HP follow.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.hp = 50;
    int16_t max_hp = run.max_hp;
    run.add_relic("STRAWBERRY");
    assert(run.has_relic("STRAWBERRY"));
    assert(run.max_hp == max_hp + 7 && run.hp == 57);

    // Any other relic leaves the HP alone.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.hp = 50;
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("STRAWBERRY"));
    assert(plain.max_hp == max_hp && plain.hp == 50);

    // The effect is paid once, when the relic comes in: not at each fight, and not when a full bag refuses it.
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.max_hp == max_hp + 7 && run.combat.player.max_hp == max_hp + 7);
    RunManager full;
    full.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    while (full.relics.size() < full.relics.capacity()) full.relics.push_back("CIRCLET");
    full.add_relic("STRAWBERRY");
    assert(!full.has_relic("STRAWBERRY") && full.max_hp == max_hp);

    // A chest can hold it, and gives its HP on the spot.
    bool found = false;
    for (uint32_t seed = 1; seed < 400 && !found; ++seed) {
        RunManager chest;
        chest.start_run(CharacterId::SILENT, 0, PaletteVariant::CLASSIC, seed);
        int16_t before = chest.max_hp;
        enter_room_of_type(chest, RoomType::TREASURE);
        if (chest.last_treasure_relic == "STRAWBERRY") {
            found = true;
            assert(chest.max_hp == before + 7);
        } else if (chest.last_treasure_relic != "MANGO" && chest.last_treasure_relic != "PEAR" &&
                   chest.last_treasure_relic != "LEES_WAFFLE") {
            // The other relics that raise the maximum HP on pickup aside.
            assert(chest.max_hp == before);
        }
    }
    assert(found);
    std::cout << "[PASS] Strawberry: +7 Max HP when obtained, once, from a chest too" << std::endl;
}

void test_seal_of_gold() {
    // With the relic: the turn start costs 3 gold and gives 1 energy.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("SEAL_OF_GOLD");
    run.gold = 10;
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.combat.player.energy == 4 && run.combat.gold == 7);

    // Without the relic: no extra energy, no gold spent.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.gold = 10;
    enter_room_of_type(plain, RoomType::MONSTER);
    assert(plain.combat.player.energy == 3 && plain.gold == 10);

    // Too poor to pay: nothing happens.
    RunManager poor;
    poor.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    poor.add_relic("SEAL_OF_GOLD");
    poor.gold = 2;
    enter_room_of_type(poor, RoomType::MONSTER);
    assert(poor.combat.player.energy == 3 && poor.combat.gold == 2);
    std::cout << "[PASS] Seal of Gold: 3 gold for 1 energy at each turn start" << std::endl;
}

void test_pumpkin_candle() {
    // Lit: one more energy each turn, for 5 combats.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("PUMPKIN_CANDLE");
    for (int fight = 1; fight <= 5; ++fight) {
        enter_room_of_type(run, RoomType::MONSTER);
        assert(run.combat.player.energy == 4);
        assert(run.relic_counter("PUMPKIN_CANDLE") == fight);
    }
    // Extinguished after the fifth combat.
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.combat.player.energy == 3);

    // Without the relic: the usual 3.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(plain, RoomType::MONSTER);
    assert(plain.combat.player.energy == 3);
    std::cout << "[PASS] Pumpkin Candle: +1 energy per turn for 5 combats" << std::endl;
}

void test_wongos_mystery_ticket() {
    // With the relic: each fight won counts; the fifth pays 3 random relics, once.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("WONGOS_MYSTERY_TICKET");
    const size_t before = run.relics.size(); // the starter relic is already there
    for (int fight = 1; fight <= 4; ++fight) {
        enter_room_of_type(run, RoomType::MONSTER);
        run.proceed_after_combat();
        assert(run.relic_counter("WONGOS_MYSTERY_TICKET") == fight);
        assert(run.relics.size() == before);
    }
    enter_room_of_type(run, RoomType::MONSTER);
    run.proceed_after_combat();
    assert(run.relics.size() == before + 3);
    assert(run.relic_counter("WONGOS_MYSTERY_TICKET") == 5);
    // Paid out once: later fights add nothing.
    enter_room_of_type(run, RoomType::MONSTER);
    run.proceed_after_combat();
    assert(run.relics.size() == before + 3);

    // Without the relic: five fights give no relic.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    const size_t plain_before = plain.relics.size();
    for (int fight = 1; fight <= 5; ++fight) {
        enter_room_of_type(plain, RoomType::MONSTER);
        plain.proceed_after_combat();
    }
    assert(!plain.has_relic("WONGOS_MYSTERY_TICKET"));
    assert(plain.relics.size() == plain_before);
    std::cout << "[PASS] Wongo's Mystery Ticket: 3 random relics after 5 combats" << std::endl;
}

void test_ember_tea() {
    // Brewed: 2 Strength at the start of each of the next 5 combats.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("EMBER_TEA");
    for (int fight = 1; fight <= 5; ++fight) {
        enter_room_of_type(run, RoomType::MONSTER);
        assert(run.combat.player.get_status(StatusType::STRENGTH) == 2);
        assert(run.relic_counter("EMBER_TEA") == fight);
    }
    // Spent after the fifth combat.
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.combat.player.get_status(StatusType::STRENGTH) == 0);

    // Without the relic: no Strength at the start of the combat.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(plain, RoomType::MONSTER);
    assert(plain.combat.player.get_status(StatusType::STRENGTH) == 0);
    std::cout << "[PASS] Ember Tea: 2 Strength at the start of 5 combats" << std::endl;
}

void test_golden_compass() {
    auto nodes_on_floor = [](const RunManager& r, uint8_t floor) {
        int n = 0;
        for (const MapNode& node : r.map.nodes) n += node.floor == floor ? 1 : 0;
        return n;
    };
    // Without the compass, Act 2 keeps its several paths.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.advance_act();
    assert(plain.current_act == 2 && !plain.has_relic("GOLDEN_COMPASS"));
    bool branches = false;
    for (uint8_t f = 0; f < 14; ++f) branches = branches || nodes_on_floor(plain, f) > 1;
    assert(branches);

    // With it, obtained once the Act 2 map exists, every floor holds a single room.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.advance_act();
    run.add_relic("GOLDEN_COMPASS");
    for (uint8_t f = 0; f <= 15; ++f) assert(nodes_on_floor(run, f) == 1);
    std::cout << "[PASS] Golden Compass: Act 2 map becomes a single path" << std::endl;
} 
void test_paels_tooth() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t deck = run.master_deck.size();
    run.add_relic("PAELS_TOOTH");
    assert(run.has_deck_choice() && run.deck_choice.kind == DeckChoiceKind::REMOVE && run.deck_choice.remaining == 5);
    for (int i = 0; i < 5; ++i) assert(run.resolve_deck_choice(0));
    assert(!run.has_deck_choice() && run.master_deck.size() == deck - 5 && run.paels_tooth_cards.size() == 5);

    // Each won fight gives one back, upgraded.
    enter_room_of_type(run, RoomType::MONSTER);
    run.proceed_after_combat();
    assert(run.master_deck.size() == deck - 4 && run.master_deck.back().upgraded && run.paels_tooth_cards.size() == 4);

    // Without the relic nothing is asked and nothing comes back.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(!plain.has_deck_choice());
    enter_room_of_type(plain, RoomType::MONSTER);
    plain.proceed_after_combat();
    assert(plain.master_deck.size() == deck);
}

void test_membership_card() {
    // Without the card, the prices are the ones the stock is drawn with.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.open_shop();
    ShopInventory drawn;
    drawn.generate(CharacterId::IRONCLAD, 4242, 0);
    assert(!plain.has_relic("MEMBERSHIP_CARD") && plain.current_shop.card_removal_cost == 75);
    assert(plain.current_shop.cards[0].cost == drawn.cards[0].cost);

    // With it, the same shelves at half price. The stock is drawn again from the seed the run used for the visit
    // (its next random number), with the same relics owned.
    RunManager member;
    member.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    member.add_relic("MEMBERSHIP_CARD");
    uint32_t seed = member.rng_state();
    seed ^= seed << 13;
    seed ^= seed >> 17;
    seed ^= seed << 5;
    ShopInventory full;
    full.generate(CharacterId::IRONCLAD, seed, 0, std::span<const std::string_view>(member.relics.begin(), member.relics.size()));
    member.open_shop();
    assert(member.has_relic("MEMBERSHIP_CARD"));

    auto half = [](int16_t cost) { return static_cast<int16_t>((cost + 1) / 2); };
    const ShopInventory& cheap = member.current_shop;
    assert(full.cards.size() == cheap.cards.size() && full.relics.size() == cheap.relics.size() &&
           full.potions.size() == cheap.potions.size());
    for (size_t i = 0; i < full.cards.size(); ++i) {
        assert(full.cards[i].card.id == cheap.cards[i].card.id && cheap.cards[i].cost == half(full.cards[i].cost));
    }
    for (size_t i = 0; i < full.relics.size(); ++i) {
        assert(full.relics[i].relic_id == cheap.relics[i].relic_id && cheap.relics[i].cost == half(full.relics[i].cost));
    }
    for (size_t i = 0; i < full.potions.size(); ++i) {
        assert(full.potions[i].potion_id == cheap.potions[i].potion_id &&
               cheap.potions[i].cost == half(full.potions[i].cost));
    }
    assert(full.card_removal_cost == 75 && cheap.card_removal_cost == 38);

    // The price paid is the lowered one.
    member.gold = 500;
    int16_t price = cheap.cards[0].cost;
    assert(member.buy_shop_card(0) && member.gold == 500 - price);
    assert(member.remove_card_at_shop(0) && member.gold == 500 - price - 38);

    // Bought from the merchant himself: full price for the card, half price for what is left on the shelves.
    RunManager buyer;
    buyer.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    buyer.gold = 999;
    buyer.open_shop();
    buyer.current_shop.relics[2] = ShopRelic{ "MEMBERSHIP_CARD", 100, false };
    int16_t card_cost = buyer.current_shop.cards[0].cost;
    assert(buyer.buy_shop_relic(2) && buyer.gold == 899);
    assert(buyer.current_shop.cards[0].cost == half(card_cost) && buyer.current_shop.card_removal_cost == 38);

    // Obtained away from the merchant, it changes nothing until the next visit.
    RunManager later;
    later.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    later.open_shop();
    later.leave_shop();
    later.add_relic("MEMBERSHIP_CARD");
    assert(later.current_shop.cards[0].cost == drawn.cards[0].cost && later.current_shop.card_removal_cost == 75);
    std::cout << "[PASS] Membership Card: every price halved, removal included, at once when bought in the shop" << std::endl;
}

void test_the_courier() {
    // Without the relic: full prices, and what is bought stays bought.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.gold = 999;
    plain.open_shop();
    assert(!plain.has_relic("THE_COURIER"));
    int16_t full_card = plain.current_shop.cards[0].cost;
    assert(plain.buy_shop_card(0) && plain.current_shop.cards[0].purchased);
    assert(plain.buy_shop_potion(0) && plain.current_shop.potions[0].purchased);

    // With it: the same shelves 20 % cheaper, and what is bought is renewed at once.
    RunManager courier;
    courier.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    courier.add_relic("THE_COURIER");
    courier.gold = 999;
    courier.open_shop();
    auto fifth_off = [](int16_t cost) { return static_cast<int16_t>((cost * 80 + 50) / 100); };
    assert(courier.current_shop.cards[0].cost == fifth_off(full_card));
    assert(courier.current_shop.card_removal_cost == fifth_off(75));
    int16_t price = courier.current_shop.cards[0].cost;
    assert(courier.buy_shop_card(0) && courier.gold == 999 - price && !courier.current_shop.cards[0].purchased);
    assert(courier.buy_shop_card(0));

    std::string_view bought_relic = courier.current_shop.relics[0].relic_id;
    int16_t relic_price = courier.current_shop.relics[0].cost;
    int gold = courier.gold;
    assert(courier.buy_shop_relic(0) && courier.gold == gold - relic_price && courier.has_relic(bought_relic));
    assert(!courier.current_shop.relics[0].purchased && courier.current_shop.relics[0].relic_id != bought_relic);

    std::string_view bought_potion = courier.current_shop.potions[0].potion_id;
    assert(courier.buy_shop_potion(0));
    assert(!courier.current_shop.potions[0].purchased && !courier.current_shop.potions[0].potion_id.empty());
    (void)bought_potion;

    // Bought from the merchant himself: the shelves are cheaper at once.
    RunManager buyer;
    buyer.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    buyer.gold = 999;
    buyer.open_shop();
    buyer.current_shop.relics[2] = ShopRelic{ "THE_COURIER", 100, false };
    int16_t card_cost = buyer.current_shop.cards[0].cost;
    assert(buyer.buy_shop_relic(2) && buyer.has_relic("THE_COURIER"));
    assert(buyer.current_shop.cards[0].cost == fifth_off(card_cost) && buyer.current_shop.card_removal_cost == fifth_off(75));
    std::cout << "[PASS] The Courier: prices -20%, bought cards, relics and potions renewed at once" << std::endl;
}

void test_yummy_cookie() {
    // Obtained: four cards of the deck are upgraded.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    auto upgraded_count = [](const RunManager& r) {
        int n = 0;
        for (size_t i = 0; i < r.master_deck.size(); ++i) n += r.master_deck[i].upgraded ? 1 : 0;
        return n;
    };
    int before = upgraded_count(run);
    run.add_relic("YUMMY_COOKIE");
    assert(run.has_relic("YUMMY_COOKIE"));
    assert(upgraded_count(run) == before + 4);

    // Any other relic upgrades nothing.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(upgraded_count(plain) == before);
    std::cout << "[PASS] Yummy Cookie: upgrades 4 cards when obtained" << std::endl;
}

void test_delicate_frond() {
    // With the relic, a fight starts with every potion slot filled.
    RunManager with;
    with.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    with.add_relic("DELICATE_FROND");
    with.potions.clear();
    enter_room_of_type(with, RoomType::MONSTER);
    assert(with.potion_belt_full());

    // Without it, the belt stays as it was.
    RunManager without;
    without.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    without.potions.clear();
    enter_room_of_type(without, RoomType::MONSTER);
    assert(without.potions.empty());
    std::cout << "[PASS] Delicate Frond: every empty potion slot filled at combat start" << std::endl;
}

void test_circlet() {
    // Circlet is a fallback relic with no effect in game: a run holding it plays exactly like one without it.
    RunManager with;
    with.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    with.add_relic("CIRCLET");
    enter_room_of_type(with, RoomType::MONSTER);

    RunManager without;
    without.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(without, RoomType::MONSTER);

    assert(with.has_relic("CIRCLET"));
    assert(!without.has_relic("CIRCLET"));
    assert(with.max_hp == without.max_hp && with.gold == without.gold);
    assert(with.combat.player.energy == without.combat.player.energy);
    assert(with.combat.player.get_status(StatusType::DEXTERITY) == without.combat.player.get_status(StatusType::DEXTERITY));
    assert(with.combat.player.get_status(StatusType::STRENGTH) == without.combat.player.get_status(StatusType::STRENGTH));
    std::cout << "[PASS] Circlet: no effect, with or without the relic" << std::endl;
}

void test_beating_remnant() {
    // With the relic: no more than 20 HP lost in a turn; the budget comes back at the start of the next turn.
    CombatEngine remnant = make_relic_combat(CharacterId::IRONCLAD, PowerId::BEATING_REMNANT, 20);
    int16_t hp_start = remnant.player.hp;
    remnant.damage_player(30);
    assert(remnant.player.hp == hp_start - 20);
    remnant.damage_player(10);
    assert(remnant.player.hp == hp_start - 20);
    remnant.start_player_turn();
    remnant.damage_player(15);
    assert(remnant.player.hp == hp_start - 35);

    // Without the relic: all of the HP is lost.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, {});
    int16_t plain_start = plain.player.hp;
    plain.damage_player(30);
    assert(plain.player.hp == plain_start - 30);
    std::cout << "[PASS] Beating Remnant: no more than 20 HP lost per turn" << std::endl;
}

void test_vambrace() {
    // With the relic: the first Block a card gives in the combat is doubled; the later ones are not.
    CombatEngine with = make_relic_combat(CharacterId::IRONCLAD, PowerId::VAMBRACE, 1);
    int16_t with_start = with.player.block;
    with.resolving_card = true;
    with.gain_block(5);
    assert(with.player.block == with_start + 10);
    with.gain_block(5);
    assert(with.player.block == with_start + 15);
    with.resolving_card = false;
    with.start_player_turn();
    with.resolving_card = true;
    int16_t next_turn = with.player.block;
    with.gain_block(4);
    assert(with.player.block == next_turn + 4);
    with.resolving_card = false;

    // Block that is not from a card (a relic or a power) is never doubled.
    CombatEngine other = make_relic_combat(CharacterId::IRONCLAD, PowerId::VAMBRACE, 1);
    int16_t other_start = other.player.block;
    other.add_block(6);
    assert(other.player.block == other_start + 6);

    // Without the relic: the Block of a card is the plain amount.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, {});
    int16_t plain_start = plain.player.block;
    plain.resolving_card = true;
    plain.gain_block(5);
    assert(plain.player.block == plain_start + 5);
    plain.resolving_card = false;
    std::cout << "[PASS] Vambrace: the first Block a card gives each combat is doubled" << std::endl;
}

void test_mr_struggles() {
    // With the relic: at the start of each turn, damage equal to the turn number to every enemy (1, then 2, then 3).
    CombatEngine carnage = make_relic_combat(CharacterId::IRONCLAD, PowerId::MR_STRUGGLES, 1, true);
    assert(carnage.enemies[0].hp == 17 && carnage.enemies[2].hp == 41);
    end_turn_idle(carnage);
    assert(carnage.enemies[0].hp == 15 && carnage.enemies[2].hp == 39);
    end_turn_idle(carnage);
    assert(carnage.enemies[0].hp == 12 && carnage.enemies[2].hp == 36);

    // Without the relic: the enemies lose no HP at the start of a turn.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, {}, true);
    assert(plain.enemies[0].hp == 18 && plain.enemies[2].hp == 42);
    end_turn_idle(plain);
    assert(plain.enemies[0].hp == 18 && plain.enemies[2].hp == 42);
    std::cout << "[PASS] Monsieur Carnage: turn number in damage to every enemy at the start of each turn" << std::endl;
}

void test_dingy_rug() {
    // Card rewards: with the relic, some offers are Colorless cards; without it, never.
    uint32_t colorless = 0, offers = 0;
    for (uint32_t seed = 1; seed <= 50; ++seed) {
        RunManager with_rug;
        with_rug.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
        with_rug.add_relic("DINGY_RUG");
        enter_room_of_type(with_rug, RoomType::MONSTER);
        with_rug.generate_rewards();

        RunManager without_rug;
        without_rug.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
        enter_room_of_type(without_rug, RoomType::MONSTER);
        without_rug.generate_rewards();

        for (size_t i = 0; i < with_rug.current_card_rewards.size(); ++i) {
            ++offers;
            if (card_meta(with_rug.current_card_rewards[i].id).pool == CardPool::COLORLESS) ++colorless;
        }
        for (size_t i = 0; i < without_rug.current_card_rewards.size(); ++i) {
            assert(card_meta(without_rug.current_card_rewards[i].id).pool != CardPool::COLORLESS);
        }
    }
    assert(offers == 150 && colorless > 0);

    std::cout << "[PASS] Dingy Rug: " << colorless << " of " << offers << " card rewards are Colorless" << std::endl;
}

void test_paels_legion() {
    // With the relic: the Block of a card is doubled, then the relic sleeps for 2 turns.
    CombatEngine legion = make_relic_combat(CharacterId::IRONCLAD, PowerId::PAELS_LEGION, 2);
    legion.player.block = 0;
    play(legion, CardId::DEFEND);
    assert(legion.player.block == 10);
    play(legion, CardId::DEFEND);
    assert(legion.player.block == 15);
    legion.end_player_turn();
    legion.player.block = 0;
    play(legion, CardId::DEFEND);
    assert(legion.player.block == 5);
    legion.end_player_turn();
    legion.player.block = 0;
    play(legion, CardId::DEFEND);
    assert(legion.player.block == 5);
    legion.end_player_turn();
    legion.player.block = 0;
    play(legion, CardId::DEFEND);
    assert(legion.player.block == 10);

    // Without the relic: nothing changes.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, {});
    plain.player.block = 0;
    play(plain, CardId::DEFEND);
    assert(plain.player.block == 5);
    std::cout << "[PASS] Pael's Legion: a card's Block doubled, then asleep for 2 turns" << std::endl;
}

void test_golden_pearl() {
    // Obtained: 150 gold, paid once.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    int16_t gold = run.gold;
    run.add_relic("GOLDEN_PEARL");
    assert(run.has_relic("GOLDEN_PEARL"));
    assert(run.gold == gold + 150);

    // Without the relic, the gold stays where it is.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("GOLDEN_PEARL") && plain.gold == gold);

    std::cout << "[PASS] Golden Pearl: +150 gold when obtained" << std::endl;
}

void test_booming_conch() {
    // Booming Conch: in an Elite room, 2 more cards and 1 more energy on the first turn, nothing after it.
    RunManager plain_elite;
    plain_elite.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(plain_elite, RoomType::ELITE);
    RunManager conch;
    conch.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    conch.add_relic("BOOMING_CONCH");
    enter_room_of_type(conch, RoomType::ELITE);
    assert(conch.combat.hand.size() == plain_elite.combat.hand.size() + 2);
    assert(conch.combat.player.energy == plain_elite.combat.player.energy + 1);
    end_turn_idle(conch.combat);
    end_turn_idle(plain_elite.combat);
    assert(conch.combat.hand.size() == plain_elite.combat.hand.size());
    assert(conch.combat.player.energy == plain_elite.combat.player.energy);

    // Not in a monster room: nothing changes.
    RunManager monster = make_relic_run(CharacterId::IRONCLAD, { "BOOMING_CONCH" });
    RunManager plain_monster = make_relic_run(CharacterId::IRONCLAD, {});
    assert(monster.combat.hand.size() == plain_monster.combat.hand.size());
    assert(monster.combat.player.energy == plain_monster.combat.player.energy);

    // Without the relic, in an Elite room: nothing changes.
    assert(plain_elite.combat.hand.size() == 5);
    std::cout << "[PASS] Booming Conch: 2 cards and 1 energy in an Elite room, first turn only" << std::endl;
}

void test_arcane_scroll() {
    // With the relic: one random Rare card of the hero's pool joins the deck on pickup.
    RunManager scroll;
    scroll.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t before = scroll.master_deck.size();
    scroll.add_relic("ARCANE_SCROLL");
    assert(scroll.has_relic("ARCANE_SCROLL"));
    assert(scroll.master_deck.size() == before + 1);
    CardId added = scroll.master_deck[scroll.master_deck.size() - 1].id;
    assert(card_meta(added).rarity == CardRarity::RARE);
    std::cout << "[PASS] Arcane Scroll: a random Rare card added to the deck on pickup" << std::endl;

    // Without the relic: nothing is added.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t start = plain.master_deck.size();
    plain.add_relic("STRAWBERRY");
    assert(plain.master_deck.size() == start);
    std::cout << "[PASS] Arcane Scroll: no card without the relic" << std::endl;
}

void test_lead_paperweight() {
    // Lead Paperweight: on pickup, 2 distinct Colorless cards are offered and one of them is taken.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t deck = run.master_deck.size();
    run.add_relic("LEAD_PAPERWEIGHT");
    assert(run.phase == RunPhase::REWARD_SCREEN && run.current_card_rewards.size() == 2);
    for (size_t i = 0; i < run.current_card_rewards.size(); ++i) {
        assert(card_meta(run.current_card_rewards[i].id).pool == CardPool::COLORLESS);
    }
    assert(run.current_card_rewards[0].id != run.current_card_rewards[1].id);
    run.pick_card_reward(1);
    assert(run.master_deck.size() == deck + 1);
    assert(run.phase == RunPhase::MAP_NAVIGATION);

    // Without the relic nothing is offered.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(plain.phase != RunPhase::REWARD_SCREEN && plain.current_card_rewards.empty());
}

void test_neows_torment() {
    // Obtained: one Neow's Fury is added to the deck.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    auto fury_count = [](const RunManager& r) {
        int n = 0;
        for (size_t i = 0; i < r.master_deck.size(); ++i) n += r.master_deck[i].id == CardId::NEOWS_FURY ? 1 : 0;
        return n;
    };
    int before = fury_count(run);
    size_t size_before = run.master_deck.size();
    run.add_relic("NEOWS_TORMENT");
    assert(run.has_relic("NEOWS_TORMENT"));
    assert(fury_count(run) == before + 1);
    assert(run.master_deck.size() == size_before + 1);

    // Without the relic: no Neow's Fury is added.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(fury_count(plain) == before);
    std::cout << "[PASS] Neow's Torment: adds 1 Neow's Fury to the deck when obtained" << std::endl;
}

void test_precise_scissors() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t deck = run.master_deck.size();
    run.add_relic("PRECISE_SCISSORS");
    assert(run.has_deck_choice() && run.deck_choice.kind == DeckChoiceKind::REMOVE && run.deck_choice.remaining == 1);
    assert(run.resolve_deck_choice(0));
    assert(!run.has_deck_choice() && run.master_deck.size() == deck - 1);

    // Without the relic nothing is asked and no card leaves the deck.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(!plain.has_deck_choice());
    assert(plain.master_deck.size() == deck);
}

void test_new_leaf() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    CardId first = run.master_deck[0].id;
    run.add_relic("NEW_LEAF");
    assert(run.has_deck_choice() && run.deck_choice.kind == DeckChoiceKind::TRANSFORM && run.deck_choice.remaining == 1);
    assert(!run.deck_choice.optional && run.deck_choice.transform_into == CardId::NONE);
    assert(run.resolve_deck_choice(0));
    assert(!run.has_deck_choice() && run.master_deck[0].id != first);
    std::cout << "[PASS] New Leaf: one chosen card transformed on pickup" << std::endl;

    // Without the relic nothing is asked.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(!plain.has_deck_choice() && plain.master_deck[0].id == first);
    std::cout << "[PASS] New Leaf: nothing asked without the relic" << std::endl;
}

void test_small_capsule() {
    // Obtained: one random relic joins the bag, on top of the capsule itself.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t before = run.relics.size();
    run.add_relic("SMALL_CAPSULE");
    assert(run.has_relic("SMALL_CAPSULE"));
    assert(run.relics.size() == before + 2);

    // Without the capsule, a relic gives nothing more.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t plain_before = plain.relics.size();
    plain.add_relic("ANCHOR");
    assert(plain.relics.size() == plain_before + 1);
    std::cout << "[PASS] Small Capsule: one random relic when obtained, none without it" << std::endl;
}

void test_nutritious_oyster() {
    // Obtained: 11 more maximum HP, and the current HP follow.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.hp = 50;
    int16_t max_hp = run.max_hp;
    run.add_relic("NUTRITIOUS_OYSTER");
    assert(run.has_relic("NUTRITIOUS_OYSTER"));
    assert(run.max_hp == max_hp + 11 && run.hp == 61);

    // Without the relic, nothing raises the maximum HP.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.hp = 50;
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("NUTRITIOUS_OYSTER"));
    assert(plain.max_hp == max_hp && plain.hp == 50);

    std::cout << "[PASS] Nutritious Oyster: +11 Max HP when obtained" << std::endl;
}

void test_lava_rock() {
    // With the rock, the Act 1 Boss gives 2 new relics when defeated, never the rock nor the same one twice.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("LAVA_ROCK");
    size_t before = run.relics.size();
    enter_room_of_type(run, RoomType::BOSS);
    run.proceed_after_combat();
    assert(run.relics.size() == before + 2);
    assert(run.relics[before] != run.relics[before + 1]);
    assert(run.relics[before] != "LAVA_ROCK" && run.relics[before + 1] != "LAVA_ROCK");

    // Without it, the Act 1 Boss adds no relic to the run.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t plain_before = plain.relics.size();
    enter_room_of_type(plain, RoomType::BOSS);
    plain.proceed_after_combat();
    assert(plain.relics.size() == plain_before && !plain.has_relic("LAVA_ROCK"));
    std::cout << "[PASS] Lava Rock: Act 1 Boss gives 2 relics" << std::endl;
}

void test_stone_humidifier() {
    // With the relic: each Rest raises Max HP by 5 (and heals by the same amount).
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("STONE_HUMIDIFIER");
    run.hp = 40;
    int16_t max_hp = run.max_hp;
    run.rest_at_campfire();
    assert(run.max_hp == max_hp + 5 && run.hp == 40 + max_hp * 3 / 10 + 5);
    run.rest_at_campfire();
    assert(run.max_hp == max_hp + 10);

    // Without the relic: a Rest leaves Max HP alone.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.hp = 40;
    plain.rest_at_campfire();
    assert(!plain.has_relic("STONE_HUMIDIFIER"));
    assert(plain.max_hp == max_hp);
    std::cout << "[PASS] Stone Humidifier: +5 Max HP at each Rest Site rest" << std::endl;
}

void test_dream_catcher() {
    // With the relic: each Rest offers one set of cards; taking one adds it to the deck, skipping adds nothing.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("DREAM_CATCHER");
    run.hp = 40;
    size_t deck = run.master_deck.size();
    run.rest_at_campfire();
    assert(run.phase == RunPhase::REWARD_SCREEN && run.current_card_rewards.size() == 3);
    run.pick_card_reward(0);
    assert(run.master_deck.size() == deck + 1);
    assert(run.phase == RunPhase::MAP_NAVIGATION);
    run.rest_at_campfire();
    assert(run.phase == RunPhase::REWARD_SCREEN);
    run.skip_card_rewards();
    assert(run.master_deck.size() == deck + 1);
    assert(run.phase == RunPhase::MAP_NAVIGATION);

    // Without the relic: a Rest offers no card.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.hp = 40;
    size_t plain_deck = plain.master_deck.size();
    plain.rest_at_campfire();
    assert(!plain.has_relic("DREAM_CATCHER"));
    assert(plain.phase == RunPhase::MAP_NAVIGATION && plain.master_deck.size() == plain_deck);
    std::cout << "[PASS] Dream Catcher: a card reward after each Rest" << std::endl;
}

void test_meal_ticket() {
    // With the relic: entering a shop heals 15 HP, capped at Max HP.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("MEAL_TICKET");
    run.hp = 10;
    enter_room_of_type(run, RoomType::SHOP);
    assert(run.hp == 25);
    run.hp = run.max_hp - 5;
    enter_room_of_type(run, RoomType::SHOP);
    assert(run.hp == run.max_hp);

    // Entering another room heals nothing.
    run.hp = 10;
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.hp == 10);

    // Without the relic: a shop heals nothing.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.hp = 10;
    enter_room_of_type(plain, RoomType::SHOP);
    assert(!plain.has_relic("MEAL_TICKET") && plain.hp == 10);
    std::cout << "[PASS] Meal Ticket: 15 HP when entering a shop" << std::endl;
}

void test_shovel() {
    // With the Shovel: a Rest Site digs up one random relic the run does not own yet.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("SHOVEL");
    size_t before = run.relics.size();
    enter_room_of_type(run, RoomType::REST_SITE);
    assert(run.dig_at_campfire());
    assert(run.relics.size() == before + 1 && run.phase == RunPhase::MAP_NAVIGATION);
    assert(run.hook_count(RunHook::RESTED) == 0); // digging is not resting

    // Without the Shovel: nothing is dug up.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t plain_before = plain.relics.size();
    enter_room_of_type(plain, RoomType::REST_SITE);
    assert(!plain.dig_at_campfire());
    assert(plain.relics.size() == plain_before && !plain.has_relic("SHOVEL"));
    std::cout << "[PASS] Shovel: a random relic dug up at a Rest Site, nothing without it" << std::endl;
}

void test_tiny_mailbox() {
    // With the relic: each Rest procures 2 potions.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("TINY_MAILBOX");
    size_t before = run.potions.size();
    run.rest_at_campfire();
    assert(run.potions.size() == before + 2);

    // Without the relic: a Rest gives no potion.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t plain_before = plain.potions.size();
    plain.rest_at_campfire();
    assert(!plain.has_relic("TINY_MAILBOX"));
    assert(plain.potions.size() == plain_before);
    std::cout << "[PASS] Tiny Mailbox: 2 potions at each Rest" << std::endl;
}

void test_regal_pillow() {
    // With the relic: each Rest heals 15 HP more than the usual 30% of Max HP.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("REGAL_PILLOW");
    run.max_hp = 80;
    run.hp = 20;
    run.rest_at_campfire();
    assert(run.hp == 59);  // 20 + 24 (30%) + 15

    // Without the relic: a Rest heals only the usual 30% of Max HP.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.max_hp = 80;
    plain.hp = 20;
    plain.rest_at_campfire();
    assert(!plain.has_relic("REGAL_PILLOW"));
    assert(plain.hp == 44);  // 20 + 24
    std::cout << "[PASS] Regal Pillow: 15 extra HP at each Rest" << std::endl;
}

void test_eternal_feather() {
    // With the relic: entering a Rest Site heals 3 HP per 5 cards of the deck, capped at Max HP.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("ETERNAL_FEATHER");
    int expected = static_cast<int>(run.master_deck.size()) / 5 * 3;
    run.hp = 1;
    enter_room_of_type(run, RoomType::REST_SITE);
    assert(run.hp == std::min<int>(run.max_hp, 1 + expected));
    run.hp = run.max_hp - 1;
    enter_room_of_type(run, RoomType::REST_SITE);
    assert(run.hp == run.max_hp);

    // Without the relic: entering a Rest Site heals nothing.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.hp = 1;
    enter_room_of_type(plain, RoomType::REST_SITE);
    assert(!plain.has_relic("ETERNAL_FEATHER") && plain.hp == 1);
    std::cout << "[PASS] Eternal Feather: heals 3 HP per 5 cards when entering a Rest Site" << std::endl;
}

void test_planisphere() {
    // Entering a ? room heals 5 HP; the other rooms do not.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 482);
    run.add_relic("PLANISPHERE");
    run.hp = 20;
    enter_room_of_type(run, RoomType::EVENT);
    assert(run.hp == 25);
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.hp == 25);

    // Without the relic: entering a ? room heals nothing.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 482);
    plain.hp = 20;
    enter_room_of_type(plain, RoomType::EVENT);
    assert(!plain.has_relic("PLANISPHERE"));
    assert(plain.hp == 20);
    std::cout << "[PASS] Planisphere: 5 HP when entering a ? room" << std::endl;
}

void test_pomander() {
    // Obtained: one card of the deck is picked to be upgraded.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("POMANDER");
    assert(run.has_relic("POMANDER"));
    assert(run.has_deck_choice() && run.deck_choice.kind == DeckChoiceKind::UPGRADE && run.deck_choice.remaining == 1);
    assert(!run.skip_deck_choice()); // it has to be made
    assert(run.resolve_deck_choice(0) && run.master_deck[0].upgraded);
    assert(!run.has_deck_choice());

    // Without the relic: nothing is asked and nothing is upgraded.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(!plain.has_deck_choice());
    assert(std::none_of(plain.master_deck.begin(), plain.master_deck.end(), [](const DeckCard& c) { return c.upgraded; }));
    std::cout << "[PASS] Pomander: one card of the deck picked to be upgraded when obtained" << std::endl;
}

void test_neows_talisman() {
    // Obtained: one Strike and one Defend of the deck are upgraded, nothing else.
    auto upgraded_of = [](const RunManager& r, CardId id) {
        int n = 0;
        for (size_t i = 0; i < r.master_deck.size(); ++i) {
            if (r.master_deck[i].upgraded && r.master_deck[i].id == id) ++n;
        }
        return n;
    };
    auto upgraded_total = [](const RunManager& r) {
        int n = 0;
        for (size_t i = 0; i < r.master_deck.size(); ++i) n += r.master_deck[i].upgraded ? 1 : 0;
        return n;
    };
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    int strikes_before = upgraded_of(run, CardId::STRIKE);
    int defends_before = upgraded_of(run, CardId::DEFEND);
    int total_before = upgraded_total(run);
    run.add_relic("NEOWS_TALISMAN");
    assert(run.has_relic("NEOWS_TALISMAN"));
    assert(upgraded_of(run, CardId::STRIKE) == strikes_before + 1);
    assert(upgraded_of(run, CardId::DEFEND) == defends_before + 1);
    assert(upgraded_total(run) == total_before + 2);

    // Without the relic: no card is upgraded.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(upgraded_total(plain) == total_before);
    std::cout << "[PASS] Neow's Talisman: one Strike and one Defend upgraded when obtained" << std::endl;
}

void test_large_capsule() {
    // Obtained: 2 random relics join the run, and a Strike and a Defend join the deck.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t relics = run.relics.size();
    size_t deck = run.master_deck.size();
    int strikes = 0;
    int defends = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == CardId::STRIKE) ++strikes;
        if (run.master_deck[i].id == CardId::DEFEND) ++defends;
    }
    run.add_relic("LARGE_CAPSULE");
    assert(run.has_relic("LARGE_CAPSULE"));
    assert(run.relics.size() == relics + 3);
    assert(run.master_deck.size() == deck + 2);
    int strikes_after = 0;
    int defends_after = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == CardId::STRIKE) ++strikes_after;
        if (run.master_deck[i].id == CardId::DEFEND) ++defends_after;
    }
    assert(strikes_after == strikes + 1 && defends_after == defends + 1);

    // Without the relic: no relic and no card is given.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t plain_relics = plain.relics.size();
    size_t plain_deck = plain.master_deck.size();
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("LARGE_CAPSULE"));
    assert(plain.relics.size() == plain_relics + 1);
    assert(plain.master_deck.size() == plain_deck);
    std::cout << "[PASS] Large Capsule: 2 random relics and a Strike and a Defend on pickup" << std::endl;
}

void test_astrolabe() {
    // Obtained: three chosen cards are transformed, then upgraded.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 778);
    size_t size = run.master_deck.size();
    run.add_relic("ASTROLABE");
    assert(run.has_deck_choice() && run.deck_choice.kind == DeckChoiceKind::TRANSFORM);
    assert(run.deck_choice.remaining == 3 && !run.deck_choice.optional && run.deck_choice.upgrade_result);
    for (int i = 0; i < 3; ++i) {
        CardId before = run.master_deck[i].id;
        assert(run.resolve_deck_choice(i));
        assert(run.master_deck[i].id != before && run.master_deck[i].upgraded);
    }
    assert(!run.has_deck_choice() && run.master_deck.size() == size);

    // Without the relic: nothing is asked and no card changes.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 778);
    plain.add_relic("ANCHOR");
    assert(!plain.has_deck_choice());
    std::cout << "[PASS] Astrolabe: transforms 3 cards then upgrades them when obtained" << std::endl;
}

void test_neows_bones() {
    // Obtained: 2 more relics among Neow's own, then 1 random curse. Neow's Bones itself is never drawn again.
    auto is_neow_relic = [](std::string_view id) {
        for (const AncientSlot& slot : get_ancient(AncientId::NEOW).slots) {
            for (const AncientRelic& option : slot.options) {
                if (option.id == id) return true;
            }
        }
        return false;
    };
    auto count_curses = [](const RunManager& run) {
        int n = 0;
        for (size_t i = 0; i < run.master_deck.size(); ++i) {
            if (card_meta(run.master_deck[i].id).pool == CardPool::CURSE) ++n;
        }
        return n;
    };

    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t relics_before = run.relics.size();
    size_t deck_before = run.master_deck.size();
    int curses_before = count_curses(run);
    run.add_relic("NEOWS_BONES");
    assert(run.has_relic("NEOWS_BONES"));
    assert(run.relics.size() == relics_before + 3);
    assert(run.master_deck.size() == deck_before + 1);
    assert(count_curses(run) == curses_before + 1);
    for (size_t i = relics_before + 1; i < run.relics.size(); ++i) {
        assert(is_neow_relic(run.relics[i]));
        assert(run.relics[i] != "NEOWS_BONES");
    }
    assert(run.relics[relics_before + 1] != run.relics[relics_before + 2]);

    // Without the relic, no Neow relic and no curse.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t plain_relics = plain.relics.size();
    size_t plain_deck = plain.master_deck.size();
    plain.add_relic("STRAWBERRY");
    assert(plain.relics.size() == plain_relics + 1);
    assert(plain.master_deck.size() == plain_deck);
    assert(count_curses(plain) == 0);
}

void test_empty_cage() {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t deck = run.master_deck.size();
    run.add_relic("EMPTY_CAGE");
    assert(run.has_deck_choice() && run.deck_choice.kind == DeckChoiceKind::REMOVE && run.deck_choice.remaining == 2);
    assert(run.resolve_deck_choice(0));
    assert(run.resolve_deck_choice(0));
    assert(!run.has_deck_choice() && run.master_deck.size() == deck - 2);

    // Without the relic nothing is asked and no card leaves the deck.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(!plain.has_deck_choice());
    assert(plain.master_deck.size() == deck);
    std::cout << "[PASS] Empty Cage: pickup asks for 2 cards to remove, nothing without the relic" << std::endl;
}

void test_pandoras_box() {
    // Obtained: every Strike and Defend of the deck is transformed, the other cards stay as they are.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 778);
    size_t size = run.master_deck.size();
    run.add_relic("PANDORAS_BOX");
    assert(run.has_relic("PANDORAS_BOX"));
    assert(run.master_deck.size() == size);
    int left = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == CardId::STRIKE || run.master_deck[i].id == CardId::DEFEND) ++left;
    }
    assert(left == 0);

    // Without the relic, the starter cards stay.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 778);
    plain.add_relic("ANCHOR");
    int starters = 0;
    for (size_t i = 0; i < plain.master_deck.size(); ++i) {
        if (plain.master_deck[i].id == CardId::STRIKE || plain.master_deck[i].id == CardId::DEFEND) ++starters;
    }
    assert(!plain.has_relic("PANDORAS_BOX") && starters > 0);

    std::cout << "[PASS] Pandora's Box: every Strike and Defend transformed when obtained" << std::endl;
}

void test_runic_pyramid() {
    // Runic Pyramid: the hand stays in hand at the end of the turn, and the next turn draws on top of it.
    RunManager pyramid = make_relic_run(CharacterId::IRONCLAD, { "RUNIC_PYRAMID" });
    assert(pyramid.combat.hand.size() == 5);
    CardId held[5];
    for (size_t i = 0; i < 5; ++i) held[i] = pyramid.combat.hand[i].id;
    end_turn_idle(pyramid.combat);
    assert(pyramid.combat.discard_pile.empty());
    assert(pyramid.combat.hand.size() >= 5);
    for (size_t i = 0; i < 5; ++i) assert(pyramid.combat.hand[i].id == held[i]);

    // Without the relic: the hand goes to the discard pile.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(plain.combat.hand.size() == 5);
    end_turn_idle(plain.combat);
    assert(plain.combat.discard_pile.size() == 5);
    std::cout << "[PASS] Runic Pyramid: the hand is kept at the end of the turn" << std::endl;
}

void test_ectoplasm() {
    // With the relic, no gold is gained: the Golden Pearl's 150 do not come in.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    int16_t gold = run.gold;
    run.add_relic("ECTOPLASM");
    assert(run.has_relic("ECTOPLASM"));
    run.add_relic("GOLDEN_PEARL");
    assert(run.has_relic("GOLDEN_PEARL") && run.gold == gold);

    // Without the relic, the same pearl gives its gold.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("GOLDEN_PEARL");
    assert(!plain.has_relic("ECTOPLASM") && plain.gold == gold + 150);

    // One more energy at the start of each turn, only with the relic.
    run.map.nodes[0].type = RoomType::MONSTER;
    run.enter_room(0);
    plain.map.nodes[0].type = RoomType::MONSTER;
    plain.enter_room(0);
    assert(run.combat.player.energy == plain.combat.player.energy + 1);

    std::cout << "[PASS] Ectoplasm: no gold gained, +1 energy each turn" << std::endl;
}

void test_philosophers_stone() {
    // Every enemy starts the combat with 1 Strength, the player does not.
    RunManager stone = make_relic_run(CharacterId::IRONCLAD, { "PHILOSOPHERS_STONE" });
    for (const Enemy& foe : stone.combat.enemies) assert(foe.get_status(StatusType::STRENGTH) == 1);
    assert(stone.combat.player.get_status(StatusType::STRENGTH) == 0);
    RunManager stone_pack = make_relic_run(CharacterId::IRONCLAD, { "PHILOSOPHERS_STONE" });
    assert(stone_pack.combat.enemy.get_status(StatusType::STRENGTH) == 1);

    // One more energy at the start of each turn: 1 more on the first turn than without the relic.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    for (const Enemy& foe : plain.combat.enemies) assert(foe.get_status(StatusType::STRENGTH) == 0);
    const int plain_energy = plain.combat.player.energy;
    assert(stone.combat.player.energy == plain_energy + 1);
    end_turn_idle(stone.combat);
    end_turn_idle(plain.combat);
    assert(stone.combat.player.energy == plain.combat.player.energy + 1);
    std::cout << "[PASS] Philosopher's Stone: 1 energy each turn, 1 Strength for every enemy" << std::endl;
}

void test_dusty_tome() {
    // Obtained: one card joins the deck, an Ancient card of the hero.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t deck_size = run.master_deck.size();
    run.add_relic("DUSTY_TOME");
    assert(run.has_relic("DUSTY_TOME"));
    assert(run.master_deck.size() == deck_size + 1);
    const CardMeta& gained = card_meta(run.master_deck[run.master_deck.size() - 1].id);
    assert(gained.rarity == CardRarity::ANCIENT && gained.pool == CardPool::IRONCLAD);

    // Without the relic, the deck stays where it is.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("DUSTY_TOME") && plain.master_deck.size() == deck_size);

    std::cout << "[PASS] Dusty Tome: an Ancient card of the hero when obtained" << std::endl;
}

void test_velvet_choker() {
    // With the relic: one more energy at the start of each turn, and six cards at most played each turn.
    RunManager run = make_relic_run(CharacterId::IRONCLAD, { "VELVET_CHOKER" });
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(run.combat.has_power(PowerId::VELVET_CHOKER));
    assert(run.combat.player.energy == plain.combat.player.energy + 1);

    run.combat.hand.clear();
    for (int i = 0; i < 7; ++i) run.combat.hand.push_back(DeckCard{ CardId::DEFEND, false });
    run.combat.player.energy = 10;
    for (int i = 0; i < 6; ++i) assert(run.combat.play_card(0, -1));
    assert(run.combat.play_blocker(0) == PlayBlocker::PLAY_LIMIT);
    assert(!run.combat.play_card(0, -1) && run.combat.hand.size() == 1);

    // Without the relic: the seventh card is played too.
    plain.combat.hand.clear();
    for (int i = 0; i < 7; ++i) plain.combat.hand.push_back(DeckCard{ CardId::DEFEND, false });
    plain.combat.player.energy = 10;
    for (int i = 0; i < 7; ++i) assert(plain.combat.play_card(0, -1));
    assert(!plain.combat.has_power(PowerId::VELVET_CHOKER));
    std::cout << "[PASS] Velvet Choker: +1 energy each turn, no more than 6 cards played" << std::endl;
}

void test_glass_eye() {
    // Obtained: 2 Common, 2 Uncommon and 1 Rare cards join the deck.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 778);
    size_t size = run.master_deck.size();
    run.add_relic("GLASS_EYE");
    assert(run.has_relic("GLASS_EYE"));
    assert(run.master_deck.size() == size + 5);
    int commons = 0, uncommons = 0, rares = 0;
    for (size_t i = size; i < run.master_deck.size(); ++i) {
        CardRarity rarity = card_meta(run.master_deck[i].id).rarity;
        if (rarity == CardRarity::COMMON) ++commons;
        if (rarity == CardRarity::UNCOMMON) ++uncommons;
        if (rarity == CardRarity::RARE) ++rares;
    }
    assert(commons == 2 && uncommons == 2 && rares == 1);

    // Without the relic, the deck stays where it is.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 778);
    size_t plain_size = plain.master_deck.size();
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("GLASS_EYE") && plain.master_deck.size() == plain_size);

    std::cout << "[PASS] Glass Eye: 2 Common, 2 Uncommon and 1 Rare cards when obtained" << std::endl;
}

void test_prismatic_gem() {
    // Start of each turn: one more energy, as with Blood Soaked Rose.
    RunManager gem = make_relic_run(CharacterId::IRONCLAD, { "PRISMATIC_GEM" });
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(gem.combat.player.energy == plain.combat.player.energy + 1);
    end_turn_idle(gem.combat);
    end_turn_idle(plain.combat);
    assert(gem.combat.player.energy == plain.combat.player.energy + 1);

    // Card rewards: with the relic, some offers come from the other heroes' pools; without it, never.
    uint32_t other_color = 0, offers = 0;
    for (uint32_t seed = 1; seed <= 50; ++seed) {
        RunManager with_gem;
        with_gem.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
        with_gem.add_relic("PRISMATIC_GEM");
        enter_room_of_type(with_gem, RoomType::MONSTER);
        with_gem.generate_rewards();

        RunManager without_gem;
        without_gem.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
        enter_room_of_type(without_gem, RoomType::MONSTER);
        without_gem.generate_rewards();

        for (size_t i = 0; i < with_gem.current_card_rewards.size(); ++i) {
            ++offers;
            if (!in_pool(CharacterId::IRONCLAD, with_gem.current_card_rewards[i].id)) ++other_color;
        }
        for (size_t i = 0; i < without_gem.current_card_rewards.size(); ++i) {
            assert(in_pool(CharacterId::IRONCLAD, without_gem.current_card_rewards[i].id));
        }
    }
    assert(offers == 150 && other_color > 0);

    std::cout << "[PASS] Prismatic Gem: +1 energy each turn, " << other_color << " of " << offers
              << " card rewards from other colors" << std::endl;
}

void test_radiant_pearl() {
    // Radiant Pearl: 1 Luminesce added to the hand at the start of the combat, on top of the usual draw.
    RunManager pearl = make_relic_run(CharacterId::REGENT, { "RADIANT_PEARL" });
    size_t luminesces = 0;
    for (const DeckCard& card : pearl.combat.hand) {
        if (card.id == CardId::LUMINESCE) ++luminesces;
    }
    assert(luminesces == 1 && pearl.combat.hand.back().id == CardId::LUMINESCE);
    assert(pearl.combat.hand.back().deck_index == NOT_IN_DECK);
    assert(pearl.combat.hand.size() == 6);

    // Without the relic: no Luminesce, the usual 5 cards.
    RunManager plain = make_relic_run(CharacterId::REGENT, {});
    for (const DeckCard& card : plain.combat.hand) assert(card.id != CardId::LUMINESCE);
    assert(plain.combat.hand.size() == 5);
    std::cout << "[PASS] Radiant Pearl: 1 Luminesce in hand at the start of combat" << std::endl;
}

void test_sand_castle() {
    // Obtained: six cards of the deck are upgraded, never more than the deck holds.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 778);
    auto upgraded_count = [](const RunManager& r) {
        int n = 0;
        for (size_t i = 0; i < r.master_deck.size(); ++i) n += r.master_deck[i].upgraded ? 1 : 0;
        return n;
    };
    int before = upgraded_count(run);
    run.add_relic("SAND_CASTLE");
    assert(run.has_relic("SAND_CASTLE"));
    assert(upgraded_count(run) == std::min(before + 6, static_cast<int>(run.master_deck.size())));

    // Without the relic, no card is upgraded.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 778);
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("SAND_CASTLE") && upgraded_count(plain) == before);
    std::cout << "[PASS] Sand Castle: upgrades 6 random cards when obtained" << std::endl;
}

void test_paels_horn() {
    // Obtained: two Relax join the deck.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t deck = run.master_deck.size();
    run.add_relic("PAELS_HORN");
    assert(run.has_relic("PAELS_HORN"));
    assert(run.master_deck.size() == deck + 2);
    assert(run.master_deck[deck].id == CardId::RELAX && run.master_deck[deck + 1].id == CardId::RELAX);

    // Without the relic, the deck stays where it is.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t plain_deck = plain.master_deck.size();
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("PAELS_HORN") && plain.master_deck.size() == plain_deck);

    std::cout << "[PASS] Paël's Horn: 2 Relax added to the deck when obtained" << std::endl;
}

void test_paels_eye() {
    // With the relic: ending the first turn without a card exhausts the hand and gives an extra turn.
    CombatEngine eye = make_relic_combat(CharacterId::IRONCLAD, PowerId::PAELS_EYE, 1);
    int16_t hp = eye.player.hp;
    size_t exhausted = eye.exhaust_pile.size();
    size_t hand_size = eye.hand.size();
    assert(hand_size > 0);
    end_turn_attacked(eye, 11);
    assert(eye.state == CombatState::PLAYER_TURN);
    assert(eye.player.hp == hp); // the enemies did not act
    assert(eye.exhaust_pile.size() == exhausted + hand_size);
    assert(eye.hand.size() > 0);
    // Once per combat: the next empty turn is a normal one.
    end_turn_attacked(eye, 11);
    assert(eye.player.hp == hp - 11);

    // A turn with a card played gives nothing.
    CombatEngine played = make_relic_combat(CharacterId::IRONCLAD, PowerId::PAELS_EYE, 1);
    play(played, CardId::DEFEND);
    hp = played.player.hp;
    end_turn_attacked(played, 11);
    assert(played.player.hp < hp);
    assert(played.exhaust_pile.empty());

    // Without the relic: the enemies act.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, {});
    hp = plain.player.hp;
    end_turn_attacked(plain, 11);
    assert(plain.player.hp == hp - 11);
    assert(plain.exhaust_pile.empty());
    std::cout << "[PASS] Pael's Eye: empty first turn exhausts the hand and gives an extra turn" << std::endl;
}

void test_ringing_triangle() {
    // With the relic: the hand is kept at the end of the first turn, and only that one.
    CombatEngine triangle = make_relic_combat(CharacterId::IRONCLAD, PowerId::RINGING_TRIANGLE, 1);
    assert(triangle.hand.size() == 5);
    end_turn_idle(triangle);
    assert(triangle.discard_pile.size() == 0);
    assert(triangle.hand.size() == 10);
    end_turn_idle(triangle);
    assert(triangle.discard_pile.size() == 10);

    // Without the relic: the first hand is discarded at the end of the turn.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, {});
    end_turn_idle(plain);
    assert(plain.discard_pile.size() == 5);
    assert(plain.hand.size() == 5);

    std::cout << "[PASS] Ringing Triangle: the first hand is kept at the end of the first turn" << std::endl;
}

void test_toolbox() {
    // With the relic: 3 Colorless cards are offered at the start of the combat, and the one picked joins the hand.
    RunManager toolbox = make_relic_run(CharacterId::IRONCLAD, { "TOOLBOX" });
    CombatEngine& combat = toolbox.combat;
    assert(combat.choice_pending() && combat.pending_choices[0].source == Pile::OFFERS);
    assert(combat.offers.size() == 3 && combat.choice_size() == 1);
    assert(combat.hand.size() == 5);
    for (const DeckCard& offered : combat.offers) assert(card_meta(offered.id).pool == CardPool::COLORLESS);
    const DeckCard picked = combat.offers[1];
    const uint8_t picks[] = { 1 };
    assert(combat.resolve_choice(picks));
    assert(!combat.choice_pending() && combat.offers.empty());
    assert(combat.hand.size() == 6 && combat.hand.back().id == picked.id);
    assert(combat.hand.back().deck_index == NOT_IN_DECK);
    // Offered once per combat: the next turn is dealt its own hand, with no new offer.
    end_turn_idle(combat);
    assert(!combat.choice_pending() && combat.hand.size() == 5);

    // Without the relic: no offer at the start of the combat.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(!plain.combat.choice_pending() && plain.combat.offers.empty() && plain.combat.hand.size() == 5);
    std::cout << "[PASS] Toolbox: a Colorless card is picked from 3 offers at the start of the combat" << std::endl;
}

void test_cursed_pearl() {
    // Obtained: a Greed curse joins the deck and 333 gold is paid once.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    size_t deck_size = run.master_deck.size();
    int16_t gold = run.gold;
    run.add_relic("CURSED_PEARL");
    assert(run.has_relic("CURSED_PEARL"));
    assert(run.master_deck.size() == deck_size + 1);
    assert(run.master_deck[run.master_deck.size() - 1].id == CardId::GREED);
    assert(run.gold == gold + 333);

    // Without the relic, the deck and the gold stay where they are.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("CURSED_PEARL") && plain.master_deck.size() == deck_size && plain.gold == gold);

    std::cout << "[PASS] Cursed Pearl: Greed curse and +333 gold when obtained" << std::endl;
}

void test_calling_bell() {
    // Obtained: the unique Curse of the Bell joins the deck, then 3 random relics join the run.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 779);
    size_t relics_before = run.relics.size();
    size_t deck_before = run.master_deck.size();
    int bells_before = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == CardId::CURSE_OF_THE_BELL) ++bells_before;
    }
    run.add_relic("CALLING_BELL");
    assert(run.has_relic("CALLING_BELL"));
    assert(run.relics.size() == relics_before + 4);
    assert(run.master_deck.size() == deck_before + 1);
    int bells_after = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (run.master_deck[i].id == CardId::CURSE_OF_THE_BELL) ++bells_after;
    }
    assert(bells_after == bells_before + 1);

    // Without the relic: no relic and no Curse of the Bell are given.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 779);
    size_t plain_relics = plain.relics.size();
    size_t plain_deck = plain.master_deck.size();
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("CALLING_BELL"));
    assert(plain.relics.size() == plain_relics + 1);
    assert(plain.master_deck.size() == plain_deck);
    std::cout << "[PASS] Calling Bell: Curse of the Bell and 3 random relics on pickup" << std::endl;
}

void test_royal_poison() {
    // With the relic: 4 HP lost at the start of the fight. Without it: the same run keeps its HP.
    RunManager poison = make_relic_run(CharacterId::IRONCLAD, { "ROYAL_POISON" });
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(poison.combat.player.hp == plain.combat.player.hp - 4);
    std::cout << "[PASS] Royal Poison: 4 HP lost at the start of each combat" << std::endl;
}

void test_ghost_seed() {
    // With the relic: the Strikes and Defends of the fight are Ethereal, the other cards are not.
    RunManager with = make_relic_run(CharacterId::IRONCLAD, { "GHOST_SEED" });
    int strikes = 0;
    int defends = 0;
    for (const DeckCard& card : with.combat.draw_pile) {
        if (card.id == CardId::STRIKE) {
            ++strikes;
            assert(CardDef::has_keyword(card, KEYWORD_ETHEREAL));
        } else if (card.id == CardId::DEFEND) {
            ++defends;
            assert(CardDef::has_keyword(card, KEYWORD_ETHEREAL));
        } else if (card.id == CardId::BASH) {
            assert(!CardDef::has_keyword(card, KEYWORD_ETHEREAL));
        }
    }
    assert(strikes > 0 && defends > 0);
    // Held at the end of the turn, a Strike is exhausted.
    DeckCard held(CardId::STRIKE);
    held.added_keywords |= KEYWORD_ETHEREAL;
    with.combat.hand.clear();
    with.combat.hand.push_back(held);
    with.combat.end_player_turn();
    bool exhausted = false;
    for (const DeckCard& card : with.combat.exhaust_pile) exhausted = exhausted || card.id == CardId::STRIKE;
    assert(exhausted);

    // Without the relic: no Strike or Defend is Ethereal.
    RunManager without = make_relic_run(CharacterId::IRONCLAD, {});
    for (const DeckCard& card : without.combat.draw_pile) assert(!CardDef::has_keyword(card, KEYWORD_ETHEREAL));
    std::cout << "[PASS] Ghost Seed: Strikes and Defends Ethereal in the fight" << std::endl;
}

void test_belt_buckle() {
    // With the relic and no potion: 2 additional Dexterity, lost as soon as a potion is held.
    RunManager with;
    with.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    with.add_relic("BELT_BUCKLE");
    with.potions.clear();
    enter_room_of_type(with, RoomType::MONSTER);
    assert(with.combat.player.get_status(StatusType::DEXTERITY) == 2);

    // Holding a potion at the start: no Dexterity; drinking it gives the Dexterity.
    RunManager held;
    held.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    held.add_relic("BELT_BUCKLE");
    held.potions.clear();
    held.potions.push_back("FIRE_POTION");
    enter_room_of_type(held, RoomType::MONSTER);
    assert(held.combat.player.get_status(StatusType::DEXTERITY) == 0);
    assert(held.use_potion(0, 0));
    assert(held.combat.player.get_status(StatusType::DEXTERITY) == 2);

    // Without the relic: nothing.
    RunManager without;
    without.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    without.potions.clear();
    enter_room_of_type(without, RoomType::MONSTER);
    assert(without.combat.player.get_status(StatusType::DEXTERITY) == 0);
    std::cout << "[PASS] Belt Buckle: 2 Dexterity while the potion belt is empty" << std::endl;
}

void test_reptile_trinket() {
    // With the relic: a potion drunk gives 3 Strength until the end of the turn.
    CombatEngine trinket = make_relic_combat(CharacterId::IRONCLAD, PowerId::REPTILE_TRINKET, 3);
    assert(trinket.player.get_status(StatusType::STRENGTH) == 0);
    assert(trinket.use_potion("BLOCK_POTION"));
    assert(trinket.player.get_status(StatusType::STRENGTH) == 3);
    assert(trinket.use_potion("BLOCK_POTION"));
    assert(trinket.player.get_status(StatusType::STRENGTH) == 6);
    end_turn_attacked(trinket, 11);
    assert(trinket.player.get_status(StatusType::STRENGTH) == 0);

    // Without the relic: no Strength.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, {});
    assert(plain.use_potion("BLOCK_POTION"));
    assert(plain.player.get_status(StatusType::STRENGTH) == 0);

    std::cout << "[PASS] Reptile Trinket: 3 Strength this turn per potion used" << std::endl;
}

static void test_pen_nib() {
    // With the relic, the 10th Attack deals double damage and the count starts over.
    CombatEngine nib = make_relic_combat(CharacterId::IRONCLAD, PowerId::PEN_NIB, 1);
    nib.enemies[0].hp = 200;
    for (int i = 0; i < 9; ++i) play(nib, CardId::STRIKE);
    assert(nib.enemies[0].hp == 200 - 9 * 6);
    play(nib, CardId::STRIKE);
    assert(nib.enemies[0].hp == 200 - 9 * 6 - 12 && nib.power_counter(PowerId::PEN_NIB) == 0);
    play(nib, CardId::STRIKE);
    assert(nib.enemies[0].hp == 200 - 9 * 6 - 12 - 6);

    // Without it, the 10th Attack is an ordinary one.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, PowerId::NUNCHAKU, 1);
    plain.enemies[0].hp = 200;
    for (int i = 0; i < 10; ++i) play(plain, CardId::STRIKE);
    assert(plain.enemies[0].hp == 200 - 10 * 6);
    std::cout << "[PASS] Pen Nib doubles every 10th Attack" << std::endl;
}

void test_strike_dummy() {
    // With the relic, every card containing "Strike" deals 3 more damage per hit; other cards do not.
    CombatEngine dummy = make_relic_combat(CharacterId::IRONCLAD, PowerId::STRIKE_DUMMY, 3);
    dummy.enemies[0].hp = 200;
    play(dummy, CardId::STRIKE);
    assert(dummy.enemies[0].hp == 200 - 9);
    play(dummy, CardId::TWIN_STRIKE);
    assert(dummy.enemies[0].hp == 200 - 9 - 2 * 8);
    play(dummy, CardId::BASH);
    assert(dummy.enemies[0].hp == 200 - 9 - 2 * 8 - 8);

    // Without it, a Strike deals its ordinary damage.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, {});
    plain.enemies[0].hp = 200;
    play(plain, CardId::STRIKE);
    assert(plain.enemies[0].hp == 200 - 6);
    std::cout << "[PASS] Strike Dummy: cards containing Strike deal 3 more damage" << std::endl;
}

void test_miniature_cannon() {
    // With the relic, an upgraded Attack deals 3 more damage per hit (Twin Strike hits twice); an ordinary one does not.
    CombatEngine cannon = make_relic_combat(CharacterId::IRONCLAD, PowerId::MINIATURE_CANNON, 3);
    cannon.enemies[0].hp = 200;
    play(cannon, DeckCard(CardId::STRIKE, true));
    const int upgraded_hit = 200 - cannon.enemies[0].hp;

    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, {});
    plain.enemies[0].hp = 200;
    play(plain, DeckCard(CardId::STRIKE, true));
    assert(200 - plain.enemies[0].hp + 3 == upgraded_hit);

    cannon.enemies[0].hp = 200;
    play(cannon, DeckCard(CardId::TWIN_STRIKE, true));
    const int twin_with = 200 - cannon.enemies[0].hp;
    plain.enemies[0].hp = 200;
    play(plain, DeckCard(CardId::TWIN_STRIKE, true));
    assert(twin_with == 200 - plain.enemies[0].hp + 2 * 3);

    cannon.enemies[0].hp = 200;
    play(cannon, CardId::STRIKE);
    assert(cannon.enemies[0].hp == 200 - 6);
    std::cout << "[PASS] Miniature Cannon: upgraded Attacks deal 3 more damage per hit" << std::endl;
}

void test_white_beast_statue() {
    // With the statue, every combat reward holds a potion; without it, some do not.
    uint32_t plain_without = 0;
    for (uint32_t seed = 1; seed <= 50; ++seed) {
        RunManager with;
        with.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
        with.add_relic("WHITE_BEAST_STATUE");
        enter_room_of_type(with, RoomType::MONSTER);
        with.generate_rewards();
        assert(!with.potion_reward.empty());

        RunManager plain;
        plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
        enter_room_of_type(plain, RoomType::MONSTER);
        plain.generate_rewards();
        if (plain.potion_reward.empty()) ++plain_without;
    }
    assert(plain_without > 0);
    std::cout << "[PASS] White Beast Statue: a potion in every combat reward, not always without it" << std::endl;
}

void test_unsettling_lamp() {
    // With the relic: the first card of the fight that debuffs an enemy doubles its debuffs, a card without one does not count.
    CombatEngine lamp = make_relic_combat(CharacterId::IRONCLAD, PowerId::UNSETTLING_LAMP, 1);
    lamp.enemies[0].hp = 200;
    play(lamp, CardId::STRIKE);
    assert(lamp.enemies[0].get_status(StatusType::VULNERABLE) == 0 && lamp.power_counter(PowerId::UNSETTLING_LAMP) == 0);
    play(lamp, CardId::BASH);
    assert(lamp.enemies[0].get_status(StatusType::VULNERABLE) == 4);
    // Only the first one: the next debuff is the plain one.
    play(lamp, CardId::BASH);
    assert(lamp.enemies[0].get_status(StatusType::VULNERABLE) == 6);

    // Without the relic: 2 Vulnerable each time.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, PowerId::NUNCHAKU, 1);
    plain.enemies[0].hp = 200;
    play(plain, CardId::BASH);
    play(plain, CardId::BASH);
    assert(plain.enemies[0].get_status(StatusType::VULNERABLE) == 4);
    std::cout << "[PASS] Unsettling Lamp: the first debuffing card of the fight has its debuffs doubled" << std::endl;
}

void test_daughter_of_the_wind() {
    // With the relic: each Attack played gives 1 Block; a Skill does not.
    RunManager wind = make_relic_run(CharacterId::IRONCLAD, { "DAUGHTER_OF_THE_WIND" });
    int16_t start = wind.combat.player.block;
    play(wind.combat, CardId::STRIKE);
    assert(wind.combat.player.block == start + 1);
    play(wind.combat, CardId::DEFEND);
    assert(wind.combat.player.block == start + 1 + 5);
    play(wind.combat, CardId::STRIKE);
    assert(wind.combat.player.block == start + 1 + 5 + 1);

    // Without the relic: a Strike gives no Block.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    int16_t plain_start = plain.combat.player.block;
    play(plain.combat, CardId::STRIKE);
    assert(!plain.has_relic("DAUGHTER_OF_THE_WIND"));
    assert(plain.combat.player.block == plain_start);
    std::cout << "[PASS] Daughter of the Wind: 1 Block per Attack played" << std::endl;
}

void test_bowler_hat() {
    // With the relic: the fight's gold reward is 25 % bigger, and that gold reaches the purse once the rewards are taken.
    // Without it: the plain reward.
    RunManager with;
    with.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    with.add_relic("BOWLER_HAT");
    enter_room_of_type(with, RoomType::MONSTER);
    with.generate_rewards();

    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(plain, RoomType::MONSTER);
    plain.generate_rewards();

    assert(plain.gold_reward > 0 && with.gold_reward == plain.gold_reward);
    const int16_t with_before = with.gold, plain_before = plain.gold;
    const int16_t reward = plain.gold_reward;
    with.skip_card_rewards(); // no card taken: the rewards are over, the gold is paid
    plain.skip_card_rewards();
    assert(plain.gold == plain_before + reward);
    assert(with.gold == with_before + reward + reward / 4);
    std::cout << "[PASS] Bowler Hat: 25 % more gold from fight rewards" << std::endl;
}

void test_cauldron() {
    // Obtained: 5 random potions are brewed into the belt.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 463);
    run.add_potion_slots(5);
    run.potions.clear();
    run.add_relic("CAULDRON");
    assert(run.has_relic("CAULDRON"));
    assert(run.potions.size() == 5);

    // Without the relic: no potion is brewed.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 463);
    plain.add_potion_slots(5);
    plain.potions.clear();
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("CAULDRON"));
    assert(plain.potions.empty());
    std::cout << "[PASS] Cauldron: 5 random potions brewed on pickup" << std::endl;
}

void test_pear() {
    // Obtained: 10 more maximum HP, and the current HP follow.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 468);
    run.hp = 50;
    int16_t max_hp = run.max_hp;
    run.add_relic("PEAR");
    assert(run.has_relic("PEAR"));
    assert(run.max_hp == max_hp + 10 && run.hp == 60);

    // Without the relic: the maximum HP is left alone.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 468);
    plain.hp = 50;
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("PEAR"));
    assert(plain.max_hp == max_hp && plain.hp == 50);
    std::cout << "[PASS] Pear: 10 more max HP on pickup" << std::endl;
}

void test_bookmark() {
    // With the relic: the Retained card in hand costs 1 less at the end of the turn, until it is played.
    CombatEngine combat = make_combat(CharacterId::IRONCLAD);
    combat.add_power(PowerId::BOOKMARK, 1);
    combat.hand.clear();
    DeckCard kept(CardId::BASH);
    kept.added_keywords |= KEYWORD_RETAIN;
    combat.hand.push_back(kept);
    combat.hand.push_back(DeckCard(CardId::STRIKE));
    assert(combat.effective_cost(combat.hand[0]) == 2);
    combat.end_player_turn();
    int discounted = 0;
    for (size_t i = 0; i < combat.hand.size(); ++i) {
        if (combat.hand[i].bookmark_discount > 0) {
            ++discounted;
            assert(combat.hand[i].id == CardId::BASH && combat.effective_cost(combat.hand[i]) == 1);
        }
    }
    assert(discounted == 1);

    // Without the relic: the Retained card keeps its cost.
    CombatEngine plain = make_combat(CharacterId::IRONCLAD);
    plain.hand.clear();
    plain.hand.push_back(kept);
    plain.end_player_turn();
    for (size_t i = 0; i < plain.hand.size(); ++i) assert(plain.hand[i].bookmark_discount == 0);
    std::cout << "[PASS] Bookmark: a Retained card costs 1 less until played" << std::endl;
}

void test_tungsten_rod() {
    // With the relic: each HP loss is 1 smaller, Block absorbs the hit first, and a 1 HP loss is cancelled.
    RunManager with;
    with.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    with.add_relic("TUNGSTEN_ROD");
    with.potions.clear();
    enter_room_of_type(with, RoomType::MONSTER);
    CombatEngine& tungsten = with.combat;
    tungsten.player.hp = 50;
    tungsten.player.block = 0;
    tungsten.lose_hp(5);
    assert(tungsten.player.hp == 46);
    tungsten.lose_hp(1);
    assert(tungsten.player.hp == 46);
    tungsten.player.block = 3;
    tungsten.damage_player(5); // 3 blocked, 2 unblocked: 1 HP lost
    assert(tungsten.player.hp == 45 && tungsten.player.block == 0);

    // Without the relic: every HP loss is whole.
    RunManager without;
    without.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    without.potions.clear();
    enter_room_of_type(without, RoomType::MONSTER);
    without.combat.player.hp = 50;
    without.combat.player.block = 0;
    without.combat.lose_hp(5);
    assert(without.combat.player.hp == 45);
    without.combat.lose_hp(1);
    assert(without.combat.player.hp == 44);
    std::cout << "[PASS] Tungsten Rod: 1 less HP lost each time" << std::endl;
}

void test_lees_waffle() {
    // Obtained: 7 more maximum HP, and all of the HP is restored.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 455);
    run.hp = 20;
    int16_t max_hp = run.max_hp;
    run.add_relic("LEES_WAFFLE");
    assert(run.has_relic("LEES_WAFFLE"));
    assert(run.max_hp == max_hp + 7 && run.hp == max_hp + 7);

    // Without the relic: the maximum HP and the current HP stay as they are.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 455);
    plain.hp = 20;
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("LEES_WAFFLE"));
    assert(plain.max_hp == max_hp && plain.hp == 20);
    std::cout << "[PASS] Lee's Waffle: +7 Max HP and full heal on pickup" << std::endl;
}

void test_mango() {
    // Obtained: 14 more maximum HP, and the current HP follow.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 457);
    run.hp = 50;
    int16_t max_hp = run.max_hp;
    run.add_relic("MANGO");
    assert(run.has_relic("MANGO"));
    assert(run.max_hp == max_hp + 14 && run.hp == 64);

    // Without the relic, nothing raises the maximum HP.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 457);
    plain.hp = 50;
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("MANGO"));
    assert(plain.max_hp == max_hp && plain.hp == 50);
    std::cout << "[PASS] Mango: 14 more max HP on pickup" << std::endl;
}

void test_fake_lees_waffle() {
    // Obtained: heals 10% of the max HP, capped at the max HP.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 512);
    int16_t max_hp = run.max_hp;
    run.hp = 20;
    run.add_relic("FAKE_LEES_WAFFLE");
    assert(run.has_relic("FAKE_LEES_WAFFLE"));
    assert(run.hp == 20 + max_hp * 10 / 100);
    assert(run.max_hp == max_hp);

    RunManager near_full;
    near_full.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 512);
    near_full.hp = static_cast<int16_t>(near_full.max_hp - 1);
    near_full.add_relic("FAKE_LEES_WAFFLE");
    assert(near_full.hp == near_full.max_hp);

    // Without the relic, the HP stays as it is.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 512);
    plain.hp = 20;
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("FAKE_LEES_WAFFLE"));
    assert(plain.hp == 20);
    std::cout << "[PASS] Fake Lee's Waffle: heals 10% of the max HP on pickup" << std::endl;
}

void test_fake_mango() {
    // Event relic, obtained: 3 more maximum HP, and the current HP follow.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 514);
    run.hp = 50;
    int16_t max_hp = run.max_hp;
    run.add_relic("FAKE_MANGO");
    assert(run.has_relic("FAKE_MANGO"));
    assert(run.max_hp == max_hp + 3 && run.hp == 53);

    // Without the relic, nothing raises the maximum HP.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 514);
    plain.hp = 50;
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("FAKE_MANGO"));
    assert(plain.max_hp == max_hp && plain.hp == 50);
    std::cout << "[PASS] Fake Mango: 3 more max HP on pickup" << std::endl;
}

void test_fake_anchor() {
    // Each combat starts with 4 Block, as with Anchor, and none without the relic.
    RunManager run = make_relic_run(CharacterId::IRONCLAD, { "FAKE_ANCHOR" });
    assert(run.has_relic("FAKE_ANCHOR"));
    assert(run.phase == RunPhase::IN_COMBAT && run.combat.player.block == 4);
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, { "ANCHOR" });
    assert(!plain.has_relic("FAKE_ANCHOR"));
    assert(plain.combat.player.block == 10);
    RunManager none = make_relic_run(CharacterId::IRONCLAD, {});
    assert(none.combat.player.block == 0);
    std::cout << "[PASS] Fake Anchor: starts each combat with 4 Block" << std::endl;
}

void test_old_coin() {
    // Obtained: 300 gold, paid once.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 466);
    int16_t gold = run.gold;
    run.add_relic("OLD_COIN");
    assert(run.has_relic("OLD_COIN"));
    assert(run.gold == gold + 300);

    // Without the relic, the gold stays where it is.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 466);
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("OLD_COIN") && plain.gold == gold);
    std::cout << "[PASS] Old Coin: +300 gold when obtained" << std::endl;
}

void test_lucky_fysh() {
    // With the relic: each card added to the deck, by any source, gains 15 gold.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 484);
    run.add_relic("LUCKY_FYSH");
    assert(run.has_relic("LUCKY_FYSH"));
    int16_t gold = run.gold;
    run.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
    assert(run.gold == gold + 15);
    run.add_card_to_deck(DeckCard{ CardId::DEFEND, false });
    assert(run.gold == gold + 30);

    // Without the relic, adding a card leaves the gold where it is.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 484);
    plain.add_relic("ANCHOR");
    int16_t plain_gold = plain.gold;
    plain.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
    assert(!plain.has_relic("LUCKY_FYSH") && plain.gold == plain_gold);
    std::cout << "[PASS] Lucky Fysh: +15 gold each time a card joins the deck" << std::endl;
}

void test_molten_egg() {
    // With the relic, every Attack added to the deck is upgraded; other cards are left as they are.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 493);
    run.add_relic("MOLTEN_EGG");
    assert(run.has_relic("MOLTEN_EGG"));
    run.add_card_to_deck(DeckCard{ CardId::BASH, false });
    assert(run.master_deck[run.master_deck.size() - 1].upgraded);
    run.add_card_to_deck(DeckCard{ CardId::DEFEND, false });
    assert(!run.master_deck[run.master_deck.size() - 1].upgraded);

    // Without the relic, an Attack added to the deck stays as it is.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 493);
    plain.add_relic("ANCHOR");
    plain.add_card_to_deck(DeckCard{ CardId::BASH, false });
    assert(!plain.has_relic("MOLTEN_EGG") && !plain.master_deck[plain.master_deck.size() - 1].upgraded);
    std::cout << "[PASS] Molten Egg: upgrades an Attack added to the deck" << std::endl;
}

void test_whetstone() {
    // Obtained: two random Attacks of the deck are upgraded, never more than the deck holds.
    auto upgraded_attacks = [](const RunManager& r) {
        int n = 0;
        for (size_t i = 0; i < r.master_deck.size(); ++i) {
            if (r.master_deck[i].upgraded && CardDef::get(r.master_deck[i].id).type == CardType::ATTACK) ++n;
        }
        return n;
    };
    auto attacks = [](const RunManager& r) {
        int n = 0;
        for (size_t i = 0; i < r.master_deck.size(); ++i) {
            if (CardDef::get(r.master_deck[i].id).type == CardType::ATTACK) ++n;
        }
        return n;
    };
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 465);
    int before = upgraded_attacks(run);
    run.add_relic("WHETSTONE");
    assert(run.has_relic("WHETSTONE"));
    assert(upgraded_attacks(run) == std::min(before + 2, attacks(run)));

    // Without the relic, no Attack is upgraded.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 465);
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("WHETSTONE") && upgraded_attacks(plain) == before);
    std::cout << "[PASS] Whetstone: upgrades 2 random Attacks when obtained" << std::endl;
}

void test_war_paint() {
    // Obtained: two random Skills of the deck are upgraded, never an Attack.
    auto upgraded_of_type = [](const RunManager& r, CardType type) {
        int n = 0;
        for (size_t i = 0; i < r.master_deck.size(); ++i) {
            if (r.master_deck[i].upgraded && CardDef::get(r.master_deck[i].id).type == type) ++n;
        }
        return n;
    };
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 464);
    int skills_before = upgraded_of_type(run, CardType::SKILL);
    int attacks_before = upgraded_of_type(run, CardType::ATTACK);
    run.add_relic("WAR_PAINT");
    assert(run.has_relic("WAR_PAINT"));
    assert(upgraded_of_type(run, CardType::SKILL) == skills_before + 2);
    assert(upgraded_of_type(run, CardType::ATTACK) == attacks_before);

    // Without the relic, no Skill is upgraded.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 464);
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("WAR_PAINT") && upgraded_of_type(plain, CardType::SKILL) == skills_before);
    std::cout << "[PASS] War Paint: upgrades 2 random Skills when obtained" << std::endl;
}

void test_toxic_egg() {
    // With the relic: a Skill added to the Deck comes in upgraded; an Attack does not.
    RunManager with;
    with.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 495);
    with.add_relic("TOXIC_EGG");
    with.add_card_to_deck(DeckCard{ CardId::DEFEND, false });
    with.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
    assert(with.master_deck.back().id == CardId::STRIKE && !with.master_deck.back().upgraded);
    assert(with.master_deck[with.master_deck.size() - 2].id == CardId::DEFEND);
    assert(with.master_deck[with.master_deck.size() - 2].upgraded);

    // Without the relic: the Skill is added as it is.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 495);
    plain.add_relic("ANCHOR");
    plain.add_card_to_deck(DeckCard{ CardId::DEFEND, false });
    assert(!plain.has_relic("TOXIC_EGG") && !plain.master_deck.back().upgraded);
    std::cout << "[PASS] Toxic Egg: a Skill added to the Deck is upgraded" << std::endl;
}

void test_dragon_fruit() {
    // With the relic: each gain of Gold raises the Max HP by 1, a gain of 0 changes nothing.
    RunManager with;
    with.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    with.add_relic("DRAGON_FRUIT");
    int16_t max_hp = with.max_hp;
    with.gain_gold(10);
    assert(with.max_hp == max_hp + 1);
    with.gain_gold(0);
    assert(with.max_hp == max_hp + 1);
    with.gain_gold(25);
    assert(with.max_hp == max_hp + 2);

    // Without the relic: the Max HP does not move with the gold.
    RunManager without;
    without.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    int16_t plain_max_hp = without.max_hp;
    without.gain_gold(10);
    assert(!without.has_relic("DRAGON_FRUIT") && without.max_hp == plain_max_hp);
    std::cout << "[PASS] Dragon Fruit: +1 Max HP with each gain of Gold" << std::endl;
}

void test_amethyst_aubergine() {
    // With the relic: every fight reward holds 15 more Gold than the same fight without it.
    for (uint32_t seed = 1; seed <= 20; ++seed) {
        RunManager with;
        with.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
        with.add_relic("AMETHYST_AUBERGINE");
        enter_room_of_type(with, RoomType::MONSTER);
        with.generate_rewards();

        RunManager plain;
        plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
        enter_room_of_type(plain, RoomType::MONSTER);
        plain.generate_rewards();
        assert(with.gold_reward == plain.gold_reward + 15);
    }
    std::cout << "[PASS] Amethyst Aubergine: 15 more Gold in each fight reward" << std::endl;
}

void test_book_of_five_rings() {
    // Obtained: every 5th card added to the Deck heals 20 HP, the first four do not.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 478);
    run.hp = 30;
    run.add_relic("BOOK_OF_FIVE_RINGS");
    assert(run.has_relic("BOOK_OF_FIVE_RINGS"));
    for (int card = 1; card <= 4; ++card) run.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
    assert(run.hp == 30);
    run.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
    assert(run.hp == 50);
    for (int card = 1; card <= 4; ++card) run.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
    assert(run.hp == 50);
    for (int card = 1; card <= 5; ++card) run.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
    assert(run.hp == 70);

    // Without the relic, adding cards never heals.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 478);
    plain.hp = 30;
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("BOOK_OF_FIVE_RINGS"));
    for (int card = 1; card <= 10; ++card) plain.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
    assert(plain.hp == 30);
    std::cout << "[PASS] Book of Five Rings: heals 20 HP on every 5th card added to the Deck" << std::endl;
}

void test_ring_of_the_drake() {
    // With the relic: 2 more cards drawn at the start of each of the first 3 turns, then the usual 5.
    RunManager ring = make_relic_run(CharacterId::IRONCLAD, { "RING_OF_THE_DRAKE" });
    assert(ring.combat.hand.size() == 7);
    end_turn_idle(ring.combat);
    assert(ring.combat.hand.size() == 7);
    end_turn_idle(ring.combat);
    assert(ring.combat.hand.size() == 7);
    end_turn_idle(ring.combat);
    assert(ring.combat.hand.size() == 5);

    // Without the relic: the usual 5 cards on every turn.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(plain.combat.hand.size() == 5);
    end_turn_idle(plain.combat);
    assert(plain.combat.hand.size() == 5);
    std::cout << "[PASS] Ring of the Drake: 2 more cards drawn on each of the first 3 turns" << std::endl;
}

void test_venerable_tea_set() {
    // With the relic: each Rest Site entered gives 2 more energy to the next combat, once.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("VENERABLE_TEA_SET");
    enter_room_of_type(run, RoomType::REST_SITE);
    enter_room_of_type(run, RoomType::REST_SITE);
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.combat.player.energy == 7);
    assert(run.relic_counter("VENERABLE_TEA_SET") == 0);
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.combat.player.energy == 3);

    // Without the relic: the usual 3 energy after a Rest Site.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(plain, RoomType::REST_SITE);
    enter_room_of_type(plain, RoomType::MONSTER);
    assert(!plain.has_relic("VENERABLE_TEA_SET"));
    assert(plain.combat.player.energy == 3);
    std::cout << "[PASS] Venerable Tea Set: 2 energy per Rest Site entered, for the next combat" << std::endl;
}

void test_fake_venerable_tea_set() {
    // With the relic: each Rest Site entered gives 1 more energy to the next combat, once.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 779);
    run.add_relic("FAKE_VENERABLE_TEA_SET");
    enter_room_of_type(run, RoomType::REST_SITE);
    enter_room_of_type(run, RoomType::REST_SITE);
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.combat.player.energy == 5);
    assert(run.relic_counter("FAKE_VENERABLE_TEA_SET") == 0);
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.combat.player.energy == 3);

    // Without the relic: the usual 3 energy after a Rest Site.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 779);
    enter_room_of_type(plain, RoomType::REST_SITE);
    enter_room_of_type(plain, RoomType::MONSTER);
    assert(!plain.has_relic("FAKE_VENERABLE_TEA_SET"));
    assert(plain.combat.player.energy == 3);
    std::cout << "[PASS] Venerable Tea Set (event): 1 energy per Rest Site entered, for the next combat" << std::endl;
}

void test_divine_destiny() {
    // With the relic: 7 Stars at the start of each combat.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 778);
    run.add_relic("DIVINE_DESTINY");
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.combat.player.sovereign_stars == 7);

    // Without the relic: no Stars at the start of the combat.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 778);
    assert(!plain.has_relic("DIVINE_DESTINY"));
    enter_room_of_type(plain, RoomType::MONSTER);
    assert(plain.combat.player.sovereign_stars == 0);
    std::cout << "[PASS] Divine Destiny (event): 7 Stars at the start of each combat" << std::endl;
}

void test_bone_tea() {
    // With the relic: the starting hand of the next combat is upgraded, once.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("BONE_TEA");
    assert(run.relic_counter("BONE_TEA") == 1);
    enter_room_of_type(run, RoomType::MONSTER);
    assert(!run.combat.hand.empty());
    for (const DeckCard& card : run.combat.hand) assert(card.upgraded);
    assert(run.relic_counter("BONE_TEA") == 0);
    run.proceed_after_combat();
    enter_room_of_type(run, RoomType::MONSTER);
    bool any_upgraded = false;
    for (const DeckCard& card : run.combat.hand) any_upgraded = any_upgraded || card.upgraded;
    assert(!any_upgraded);

    // Without the relic: the starting hand stays as it is.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    enter_room_of_type(plain, RoomType::MONSTER);
    assert(!plain.has_relic("BONE_TEA"));
    for (const DeckCard& card : plain.combat.hand) assert(!card.upgraded);
    std::cout << "[PASS] Bone Tea: the starting hand of the next combat is upgraded" << std::endl;
}

void test_lava_lamp() {
    // At the end of a fight without HP lost, every card reward of it is upgraded; with HP lost, none is.
    auto all_upgraded = [](const RunManager& r) {
        if (r.current_card_rewards.empty()) return false;
        for (const DeckCard& card : r.current_card_rewards) {
            if (!card.upgraded) return false;
        }
        return true;
    };
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 476);
    run.add_relic("LAVA_LAMP");
    enter_room_of_type(run, RoomType::MONSTER);
    run.combat.state = CombatState::VICTORY;
    run.proceed_after_combat();
    assert(run.phase == RunPhase::REWARD_SCREEN && all_upgraded(run));
    run.skip_card_rewards();

    RunManager hurt;
    hurt.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 476);
    hurt.add_relic("LAVA_LAMP");
    enter_room_of_type(hurt, RoomType::MONSTER);
    hurt.combat.counters.combat_hp_losses = 1;
    hurt.combat.state = CombatState::VICTORY;
    hurt.proceed_after_combat();
    assert(hurt.phase == RunPhase::REWARD_SCREEN && !hurt.current_card_rewards.empty());
    for (const DeckCard& card : hurt.current_card_rewards) assert(!card.upgraded);
    hurt.skip_card_rewards();

    // Without the relic, a fight without HP lost gives no upgraded card.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 476);
    plain.add_relic("ANCHOR");
    enter_room_of_type(plain, RoomType::MONSTER);
    plain.combat.state = CombatState::VICTORY;
    plain.proceed_after_combat();
    assert(plain.phase == RunPhase::REWARD_SCREEN && !plain.current_card_rewards.empty());
    for (const DeckCard& card : plain.current_card_rewards) assert(!card.upgraded);
    std::cout << "[PASS] Lava Lamp: card rewards upgraded after a fight without HP lost" << std::endl;
}

void test_prayer_wheel() {
    // A normal fight gives a second set of cards once the first one is taken or skipped.
    RunManager run = make_relic_run(CharacterId::IRONCLAD, { "PRAYER_WHEEL" });
    run.proceed_after_combat();
    assert(run.phase == RunPhase::REWARD_SCREEN);
    run.pick_card_reward(0);
    assert(run.phase == RunPhase::REWARD_SCREEN && run.current_card_rewards.size() > 0);
    run.skip_card_rewards();
    assert(run.phase == RunPhase::MAP_NAVIGATION);

    // Without the relic, a normal fight gives one set only.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    plain.proceed_after_combat();
    plain.pick_card_reward(0);
    assert(plain.phase == RunPhase::MAP_NAVIGATION);

    // An elite fight is not a normal fight: one set only.
    RunManager elite = make_relic_run(CharacterId::IRONCLAD, { "PRAYER_WHEEL" });
    enter_room_of_type(elite, RoomType::ELITE);
    elite.proceed_after_combat();
    elite.pick_card_reward(0);
    assert(elite.phase == RunPhase::MAP_NAVIGATION);
    std::cout << "[PASS] Prayer Wheel: a normal fight gives an extra card reward" << std::endl;
}

void test_frozen_egg() {
    // With the relic: a Power added to the Deck is upgraded, another card is not.
    RunManager with = make_relic_run(CharacterId::IRONCLAD, { "FROZEN_EGG" });
    with.add_card_to_deck(DeckCard{ CardId::INFLAME, false });
    assert(with.master_deck.back().id == CardId::INFLAME && with.master_deck.back().upgraded);
    with.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
    assert(with.master_deck.back().id == CardId::STRIKE && !with.master_deck.back().upgraded);

    // Without the relic: the Power stays as it is.
    RunManager without = make_relic_run(CharacterId::IRONCLAD, {});
    without.add_card_to_deck(DeckCard{ CardId::INFLAME, false });
    assert(without.master_deck.back().id == CardId::INFLAME && !without.master_deck.back().upgraded);
    std::cout << "[PASS] Frozen Egg: a Power added to the Deck is upgraded" << std::endl;
}

void test_girya() {
    // Rest sites: each lift gives 1 Strength for the rest of the run, 3 times at most, and uses up the rest site.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.phase = RunPhase::CAMPFIRE;
    assert(!run.lift_at_campfire() && run.relic_counter("GIRYA") == 0 && run.phase == RunPhase::CAMPFIRE);
    run.add_relic("GIRYA");
    for (uint8_t lift = 1; lift <= 3; ++lift) {
        run.phase = RunPhase::CAMPFIRE;
        assert(run.lift_at_campfire() && run.relic_counter("GIRYA") == lift);
        assert(run.phase == RunPhase::MAP_NAVIGATION);
    }
    run.phase = RunPhase::CAMPFIRE;
    assert(!run.lift_at_campfire() && run.relic_counter("GIRYA") == 3 && run.phase == RunPhase::CAMPFIRE);

    // Every combat starts with the Strength lifted so far, and with none without the relic.
    RunManager fight;
    fight.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    fight.add_relic("GIRYA");
    assert(fight.set_relic_counter("GIRYA", 2));
    enter_room_of_type(fight, RoomType::MONSTER);
    assert(fight.phase == RunPhase::IN_COMBAT && fight.combat.player.get_status(StatusType::STRENGTH) == 2);
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(plain.combat.player.get_status(StatusType::STRENGTH) == 0);
    std::cout << "[PASS] Girya: +1 Strength per lift at a rest site, 3 times max, carried into each combat" << std::endl;
}

void test_petrified_toad() {
    // Each combat starts with a Potion-Shaped Rock in the belt; drinking it deals 15 damage.
    RunManager toad = make_relic_run(CharacterId::IRONCLAD, { "PETRIFIED_TOAD" });
    assert(toad.has_relic("PETRIFIED_TOAD"));
    assert(toad.potions.size() == 1 && toad.potions[0] == "POTION_SHAPED_ROCK");
    toad.combat.enemies[0].hp = 40;
    assert(toad.use_potion(0, 0));
    assert(toad.combat.enemies[0].hp == 25);
    assert(toad.potions.empty());

    // Without the relic, no potion is given at the start of the combat.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(!plain.has_relic("PETRIFIED_TOAD") && plain.potions.empty());
    std::cout << "[PASS] Petrified Toad: a Potion-Shaped Rock at the start of each combat" << std::endl;
}

void test_fake_happy_flower() {
    // Every 5 turns the turn starts with 1 more energy; nothing without the relic.
    RunManager with = make_relic_run(CharacterId::IRONCLAD, { "FAKE_HAPPY_FLOWER" });
    RunManager without = make_relic_run(CharacterId::IRONCLAD, {});
    for (int turn = 1; turn <= 5; ++turn) {
        int expected = turn == 5 ? with.combat.player.max_energy + 1 : with.combat.player.max_energy;
        assert(with.combat.player.energy == expected);
        assert(without.combat.player.energy == without.combat.player.max_energy);
        if (turn < 5) {
            end_turn_idle(with.combat);
            end_turn_idle(without.combat);
        }
    }
    std::cout << "[PASS] Fake Happy Flower: 1 energy every 5 turns" << std::endl;
}

void test_white_star() {
    // With the relic, an Elite offers one more set of Rare cards once its first set is taken or skipped.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 492);
    run.add_relic("WHITE_STAR");
    enter_room_of_type(run, RoomType::ELITE);
    run.proceed_after_combat();
    assert(run.phase == RunPhase::REWARD_SCREEN);
    run.pick_card_reward(0);
    assert(run.phase == RunPhase::REWARD_SCREEN);
    assert(!run.current_card_rewards.empty());
    for (DeckCard card : run.current_card_rewards) assert(card_meta(card.id).rarity == CardRarity::RARE);
    run.skip_card_rewards();
    assert(run.phase == RunPhase::MAP_NAVIGATION);

    // Without the relic, the Elite offers only its first set.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 492);
    plain.add_relic("ANCHOR");
    enter_room_of_type(plain, RoomType::ELITE);
    plain.proceed_after_combat();
    plain.pick_card_reward(0);
    assert(plain.phase == RunPhase::MAP_NAVIGATION);

    // With the relic, a Monster room gives no extra set.
    RunManager monster = make_relic_run(CharacterId::IRONCLAD, { "WHITE_STAR" });
    monster.proceed_after_combat();
    monster.pick_card_reward(0);
    assert(monster.phase == RunPhase::MAP_NAVIGATION);
    std::cout << "[PASS] White Star: an Elite offers one more set of Rare cards" << std::endl;
}

void test_chosen_cheese() {
    // With the relic: each fight won raises Max HP by 1.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("CHOSEN_CHEESE");
    int16_t max_hp = run.max_hp;
    enter_room_of_type(run, RoomType::MONSTER);
    run.combat.state = CombatState::VICTORY;
    run.proceed_after_combat();
    assert(run.max_hp == max_hp + 1);
    enter_room_of_type(run, RoomType::MONSTER);
    run.combat.state = CombatState::VICTORY;
    run.proceed_after_combat();
    assert(run.max_hp == max_hp + 2);

    // Without the relic: a fight leaves Max HP alone.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    assert(!plain.has_relic("CHOSEN_CHEESE"));
    enter_room_of_type(plain, RoomType::MONSTER);
    plain.combat.state = CombatState::VICTORY;
    plain.proceed_after_combat();
    assert(plain.max_hp == max_hp);
    std::cout << "[PASS] Chosen Cheese: +1 Max HP at the end of each combat" << std::endl;
}

void test_chemical_x() {
    // With the relic: an X card gets 2 more X for the same energy spent (Skewer: a hit of 8 per X).
    RunManager run = make_relic_run(CharacterId::SILENT, { "CHEMICAL_X" });
    assert(run.combat.has_power(PowerId::CHEMICAL_X));
    CombatEngine with = make_relic_combat(CharacterId::SILENT, PowerId::CHEMICAL_X, 2);
    with.enemies[0].hp = 100;
    with.hand.clear();
    with.hand.push_back(DeckCard(CardId::SKEWER, false));
    with.player.energy = 3;
    assert(with.play_card(0, 0));
    assert(with.enemies[0].hp == 100 - 5 * 8 && with.player.energy == 0);
    assert(with.counters.energy_spent == 3 && with.counters.hits_dealt == 5);

    // Played with no energy, the X is still raised: 2 hits.
    with.hand.clear();
    with.hand.push_back(DeckCard(CardId::SKEWER, false));
    with.player.energy = 0;
    assert(with.play_card(0, 0));
    assert(with.enemies[0].hp == 100 - 5 * 8 - 2 * 8);

    // Without the relic: 3 hits for 3 energy, and no X card is changed by it.
    CombatEngine plain = make_relic_combat(CharacterId::SILENT, PowerId::NUNCHAKU, 1);
    plain.enemies[0].hp = 100;
    plain.hand.clear();
    plain.hand.push_back(DeckCard(CardId::SKEWER, false));
    plain.player.energy = 3;
    assert(plain.play_card(0, 0));
    assert(plain.enemies[0].hp == 100 - 3 * 8 && plain.counters.hits_dealt == 3);
    std::cout << "[PASS] Chemical X: X cards get 2 more X, the energy spent is unchanged" << std::endl;
}

void test_fragrant_mushroom() {
    // Obtained: 15 HP are lost (never below 1), then 2 random cards are upgraded.
    auto upgraded_count = [](const RunManager& r) {
        int n = 0;
        for (size_t i = 0; i < r.master_deck.size(); ++i) {
            if (r.master_deck[i].upgraded) ++n;
        }
        return n;
    };
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 511);
    run.hp = 50;
    int upgraded_before = upgraded_count(run);
    run.add_relic("FRAGRANT_MUSHROOM");
    assert(run.has_relic("FRAGRANT_MUSHROOM"));
    assert(run.hp == 35);
    assert(run.max_hp == 80);
    assert(upgraded_count(run) == upgraded_before + 2);

    // Losing HP never takes the hero below 1.
    RunManager low;
    low.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 511);
    low.hp = 10;
    low.add_relic("FRAGRANT_MUSHROOM");
    assert(low.hp == 1);

    // Without the relic, no HP is lost and no card is upgraded.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 511);
    plain.hp = 50;
    int plain_before = upgraded_count(plain);
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("FRAGRANT_MUSHROOM") && plain.hp == 50 && upgraded_count(plain) == plain_before);
    std::cout << "[PASS] Fragrant Mushroom: loses 15 HP and upgrades 2 random cards when obtained" << std::endl;
}

void test_darkstone_periapt() {
    // Obtained cards: each Curse that joins the deck raises the Max HP by 6, other cards do not.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 781);
    run.add_relic("DARKSTONE_PERIAPT");
    assert(run.has_relic("DARKSTONE_PERIAPT"));
    run.hp = 40;
    run.max_hp = 60;
    run.add_card_to_deck(DeckCard{ CardId::GREED, false });
    assert(run.max_hp == 66);
    assert(run.hp == 46);
    run.add_card_to_deck(DeckCard{ CardId::STRIKE, false });
    assert(run.max_hp == 66);

    // Without the relic, a Curse leaves the Max HP alone.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 781);
    plain.add_relic("ANCHOR");
    plain.max_hp = 60;
    plain.add_card_to_deck(DeckCard{ CardId::GREED, false });
    assert(!plain.has_relic("DARKSTONE_PERIAPT") && plain.max_hp == 60);

    std::cout << "[PASS] Darkstone Periapt: +6 Max HP for each Curse obtained" << std::endl;
}

void test_big_mushroom() {
    // Obtained: 20 more maximum HP, and the current HP follow.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 513);
    run.hp = 50;
    int16_t max_hp = run.max_hp;
    run.add_relic("BIG_MUSHROOM");
    assert(run.has_relic("BIG_MUSHROOM"));
    assert(run.max_hp == max_hp + 20 && run.hp == 70);

    // Start of the combat: 2 cards fewer in the first hand, not in the next ones.
    RunManager mushroom = make_relic_run(CharacterId::IRONCLAD, { "BIG_MUSHROOM" });
    assert(mushroom.combat.hand.size() == 3);
    end_turn_idle(mushroom.combat);
    assert(mushroom.combat.hand.size() == 5);

    // Without the relic, the first hand keeps its 5 cards and nothing raises the maximum HP.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, { "ANCHOR" });
    assert(plain.combat.hand.size() == 5);
    RunManager plain_max;
    plain_max.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 513);
    plain_max.add_relic("ANCHOR");
    assert(plain_max.max_hp == max_hp);
    std::cout << "[PASS] Big Mushroom: 20 more max HP on pickup, 2 cards fewer in the first hand" << std::endl;
}

void test_fake_orichalcum() {
    // With the relic, a turn ended without Block gives 3 Block: the 11 damage of the attack is cut to 8.
    RunManager run = make_relic_run(CharacterId::IRONCLAD, { "FAKE_ORICHALCUM" });
    int16_t hp_before = run.combat.player.hp;
    end_turn_attacked(run.combat, 11);
    assert(run.combat.player.hp == hp_before - (11 - 3));

    // With Block already, the relic gives nothing: the attack only meets the Block of the turn.
    RunManager blocked = make_relic_run(CharacterId::IRONCLAD, { "FAKE_ORICHALCUM" });
    hp_before = blocked.combat.player.hp;
    blocked.combat.player.block = 2;
    end_turn_attacked(blocked.combat, 11);
    assert(blocked.combat.player.hp == hp_before - (11 - 2));

    // Without the relic, the attack lands in full.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    hp_before = plain.combat.player.hp;
    end_turn_attacked(plain.combat, 11);
    assert(plain.combat.player.hp == hp_before - 11);
    std::cout << "[PASS] Orichalque ???: without Block at the end of the turn, gain 3 Block" << std::endl;
}

void test_maw_bank() {
    // With the relic, each floor climbed gives 12 Gold, until Gold is spent at the merchant.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.add_relic("MAW_BANK");
    assert(run.has_relic("MAW_BANK"));
    int16_t gold = run.gold;
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.gold == gold + 12);

    // Buying a card at the merchant ends the effect: the next floor gives nothing.
    run.gold = 999;
    run.open_shop();
    assert(run.buy_shop_card(0) && run.gold < 999);
    run.leave_shop();
    gold = run.gold;
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.gold == gold);

    // Removing a card at the merchant ends it too.
    RunManager remover;
    remover.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    remover.add_relic("MAW_BANK");
    remover.gold = 999;
    remover.open_shop();
    assert(remover.remove_card_at_shop(0) && remover.gold < 999);
    remover.leave_shop();
    gold = remover.gold;
    enter_room_of_type(remover, RoomType::MONSTER);
    assert(remover.gold == gold);

    // Without the relic, a floor gives no Gold.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.add_relic("ANCHOR");
    assert(!plain.has_relic("MAW_BANK"));
    gold = plain.gold;
    enter_room_of_type(plain, RoomType::MONSTER);
    assert(plain.gold == gold);
    std::cout << "[PASS] Maw Bank: 12 Gold on each floor climbed, until Gold is spent at the merchant" << std::endl;
}

void test_history_course() {
    // With the relic: the next turn starts by playing a copy of the last Attack played.
    CombatEngine history = make_relic_combat(CharacterId::IRONCLAD, PowerId::HISTORY_COURSE, 0);
    play(history, CardId::STRIKE, 0);
    int hp_after_play = history.enemies[0].hp;
    end_turn_idle(history);
    assert(history.state == CombatState::PLAYER_TURN);
    assert(history.enemies[0].hp == hp_after_play - 6);

    // A Skill played after the Attack does not replace it.
    CombatEngine skill = make_relic_combat(CharacterId::IRONCLAD, PowerId::HISTORY_COURSE, 0);
    play(skill, CardId::STRIKE, 0);
    play(skill, CardId::DEFEND);
    int hp_before = skill.enemies[0].hp;
    end_turn_idle(skill);
    assert(skill.enemies[0].hp == hp_before - 6);

    // Without the relic, nothing is played.
    CombatEngine plain = make_relic_combat(CharacterId::IRONCLAD, {});
    play(plain, CardId::STRIKE, 0);
    int plain_hp = plain.enemies[0].hp;
    end_turn_idle(plain);
    assert(plain.enemies[0].hp == plain_hp);

    std::cout << "[PASS] History Course: a copy of the last Attack played at the start of the turn" << std::endl;
}

void test_pollinous_core() {
    // With the relic, turn 4 draws 2 more cards than a plain turn; the other turns do not.
    RunManager run = make_relic_run(CharacterId::IRONCLAD, { "POLLINOUS_CORE" });
    CombatEngine& with = run.combat;
    assert(with.has_power(PowerId::POLLINOUS_CORE));
    for (int turn = 1; turn <= 3; ++turn) {
        assert(with.hand.size() == 5);
        end_turn_idle(with);
    }
    assert(with.hand.size() == 7);
    end_turn_idle(with);
    assert(with.hand.size() == 5);

    // Without the relic, turn 4 draws the usual hand.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, { "ANCHOR" });
    assert(!plain.combat.has_power(PowerId::POLLINOUS_CORE));
    for (int turn = 1; turn <= 3; ++turn) end_turn_idle(plain.combat);
    assert(plain.combat.hand.size() == 5);
    std::cout << "[PASS] Pollinous Core: every 4 turns, draw 2 more cards" << std::endl;
}

void test_black_blood() {
    // Black Blood: 12 HP after every fight, capped at the max HP.
    RunManager blood = make_relic_run(CharacterId::SILENT, { "BLACK_BLOOD" });
    blood.combat.player.hp = 30;
    blood.proceed_after_combat();
    assert(blood.hp == 42);
    RunManager near_full = make_relic_run(CharacterId::SILENT, { "BLACK_BLOOD" });
    near_full.combat.player.hp = near_full.max_hp - 5;
    near_full.proceed_after_combat();
    assert(near_full.hp == near_full.max_hp);
    // Without the relic: no heal.
    RunManager plain = make_relic_run(CharacterId::SILENT, {});
    plain.combat.player.hp = 30;
    plain.proceed_after_combat();
    assert(plain.hp == 30);
    std::cout << "[PASS] Black Blood" << std::endl;
}

void test_fake_blood_vial() {
    // With the relic: 1 HP back at the start of each fight, up to the maximum.
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    run.hp = 50;
    run.add_relic("FAKE_BLOOD_VIAL");
    assert(run.has_relic("FAKE_BLOOD_VIAL"));
    enter_room_of_type(run, RoomType::MONSTER);
    assert(run.combat.player.hp == 51);

    // Without the relic: no heal.
    RunManager plain;
    plain.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain.hp = 50;
    enter_room_of_type(plain, RoomType::MONSTER);
    assert(plain.combat.player.hp == 50);

    // Full HP stays at the maximum.
    RunManager full;
    full.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    full.add_relic("FAKE_BLOOD_VIAL");
    enter_room_of_type(full, RoomType::MONSTER);
    assert(full.combat.player.hp == full.max_hp);
    std::cout << "[PASS] Fake Blood Vial: heals 1 HP at the start of each combat" << std::endl;
}

void test_infused_core() {
    // At the start of each combat, 3 Lightning orbs are channeled; each deals 1 more damage, passive or evoked.
    auto total_hp = [](const CombatEngine& combat) {
        int sum = 0;
        for (const Enemy& foe : combat.enemies) sum += foe.hp;
        return sum;
    };
    RunManager core = make_relic_run(CharacterId::DEFECT, { "INFUSED_CORE" });
    assert(core.combat.player.orbs.size() == 3);
    for (const Orb& orb : core.combat.player.orbs) assert(orb == OrbType::LIGHTNING);

    int hp = total_hp(core.combat);
    core.combat.evoke_orb(0);
    assert(total_hp(core.combat) == hp - (LIGHTNING_EVOKE + 1));
    assert(core.combat.player.orbs.size() == 2);

    Enemy& first = core.combat.enemies[0];
    int16_t first_hp = first.hp;
    core.combat.trigger_orb_passive(0, &first);
    assert(first.hp == first_hp - (LIGHTNING_PASSIVE + 1));

    // Without the relic, only Cracked Core's Lightning orb is channeled at the start, and the evoke keeps its usual damage.
    RunManager plain = make_relic_run(CharacterId::DEFECT, {});
    assert(plain.combat.player.orbs.size() == 1);
    plain.combat.channel_orb(OrbType::LIGHTNING);
    int plain_hp = total_hp(plain.combat);
    plain.combat.evoke_orb(0);
    assert(total_hp(plain.combat) == plain_hp - LIGHTNING_EVOKE);
    std::cout << "[PASS] Infused Core: 3 Lightning orbs at the start of each combat, each 1 damage more" << std::endl;
}

void test_sword_of_jade() {
    // With the relic: 3 Strength at the start of the combat, so an Attack hits for 3 more.
    RunManager with = make_relic_run(CharacterId::IRONCLAD, { "SWORD_OF_JADE" });
    assert(with.combat.player.get_status(StatusType::STRENGTH) == 3);
    with.combat.enemies[0].hp = 100;
    with.combat.enemies[0].block = 0;
    play(with.combat, CardId::STRIKE);
    assert(with.combat.enemies[0].hp == 100 - (6 + 3));

    // Without the relic: no Strength at the start of the combat.
    RunManager plain = make_relic_run(CharacterId::IRONCLAD, {});
    assert(plain.combat.player.get_status(StatusType::STRENGTH) == 0);
    plain.combat.enemies[0].hp = 100;
    plain.combat.enemies[0].block = 0;
    play(plain.combat, CardId::STRIKE);
    assert(plain.combat.enemies[0].hp == 100 - 6);
    std::cout << "[PASS] Sword of Jade: 3 Strength at the start of each combat" << std::endl;
}

int main() {
  std::cout << "=== Relics on Combat Hooks ===" << std::endl;
    test_attack_relics();
    test_skill_and_power_relics();
    test_exhaust_and_creation_relics();
    test_turn_relics();
    test_paels_tears();
    test_hp_relics();
    test_poison_and_star_relics();
    test_combat_start_relics();
    test_paels_blood();
    test_relics_in_a_run();
    test_black_blood();
    test_very_hot_cocoa();
    test_strawberry();
    test_golden_compass();
    test_paels_tooth();
    test_membership_card();
    test_the_courier();
    test_yummy_cookie();
    test_delicate_frond();
    test_seal_of_gold();
    test_pumpkin_candle();
    test_wongos_mystery_ticket();
    test_ember_tea();
    test_paels_legion();
    test_golden_pearl();
    test_venerable_tea_set();
    test_fake_venerable_tea_set();
    test_divine_destiny();
    test_bone_tea();
    std::cout << "All relic hook tests passed." << std::endl;
    test_booming_conch();
    test_arcane_scroll();
    test_lead_paperweight();
    test_neows_torment();
    test_precise_scissors();
    test_new_leaf();
    test_small_capsule();
    test_nutritious_oyster();
    test_lava_rock();
    test_stone_humidifier();
    test_tiny_mailbox();
    test_dream_catcher();
    test_regal_pillow();
    test_eternal_feather();
    test_planisphere();
    test_meal_ticket();
    test_pomander();
    test_neows_talisman();
    test_large_capsule();
    test_astrolabe();
    test_neows_bones();
    test_empty_cage();
    test_pandoras_box();
    test_runic_pyramid();
    test_ectoplasm();
    test_philosophers_stone();
    test_dusty_tome();
    test_velvet_choker();
    test_glass_eye();
    test_prismatic_gem();
    test_radiant_pearl();
    test_sand_castle();
    test_paels_horn();
    test_paels_eye();
    test_cursed_pearl();
    test_calling_bell();
    test_war_paint();
    test_fragrant_mushroom();
    test_lava_lamp();
    test_petrified_toad();
    test_mr_struggles();
    test_belt_buckle();
    test_circlet();
    test_dingy_rug();
    test_beating_remnant();
    test_vambrace();
    test_ghost_seed();
    test_royal_poison();
    test_reptile_trinket();
    test_ringing_triangle();
    test_toolbox();
    test_pen_nib();
    test_strike_dummy();
    test_miniature_cannon();
    test_white_beast_statue();
    test_cauldron();
    test_lees_waffle();
    test_bowler_hat();
    test_unsettling_lamp();
    test_daughter_of_the_wind();
    test_bookmark();
    test_tungsten_rod();
    test_mango();
    test_fake_lees_waffle();
    test_fake_mango();
    test_fake_anchor();
    test_old_coin();
    test_pear();
    test_whetstone();
    test_dragon_fruit();
    test_toxic_egg();
    test_amethyst_aubergine();
    test_sword_of_jade();
    test_lucky_fysh();
    test_book_of_five_rings();
    test_prayer_wheel();
    test_molten_egg();
    test_frozen_egg();
    test_girya();
    test_shovel();
    test_white_star();
    test_chemical_x();
    test_ring_of_the_drake();
    test_darkstone_periapt();
    test_big_mushroom();
    test_fake_orichalcum();
    test_maw_bank();
    test_fake_happy_flower();
    test_history_course();
    test_pollinous_core();
    test_chosen_cheese();
    test_fake_blood_vial();
    test_infused_core();
    return 0;
}
