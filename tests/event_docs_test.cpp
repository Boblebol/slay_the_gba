// What the documentation of the events (#252) is generated from: every event the game has must give, for each
// choice, a label, a cost, its effects and the text of its result, and every page must lead somewhere that
// exists. tools/dump_events.cpp writes these same tables out; scripts/build_event_docs.py turns them into
// bible/data/event_game_v0.111.0.json and the choices of bible/18_evenements.md.
#include <cassert>
#include <iostream>
#include <set>
#include <string_view>

#include "engine/event_manager.h"

using namespace engine;

namespace {

size_t implemented_events() {
    size_t count = 0;
    for (const EventSlot& slot : EventManager::slots()) count += slot.def ? 1 : 0;
    return count;
}

// Label of each choice: a short line, and a page never has more options than the screen shows.
void test_event_choice_labels() {
    assert(implemented_events() > 0);
    for (const EventSlot& slot : EventManager::slots()) {
        if (!slot.def) continue;
        const EventDef& def = *slot.def;
        assert(def.id == slot.id && !def.title.empty() && !def.pages.empty());
        for (const EventPage& page : def.pages) {
            assert(!page.text.empty());
            assert(!page.options.empty() && page.options.size() <= MAX_EVENT_OPTIONS);
            for (const EventOption& option : page.options) assert(!option.label.empty() && option.label.size() <= 60);
        }
    }
    std::cout << "[PASS] Event docs: every choice has a label" << std::endl;
}

// Cost of each choice (the line under the options) and the text shown once it is chosen.
void test_event_choice_costs_and_results() {
    for (const EventSlot& slot : EventManager::slots()) {
        if (!slot.def) continue;
        for (const EventPage& page : slot.def->pages) {
            for (const EventOption& option : page.options) {
                assert(!option.cost_benefit.empty());
                assert(!option.result.empty());
            }
        }
    }
    std::cout << "[PASS] Event docs: every choice has a cost line and a result text" << std::endl;
}

// Effects of each choice: they are typed, and a page jump targets a page of the event. A choice without effect
// is a way to leave: its cost line says so.
void test_event_choice_effects() {
    for (const EventSlot& slot : EventManager::slots()) {
        if (!slot.def) continue;
        const EventDef& def = *slot.def;
        for (const EventPage& page : def.pages) {
            for (const EventOption& option : page.options) {
                for (const EventEffect& effect : option.effects) {
                    assert(static_cast<uint8_t>(effect.type) <= static_cast<uint8_t>(EventEffectType::TRADE_RELIC));
                    if (effect.type != EventEffectType::GOTO_PAGE) continue;
                    const int first = effect.amount;
                    const int last = first + (effect.enchant_amount > 1 ? effect.enchant_amount - 1 : 0);
                    assert(first >= 0 && static_cast<size_t>(last) < def.pages.size());
                }
            }
        }
    }
    std::cout << "[PASS] Event docs: every effect is typed and every page jump lands on a page" << std::endl;
}

// Every event of the registry is documented once: the registry holds the 57 events of the reference, plus the
// Mystic Altar kept from the first versions.
void test_event_docs_registry() {
    std::set<std::string_view> ids;
    for (const EventSlot& slot : EventManager::slots()) assert(ids.insert(slot.id).second);
    assert(ids.size() >= 57 && ids.count("ABYSSAL_BATHS") == 1);
    std::cout << "[PASS] Event docs: the registry lists each event once" << std::endl;
}

} // namespace

int main() {
    test_event_docs_registry();
    test_event_choice_labels();
    test_event_choice_costs_and_results();
    test_event_choice_effects();
    std::cout << "[PASS] All event docs tests" << std::endl;
    return 0;
}
