// Events described by data (include/engine/event_manager.h): the table, every choice and every outcome of
// every event, the conditions, and the draw of the event of a room.

#include <cassert>
#include <iostream>
#include <set>
#include <string_view>

#include "engine/event_manager.h"
#include "engine/potion.h"
#include "engine/relic.h"
#include "engine/rewards.h"
#include "engine/run.h"

using namespace engine;

namespace {

RunManager fresh_run(uint32_t seed = 42, CharacterId hero = CharacterId::IRONCLAD) {
    RunManager run;
    run.start_run(hero, 0, PaletteVariant::CLASSIC, seed);
    run.hp = 40;
    run.max_hp = 80;
    run.gold = 100;
    return run;
}

// Index of the first event room of the map, -1 when the map has none.
int first_event_node(const RunManager& run) {
    for (size_t i = 0; i < run.map.nodes.size(); ++i) {
        if (run.map.nodes[i].type == RoomType::EVENT) return static_cast<int>(i);
    }
    return -1;
}

// A run standing in an event room of its map, as the game is when a choice is made.
RunManager run_in_event_room(uint32_t seed = 42) {
    for (; seed < 200; ++seed) {
        RunManager run = fresh_run(seed);
        int node = first_event_node(run);
        if (node < 0) continue;
        run.enter_room(node);
        assert(run.phase == RunPhase::EVENT && run.current_event != nullptr);
        run.hp = 40;
        return run;
    }
    assert(false && "no map with an event room");
    return {};
}

size_t upgraded_cards(const RunManager& run) {
    size_t count = 0;
    for (const DeckCard& card : run.master_deck) count += card.upgraded;
    return count;
}

bool same_state(const RunManager& a, const RunManager& b) {
    if (a.hp != b.hp || a.max_hp != b.max_hp || a.gold != b.gold || a.phase != b.phase) return false;
    if (a.master_deck.size() != b.master_deck.size() || a.relics.size() != b.relics.size() ||
        a.potions.size() != b.potions.size()) {
        return false;
    }
    for (size_t i = 0; i < a.master_deck.size(); ++i) {
        if (!(a.master_deck[i] == b.master_deck[i])) return false;
    }
    for (size_t i = 0; i < a.relics.size(); ++i) {
        if (a.relics[i] != b.relics[i]) return false;
    }
    for (size_t i = 0; i < a.potions.size(); ++i) {
        if (a.potions[i] != b.potions[i]) return false;
    }
    return true;
}

EventChoice choice_of(std::initializer_list<EventOutcome> outcomes) {
    EventChoice choice;
    choice.label = "Test";
    choice.cost_benefit = "Test.";
    choice.outcomes = outcomes;
    choice.result_message = "Done.";
    return choice;
}

EventResult apply(RunManager& run, const EventChoice& choice, uint32_t seed = 7) {
    char message[128];
    EventResult result;
    bool ok = EventManager::apply_choice(run, choice, message, sizeof(message), &result, seed);
    assert(ok);
    return result;
}

bool in_reward_pool(const RunManager& run, CardId id) {
    for (CardId candidate : RunManager::card_reward_pool(run.character)) {
        if (candidate == id) return true;
    }
    return false;
}

// ---------------------------------------------------------------------------------------------------------------------
// 1. The table
// ---------------------------------------------------------------------------------------------------------------------

void test_table_is_well_formed() {
    std::set<std::string_view> ids;
    for (const EventEncounter& event : EventManager::all()) {
        assert(!event.id.empty() && !event.title.empty() && !event.dialogue.empty());
        assert(ids.insert(event.id).second && "duplicate event id");
        assert(EventManager::find(event.id) == &event);
        assert(event.zones != 0 && (event.zones & ~EVENT_ZONE_ALL) == 0);
        assert(event.choices.size() >= 2 && event.choices.size() <= EVENT_MAX_CHOICES);

        bool always_possible = false;
        for (const EventChoice& choice : event.choices) {
            assert(!choice.label.empty() && !choice.cost_benefit.empty() && !choice.result_message.empty());
            always_possible = always_possible || choice.condition == EventCondition::NONE;
            for (const EventOutcome& outcome : choice.outcomes) {
                assert(outcome.type != EventOutcomeType::NONE && outcome.value >= 0 && outcome.spread >= 0);
                // What an outcome names exists in the engine.
                if (outcome.type == EventOutcomeType::GAIN_RELIC && !outcome.item.empty()) {
                    assert(find_relic(outcome.item) != nullptr);
                }
                if (outcome.type == EventOutcomeType::GAIN_POTION && !outcome.item.empty()) {
                    assert(find_potion(outcome.item) != nullptr);
                }
                if (outcome.type == EventOutcomeType::GAIN_CARD && outcome.card != CardId::NONE) {
                    assert(!CardDef::get(outcome.card).name.empty());
                }
            }
        }
        // A hero is never stuck in an event: one choice at least asks for nothing.
        assert(always_possible);
    }
    assert(!ids.empty());
    assert(EventManager::find("") == nullptr && EventManager::find("NOT_AN_EVENT") == nullptr);
    std::cout << "[PASS] Table: " << ids.size() << " events, unique ids, 2 to 4 choices, one always possible" << std::endl;
}

// ---------------------------------------------------------------------------------------------------------------------
// 2. Every choice and every outcome of every event of the table
// ---------------------------------------------------------------------------------------------------------------------

// Checks one choice against what its outcomes say, with a model of the rules kept apart from the engine's.
void check_choice(const EventEncounter& event, size_t choice_index, CharacterId hero) {
    const EventChoice& choice = event.choices[choice_index];
    RunManager run = run_in_event_room();
    run.character = hero;
    run.current_event = &event;
    run.gold = 500;
    // Whatever the condition, this run meets it.
    assert(EventManager::choice_available(run, choice));

    // Lowest and highest value each outcome can leave, in the order of the list.
    struct Range { int lo, hi; };
    Range hp { run.hp, run.hp };
    Range max_hp { run.max_hp, run.max_hp };
    Range gold { run.gold, run.gold };
    Range deck { static_cast<int>(run.master_deck.size()), static_cast<int>(run.master_deck.size()) };
    int relics = static_cast<int>(run.relics.size());
    int potions = static_cast<int>(run.potions.size());
    int upgrades = 0;
    bool deck_reshaped = false;
    bool fight = false;

    for (const EventOutcome& outcome : choice.outcomes) {
        const int least = outcome.value;
        const int most = outcome.value + outcome.spread;
        switch (outcome.type) {
            case EventOutcomeType::GAIN_GOLD: gold = { gold.lo + least, gold.hi + most }; break;
            case EventOutcomeType::LOSE_GOLD: gold = { std::max(0, gold.lo - most), std::max(0, gold.hi - least) }; break;
            case EventOutcomeType::HEAL:
                hp = { std::min(max_hp.lo, hp.lo + least), std::min(max_hp.hi, hp.hi + most) };
                break;
            case EventOutcomeType::LOSE_HP: hp = { std::max(1, hp.lo - most), std::max(1, hp.hi - least) }; break;
            case EventOutcomeType::GAIN_MAX_HP:
                max_hp = { max_hp.lo + least, max_hp.hi + most };
                hp = { hp.lo + least, hp.hi + most };
                break;
            case EventOutcomeType::LOSE_MAX_HP:
                max_hp = { std::max(1, max_hp.lo - most), std::max(1, max_hp.hi - least) };
                hp = { std::min(hp.lo, max_hp.lo), std::min(hp.hi, max_hp.hi) };
                break;
            case EventOutcomeType::GAIN_RELIC: ++relics; break;
            case EventOutcomeType::GAIN_POTION: ++potions; break;
            case EventOutcomeType::GAIN_CARD: deck = { deck.lo + 1, deck.hi + 1 }; break;
            case EventOutcomeType::REMOVE_CARD:
                deck = { std::max(0, deck.lo - most), std::max(0, deck.hi - least) };
                deck_reshaped = true;
                break;
            case EventOutcomeType::UPGRADE_CARD: upgrades += least; break;
            case EventOutcomeType::TRANSFORM_CARD: deck_reshaped = true; break;
            case EventOutcomeType::FIGHT: fight = true; break;
            default: assert(false && "outcome outside the fixed set");
        }
    }
    const size_t upgraded_before = upgraded_cards(run);

    char message[160] = {};
    EventResult result;
    assert(run.choose_event_choice(static_cast<int>(choice_index), message, sizeof(message), &result));
    assert(std::string_view(message) == choice.result_message);
    assert(result.fight == fight);
    assert(run.phase == (fight ? RunPhase::IN_COMBAT : RunPhase::MAP_NAVIGATION));

    assert(run.gold >= gold.lo && run.gold <= gold.hi);
    assert(run.hp >= hp.lo && run.hp <= hp.hi && run.hp <= run.max_hp);
    assert(run.max_hp >= max_hp.lo && run.max_hp <= max_hp.hi);
    assert(static_cast<int>(run.relics.size()) == relics);
    assert(static_cast<int>(run.potions.size()) == potions);
    assert(static_cast<int>(run.master_deck.size()) >= deck.lo && static_cast<int>(run.master_deck.size()) <= deck.hi);
    if (!deck_reshaped) assert(upgraded_cards(run) >= upgraded_before + static_cast<size_t>(upgrades));
    // A choice without outcome tells nothing; a list never tells more than it holds.
    if (choice.outcomes.empty()) assert(result.effects.empty());
    if (!choice.outcomes.empty() && upgrades == 0) assert(!result.effects.empty());

    // Same room, same choice: same result.
    RunManager again = run_in_event_room();
    again.character = hero;
    again.current_event = &event;
    again.gold = 500;
    assert(again.choose_event_choice(static_cast<int>(choice_index)));
    if (!fight) assert(same_state(run, again));
}

void test_every_choice_of_every_event() {
    size_t choices = 0;
    size_t outcomes = 0;
    for (const EventEncounter& event : EventManager::all()) {
        for (size_t i = 0; i < event.choices.size(); ++i) {
            for (CharacterId hero : { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT,
                                      CharacterId::REGENT, CharacterId::NECROBINDER }) {
                check_choice(event, i, hero);
            }
            ++choices;
            outcomes += event.choices[i].outcomes.size();
        }
    }
    std::cout << "[PASS] Every event: " << choices << " choices and " << outcomes
              << " outcomes applied for the five heroes, twice with the same result" << std::endl;
}

// ---------------------------------------------------------------------------------------------------------------------
// 3. The fixed set of outcomes, one by one
// ---------------------------------------------------------------------------------------------------------------------

void test_gold_hp_and_max_hp() {
    {
        RunManager run = fresh_run();
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::GAIN_GOLD, .value = 60 } }));
        assert(run.gold == 160 && result.effects.size() == 1);
        assert(result.effects[0].type == EventOutcomeType::GAIN_GOLD && result.effects[0].amount == 60);
    }
    {
        RunManager run = fresh_run();
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::LOSE_GOLD, .value = 35 } }));
        assert(run.gold == 65 && result.effects[0].amount == 35);
        // Never below 0, and the amount told is the gold really lost.
        result = apply(run, choice_of({ { .type = EventOutcomeType::LOSE_GOLD, .value = 500 } }));
        assert(run.gold == 0 && result.effects[0].amount == 65);
    }
    {
        RunManager run = fresh_run();
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::HEAL, .value = 25 } }));
        assert(run.hp == 65 && result.effects[0].amount == 25);
        result = apply(run, choice_of({ { .type = EventOutcomeType::HEAL, .value = 25 } }));
        assert(run.hp == 80 && result.effects[0].amount == 15);
    }
    {
        RunManager run = fresh_run();
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::LOSE_HP, .value = 8 } }));
        assert(run.hp == 32 && result.effects[0].amount == 8);
        // An event does not kill.
        result = apply(run, choice_of({ { .type = EventOutcomeType::LOSE_HP, .value = 99 } }));
        assert(run.hp == 1 && result.effects[0].amount == 31);
    }
    {
        RunManager run = fresh_run();
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::GAIN_MAX_HP, .value = 5 } }));
        assert(run.max_hp == 85 && run.hp == 45 && result.effects[0].amount == 5);
    }
    {
        RunManager run = fresh_run();
        run.hp = 78;
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::LOSE_MAX_HP, .value = 6 } }));
        assert(run.max_hp == 74 && run.hp == 74 && result.effects[0].amount == 6);
        apply(run, choice_of({ { .type = EventOutcomeType::LOSE_MAX_HP, .value = 500 } }));
        assert(run.max_hp == 1 && run.hp == 1);
    }
    {
        // A choice that does nothing tells nothing and only gives its message.
        RunManager run = fresh_run();
        RunManager before = run;
        EventResult result = apply(run, choice_of({}));
        assert(result.effects.empty() && !result.fight && same_state(run, before));
    }
    std::cout << "[PASS] Outcomes: gold, HP and Max HP gained and lost, with their limits" << std::endl;
}

