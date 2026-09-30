#include "gba/card_picker.h"

#include "bn_keypad.h"
#include "bn_sstream.h"

#include "gba/assets.h"
#include "gba/audio.h"
#include "gba/card_text.h"
#include "gba/fx.h"
#include "gba/layers.h"

namespace gba
{

CardPicker::CardPicker(const CardPickerLayout& layout) :
    _layout(layout)
{
}

void CardPicker::clear()
{
    _cards.clear();
    _slots.clear();
    _preview.reset();
    _text.clear();
    _counter.clear();
    _selected = 0;
    _first = 0;
    _dirty = true;
}

void CardPicker::add(const engine::DeckCard& card, int tag)
{
    if(! _cards.full())
    {
        _cards.push_back(Entry{ card, tag });
        _dirty = true;
    }
}

void CardPicker::set_preview_upgraded(bool upgraded)
{
    _upgraded_preview = upgraded;
    _dirty = true;
}

void CardPicker::set_selected(int index)
{
    _selected = bn::clamp(index, 0, bn::max(0, _cards.size() - 1));
    _dirty = true;
}

void CardPicker::set_visible(bool visible)
{
    _visible = visible;

    for(Slot& slot : _slots)
    {
        slot.sprite.set_visible(visible);
    }

    if(_preview)
    {
        _preview->set_visible(visible);
    }

    for(Label& label : _text)
    {
        label.set_visible(visible);
    }

    _counter.set_visible(visible);
}

bool CardPicker::update(int frame)
{
    bool changed = false;
    int count = _cards.size();

    if(count > 0 && _visible)
    {
        if(bn::keypad::left_pressed())
        {
            _selected = (_selected + count - 1) % count;
            changed = true;
            play_sfx(Sfx::UI_CURSOR);
        }
        else if(bn::keypad::right_pressed())
        {
            _selected = (_selected + 1) % count;
            changed = true;
            play_sfx(Sfx::UI_CURSOR);
        }
    }

    if(changed || _dirty)
    {
        // Scroll the window so the selection stays visible.
        if(_selected < _first)
        {
            _first = _selected;
        }
        else if(_selected >= _first + VISIBLE)
        {
            _first = _selected - VISIBLE + 1;
        }

        _rebuild_slots();
        _refresh_preview();
        _dirty = false;
    }

    int shown = _slots.size();
    int start = -((shown - 1) * SPACING) / 2;

    for(int index = 0; index < shown; ++index)
    {
        Slot& slot = _slots[index];
        bool focused = slot.index == _selected;
        bn::fixed target = _layout.strip_y - (focused ? 7 : 0);
        slot.y = approach(slot.y, target, bn::fixed(0.35));
        slot.sprite.set_position(start + index * SPACING, slot.y);
        slot.sprite.set_z_order(focused ? Z_CARD_FOCUS : Z_CARD);
    }

    // The preview pops in with a small scale bounce and then floats.
    if(_preview)
    {
        ++_preview_age;
        bn::fixed scale = 1;

        if(_preview_age < 8)
        {
            scale = lerp(bn::fixed(0.7), bn::fixed(1.08), ease_out(bn::fixed(_preview_age) / 8));
        }
        else if(_preview_age < 12)
        {
            scale = bn::fixed(1.08) - bn::fixed(_preview_age - 8) / 50;
        }

        if(_preview_age < 12)
        {
            _preview->set_scale(scale);
        }
        else if(_preview_age == 12)
        {
            _preview->remove_affine_mat();
        }

        _preview->set_y(_layout.preview_y + wave(frame, 80, bn::fixed(1.5)));
    }

    return changed;
}

void CardPicker::_rebuild_slots()
{
    _slots.clear();
    int last = bn::min(_cards.size(), _first + VISIBLE);

    for(int index = _first; index < last; ++index)
    {
        bn::sprite_ptr sprite = hero_hand_cards().create_sprite(0, _layout.strip_y,
                                                               hand_card_frame(_cards[index].card));
        sprite.set_visible(_visible);
        _slots.push_back(Slot{ bn::move(sprite), index, _layout.strip_y });
    }

    if(_cards.size() > VISIBLE)
    {
        bn::string<16> text;
        bn::ostringstream stream(text);
        stream << (_selected + 1) << '/' << _cards.size();
        _counter.set(112, _layout.strip_y - 22, text, Align::RIGHT, TextColor::GRAY);
        _counter.set_visible(_visible);
    }
}

void CardPicker::_refresh_preview()
{
    _text.clear();

    if(_cards.empty())
    {
        _preview.reset();
        return;
    }

    engine::DeckCard card = _cards[_selected].card;

    if(_upgraded_preview)
    {
        card.upgraded = true;
    }

    bn::sprite_ptr sprite = big_card_item(card.id).create_sprite(_layout.preview_x, _layout.preview_y);
    sprite.set_z_order(Z_CARD_FOCUS);
    sprite.set_scale(bn::fixed(0.7));
    sprite.set_visible(_visible);
    _preview = bn::move(sprite);
    _preview_age = 0;

    TextLine line;
    card_name(card, line);
    Label& name = _text.emplace_back();
    name.set(_layout.text_x, _layout.text_y, line, Align::LEFT, card.upgraded ? TextColor::GREEN : TextColor::GOLD);

    const engine::CardDef& def = engine::CardDef::get(card.id);
    TextLine type;
    bn::ostringstream stream(type);
    stream << card_type_name(def.type) << " - " << int(engine::CardDef::get_cost(card.id, card.upgraded)) << " ●";
    Label& type_label = _text.emplace_back();
    type_label.set(_layout.text_x, _layout.text_y + 11, type, Align::LEFT, TextColor::GRAY);

    card_description(card, line);
    bn::vector<TextLine, 2> lines;
    wrap_text(line, _layout.text_width, lines);

    for(int index = 0; index < lines.size(); ++index)
    {
        Label& label = _text.emplace_back();
        label.set(_layout.text_x, _layout.text_y + 24 + index * 10, lines[index], Align::LEFT, TextColor::WHITE);
    }

    for(Label& label : _text)
    {
        label.set_visible(_visible);
    }
}

}
