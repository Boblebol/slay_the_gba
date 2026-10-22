#pragma once
// Dialogue text of the event, Ancient and shop panels (#19): UTF-8 aware word wrap for the 240x160 screen and the
// letter-by-letter reveal ("typewriter"). Header only and free of std::string/find/substr so the GBA build links.
#include <cstdint>
#include <string_view>

namespace engine {

constexpr int DIALOGUE_MAX_LINES = 16;

// One wrapped line: a slice of the source text.
struct DialogueSpan {
    uint16_t begin = 0;
    uint16_t length = 0;
};

// Number of bytes of the UTF-8 character starting with `lead` (1 for a stray continuation byte).
constexpr int utf8_char_length(unsigned char lead) {
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 1;
}

// string_view::substr throws on the GBA build, which has no exceptions: slice by hand.
constexpr std::string_view slice(std::string_view text, size_t begin, size_t length) {
    return std::string_view(text.data() + begin, length);
}

// Number of characters (not bytes) of `text`: "é" counts for one.
constexpr int utf8_glyph_count(std::string_view text) {
    int count = 0;
    for (size_t index = 0; index < text.size(); index += size_t(utf8_char_length((unsigned char)text[index]))) {
        ++count;
    }
    return count;
}

// Greedy word wrap. `width(std::string_view) -> int` measures a candidate line in pixels (accents included); a line
// never exceeds `max_width` unless a single word does, in which case it is cut at a character boundary. A '\n'
// forces a new line. Returns the number of spans written (at most `max_lines`, the rest of the text is dropped).
template <class Width>
int wrap_dialogue(std::string_view text, int max_width, Width width, DialogueSpan* out, int max_lines) {
    int lines = 0;
    size_t size = text.size();
    size_t line_start = 0; // first byte of the line being built
    size_t line_end = 0;   // end of the last word placed on it (== line_start while empty)
    size_t pos = 0;

    auto flush = [&]() {
        if (lines < max_lines) {
            out[lines].begin = uint16_t(line_start);
            out[lines].length = uint16_t(line_end - line_start);
        }
        ++lines;
    };

    while (pos < size && lines < max_lines) {
        if (text[pos] == '\n') {
            flush();
            ++pos;
            line_start = line_end = pos;
            continue;
        }
        if (text[pos] == ' ') {
            ++pos;
            continue;
        }

        size_t word_end = pos;
        while (word_end < size && text[word_end] != ' ' && text[word_end] != '\n') {
            ++word_end;
        }

        if (line_end > line_start && width(slice(text, line_start, word_end - line_start)) > max_width) {
            flush();
            line_start = line_end = pos;
            if (lines >= max_lines) break;
        } else if (line_end == line_start) {
            line_start = pos;
        }

        // A word wider than the whole line is cut at a character boundary.
        while (width(slice(text, line_start, word_end - line_start)) > max_width) {
            size_t cut = line_start;
            size_t next = cut;
            while (next < word_end) {
                size_t after = next + size_t(utf8_char_length((unsigned char)text[next]));
                if (after > word_end) after = word_end;
                if (width(slice(text, line_start, after - line_start)) > max_width) break;
                next = after;
            }
            if (next == cut) {
                next = cut + size_t(utf8_char_length((unsigned char)text[cut])); // at least one character
                if (next > word_end) next = word_end;
            }
            line_end = next;
            flush();
            line_start = line_end = next;
            if (lines >= max_lines || next >= word_end) break;
        }

        if (lines >= max_lines) break;
        line_end = word_end;
        pos = word_end;
    }

    if (line_end > line_start && lines < max_lines) {
        flush();
    }
    return lines < max_lines ? lines : max_lines;
}

// Letter-by-letter reveal of wrapped lines. update() is called once per frame.
class DialogueReveal {
public:
    // `text` must outlive the reveal (the spans index into it).
    void start(std::string_view text, const DialogueSpan* spans, int count, int frames_per_glyph = 2) {
        _text = text;
        _count = count < DIALOGUE_MAX_LINES ? count : DIALOGUE_MAX_LINES;
        for (int index = 0; index < _count; ++index) {
            _spans[index] = spans[index];
        }
        _frames_per_glyph = frames_per_glyph < 1 ? 1 : frames_per_glyph;
        _line = 0;
        _bytes = 0;
        _tick = 0;
        _blips = 0;
    }

    bool done() const { return _line >= _count; }
    int line_count() const { return _count; }
    // The line being typed (== line_count() once done).
    int current_line() const { return _line; }
    // Bytes of line `index` that are on screen: all of it for finished lines, none for the lines still to come.
    int visible_bytes(int index) const {
        if (index < _line) return _spans[index].length;
        return index == _line ? _bytes : 0;
    }
    std::string_view line(int index) const { return slice(_text, _spans[index].begin, _spans[index].length); }
    std::string_view visible_line(int index) const { return slice(_text, _spans[index].begin, size_t(visible_bytes(index))); }

    // Shows everything at once (A pressed while the text is still typing).
    void skip() {
        _line = _count;
        _bytes = 0;
    }

    // Advances one frame. Returns true when the voice should blip: every second visible (non-space) glyph, so the
    // text gets its "ambient" sound without a machine-gun of beeps.
    bool update() {
        if (done() || ++_tick < _frames_per_glyph) return false;
        _tick = 0;

        std::string_view full = line(_line);
        if (_bytes < int(full.size())) {
            _bytes += utf8_char_length((unsigned char)full[size_t(_bytes)]);
            if (_bytes > int(full.size())) _bytes = int(full.size());
        }
        bool visible = _bytes > 0 && full[size_t(_bytes - 1)] != ' ';

        if (_bytes >= int(full.size())) {
            ++_line;
            _bytes = 0;
        }
        if (!visible) return false;
        return (++_blips & 1) == 1;
    }

private:
    std::string_view _text;
    DialogueSpan _spans[DIALOGUE_MAX_LINES];
    int _count = 0;
    int _line = 0;
    int _bytes = 0;
    int _tick = 0;
    int _frames_per_glyph = 2;
    int _blips = 0;
};

} // namespace engine