void test_random_amounts() {
    const EventChoice choice = choice_of({ { .type = EventOutcomeType::GAIN_GOLD, .value = 50, .spread = 30 } });
    std::set<int> amounts;
    for (uint32_t seed = 0; seed < 200; ++seed) {
        RunManager run = fresh_run();
        apply(run, choice, seed);
        int gained = run.gold - 100;
        assert(gained >= 50 && gained <= 80);
        amounts.insert(gained);

        RunManager again = fresh_run();
        apply(again, choice, seed);
        assert(again.gold == run.gold);
    }
    // The whole range is reached, both ends included.
    assert(amounts.size() == 31 && *amounts.begin() == 50 && *amounts.rbegin() == 80);
    std::cout << "[PASS] Outcomes: a random amount covers value to value + spread, the same for a same seed" << std::endl;
}

void test_relics_and_potions() {
    {
        RunManager run = fresh_run();
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::GAIN_RELIC, .item = "VAJRA" } }));
        assert(run.has_relic("VAJRA") && run.relics.size() == 2);
        assert(result.effects.size() == 1 && result.effects[0].name == find_relic("VAJRA")->name);
    }
    {
        // A random relic: one with an implemented effect, of this hero or shared, never one already owned.
        std::set<std::string_view> seen;
        for (uint32_t seed = 0; seed < 300; ++seed) {
            RunManager run = fresh_run(42, CharacterId::SILENT);
            run.add_relic("ANCHOR");
            EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::GAIN_RELIC } }), seed);
            assert(run.relics.size() == 3);
            std::string_view id = run.relics.back();
            assert(id != "ANCHOR" && id != run.relics[0]);
            const RelicData* relic = find_relic(id);
            assert(relic && (relic->pool == "Shared" || relic->pool == "Silent"));
            bool active = false;
            for (std::string_view candidate : RunManager::active_relics()) active = active || candidate == id;
            assert(active);
            assert(result.effects[0].name == relic->name);
            seen.insert(id);
        }
        assert(seen.size() > 20);
    }
    {
        // No room left for a relic: nothing is gained and nothing is told.
        RunManager run = fresh_run();
        while (run.relics.size() < run.relics.capacity()) run.relics.push_back("ANCHOR");
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::GAIN_RELIC, .item = "VAJRA" } }));
        assert(!run.has_relic("VAJRA") && result.effects.empty());
    }
    {
        RunManager run = fresh_run();
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::GAIN_POTION, .item = "BLOCK_POTION" } }));
        assert(run.potions.size() == 1 && run.potions[0] == "BLOCK_POTION");
        assert(result.effects[0].name == find_potion("BLOCK_POTION")->name);
        // Without an id, a random potion among the ones the hero can use.
        std::set<std::string_view> seen;
        for (uint32_t seed = 0; seed < 200; ++seed) {
            RunManager other = fresh_run(42, CharacterId::REGENT);
            apply(other, choice_of({ { .type = EventOutcomeType::GAIN_POTION } }), seed);
            assert(other.potions.size() == 1 && potion_fits_hero(other.potions[0], CharacterId::REGENT));
            seen.insert(other.potions[0]);
        }
        assert(seen.size() > 10);
        apply(run, choice_of({ { .type = EventOutcomeType::GAIN_POTION } }));
        assert(run.potions.size() == 2);
        // A full belt loses the potion.
        apply(run, choice_of({ { .type = EventOutcomeType::GAIN_POTION } }));
        result = apply(run, choice_of({ { .type = EventOutcomeType::GAIN_POTION } }));
        assert(run.potions.size() == run.potions.capacity() && result.effects.empty());
    }
    std::cout << "[PASS] Outcomes: named and random relics, potions, full relic list and full belt" << std::endl;
}

