#include "engine/title_menu.h"
#include <algorithm>

namespace engine {

void TitleMenuManager::init(CharacterId hero) {
    current_screen = MenuScreen::TITLE_SCREEN;
    selected_index = 0;
    fixed_hero = hero;
    selected_palette = PaletteVariant::CLASSIC;
    selected_ascension = 0;
    SaveManager::instance().init();
}

bool TitleMenuManager::has_continuable_run() const {
    const auto& p = SaveManager::instance().get_active_profile();
    return p.active_run.has_active_run;
}

std::string_view TitleMenuManager::get_current_screen_name() const {
    switch (current_screen) {
        case MenuScreen::TITLE_SCREEN:      return "Écran Titre";
        case MenuScreen::PROFILE_SELECT:    return "Choix du Profil";
        case MenuScreen::MAIN_MENU:         return "Menu Principal";
        case MenuScreen::PALETTE_SELECT:    return "Choix de la Teinte (Couleur)";
        case MenuScreen::ASCENSION_SELECT:  return "Niveau d'Ascension";
        case MenuScreen::COMPENDIUM_MENU:   return "Encyclopédie de la Tour";
        case MenuScreen::COMPENDIUM_CARDS:   return "Codex des Cartes";
        case MenuScreen::COMPENDIUM_RELICS:  return "Codex des Reliques";
        case MenuScreen::COMPENDIUM_MONSTERS:return "Bestiaire des Monstres";
        case MenuScreen::COMPENDIUM_POTIONS: return "Alchimie des Potions";
        case MenuScreen::COMPENDIUM_ANCIENTS:return "Tome des Anciens";
        case MenuScreen::STATISTICS:        return "Statistiques de Jeu";
        case MenuScreen::IN_RUN:            return "Tentative en Cours";
        default:                            break;
    }
    return "";
}

void TitleMenuManager::navigate_up() {
    if (selected_index > 0) {
        selected_index--;
    }
}

void TitleMenuManager::navigate_down() {
    int max_items = 3;
    switch (current_screen) {
        case MenuScreen::PROFILE_SELECT:
            max_items = 3;
            break;
        case MenuScreen::MAIN_MENU:
            max_items = has_continuable_run() ? 5 : 4;
            break;
        case MenuScreen::PALETTE_SELECT:
            max_items = 4;
            break;
        case MenuScreen::ASCENSION_SELECT: {
            uint8_t cid = static_cast<uint8_t>(fixed_hero);
            max_items = SaveManager::instance().get_active_profile().max_ascension[cid] + 1;
            break;
        }
        case MenuScreen::COMPENDIUM_MENU:
            max_items = 5;
            break;
        default:
            max_items = 1;
            break;
    }

    if (selected_index + 1 < max_items) {
        selected_index++;
    }
}

void TitleMenuManager::navigate_left() {
    navigate_up();
}

void TitleMenuManager::navigate_right() {
    navigate_down();
}

void TitleMenuManager::press_confirm() {
    switch (current_screen) {
        case MenuScreen::TITLE_SCREEN:
            current_screen = MenuScreen::PROFILE_SELECT;
            selected_index = 0;
            break;

        case MenuScreen::PROFILE_SELECT:
            SaveManager::instance().select_profile(static_cast<uint8_t>(selected_index));
            current_screen = MenuScreen::MAIN_MENU;
            selected_index = 0;
            break;

        case MenuScreen::MAIN_MENU: {
            bool cont = has_continuable_run();
            if (cont && selected_index == 0) {
                continue_active_run();
                current_screen = MenuScreen::IN_RUN;
            } else {
                int action = cont ? (selected_index - 1) : selected_index;
                if (action == 0) {
                    current_screen = MenuScreen::PALETTE_SELECT;
                    selected_index = 0;
                } else if (action == 1) {
                    current_screen = MenuScreen::COMPENDIUM_MENU;
                    selected_index = 0;
                } else if (action == 2) {
                    current_screen = MenuScreen::STATISTICS;
                    selected_index = 0;
                } else if (action == 3) {
                    current_screen = MenuScreen::PROFILE_SELECT;
                    selected_index = 0;
                }
            }
            break;
        }

        case MenuScreen::PALETTE_SELECT:
            selected_palette = static_cast<PaletteVariant>(selected_index);
            current_screen = MenuScreen::ASCENSION_SELECT;
            selected_index = 0;
            break;

        case MenuScreen::ASCENSION_SELECT:
            selected_ascension = static_cast<uint8_t>(selected_index);
            start_selected_run();
            current_screen = MenuScreen::IN_RUN;
            break;

        case MenuScreen::COMPENDIUM_MENU:
            if (selected_index == 0) current_screen = MenuScreen::COMPENDIUM_CARDS;
            else if (selected_index == 1) current_screen = MenuScreen::COMPENDIUM_RELICS;
            else if (selected_index == 2) current_screen = MenuScreen::COMPENDIUM_MONSTERS;
            else if (selected_index == 3) current_screen = MenuScreen::COMPENDIUM_POTIONS;
            else if (selected_index == 4) current_screen = MenuScreen::COMPENDIUM_ANCIENTS;
            selected_index = 0;
            break;

        default:
            break;
    }
}

void TitleMenuManager::press_back() {
    switch (current_screen) {
        case MenuScreen::PROFILE_SELECT:
            current_screen = MenuScreen::TITLE_SCREEN;
            selected_index = 0;
            break;
        case MenuScreen::MAIN_MENU:
            current_screen = MenuScreen::PROFILE_SELECT;
            selected_index = 0;
            break;
        case MenuScreen::PALETTE_SELECT:
            current_screen = MenuScreen::MAIN_MENU;
            selected_index = 0;
            break;
        case MenuScreen::ASCENSION_SELECT:
            current_screen = MenuScreen::PALETTE_SELECT;
            selected_index = static_cast<int>(selected_palette);
            break;
        case MenuScreen::COMPENDIUM_MENU:
        case MenuScreen::STATISTICS:
            current_screen = MenuScreen::MAIN_MENU;
            selected_index = 0;
            break;
        case MenuScreen::COMPENDIUM_CARDS:
        case MenuScreen::COMPENDIUM_RELICS:
        case MenuScreen::COMPENDIUM_MONSTERS:
        case MenuScreen::COMPENDIUM_POTIONS:
        case MenuScreen::COMPENDIUM_ANCIENTS:
            current_screen = MenuScreen::COMPENDIUM_MENU;
            selected_index = 0;
            break;
        default:
            break;
    }
}

void TitleMenuManager::start_selected_run() {
    active_run.start_run(fixed_hero, selected_ascension, selected_palette, 777);
    SaveManager::instance().autosave_run(active_run);
}

void TitleMenuManager::continue_active_run() {
    SaveManager::instance().restore_run(active_run);
}

void TitleMenuManager::abandon_active_run() {
    SaveManager::instance().record_defeat();
    current_screen = MenuScreen::MAIN_MENU;
    selected_index = 0;
}

} // namespace engine
