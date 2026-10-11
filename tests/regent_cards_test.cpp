// Regent cards added on top of the existing mechanics: each one is played in its base and upgraded versions,
// and must be offered in the hero's card pool. Also the random card draws those cards rely on.
#include <algorithm>
#include <cassert>
#include <iostream>
#include <map>
#include <set>
#include <span>
#include "engine/combat.h"
#include "engine/run.h"

using namespace engine;

namespace {

constexpr CharacterId HEROES[] = { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT, CharacterId::REGENT,
                                   CharacterId::NECROBINDER };
constexpr CardPool HERO_POOLS[] = { CardPool::IRONCLAD, CardPool::SILENT, CardPool::DEFECT, CardPool::REGENT,
                                    CardPool::NECROBINDER };

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

void assert_in_pool(CharacterId hero, std::initializer_list<CardId> cards) {
    std::span<const CardId> pool = RunManager::card_reward_pool(hero);
    for (CardId id : cards) {
        assert(std::find(pool.begin(), pool.end(), id) != pool.end());
    }
}

int count_fitting(const CombatEngine& combat, const CardFilter& filter) {
    int count = 0;
    for (uint16_t i = 0; i < static_cast<uint16_t>(CardId::COUNT); ++i) count += combat.card_fits(static_cast<CardId>(i), filter);
    return count;
}

bool is_normal_rarity(CardRarity rarity) {
    return rarity == CardRarity::COMMON || rarity == CardRarity::UNCOMMON || rarity == CardRarity::RARE;
}

void test_card_meta() {
    // Rarity and pool of the reference, for cards of every kind.
    assert(card_meta(CardId::BASH).rarity == CardRarity::BASIC && card_meta(CardId::BASH).pool == CardPool::IRONCLAD);
    assert(card_meta(CardId::IRON_WAVE).rarity == CardRarity::COMMON && card_meta(CardId::IRON_WAVE).pool == CardPool::IRONCLAD);
    assert(card_meta(CardId::DEMON_FORM).rarity == CardRarity::RARE);
    assert(card_meta(CardId::MANIFEST_AUTHORITY).rarity == CardRarity::UNCOMMON &&
           card_meta(CardId::MANIFEST_AUTHORITY).pool == CardPool::REGENT);
    assert(card_meta(CardId::POISON_STAB).pool == CardPool::SILENT);
    assert(card_meta(CardId::STRIKE).rarity == CardRarity::BASIC && card_meta(CardId::STRIKE).pool == CardPool::NONE);
    // Status cards and tokens are not cards of the reference.
    for (CardId id : { CardId::NONE, CardId::DAZED, CardId::BURN, CardId::SHIV, CardId::SOUL, CardId::SOVEREIGN_BLADE }) {
        assert(card_meta(id).rarity == CardRarity::NONE && card_meta(id).pool == CardPool::NONE);
    }
    assert(card_meta(CardId::COUNT).rarity == CardRarity::NONE); // out of range: the blank card

    // Every card a hero is offered after a fight belongs to that hero's pool.
    for (size_t h = 0; h < 5; ++h) {
        for (CardId id : RunManager::card_reward_pool(HEROES[h])) assert(card_meta(id).pool == HERO_POOLS[h]);
    }
    std::cout << "[PASS] Card meta: rarity and pool of the reference for every engine card" << std::endl;
}

void test_random_card_by_pool() {
    for (size_t h = 0; h < 5; ++h) {
        CombatEngine combat = make_pack(HEROES[h]);
        // By default: the pool of the hero of the fight, Common to Rare.
        assert(count_fitting(combat, {}) > 40);
        std::set<CardId> seen;
        for (int i = 0; i < 600; ++i) {
            CardId id = combat.random_card();
            assert(id != CardId::NONE && id < CardId::COUNT);
            assert(card_meta(id).pool == HERO_POOLS[h] && is_normal_rarity(card_meta(id).rarity));
            seen.insert(id);
        }
        // Many different cards come out: the draw is not stuck on a few.
        assert(static_cast<int>(seen.size()) > count_fitting(combat, {}) * 3 / 4);

        // Another hero's pool, whoever fights.
        CardPool other = HERO_POOLS[(h + 1) % 5];
        for (int i = 0; i < 100; ++i) assert(card_meta(combat.random_card(other)).pool == other);
    }

    // Never a Basic or Ancient card, a Status card or a token, whatever the filter.
    CombatEngine ironclad = make_pack(CharacterId::IRONCLAD);
    for (CardId id : { CardId::NONE, CardId::STRIKE, CardId::DEFEND, CardId::BASH, CardId::BREAK, CardId::DAZED, CardId::SHIV,
                       CardId::COUNT }) {
        assert(!ironclad.card_fits(id, {}));
    }
    assert(ironclad.card_fits(CardId::IRON_WAVE, {}) && !ironclad.card_fits(CardId::IRON_WAVE, CardPool::SILENT));

    // A pool with no card in the engine yet gives nothing, and draws no number: the fight's sequence does not move.
    CombatEngine first = make_pack(CharacterId::REGENT);
    CombatEngine second = make_pack(CharacterId::REGENT);
    if (count_fitting(first, CardPool::COLORLESS) == 0) {
        assert(first.random_card(CardPool::COLORLESS) == CardId::NONE);
        assert(!first.create_random_card(CardPool::COLORLESS, Pile::HAND));
    }
    assert(first.random_card(CardPool::CURSE) == CardId::NONE || card_meta(first.random_card(CardPool::CURSE)).pool == CardPool::CURSE);
    if (count_fitting(first, CardPool::COLORLESS) == 0 && count_fitting(first, CardPool::CURSE) == 0) {
        for (int i = 0; i < 20; ++i) assert(first.random_card() == second.random_card());
    }
    std::cout << "[PASS] Random card: by pool (the hero's by default, another hero's, Colorless), only real cards" << std::endl;
}

void test_random_card_by_type_and_rarity() {
    CombatEngine combat = make_pack(CharacterId::IRONCLAD);
    for (CardType type : { CardType::ATTACK, CardType::SKILL, CardType::POWER }) {
        assert(count_fitting(combat, type) > 5);
        for (int i = 0; i < 200; ++i) {
            CardId id = combat.random_card(type);
            assert(CardDef::get(id).type == type && card_meta(id).pool == CardPool::IRONCLAD);
        }
    }
    for (CardRarity rarity : { CardRarity::COMMON, CardRarity::UNCOMMON, CardRarity::RARE }) {
        assert(count_fitting(combat, rarity) > 5);
        for (int i = 0; i < 200; ++i) assert(card_meta(combat.random_card(rarity)).rarity == rarity);
    }
    // Type and pool together: an Attack of the Silent, drawn by the Ironclad.
    for (int i = 0; i < 200; ++i) {
        CardId id = combat.random_card(CardFilter(CardPool::SILENT, CardType::ATTACK));
        assert(CardDef::get(id).type == CardType::ATTACK && card_meta(id).pool == CardPool::SILENT);
    }
    // Rarity and pool together.
    for (int i = 0; i < 200; ++i) {
        CardId id = combat.random_card(CardFilter(CardPool::DEFECT, CardRarity::RARE));
        assert(card_meta(id).rarity == CardRarity::RARE && card_meta(id).pool == CardPool::DEFECT);
    }
    // A rarity asked by name can be one the default leaves out.
    assert(combat.random_card(CardRarity::ANCIENT) == CardId::BREAK || card_meta(combat.random_card(CardRarity::ANCIENT)).rarity == CardRarity::ANCIENT);
    // The three types share the pool, and every rarity too.
    assert(count_fitting(combat, CardType::ATTACK) + count_fitting(combat, CardType::SKILL) +
           count_fitting(combat, CardType::POWER) == count_fitting(combat, {}));
    assert(count_fitting(combat, CardRarity::COMMON) + count_fitting(combat, CardRarity::UNCOMMON) +
           count_fitting(combat, CardRarity::RARE) == count_fitting(combat, {}));
    std::cout << "[PASS] Random card: by type, by rarity, and both with a pool" << std::endl;
}

void test_random_card_uses_the_fight_generator() {
    // Same fight, same cards; the draws move the sequence the shuffles use.
    CombatEngine first = make_pack(CharacterId::SILENT);
    CombatEngine second = make_pack(CharacterId::SILENT);
    std::map<int, CardId> drawn;
    for (int i = 0; i < 30; ++i) {
        CardId id = first.random_card();
        assert(id == second.random_card());
        drawn[i] = id;
    }
    CombatEngine third = make_pack(CharacterId::SILENT);
    third.random_card();
    int same = 0;
    for (int i = 0; i < 29; ++i) same += third.random_card() == drawn[i];
    assert(same < 29); // one draw earlier, the cards that follow are others

    // Created in a pile like any card: counted as created, upgraded when asked.
    CombatEngine combat = make_pack(CharacterId::DEFECT);
    combat.hand.clear();
    assert(combat.create_random_card(CardType::POWER, Pile::HAND, true));
    assert(combat.hand.size() == 1 && combat.hand[0].upgraded && CardDef::get(combat.hand[0].id).type == CardType::POWER);
    assert(combat.counters.combat_cards_created == 1);
    size_t discard = combat.discard_pile.size();
    assert(combat.create_random_card({}, Pile::DISCARD));
    assert(combat.discard_pile.size() == discard + 1 && !combat.discard_pile.back().upgraded);
    std::cout << "[PASS] Random card: drawn with the fight's generator, created like any card" << std::endl;
}

void test_manifest_authority() {
    assert_in_pool(CharacterId::REGENT, { CardId::MANIFEST_AUTHORITY });
    const CardDef& def = CardDef::get(CardId::MANIFEST_AUTHORITY);
    assert(def.cost == 1 && def.upgraded_cost == 1 && def.type == CardType::SKILL && def.target == TargetType::SELF);
    assert(card_meta(CardId::MANIFEST_AUTHORITY).rarity == CardRarity::UNCOMMON);

    for (bool upgraded : { false, true }) {
        // 7 Block (8 upgraded) for 1 energy, and 1 random Colorless card in hand, Upgraded when the card is.
        CombatEngine combat = make_pack(CharacterId::REGENT);
        int colorless = count_fitting(combat, CardPool::COLORLESS);
        play(combat, DeckCard(CardId::MANIFEST_AUTHORITY, upgraded));
        assert(combat.player.block == (upgraded ? 8 : 7));
        assert(combat.player.energy == combat.player.max_energy - 1);
        assert(combat.discard_pile.back().id == CardId::MANIFEST_AUTHORITY);
        if (colorless > 0) {
            assert(combat.hand.size() == 1);
            assert(card_meta(combat.hand[0].id).pool == CardPool::COLORLESS && combat.hand[0].upgraded == upgraded);
            assert(combat.counters.combat_cards_created == 1);
        } else {
            // No Colorless card has its effect in the engine yet: nothing can be drawn, the Block is still gained.
            assert(combat.hand.empty() && combat.counters.combat_cards_created == 0);
        }
        // The enemies are left alone.
        assert(combat.enemies[0].hp == 18 && combat.enemies[2].hp == 42);
    }
    std::cout << "[PASS] Regent: Manifest Authority" << std::endl;
}

void test_royalties() {
    assert_in_pool(CharacterId::REGENT, { CardId::ROYALTIES });
    const CardDef& def = CardDef::get(CardId::ROYALTIES);
    assert(def.cost == 1 && def.upgraded_cost == 1 && def.type == CardType::POWER && def.target == TargetType::SELF);
    assert(card_meta(CardId::ROYALTIES).rarity == CardRarity::RARE);

    for (bool upgraded : { false, true }) {
        // 30 gold (40 upgraded) at the end of the combat, paid by the run.
        CombatEngine combat = make_pack(CharacterId::REGENT);
        play(combat, DeckCard(CardId::ROYALTIES, upgraded));
        assert(combat.player.energy == combat.player.max_energy - 1);
        assert(combat.power_amount(PowerId::ROYALTIES) == (upgraded ? 40 : 30));
        assert(combat.enemies[0].hp == 18 && combat.enemies[2].hp == 42);

        RunManager run;
        run.start_run(CharacterId::REGENT, 1234);
        int16_t before = run.gold;
        run.combat.start_combat(EnemyType::JAW_WORM, CharacterId::REGENT);
        play(run.combat, DeckCard(CardId::ROYALTIES, upgraded));
        run.combat.state = CombatState::VICTORY;
        run.phase = RunPhase::IN_COMBAT;
        run.proceed_after_combat();
        assert(run.gold == before + (upgraded ? 40 : 30));
    }
    std::cout << "[PASS] Regent: Royalties" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Regent Cards ===" << std::endl;
    test_card_meta();
    test_random_card_by_pool();
    test_random_card_by_type_and_rarity();
    test_random_card_uses_the_fight_generator();
    test_manifest_authority();
    test_royalties();
    std::cout << "=== All Regent Cards Passed! ===" << std::endl;
    return 0;
}