void test_cards() {
    {
        RunManager run = fresh_run();
        size_t deck = run.master_deck.size();
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::GAIN_CARD, .card = CardId::IRON_WAVE } }));
        assert(run.master_deck.size() == deck + 1 && run.master_deck.back() == CardId::IRON_WAVE);
        assert(!run.master_deck.back().upgraded && result.effects[0].name == CardDef::get(CardId::IRON_WAVE).name);
    }
    {
        // A random card comes from the reward pool of the hero.
        std::set<CardId> seen;
        for (uint32_t seed = 0; seed < 200; ++seed) {
            RunManager run = fresh_run(42, CharacterId::DEFECT);
            size_t deck = run.master_deck.size();
            apply(run, choice_of({ { .type = EventOutcomeType::GAIN_CARD } }), seed);
            assert(run.master_deck.size() == deck + 1 && in_reward_pool(run, run.master_deck.back().id));
            seen.insert(run.master_deck.back().id);
        }
        assert(seen.size() > 20);
    }
    {
        // A full deck loses the card.
        RunManager run = fresh_run();
        while (run.master_deck.size() < run.master_deck.capacity()) run.master_deck.push_back(CardId::STRIKE);
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::GAIN_CARD } }));
        assert(run.master_deck.size() == run.master_deck.capacity() && result.effects.empty());
    }
    {
        // Removing: the first card, or random ones; never more than the deck holds.
        RunManager run = fresh_run();
        size_t deck = run.master_deck.size();
        run.master_deck[0] = DeckCard(CardId::BASH);
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::REMOVE_CARD, .value = 1, .pick = EventPick::FIRST } }));
        assert(run.master_deck.size() == deck - 1 && !(run.master_deck[0] == CardId::BASH));
        assert(result.effects.size() == 1 && result.effects[0].name == CardDef::get(CardId::BASH).name);

        result = apply(run, choice_of({ { .type = EventOutcomeType::REMOVE_CARD, .value = 2 } }));
        assert(run.master_deck.size() == deck - 3 && result.effects.size() == 2);
        apply(run, choice_of({ { .type = EventOutcomeType::REMOVE_CARD, .value = 100 } }));
        assert(run.master_deck.empty());
        result = apply(run, choice_of({ { .type = EventOutcomeType::REMOVE_CARD, .value = 1 } }));
        assert(run.master_deck.empty() && result.effects.empty());
    }
    {
        // Random removals reach every place of the deck.
        std::set<size_t> removed;
        for (uint32_t seed = 0; seed < 300; ++seed) {
            RunManager run = fresh_run();
            for (size_t i = 0; i < run.master_deck.size(); ++i) run.master_deck[i].bonus = static_cast<uint8_t>(i);
            size_t deck = run.master_deck.size();
            apply(run, choice_of({ { .type = EventOutcomeType::REMOVE_CARD, .value = 1 } }), seed);
            size_t gone = deck - 1;
            for (size_t i = 0; i < run.master_deck.size(); ++i) {
                if (run.master_deck[i].bonus != i) { gone = i; break; }
            }
            removed.insert(gone);
        }
        assert(removed.size() == fresh_run().master_deck.size());
    }
    {
        // Upgrading: the first card not upgraded yet, or random ones among them.
        RunManager run = fresh_run();
        run.master_deck[0].upgraded = true;
        apply(run, choice_of({ { .type = EventOutcomeType::UPGRADE_CARD, .value = 1, .pick = EventPick::FIRST } }));
        assert(run.master_deck[1].upgraded && upgraded_cards(run) == 2);
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::UPGRADE_CARD, .value = 3 } }));
        assert(upgraded_cards(run) == 5 && result.effects.size() == 3);
        apply(run, choice_of({ { .type = EventOutcomeType::UPGRADE_CARD, .value = 100 } }));
        assert(upgraded_cards(run) == run.master_deck.size());

        // Nothing left to upgrade: nothing changes and the message says so.
        char message[128];
        const EventChoice choice = choice_of({ { .type = EventOutcomeType::UPGRADE_CARD, .value = 1 } });
        assert(EventManager::apply_choice(run, choice, message, sizeof(message), &result));
        assert(std::string_view(message) != choice.result_message && result.effects.empty());
    }
    {
        // Transforming: the card becomes another card of the hero's reward pool, not upgraded.
        for (uint32_t seed = 0; seed < 200; ++seed) {
            RunManager run = fresh_run(42, CharacterId::SILENT);
            run.master_deck[0] = DeckCard(CardId::POISON_STAB, true);
            size_t deck = run.master_deck.size();
            EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::TRANSFORM_CARD, .value = 1, .pick = EventPick::FIRST } }), seed);
            assert(run.master_deck.size() == deck);
            const DeckCard& card = run.master_deck[0];
            assert(card.id != CardId::POISON_STAB && !card.upgraded && in_reward_pool(run, card.id));
            assert(result.effects.size() == 1 && result.effects[0].name == CardDef::get(card.id).name);
        }
        RunManager run = fresh_run();
        RunManager before = run;
        apply(run, choice_of({ { .type = EventOutcomeType::TRANSFORM_CARD, .value = 2 } }));
        size_t changed = 0;
        for (size_t i = 0; i < run.master_deck.size(); ++i) changed += !(run.master_deck[i] == before.master_deck[i]);
        assert(run.master_deck.size() == before.master_deck.size() && changed >= 1 && changed <= 2);
    }
    std::cout << "[PASS] Outcomes: cards gained, removed, upgraded and transformed, first or random" << std::endl;
}

