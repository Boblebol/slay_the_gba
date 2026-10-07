// Relics that act on the combat events: one case per relic, on hand-built situations.
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
        } else {
            assert(chest.max_hp == before);
        }
    }
    assert(found);
    std::cout << "[PASS] Strawberry: +7 Max HP when obtained, once, from a chest too" << std::endl;
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
    test_very_hot_cocoa();
    test_strawberry();
    test_paels_tooth();
    test_membership_card();
    test_paels_legion();
    std::cout << "All relic hook tests passed." << std::endl;
    return 0;
}
