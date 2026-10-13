// Pause menu (#10): entries in combat and on the map, START/B to resume, confirmation before abandoning.
#include <cassert>
#include <cstring>
#include <iostream>
#include "engine/pause_menu.h"

using namespace engine;

namespace {

bool has_item(const PauseMenu& menu, PauseItem item) {
    for (int i = 0; i < menu.count; ++i) if (menu.items[i] == item) return true;
    return false;
}

int index_of(const PauseMenu& menu, PauseItem item) {
    for (int i = 0; i < menu.count; ++i) if (menu.items[i] == item) return i;
    return -1;
}

void test_start_opens_the_pause() {
    for (ControlPreset preset : { ControlPreset::CLASSIC, ControlPreset::QUICK }) {
        assert(shortcut_for(preset, Button::START) == Shortcut::PAUSE);
        // The turn still ends with a shoulder button, whatever the preset.
        assert(shortcut_for(preset, Button::L) == Shortcut::END_TURN || shortcut_for(preset, Button::R) == Shortcut::END_TURN);
    }
    assert(std::strstr(end_turn_reminder(ControlPreset::CLASSIC), "R :") != nullptr);
    assert(std::strstr(end_turn_reminder(ControlPreset::QUICK), "L :") != nullptr);
    std::cout << "  start pauses: OK\n";
}

void test_entries_by_origin() {
    PauseMenu combat = make_pause_menu(PauseOrigin::COMBAT, true);
    assert(combat.items[0] == PauseItem::RESUME);
    assert(has_item(combat, PauseItem::DECK) && has_item(combat, PauseItem::RELICS) && has_item(combat, PauseItem::OPTIONS));
    assert(has_item(combat, PauseItem::END_TURN));
    assert(! has_item(combat, PauseItem::SAVE_QUIT)); // no save in the middle of a fight
    assert(combat.items[combat.count - 1] == PauseItem::ABANDON);

    PauseMenu map = make_pause_menu(PauseOrigin::MAP, true);
    assert(has_item(map, PauseItem::SAVE_QUIT));
    assert(! has_item(map, PauseItem::END_TURN));
    assert(map.items[map.count - 1] == PauseItem::ABANDON);

    PauseMenu bare = make_pause_menu(PauseOrigin::MAP, false, false);
    assert(! has_item(bare, PauseItem::RELICS));
    assert(! has_item(bare, PauseItem::SAVE_QUIT)); // the map is not between two rooms

    for (int i = 0; i < map.count; ++i) assert(pause_label(map.items[i])[0] != '\0');
    std::cout << "  entries: OK\n";
}

void test_resume_buttons_and_cursor() {
    PauseMenu menu = make_pause_menu(PauseOrigin::MAP, true);
    assert(press_pause(menu, Button::START) == PauseAction::RESUME);
    assert(press_pause(menu, Button::B) == PauseAction::RESUME);
    assert(press_pause(menu, Button::A) == PauseAction::RESUME); // cursor on "Reprendre"
    assert(press_pause(menu, Button::UP) == PauseAction::NONE);
    assert(menu.cursor == menu.count - 1); // wraps to the last entry
    assert(press_pause(menu, Button::DOWN) == PauseAction::NONE);
    assert(menu.cursor == 0);
    menu.cursor = index_of(menu, PauseItem::DECK);
    assert(press_pause(menu, Button::A) == PauseAction::DECK);
    menu.cursor = index_of(menu, PauseItem::RELICS);
    assert(press_pause(menu, Button::A) == PauseAction::RELICS);
    menu.cursor = index_of(menu, PauseItem::OPTIONS);
    assert(press_pause(menu, Button::A) == PauseAction::OPTIONS);
    menu.cursor = index_of(menu, PauseItem::SAVE_QUIT);
    assert(press_pause(menu, Button::A) == PauseAction::SAVE_QUIT);

    PauseMenu combat = make_pause_menu(PauseOrigin::COMBAT, true);
    combat.cursor = index_of(combat, PauseItem::END_TURN);
    assert(press_pause(combat, Button::A) == PauseAction::END_TURN);
    std::cout << "  resume and cursor: OK\n";
}

void test_abandon_asks_first() {
    PauseMenu menu = make_pause_menu(PauseOrigin::COMBAT, true);
    menu.cursor = index_of(menu, PauseItem::ABANDON);
    assert(press_pause(menu, Button::A) == PauseAction::NONE);
    assert(menu.confirming && ! menu.confirm_yes);
    // A on "Non" closes the question, START or B too, without leaving the menu.
    assert(press_pause(menu, Button::A) == PauseAction::NONE);
    assert(! menu.confirming);
    assert(press_pause(menu, Button::A) == PauseAction::NONE);
    assert(press_pause(menu, Button::START) == PauseAction::NONE);
    assert(! menu.confirming);
    assert(press_pause(menu, Button::A) == PauseAction::NONE);
    assert(press_pause(menu, Button::B) == PauseAction::NONE);
    assert(! menu.confirming);
    // Only "Oui" abandons.
    assert(press_pause(menu, Button::A) == PauseAction::NONE);
    assert(press_pause(menu, Button::RIGHT) == PauseAction::NONE);
    assert(menu.confirm_yes);
    assert(press_pause(menu, Button::A) == PauseAction::ABANDON);
    std::cout << "  abandon confirmation: OK\n";
}

} // namespace

int main() {
    std::cout << "pause menu test\n";
    test_start_opens_the_pause();
    test_entries_by_origin();
    test_resume_buttons_and_cursor();
    test_abandon_asks_first();
    std::cout << "ALL OK\n";
    return 0;
}
