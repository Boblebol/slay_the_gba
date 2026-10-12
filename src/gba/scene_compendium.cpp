#include "gba/scene_factories.h"

#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sstream.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "bn_regular_bg_items_bg_map.h"

#include "engine/card.h"
#include "engine/character.h"
#include "engine/compendium.h"
#include "engine/monster.h"
#include "engine/potion.h"
#include "engine/relic.h"
#include "engine/save_system.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/card_text.h"
#include "gba/loot_text.h"
#include "gba/text.h"

namespace gba
{

namespace
{

constexpr int TITLE_Y = -72;
constexpr int HINT_Y = 73;
constexpr int LEFT_X = -112;

// Compendium (#273): the four sections, then the scrolling list of what the profile has met, with the sheet of the
// entry under the cursor.
class CompendiumScene final : public Scene
{
public:
    explicit CompendiumScene(GameContext& context) :
        _ctx(context),
        _bg(bn::regular_bg_items::bg_map.create_bg(BG_ORIGIN_X, 176 + (context.frame % 512)))
    {
        _show_sections();
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        return _in_list ? _update_list() : _update_sections();
    }

private:
    static constexpr int SECTION_Y = -40;
    static constexpr int SECTION_STEP = 16;
    static constexpr int ROWS = 6;
    static constexpr int ROW_Y = -56;
    static constexpr int ROW_STEP = 12;
    static constexpr int KIND_Y = 18;
    static constexpr int TEXT_Y = 30;
    static constexpr int TEXT_STEP = 10;
    static constexpr int TEXT_LINES = 4;
    static constexpr int TEXT_WIDTH = 224;
    static constexpr int MAX_ENTRIES = int(engine::CardId::COUNT);

    using LongText = bn::string<192>;

    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    Label _title;
    Label _hint;
    Label _section_names[engine::COMPENDIUM_SECTION_COUNT];
    Label _section_counts[engine::COMPENDIUM_SECTION_COUNT];
    Label _rows[ROWS];
    Label _kind;
    Label _text[TEXT_LINES];
    Label _empty;
    uint16_t _entries[MAX_ENTRIES] = {};
    int _entry_count = 0;
    int _section = 0;
    int _cursor = 0;
    bool _in_list = false;

    [[nodiscard]] engine::CompendiumSection _current() const
    {
        return engine::CompendiumSection(_section);
    }

    [[nodiscard]] static const engine::SaveProfile& _profile()
    {
        return engine::SaveManager::instance().get_active_profile();
    }

    [[nodiscard]] bn::optional<SceneId> _update_sections()
    {
        if(bn::keypad::up_pressed() || bn::keypad::down_pressed())
        {
            play_sfx(Sfx::UI_CURSOR);
            int step = bn::keypad::down_pressed() ? 1 : engine::COMPENDIUM_SECTION_COUNT - 1;
            _section = (_section + step) % engine::COMPENDIUM_SECTION_COUNT;
            _refresh_sections();
        }
        else if(bn::keypad::a_pressed())
        {
            play_sfx(Sfx::UI_CONFIRM);
            _show_list();
        }
        else if(bn::keypad::b_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _ctx.title_menu = true;
            _ctx.transition = Transition::QUICK;
            return SceneId::TITLE;
        }

        return bn::nullopt;
    }

    [[nodiscard]] bn::optional<SceneId> _update_list()
    {
        int step = 0;

        if(bn::keypad::down_pressed())
        {
            step = 1;
        }
        else if(bn::keypad::up_pressed())
        {
            step = -1;
        }
        else if(bn::keypad::right_pressed())
        {
            step = ROWS;
        }
        else if(bn::keypad::left_pressed())
        {
            step = -ROWS;
        }

        if(step != 0)
        {
            int moved = engine::compendium_move(_cursor, step, _entry_count);

            if(moved != _cursor)
            {
                _cursor = moved;
                play_sfx(Sfx::UI_CURSOR);
                _refresh_list();
            }
        }
        else if(bn::keypad::b_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _show_sections();
        }

        return bn::nullopt;
    }

    void _clear_list()
    {
        for(Label& label : _rows)
        {
            label.clear();
        }

        for(Label& label : _text)
        {
            label.clear();
        }

        _kind.clear();
        _empty.clear();
    }

    void _show_sections()
    {
        _in_list = false;
        _clear_list();
        _title.set(0, TITLE_Y, "COMPENDIUM", Align::CENTER, TextColor::GOLD);
        _hint.set(0, HINT_Y, "▲▼ Choisir   A Ouvrir   B Retour", Align::CENTER, TextColor::GRAY);
        _refresh_sections();
    }

