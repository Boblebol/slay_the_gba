#pragma once

namespace gba
{

// Sprite z orders (lower is drawn on top). Butano groups sprites by unique z order, so keep few of them.
constexpr int Z_BACK = 8;
constexpr int Z_ACTOR = 6;
constexpr int Z_HUD = 4;
constexpr int Z_FX = 2;
constexpr int Z_CARD = 1;
constexpr int Z_CARD_FOCUS = 0;
constexpr int Z_TEXT = -2;
constexpr int Z_OVERLAY = -4;

// Screen geometry (Butano coordinates: origin at the screen centre).
constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 160;
constexpr int HALF_W = SCREEN_W / 2;
constexpr int HALF_H = SCREEN_H / 2;

}
