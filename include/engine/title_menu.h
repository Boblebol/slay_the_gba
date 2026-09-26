#pragma once
#include <cstdint>
#include <string_view>
#include "engine/save_system.h"
#include "engine/run.h"

namespace engine {

enum class MenuScreen : uint8_t {
    TITLE_SCREEN,
    PROFILE_SELECT,
    MAIN_MENU,
    PALETTE_SELECT, // Choix de la couleur / teinte (Pokémon style)
    ASCENSION_SELECT,
    COMPENDIUM_MENU,
    COMPENDIUM_CARDS,
    COMPENDIUM_RELICS,
    COMPENDIUM_MONSTERS,
    COMPENDIUM_POTIONS,
    COMPENDIUM_ANCIENTS,
    STATISTICS,
    IN_RUN
};

class TitleMenuManager {
public:
    MenuScreen current_screen = MenuScreen::TITLE_SCREEN;
    int selected_index = 0;
    CharacterId fixed_hero = CharacterId::IRONCLAD; // Fixed by ROM edition
    PaletteVariant selected_palette = PaletteVariant::CLASSIC;
    uint8_t selected_ascension = 0;

    RunManager active_run;

    void init(CharacterId hero = CharacterId::IRONCLAD);
    void press_confirm(); // A or START
    void navigate_up();
    void navigate_down();
    void navigate_left();
    void navigate_right();
    void press_back();    // B

    bool has_continuable_run() const;
    std::string_view get_current_screen_name() const;
    void start_selected_run();
    void continue_active_run();
    void abandon_active_run();
};

} // namespace engine