    void _refresh_sections()
    {
        const engine::SaveProfile& profile = _profile();

        for(int index = 0; index < engine::COMPENDIUM_SECTION_COUNT; ++index)
        {
            auto section = engine::CompendiumSection(index);
            bool focused = index == _section;
            std::string_view name = engine::compendium_section_name(section);
            TextLine line;
            line.append(focused ? "► " : "  ");
            line.append(bn::string_view(name.data(), name.size()));
            int y = SECTION_Y + index * SECTION_STEP;
            TextColor color = focused ? TextColor::GOLD : TextColor::WHITE;
            _section_names[index].set(-80, y, line, Align::LEFT, color);

            TextLine count;
            bn::ostringstream stream(count);
            stream << int(engine::compendium_seen_count(profile, section)) << " / " << int(engine::compendium_total(section));
            _section_counts[index].set(80, y, count, Align::RIGHT, color);
        }
    }

    void _show_list()
    {
        for(int index = 0; index < engine::COMPENDIUM_SECTION_COUNT; ++index)
        {
            _section_names[index].clear();
            _section_counts[index].clear();
        }

        _in_list = true;
        _cursor = 0;
        _entry_count = int(engine::compendium_seen_entries(_profile(), _current(), _entries));

        std::string_view name = engine::compendium_section_name(_current());
        TextLine title;
        bn::ostringstream stream(title);
        stream << bn::string_view(name.data(), name.size()) << " - " << _entry_count << " / "
               << int(engine::compendium_total(_current()));
        _title.set(0, TITLE_Y, title, Align::CENTER, TextColor::GOLD);
        _hint.set(0, HINT_Y, "▲▼ Parcourir  ◄► Page  B Retour", Align::CENTER, TextColor::GRAY);
        _refresh_list();
    }

    void _refresh_list()
    {
        _clear_list();

        if(_entry_count == 0)
        {
            _empty.set(0, 0, "Rien de rencontré pour l'instant.", Align::CENTER, TextColor::GRAY);
            return;
        }

        int first = engine::compendium_first_row(_cursor, _entry_count, ROWS);

        for(int row = 0; row < ROWS && first + row < _entry_count; ++row)
        {
            int index = first + row;
            bool focused = index == _cursor;
            TextLine line;
            line.append(focused ? "► " : "  ");
            _append_name(_entries[index], line);
            _rows[row].set(LEFT_X, ROW_Y + row * ROW_STEP, line, Align::LEFT,
                           focused ? TextColor::GOLD : TextColor::WHITE);
        }

        TextLine kind;
        LongText text;
        _sheet(_entries[_cursor], kind, text);
        _kind.set(LEFT_X, KIND_Y, kind, Align::LEFT, TextColor::GRAY);

        bn::vector<TextLine, TEXT_LINES> lines;
        wrap_text(text, TEXT_WIDTH, lines);

        for(int index = 0; index < lines.size(); ++index)
        {
            _text[index].set(LEFT_X, TEXT_Y + index * TEXT_STEP, lines[index], Align::LEFT, TextColor::WHITE);
        }
    }

    void _append_name(uint16_t entry, TextLine& line) const
    {
        switch(_current())
        {
        case engine::CompendiumSection::CARDS:
        {
            TextLine name;
            card_name(engine::DeckCard(engine::CardId(entry)), name);
            line.append(name);
            break;
        }
        case engine::CompendiumSection::RELICS:
            line.append(relic_name(engine::get_all_relics()[entry].id));
            break;
        case engine::CompendiumSection::MONSTERS:
        {
            std::string_view name = engine::get_all_monsters()[entry].name;
            line.append(bn::string_view(name.data(), name.size()));
            break;
        }
        default:
            line.append(potion_name(engine::get_all_potions()[entry].id));
            break;
        }
    }

    // The sheet of an entry: its kind on one line, then what it does.
    void _sheet(uint16_t entry, TextLine& kind, LongText& text) const
    {
        bn::ostringstream kind_stream(kind);

        switch(_current())
        {
        case engine::CompendiumSection::CARDS:
        {
            engine::DeckCard card{ engine::CardId(entry) };
            const engine::CardDef& def = engine::CardDef::get(card.id);
            kind_stream << card_type_name(def.type) << " - " << card_cost_label(card) << " ●";

            if(bn::string_view stars = card_star_cost_label(card); ! stars.empty())
            {
                kind_stream << "  " << stars << " ★";
            }

            card_description(card, text);
            break;
        }
        case engine::CompendiumSection::RELICS:
        {
            std::string_view id = engine::get_all_relics()[entry].id;
            kind_stream << relic_kind(id);
            text.append(relic_description(id));
            break;
        }
        case engine::CompendiumSection::MONSTERS:
            _monster_sheet(engine::get_all_monsters()[entry], kind_stream, text);
            break;
        default:
        {
            std::string_view id = engine::get_all_potions()[entry].id;
            kind_stream << potion_kind(id);
            text.append(potion_description(id));
            break;
        }
        }
    }