void test_fight() {
    for (int16_t tier : { EVENT_FIGHT_MONSTER, EVENT_FIGHT_ELITE }) {
        RunManager run = run_in_event_room();
        const int node = run.current_node_index;
        const uint8_t floor = run.current_floor;
        // The fight is listed first and still starts last, with the HP the other outcome left.
        const EventChoice choice = choice_of({ { .type = EventOutcomeType::FIGHT, .value = tier },
                                               { .type = EventOutcomeType::LOSE_HP, .value = 5 } });
        char message[64];
        EventResult result;
        assert(EventManager::apply_choice(run, choice, message, sizeof(message), &result, 3));
        assert(result.fight && run.phase == RunPhase::IN_COMBAT);
        assert(run.hp == 35 && run.combat.player.hp == 35 && run.combat.player.max_hp == run.max_hp);
        assert(!run.combat.enemies.empty() && run.combat.has_alive_enemies());
        assert(!run.current_encounter.empty());
        // The room is an event room again, on the same floor.
        assert(run.current_node_index == node && run.current_floor == floor);
        assert(run.map.nodes[node].type == RoomType::EVENT);
        assert(result.effects.size() == 2 && result.effects[1].type == EventOutcomeType::FIGHT);

        // Won: the rewards of a fight, then the map.
        for (Enemy& foe : run.combat.enemies) foe.hp = 0;
        run.proceed_after_combat();
        assert(run.phase == RunPhase::REWARD_SCREEN);
        run.skip_card_rewards();
        assert(run.phase == RunPhase::MAP_NAVIGATION);
    }
    {
        // Outside a room of the map there is nowhere to fight: the other outcomes still apply.
        RunManager run = fresh_run();
        EventResult result = apply(run, choice_of({ { .type = EventOutcomeType::FIGHT }, { .type = EventOutcomeType::GAIN_GOLD, .value = 10 } }));
        assert(!result.fight && run.phase == RunPhase::MAP_NAVIGATION && run.gold == 110);
    }
    std::cout << "[PASS] Outcomes: a fight starts after the other outcomes, in the event room, and gives rewards" << std::endl;
}

