#include "gba/scene_factories.h"

#include <algorithm>
#include <span>

#include "bn_blending.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_optional.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_tiles_ptr.h"
#include "bn_sprites_mosaic.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "bn_sprite_items_fx_burst.h"
#include "bn_sprite_items_fx_slash.h"

#include "engine/compendium.h"
#include "engine/options.h"
#include "engine/potion.h"
#include "engine/save_system.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/card_text.h"
#include "gba/fx.h"
#include "gba/layers.h"
#include "gba/text.h"
#include "gba/widgets.h"

namespace gba
{

namespace
{

constexpr int PLAYER_X = -76;
constexpr int PLAYER_Y = -14;
constexpr int BAR_Y = 19;
constexpr int STATUS_Y = 9;
// Half the screen for a status line: the player's and a lone enemy's then never overlap.
constexpr int STATUS_WIDTH = 112;
constexpr int HAND_Y = 64;
constexpr int HAND_FOCUS_Y = 57;
constexpr int HAND_PICKED_Y = 50;
constexpr int BANNER_Y = -48;
// Prompt of a card choice, centred under the intent icons (lowest bottom edge -36, a boss's) and the pile counters
// (bottom edge -41): the glyphs span -34 to -26, and the widest prompt is 204 px, inside the 240 px screen.
constexpr int CHOICE_BANNER_Y = -30;
constexpr int LINE1_Y = 30;
constexpr int LINE2_Y = 41;
constexpr int LINE_STEP = LINE2_Y - LINE1_Y;
// The key line of the belt: under a three-line description, raised so the bottom edge of the screen does not cut it.
constexpr int BELT_HINT_Y = LINE1_Y + 4 * LINE_STEP - 3;
constexpr int TEXT_WIDTH = 224;
constexpr int STAGE_X = 0;
constexpr int STAGE_Y = -30;

constexpr int INTRO_FRAMES = 46;
constexpr int FLIGHT_GROW = 7;
constexpr int FLIGHT_HOLD = 5;
constexpr int FLIGHT_TRAVEL = 6;
constexpr int FLIGHT_IMPACT = FLIGHT_GROW + FLIGHT_HOLD + FLIGHT_TRAVEL;
constexpr int FLIGHT_END = FLIGHT_IMPACT + 8;
constexpr int ENEMY_TURN_LEAD = 14;
constexpr int ENEMY_STEP = 30;
constexpr int ENEMY_STEP_IMPACT = 9;
constexpr int DEATH_FRAMES = 32;
constexpr int ARRIVE_FRAMES = 24; // a summoned enemy drops into its place
// Whispering Earring: frames Vakuu waits before each card he plays, and before he ends the turn.
constexpr int VAKUU_DELAY = 24;

struct Point
{
    int x;
    int y;
};

// Three enemies or more share the room right of the hero in lanes of equal width, from the left edge of the first
// lane (the hero's HP bar ends at -44 and his widest HP label at -51) to the right edge of the screen.
constexpr int ENEMY_LEFT = -36;
constexpr int SCREEN_EDGE = 119;
// Rightmost centre of a 64 px wide enemy sprite, so that the edge of the screen does not cut it.
constexpr int SPRITE_RIGHT = 88;
// Right edge of the pile counters (top left): an enemy's intent icon and block badge stay right of it.
constexpr int PILE_RIGHT = -16;
// Four to six enemies: the heights of the front and back rows.
constexpr int CROWD_FRONT_Y = -12;
constexpr int CROWD_BACK_Y = -30;
// The back row's statuses are written one line above the front row's.
constexpr int CROWD_STATUS_STEP = 10;
// Full width of an HP bar at scale 1.
constexpr int BAR_WIDTH = 64;

int enemy_lane_width(int count)
{
    return (SCREEN_EDGE + 1 - ENEMY_LEFT) / bn::max(count, 1);
}

int enemy_bar_x(int index, int count, bool boss);

Point enemy_slot(int index, int count, bool boss)
{
    if(boss)
    {
        return { 56, -52 };
    }

    if(count <= 1)
    {
        return { 54, -20 };
    }

    if(count == 2)
    {
        return index == 0 ? Point{ 22, -20 } : Point{ 86, -22 };
    }

    // Three or more: the sprites spread evenly from the first lane to the last one, held back from the right edge.
    index = bn::clamp(index, 0, count - 1);
    int first = enemy_bar_x(0, count, false);
    int last = bn::min(enemy_bar_x(count - 1, count, false), SPRITE_RIGHT);
    int x = first + (last - first) * index / (count - 1);

    if(count == 3)
    {
        return { x, index == 1 ? -24 : -18 };
    }

    // Four to six: every other one a row further back (higher up), so that two neighbours never draw their intent
    // at the same height.
    return { x, index % 2 ? CROWD_BACK_Y : CROWD_FRONT_Y };
}

// Centre of an enemy's HP bar and statuses: under its sprite for one or two enemies, else the middle of its lane.
int enemy_bar_x(int index, int count, bool boss)
{
    if(boss || count <= 2)
    {
        return enemy_slot(index, count, boss).x;
    }

    return ENEMY_LEFT + (2 * index + 1) * (SCREEN_EDGE + 1 - ENEMY_LEFT) / (2 * count);
}

// Width of an enemy's HP bar, by the number of enemies in the row: a few pixels narrower than its lane.
bn::fixed enemy_bar_scale(int count)
{
    if(count <= 2)
    {
        return bn::fixed(1);
    }

    return bn::fixed(bn::min(enemy_lane_width(count) - 6, BAR_WIDTH)) / BAR_WIDTH;
}

// Width of the label of an HP bar at `max_hp` out of `max_hp`, or of `max_hp` alone when compact.
int hp_label_width(int max_hp, bool compact)
{
    bn::string<16> text;
    bn::ostringstream stream(text);
    stream << max_hp;

    if(! compact)
    {
        stream << '/' << max_hp;
    }

    return text_width(text);
}

// Whether an enemy's HP label shows its HP alone: "190/190" does not fit the lane of one of four enemies.
bool enemy_hp_compact(int count, int max_hp)
{
    return count > 2 && hp_label_width(max_hp, false) > enemy_lane_width(count) - 2;
}

struct Span
{
    int left;
    int right;
};

// Room for the statuses of an enemy, from halfway to its neighbour on the left to halfway to the one on its right.
// With four enemies or more, the front and back rows write them on two lines: the neighbours are those of its row.
Span enemy_status_span(int index, int count, bool boss)
{
    int step = count > 3 ? 2 : 1;
    int x = enemy_bar_x(index, count, boss);
    Span span = { ENEMY_LEFT, SCREEN_EDGE };

    if(index - step >= 0)
    {
        span.left = (enemy_bar_x(index - step, count, boss) + x) / 2 + 2;
    }

    if(index + step < count)
    {
        span.right = (x + enemy_bar_x(index + step, count, boss)) / 2 - 2;
    }

    // A lone enemy keeps half the screen, as the hero does.
    if(span.right - span.left > STATUS_WIDTH)
    {
        span.left = bn::max(span.left, bn::min(x - STATUS_WIDTH / 2, span.right - STATUS_WIDTH));
        span.right = span.left + STATUS_WIDTH;
    }

    return span;
}

int enemy_status_y(int index, int count)
{
    return count > 3 && index % 2 ? STATUS_Y - CROWD_STATUS_STEP : STATUS_Y;
}

// Centre of a line `width` pixels wide, as close to `wanted_x` as `span` lets it.
int place_in_span(int wanted_x, int width, Span span)
{
    int half = (width + 1) / 2;

    if(2 * half >= span.right - span.left)
    {
        return (span.left + span.right) / 2;
    }

    return bn::clamp(wanted_x, span.left + half, span.right - half);
}

// Centre of an enemy's intent icon: above its head (beside it for a boss), right of the pile counters.
int enemy_intent_x(int slot_x, bool boss)
{
    return boss ? slot_x - 62 : bn::max(slot_x - 4, PILE_RIGHT + 8);
}

// Half the width of a block badge: the shield icon (16 px), or its number when wider.
int block_badge_half(const bn::string_view& text)
{
    return bn::max(8, (text_width(text) + 1) / 2);
}

// Offsets of the four 64x64 quarters of a 128x128 boss sprite.
constexpr Point boss_parts[] = { { -32, -32 }, { 32, -32 }, { -32, 32 }, { 32, 32 } };

// Shortcut of the button pressed this frame, following the control preset chosen in the options.
[[nodiscard]] engine::Shortcut pressed_shortcut()
{
    engine::ControlPreset preset = engine::control_preset(engine::SaveManager::instance().settings());

    if(bn::keypad::select_pressed())
    {
        return engine::shortcut_for(preset, engine::Button::SELECT);
    }

    if(bn::keypad::l_pressed())
    {
        return engine::shortcut_for(preset, engine::Button::L);
    }

    if(bn::keypad::start_pressed())
    {
        return engine::shortcut_for(preset, engine::Button::START);
    }

    if(bn::keypad::r_pressed())
    {
        return engine::shortcut_for(preset, engine::Button::R);
    }

    return engine::Shortcut::NONE;
}

class CombatScene final : public Scene
{
public:
    explicit CombatScene(GameContext& context);
    ~CombatScene() final;

    [[nodiscard]] bn::optional<SceneId> update() final;

private:
    enum class Phase : uint8_t
    {
        INTRO,
        PLAYER,
        TARGET,
        POTIONS,
        CHOICE,
        CARD_FLIGHT,
        ENEMY_TURN,
        END_BANNER
    };

    struct EnemyView
    {
        EnemyView(int slot_x, int slot_y, int bar_center, bool is_boss) :
            bar(bar_center, BAR_Y),
            intent_icon(create_icon(Icon::ATTACK, slot_x, slot_y)),
            x(slot_x),
            y(slot_y),
            bar_x(bar_center),
            intent_x(enemy_intent_x(slot_x, is_boss)),
            boss(is_boss)
        {
        }

        bn::vector<bn::sprite_ptr, 4> parts;
        HpBar bar;
        bn::sprite_ptr intent_icon;
        Label intent_label;
        Label status_label;
        Label block_label;
        bn::optional<bn::sprite_ptr> block_icon;
        TextLine status_text;
        int x;
        int y;
        // Centre of the HP bar, its label and the statuses: the middle of the enemy's lane.
        int bar_x;
        // Centre of the intent icon, moved so that the intent and the block badge beside it stay on screen.
        int intent_x;
        int shown_block = -1;
        int shown_icon = -1;
        int hurt = 0;
        int lunge = 0;
        int death = 0;
        int arrive = 0;
        bool boss;
        bool gone = false;
        bool intent_visible = true;
    };

    struct HandCardView
    {
        engine::DeckCard card;
        bn::sprite_ptr sprite;
        bn::fixed x;
        bn::fixed y;
        int delay = 0;
        bool dim = false;
    };

    GameContext& _ctx;
    engine::CombatEngine& _combat;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _player;
    HpBar _player_bar;
    bn::sprite_ptr _energy_orb;
    bn::sprite_ptr _target_cursor;
    bn::color _dim_colors[16];
    bn::sprite_palette_ptr _dim_palette;
    bn::vector<EnemyView, engine::MAX_ENEMIES> _enemies;
    bn::vector<HandCardView, 10> _hand;
    bn::optional<bn::sprite_ptr> _flying_card;
    // Choice the engine is asking: marked cards (indices in the hand or in the pile the choice is about), and,
    // for a pile, the order its cards are shown in and the first one of the ten on screen.
    bn::vector<uint8_t, 50> _picked;
    bn::vector<uint8_t, 50> _choice_order;
    int _pile_offset = 0;
    bool _pile_view = false;
    bn::optional<bn::sprite_ptr> _player_block_icon;