    // "Élite - PV 40-44", then its moves with their damage and Armor: "Morsure 7, Armure 7 ; ...".
    static void _monster_sheet(const engine::MonsterData& monster, bn::ostringstream& kind, LongText& text)
    {
        std::string_view type = monster.encounter_type;
        bn::string_view label = "Monstre";

        if(type == "boss")
        {
            label = "Boss";
        }
        else if(type == "elite")
        {
            label = "Élite";
        }
        else if(type == "summon")
        {
            label = "Invocation";
        }

        kind << label << " - PV " << int(monster.min_hp);

        if(monster.max_hp != monster.min_hp)
        {
            kind << "-" << int(monster.max_hp);
        }

        bn::ostringstream stream(text);
        bool first = true;

        for(const engine::MonsterMoveData& move : monster.moves)
        {
            // Room for the longest move entry: stop before the text overflows.
            if(text.size() + 48 > text.max_size())
            {
                break;
            }

            if(! first)
            {
                stream << " ; ";
            }

            first = false;
            stream << bn::string_view(move.name.data(), move.name.size());

            if(move.damage > 0)
            {
                stream << " " << int(move.damage);

                if(move.hits > 1)
                {
                    stream << "x" << int(move.hits);
                }
            }

            if(move.block > 0)
            {
                stream << (move.damage > 0 ? ", Armure " : " Armure ") << int(move.block);
            }
        }
    }
};

// Statistics (#273): runs and victories of the active profile, and the Ascension unlocked with each hero.
class StatisticsScene final : public Scene
{
public:
    explicit StatisticsScene(GameContext& context) :
        _ctx(context),
        _bg(bn::regular_bg_items::bg_map.create_bg(BG_ORIGIN_X, 176 + (context.frame % 512)))
    {
        const engine::SaveProfile& profile = engine::SaveManager::instance().get_active_profile();
        engine::ProfileStats stats = engine::profile_stats(profile);

        _title.set(0, TITLE_Y, "STATISTIQUES", Align::CENTER, TextColor::GOLD);

        TextLine name;
        int length = 0;

        while(length < int(sizeof(profile.name)) && profile.name[length] != '\0')
        {
            ++length;
        }

        name.append(bn::string_view(profile.name, length));
        _profile.set(0, TITLE_Y + 12, name, Align::CENTER, TextColor::GRAY);

        _set_row(0, -46, "Parties jouées", int(stats.runs), "");
        _set_row(1, -36, "Victoires", int(stats.victories), "");
        _set_row(2, -26, "Défaites", int(stats.defeats), "");
        _set_row(3, -16, "Taux de victoire", int(stats.win_percent), " %");

        _heading.set(0, 0, "ASCENSION DÉBLOQUÉE", Align::CENTER, TextColor::GOLD);

        for(int index = 0; index < HEROES; ++index)
        {
            auto hero = engine::CharacterId(index);
            const engine::CharacterDef& def = engine::get_character(hero);
            TextColor color = hero == current_hero() ? TextColor::GOLD : TextColor::WHITE;
            int y = 12 + index * 10;
            _hero_names[index].set(-100, y, bn::string_view(def.name.data(), def.name.size()), Align::LEFT, color);

            TextLine level;
            bn::ostringstream stream(level);
            stream << "A" << int(engine::hero_ascension(profile, hero));
            _hero_levels[index].set(100, y, level, Align::RIGHT, color);
        }

        _hint.set(0, HINT_Y, "B Retour", Align::CENTER, TextColor::GRAY);
    }

    [[nodiscard]] bn::optional<SceneId> update() final
    {
        if(bn::keypad::b_pressed() || bn::keypad::a_pressed())
        {
            play_sfx(Sfx::UI_CANCEL);
            _ctx.title_menu = true;
            _ctx.transition = Transition::QUICK;
            return SceneId::TITLE;
        }

        return bn::nullopt;
    }

private:
    static constexpr int HEROES = int(engine::CharacterId::COUNT);
    static constexpr int ROWS = 4;

    GameContext& _ctx;
    bn::regular_bg_ptr _bg;
    Label _title;
    Label _profile;
    Label _labels[ROWS];
    Label _values[ROWS];
    Label _heading;
    Label _hero_names[HEROES];
    Label _hero_levels[HEROES];
    Label _hint;

    void _set_row(int row, int y, const bn::string_view& label, int value, const bn::string_view& unit)
    {
        _labels[row].set(-100, y, label, Align::LEFT, TextColor::WHITE);
        TextLine text;
        bn::ostringstream stream(text);
        stream << value << unit;
        _values[row].set(100, y, text, Align::RIGHT, TextColor::WHITE);
    }
};

}

bn::unique_ptr<Scene> make_compendium_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new CompendiumScene(context));
}

bn::unique_ptr<Scene> make_statistics_scene(GameContext& context)
{
    return bn::unique_ptr<Scene>(new StatisticsScene(context));
}

}