// ---------------------------------------------------------------------------------------------------------------------
// 4. Conditions
// ---------------------------------------------------------------------------------------------------------------------

struct ConditionCase {
    EventCondition condition;
    int16_t value;
    void (*make_unmet)(RunManager&);
    void (*make_met)(RunManager&);
};

void test_conditions() {
    const ConditionCase cases[] = {
        { EventCondition::GOLD_AT_LEAST, 35, [](RunManager& r) { r.gold = 34; }, [](RunManager& r) { r.gold = 35; } },
        { EventCondition::HP_ABOVE, 10, [](RunManager& r) { r.hp = 10; }, [](RunManager& r) { r.hp = 11; } },
        { EventCondition::MAX_HP_ABOVE, 80, [](RunManager& r) { r.max_hp = 80; }, [](RunManager& r) { r.max_hp = 81; } },
        { EventCondition::DECK_AT_LEAST, 3,
          [](RunManager& r) { while (r.master_deck.size() > 2) r.master_deck.pop_back(); },
          [](RunManager& r) { r.master_deck.push_back(CardId::STRIKE); } },
        { EventCondition::UPGRADABLE_CARD, 0,
          [](RunManager& r) { for (DeckCard& card : r.master_deck) card.upgraded = true; },
          [](RunManager& r) { r.master_deck[0].upgraded = false; } },
        { EventCondition::FREE_POTION_SLOT, 0,
          [](RunManager& r) { while (r.potions.size() < r.potions.capacity()) r.potions.push_back("FIRE_POTION"); },
          [](RunManager& r) { r.potions.pop_back(); } },
    };

    for (const ConditionCase& test : cases) {
        EventChoice choice = choice_of({ { .type = EventOutcomeType::GAIN_GOLD, .value = 10 },
                                         { .type = EventOutcomeType::LOSE_HP, .value = 1 } });
        choice.condition = test.condition;
        choice.condition_value = test.value;
        const EventEncounter event { .id = "TEST", .title = "Test", .dialogue = "Test.", .choices = { choice, choice_of({}) } };

        RunManager run = run_in_event_room();
        run.current_event = &event;
        test.make_unmet(run);
        RunManager before = run;

        // Refused: nothing changes, nothing is told, and the hero is still in the event.
        char message[64] = "untouched";
        EventResult result;
        result.effects.push_back(EventEffect{});
        assert(!EventManager::choice_available(run, choice));
        assert(!run.choose_event_choice(0, message, sizeof(message), &result));
        assert(same_state(run, before) && run.phase == RunPhase::EVENT);
        assert(std::string_view(message) == "untouched" && result.effects.empty());
        // The other choice of the event is still there.
        assert(EventManager::choice_available(run, event.choices[1]));

        test.make_met(run);
        const int16_t gold = run.gold;
        assert(EventManager::choice_available(run, choice));
        assert(run.choose_event_choice(0, message, sizeof(message), &result));
        assert(run.gold == gold + 10 && run.phase == RunPhase::MAP_NAVIGATION && result.effects.size() == 2);
    }

    // A choice that does not exist is refused the same way.
    RunManager run = run_in_event_room();
    RunManager before = run;
    assert(!run.choose_event_choice(-1) && !run.choose_event_choice(static_cast<int>(run.current_event->choices.size())));
    assert(same_state(run, before));
    run.current_event = nullptr;
    assert(!run.choose_event_choice(0));
    std::cout << "[PASS] Conditions: gold, HP, Max HP, deck size, upgradable card and potion slot refused when not met"
              << std::endl;
}

