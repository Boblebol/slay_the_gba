#pragma once

#include <cstdint>

#include "engine/save_system.h"

// Options screen (#21): audio volumes, button preset and the legend of the GBA buttons. The values live in
// GameSettings, which the save already stores in SRAM; the GBA scene only draws what is computed here.
namespace engine
{

inline constexpr uint8_t VOLUME_MAX = 10;

enum class ControlPreset : uint8_t
{
    CLASSIC, // L opens the potion belt, R ends the turn
    QUICK,   // L ends the turn, R opens the potion belt (both hands on the shoulder buttons)
    COUNT
};

enum class Button : uint8_t
{
    A, B, L, R, START, SELECT, UP, DOWN, LEFT, RIGHT
};

// What a shoulder or system button does in combat, whatever the preset.
enum class Shortcut : uint8_t
{
    NONE,
    END_TURN,
    POTIONS,
    DECK,
    PAUSE // START opens the pause menu (#10), which can still end the turn
};

enum class OptionRow : uint8_t
{
    MUSIC,
    SFX,
    CONTROLS,
    COUNT
};

inline constexpr int OPTION_ROW_COUNT = static_cast<int>(OptionRow::COUNT);

[[nodiscard]] constexpr ControlPreset control_preset(const GameSettings& settings)
{
    return settings.control_preset < static_cast<uint8_t>(ControlPreset::COUNT) ?
               static_cast<ControlPreset>(settings.control_preset) : ControlPreset::CLASSIC;
}

// Brings values read from a cartridge back in range.
constexpr void sanitize_settings(GameSettings& settings)
{
    if(settings.music_volume > VOLUME_MAX) settings.music_volume = VOLUME_MAX;
    if(settings.sfx_volume > VOLUME_MAX) settings.sfx_volume = VOLUME_MAX;
    settings.control_preset = static_cast<uint8_t>(control_preset(settings));
}

[[nodiscard]] constexpr Shortcut shortcut_for(ControlPreset preset, Button button)
{
    bool quick = preset == ControlPreset::QUICK;

    switch(button)
    {
    case Button::L:
        return quick ? Shortcut::END_TURN : Shortcut::POTIONS;
    case Button::R:
        return quick ? Shortcut::POTIONS : Shortcut::END_TURN;
    case Button::START:
        return Shortcut::PAUSE;
    case Button::SELECT:
        return Shortcut::DECK;
    default:
        return Shortcut::NONE;
    }
}

// Moves a row by `step` (-1 or +1): volumes stop at 0 (off) and VOLUME_MAX, the preset wraps around.
constexpr void adjust_option(GameSettings& settings, OptionRow row, int step)
{
    sanitize_settings(settings);

    switch(row)
    {
    case OptionRow::MUSIC:
    case OptionRow::SFX:
    {
        uint8_t& volume = row == OptionRow::MUSIC ? settings.music_volume : settings.sfx_volume;
        int value = static_cast<int>(volume) + step;
        volume = static_cast<uint8_t>(value < 0 ? 0 : (value > VOLUME_MAX ? VOLUME_MAX : value));
        break;
    }
    case OptionRow::CONTROLS:
    {
        int count = static_cast<int>(ControlPreset::COUNT);
        settings.control_preset = static_cast<uint8_t>((settings.control_preset + count + step) % count);
        break;
    }
    default:
        break;
    }
}

[[nodiscard]] constexpr const char* preset_name(ControlPreset preset)
{
    return preset == ControlPreset::QUICK ? "Rapide" : "Classique";
}

[[nodiscard]] constexpr const char* option_label(OptionRow row)
{
    switch(row)
    {
    case OptionRow::MUSIC:
        return "Musique";
    case OptionRow::SFX:
        return "Effets";
    default:
        return "Commandes";
    }
}

// Legend of the GBA drawn as text: one line per group of buttons, for the chosen preset.
inline constexpr int LEGEND_LINE_COUNT = 5;

[[nodiscard]] constexpr const char* legend_line(ControlPreset preset, int index)
{
    bool quick = preset == ControlPreset::QUICK;

    switch(index)
    {
    case 0:
        return quick ? "L : fin du tour    R : potions" : "L : potions    R : fin du tour";
    case 1:
        return "SELECT : deck    START : pause";
    case 2:
        return "Croix : choisir carte ou cible";
    case 3:
        return "A : valider / jouer";
    default:
        return "B : retour / annuler";
    }
}

// Key reminders of the combat screen, for the chosen preset.
[[nodiscard]] constexpr const char* end_turn_reminder(ControlPreset preset)
{
    return preset == ControlPreset::QUICK ? "L : fin du tour" : "R : fin du tour";
}

[[nodiscard]] constexpr const char* potions_reminder(ControlPreset preset)
{
    return preset == ControlPreset::QUICK ? "R : potions" : "L : potions";
}

[[nodiscard]] constexpr const char* belt_reminder(ControlPreset preset)
{
    return preset == ControlPreset::QUICK ? "R : potions   L : fin du tour" : "L : potions   R : fin du tour";
}

} // namespace engine
