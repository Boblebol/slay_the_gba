#include "gba/scene_factories.h"

#include <span>

#include "bn_blending.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprites_mosaic.h"
#include "bn_sstream.h"
#include "bn_string.h"

#include "bn_sprite_items_fx_burst.h"
#include "bn_sprite_items_fx_slash.h"

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
constexpr int HAND_Y = 64;
constexpr int HAND_FOCUS_Y = 57;
constexpr int HAND_PICKED_Y = 50;
constexpr int CHOICE_BANNER_X = 36;
constexpr int LINE1_Y = 30;
constexpr int LINE2_Y = 41;
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

struct Point
{
    int x;
    int y;
};

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

    constexpr Point three[] = { { 4, -18 }, { 54, -24 }, { 104, -18 } };
    return three[bn::clamp(index, 0, 2)];
}

// Centre of a status line, moved towards the middle of the screen when the line is too long to fit around
// `wanted_x`: with three or four statuses it would run past the edge.
int status_line_x(int wanted_x, const bn::string_view& text)
{
    constexpr int EDGE = 118;
    int half = (text_width(text) + 1) / 2;

    if(half >= EDGE)
    {
        return 0;
    }

    return bn::clamp(wanted_x, half - EDGE, EDGE - half);
}

// Offsets of the four 64x64 quarters of a 128x128 boss sprite.
constexpr Point boss_parts[] = { { -32, -32 }, { 32, -32 }, { -32, 32 }, { 32, 32 } };

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
        CHOICE,
        CARD_FLIGHT,
        ENEMY_TURN,
        END_BANNER
    };

    struct EnemyView
    {
        EnemyView(int slot_x, int slot_y, int bar_x, bool is_boss) :
            bar(bar_x, BAR_Y),
            intent_icon(create_icon(Icon::ATTACK, slot_x, slot_y)),
            x(slot_x),
            y(slot_y),
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
        int shown_block = -1;
        int shown_icon = -1;
        int hurt = 0;
        int lunge = 0;
        int death = 0;
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
    bn::vector<EnemyView, 3> _enemies;
    bn::vector<HandCardView, 10> _hand;
    bn::optional<bn::sprite_ptr> _flying_card;
    bn::vector<uint8_t, 10> _picked; // hand cards marked for the choice the engine is asking
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

    PopupPool _popups;
    FxPool _fx;
    Shake _shake;

    Phase _phase = Phase::INTRO;
    int _timer = 0;
    int _selected = 0;
    int _target = 0;
    int _player_hurt = 0;
    int _player_lunge = 0;
    int _player_guard = 0;
    int _player_death = 0;
    int _banner_timer = 0;
    int _banner_x = 0;
    bool _victory = false;

    // Card flight
    engine::DeckCard _flying;
    int _flying_hand_index = 0;
    int _flying_target = 0;
    bn::fixed _flight_x0;
    bn::fixed _flight_y0;

    // Snapshot taken before an engine action, diffed afterwards to drive the animations.
    int _snap_player_hp = 0;
    int _snap_player_block = 0;
    int _snap_hp[3] = {};
    int _snap_block[3] = {};

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
    void _update_target_input();
    void _enter_choice();
    void _update_choice_input();
    void _resolve_choice();
    void _refresh_choice_banner();
    [[nodiscard]] bool _is_picked(int index) const;
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

    if(_ctx.combat_resume)
    {
        _ctx.combat_resume = false;
        _phase = Phase::PLAYER;
        _build_hand(false);
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
        int bar_x = slot.x;
        EnemyView& view = _enemies.emplace_back(slot.x, slot.y, bar_x, art.boss);

        if(art.boss)
        {
            for(int part = 0; part < 4; ++part)
            {
                bn::sprite_ptr sprite = art.item->create_sprite(slot.x + boss_parts[part].x,
                                                                slot.y + boss_parts[part].y, part);
                sprite.set_z_order(Z_ACTOR);
                view.parts.push_back(bn::move(sprite));
            }
        }
        else
        {
            bn::sprite_ptr sprite = art.item->create_sprite(slot.x, slot.y);
            sprite.set_z_order(Z_ACTOR);
            view.parts.push_back(bn::move(sprite));
        }

        if(count >= 3)
        {
            view.bar.set_horizontal_scale(bn::fixed(0.75));
        }

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
    bool interactive = _phase == Phase::PLAYER || _phase == Phase::TARGET || choosing;
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

        bool dim = interactive && (choosing ? ! _combat.choice_allows(index) : ! _combat.can_play_card(index));

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
        _line2.set(-112, LINE2_Y, "A : attaquer   B : annuler", Align::LEFT, TextColor::WHITE);
        _selected_shown = -1;
        return;
    }

    if(_phase == Phase::CARD_FLIGHT)
    {
        if(_selected_shown != -2)
        {
            _selected_shown = -2;
            TextLine text;
            card_name(_flying, text);
            _line1.set(-112, LINE1_Y, text, Align::LEFT, _flying.upgraded ? TextColor::GREEN : TextColor::GOLD);
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
        _line2.clear();
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

    if(_hand.empty())
    {
        _line1.set(-112, LINE1_Y, "Main vide", Align::LEFT, TextColor::GRAY);
        _line1_right.clear();
        _line2.set(-112, LINE2_Y, "START : finir le tour", Align::LEFT, TextColor::WHITE);
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
    _line1.set(-112, LINE1_Y, text, Align::LEFT, card.upgraded ? TextColor::GREEN : TextColor::GOLD);

    if(_phase == Phase::CHOICE)
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
        resource = player.minion_hp * 100 + player.souls;
        resource_stream << "Titos " << player.minion_hp << " PV";
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
            _player_block_label.set(PLAYER_X - 30, BAR_Y, text, Align::CENTER, TextColor::WHITE);
        }
        else
        {
            _player_block_icon.reset();
            _player_block_label.clear();
        }
    }

    TextLine status;
    status_line(player, status);

    if(status != _player_status_text)
    {
        _player_status_text = status;
        _player_status.set(status_line_x(PLAYER_X, status), STATUS_Y, status, Align::CENTER, TextColor::PURPLE);
    }

    bool interactive = _phase == Phase::PLAYER;

    if(interactive && ! _any_playable())
    {
        _hint_label.set(112, LINE1_Y, "START : fin du tour", Align::RIGHT, TextColor::GOLD);
        _hint_label.set_visible((_ctx.frame / 16) % 2 == 0);
    }
    else if(interactive && ! _hand.empty())
    {
        // The mini card shows the printed cost: a cost changed by the combat is written here instead.
        constexpr bn::string_view REMINDER = "START : fin du tour";
        TextLine cost;
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
                // The reminder gives way to a card name that would run into it.
                TextLine name;
                card_name(card, name);
                room = text_width(name) + text_width(REMINDER) + 4 <= 224;
            }
        }

        if(cost.empty())
        {
            _hint_label.set(112, LINE1_Y, REMINDER, Align::RIGHT, TextColor::GRAY);
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
    case engine::EnemyIntentType::DEFEND:
        icon = Icon::DEFEND;
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

    view.intent_icon.set_visible(view.intent_visible);
    view.intent_label.set(view.intent_label.x(), view.intent_label.y(), value, Align::LEFT,
                          icon == Icon::ATTACK ? TextColor::RED : TextColor::WHITE);
    view.intent_label.set_visible(view.intent_visible);

    TextLine status;
    status_line(enemy, status);

    if(status != view.status_text)
    {
        view.status_text = status;
        view.status_label.set(status_line_x(view.x, status), STATUS_Y, status, Align::CENTER, TextColor::PURPLE);
    }

    int block = enemy.block;

    if(block != view.shown_block)
    {
        view.shown_block = block;

        if(block > 0)
        {
            if(! view.block_icon)
            {
                view.block_icon = create_icon(Icon::DEFEND, view.x - 30, BAR_Y);
                view.block_icon->set_z_order(Z_FX);
            }

            bn::string<8> text;
            bn::ostringstream block_stream(text);
            block_stream << block;
            view.block_label.set(view.x - 30, BAR_Y, text);
        }
        else
        {
            view.block_icon.reset();
            view.block_label.clear();
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
    return view.boss ? view.x - 62 : view.x - 4;
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
    _banner_x = 0;
    _banner.set(0, -48, text, Align::CENTER, color);
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

    if(bn::keypad::select_pressed())
    {
        play_sfx(Sfx::UI_OPEN);
        _ctx.deck_return = SceneId::COMBAT;
        _ctx.combat_resume = true;
        _ctx.transition = Transition::QUICK;
        return;
    }

    if(bn::keypad::start_pressed() || bn::keypad::r_pressed())
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
        _phase = Phase::PLAYER;
    }
    else if(bn::keypad::a_pressed())
    {
        _start_card(_target);
    }
}

void CombatScene::_enter_choice()
{
    _phase = Phase::CHOICE;
    _picked.clear();
    _selected = bn::clamp(_selected, 0, bn::max(0, _hand.size() - 1));
    _selected_shown = -1;
    _refresh_choice_banner();
    play_sfx(Sfx::UI_OPEN);
}

bool CombatScene::_is_picked(int index) const
{
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

    const engine::HandChoice& choice = _combat.pending_choices[0];
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

    // Off centre: the pile counters are on the left, and the prompt stays as long as the choice does.
    _banner_x = CHOICE_BANNER_X;
    _banner.set(_banner_x, -48, text, Align::CENTER, TextColor::GOLD);
    _banner_timer = bn::max(_banner_timer, 50);
}

void CombatScene::_update_choice_input()
{
    if(! _combat.choice_pending() || _hand.empty())
    {
        _phase = Phase::PLAYER;
        _banner_timer = 1;
        _selected_shown = -1;
        return;
    }

    // The prompt stays on screen for as long as the choice is asked.
    _banner_timer = bn::max(_banner_timer, 50);
    int count = _hand.size();

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

    const bool optional = _combat.pending_choices[0].optional;
    const int wanted = _combat.choice_size();

    if(bn::keypad::a_pressed())
    {
        if(_is_picked(_selected))
        {
            for(int index = 0; index < _picked.size(); ++index)
            {
                if(_picked[index] == _selected)
                {
                    _picked.erase(_picked.begin() + index);
                    break;
                }
            }

            play_sfx(Sfx::UI_CANCEL);
        }
        else if(! _combat.choice_allows(_selected) || _picked.size() >= wanted)
        {
            play_sfx(Sfx::ERROR);
            _popups.spawn(_hand[_selected].x.round_integer(), HAND_FOCUS_Y - 24, "Impossible", TextColor::RED);
            return;
        }
        else
        {
            _picked.push_back(uint8_t(_selected));
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
        // Another choice follows (a card played twice asks twice).
        _refresh_choice_banner();
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

        // The view was removed from the hand; the engine plays the same index.
        _combat.play_card(_flying_hand_index, _flying_target);
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
                            record.intent_type == engine::EnemyIntentType::ATTACK_DEFEND;

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

            if(record.intent_type == engine::EnemyIntentType::BUFF)
            {
                _popups.spawn(view.x, _body_y(view) - 6, "Force ▲", TextColor::GOLD);
                _fx.spawn(bn::sprite_items::fx_burst, view.x, _body_y(view), FX_BUFF, 4, 4);
                play_sfx(Sfx::BUFF);
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
                play_sfx(Sfx::ENEMY_DEATH);

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
        _set_banner(_victory ? "VICTOIRE !" : "DÉFAITE...", _victory ? TextColor::GOLD : TextColor::RED);
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

    if(run.phase == engine::RunPhase::ANCIENT_BLESSING)
    {
        run.phase = engine::RunPhase::MAP_NAVIGATION;
        _ctx.nav.reset_to_player_floor();
        _ctx.map_toast.clear();
        bn::ostringstream stream(_ctx.map_toast);
        stream << "ACTE " << int(run.current_act) << " : l'ascension continue !";
        return SceneId::MAP;
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
        // The engine waits for cards to be picked: after a card, at the start of a turn or of the combat.
        if(_combat.choice_pending() && ! _hand.empty())
        {
            _enter_choice();
            break;
        }

        _update_player_input();

        if(_ctx.transition == Transition::QUICK)
        {
            return SceneId::DECK;
        }
        break;
    case Phase::TARGET:
        _update_target_input();
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
        _banner.move_to(_banner_x - x, -48);
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
