#pragma once

#include <cstdint>

#include "engine/options.h"

// Pause menu (#10), opened with START in combat and on the map. The list of entries depends on where it was
// opened from; the GBA scene only draws what is computed here and acts on the returned PauseAction.
namespace engine
{

enum class PauseOrigin : uint8_t
{
    MAP,
    COMBAT
};

enum class PauseItem : uint8_t
{
    RESUME,
    DECK,
    RELICS,
    OPTIONS,
    END_TURN,  // combat only: START used to end the turn, the menu keeps that action at hand
    SAVE_QUIT, // map only, between rooms: the run save ("Continuer") never holds a fight in progress
    ABANDON
};

// What the scene must do after a button press in the menu.
enum class PauseAction : uint8_t
{
    NONE,
    RESUME,
    DECK,
    RELICS,
    OPTIONS,
    END_TURN,
    SAVE_QUIT,
    ABANDON
};

inline constexpr int PAUSE_MAX_ITEMS = 7;

struct PauseMenu
{
    PauseItem items[PAUSE_MAX_ITEMS] = {};
    int count = 0;
    int cursor = 0;
    bool confirming = false; // "Abandonner la partie ?" is on screen
    bool confirm_yes = false; // cursor of the confirmation: No by default
};

[[nodiscard]] constexpr PauseMenu make_pause_menu(PauseOrigin origin, bool has_relics, bool can_save = true)
{
    PauseMenu menu;
    menu.items[menu.count++] = PauseItem::RESUME;

    if(origin == PauseOrigin::COMBAT)
    {
        menu.items[menu.count++] = PauseItem::END_TURN;
    }

    menu.items[menu.count++] = PauseItem::DECK;

    if(has_relics)
    {
        menu.items[menu.count++] = PauseItem::RELICS;
    }

    menu.items[menu.count++] = PauseItem::OPTIONS;

    if(origin == PauseOrigin::MAP && can_save)
    {
        menu.items[menu.count++] = PauseItem::SAVE_QUIT;
    }

    menu.items[menu.count++] = PauseItem::ABANDON;
    return menu;
}

[[nodiscard]] constexpr PauseAction action_for(PauseItem item)
{
    switch(item)
    {
    case PauseItem::DECK:
        return PauseAction::DECK;
    case PauseItem::RELICS:
        return PauseAction::RELICS;
    case PauseItem::OPTIONS:
        return PauseAction::OPTIONS;
    case PauseItem::END_TURN:
        return PauseAction::END_TURN;
    case PauseItem::SAVE_QUIT:
        return PauseAction::SAVE_QUIT;
    case PauseItem::ABANDON:
        return PauseAction::ABANDON;
    default:
        return PauseAction::RESUME;
    }
}

// Applies one button press. START and B resume the game (or close the confirmation), the cross moves the cursor
// (it wraps around), A picks the entry. Abandoning asks first: only A on "Oui" returns ABANDON.
[[nodiscard]] constexpr PauseAction press_pause(PauseMenu& menu, Button button)
{
    if(menu.confirming)
    {
        switch(button)
        {
        case Button::LEFT:
        case Button::RIGHT:
        case Button::UP:
        case Button::DOWN:
            menu.confirm_yes = ! menu.confirm_yes;
            return PauseAction::NONE;
        case Button::A:
            if(menu.confirm_yes)
            {
                return PauseAction::ABANDON;
            }
            menu.confirming = false;
            return PauseAction::NONE;
        case Button::B:
        case Button::START:
            menu.confirming = false;
            return PauseAction::NONE;
        default:
            return PauseAction::NONE;
        }
    }

    switch(button)
    {
    case Button::UP:
        menu.cursor = (menu.cursor + menu.count - 1) % menu.count;
        return PauseAction::NONE;
    case Button::DOWN:
        menu.cursor = (menu.cursor + 1) % menu.count;
        return PauseAction::NONE;
    case Button::B:
    case Button::START:
        return PauseAction::RESUME;
    case Button::A:
    {
        PauseItem item = menu.items[menu.cursor];

        if(item == PauseItem::ABANDON)
        {
            menu.confirming = true;
            menu.confirm_yes = false;
            return PauseAction::NONE;
        }

        return action_for(item);
    }
    default:
        return PauseAction::NONE;
    }
}

[[nodiscard]] constexpr const char* pause_label(PauseItem item)
{
    switch(item)
    {
    case PauseItem::RESUME:
        return "Reprendre";
    case PauseItem::DECK:
        return "Deck actuel";
    case PauseItem::RELICS:
        return "Reliques";
    case PauseItem::OPTIONS:
        return "Options";
    case PauseItem::END_TURN:
        return "Finir le tour";
    case PauseItem::SAVE_QUIT:
        return "Sauvegarder et quitter";
    default:
        return "Abandonner la partie";
    }
}

} // namespace engine
