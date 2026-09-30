#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sstream.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bg_map.h"
#include "bn_sprite_items_map_boss_ceremonial_beast.h"
#include "bn_sprite_items_map_node_elite.h"
#include "bn_sprite_items_map_node_event.h"
#include "bn_sprite_items_map_node_monster.h"
#include "bn_sprite_items_map_node_rest.h"
#include "bn_sprite_items_map_node_shop.h"
#include "bn_sprite_items_map_node_treasure.h"
#include "bn_sprite_items_map_path_dot.h"

#include "engine/save_system.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/fx.h"
#include "gba/layers.h"
#include "gba/text.h"
#include "gba/widgets.h"

namespace gba
{

namespace
{

constexpr int FLOOR_SPACING = 30;
constexpr int SCREEN_ANCHOR_Y = 12;     // screen y of the camera's focus point
constexpr int LANE_X[] = { -62, 0, 62 };
constexpr int WALK_FRAMES = 26;
constexpr int WINDOW_BELOW = 2;
constexpr int WINDOW_ABOVE = 4;

const bn::sprite_item& node_item(engine::RoomType type)
{
    switch(type)
    {
    case engine::RoomType::ELITE:
        return bn::sprite_items::map_node_elite;
    case engine::RoomType::REST_SITE:
        return bn::sprite_items::map_node_rest;
    case engine::RoomType::SHOP:
        return bn::sprite_items::map_node_shop;
    case engine::RoomType::TREASURE:
        return bn::sprite_items::map_node_treasure;
    case engine::RoomType::EVENT:
        return bn::sprite_items::map_node_event;
    case engine::RoomType::BOSS:
        return bn::sprite_items::map_boss_ceremonial_beast;
    default:
        return bn::sprite_items::map_node_monster;
    }
}

class MapScene final : public Scene
{
public:
    explicit MapScene(GameContext& context) :
        _ctx(context),
        _run(context.run),
        _bg(bn::regular_bg_items::bg_map.create_bg(BG_ORIGIN_X, 176)),
        _pawn(hero_map_pawn().create_sprite(0, 0)),
        _cursor(hero_map_cursor().create_sprite(0, 0)),
        _heart(create_icon(Icon::HEART, -106, -72)),
        _gold(create_icon(Icon::GOLD, -46, -72))
    {
        _pawn.set_z_order(Z_ACTOR);
        _cursor.set_z_order(Z_HUD);
        _ctx.nav.reset_to_player_floor();
        _cam = _focus_world_y();
        play_music(Music::MAP);

        // Checkpoint between rooms: "Continuer" on the title screen resumes from here.
        if(_run.phase == engine::RunPhase::MAP_NAVIGATION)
        {
            engine::SaveManager::instance().autosave_run(_run);
        }
        _hints.set(0, 72, "A Entrer  ◄► Route  ▲▼ Carte  SEL Deck", Align::CENTER, TextColor::GRAY);

        if(! _ctx.map_toast.empty())
        {
            _toast_timer = 150;
            _toast.set(0, -56, _ctx.map_toast, Align::CENTER, TextColor::GOLD);
            _ctx.map_toast.clear();
        }

        _rebuild_window();
        _refresh_hud();
        _refresh_route_text();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        ++_timer;

        if(_walk > 0)
        {
            if(bn::optional<SceneId> next = _update_walk())
            {
                return next;
            }
        }
        else
        {
            if(bn::optional<SceneId> next = _update_input())
            {
                return next;
            }
        }

        _cam = approach(_cam, _focus_world_y(), bn::fixed(0.14));

        int window = (_cam / FLOOR_SPACING).floor_integer();

        if(window != _window_floor)
        {
            _rebuild_window();
        }

        _bg.set_y(176 + (_cam.floor_integer() % 512));
        _place_sprites();

        if(_toast_timer > 0)
        {
            --_toast_timer;
            _toast.set_visible(_toast_timer > 30 || (_toast_timer / 3) % 2 == 0);

            if(_toast_timer == 0)
            {
                _toast.clear();
            }
        }

        return bn::nullopt;
    }

private:
    struct NodeSprite
    {
        int node;
        bn::sprite_ptr sprite;
    };

    struct DotSprite
    {
        bn::sprite_ptr sprite;
        int world_x;
        int world_y;
        bool reachable;
    };

    GameContext& _ctx;
    engine::RunManager& _run;
    bn::regular_bg_ptr _bg;
    bn::sprite_ptr _pawn;
    bn::sprite_ptr _cursor;
    bn::sprite_ptr _heart;
    bn::sprite_ptr _gold;
    bn::vector<NodeSprite, 24> _nodes;
    bn::vector<DotSprite, 40> _dots;
    Label _zone;
    Label _hp;
    Label _gold_label;
    Label _floor;
    Label _route;
    Label _hints;
    Label _toast;
    bn::fixed _cam;
    int _window_floor = -1000;
    int _timer = 0;
    int _walk = 0;
    int _walk_target = -1;
    int _toast_timer = 0;
    bn::fixed _walk_x0;
    bn::fixed _walk_y0;

