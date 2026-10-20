#pragma once
// Scriptorium englouti (WATERLOGGED_SCRIPTORIUM): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_waterlogged_scriptorium() {
    using namespace event_test;

    const EventDef* def = EventManager::find("WATERLOGGED_SCRIPTORIUM");
    assert(def && def->acts == EVENT_ACT_1B && def->pages.size() == 1 && def->pages[0].options.size() == 3);

    const auto steady_cards = [](const RunManager& run) {
        int count = 0;
        for (const DeckCard& card : run.master_deck) {
            if (card.enchantment == Enchantment::STEADY) {
                ++count;
                assert(CardDef::has_keyword(card, KEYWORD_RETAIN));
            }
        }
        return count;
    };

    // Bloody Ink: 6 Max HP (and HP), nothing paid.
    {
        EventRun ink("WATERLOGGED_SCRIPTORIUM");
        assert(ink.run.current_event.choices.size() == 3 && ink.run.current_event.choices[0].enabled);
        const size_t deck = ink.run.master_deck.size();
        ink.pick(0);
        assert(ink.run.max_hp == 86 && ink.run.hp == 46 && ink.run.gold == 100);
        assert(ink.run.master_deck.size() == deck && steady_cards(ink.run) == 0 && !ink.run.has_deck_choice());
        assert(ink.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Prickly Sponge: 99 gold, 2 cards of the player's choice become Steady (Retain).
    {
        EventRun sponge("WATERLOGGED_SCRIPTORIUM");
        sponge.pick(1);
        assert(sponge.run.gold == 1 && sponge.run.hp == 40 && sponge.run.max_hp == 80);
        assert(sponge.run.has_deck_choice() && sponge.run.deck_choice.kind == DeckChoiceKind::ENCHANT);
        assert(sponge.run.deck_choice.enchantment == Enchantment::STEADY && sponge.run.deck_choice.remaining == 2);
        assert(sponge.run.resolve_deck_choice(0));
        assert(sponge.run.resolve_deck_choice(1));
        assert(!sponge.run.has_deck_choice() && steady_cards(sponge.run) == 2);
        assert(sponge.run.master_deck[0].enchantment == Enchantment::STEADY);
    }

    // Tentacle Quill: 55 gold, 1 card Steady.
    {
        EventRun quill("WATERLOGGED_SCRIPTORIUM");
        quill.pick(2);
        assert(quill.run.gold == 45 && quill.run.hp == 40 && quill.run.max_hp == 80);
        assert(quill.run.has_deck_choice() && quill.run.deck_choice.remaining == 1);
        assert(quill.run.resolve_deck_choice(3));
        assert(!quill.run.has_deck_choice() && steady_cards(quill.run) == 1);
        assert(quill.run.master_deck[3].enchantment == Enchantment::STEADY);
    }

    // Without the gold, the paid options are greyed out and cannot be chosen.
    {
        RunManager poor;
        poor.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 42);
        poor.gold = 60;
        assert(poor.start_event("WATERLOGGED_SCRIPTORIUM"));
        assert(poor.current_event.choices[0].enabled && !poor.current_event.choices[1].enabled);
        assert(poor.current_event.choices[2].enabled);
        char message[256] = {};
        assert(!poor.choose_event_choice(1, message, sizeof(message)));
        poor.gold = 54;
        assert(poor.start_event("WATERLOGGED_SCRIPTORIUM"));
        assert(!poor.current_event.choices[2].enabled);
    }

    std::cout << "[PASS] Waterlogged Scriptorium: Bloody Ink (+6 Max HP) / Prickly Sponge (99 gold, 2 Steady) / "
                 "Tentacle Quill (55 gold, 1 Steady)"
              << std::endl;
}
