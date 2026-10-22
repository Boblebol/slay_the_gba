// Title screen of each edition (#3): the edition name, the colour theme and the version tag.
#include <cassert>
#include <iostream>
#include <string_view>
#include "engine/title_edition.h"

using namespace engine;
using namespace engine::title_edition;

constexpr CharacterId ALL[] = { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT,
                                CharacterId::REGENT, CharacterId::NECROBINDER };

// Every edition has its own name, tint and strength inside the readable range.
void test_each_edition_has_its_own_identity() {
    for (CharacterId a : ALL) {
        assert(!theme(a).edition.empty());
        assert(theme(a).tint_strength > 0 && theme(a).tint_strength <= 16);
        for (CharacterId b : ALL) {
            if (a != b) {
                assert(theme(a).edition != theme(b).edition);
                assert(theme(a).tint_bgr555 != theme(b).tint_bgr555);
            }
        }
    }
}

// The theme follows the heroes' colours (tint): red for Ironclad, green for Silent, blue for Defect, violet / gold for the
// Regent and the Necrobinder.
void test_theme_colours() {
    auto channel = [](uint16_t c, int shift) { return (c >> shift) & 31; };
    uint16_t iron = theme(CharacterId::IRONCLAD).tint_bgr555;
    uint16_t silent = theme(CharacterId::SILENT).tint_bgr555;
    uint16_t defect = theme(CharacterId::DEFECT).tint_bgr555;
    assert(channel(iron, 0) > channel(iron, 5) && channel(iron, 0) > channel(iron, 10));      // red dominates
    assert(channel(silent, 5) > channel(silent, 0) && channel(silent, 5) > channel(silent, 10)); // green dominates
    assert(channel(defect, 10) > channel(defect, 0) && channel(defect, 10) > channel(defect, 5)); // blue dominates
    assert(theme(CharacterId::IRONCLAD).accent == Accent::GOLD);
    assert(theme(CharacterId::SILENT).accent == Accent::GREEN);
    assert(theme(CharacterId::DEFECT).accent == Accent::BLUE);
    assert(theme(CharacterId::REGENT).accent == Accent::GOLD);
    assert(theme(CharacterId::NECROBINDER).accent == Accent::PURPLE);
}

// The edition names are the ones of the website and the booklets (scripts/editions.py).
void test_edition_names() {
    assert(theme(CharacterId::IRONCLAD).edition == "Édition Soldat de Fer");
    assert(theme(CharacterId::SILENT).edition == "Édition La Silencieuse");
    assert(theme(CharacterId::DEFECT).edition == "Édition Le Défectueux");
    assert(theme(CharacterId::REGENT).edition == "Édition Le Régent");
    assert(theme(CharacterId::NECROBINDER).edition == "Édition La Nécrolieuse");
}

std::string_view version_tag(std::string_view version, char* buffer, int capacity) {
    return std::string_view(buffer, format_version(version, buffer, capacity));
}

// "0.2.0" becomes "v0.2.0"; a tag already starting with v is kept; no version shows "dev"; a short buffer truncates.
void test_version_tag() {
    char buffer[16];
    assert(version_tag("0.2.0", buffer, 16) == "v0.2.0");
    assert(version_tag("0.98.6", buffer, 16) == "v0.98.6");
    assert(version_tag("v1.0.0", buffer, 16) == "v1.0.0");
    assert(version_tag("", buffer, 16) == "dev");
    assert(version_tag("12.345.678", buffer, 6) == "v12.34");
    assert(format_version("1.0.0", buffer, 0) == 0);
}

int main() {
    test_each_edition_has_its_own_identity();
    test_theme_colours();
    test_edition_names();
    test_version_tag();
    std::cout << "title_edition_test: OK\n";
    return 0;
}