    [[nodiscard]] static int node_world_x(const engine::MapNode& node)
    {
        if(node.type == engine::RoomType::BOSS)
        {
            return 0;
        }

        // Deterministic jitter so the tower does not look like a spreadsheet.
        int jitter = ((node.floor * 7 + node.lane * 13) % 9) - 4;
        return LANE_X[node.lane] + jitter;
    }

    [[nodiscard]] static int node_world_y(const engine::MapNode& node)
    {
        return node.floor * FLOOR_SPACING + (node.type == engine::RoomType::BOSS ? 8 : 0);
    }

    [[nodiscard]] int screen_y(int world_y) const
    {
        return (_cam - world_y).round_integer() + SCREEN_ANCHOR_Y;
    }

    [[nodiscard]] bn::fixed _focus_world_y() const
    {
        return _ctx.nav.scroll_floor * FLOOR_SPACING + FLOOR_SPACING / 2;
    }

    [[nodiscard]] int _current_floor() const
    {
        if(_run.current_node_index >= 0)
        {
            return _run.map.nodes[_run.current_node_index].floor;
        }

        return -1;
    }

    [[nodiscard]] bool _is_available(int node_index) const
    {
        auto routes = _ctx.nav.get_available_routes();

        for(int route : routes)
        {
            if(route == node_index)
            {
                return true;
            }
        }

        return false;
    }

    void _rebuild_window()
    {
        _window_floor = (_cam / FLOOR_SPACING).floor_integer();
        int low = _window_floor - WINDOW_BELOW;
        int high = _window_floor + WINDOW_ABOVE;
        _nodes.clear();
        _dots.clear();

        const auto& nodes = _run.map.nodes;

        for(int index = 0; index < int(nodes.size()); ++index)
        {
            const engine::MapNode& node = nodes[index];

            if(node.floor < low || node.floor > high || _nodes.full())
            {
                continue;
            }

            bn::sprite_ptr sprite = node_item(node.type).create_sprite(node_world_x(node), 200);
            sprite.set_z_order(Z_HUD);
            _nodes.push_back(NodeSprite{ index, bn::move(sprite) });

            // Path dots towards the children (drawn only once per edge, from the lower node).
            bool from_current = index == _run.current_node_index;

            for(int child_slot = 0; child_slot < node.children_count; ++child_slot)
            {
                const engine::MapNode& child = nodes[node.children[child_slot]];

                if(child.floor > high)
                {
                    continue;
                }

                bool reachable = from_current;

                for(int step = 1; step <= 2 && ! _dots.full(); ++step)
                {
                    int wx = node_world_x(node) + (node_world_x(child) - node_world_x(node)) * step / 3;
                    int wy = node_world_y(node) + (node_world_y(child) - node_world_y(node)) * step / 3;
                    bn::sprite_ptr dot = bn::sprite_items::map_path_dot.create_sprite(wx, 200);
                    dot.set_z_order(Z_BACK);
                    _dots.push_back(DotSprite{ bn::move(dot), wx, wy, reachable });
                }
            }
        }
    }

    void _place_sprites()
    {
        int target = _ctx.nav.get_selected_target_node();

        for(NodeSprite& entry : _nodes)
        {
            const engine::MapNode& node = _run.map.nodes[entry.node];
            int y = screen_y(node_world_y(node));
            bool available = _walk == 0 && _is_available(entry.node);

            if(available)
            {
                y += wave(_timer + entry.node * 9, 36, bn::fixed(1.5)).round_integer();
            }

            entry.sprite.set_position(node_world_x(node), y);
            entry.sprite.set_visible(y > -62 && y < 50);
        }

        for(DotSprite& dot : _dots)
        {
            int y = screen_y(dot.world_y);
            dot.sprite.set_position(dot.world_x, y);
            dot.sprite.set_visible(y > -60 && y < 48 && (! dot.reachable || (_timer / 8) % 4 != 0));
        }

        // Selection ring on the chosen route.
        if(target >= 0 && _walk == 0)
        {
            const engine::MapNode& node = _run.map.nodes[target];
            int pulse = wave(_timer, 24, bn::fixed(1.5)).round_integer();
            int cursor_y = screen_y(node_world_y(node)) + pulse;
            _cursor.set_position(node_world_x(node), cursor_y);
            _cursor.set_visible(cursor_y > -62 && cursor_y < 50);
        }
        else
        {
            _cursor.set_visible(false);
        }

        // Pawn idles on the current room, or at the tower's gate before the first floor.
        if(_walk == 0)
        {
            int px = 0;
            int py = -FLOOR_SPACING;

            if(_run.current_node_index >= 0)
            {
                const engine::MapNode& node = _run.map.nodes[_run.current_node_index];
                px = node_world_x(node);
                py = node_world_y(node);
            }

            int bob = wave(_timer, 48, 2).round_integer();
            int pawn_y = screen_y(py) - 11 - bn::abs(bob);
            _pawn.set_position(px, pawn_y);
            _pawn.set_visible(pawn_y > -66 && pawn_y < 48);
        }
    }

