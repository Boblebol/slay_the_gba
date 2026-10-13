// The Ancients of acts 2 and 3 (#801 to #807): slots, relics offered and act, as in bible/data/ancient_v0.111.0.json.
#include <cassert>
#include <initializer_list>
#include <iostream>
#include <string_view>
#include <vector>
#include "engine/ancient.h"
#include "engine/run.h"

using namespace engine;

namespace {

struct ExpectedSlot {
    uint8_t picks;
    std::vector<std::string_view> relics;
};

bool in_act(AncientId id, uint8_t act) {
    return (get_ancient(id).act_mask & (1u << (act - 1))) != 0;
}

// Slots, picks and relics offered equal the reference, in the same order.
void check_slots(AncientId id, std::string_view name, const std::vector<ExpectedSlot>& expected) {
    const Ancient& ancient = get_ancient(id);
    assert(ancient.id == id && ancient.name == name);
    assert(ancient.slots.size() == expected.size());
    for (size_t slot = 0; slot < expected.size(); ++slot) {
        assert(ancient.slots[slot].picks_allowed == expected[slot].picks);
        assert(ancient.slots[slot].options.size() == expected[slot].relics.size());
        for (size_t option = 0; option < expected[slot].relics.size(); ++option) {
            assert(ancient.slots[slot].options[option].id == expected[slot].relics[option]);
        }
    }
}

// The run meets the Ancient, takes the first relic offered, and owns it.
void check_choice(AncientId id) {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 4242);
    run.meet_ancient(id);
    assert(run.phase == RunPhase::ANCIENT_BLESSING && run.current_ancient == id);
    assert(!run.ancient_offers.empty());
    const std::string_view chosen = run.ancient_offer(0).id;
    assert(!run.has_relic(chosen));
    assert(run.choose_ancient_relic(0));
    assert(run.has_relic(chosen));
    assert(run.current_ancient == AncientId::COUNT && run.ancient_offers.empty());
}

// The act draw can bring the Ancient up in each act of its mask, and only there.
void check_drawn(AncientId id, std::initializer_list<uint8_t> acts) {
    for (uint8_t act : { 1, 2, 3 }) {
        bool listed = false;
        for (uint8_t a : acts) listed = listed || a == act;
        assert(in_act(id, act) == listed);
        bool seen = false;
        for (uint32_t seed = 1; seed <= 400; ++seed) {
            seen = seen || RunManager::draw_ancient(act, seed * 2654435761u) == id;
        }
        assert(seen == listed);
    }
}

void test_darv() {
    check_slots(AncientId::DARV, "Darv", {
        { 3, { "ASTROLABE", "BLACK_STAR", "CALLING_BELL", "EMPTY_CAGE", "PANDORAS_BOX", "RUNIC_PYRAMID", "SNECKO_EYE", "ECTOPLASM", "SOZU", "PHILOSOPHERS_STONE", "VELVET_CHOKER", "DUSTY_TOME" } },
    });
    check_drawn(AncientId::DARV, { 1, 2, 3 });
    check_choice(AncientId::DARV);
    std::cout << "[PASS] Ancient Darv: slots, relics and act as in the reference, a chosen relic joins the run" << std::endl;
}

void test_orobas() {
    check_slots(AncientId::OROBAS, "Orobas", {
        { 1, { "ELECTRIC_SHRYMP", "GLASS_EYE", "PRISMATIC_GEM", "SEA_GLASS" } },
        { 1, { "ALCHEMICAL_COFFER", "DRIFTWOOD", "RADIANT_PEARL", "SAND_CASTLE" } },
        { 1, { "TOUCH_OF_OROBAS", "ARCHAIC_TOOTH" } },
    });
    check_drawn(AncientId::OROBAS, { 2 });
    check_choice(AncientId::OROBAS);
    std::cout << "[PASS] Ancient Orobas: slots, relics and act as in the reference, a chosen relic joins the run" << std::endl;
}

void test_paël() {
    check_slots(AncientId::PAEL, "Paël", {
        { 1, { "PAELS_FLESH", "PAELS_HORN", "PAELS_TEARS" } },
        { 1, { "PAELS_WING", "PAELS_CLAW", "PAELS_TOOTH", "PAELS_GROWTH" } },
        { 1, { "PAELS_EYE", "PAELS_BLOOD", "PAELS_LEGION" } },
    });
    check_drawn(AncientId::PAEL, { 2 });
    check_choice(AncientId::PAEL);
    std::cout << "[PASS] Ancient Paël: slots, relics and act as in the reference, a chosen relic joins the run" << std::endl;
}

void test_tezcatara() {
    check_slots(AncientId::TEZCATARA, "Tezcatara", {
        { 1, { "VERY_HOT_COCOA", "YUMMY_COOKIE", "NUTRITIOUS_SOUP" } },
        { 1, { "BIIIG_HUG", "STORYBOOK", "TOASTY_MITTENS" } },
        { 1, { "GOLDEN_COMPASS", "PUMPKIN_CANDLE", "TOY_BOX", "SEAL_OF_GOLD" } },
    });
    check_drawn(AncientId::TEZCATARA, { 2 });
    check_choice(AncientId::TEZCATARA);
    std::cout << "[PASS] Ancient Tezcatara: slots, relics and act as in the reference, a chosen relic joins the run" << std::endl;
}

void test_nonupeipée() {
    check_slots(AncientId::NONUPEIPE, "Nonupeipée", {
        { 3, { "BLESSED_ANTLER", "BRILLIANT_SCARF", "DELICATE_FROND", "DIAMOND_DIADEM", "FUR_COAT", "GLITTER", "JEWELRY_BOX", "LOOMING_FRUIT", "SIGNET_RING", "BEAUTIFUL_BRACELET" } },
    });
    check_drawn(AncientId::NONUPEIPE, { 3 });
    check_choice(AncientId::NONUPEIPE);
    std::cout << "[PASS] Ancient Nonupeipée: slots, relics and act as in the reference, a chosen relic joins the run" << std::endl;
}

void test_tanx() {
    check_slots(AncientId::TANX, "Tanx", {
        { 3, { "CLAWS", "CROSSBOW", "IRON_CLUB", "MEAT_CLEAVER", "SAI", "SPIKED_GAUNTLETS", "TANXS_WHISTLE", "THROWING_AXE", "WAR_HAMMER", "TRI_BOOMERANG" } },
    });
    check_drawn(AncientId::TANX, { 3 });
    check_choice(AncientId::TANX);
    std::cout << "[PASS] Ancient Tanx: slots, relics and act as in the reference, a chosen relic joins the run" << std::endl;
}

void test_vakuu() {
    check_slots(AncientId::VAKUU, "Vakuu", {
        { 1, { "BLOOD_SOAKED_ROSE", "WHISPERING_EARRING", "FIDDLE" } },
        { 1, { "PRESERVED_FOG", "SERE_TALON", "DISTINGUISHED_CAPE" } },
        { 1, { "CHOICES_PARADOX", "MUSIC_BOX", "LORDS_PARASOL", "JEWELED_MASK" } },
    });
    check_drawn(AncientId::VAKUU, { 3 });
    check_choice(AncientId::VAKUU);
    std::cout << "[PASS] Ancient Vakuu: slots, relics and act as in the reference, a chosen relic joins the run" << std::endl;
}

} // namespace

int main() {
    test_darv();
    test_orobas();
    test_paël();
    test_tezcatara();
    test_nonupeipée();
    test_tanx();
    test_vakuu();
    return 0;
}