    Label _energy_label;
    Label _resource_label;
    Label _pile_label;
    Label _discard_label;
    Label _exhaust_label;
    Label _hint_label;
    Label _line1;
    Label _line1_right;
    Label _line2;
    Label _banner;
    Label _player_status;
    Label _player_block_label;
    // Potion belt: two more description lines and the key reminder under them.
    Label _line3;
    // Power an enemy move just put on the hero: its description is shown while the enemies play.
    engine::PowerId _landed_power = engine::PowerId::NONE;
    Label _line4;
    Label _belt_hint;

    PopupPool _popups;
    FxPool _fx;
    Shake _shake;

    Phase _phase = Phase::INTRO;
    SceneId _overlay = SceneId::DECK; // deck viewer or pause menu, opened over the fight with a quick fade
    int _timer = 0;
    // Whispering Earring: frames since Vakuu's last card, and whether his turn has been announced.
    int _vakuu_timer = 0;
    bool _vakuu_announced = false;
    int _selected = 0;
    int _target = 0;
    int _player_hurt = 0;
    int _player_lunge = 0;
    int _player_guard = 0;
    int _player_death = 0;
    int _banner_timer = 0;
    int _banner_y = -48;
    bool _victory = false;

    // Potion belt: the potion under the cursor, the one shown, and the one waiting for its target (-1: none).
    int _potion = 0;
    int _belt_shown = -1;
    int _aimed_potion = -1;

    // Card flight
    engine::DeckCard _flying;
    int _flying_hand_index = 0;
    int _flying_target = 0;
    bn::fixed _flight_x0;
    bn::fixed _flight_y0;

    // Snapshot taken before an engine action, diffed afterwards to drive the animations.
    int _snap_player_hp = 0;
    int _snap_player_block = 0;
    int _snap_hp[engine::MAX_ENEMIES] = {};
    int _snap_block[engine::MAX_ENEMIES] = {};

    // Cached HUD values so labels regenerate only on change.
    int _shown_energy = -1;
    int _shown_player_block = -1;
    int _shown_draw = -1;
    int _shown_discard = -1;
    int _shown_exhaust = -1;
    int _shown_resource = -1;
    int _selected_shown = -1;
    int _hand_size_shown = -1;
    TextLine _player_status_text;

    void _build_enemies();
    void _create_enemy_parts(EnemyView& view, const EnemyArt& art);
    void _summon_enemies(int summoner, unsigned summoned);
    void _layout_enemies();
    void _build_hand(bool deal);
    void _layout_hand();
    void _refresh_card_text();
    void _refresh_hud();
    void _refresh_enemy_hud(int index);
    void _update_enemies();
    void _update_player();
    void _update_cursor();
    void _apply_player_tint();

    void _update_intro();
    void _update_player_input();
    void _update_vakuu_turn();
    void _update_target_input();
    void _open_belt();
    void _close_belt();
    void _clear_belt_text();
    void _refresh_belt();
    void _update_belt_input();
    void _drink_potion(int index, int target);
    void _enter_choice();
    void _update_choice_input();
    void _resolve_choice();
    void _refresh_choice_banner();
    [[nodiscard]] std::span<const uint8_t> _choice_span() const;
    void _move_to_position(int position);
    void _build_choice_views();
    void _move_in_pile(int step);
    [[nodiscard]] int _choice_index(int view_index) const;
    [[nodiscard]] bool _is_picked(int view_index) const;
    void _start_card(int target);
    void _update_card_flight();
    void _start_enemy_turn();
    void _update_enemy_turn();
    bn::optional<SceneId> _update_end_banner();

