// Dialogue panels (#19): UTF-8 aware wrap for the 240x160 screen, letter-by-letter reveal with its voice blip, and
// every event of the game giving a readable page and a one-line preview of each option.
#include <cassert>
#include <iostream>
#include <string>
#include <string_view>
#include "engine/dialogue.h"
#include "engine/event.h"
#include "engine/run.h"

using namespace engine;

namespace {

// Stand-in for the font: 5 pixels per character, accents included (the GBA font is no wider for them).
int fixed_width(std::string_view text) { return utf8_glyph_count(text) * 5; }
// Closer to the real font (7 px for most letters): the pessimistic width for the page counts.
int wide_width(std::string_view text) { return utf8_glyph_count(text) * 7; }

std::string line_of(std::string_view text, const DialogueSpan& span) { return std::string(slice(text, span.begin, span.length)); }

void test_wrap_counts_characters_not_bytes() {
    // 8 accented characters = 16 bytes but only 40 px: they must fit a 40 px line together.
    const std::string_view text = "éèàùçêîô";
    DialogueSpan spans[DIALOGUE_MAX_LINES];
    assert(utf8_glyph_count(text) == 8 && text.size() == 16);
    assert(wrap_dialogue(text, 40, fixed_width, spans, DIALOGUE_MAX_LINES) == 1);
    assert(line_of(text, spans[0]) == text);
    std::cout << "[PASS] Wrap: accented characters are measured as one character" << std::endl;
}

void test_wrap_breaks_between_words() {
    const std::string_view text = "Salut la flamme, voici une offrande pour la route";
    DialogueSpan spans[DIALOGUE_MAX_LINES];
    int count = wrap_dialogue(text, 100, fixed_width, spans, DIALOGUE_MAX_LINES); // 20 characters per line
    assert(count == 3);
    for (int i = 0; i < count; ++i) {
        assert(fixed_width(line_of(text, spans[i])) <= 100);
        assert(line_of(text, spans[i]).front() != ' ' && line_of(text, spans[i]).back() != ' ');
    }
    assert(line_of(text, spans[0]) == "Salut la flamme,");
    assert(line_of(text, spans[2]) == "pour la route");
    std::cout << "[PASS] Wrap: lines break between words and fit the width" << std::endl;
}

void test_wrap_forced_break_and_long_word() {
    DialogueSpan spans[DIALOGUE_MAX_LINES];
    const std::string_view two = "Une\nDeux";
    assert(wrap_dialogue(two, 200, fixed_width, spans, DIALOGUE_MAX_LINES) == 2);
    assert(line_of(two, spans[1]) == "Deux");
    // A word wider than the line is cut on a character boundary, never inside an accent.
    const std::string_view word = "anticonstitutionnellement été";
    int count = wrap_dialogue(word, 50, fixed_width, spans, DIALOGUE_MAX_LINES);
    assert(count >= 3);
    std::string joined;
    for (int i = 0; i < count; ++i) {
        assert(fixed_width(line_of(word, spans[i])) <= 50);
        joined += line_of(word, spans[i]);
        if (i + 1 < count && word[spans[i].begin + spans[i].length] == ' ') joined += ' ';
    }
    assert(joined == word);
    assert(wrap_dialogue("", 50, fixed_width, spans, DIALOGUE_MAX_LINES) == 0);
    std::cout << "[PASS] Wrap: forced breaks and over-long words" << std::endl;
}

void test_wrap_stops_at_max_lines() {
    DialogueSpan spans[2];
    const std::string_view text = "un deux trois quatre cinq six sept huit neuf dix";
    assert(wrap_dialogue(text, 30, fixed_width, spans, 2) == 2);
    std::cout << "[PASS] Wrap: never writes past the line budget" << std::endl;
}

void test_reveal_types_one_character_at_a_time() {
    const std::string_view text = "Été à Rô"; // 8 characters, 12 bytes
    DialogueSpan spans[DIALOGUE_MAX_LINES];
    int count = wrap_dialogue(text, 200, fixed_width, spans, DIALOGUE_MAX_LINES);
    DialogueReveal reveal;
    reveal.start(text, spans, count, 2);
    assert(!reveal.done() && reveal.visible_bytes(0) == 0);

    assert(!reveal.update()); // first frame of the 2-frame beat
    reveal.update();
    assert(reveal.visible_line(0) == "É"); // the accent is revealed whole, 2 bytes at once
    int blips = 1;
    int frames = 2;
    while (!reveal.done()) {
        std::string_view shown = reveal.visible_line(0);
        assert(utf8_glyph_count(shown) <= 8);
        blips += reveal.update();
        ++frames;
    }
    assert(frames == 16 && reveal.visible_bytes(0) == 12);
    // 6 visible glyphs (2 spaces are silent): the voice blips on every second one.
    assert(blips >= 2 && blips <= 4);
    std::cout << "[PASS] Reveal: one character per beat, accents whole, voice skips spaces" << std::endl;
}

void test_reveal_skip_and_lines() {
    const std::string_view text = "Premiere ligne assez longue pour deborder sur la deuxieme ligne";
    DialogueSpan spans[DIALOGUE_MAX_LINES];
    int count = wrap_dialogue(text, 100, fixed_width, spans, DIALOGUE_MAX_LINES);
    assert(count >= 3);
    DialogueReveal reveal;
    reveal.start(text, spans, count, 1);
    while (reveal.current_line() == 0) reveal.update();
    assert(reveal.visible_bytes(0) == spans[0].length && reveal.visible_bytes(1) == 0);
    reveal.skip();
    assert(reveal.done());
    for (int i = 0; i < count; ++i) assert(reveal.visible_bytes(i) == spans[i].length);
    assert(!reveal.update());
    std::cout << "[PASS] Reveal: lines come one after the other and A skips to the end" << std::endl;
}

// Every event of the game: its first page fits the dialogue panel and each option previews its consequence.
void test_every_event_page_fits_the_panel() {
    int checked = 0;
    for (const EventData& data : get_all_events()) {
        RunManager run;
        run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 42);
        if (!run.start_event(data.id)) continue;
        const EventEncounter& event = run.current_event;
        DialogueSpan spans[DIALOGUE_MAX_LINES];
        assert(!event.dialogue.empty());
        // At 7 px per character a page is at most 16 lines: the dialogue box shows them 5 at a time, nothing is cut.
        assert(wrap_dialogue(event.dialogue, 224, wide_width, spans, DIALOGUE_MAX_LINES) < DIALOGUE_MAX_LINES);
        assert(event.choices.size() >= 1);
        for (const EventChoice& choice : event.choices) {
            assert(!choice.label.empty() && !choice.cost_benefit.empty());
            assert(wrap_dialogue(choice.cost_benefit, 224, wide_width, spans, DIALOGUE_MAX_LINES) <= 2);
        }
        ++checked;
    }
    assert(checked >= 50);
    std::cout << "[PASS] Events: " << checked << " pages fit the dialogue box, every option previews its consequence" << std::endl;
}

} // namespace

int main() {
    test_wrap_counts_characters_not_bytes();
    test_wrap_breaks_between_words();
    test_wrap_forced_break_and_long_word();
    test_wrap_stops_at_max_lines();
    test_reveal_types_one_character_at_a_time();
    test_reveal_skip_and_lines();
    test_every_event_page_fits_the_panel();
    return 0;
}
