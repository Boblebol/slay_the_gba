#pragma once
// Pierre de tous les temps (STONE_OF_ALL_TIME): each option is chosen once, with and without what it needs.
#include <iostream>
#include "event_test_support.h"

inline void test_stone_of_all_time() {
    using namespace event_test;

    const EventDef* def = EventManager::find("STONE_OF_ALL_TIME");
    assert(def && def->acts == EVENT_ALL_ACTS && def->pages.size() == 1 && def->pages[0].options.size() == 3);

    // Without a potion, "Drink and Lift" is greyed out; the Push and the leave option are open.
    {
        EventRun none("STONE_OF_ALL_TIME");
        assert(none.run.potions.empty() && !none.run.current_event.choices[0].enabled);
        assert(none.run.current_event.choices[1].enabled && none.run.current_event.choices[2].enabled);
        assert(!none.run.choose_event_choice(0));
        assert(none.run.phase == RunPhase::EVENT && none.run.max_hp == 80);
    }

    // Drink and Lift: the potion is lost, 10 Max HP gained (the HP rise with them).
    {
        EventRun drink("STONE_OF_ALL_TIME");
        drink.run.potions.push_back("GLOWWATER_POTION");
        drink.run.potions.push_back("FIRE_POTION");
        assert(drink.run.current_event.choices.size() == 3);
        drink.run.start_event("STONE_OF_ALL_TIME");
        assert(drink.run.current_event.choices[0].enabled);
        drink.pick(0);
        assert(drink.run.potions.size() == 1);
        assert(drink.run.max_hp == 80 + 10 && drink.run.hp == 40 + 10 && drink.run.gold == 100);
        assert(!drink.run.has_deck_choice() && drink.run.phase == RunPhase::MAP_NAVIGATION);
    }

    // Push: 6 HP lost, an Attack chosen by the player gets Vigorous 8.
    {
        EventRun push("STONE_OF_ALL_TIME");
        assert(push.run.current_event.choices[1].enabled);
        const size_t deck = push.run.master_deck.size();
        push.pick(1);
        assert(push.run.hp == 40 - 6 && push.run.max_hp == 80 && push.run.gold == 100);
        assert(push.run.has_deck_choice() && push.run.deck_choice.kind == DeckChoiceKind::ENCHANT);
        assert(push.run.deck_choice.enchantment == Enchantment::VIGOROUS && push.run.deck_choice.enchant_amount == 8);
        // Only an Attack can be picked: the Defends of the starter deck cannot.
        int attack = -1;
        int skill = -1;
        for (size_t i = 0; i < push.run.master_deck.size(); ++i) {
            const bool is_attack = CardDef::get(push.run.master_deck[i].id).type == CardType::ATTACK;
            if (is_attack && attack < 0) attack = static_cast<int>(i);
            if (!is_attack && skill < 0) skill = static_cast<int>(i);
        }
        assert(attack >= 0 && skill >= 0 && !push.run.deck_choice_allows(skill) && push.run.deck_choice_allows(attack));
        assert(push.run.resolve_deck_choice(attack));
        assert(push.run.master_deck[attack].enchantment == Enchantment::VIGOROUS && push.run.master_deck[attack].enchant_amount == 8);
        assert(!push.run.has_deck_choice() && push.run.master_deck.size() == deck);
    }

    // Without an Attack in the deck, "Push" is greyed out.
    {
        EventRun nothing("STONE_OF_ALL_TIME");
        for (size_t i = nothing.run.master_deck.size(); i-- > 0;) {
            if (CardDef::get(nothing.run.master_deck[i].id).type == CardType::ATTACK) nothing.run.master_deck.erase(i);
        }
        nothing.run.start_event("STONE_OF_ALL_TIME");
        assert(!nothing.run.current_event.choices[1].enabled && !nothing.run.choose_event_choice(1));
    }

    // Leave: nothing changes.
    {
        EventRun leave("STONE_OF_ALL_TIME");
        const size_t deck = leave.run.master_deck.size();
        leave.pick(2);
        assert(leave.run.hp == 40 && leave.run.max_hp == 80 && leave.run.gold == 100 && leave.run.master_deck.size() == deck);
        assert(leave.run.phase == RunPhase::MAP_NAVIGATION && !leave.run.has_deck_choice());
    }

    std::cout << "[PASS] Stone of All Time: Drink and Lift (potion) / Push (6 HP, Vigorous 8) / leave" << std::endl;
}