    void _snapshot();
    void _resolve_diffs(bool attack_card);
    void _set_banner(const bn::string_view& text, TextColor color);
    [[nodiscard]] int _enemy_top(const EnemyView& view) const;
    [[nodiscard]] int _intent_x(const EnemyView& view) const;
    [[nodiscard]] int _body_y(const EnemyView& view) const;
    [[nodiscard]] bool _any_playable() const;
    [[nodiscard]] int _next_alive(int from, int step) const;
};

CombatScene::CombatScene(GameContext& context) :
    _ctx(context),
    _combat(context.run.combat),
    _bg(hero_battle_bg().create_bg(BG_ORIGIN_X, BG_ORIGIN_Y)),
    _player(hero_battler().create_sprite(PLAYER_X, PLAYER_Y)),
    _player_bar(PLAYER_X, BAR_Y),
    _energy_orb(hero_energy_orb().create_sprite(-100, -58)),
    _target_cursor(create_icon(Icon::TARGET, 0, -70)),
    _dim_palette([this] {
        bn::span<const bn::color> source = hero_hand_cards().palette_item().colors_ref();

        for(int index = 0; index < 16; ++index)
        {
            bn::color color = index < source.size() ? source[index] : bn::color();
            _dim_colors[index] = bn::color(color.red() / 3 + 2, color.green() / 3 + 2, color.blue() / 3 + 3);
        }

        return bn::sprite_palette_ptr::create(
                bn::sprite_palette_item(bn::span<const bn::color>(_dim_colors), bn::bpp_mode::BPP_4));
    }())
{
    _player.set_z_order(Z_ACTOR);
    _energy_orb.set_z_order(Z_HUD);
    _target_cursor.set_visible(false);

    engine::RoomType room = engine::RoomType::MONSTER;

    if(_ctx.run.current_node_index >= 0)
    {
        room = _ctx.run.map.nodes[_ctx.run.current_node_index].type;
    }

    play_music(room == engine::RoomType::BOSS ? Music::BOSS
               : room == engine::RoomType::ELITE ? Music::ELITE : Music::COMBAT);
    _apply_player_tint();

    _player_bar.set(_combat.player.hp, _combat.player.max_hp);
    _player_bar.snap();

    _build_enemies();
    engine::discover_enemies(engine::SaveManager::instance().get_active_profile(), _combat);

    if(_ctx.combat_resume)
    {
        _ctx.combat_resume = false;
        _phase = Phase::PLAYER;
        _build_hand(false);

        // "Finir le tour" picked in the pause menu.
        if(_ctx.pause_end_turn)
        {
            _ctx.pause_end_turn = false;
            play_sfx(Sfx::DISCARD);
            _start_enemy_turn();
        }
    }
    else
    {
        _phase = Phase::INTRO;
        _player.set_x(PLAYER_X - 120);
        _player_bar.set_visible(false);

        for(EnemyView& view : _enemies)
        {
            view.bar.set_visible(false);
            view.intent_icon.set_visible(false);
        }

        bool boss = ! _enemies.empty() && _enemies[0].boss;
        TextLine title;
        bn::ostringstream stream(title);

        if(boss)
        {
            stream << "BOSS : " << bn::string_view(_combat.enemies[0].name.data(), _combat.enemies[0].name.size());
        }
        else if(_combat.enemies.size() > 1)
        {
            stream << "Des ennemis surgissent !";
        }
        else
        {
            stream << bn::string_view(_combat.enemies[0].name.data(), _combat.enemies[0].name.size())
                   << " surgit !";
        }

        _set_banner(title, boss ? TextColor::RED : TextColor::GOLD);
        _banner_timer = INTRO_FRAMES + 20;
    }

    _refresh_hud();
}

CombatScene::~CombatScene()
{
    bn::blending::set_transparency_alpha(1);
    bn::sprites_mosaic::set_stretch(0);
}

void CombatScene::_apply_player_tint()
{
    bn::sprite_palette_ptr palette = _player.palette();
    bn::fixed intensity = palette_variant_intensity(_ctx.palette);

    if(_player_hurt > 0)
    {
        palette.set_fade(bn::color(31, 4, 4), bn::fixed(_player_hurt) / 20);
    }
    else if(intensity > 0)
    {
        palette.set_fade(palette_variant_color(_ctx.palette), intensity);
    }
    else
    {
        palette.set_fade(bn::color(), 0);
    }
}

void CombatScene::_build_enemies()
{
    int count = int(_combat.enemies.size());

    for(int index = 0; index < count; ++index)
    {
        const engine::Enemy& enemy = _combat.enemies[index];
        EnemyArt art = enemy_art(enemy.type);
        Point slot = enemy_slot(index, count, art.boss);
        int bar_x = enemy_bar_x(index, count, art.boss);
        EnemyView& view = _enemies.emplace_back(slot.x, slot.y, bar_x, art.boss);
        _create_enemy_parts(view, art);

        if(count >= 3)
        {
            view.bar.set_horizontal_scale(enemy_bar_scale(count));
        }

        view.bar.set_compact(enemy_hp_compact(count, enemy.max_hp));
        view.bar.set(enemy.hp, enemy.max_hp);
        view.bar.snap();

        if(! enemy.is_alive())
        {
            view.gone = true;

            for(bn::sprite_ptr& part : view.parts)
            {
                part.set_visible(false);
            }

            view.bar.set_visible(false);
            view.intent_icon.set_visible(false);
        }

        _snap_hp[index] = enemy.hp;
        _snap_block[index] = enemy.block;
        view.intent_label.move_to(_intent_x(view) + 10, _enemy_top(view));
        view.intent_icon.set_position(_intent_x(view), _enemy_top(view));
        _refresh_enemy_hud(index);
    }

    _target = _combat.get_first_alive_enemy_index();
}

void CombatScene::_create_enemy_parts(EnemyView& view, const EnemyArt& art)
{
    view.parts.clear();
    view.boss = art.boss;

    // A summon can bring a sixth or seventh different monster into a fight: when no OBJ palette is left for its
    // colours, it borrows the hero's (wrong colours) instead of stopping the game.
    bn::optional<bn::sprite_palette_ptr> palette = art.item->palette_item().create_palette_optional();

    if(! palette)
    {
        palette = _player.palette();
    }

    int parts = art.boss ? 4 : 1;

    for(int part = 0; part < parts; ++part)
    {
        int x = art.boss ? view.x + boss_parts[part].x : view.x;
        int y = art.boss ? view.y + boss_parts[part].y : view.y;
        bn::sprite_ptr sprite = bn::sprite_ptr::create(x, y, art.item->shape_size(),
                                                       art.item->tiles_item().create_tiles(part), *palette);
        sprite.set_z_order(Z_ACTOR);
        view.parts.push_back(bn::move(sprite));
    }
}

void CombatScene::_summon_enemies(int summoner, unsigned summoned)
{
    int count = int(_combat.enemies.size());

    for(int index = 0; index < count; ++index)
    {
        if(! (summoned & (1u << index)))
        {
            continue;
        }

        const engine::Enemy& enemy = _combat.enemies[index];
        EnemyArt art = enemy_art(enemy.type);

        // A new place at the end of the row, or the place of a dead enemy taken again.
        while(_enemies.size() <= index)
        {
            Point slot = enemy_slot(_enemies.size(), count, art.boss);
            EnemyView& added = _enemies.emplace_back(slot.x, slot.y, enemy_bar_x(_enemies.size(), count, art.boss),
                                                     art.boss);
            added.gone = true;
            added.bar.set_visible(false);
            added.intent_icon.set_visible(false);
        }

        EnemyView& view = _enemies[index];
        _create_enemy_parts(view, art);
        view.gone = false;
        view.death = 0;
        view.hurt = 0;
        view.lunge = 0;
        view.arrive = ARRIVE_FRAMES;
        view.shown_icon = -1;
        // Its move shows with the others' once the enemy turn is over.
        view.intent_visible = false;
        view.intent_icon.set_visible(false);
        view.bar.set(enemy.hp, enemy.max_hp);
        view.bar.snap();
        view.bar.set_visible(true);
        _snap_hp[index] = enemy.hp;
        _snap_block[index] = enemy.block;
    }

    _layout_enemies();
    engine::discover_enemies(engine::SaveManager::instance().get_active_profile(), _combat);

    for(int index = 0; index < _enemies.size(); ++index)
    {
        if(summoned & (1u << index))
        {
            _fx.spawn(bn::sprite_items::fx_burst, _enemies[index].x, _body_y(_enemies[index]), FX_BUFF, 4, 4);
        }

        _refresh_enemy_hud(index);
    }

    if(summoner >= 0 && summoner < _enemies.size())
    {
        EnemyView& caller = _enemies[summoner];
        _popups.spawn(caller.x, _body_y(caller) - 6, "Invocation !", TextColor::GOLD);
    }
}

void CombatScene::_layout_enemies()
{
    // The row spreads out again when it grows: every enemy moves to its place for the new count.
    int count = _enemies.size();

    for(int index = 0; index < count; ++index)
    {
        EnemyView& view = _enemies[index];
        Point slot = enemy_slot(index, count, view.boss);
        view.x = slot.x;
        view.y = slot.y;
        view.bar_x = enemy_bar_x(index, count, view.boss);
        view.intent_x = enemy_intent_x(slot.x, view.boss);
        view.bar.set_position(view.bar_x, BAR_Y);
        view.bar.set_horizontal_scale(enemy_bar_scale(count));

        if(index < int(_combat.enemies.size()))
        {
            view.bar.set_compact(enemy_hp_compact(count, _combat.enemies[index].max_hp));
        }

        // The labels under it are written again at the new place.
        view.status_text = "-";
        view.shown_block = -1;
        view.block_icon.reset();
    }
}

void CombatScene::_build_hand(bool deal)
{
    _hand.clear();
    const bn::sprite_item& item = hero_hand_cards();

    for(int index = 0; index < int(_combat.hand.size()); ++index)
    {
        const engine::DeckCard& card = _combat.hand[index];
        bn::sprite_ptr sprite = item.create_sprite(deal ? -150 : 0, HAND_Y, hand_card_frame(card));
        sprite.set_z_order(Z_CARD);
        HandCardView view{ card, bn::move(sprite), deal ? -150 : 0, HAND_Y, deal ? index * 3 : 0, false };
        _hand.push_back(bn::move(view));
    }

    _selected = bn::clamp(_selected, 0, bn::max(0, _hand.size() - 1));

    if(! deal)
    {
        _layout_hand();

        for(HandCardView& view : _hand)
        {
            view.sprite.set_position(view.x, view.y);
        }
    }

    _selected_shown = -1;
}

void CombatScene::_layout_hand()
{
    int count = _hand.size();

    if(count == 0)
    {
        return;
    }

    bool choosing = _phase == Phase::CHOICE;
    // While Vakuu plays, no card is under the player's cursor.
    bool interactive = (_phase == Phase::PLAYER && ! _combat.vakuu_turn_active) || _phase == Phase::TARGET || choosing;
    int spacing = count > 1 ? bn::min(27, 196 / (count - 1)) : 0;
    int start = -((count - 1) * spacing) / 2;

    for(int index = 0; index < count; ++index)
    {
        HandCardView& view = _hand[index];
        bool focused = interactive && index == _selected;
        int target_x = start + index * spacing;
        int target_y = focused ? HAND_FOCUS_Y : HAND_Y;

        if(choosing && _is_picked(index))
        {
            target_y = HAND_PICKED_Y;
        }

        if(view.delay > 0)
        {
            --view.delay;
            continue;
        }

        view.x = approach(view.x, target_x, bn::fixed(0.3));
        view.y = approach(view.y, target_y, bn::fixed(0.35));
        view.sprite.set_position(view.x, view.y);
        view.sprite.set_z_order(focused ? Z_CARD_FOCUS : Z_CARD);

        bool dim = interactive && (choosing ? ! _combat.choice_allows(_choice_index(index)) : ! _combat.can_play_card(index));

        if(dim != view.dim)
        {
            view.dim = dim;

            if(dim)
            {
                view.sprite.set_palette(_dim_palette);
            }
            else
            {
                view.sprite.set_palette(hero_hand_cards().palette_item());
            }
        }
    }
}

void CombatScene::_refresh_card_text()
{
    if(_phase == Phase::TARGET)
    {
        const engine::Enemy& enemy = _combat.enemies[_target];
        TextLine text;
        bn::ostringstream stream(text);
        stream << "Cible : " << bn::string_view(enemy.name.data(), enemy.name.size());
        _line1.set(-112, LINE1_Y, text, Align::LEFT, TextColor::GOLD);
        _line1_right.set(112, LINE1_Y, "◄ ► changer", Align::RIGHT, TextColor::GRAY);
        // An enemy that holds a power: its description takes turns with the key reminder.
        bn::string_view held;

        for(const engine::ActivePower& power : _combat.powers)
        {
            if(power.holder == _target && ! enemy_power_description(power.id).empty())
            {
                held = enemy_power_description(power.id);
                break;
            }
        }

        if(! held.empty() && (_ctx.frame / 150) % 2 == 0)
        {
            _line2.set(-112, LINE2_Y, held, Align::LEFT, TextColor::PURPLE);
        }
        else
        {
            _line2.set(-112, LINE2_Y, _aimed_potion >= 0 ? "A : lancer   B : retour" : "A : attaquer   B : annuler",
                       Align::LEFT, TextColor::WHITE);
        }

        _selected_shown = -1;
        return;
    }

    if(_phase == Phase::POTIONS)
    {
        _refresh_belt();
        return;
    }

    if(_phase == Phase::CARD_FLIGHT)
    {
        if(_selected_shown != -2)
        {
            _selected_shown = -2;
            TextLine text;
            card_name(_flying, text);
            _line1.set(-112, LINE1_Y, text, Align::LEFT,
                       is_enchanted(_flying) ? TextColor::PURPLE : _flying.upgraded ? TextColor::GREEN : TextColor::GOLD);
            _line1_right.clear();
            card_description(_flying, text);
            _line2.set(-112, LINE2_Y, text, Align::LEFT, TextColor::WHITE);
        }

        return;
    }

    if(_phase == Phase::ENEMY_TURN)
    {
        _line1.set(-112, LINE1_Y, "Tour ennemi...", Align::LEFT, TextColor::RED);
        _line1_right.clear();
        _line2.set(-112, LINE2_Y, enemy_power_description(_landed_power), Align::LEFT, TextColor::PURPLE);
        _selected_shown = -1;
        return;
    }

    if(_phase == Phase::END_BANNER || _phase == Phase::INTRO)
    {
        _line1.clear();
        _line1_right.clear();
        _line2.clear();
        _selected_shown = -1;
        return;
    }

    if(_combat.vakuu_turn_active)
    {
        _line1.set(-112, LINE1_Y, "Vakuu joue votre tour...", Align::LEFT, TextColor::PURPLE);
        _line1_right.clear();
        _line2.clear();
        _selected_shown = -1;
        return;
    }

    if(_hand.empty())
    {
        _line1.set(-112, LINE1_Y, "Main vide", Align::LEFT, TextColor::GRAY);
        _line1_right.clear();
        engine::ControlPreset preset = engine::control_preset(engine::SaveManager::instance().settings());
        _line2.set(-112, LINE2_Y, engine::end_turn_reminder(preset), Align::LEFT, TextColor::WHITE);
        _selected_shown = -1;
        return;
    }

    _selected = bn::clamp(_selected, 0, _hand.size() - 1);

    if(_selected == _selected_shown && _hand.size() == _hand_size_shown)
    {
        return;
    }

    _selected_shown = _selected;
    _hand_size_shown = _hand.size();

    const engine::DeckCard& card = _hand[_selected].card;
    TextLine text;
    card_name(card, text);
    _line1.set(-112, LINE1_Y, text, Align::LEFT,
               is_enchanted(card) ? TextColor::PURPLE : card.upgraded ? TextColor::GREEN : TextColor::GOLD);

    if(_phase == Phase::CHOICE && _pile_view && _combat.choice_pending())
    {
        // Which pile is shown, and where the cursor is in it.
        TextLine place;
        bn::ostringstream place_stream(place);
        engine::Pile source = _combat.pending_choices[0].source;
        place_stream << (source == engine::Pile::DRAW ? "Pioche " : source == engine::Pile::OFFERS ? "Choix " : "Défausse ")
                     << (_pile_offset + _selected + 1) << "/" << _choice_order.size();
        _line1_right.set(112, LINE1_Y, place, Align::RIGHT, TextColor::GRAY);
    }
    else if(_phase == Phase::CHOICE)
    {
        bool optional = _combat.choice_pending() && _combat.pending_choices[0].optional;
        _line1_right.set(112, LINE1_Y, optional ? "START : valider" : "A : choisir", Align::RIGHT, TextColor::GRAY);
    }
    else
    {
        _line1_right.clear();
    }

    card_description(card, text);
    _line2.set(-112, LINE2_Y, text, Align::LEFT, TextColor::WHITE);
}

void CombatScene::_refresh_hud()
{
    const engine::Player& player = _combat.player;

    if(player.energy != _shown_energy)
    {
        _shown_energy = player.energy;
        bn::string<8> text;
        bn::ostringstream stream(text);
        stream << int(player.energy) << '/' << int(player.max_energy);
        _energy_label.set(-100, -58, text, Align::CENTER, player.energy > 0 ? TextColor::WHITE : TextColor::GRAY);

        bn::sprite_palette_ptr palette = _energy_orb.palette();
        palette.set_fade(bn::color(6, 6, 8), player.energy > 0 ? bn::fixed(0) : bn::fixed(0.6));
    }

    int draw = int(_combat.draw_pile.size());
    int discard = int(_combat.discard_pile.size());

    if(draw != _shown_draw || discard != _shown_discard)
    {
        _shown_draw = draw;
        _shown_discard = discard;
        TextLine text;
        bn::ostringstream stream(text);
        stream << "Pioche " << draw;
        _pile_label.set(-80, -66, text, Align::LEFT, TextColor::GRAY);
        text.clear();
        stream << "Défausse " << discard;
        _discard_label.set(-80, -56, text, Align::LEFT, TextColor::GRAY);
    }

    // The exhaust pile only shows once a card left the combat.
    int exhausted = int(_combat.exhaust_pile.size());

    if(exhausted != _shown_exhaust)
    {
        _shown_exhaust = exhausted;

        if(exhausted > 0)
        {
            TextLine text;
            bn::ostringstream stream(text);
            stream << "Épuisées " << exhausted;
            _exhaust_label.set(-80, -46, text, Align::LEFT, TextColor::GRAY);
        }
        else
        {
            _exhaust_label.clear();
        }
    }

    // Hero-specific resource under the energy orb.
    int resource = -1;
    TextLine resource_text;
    bn::ostringstream resource_stream(resource_text);

    switch(current_hero())
    {
    case engine::CharacterId::DEFECT:
        // Both numbers are part of what is shown: the label is redrawn when either changes.
        resource = int(player.orbs.size()) + 16 * player.orb_slots;
        resource_stream << "Orbes " << int(player.orbs.size()) << "/" << int(player.orb_slots);
        break;
    case engine::CharacterId::REGENT:
        resource = player.sovereign_stars;
        resource_stream << "★ " << resource;
        break;
    case engine::CharacterId::NECROBINDER:
        // Both numbers are part of what is shown, as for the orbs.
        resource = player.minion_hp * 1024 + player.minion_max_hp;
        resource_stream << "Titos " << player.minion_hp << "/" << player.minion_max_hp;
        break;
    default:
        break;
    }

    if(resource != _shown_resource)
    {
        _shown_resource = resource;

        if(resource >= 0)
        {
            _resource_label.set(-116, -38, resource_text, Align::LEFT, TextColor::GOLD);
        }
        else
        {
            _resource_label.clear();
        }
    }

    // Player block badge and statuses.
    int block = _phase == Phase::ENEMY_TURN ? 0 : player.block;

    if(block != _shown_player_block)
    {
        _shown_player_block = block;

        if(block > 0)
        {
            if(! _player_block_icon)
            {
                _player_block_icon = create_icon(Icon::DEFEND, PLAYER_X - 30, BAR_Y);
                _player_block_icon->set_z_order(Z_FX);
            }

            bn::string<8> text;
            bn::ostringstream stream(text);
            stream << block;
            // Left of the widest HP label, so that a two or three digit block never covers it.
            int block_x = PLAYER_X - (hp_label_width(player.max_hp, false) + 1) / 2 - block_badge_half(text) - 2;
            _player_block_icon->set_position(block_x, BAR_Y);
            _player_block_label.set(block_x, BAR_Y, text, Align::CENTER, TextColor::WHITE);
        }
        else
        {
            _player_block_icon.reset();
            _player_block_label.clear();
        }
    }

    // The hero's statuses stop before the room of the first enemy's, on the same line.
    Span player_span = { -SCREEN_EDGE, STATUS_WIDTH - SCREEN_EDGE };

    if(! _enemies.empty())
    {
        player_span.right = bn::min(player_span.right,
                                    enemy_status_span(0, _enemies.size(), _enemies[0].boss).left - 4);
    }

    int player_width = player_span.right - player_span.left;
    TextLine status;
    status_line(player, status, player_width);
    append_enemy_powers(_combat, -1, status, player_width);

    if(status != _player_status_text)
    {
        _player_status_text = status;
        _player_status.set(place_in_span(PLAYER_X, text_width(status), player_span), STATUS_Y, status, Align::CENTER,
                           TextColor::PURPLE);
    }

    bool interactive = _phase == Phase::PLAYER && ! _combat.vakuu_turn_active;
    // Key reminders on the right of line 1. With a potion in the belt, its key joins the START one when the card
    // name leaves room for both.
    engine::ControlPreset preset = engine::control_preset(engine::SaveManager::instance().settings());
    const bn::string_view REMINDER = engine::end_turn_reminder(preset);
    const bn::string_view BELT_REMINDER = engine::belt_reminder(preset);
    const bn::string_view BELT_ONLY = engine::potions_reminder(preset);
    bool belt = ! _ctx.run.potions.empty();

    if(interactive && ! _any_playable())
    {
        // Line 1 holds the name of the selected card, or "Main vide".
        int name_width = text_width("Main vide");

        if(_selected >= 0 && _selected < int(_combat.hand.size()))
        {
            TextLine name;
            card_name(_combat.hand[_selected], name);
            name_width = text_width(name);
        }

        bool both = belt && name_width + text_width(BELT_REMINDER) + 4 <= TEXT_WIDTH;
        _hint_label.set(112, LINE1_Y, both ? BELT_REMINDER : REMINDER, Align::RIGHT, TextColor::GOLD);
        _hint_label.set_visible((_ctx.frame / 16) % 2 == 0);
    }
    else if(interactive && ! _hand.empty())
    {
        // The mini card shows the printed cost: a cost changed by the combat is written here instead.
        TextLine cost;
        bn::string_view reminder = REMINDER;
        bool room = true;

        if(_selected >= 0 && _selected < int(_combat.hand.size()))
        {
            const engine::DeckCard& card = _combat.hand[_selected];
            int actual = _combat.effective_cost(card);

            if(actual != engine::CardDef::get_cost(card.id, card.upgraded))
            {
                bn::ostringstream stream(cost);
                stream << "Coût " << cost_label(actual) << " ●";
            }
            else
            {
                // The reminder gives way to a card name that would run into it: the shorter ones are tried.
                TextLine name;
                card_name(card, name);
                int name_width = text_width(name) + 4;

                if(belt && name_width + text_width(BELT_REMINDER) <= TEXT_WIDTH)
                {
                    reminder = BELT_REMINDER;
                }
                else if(name_width + text_width(REMINDER) <= TEXT_WIDTH)
                {
                    reminder = REMINDER;
                }
                else if(belt && name_width + text_width(BELT_ONLY) <= TEXT_WIDTH)
                {
                    reminder = BELT_ONLY;
                }
                else
                {
                    room = false;
                }
            }
        }

        if(cost.empty())
        {
            _hint_label.set(112, LINE1_Y, reminder, Align::RIGHT, TextColor::GRAY);
        }
        else
        {
            _hint_label.set(112, LINE1_Y, cost, Align::RIGHT, TextColor::GREEN);
        }

        _hint_label.set_visible(room);
    }
    else
    {
        _hint_label.set_visible(false);
    }
}

void CombatScene::_refresh_enemy_hud(int index)
{
    EnemyView& view = _enemies[index];
    const engine::Enemy& enemy = _combat.enemies[index];

    if(view.gone || view.death > 0)
    {
        view.intent_icon.set_visible(false);
        view.intent_label.clear();
        view.status_label.clear();
        view.block_label.clear();
        view.block_icon.reset();
        return;
    }

    // Intent icon + value above the monster.
    const engine::EnemyIntent& intent = enemy.current_intent;
    Icon icon = Icon::UNKNOWN;
    TextLine value;
    bn::ostringstream stream(value);
    int damage = enemy.intent_damage(_combat.player);

    switch(intent.type)
    {
    case engine::EnemyIntentType::ATTACK:
        icon = Icon::ATTACK;
        stream << damage;
        break;
    case engine::EnemyIntentType::ATTACK_DEFEND:
        icon = Icon::ATTACK;
        stream << damage << "+";
        break;
    case engine::EnemyIntentType::ATTACK_BUFF:
        icon = Icon::ATTACK;
        stream << damage << "▲";
        break;
    case engine::EnemyIntentType::ATTACK_DEBUFF:
        icon = Icon::ATTACK;
        stream << damage << "▼";
        break;
    case engine::EnemyIntentType::DEBUFF:
        icon = Icon::DEBUFF;
        break;
    case engine::EnemyIntentType::STUN:
        icon = Icon::STUN;
        break;
    case engine::EnemyIntentType::HEAL:
        icon = Icon::HEAL;
        break;
    case engine::EnemyIntentType::ESCAPE:
        icon = Icon::ESCAPE;
        break;
    case engine::EnemyIntentType::STATUS:
        icon = Icon::STATUS;
        stream << int(intent.status_cards);
        break;
    case engine::EnemyIntentType::DEFEND:
        icon = Icon::DEFEND;
        break;
    case engine::EnemyIntentType::SUMMON:
        icon = Icon::UNKNOWN;

        if(intent.summons > 0)
        {
            stream << "+" << int(intent.summons);
        }
        break;
    default:
        icon = Icon::BUFF;
        break;
    }

    if(static_cast<int>(icon) != view.shown_icon)
    {
        view.shown_icon = static_cast<int>(icon);
        set_icon(view.intent_icon, icon);
    }

    if(damage > 0 && intent.hits > 1)
    {
        stream << "x" << int(intent.hits);
    }

    view.intent_icon.set_visible(view.intent_visible);
    view.intent_label.set(view.intent_label.x(), view.intent_label.y(), value, Align::LEFT,
                          icon == Icon::ATTACK ? TextColor::RED : TextColor::WHITE);
    view.intent_label.set_visible(view.intent_visible);

    TextLine status;
    int count = _enemies.size();
    Span status_span = enemy_status_span(index, count, view.boss);
    int status_width = status_span.right - status_span.left;
    status_line(enemy, status, status_width);
    append_enemy_powers(_combat, index, status, status_width);

    if(status != view.status_text)
    {
        view.status_text = status;
        view.status_label.set(place_in_span(view.bar_x, text_width(status), status_span), enemy_status_y(index, count),
                              status, Align::CENTER, TextColor::PURPLE);
    }

    int block = enemy.block;

    if(block != view.shown_block)
    {
        view.shown_block = block;

        if(block > 0)
        {
            bn::string<8> text;
            bn::ostringstream block_stream(text);
            block_stream << block;
            int half = block_badge_half(text);
            int block_x = 0;
            int block_y = BAR_Y;

            if(count > 3)
            {
                // In a row of four or more there is no room left of the HP bar, so left of the intent, which steps
                // right when the badge would run into the pile counters.
                int intent_x = bn::max(enemy_intent_x(view.x, view.boss), PILE_RIGHT + 2 * half + 9);
                block_x = intent_x - 9 - half;
                block_y = _enemy_top(view);

                if(intent_x != view.intent_x)
                {
                    view.intent_x = intent_x;
                    view.intent_icon.set_position(intent_x, view.intent_icon.y());
                    view.intent_label.move_to(intent_x + 10, view.intent_label.y());
                }
            }
            else
            {
                // Left of the widest HP label, so that a two or three digit block never covers it.
                block_x = view.bar_x - (hp_label_width(enemy.max_hp, enemy_hp_compact(count, enemy.max_hp)) + 1) / 2 -
                          half - 2;
            }

            if(! view.block_icon)
            {
                view.block_icon = create_icon(Icon::DEFEND, block_x, block_y);
                view.block_icon->set_z_order(Z_FX);
            }

            view.block_icon->set_position(block_x, block_y);
            view.block_label.set(block_x, block_y, text);
        }
        else
        {
            view.block_icon.reset();
            view.block_label.clear();

            if(count > 3 && view.intent_x != enemy_intent_x(view.x, view.boss))
            {
                view.intent_x = enemy_intent_x(view.x, view.boss);
                view.intent_icon.set_position(view.intent_x, view.intent_icon.y());
                view.intent_label.move_to(view.intent_x + 10, view.intent_label.y());
            }
        }
    }
}

int CombatScene::_enemy_top(const EnemyView& view) const
{
    // Bosses fill the arena's height, so their intent floats beside them instead of above.
    int top = view.boss ? -44 : view.y - 38;
    return bn::max(top, -72);
}

int CombatScene::_body_y(const EnemyView& view) const
{
    // Boss art sits in the lower part of its 128x128 sprite.
    return view.boss ? view.y + 18 : view.y;
}

int CombatScene::_intent_x(const EnemyView& view) const
{
    return view.intent_x;
}

bool CombatScene::_any_playable() const
{
    for(int index = 0; index < int(_combat.hand.size()); ++index)
    {
        if(_combat.can_play_card(index))
        {
            return true;
        }
    }

    return false;
}

int CombatScene::_next_alive(int from, int step) const
{
    int count = int(_combat.enemies.size());

    for(int offset = 1; offset <= count; ++offset)
    {
        int index = (from + step * offset + count * 4) % count;

        if(_combat.enemies[index].is_alive())
        {
            return index;
        }
    }

    return from;
}

void CombatScene::_set_banner(const bn::string_view& text, TextColor color)
{
    _banner_y = BANNER_Y;
    _banner.set(0, _banner_y, text, Align::CENTER, color);
    _banner_timer = 60;
}

void CombatScene::_snapshot()
{
    _snap_player_hp = _combat.player.hp;
    _snap_player_block = _combat.player.block;

    for(int index = 0; index < int(_combat.enemies.size()); ++index)
    {
        _snap_hp[index] = _combat.enemies[index].hp;
        _snap_block[index] = _combat.enemies[index].block;
    }
}

void CombatScene::_resolve_diffs(bool attack_card)
{
    bn::string<16> text;
    int max_lost_hp = -1;   // -1: nobody hit, 0: only block was hit
    bool killed = false;

    for(int index = 0; index < int(_combat.enemies.size()); ++index)
    {
        const engine::Enemy& enemy = _combat.enemies[index];
        EnemyView& view = _enemies[index];
        int lost_hp = _snap_hp[index] - enemy.hp;
        int lost_block = _snap_block[index] - enemy.block;

        if(lost_hp > 0 || lost_block > 0)
        {
            view.hurt = 16;
            _fx.spawn(bn::sprite_items::fx_slash, view.x, _body_y(view), 0, 4, 2);
            _fx.spawn(bn::sprite_items::fx_burst, view.x + 6, _body_y(view) + 4, FX_HIT, 4, 3);
            text.clear();
            bn::ostringstream stream(text);

            if(lost_hp > 0)
            {
                stream << '-' << lost_hp;
                _popups.spawn(view.x + 12, _body_y(view) - 6, text, TextColor::RED);
            }
            else
            {
                _popups.spawn(view.x + 12, _body_y(view) - 6, "Bloqué", TextColor::BLUE);
            }

            _shake.start(8, lost_hp >= 10 ? 4 : 2);
            max_lost_hp = bn::max(max_lost_hp, lost_hp);
        }

        view.bar.set(enemy.hp, enemy.max_hp);

        if(! enemy.is_alive() && ! view.gone && view.death == 0)
        {
            view.death = DEATH_FRAMES;
            killed = true;

            if(enemy.escaped)
            {
                // A Minion whose leader just fell.
                _popups.spawn(view.x, _body_y(view) - 6, "Fuite !", TextColor::GRAY);
            }

            for(bn::sprite_ptr& part : view.parts)
            {
                part.set_blending_enabled(true);
                part.set_mosaic_enabled(true);
            }
        }
    }

    // One impact sound per card, the strongest one wins.
    if(killed)
    {
        play_sfx(Sfx::ENEMY_DEATH);
    }
    else if(max_lost_hp >= 10)
    {
        play_sfx(Sfx::HIT_HEAVY);
    }
    else if(max_lost_hp > 0)
    {
        play_sfx(Sfx::HIT_SLASH);
    }
    else if(max_lost_hp == 0)
    {
        play_sfx(Sfx::HIT_BLUNT);
    }

    int gained_block = _combat.player.block - _snap_player_block;

    if(gained_block > 0)
    {
        _player_guard = 14;

        if(max_lost_hp < 0)
        {
            play_sfx(Sfx::BLOCK);
        }

        _fx.spawn(bn::sprite_items::fx_burst, PLAYER_X, PLAYER_Y, FX_BLOCK, 4, 3);
        text.clear();
        bn::ostringstream stream(text);
        stream << '+' << gained_block << " Armure";
        _popups.spawn(PLAYER_X + 8, PLAYER_Y - 10, text, TextColor::BLUE, attack_card ? 6 : 0);
    }
    else if(! attack_card)
    {
        _fx.spawn(bn::sprite_items::fx_burst, PLAYER_X, PLAYER_Y - 6, FX_BUFF, 4, 4);
        play_sfx(Sfx::BUFF);
    }

    _player_bar.set(_combat.player.hp, _combat.player.max_hp);
}

void CombatScene::_update_intro()
{
    ++_timer;
    bn::fixed t = ease_out(bn::min(bn::fixed(_timer) / 30, bn::fixed(1)));
    _player.set_x(lerp(PLAYER_X - 120, PLAYER_X, t));

    for(int index = 0; index < _enemies.size(); ++index)
    {
        EnemyView& view = _enemies[index];
        bn::fixed offset = lerp(150, 0, t);

        for(int part = 0; part < view.parts.size(); ++part)
        {
            int px = view.boss ? boss_parts[part].x : 0;
            int py = view.boss ? boss_parts[part].y : 0;
            view.parts[part].set_position(view.x + px + offset, view.y + py);
        }
    }

    if(_timer == 30)
    {
        _player_bar.set_visible(true);

        for(int index = 0; index < _enemies.size(); ++index)
        {
            if(! _enemies[index].gone)
            {
                _enemies[index].bar.set_visible(true);
                _refresh_enemy_hud(index);
            }
        }

        _build_hand(true);
    }

    if(_timer >= INTRO_FRAMES)
    {
        _phase = Phase::PLAYER;
        _timer = 0;
    }
}

void CombatScene::_update_player_input()
{
    int count = _hand.size();

    if(count > 0)
    {
        if(bn::keypad::left_pressed())
        {
            _selected = (_selected + count - 1) % count;
            play_sfx(Sfx::UI_CURSOR);
        }
        else if(bn::keypad::right_pressed())
        {
            _selected = (_selected + 1) % count;
            play_sfx(Sfx::UI_CURSOR);
        }
    }

    engine::Shortcut shortcut = pressed_shortcut();

    if(shortcut == engine::Shortcut::DECK)
    {
        play_sfx(Sfx::UI_OPEN);
        _ctx.deck_return = SceneId::COMBAT;
        _ctx.combat_resume = true;
        _ctx.transition = Transition::QUICK;
        _overlay = SceneId::DECK;
        return;
    }

    if(shortcut == engine::Shortcut::PAUSE)
    {
        play_sfx(Sfx::UI_OPEN);
        _ctx.pause_return = SceneId::COMBAT;
        _ctx.pause_cursor = 0;
        _ctx.combat_resume = true;
        _ctx.transition = Transition::QUICK;
        _overlay = SceneId::PAUSE;
        return;
    }

    if(shortcut == engine::Shortcut::POTIONS)
    {
        if(_ctx.run.potions.empty())
        {
            play_sfx(Sfx::ERROR);
            _popups.spawn(0, HAND_FOCUS_Y - 24, "Aucune potion", TextColor::RED);
        }
        else
        {
            _open_belt();
        }

        return;
    }

    if(shortcut == engine::Shortcut::END_TURN)
    {
        play_sfx(Sfx::DISCARD);
        _start_enemy_turn();
        return;
    }

    if(bn::keypad::a_pressed() && count > 0)
    {
        if(! _combat.can_play_card(_selected))
        {
            bn::string_view reason = "Pas assez d'énergie";

            switch(_combat.play_blocker(_selected))
            {
            case engine::PlayBlocker::STARS:
                reason = "Pas assez d'Étoiles";
                break;
            case engine::PlayBlocker::CONDITION:
                reason = "Condition non remplie";
                break;
            case engine::PlayBlocker::PLAY_FIRST:
                reason = "Autre carte d'abord";
                break;
            case engine::PlayBlocker::PLAY_LIMIT:
                reason = "Limite atteinte";
                break;
            case engine::PlayBlocker::NOT_PLAYABLE:
                reason = "Injouable";
                break;
            case engine::PlayBlocker::CHOICE:
                reason = "Choix en attente";
                break;
            default:
                break;
            }

            play_sfx(Sfx::ERROR);
            _popups.spawn(_hand[_selected].x.round_integer(), HAND_FOCUS_Y - 24, reason, TextColor::RED);
            _hand[_selected].x += 6;
            return;
        }

        const engine::CardDef& def = engine::CardDef::get(_hand[_selected].card.id);

        if(def.target == engine::TargetType::SINGLE_ENEMY && _combat.count_alive_enemies() > 1)
        {
            play_sfx(Sfx::UI_CONFIRM);
            _phase = Phase::TARGET;

            if(_target < 0 || ! _combat.enemies[_target].is_alive())
            {
                _target = _combat.get_first_alive_enemy_index();
            }
        }
        else
        {
            _start_card(_combat.get_first_alive_enemy_index());
        }
    }
}

// Whispering Earring: Vakuu plays the first turn. The keys are not read; after a short wait the card the
// engine names flies like one the player picked, and when none is left the turn ends the usual way.
void CombatScene::_update_vakuu_turn()
{
    if(! _vakuu_announced)
    {
        _vakuu_announced = true;
        _set_banner("Vakuu joue...", TextColor::PURPLE);
    }

    ++_vakuu_timer;

    if(_vakuu_timer < VAKUU_DELAY)
    {
        return;
    }

    _vakuu_timer = 0;

    // A choice asked at the start of the turn is answered by the engine before it names a card.
    bool settled = _combat.choice_pending();
    _snapshot();
    int index = _combat.vakuu_next_card();

    if(settled)
    {
        _resolve_diffs(false);
    }

    if(settled || _hand.size() != int(_combat.hand.size()))
    {
        _build_hand(false);
        _selected_shown = -1;
    }

    if(index < 0 || index >= _hand.size())
    {
        play_sfx(Sfx::DISCARD);
        _start_enemy_turn();
        return;
    }

    _selected = index;
    _target = bn::max(0, _combat.get_first_alive_enemy_index());
    _start_card(_target);
}

void CombatScene::_update_target_input()
{
    if(bn::keypad::left_pressed() || bn::keypad::up_pressed())
    {
        _target = _next_alive(_target, -1);
        play_sfx(Sfx::UI_CURSOR);
    }
    else if(bn::keypad::right_pressed() || bn::keypad::down_pressed())
    {
        _target = _next_alive(_target, 1);
        play_sfx(Sfx::UI_CURSOR);
    }

    if(bn::keypad::b_pressed())
    {
        play_sfx(Sfx::UI_CANCEL);

        if(_aimed_potion >= 0)
        {
            // Back to the belt, on the potion that was being aimed.
            _potion = _aimed_potion;
            _aimed_potion = -1;
            _belt_shown = -1;
            _phase = Phase::POTIONS;
        }
        else
        {
            _phase = Phase::PLAYER;
        }
    }
    else if(bn::keypad::a_pressed())
    {
        if(_aimed_potion >= 0)
        {
            _drink_potion(_aimed_potion, _target);
        }
        else
        {
            _start_card(_target);
        }
    }
}

void CombatScene::_open_belt()
{
    _phase = Phase::POTIONS;
    _potion = bn::clamp(_potion, 0, int(_ctx.run.potions.size()) - 1);
    _aimed_potion = -1;
    _belt_shown = -1;

    // The hand leaves the screen while the belt is open: its sprites make room for the belt's text.
    _hand.clear();
    _hint_label.clear();
    play_sfx(Sfx::UI_OPEN);
}

void CombatScene::_close_belt()
{
    _phase = Phase::PLAYER;
    _aimed_potion = -1;
    _clear_belt_text();
    _build_hand(false);
    _selected_shown = -1;
}

void CombatScene::_clear_belt_text()
{
    _line3.clear();
    _line4.clear();
    _belt_hint.clear();
    _belt_shown = -1;
}

void CombatScene::_refresh_belt()
{
    const engine::RunManager& run = _ctx.run;
    int count = int(run.potions.size());

    if(_potion == _belt_shown || count == 0)
    {
        return;
    }

    _belt_shown = _potion;
    std::string_view id = run.potions[_potion];
    const engine::PotionData* potion = engine::find_potion(id);
    bn::string_view name(id.data(), int(id.size()));

    if(potion)
    {
        name = bn::string_view(potion->name.data(), int(potion->name.size()));
    }

    _line1.set(-112, LINE1_Y, name, Align::LEFT, TextColor::GOLD);

    if(count > 1)
    {
        TextLine place;
        bn::ostringstream stream(place);
        stream << "◄ " << (_potion + 1) << "/" << count << " ►";
        _line1_right.set(112, LINE1_Y, place, Align::RIGHT, TextColor::GRAY);
    }
    else
    {
        _line1_right.clear();
    }

    // The description of the potion, on up to three lines.
    bn::vector<TextLine, 3> lines;

    if(potion)
    {
        wrap_text(bn::string_view(potion->description.data(), int(potion->description.size())), TEXT_WIDTH, lines);
    }

    Label* rows[] = { &_line2, &_line3, &_line4 };

    for(int row = 0; row < 3; ++row)
    {
        if(row < lines.size())
        {
            rows[row]->set(-112, LINE2_Y + row * LINE_STEP, lines[row], Align::LEFT, TextColor::WHITE);
        }
        else
        {
            rows[row]->clear();
        }
    }

    _belt_hint.set(-112, BELT_HINT_Y, "A : boire   B : fermer", Align::LEFT, TextColor::GRAY);
}

void CombatScene::_update_belt_input()
{
    int count = int(_ctx.run.potions.size());

    if(count == 0)
    {
        _close_belt();
        return;
    }

    if(bn::keypad::left_pressed() || bn::keypad::up_pressed())
    {
        if(count > 1)
        {
            _potion = (_potion + count - 1) % count;
            play_sfx(Sfx::UI_CURSOR);
        }
    }
    else if(bn::keypad::right_pressed() || bn::keypad::down_pressed())
    {
        if(count > 1)
        {
            _potion = (_potion + 1) % count;
            play_sfx(Sfx::UI_CURSOR);
        }
    }

    if(bn::keypad::b_pressed() || bn::keypad::l_pressed())
    {
        play_sfx(Sfx::UI_CANCEL);
        _close_belt();
        return;
    }

    if(! bn::keypad::a_pressed())
    {
        return;
    }

    const engine::PotionData* potion = engine::find_potion(_ctx.run.potions[_potion]);

    if(potion && potion->target == "AnyEnemy" && _combat.count_alive_enemies() > 1)
    {
        // Thrown at one enemy: the target cursor of the cards picks it.
        play_sfx(Sfx::UI_CONFIRM);
        _aimed_potion = _potion;
        _clear_belt_text();
        _phase = Phase::TARGET;

        if(_target < 0 || ! _combat.enemies[_target].is_alive())
        {
            _target = _combat.get_first_alive_enemy_index();
        }

        return;
    }

    _drink_potion(_potion, _combat.get_first_alive_enemy_index());
}

void CombatScene::_drink_potion(int index, int target)
{
    engine::RunManager& run = _ctx.run;
    const engine::PotionData* potion = engine::find_potion(run.potions[index]);
    bool hostile = potion && (potion->target == "AnyEnemy" || potion->target == "AllEnemies");
    _aimed_potion = -1;
    _snapshot();

    if(! run.use_potion(index, target))
    {
        // The engine does not resolve this potion yet: it stays in the belt.
        play_sfx(Sfx::ERROR);
        _potion = index;
        _phase = Phase::POTIONS;
        _belt_shown = -1;
        _refresh_belt();
        _belt_hint.set(-112, BELT_HINT_Y, "Potion pas encore utilisable", Align::LEFT, TextColor::RED);
        return;
    }

    // A potion thrown without damage (Weak, Poison...) has no impact sound of its own.
    bool hit = false;

    for(int enemy = 0; enemy < int(_combat.enemies.size()); ++enemy)
    {
        hit = hit || _snap_hp[enemy] != _combat.enemies[enemy].hp || _snap_block[enemy] != _combat.enemies[enemy].block;
    }

    if(hostile && ! hit)
    {
        play_sfx(Sfx::DEBUFF);
    }

    _resolve_diffs(hostile);

    if(potion)
    {
        _set_banner(bn::string_view(potion->name.data(), int(potion->name.size())), TextColor::GOLD);
    }

    // Same refresh as after a card: the potion may have drawn, exhausted or upgraded cards.
    _clear_belt_text();
    _potion = bn::clamp(index, 0, bn::max(0, int(run.potions.size()) - 1));
    _phase = Phase::PLAYER;
    _build_hand(false);
    _selected_shown = -1;

    for(int enemy = 0; enemy < _enemies.size(); ++enemy)
    {
        _refresh_enemy_hud(enemy);
    }

    if(_combat.state == engine::CombatState::VICTORY)
    {
        _victory = true;
        _phase = Phase::END_BANNER;
        _timer = 0;
    }
    else if(_combat.state == engine::CombatState::DEFEAT)
    {
        // A card played by the potion cost the player's last HP.
        _player_death = DEATH_FRAMES;
        _victory = false;
        _phase = Phase::END_BANNER;
        _timer = 0;
    }
    else if(_combat.choice_pending())
    {
        _enter_choice();
    }
    else
    {
        if(_target < 0 || ! _combat.enemies[_target].is_alive())
        {
            _target = bn::max(0, _combat.get_first_alive_enemy_index());
        }

        if(_combat.end_turn_requested)
        {
            _start_enemy_turn();
        }
    }
}

void CombatScene::_enter_choice()
{
    _phase = Phase::CHOICE;
    _picked.clear();
    _choice_order.clear();
    _pile_offset = 0;
    const engine::CardChoice& choice = _combat.pending_choices[0];
    _pile_view = choice.source != engine::Pile::HAND;

    if(_pile_view)
    {
        // The cards of the pile take the place of the hand while the choice lasts.
        for(int index = 0; index < _combat.choice_pool_size(); ++index)
        {
            _choice_order.push_back(uint8_t(index));
        }

        if(choice.source == engine::Pile::DRAW)
        {
            // Sorted by card: the order of the draw pile stays hidden.
            std::sort(_choice_order.begin(), _choice_order.end(), [this](uint8_t a, uint8_t b) {
                int id_a = int(_combat.draw_pile[a].id);
                int id_b = int(_combat.draw_pile[b].id);
                return id_a != id_b ? id_a < id_b : a < b;
            });
        }

        _selected = 0;
        _build_choice_views();
    }

    // The cursor starts on a card that can be picked, so that A works at once (stays put when none can).
    int first = _combat.next_pickable_choice(_choice_span(), 0, 1, false);

    if(first >= 0)
    {
        _move_to_position(first);
    }

    _selected = bn::clamp(_selected, 0, bn::max(0, _hand.size() - 1));
    _selected_shown = -1;
    _refresh_choice_banner();
    play_sfx(Sfx::UI_OPEN);
}

void CombatScene::_build_choice_views()
{
    _hand.clear();
    const bn::sprite_item& item = hero_hand_cards();
    engine::Pile source = _combat.pending_choices[0].source;

    for(int position = _pile_offset; position < _choice_order.size() && _hand.size() < _hand.max_size(); ++position)
    {
        const engine::DeckCard& card = _combat.pile_card(source, _choice_order[position]);
        bn::sprite_ptr sprite = item.create_sprite(0, HAND_Y, hand_card_frame(card));
        sprite.set_z_order(Z_CARD);
        HandCardView view{ card, bn::move(sprite), 0, HAND_Y, 0, false };
        _hand.push_back(bn::move(view));
    }

    _layout_hand();

    for(HandCardView& view : _hand)
    {
        view.sprite.set_position(view.x, view.y);
    }

    _selected_shown = -1;
}

std::span<const uint8_t> CombatScene::_choice_span() const
{
    return std::span<const uint8_t>(_choice_order.data(), size_t(_choice_order.size()));
}

void CombatScene::_move_in_pile(int step)
{
    // Cards that cannot be picked are skipped; the cursor stays when no card lies that way.
    int position = _combat.next_pickable_choice(_choice_span(), _pile_offset + _selected + step, step, false);

    if(position < 0)
    {
        if(_combat.next_pickable_choice(_choice_span(), 0, 1, false) >= 0)
        {
            return;
        }

        // Nothing can be picked at all: every card can be looked at.
        position = bn::clamp(_pile_offset + _selected + step, 0, int(_choice_order.size()) - 1);
    }

    _move_to_position(position);
}

void CombatScene::_move_to_position(int position)
{
    if(! _pile_view)
    {
        _selected = bn::clamp(position, 0, bn::max(0, _hand.size() - 1));
        return;
    }

    int total = _choice_order.size();
    position = bn::clamp(position, 0, total - 1);

    // The cursor left the ten cards on screen: the window moves so that it sits in the middle.
    if(position < _pile_offset || position >= _pile_offset + _hand.max_size())
    {
        _pile_offset = bn::clamp(position - _hand.max_size() / 2, 0, bn::max(0, total - _hand.max_size()));
        _selected = position - _pile_offset;
        _build_choice_views();
    }

    _selected = position - _pile_offset;
}

int CombatScene::_choice_index(int view_index) const
{
    return _pile_view ? _choice_order[_pile_offset + view_index] : view_index;
}

bool CombatScene::_is_picked(int view_index) const
{
    int index = _choice_index(view_index);

    for(uint8_t picked : _picked)
    {
        if(picked == index)
        {
            return true;
        }
    }

    return false;
}

void CombatScene::_refresh_choice_banner()
{
    if(! _combat.choice_pending())
    {
        return;
    }

    const engine::CardChoice& choice = _combat.pending_choices[0];
    int wanted = _combat.choice_size();
    TextLine text;
    bn::ostringstream stream(text);

    switch(choice.kind)
    {
    case engine::ChoiceKind::DISCARD:
        stream << "Défausse";
        break;
    case engine::ChoiceKind::EXHAUST:
        stream << "Épuise";
        break;
    case engine::ChoiceKind::TOP_OF_DRAW:
        stream << "Sur la pioche";
        break;
    case engine::ChoiceKind::UPGRADE:
        stream << "Améliore";
        break;
    case engine::ChoiceKind::TO_HAND:
        stream << "Prends en main";
        break;
    case engine::ChoiceKind::MAKE_RETAIN:
        stream << "Retenue";
        break;
    case engine::ChoiceKind::MAKE_SLY:
        stream << "Ruse ce tour";
        break;
    case engine::ChoiceKind::MAKE_FREE:
        stream << "Coûte 0";
        break;
    case engine::ChoiceKind::COPY_ATTACK_OR_POWER:
    case engine::ChoiceKind::COPY_COLORLESS:
        stream << "Copie";
        break;
    case engine::ChoiceKind::COPY_NEXT_TURN:
        stream << "Copie au prochain tour";
        break;
    case engine::ChoiceKind::PLAY_REPEATED:
        stream << "Joue " << int(choice.amount) << " fois";
        break;
    default:
        stream << "Rends Éthérée";
        break;
    }

    if(choice.optional)
    {
        stream << " : " << _picked.size() << " carte(s)";
    }
    else if(wanted > 1)
    {
        stream << " : " << _picked.size() << "/" << wanted << " cartes";
    }
    else
    {
        stream << " : 1 carte";
    }

    // Under the pile counters and the intent icons, and it stays as long as the choice does.
    _banner_y = CHOICE_BANNER_Y;
    _banner.set(0, _banner_y, text, Align::CENTER, TextColor::GOLD);
    _banner_timer = bn::max(_banner_timer, 50);
}

void CombatScene::_update_choice_input()
{
    if(! _combat.choice_pending() || _hand.empty())
    {
        _phase = Phase::PLAYER;
        _banner_timer = 1;

        if(_pile_view)
        {
            _pile_view = false;
            _build_hand(false);
        }

        _selected_shown = -1;
        return;
    }

    // The prompt stays on screen for as long as the choice is asked.
    _banner_timer = bn::max(_banner_timer, 50);
    int count = _hand.size();

    if(bn::keypad::left_pressed() || bn::keypad::right_pressed())
    {
        int step = bn::keypad::left_pressed() ? -1 : 1;

        if(_pile_view)
        {
            _move_in_pile(step);
        }
        else
        {
            // Cards that cannot be picked are skipped, wrapping around the hand.
            int next = _combat.next_pickable_choice(_choice_span(), (_selected + count + step) % count, step, true);
            _selected = next >= 0 ? next : (_selected + count + step) % count;
        }

        play_sfx(Sfx::UI_CURSOR);
    }

    const bool optional = _combat.pending_choices[0].optional;
    const int wanted = _combat.choice_size();

    if(bn::keypad::a_pressed())
    {
        if(_is_picked(_selected))
        {
            for(int index = 0; index < _picked.size(); ++index)
            {
                if(_picked[index] == _choice_index(_selected))
                {
                    _picked.erase(_picked.begin() + index);
                    break;
                }
            }

            play_sfx(Sfx::UI_CANCEL);
        }
        else if(! _combat.choice_allows(_choice_index(_selected)) || _picked.size() >= wanted)
        {
            play_sfx(Sfx::ERROR);
            _popups.spawn(_hand[_selected].x.round_integer(), HAND_FOCUS_Y - 24, "Impossible", TextColor::RED);
            return;
        }
        else
        {
            _picked.push_back(uint8_t(_choice_index(_selected)));
            play_sfx(Sfx::UI_CONFIRM);
        }

        _refresh_choice_banner();

        // A choice that is not optional is over as soon as enough cards are marked.
        if(! optional && _picked.size() == wanted)
        {
            _resolve_choice();
        }
    }
    else if(bn::keypad::start_pressed())
    {
        if(optional)
        {
            _resolve_choice();
        }
        else
        {
            play_sfx(Sfx::ERROR);
            _popups.spawn(0, HAND_FOCUS_Y - 24, "Choisis d'abord", TextColor::RED);
        }
    }
}

void CombatScene::_resolve_choice()
{
    _snapshot();
    _combat.resolve_choice(std::span<const uint8_t>(_picked.data(), size_t(_picked.size())));
    _resolve_diffs(false);
    play_sfx(Sfx::DISCARD);

    _picked.clear();
    _pile_view = false;
    _build_hand(false);
    _selected_shown = -1;

    for(int index = 0; index < _enemies.size(); ++index)
    {
        _refresh_enemy_hud(index);
    }

    if(_combat.state == engine::CombatState::VICTORY)
    {
        _victory = true;
        _phase = Phase::END_BANNER;
        _timer = 0;
        _banner_timer = 1;
    }
    else if(_combat.state == engine::CombatState::DEFEAT)
    {
        _player_death = DEATH_FRAMES;
        _victory = false;
        _phase = Phase::END_BANNER;
        _timer = 0;
        _banner_timer = 1;
    }
    else if(_combat.choice_pending())
    {
        // Another choice follows (a card played twice asks twice), maybe about another pile.
        _enter_choice();
    }
    else
    {
        _phase = Phase::PLAYER;
        _banner_timer = 1;

        if(_target < 0 || ! _combat.enemies[_target].is_alive())
        {
            _target = bn::max(0, _combat.get_first_alive_enemy_index());
        }

        if(_combat.end_turn_requested)
        {
            _start_enemy_turn();
        }
    }
}

void CombatScene::_start_card(int target)
{
    HandCardView& view = _hand[_selected];
    _flying = view.card;
    _flying_hand_index = _selected;
    _flying_target = target;
    _flight_x0 = view.x;
    _flight_y0 = view.y;

    bn::sprite_ptr big = big_card_item(_flying.id).create_sprite(view.x, view.y);
    big.set_z_order(Z_OVERLAY);
    big.set_scale(bn::fixed(0.5));
    _flying_card = bn::move(big);

    _hand.erase(_hand.begin() + _selected);
    _selected = bn::clamp(_selected, 0, bn::max(0, _hand.size() - 1));
    _phase = Phase::CARD_FLIGHT;
    _timer = 0;
    _selected_shown = -1;
    play_sfx(Sfx::CARD_PLAY);
}

void CombatScene::_update_card_flight()
{
    ++_timer;
    const engine::CardDef& def = engine::CardDef::get(_flying.id);
    bool attack = def.type == engine::CardType::ATTACK;

    if(_flying_card)
    {
        bn::sprite_ptr& card = *_flying_card;

        if(_timer <= FLIGHT_GROW)
        {
            bn::fixed t = ease_out(bn::fixed(_timer) / FLIGHT_GROW);
            card.set_position(lerp(_flight_x0, STAGE_X, t), lerp(_flight_y0, STAGE_Y, t));
            card.set_scale(lerp(bn::fixed(0.5), bn::fixed(1.15), t));
        }
        else if(_timer <= FLIGHT_GROW + FLIGHT_HOLD)
        {
            card.set_scale(bn::fixed(1.15) - bn::fixed(_timer - FLIGHT_GROW) / 40);
        }
        else if(_timer <= FLIGHT_IMPACT)
        {
            bn::fixed t = bn::fixed(_timer - FLIGHT_GROW - FLIGHT_HOLD) / FLIGHT_TRAVEL;
            t = t * t;
            int tx = PLAYER_X;
            int ty = PLAYER_Y;

            if(attack && _flying_target >= 0 && _flying_target < _enemies.size())
            {
                tx = _enemies[_flying_target].x;
                ty = _body_y(_enemies[_flying_target]);
            }

            card.set_position(lerp(STAGE_X, tx, t), lerp(STAGE_Y, ty, t));
            card.set_scale(lerp(bn::fixed(1.0), bn::fixed(0.25), t));
        }
    }

    if(attack && _timer == FLIGHT_GROW + FLIGHT_HOLD)
    {
        _player_lunge = 16;
    }

    if(_timer == FLIGHT_IMPACT)
    {
        _flying_card.reset();
        _snapshot();

        // The view was removed from the hand; the engine plays the same index. On Vakuu's turn the engine
        // plays the card it named, the one that is flying, and counts it.
        if(_combat.vakuu_turn_active)
        {
            _combat.vakuu_step();
        }
        else
        {
            _combat.play_card(_flying_hand_index, _flying_target);
        }
        _resolve_diffs(attack);

        // Re-sync the hand views with the engine (some cards add, discard or change cards).
        bool stale = _hand.size() != int(_combat.hand.size());

        for(int index = 0; index < _hand.size() && ! stale; ++index)
        {
            stale = ! (_hand[index].card == _combat.hand[index]);
        }

        if(stale)
        {
            bn::fixed keep_x[10];

            for(int index = 0; index < _hand.size(); ++index)
            {
                keep_x[index] = _hand[index].x;
            }

            int old_size = _hand.size();
            _build_hand(false);

            for(int index = 0; index < _hand.size() && index < old_size; ++index)
            {
                _hand[index].x = keep_x[index];
                _hand[index].sprite.set_x(keep_x[index]);
            }
        }

        // The views keep a copy of each card: what a card gained without changing its look (damage of the
        // other Claws, a cost change) is taken from the engine, or the text shown would be the old one.
        for(int index = 0; index < _hand.size() && index < int(_combat.hand.size()); ++index)
        {
            _hand[index].card = _combat.hand[index];
        }

        _selected = bn::clamp(_flying_hand_index, 0, bn::max(0, _hand.size() - 1));
        _selected_shown = -1;

        for(int index = 0; index < _enemies.size(); ++index)
        {
            _refresh_enemy_hud(index);
        }
    }

    if(_timer >= FLIGHT_END)
    {
        _timer = 0;

        if(_combat.state == engine::CombatState::VICTORY)
        {
            _victory = true;
            _phase = Phase::END_BANNER;
        }
        else if(_combat.state == engine::CombatState::DEFEAT)
        {
            // A card that costs HP killed its player.
            _player_death = DEATH_FRAMES;
            _victory = false;
            _phase = Phase::END_BANNER;
        }
        else
        {
            _phase = Phase::PLAYER;

            if(_target < 0 || ! _combat.enemies[_target].is_alive())
            {
                _target = bn::max(0, _combat.get_first_alive_enemy_index());
            }

            // Void Form ends the turn it is played on.
            if(_combat.end_turn_requested)
            {
                _start_enemy_turn();
            }
        }
    }
}

void CombatScene::_start_enemy_turn()
{
    _phase = Phase::ENEMY_TURN;
    _landed_power = engine::PowerId::NONE;
    _timer = 0;

    for(EnemyView& view : _enemies)
    {
        view.intent_visible = true;
    }
}

void CombatScene::_update_enemy_turn()
{
    ++_timer;

    // Discard the hand: cards dive off the bottom of the screen.
    if(_timer < ENEMY_TURN_LEAD)
    {
        for(int index = 0; index < _hand.size(); ++index)
        {
            HandCardView& view = _hand[index];
            view.y += 2 + index / 2;
            view.x = approach(view.x, 100, bn::fixed(0.12));
            view.sprite.set_position(view.x, view.y);
        }

        return;
    }

    if(_timer == ENEMY_TURN_LEAD)
    {
        _hand.clear();
        _snapshot();
        _combat.end_player_turn();

        for(EnemyView& view : _enemies)
        {
            view.intent_visible = false;
            view.intent_icon.set_visible(false);
            view.intent_label.set_visible(false);
        }

        _set_banner("Tour ennemi", TextColor::RED);
        _banner_timer = 36;
    }

    int step_time = _timer - ENEMY_TURN_LEAD;
    int step = step_time / ENEMY_STEP;
    int local = step_time % ENEMY_STEP;
    int records = int(_combat.last_enemy_actions.size());

    if(step < records)
    {
        const engine::EnemyActionRecord& record = _combat.last_enemy_actions[step];

        // A record for an enemy the scene does not show has nothing to animate.
        if(record.enemy_index < 0 || record.enemy_index >= _enemies.size() ||
           record.enemy_index >= int(_combat.enemies.size()))
        {
            return;
        }

        EnemyView& view = _enemies[record.enemy_index];

        if(local == 0)
        {
            view.lunge = 18;
        }

        if(local == ENEMY_STEP_IMPACT)
        {
            bn::string<16> text;
            bn::ostringstream stream(text);
            bool attacked = record.intent_type == engine::EnemyIntentType::ATTACK ||
                            record.intent_type == engine::EnemyIntentType::ATTACK_DEFEND ||
                            record.intent_type == engine::EnemyIntentType::ATTACK_DEBUFF ||
                            record.intent_type == engine::EnemyIntentType::ATTACK_BUFF;

            if(record.damage_to_player > 0)
            {
                stream << '-' << record.damage_to_player;
                _popups.spawn(PLAYER_X + 8, PLAYER_Y - 10, text, TextColor::RED);
                _player_hurt = 20;
                play_sfx(record.damage_to_player >= 10 ? Sfx::HIT_HEAVY : Sfx::HIT_SLASH);
                _shake.start(12, record.damage_to_player >= 10 ? 5 : 3);
                _fx.spawn(bn::sprite_items::fx_slash, PLAYER_X, PLAYER_Y, 0, 4, 2, true);
                _fx.spawn(bn::sprite_items::fx_burst, PLAYER_X - 4, PLAYER_Y, FX_HIT, 4, 3);
            }
            else if(attacked)
            {
                _popups.spawn(PLAYER_X + 8, PLAYER_Y - 10, "Bloqué", TextColor::BLUE);
                _fx.spawn(bn::sprite_items::fx_burst, PLAYER_X, PLAYER_Y, FX_BLOCK, 4, 3);
                _shake.start(4, 1);
                play_sfx(Sfx::HIT_BLUNT);
            }

            if(record.block_gained > 0)
            {
                text.clear();
                stream << '+' << record.block_gained << " Armure";
                _popups.spawn(view.x, _body_y(view) - 6, text, TextColor::BLUE, attacked ? 8 : 0);
                _fx.spawn(bn::sprite_items::fx_burst, view.x, _body_y(view), FX_BLOCK, 4, 3);

                if(! attacked)
                {
                    play_sfx(Sfx::BLOCK);
                }
            }

            if(record.intent_type == engine::EnemyIntentType::BUFF ||
               record.intent_type == engine::EnemyIntentType::ATTACK_BUFF)
            {
                _popups.spawn(view.x, _body_y(view) - 6, "Force ▲", TextColor::GOLD, attacked ? 8 : 0);
                _fx.spawn(bn::sprite_items::fx_burst, view.x, _body_y(view), FX_BUFF, 4, 4);
                play_sfx(Sfx::BUFF);
            }

            if(record.cards_added_count > 0)
            {
                // "+3 Hébété": the cards the move slipped into the hero's piles.
                TextLine card;
                card_name(engine::DeckCard(record.cards_added), card);
                TextLine added;
                bn::ostringstream added_stream(added);
                added_stream << '+' << int(record.cards_added_count) << ' ' << card;
                _popups.spawn(PLAYER_X + 8, PLAYER_Y - 10, added, TextColor::PURPLE, attacked ? 12 : 0);
                play_sfx(Sfx::DEBUFF);
            }

            if(record.healed > 0)
            {
                text.clear();
                stream << '+' << record.healed << " PV";
                _popups.spawn(view.x, _body_y(view) - 6, text, TextColor::GREEN, attacked ? 8 : 0);
                play_sfx(Sfx::HEAL);
            }

            if(record.summoned != 0)
            {
                _summon_enemies(record.enemy_index, record.summoned);
                play_sfx(Sfx::BUFF);
            }
            else if(record.intent_type == engine::EnemyIntentType::SUMMON)
            {
                _fx.spawn(bn::sprite_items::fx_burst, view.x, _body_y(view), FX_BUFF, 4, 4);
                play_sfx(Sfx::BUFF);
            }

            if(record.intent_type == engine::EnemyIntentType::STUN)
            {
                _popups.spawn(view.x, _body_y(view) - 6, "Étourdi", TextColor::GRAY);
            }

            if(record.escaped)
            {
                _popups.spawn(view.x, _body_y(view) - 6, "Fuite !", TextColor::GRAY);
                play_sfx(Sfx::UI_CANCEL);
            }

            if(record.debuff_attempted)
            {
                // What the move did to the hero, one popup per status; nothing landed when Artifact took it all.
                int landed = 0;

                for(const engine::IntentDebuff& debuff : record.debuffs)
                {
                    if(debuff.type == engine::StatusType::NONE)
                    {
                        continue;
                    }

                    text.clear();

                    switch(debuff.type)
                    {
                    case engine::StatusType::WEAK:
                        stream << "Faible " << debuff.amount;
                        break;
                    case engine::StatusType::VULNERABLE:
                        stream << "Vuln " << debuff.amount;
                        break;
                    case engine::StatusType::FRAIL:
                        stream << "Fragile " << debuff.amount;
                        break;
                    case engine::StatusType::STRENGTH:
                        stream << "Force " << debuff.amount;
                        break;
                    case engine::StatusType::DEXTERITY:
                        stream << "Dex " << debuff.amount;
                        break;
                    default:
                        stream << "Affaibli";
                        break;
                    }

                    // One after the other at the same place: stacked, they would climb over the counters.
                    _popups.spawn(PLAYER_X + 8, PLAYER_Y - 10, text, TextColor::PURPLE,
                                  (attacked ? 12 : 0) + landed * 14);
                    ++landed;
                }

                if(record.power != engine::PowerId::NONE)
                {
                    // A power instead of a status: its short name, and its text in the box under the fight.
                    text.clear();
                    stream << enemy_power_label(record.power);
                    _popups.spawn(PLAYER_X + 8, PLAYER_Y - 10, text, TextColor::PURPLE,
                                  (attacked ? 12 : 0) + landed * 14);
                    _landed_power = record.power;
                    ++landed;
                }

                if(landed == 0)
                {
                    _popups.spawn(PLAYER_X + 8, PLAYER_Y - 10, "Annulé", TextColor::GRAY, attacked ? 12 : 0);
                }
                else
                {
                    _fx.spawn(bn::sprite_items::fx_burst, PLAYER_X, PLAYER_Y, FX_BUFF, 4, 4);
                    play_sfx(Sfx::DEBUFF);
                }
            }

            _player_bar.set(record.player_hp_after, _combat.player.max_hp);
            const engine::Enemy& enemy = _combat.enemies[record.enemy_index];
            view.bar.set(enemy.hp, enemy.max_hp);

            if(record.player_hp_after <= 0)
            {
                _player_death = DEATH_FRAMES;
                _phase = Phase::END_BANNER;
                _victory = false;
                _timer = 0;
                return;
            }
        }

        return;
    }

    int end_time = records * ENEMY_STEP;

    if(step_time == end_time)
    {
        // Poison may have killed enemies during their turn.
        for(int index = 0; index < _enemies.size(); ++index)
        {
            const engine::Enemy& enemy = _combat.enemies[index];
            _enemies[index].bar.set(enemy.hp, enemy.max_hp);

            if(! enemy.is_alive() && ! _enemies[index].gone && _enemies[index].death == 0)
            {
                _enemies[index].death = DEATH_FRAMES;

                if(! enemy.escaped)
                {
                    play_sfx(Sfx::ENEMY_DEATH);
                }

                for(bn::sprite_ptr& part : _enemies[index].parts)
                {
                    part.set_blending_enabled(true);
                    part.set_mosaic_enabled(true);
                }
            }
        }

        if(_combat.state == engine::CombatState::VICTORY)
        {
            _victory = true;
            _phase = Phase::END_BANNER;
            _timer = 0;
            return;
        }

        if(_combat.state == engine::CombatState::DEFEAT)
        {
            _player_death = DEATH_FRAMES;
            _victory = false;
            _phase = Phase::END_BANNER;
            _timer = 0;
            return;
        }

        _player_bar.set(_combat.player.hp, _combat.player.max_hp);
        TextLine text;
        bn::ostringstream stream(text);
        stream << "Tour " << _combat.turn_number;
        _set_banner(text, TextColor::GOLD);
        play_sfx(Sfx::TURN_START);
        _banner_timer = 40;

        for(int index = 0; index < _enemies.size(); ++index)
        {
            _enemies[index].intent_visible = true;
            _refresh_enemy_hud(index);
        }

        _selected = 0;
        _build_hand(true);
    }

    if(step_time >= end_time + 12)
    {
        _phase = Phase::PLAYER;
        _timer = 0;
    }
}

bn::optional<SceneId> CombatScene::_update_end_banner()
{
    ++_timer;

    if(_timer == 26)
    {
        if(_victory && _combat.every_enemy_escaped())
        {
            // Nobody was beaten: no reward follows.
            _set_banner("ILS ONT FUI...", TextColor::GRAY);
        }
        else
        {
            _set_banner(_victory ? "VICTOIRE !" : "DÉFAITE...", _victory ? TextColor::GOLD : TextColor::RED);
        }

        _banner_timer = 120;
        _hand.clear();
    }

    if(_timer < 90)
    {
        return bn::nullopt;
    }

    if(! _victory)
    {
        return SceneId::GAME_OVER;
    }

    engine::RunManager& run = _ctx.run;
    run.proceed_after_combat();

    if(run.phase == engine::RunPhase::VICTORY_ACT)
    {
        return SceneId::VICTORY;
    }

    if(run.phase == engine::RunPhase::MAP_NAVIGATION)
    {
        // The enemies ran away: straight back to the map.
        return SceneId::MAP;
    }

    if(run.phase == engine::RunPhase::ANCIENT_BLESSING)
    {
        return SceneId::ANCIENT;
    }

    return SceneId::REWARDS;
}

void CombatScene::_update_enemies()
{
    bn::fixed_point shake = _shake.offset();

    for(int index = 0; index < _enemies.size(); ++index)
    {
        EnemyView& view = _enemies[index];

        if(view.gone)
        {
            continue;
        }

        bn::fixed x = view.x + shake.x();
        bn::fixed y = view.y + shake.y() + wave(_ctx.frame + index * 23, 72, bn::fixed(1.5));

        if(view.hurt > 0)
        {
            x += (view.hurt % 4 < 2 ? 1 : -1) * (view.hurt / 4 + 1);
            bn::sprite_palette_ptr palette = view.parts[0].palette();
            palette.set_fade(bn::color(31, 31, 31), view.hurt > 10 ? bn::fixed(0.75) : bn::fixed(0));
            --view.hurt;
        }

        if(view.lunge > 0)
        {
            // Jump towards the player and back.
            bn::fixed t = bn::fixed(18 - view.lunge) / 18;
            bn::fixed reach = t < bn::fixed(0.5) ? ease_out(t * 2) : 1 - ease_in_out((t - bn::fixed(0.5)) * 2);
            x -= reach * 26;
            y -= hop(t, 6);
            --view.lunge;
        }

        if(view.arrive > 0)
        {
            // Drops from above into its place, glowing white until it lands.
            bn::fixed t = bn::fixed(ARRIVE_FRAMES - view.arrive) / ARRIVE_FRAMES;
            y -= (1 - ease_out(t)) * 48;
            bn::sprite_palette_ptr palette = view.parts[0].palette();
            palette.set_fade(bn::color(31, 31, 31), (1 - t) * bn::fixed(0.75));
            --view.arrive;

            if(view.arrive == 0)
            {
                palette.set_fade(bn::color(), 0);
            }
        }

        if(view.death > 0)
        {
            bn::fixed t = 1 - bn::fixed(view.death) / DEATH_FRAMES;
            bn::blending::set_transparency_alpha(1 - t);
            bn::sprites_mosaic::set_stretch(t);
            y += t * 10;
            --view.death;

            if(view.death == 0)
            {
                view.gone = true;

                for(bn::sprite_ptr& part : view.parts)
                {
                    part.set_visible(false);
                    part.set_blending_enabled(false);
                    part.set_mosaic_enabled(false);
                }

                bn::blending::set_transparency_alpha(1);
                bn::sprites_mosaic::set_stretch(0);
                view.bar.set_visible(false);
                _refresh_enemy_hud(index);
                continue;
            }
        }

        for(int part = 0; part < view.parts.size(); ++part)
        {
            int px = view.boss ? boss_parts[part].x : 0;
            int py = view.boss ? boss_parts[part].y : 0;
            view.parts[part].set_position(x + px, y + py);
        }

        // Intent bobbing above the head.
        if(view.intent_visible && view.death == 0)
        {
            int top = _enemy_top(view) + wave(_ctx.frame + index * 11, 40, 2).round_integer();
            view.intent_icon.set_position(_intent_x(view), top);
            view.intent_label.move_to(_intent_x(view) + 10, top);
        }

        view.bar.update();
    }
}

void CombatScene::_update_player()
{
    bn::fixed_point shake = _shake.offset();
    bn::fixed x = PLAYER_X + shake.x();
    bn::fixed y = PLAYER_Y + shake.y() + wave(_ctx.frame, 64, 1);

    if(_player_lunge > 0)
    {
        bn::fixed t = bn::fixed(16 - _player_lunge) / 16;
        bn::fixed reach = t < bn::fixed(0.4) ? ease_out(t / bn::fixed(0.4)) : 1 - ease_in_out((t - bn::fixed(0.4)) / bn::fixed(0.6));
        x += reach * 30;
        --_player_lunge;
    }

    if(_player_guard > 0)
    {
        x -= _player_guard > 7 ? 3 : 1;
        --_player_guard;
    }

    if(_player_hurt > 0)
    {
        x -= (_player_hurt % 4 < 2 ? 3 : 0) + _player_hurt / 5;
        --_player_hurt;
        _apply_player_tint();
    }

    if(_player_death > 0)
    {
        bn::fixed t = 1 - bn::fixed(_player_death) / DEATH_FRAMES;
        _player.set_blending_enabled(true);
        bn::blending::set_transparency_alpha(1 - t * bn::fixed(0.8));
        y += t * 12;
        --_player_death;
    }

    if(_phase != Phase::INTRO)
    {
        _player.set_position(x, y);
    }

    _player_bar.update();
}

void CombatScene::_update_cursor()
{
    if(_phase == Phase::TARGET && _target >= 0 && _target < _enemies.size())
    {
        EnemyView& view = _enemies[_target];
        int top = _enemy_top(view) - 10 + wave(_ctx.frame, 24, 2).round_integer();
        _target_cursor.set_position(view.x, bn::max(top, -74));
        _target_cursor.set_visible(true);
    }
    else
    {
        _target_cursor.set_visible(false);
    }
}

bn::optional<SceneId> CombatScene::update()
{
    _shake.update();

    switch(_phase)
    {
    case Phase::INTRO:
        _update_intro();
        break;
    case Phase::PLAYER:
        if(_combat.vakuu_turn_active)
        {
            _update_vakuu_turn();
            break;
        }

        // The engine waits for cards to be picked: after a card, at the start of a turn or of the combat.
        if(_combat.choice_pending())
        {
            _enter_choice();
            break;
        }

        _update_player_input();

        if(_ctx.transition == Transition::QUICK)
        {
            return _overlay;
        }
        break;
    case Phase::TARGET:
        _update_target_input();
        break;
    case Phase::POTIONS:
        _update_belt_input();
        break;
    case Phase::CHOICE:
        _update_choice_input();
        break;
    case Phase::CARD_FLIGHT:
        _update_card_flight();
        break;
    case Phase::ENEMY_TURN:
        _update_enemy_turn();
        break;
    default:
        if(bn::optional<SceneId> next = _update_end_banner())
        {
            return next;
        }
        break;
    }

    if(_phase != Phase::ENEMY_TURN || _timer >= ENEMY_TURN_LEAD)
    {
        _layout_hand();
    }

    _update_enemies();
    _update_player();
    _update_cursor();
    _refresh_card_text();
    _refresh_hud();

    if(_banner_timer > 0)
    {
        --_banner_timer;
        int age = 60 - _banner_timer;
        int x = age < 10 ? ((10 - age) * (10 - age) * 2) : 0;
        _banner.move_to(-x, _banner_y);
        _banner.set_visible(_banner_timer > 0);
    }

    _popups.update();
    _fx.update();
    return bn::nullopt;
}

}

bn::unique_ptr<Scene> make_combat_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new CombatScene(context));
}

}
