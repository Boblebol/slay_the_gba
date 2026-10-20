#pragma once
// Développement personnel (SELF_HELP_BOOK): each option is chosen once, on its own run.
#include <iostream>
#include "event_test_support.h"

inline void test_self_help_book() {
    using namespace event_test;

    const EventDef* def = EventManager::find("SELF_HELP_BOOK");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 4);

    // Picks the card the choice offers first and checks what it did to it.
    const auto enchant_first = [](EventRun& event, CardType type, Enchantment enchantment, uint8_t amount) {
        assert(event.run.has_deck_choice() && event.run.deck_choice.kind == DeckChoiceKind::ENCHANT);
        int pick = -1;
        for (size_t i = 0; i < event.run.master_deck.size(); ++i) {
            const bool allowed = event.run.deck_choice_allows(static_cast<int>(i));
            assert(allowed == (CardDef::get(event.run.master_deck[i].id).type == type));
            if (allowed && pick < 0) pick = static_cast<int>(i);
        }
        assert(pick >= 0 && event.run.resolve_deck_choice(pick) && !event.run.has_deck_choice());
        assert(event.run.master_deck[pick].enchantment == enchantment && event.run.master_deck[pick].enchant_amount == amount);
        int enchanted = 0;
        for (const DeckCard& card : event.run.master_deck) enchanted += card.enchantment != Enchantment::NONE;
        assert(enchanted == 1);
        (void)type;
        (void)enchantment;
        (void)amount;
    };

    // Read the Entire Book: a Power gets Swift 2. The Ironclad starts without a Power, so the option is greyed out.
    {
        EventRun none("SELF_HELP_BOOK");
        const auto& choices = none.run.current_event.choices;
        assert(!choices[0].enabled && choices[1].enabled && choices[2].enabled && choices[3].enabled);
        assert(!none.run.choose_event_choice(0, none.message, sizeof(none.message)));

        EventRun book("SELF_HELP_BOOK");
        book.run.master_deck.push_back(DeckCard{ CardId::INFLAME, false });
        book.run.master_deck.push_back(DeckCard{ CardId::DEMON_FORM, false });
        EventManager::open_page(book.run, *def, 0);
        assert(book.run.current_event.choices[0].enabled);
        book.pick(0);
        assert(book.run.phase == RunPhase::MAP_NAVIGATION && book.run.hp == 40 && book.run.gold == 100);
        enchant_first(book, CardType::POWER, Enchantment::SWIFT, 2);
    }

    // Read a Random Passage: a Skill gets Nimble 2.
    {
        EventRun passage("SELF_HELP_BOOK");
        const size_t deck = passage.run.master_deck.size();
        passage.pick(1);
        assert(passage.run.master_deck.size() == deck && passage.run.hp == 40 && passage.run.gold == 100);
        enchant_first(passage, CardType::SKILL, Enchantment::NIMBLE, 2);
    }

    // Read the Back: an Attack gets Sharp 2.
    {
        EventRun back("SELF_HELP_BOOK");
        back.pick(2);
        enchant_first(back, CardType::ATTACK, Enchantment::SHARP, 2);
    }

    // Move On: nothing happens.
    {
        EventRun leave("SELF_HELP_BOOK");
        const size_t deck = leave.run.master_deck.size();
        leave.pick(3);
        assert(!leave.run.has_deck_choice() && leave.run.master_deck.size() == deck);
        assert(leave.run.hp == 40 && leave.run.max_hp == 80 && leave.run.gold == 100 && leave.run.potions.empty());
        assert(leave.run.phase == RunPhase::MAP_NAVIGATION);
    }

    std::cout << "[PASS] Self-Help Book: Swift on a Power / Nimble on a Skill / Sharp on an Attack / leave" << std::endl;
}