    void _refresh_hud()
    {
        bn::string<24> text;
        bn::ostringstream stream(text);
        stream << _run.hp << '/' << _run.max_hp;
        _hp.set(-96, -72, text, Align::LEFT, TextColor::WHITE);

        text.clear();
        stream << _run.gold;
        _gold_label.set(-36, -72, text, Align::LEFT, TextColor::GOLD);

        text.clear();
        stream << "Acte " << int(_run.current_act) << " - Étage " << _ctx.nav.get_current_floor() << "/16";
        _floor.set(114, -72, text, Align::RIGHT, TextColor::WHITE);
    }

    void _refresh_route_text()
    {
        int target = _ctx.nav.get_selected_target_node();

        if(target < 0)
        {
            _route.clear();
            return;
        }

        const engine::MapNode& node = _run.map.nodes[target];
        std::string_view name = _run.map.get_room_name(node.type);
        TextLine text;
        text.append("► ");
        text.append(bn::string_view(name.data(), name.size()));
        TextColor color = node.type == engine::RoomType::ELITE || node.type == engine::RoomType::BOSS
                ? TextColor::RED : TextColor::WHITE;
        _route.set(0, 58, text, Align::CENTER, color);
    }

    bn::optional<SceneId> _update_input()
    {
        if(bn::keypad::up_pressed())
        {
            _ctx.nav.scroll_up();
            play_sfx(Sfx::UI_CURSOR);
        }
        else if(bn::keypad::down_pressed())
        {
            _ctx.nav.scroll_down();
            play_sfx(Sfx::UI_CURSOR);
        }

        if(bn::keypad::left_pressed())
        {
            _ctx.nav.prev_route();
            play_sfx(Sfx::UI_CURSOR);
            _ctx.nav.scroll_floor = bn::max(0, _current_floor());
            _refresh_route_text();
        }
        else if(bn::keypad::right_pressed())
        {
            _ctx.nav.next_route();
            play_sfx(Sfx::UI_CURSOR);
            _ctx.nav.scroll_floor = bn::max(0, _current_floor());
            _refresh_route_text();
        }

        if(bn::keypad::select_pressed())
        {
            play_sfx(Sfx::UI_OPEN);
            _ctx.deck_return = SceneId::MAP;
            _ctx.transition = Transition::QUICK;
            return SceneId::DECK;
        }

        if(bn::keypad::a_pressed())
        {
            int target = _ctx.nav.get_selected_target_node();

            if(target >= 0)
            {
                play_sfx(Sfx::UI_CONFIRM);
                _walk_target = target;
                _walk = WALK_FRAMES;
                _walk_x0 = _pawn.x();
                _walk_y0 = -FLOOR_SPACING;

                if(_run.current_node_index >= 0)
                {
                    _walk_y0 = node_world_y(_run.map.nodes[_run.current_node_index]);
                }
                _ctx.nav.scroll_floor = bn::max(0, _current_floor());
                _route.clear();
            }
        }

        return bn::nullopt;
    }

    bn::optional<SceneId> _update_walk()
    {
        --_walk;
        const engine::MapNode& node = _run.map.nodes[_walk_target];
        bn::fixed t = bn::fixed(WALK_FRAMES - _walk) / WALK_FRAMES;
        bn::fixed eased = ease_in_out(t);
        bn::fixed wx = lerp(_walk_x0, node_world_x(node), eased);
        bn::fixed wy = lerp(_walk_y0, node_world_y(node), eased);

        // Two little hops along the way.
        bn::fixed hop_t = (t * 2) - (t * 2).floor_integer();
        bn::fixed height = hop(hop_t, 7);
        _pawn.set_position(wx, screen_y(wy.round_integer()) - 11 - height);
        _pawn.set_horizontal_flip(node_world_x(node) < _walk_x0);

        // Let the camera follow the pawn upwards.
        _ctx.nav.scroll_floor = node.floor;

        if(_walk > 0)
        {
            return bn::nullopt;
        }

        _pawn.set_horizontal_flip(false);
        _ctx.nav.select_current_route();
        SceneId next = scene_for_run_phase(_ctx);

        if(next == SceneId::MAP)
        {
            // Treasure: stay here and announce the loot.
            _refresh_hud();
            _refresh_route_text();
            _rebuild_window();

            if(! _ctx.map_toast.empty())
            {
                _toast_timer = 150;
                _toast.set(0, -56, _ctx.map_toast, Align::CENTER, TextColor::GOLD);
                _ctx.map_toast.clear();
            }

            return bn::nullopt;
        }

        return next;
    }
};

}

bn::unique_ptr<Scene> make_map_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new MapScene(context));
}

}
