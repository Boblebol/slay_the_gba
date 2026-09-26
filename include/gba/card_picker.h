#pragma once

#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "engine/card.h"
#include "gba/text.h"

namespace gba
{

struct CardPickerLayout
{
    int strip_y;        // centre line of the mini card strip
    int preview_x;      // big card position
    int preview_y;
    int text_x;         // left edge of the name / description block
    int text_y;
    int text_width;
};

/**
 * Horizontal strip of mini cards with a big illustrated preview and its rules text.
 * Only the visible window of cards owns sprites, so a 50 card deck stays within VRAM budget.
 */
class CardPicker
{
public:
    explicit CardPicker(const CardPickerLayout& layout);

    void clear();
    void add(const engine::DeckCard& card, int tag);
    void set_preview_upgraded(bool upgraded);   // show the "+" version (campfire forge)
    void set_selected(int index);
    void set_visible(bool visible);

    // Handles LEFT/RIGHT and animates. Returns true when the selection changed.
    bool update(int frame);

    [[nodiscard]] int selected() const
    {
        return _selected;
    }

    [[nodiscard]] int size() const
    {
        return _cards.size();
    }

    [[nodiscard]] int selected_tag() const
    {
        return _cards.empty() ? -1 : _cards[_selected].tag;
    }

    [[nodiscard]] engine::DeckCard selected_card() const
    {
        return _cards.empty() ? engine::DeckCard() : _cards[_selected].card;
    }

    // Big preview sprite, e.g. to animate it when the card is chosen.
    [[nodiscard]] bn::optional<bn::sprite_ptr>& preview()
    {
        return _preview;
    }

private:
    static constexpr int VISIBLE = 7;
    static constexpr int SPACING = 30;

    struct Entry
    {
        engine::DeckCard card;
        int tag;
    };

    struct Slot
    {
        bn::sprite_ptr sprite;
        int index;
        bn::fixed y;
    };

    CardPickerLayout _layout;
    bn::vector<Entry, 50> _cards;
    bn::vector<Slot, VISIBLE> _slots;
    bn::optional<bn::sprite_ptr> _preview;
    bn::vector<Label, 4> _text;
    Label _counter;
    int _selected = 0;
    int _first = 0;
    int _preview_age = 0;
    bool _upgraded_preview = false;
    bool _visible = true;
    bool _dirty = true;

    void _rebuild_slots();
    void _refresh_preview();
};

}
