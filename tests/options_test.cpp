// Options screen (#21): volumes, button presets, legend of the GBA buttons and their save in SRAM.
#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include "engine/options.h"
#include "engine/save_system.h"

using namespace engine;

namespace {

class MemoryStorage final : public SaveStorage {
public:
    std::vector<uint8_t> bytes = std::vector<uint8_t>(32 * 1024, 0xFF);
    void read(void* destination, size_t size, size_t offset) override { std::memcpy(destination, bytes.data() + offset, size); }
    void write(const void* source, size_t size, size_t offset) override { std::memcpy(bytes.data() + offset, source, size); }
};

void test_volumes_are_bounded() {
    GameSettings settings;
    for (int i = 0; i < 20; ++i) adjust_option(settings, OptionRow::MUSIC, 1);
    assert(settings.music_volume == VOLUME_MAX);
    for (int i = 0; i < 20; ++i) adjust_option(settings, OptionRow::MUSIC, -1);
    assert(settings.music_volume == 0); // off
    adjust_option(settings, OptionRow::SFX, -1);
    assert(settings.sfx_volume == 7);
    assert(settings.music_volume == 0); // the rows are independent
    std::cout << "  volumes: OK\n";
}

void test_preset_changes_buttons_and_wraps() {
    GameSettings settings;
    assert(control_preset(settings) == ControlPreset::CLASSIC);
    assert(shortcut_for(ControlPreset::CLASSIC, Button::L) == Shortcut::POTIONS);
    assert(shortcut_for(ControlPreset::CLASSIC, Button::R) == Shortcut::END_TURN);
    adjust_option(settings, OptionRow::CONTROLS, 1);
    assert(control_preset(settings) == ControlPreset::QUICK);
    assert(shortcut_for(ControlPreset::QUICK, Button::L) == Shortcut::END_TURN);
    assert(shortcut_for(ControlPreset::QUICK, Button::R) == Shortcut::POTIONS);
    for (ControlPreset preset : { ControlPreset::CLASSIC, ControlPreset::QUICK }) {
        assert(shortcut_for(preset, Button::START) == Shortcut::END_TURN);
        assert(shortcut_for(preset, Button::SELECT) == Shortcut::DECK);
        assert(shortcut_for(preset, Button::A) == Shortcut::NONE);
    }
    adjust_option(settings, OptionRow::CONTROLS, 1);
    assert(control_preset(settings) == ControlPreset::CLASSIC);
    adjust_option(settings, OptionRow::CONTROLS, -1);
    assert(control_preset(settings) == ControlPreset::QUICK);
    std::cout << "  presets: OK\n";
}

void test_legend_follows_preset() {
    for (int i = 0; i < LEGEND_LINE_COUNT; ++i) assert(legend_line(ControlPreset::CLASSIC, i)[0] != '\0');
    assert(std::strcmp(legend_line(ControlPreset::CLASSIC, 0), legend_line(ControlPreset::QUICK, 0)) != 0);
    // Every button of the GBA appears in the legend.
    const char* needles[] = { "L :", "R :", "SELECT", "START", "Croix", "A :", "B :" };
    for (const char* needle : needles) {
        bool found = false;
        for (int i = 0; i < LEGEND_LINE_COUNT; ++i) {
            found = found || std::strstr(legend_line(ControlPreset::QUICK, i), needle) != nullptr;
        }
        assert(found);
    }
    std::cout << "  legend: OK\n";
}

void test_saved_in_sram() {
    MemoryStorage storage;
    SaveManager& store = SaveManager::instance();
    store.set_storage(&storage);
    store.reset_to_defaults();
    adjust_option(store.settings(), OptionRow::MUSIC, -3);
    adjust_option(store.settings(), OptionRow::SFX, -8);
    adjust_option(store.settings(), OptionRow::CONTROLS, 1);
    store.commit_to_storage();

    store.reset_to_defaults();
    assert(store.settings().music_volume == 8 && store.settings().control_preset == 0);
    assert(store.load_from_storage());
    assert(store.settings().music_volume == 5);
    assert(store.settings().sfx_volume == 0);
    assert(control_preset(store.settings()) == ControlPreset::QUICK);

    // A cartridge holding values out of range loads with them brought back in.
    store.settings().music_volume = 200;
    store.settings().control_preset = 9;
    store.commit_to_storage();
    store.reset_to_defaults();
    assert(store.load_from_storage());
    assert(store.settings().music_volume == VOLUME_MAX);
    assert(store.settings().control_preset == 0);

    store.set_storage(nullptr);
    store.reset_to_defaults();
    std::cout << "  SRAM save: OK\n";
}

} // namespace

int main() {
    std::cout << "options test\n";
    test_volumes_are_bounded();
    test_preset_changes_buttons_and_wraps();
    test_legend_follows_preset();
    test_saved_in_sram();
    std::cout << "ALL OK\n";
    return 0;
}
