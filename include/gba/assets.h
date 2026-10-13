#pragma once

#include <string_view>

#include "bn_color.h"
#include "bn_regular_bg_item.h"
#include "bn_sprite_item.h"
#include "bn_string_view.h"

#include "engine/ancient.h"
#include "engine/card.h"
#include "engine/character.h"
#include "engine/entity.h"

namespace gba
{

// Hero of this cartridge (one ROM per hero, selected with -DHERO_*).
[[nodiscard]] engine::CharacterId current_hero();

[[nodiscard]] const bn::sprite_item& hero_battler();
[[nodiscard]] const bn::sprite_item& hero_map_pawn();
[[nodiscard]] const bn::sprite_item& hero_map_cursor();
[[nodiscard]] const bn::sprite_item& hero_hand_cards();
[[nodiscard]] const bn::sprite_item& hero_energy_orb();
[[nodiscard]] const bn::regular_bg_item& hero_battle_bg();
[[nodiscard]] bn::color hero_backdrop_color();
[[nodiscard]] bn::string_view hero_zone_name();
[[nodiscard]] bn::string_view hero_campfire_upgrade();

// Pokemon-like palette variants chosen on the title screen, applied as a palette fade on the hero.
[[nodiscard]] bn::color palette_variant_color(engine::PaletteVariant variant);
[[nodiscard]] bn::fixed palette_variant_intensity(engine::PaletteVariant variant);

// Card art: 64x64 illustrated card (own palette) and the frame index in hero_hand_cards().
[[nodiscard]] const bn::sprite_item& big_card_item(engine::CardId id);
[[nodiscard]] int hand_card_frame(const engine::DeckCard& card);

// Monster art. Bosses are 128x128, split into four 64x64 frames (TL, TR, BL, BR).
struct EnemyArt
{
    const bn::sprite_item* item;
    bool boss;
};

[[nodiscard]] EnemyArt enemy_art(engine::EnemyType type);

[[nodiscard]] const bn::regular_bg_item& event_bg(std::string_view event_id);

// 64x64 portrait of an Ancient (graphics/ancients).
[[nodiscard]] const bn::sprite_item& ancient_portrait(engine::AncientId id);

// Top-left aligned position for a 256xN regular BG showing its first 240x160 pixels.
constexpr int BG_ORIGIN_X = 8;
constexpr int BG_ORIGIN_Y = 48;

}