// ---------------------------------------------------------------------------------------------------------------------
// 5. The event of a room
// ---------------------------------------------------------------------------------------------------------------------

bool in_zone(const EventEncounter& event, uint8_t act, uint32_t map_seed) {
    return (event.zones & zone_bit(act_zone(act, map_seed))) != 0;
}

size_t zone_size(uint8_t act, uint32_t map_seed) {
    size_t count = 0;
    for (const EventEncounter& event : EventManager::all()) count += in_zone(event, act, map_seed);
    return count;
}

void test_event_of_a_room() {
    const size_t total = EventManager::all().size();
    std::set<std::string_view> met;
    bool zones_seen[4] = {};

    for (uint32_t n = 1; n <= 300; ++n) {
        const uint32_t seed = n * 2654435761u; // spread over the 32 bits: the variant of Act 1 is a high bit
        for (uint8_t act = 1; act <= 3; ++act) {
            MapGraph map;
            map.generate(act, seed, 0);
            zones_seen[static_cast<uint8_t>(act_zone(act, seed))] = true;
            const size_t own = zone_size(act, seed);

            // The events of the event floors of the act, bottom to top.
            FixedVector<const EventEncounter*, 16> events;
            for (uint8_t floor = 1; floor <= 16; ++floor) {
                bool has_event = false;
                for (const MapNode& node : map.nodes) has_event = has_event || (node.type == RoomType::EVENT && node.floor + 1 == floor);
                if (!has_event) continue;
                const EventEncounter& event = EventManager::pick(map, act, seed, floor);
                // Same act, same map, same floor: same event.
                assert(&EventManager::pick(map, act, seed, floor) == &event);
                events.push_back(&event);
                met.insert(event.id);
            }

            for (size_t i = 0; i < events.size(); ++i) {
                // The events of the zone come first, the others only once the zone has none left.
                assert(in_zone(*events[i], act, seed) == (i % total < own));
                // Never the same event twice before every event has been met.
                for (size_t j = i + 1; j < events.size() && j < i + total; ++j) assert(events[i] != events[j]);
            }
        }
    }
    assert(met.size() == total);
    assert(zones_seen[0] && zones_seen[1] && zones_seen[2] && zones_seen[3]);

    // Through the run: entering an event room shows the event of its floor, whatever was drawn before, and
    // moves the run's random sequence by one draw exactly, as it did before the events were a table.
    size_t rooms_entered = 0;
    for (uint32_t seed : { 42u, 777u, 1234u }) {
        RunManager run = fresh_run(seed);
        int node = first_event_node(run);
        if (node < 0) continue;
        ++rooms_entered;
        RunManager other = fresh_run(seed);
        other.open_shop(); // one more draw of the main sequence before the event
        other.phase = RunPhase::MAP_NAVIGATION;

        uint32_t state = run.rng_state();
        run.enter_room(node);
        other.enter_room(node);
        assert(run.phase == RunPhase::EVENT && run.current_event != nullptr);
        assert(run.current_event == other.current_event);
        assert(run.current_event == &EventManager::pick(run.map, run.current_act, run.map_seed, run.current_floor));

        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        assert(run.rng_state() == state);

        // A choice leaves the main sequence alone.
        assert(run.choose_event_choice(static_cast<int>(run.current_event->choices.size()) - 1));
        assert(run.rng_state() == state);
    }
    assert(rooms_entered > 0);
    std::cout << "[PASS] Event of a room: same seed same event, the zone first, no event twice in an act, "
              << "one draw of the run's sequence" << std::endl;
}

} // namespace

int main() {
    test_table_is_well_formed();
    test_every_choice_of_every_event();
    test_gold_hp_and_max_hp();
    test_random_amounts();
    test_relics_and_potions();
    test_cards();
    test_fight();
    test_conditions();
    test_event_of_a_room();
    std::cout << "[PASS] Events: all groups validated" << std::endl;
    return 0;
}
