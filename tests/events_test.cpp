// Events described by data (#253): the interpreter of the effects, the pages, the conditions, the draw of an
// act, then the test of each event (tests/events/<id>_test.h, picked up below with __has_include: adding an
// event never touches this file).
#include <cassert>
#include <iostream>
#include <set>
#include <string_view>
#include "engine/card_meta.h"
#include "engine/event.h"
#include "engine/event_manager.h"
#include "engine/run.h"
#include "engine/save_system.h"
#include "events/event_test_support.h"

// @event-tests-begin (one block per slot of src/engine/event_registry.cpp)
struct EventTest {
    std::string_view id;
    void (*run)();
};

#if __has_include("events/abyssal_baths_test.h")
#include "events/abyssal_baths_test.h"
#endif
#if __has_include("events/amalgamator_test.h")
#include "events/amalgamator_test.h"
#endif
#if __has_include("events/aroma_of_chaos_test.h")
#include "events/aroma_of_chaos_test.h"
#endif
#if __has_include("events/battleworn_dummy_test.h")
#include "events/battleworn_dummy_test.h"
#endif
#if __has_include("events/brain_leech_test.h")
#include "events/brain_leech_test.h"
#endif
#if __has_include("events/bugslayer_test.h")
#include "events/bugslayer_test.h"
#endif
#if __has_include("events/byrdonis_nest_test.h")
#include "events/byrdonis_nest_test.h"
#endif
#if __has_include("events/colorful_philosophers_test.h")
#include "events/colorful_philosophers_test.h"
#endif
#if __has_include("events/colossal_flower_test.h")
#include "events/colossal_flower_test.h"
#endif
#if __has_include("events/crystal_sphere_test.h")
#include "events/crystal_sphere_test.h"
#endif
#if __has_include("events/dense_vegetation_test.h")
#include "events/dense_vegetation_test.h"
#endif
#if __has_include("events/doll_room_test.h")
#include "events/doll_room_test.h"
#endif
#if __has_include("events/doors_of_light_and_dark_test.h")
#include "events/doors_of_light_and_dark_test.h"
#endif
#if __has_include("events/drowning_beacon_test.h")
#include "events/drowning_beacon_test.h"
#endif
#if __has_include("events/endless_conveyor_test.h")
#include "events/endless_conveyor_test.h"
#endif
#if __has_include("events/fake_merchant_test.h")
#include "events/fake_merchant_test.h"
#endif
#if __has_include("events/field_of_man_sized_holes_test.h")
#include "events/field_of_man_sized_holes_test.h"
#endif
#if __has_include("events/grave_of_the_forgotten_test.h")
#include "events/grave_of_the_forgotten_test.h"
#endif
#if __has_include("events/hungry_for_mushrooms_test.h")
#include "events/hungry_for_mushrooms_test.h"
#endif
#if __has_include("events/infested_automaton_test.h")
#include "events/infested_automaton_test.h"
#endif
#if __has_include("events/jungle_maze_adventure_test.h")
#include "events/jungle_maze_adventure_test.h"
#endif
#if __has_include("events/lost_wisp_test.h")
#include "events/lost_wisp_test.h"
#endif
#if __has_include("events/luminous_choir_test.h")
#include "events/luminous_choir_test.h"
#endif
#if __has_include("events/morphic_grove_test.h")
#include "events/morphic_grove_test.h"
#endif
#if __has_include("events/potion_courier_test.h")
#include "events/potion_courier_test.h"
#endif
#if __has_include("events/punch_off_test.h")
#include "events/punch_off_test.h"
#endif
#if __has_include("events/ranwid_the_elder_test.h")
#include "events/ranwid_the_elder_test.h"
#endif
#if __has_include("events/reflections_test.h")
#include "events/reflections_test.h"
#endif
#if __has_include("events/relic_trader_test.h")
#include "events/relic_trader_test.h"
#endif
#if __has_include("events/room_full_of_cheese_test.h")
#include "events/room_full_of_cheese_test.h"
#endif
#if __has_include("events/round_tea_party_test.h")
#include "events/round_tea_party_test.h"
#endif
#if __has_include("events/sapphire_seed_test.h")
#include "events/sapphire_seed_test.h"
#endif
#if __has_include("events/self_help_book_test.h")
#include "events/self_help_book_test.h"
#endif
#if __has_include("events/slippery_bridge_test.h")
#include "events/slippery_bridge_test.h"
#endif
#if __has_include("events/spiraling_whirlpool_test.h")
#include "events/spiraling_whirlpool_test.h"
#endif
#if __has_include("events/spirit_grafter_test.h")
#include "events/spirit_grafter_test.h"
#endif
#if __has_include("events/stone_of_all_time_test.h")
#include "events/stone_of_all_time_test.h"
#endif
#if __has_include("events/sunken_statue_test.h")
#include "events/sunken_statue_test.h"
#endif
#if __has_include("events/sunken_treasury_test.h")
#include "events/sunken_treasury_test.h"
#endif
#if __has_include("events/symbiote_test.h")
#include "events/symbiote_test.h"
#endif
#if __has_include("events/tablet_of_truth_test.h")
#include "events/tablet_of_truth_test.h"
#endif
#if __has_include("events/tea_master_test.h")
#include "events/tea_master_test.h"
#endif
#if __has_include("events/the_future_of_potions_test.h")
#include "events/the_future_of_potions_test.h"
#endif
#if __has_include("events/the_lantern_key_test.h")
#include "events/the_lantern_key_test.h"
#endif
#if __has_include("events/the_legends_were_true_test.h")
#include "events/the_legends_were_true_test.h"
#endif
#if __has_include("events/this_or_that_test.h")
#include "events/this_or_that_test.h"
#endif
#if __has_include("events/tinker_time_test.h")
#include "events/tinker_time_test.h"
#endif
#if __has_include("events/trash_heap_test.h")
#include "events/trash_heap_test.h"
#endif
#if __has_include("events/trial_test.h")
#include "events/trial_test.h"
#endif
#if __has_include("events/unrest_site_test.h")
#include "events/unrest_site_test.h"
#endif
#if __has_include("events/war_historian_repy_test.h")
#include "events/war_historian_repy_test.h"
#endif
#if __has_include("events/waterlogged_scriptorium_test.h")
#include "events/waterlogged_scriptorium_test.h"
#endif
#if __has_include("events/welcome_to_wongos_test.h")
#include "events/welcome_to_wongos_test.h"
#endif
#if __has_include("events/wellspring_test.h")
#include "events/wellspring_test.h"
#endif
#if __has_include("events/whispering_hollow_test.h")
#include "events/whispering_hollow_test.h"
#endif
#if __has_include("events/wood_carvings_test.h")
#include "events/wood_carvings_test.h"
#endif
#if __has_include("events/zen_weaver_test.h")
#include "events/zen_weaver_test.h"
#endif
#if __has_include("events/mystic_altar_test.h")
#include "events/mystic_altar_test.h"
#endif

constexpr EventTest EVENT_TESTS[] = {
#if __has_include("events/abyssal_baths_test.h")
    { "ABYSSAL_BATHS", &test_abyssal_baths },
#else
    { "ABYSSAL_BATHS", nullptr },
#endif
#if __has_include("events/amalgamator_test.h")
    { "AMALGAMATOR", &test_amalgamator },
#else
    { "AMALGAMATOR", nullptr },
#endif
#if __has_include("events/aroma_of_chaos_test.h")
    { "AROMA_OF_CHAOS", &test_aroma_of_chaos },
#else
    { "AROMA_OF_CHAOS", nullptr },
#endif
#if __has_include("events/battleworn_dummy_test.h")
    { "BATTLEWORN_DUMMY", &test_battleworn_dummy },
#else
    { "BATTLEWORN_DUMMY", nullptr },
#endif
#if __has_include("events/brain_leech_test.h")
    { "BRAIN_LEECH", &test_brain_leech },
#else
    { "BRAIN_LEECH", nullptr },
#endif
#if __has_include("events/bugslayer_test.h")
    { "BUGSLAYER", &test_bugslayer },
#else
    { "BUGSLAYER", nullptr },
#endif
#if __has_include("events/byrdonis_nest_test.h")
    { "BYRDONIS_NEST", &test_byrdonis_nest },
#else
    { "BYRDONIS_NEST", nullptr },
#endif
#if __has_include("events/colorful_philosophers_test.h")
    { "COLORFUL_PHILOSOPHERS", &test_colorful_philosophers },
#else
    { "COLORFUL_PHILOSOPHERS", nullptr },
#endif
#if __has_include("events/colossal_flower_test.h")
    { "COLOSSAL_FLOWER", &test_colossal_flower },
#else
    { "COLOSSAL_FLOWER", nullptr },
#endif
#if __has_include("events/crystal_sphere_test.h")
    { "CRYSTAL_SPHERE", &test_crystal_sphere },
#else
    { "CRYSTAL_SPHERE", nullptr },
#endif
#if __has_include("events/dense_vegetation_test.h")
    { "DENSE_VEGETATION", &test_dense_vegetation },
#else
    { "DENSE_VEGETATION", nullptr },
#endif
#if __has_include("events/doll_room_test.h")
    { "DOLL_ROOM", &test_doll_room },
#else
    { "DOLL_ROOM", nullptr },
#endif
#if __has_include("events/doors_of_light_and_dark_test.h")
    { "DOORS_OF_LIGHT_AND_DARK", &test_doors_of_light_and_dark },
#else
    { "DOORS_OF_LIGHT_AND_DARK", nullptr },
#endif
#if __has_include("events/drowning_beacon_test.h")
    { "DROWNING_BEACON", &test_drowning_beacon },
#else
    { "DROWNING_BEACON", nullptr },
#endif
#if __has_include("events/endless_conveyor_test.h")
    { "ENDLESS_CONVEYOR", &test_endless_conveyor },
#else
    { "ENDLESS_CONVEYOR", nullptr },
#endif
#if __has_include("events/fake_merchant_test.h")
    { "FAKE_MERCHANT", &test_fake_merchant },
#else
    { "FAKE_MERCHANT", nullptr },
#endif
#if __has_include("events/field_of_man_sized_holes_test.h")
    { "FIELD_OF_MAN_SIZED_HOLES", &test_field_of_man_sized_holes },
#else
    { "FIELD_OF_MAN_SIZED_HOLES", nullptr },
#endif
#if __has_include("events/grave_of_the_forgotten_test.h")
    { "GRAVE_OF_THE_FORGOTTEN", &test_grave_of_the_forgotten },
#else
    { "GRAVE_OF_THE_FORGOTTEN", nullptr },
#endif
#if __has_include("events/hungry_for_mushrooms_test.h")
    { "HUNGRY_FOR_MUSHROOMS", &test_hungry_for_mushrooms },
#else
    { "HUNGRY_FOR_MUSHROOMS", nullptr },
#endif
#if __has_include("events/infested_automaton_test.h")
    { "INFESTED_AUTOMATON", &test_infested_automaton },
#else
    { "INFESTED_AUTOMATON", nullptr },
#endif
#if __has_include("events/jungle_maze_adventure_test.h")
    { "JUNGLE_MAZE_ADVENTURE", &test_jungle_maze_adventure },
#else
    { "JUNGLE_MAZE_ADVENTURE", nullptr },
#endif
#if __has_include("events/lost_wisp_test.h")
    { "LOST_WISP", &test_lost_wisp },
#else
    { "LOST_WISP", nullptr },
#endif
#if __has_include("events/luminous_choir_test.h")
    { "LUMINOUS_CHOIR", &test_luminous_choir },
#else
    { "LUMINOUS_CHOIR", nullptr },
#endif
#if __has_include("events/morphic_grove_test.h")
    { "MORPHIC_GROVE", &test_morphic_grove },
#else
    { "MORPHIC_GROVE", nullptr },
#endif
#if __has_include("events/potion_courier_test.h")
    { "POTION_COURIER", &test_potion_courier },
#else
    { "POTION_COURIER", nullptr },
#endif
#if __has_include("events/punch_off_test.h")
    { "PUNCH_OFF", &test_punch_off },
#else
    { "PUNCH_OFF", nullptr },
#endif
#if __has_include("events/ranwid_the_elder_test.h")
    { "RANWID_THE_ELDER", &test_ranwid_the_elder },
#else
    { "RANWID_THE_ELDER", nullptr },
#endif
#if __has_include("events/reflections_test.h")
    { "REFLECTIONS", &test_reflections },
#else
    { "REFLECTIONS", nullptr },
#endif
#if __has_include("events/relic_trader_test.h")
    { "RELIC_TRADER", &test_relic_trader },
#else
    { "RELIC_TRADER", nullptr },
#endif
#if __has_include("events/room_full_of_cheese_test.h")
    { "ROOM_FULL_OF_CHEESE", &test_room_full_of_cheese },
#else
    { "ROOM_FULL_OF_CHEESE", nullptr },
#endif
#if __has_include("events/round_tea_party_test.h")
    { "ROUND_TEA_PARTY", &test_round_tea_party },
#else
    { "ROUND_TEA_PARTY", nullptr },
#endif
#if __has_include("events/sapphire_seed_test.h")
    { "SAPPHIRE_SEED", &test_sapphire_seed },
#else
    { "SAPPHIRE_SEED", nullptr },
#endif
#if __has_include("events/self_help_book_test.h")
    { "SELF_HELP_BOOK", &test_self_help_book },
#else
    { "SELF_HELP_BOOK", nullptr },
#endif
#if __has_include("events/slippery_bridge_test.h")
    { "SLIPPERY_BRIDGE", &test_slippery_bridge },
#else
    { "SLIPPERY_BRIDGE", nullptr },
#endif
#if __has_include("events/spiraling_whirlpool_test.h")
    { "SPIRALING_WHIRLPOOL", &test_spiraling_whirlpool },
#else
    { "SPIRALING_WHIRLPOOL", nullptr },
#endif
#if __has_include("events/spirit_grafter_test.h")
    { "SPIRIT_GRAFTER", &test_spirit_grafter },
#else
    { "SPIRIT_GRAFTER", nullptr },
#endif
#if __has_include("events/stone_of_all_time_test.h")
    { "STONE_OF_ALL_TIME", &test_stone_of_all_time },
#else
    { "STONE_OF_ALL_TIME", nullptr },
#endif
#if __has_include("events/sunken_statue_test.h")
    { "SUNKEN_STATUE", &test_sunken_statue },
#else
    { "SUNKEN_STATUE", nullptr },
#endif
#if __has_include("events/sunken_treasury_test.h")
    { "SUNKEN_TREASURY", &test_sunken_treasury },
#else
    { "SUNKEN_TREASURY", nullptr },
#endif
#if __has_include("events/symbiote_test.h")
    { "SYMBIOTE", &test_symbiote },
#else
    { "SYMBIOTE", nullptr },
#endif
#if __has_include("events/tablet_of_truth_test.h")
    { "TABLET_OF_TRUTH", &test_tablet_of_truth },
#else
    { "TABLET_OF_TRUTH", nullptr },
#endif
#if __has_include("events/tea_master_test.h")
    { "TEA_MASTER", &test_tea_master },
#else
    { "TEA_MASTER", nullptr },
#endif
#if __has_include("events/the_future_of_potions_test.h")
    { "THE_FUTURE_OF_POTIONS", &test_the_future_of_potions },
#else
    { "THE_FUTURE_OF_POTIONS", nullptr },
#endif
#if __has_include("events/the_lantern_key_test.h")
    { "THE_LANTERN_KEY", &test_the_lantern_key },
#else
    { "THE_LANTERN_KEY", nullptr },
#endif
#if __has_include("events/the_legends_were_true_test.h")
    { "THE_LEGENDS_WERE_TRUE", &test_the_legends_were_true },
#else
    { "THE_LEGENDS_WERE_TRUE", nullptr },
#endif
#if __has_include("events/this_or_that_test.h")
    { "THIS_OR_THAT", &test_this_or_that },
#else
    { "THIS_OR_THAT", nullptr },
#endif
#if __has_include("events/tinker_time_test.h")
    { "TINKER_TIME", &test_tinker_time },
#else
    { "TINKER_TIME", nullptr },
#endif
#if __has_include("events/trash_heap_test.h")
    { "TRASH_HEAP", &test_trash_heap },
#else
    { "TRASH_HEAP", nullptr },
#endif
#if __has_include("events/trial_test.h")
    { "TRIAL", &test_trial },
#else
    { "TRIAL", nullptr },
#endif
#if __has_include("events/unrest_site_test.h")
    { "UNREST_SITE", &test_unrest_site },
#else
    { "UNREST_SITE", nullptr },
#endif
#if __has_include("events/war_historian_repy_test.h")
    { "WAR_HISTORIAN_REPY", &test_war_historian_repy },
#else
    { "WAR_HISTORIAN_REPY", nullptr },
#endif
#if __has_include("events/waterlogged_scriptorium_test.h")
    { "WATERLOGGED_SCRIPTORIUM", &test_waterlogged_scriptorium },
#else
    { "WATERLOGGED_SCRIPTORIUM", nullptr },
#endif
#if __has_include("events/welcome_to_wongos_test.h")
    { "WELCOME_TO_WONGOS", &test_welcome_to_wongos },
#else
    { "WELCOME_TO_WONGOS", nullptr },
#endif
#if __has_include("events/wellspring_test.h")
    { "WELLSPRING", &test_wellspring },
#else
    { "WELLSPRING", nullptr },
#endif
#if __has_include("events/whispering_hollow_test.h")
    { "WHISPERING_HOLLOW", &test_whispering_hollow },
#else
    { "WHISPERING_HOLLOW", nullptr },
#endif
#if __has_include("events/wood_carvings_test.h")
    { "WOOD_CARVINGS", &test_wood_carvings },
#else
    { "WOOD_CARVINGS", nullptr },
#endif
#if __has_include("events/zen_weaver_test.h")
    { "ZEN_WEAVER", &test_zen_weaver },
#else
    { "ZEN_WEAVER", nullptr },
#endif
#if __has_include("events/mystic_altar_test.h")
    { "MYSTIC_ALTAR", &test_mystic_altar },
#else
    { "MYSTIC_ALTAR", nullptr },
#endif
};
// @event-tests-end

using namespace engine;
using event_test::EventRun;

namespace {

RunManager fresh_run(uint32_t seed = 42) {
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, seed);
    run.hp = 40;
    run.max_hp = 80;
    run.gold = 100;
    return run;
}

// Opens `def` (an event of this test only) on its first page.
void open(RunManager& run, const EventDef& def) {
    run.current_event = {};
    EventManager::open_page(run, def, 0);
    run.phase = RunPhase::EVENT;
}

int count_cards(const RunManager& run, CardId id) {
    int count = 0;
    for (const DeckCard& card : run.master_deck) count += card.id == id;
    return count;
}

// ── An event of three pages, for the tests of the pages and of the chained effects ──
constexpr EventEffect PAGES_FORWARD[] = {
    { .type = EventEffectType::GAIN_GOLD, .amount = 10 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 1 },
};
constexpr EventEffect PAGES_SKIP[] = {
    { .type = EventEffectType::LOSE_HP, .amount = 5 },
    { .type = EventEffectType::GOTO_PAGE, .amount = 2 },
};
constexpr EventEffect PAGES_CHAIN[] = {
    { .type = EventEffectType::LOSE_GOLD, .amount = 50 },
    { .type = EventEffectType::GAIN_MAX_HP, .amount = 6 },
    { .type = EventEffectType::HEAL, .amount = 100 },
};
constexpr EventOption PAGES_FIRST[] = {
    { .label = "Avancer", .cost_benefit = "+10 Or", .result = "Vous avancez.", .effects = PAGES_FORWARD },
    { .label = "Sauter", .cost_benefit = "-5 PV", .result = "Vous sautez.", .effects = PAGES_SKIP },
    { .label = "Partir", .cost_benefit = "", .result = "Vous partez." },
};
constexpr EventOption PAGES_SECOND[] = {
    { .label = "Payer", .cost_benefit = "-50 Or, +6 PV Max, soin", .result = "Payé.", .effects = PAGES_CHAIN,
      .condition = { .check = EventCheck::MIN_GOLD, .amount = 50 } },
    { .label = "Partir", .cost_benefit = "", .result = "Vous partez.",
      .effects = std::span<const EventEffect>{} },
};
constexpr EventOption PAGES_THIRD[] = {
    { .label = "Fin", .cost_benefit = "", .result = "Fin." },
};
constexpr EventPage PAGES[] = {
    { .text = "Page 1", .options = PAGES_FIRST },
    { .text = "Page 2", .options = PAGES_SECOND },
    { .text = "Page 3", .options = PAGES_THIRD },
};
constexpr EventDef PAGES_EVENT = { .id = "TEST_PAGES", .title = "Pages", .pages = PAGES };

// ── An event whose options start a fight or open a reward screen ──
constexpr EventEffect FIGHT_NORMAL[] = { { .type = EventEffectType::FIGHT } };
constexpr EventEffect FIGHT_ELITE[] = { { .type = EventEffectType::FIGHT, .ref = "ELITE" } };
constexpr EventEffect CARD_OFFER[] = { { .type = EventEffectType::CARD_REWARD } };
constexpr EventEffect RELIC_OFFER[] = { { .type = EventEffectType::RELIC_REWARD } };
constexpr EventOption OUTCOME_OPTIONS[] = {
    { .label = "Combat", .cost_benefit = "", .result = "Combat !", .effects = FIGHT_NORMAL },
    { .label = "Élite", .cost_benefit = "", .result = "Élite !", .effects = FIGHT_ELITE },
    { .label = "Carte", .cost_benefit = "", .result = "Carte.", .effects = CARD_OFFER },
    { .label = "Relique", .cost_benefit = "", .result = "Relique.", .effects = RELIC_OFFER },
};
constexpr EventPage OUTCOME_PAGES[] = { { .text = "Issues", .options = OUTCOME_OPTIONS } };
constexpr EventDef OUTCOME_EVENT = { .id = "TEST_OUTCOMES", .title = "Issues", .pages = OUTCOME_PAGES };

} // namespace

// Every slot holds a known event, once; every event file is consistent and has its test.
void test_event_registry() {
    std::span<const EventSlot> slots = EventManager::slots();
    assert(slots.size() == get_all_events().size() + 1); // the 57 events of the reference, and MYSTIC_ALTAR
    std::set<std::string_view> ids;
    for (size_t i = 0; i < slots.size(); ++i) {
        const EventSlot& slot = slots[i];
        assert(ids.insert(slot.id).second);
        assert(slot.id == "MYSTIC_ALTAR" || find_event(slot.id) != nullptr);
        assert(EventManager::slot_index(slot.id) == static_cast<int>(i));
        if (!slot.def) continue;
        const EventDef& def = *slot.def;
        assert(def.id == slot.id && !def.title.empty() && !def.pages.empty() && (def.acts & EVENT_ALL_ACTS) != 0);
        for (const EventPage& page : def.pages) {
            assert(!page.text.empty() && !page.options.empty() && page.options.size() <= MAX_EVENT_OPTIONS);
            for (const EventOption& option : page.options) {
                assert(!option.label.empty() && !option.result.empty());
                for (const EventEffect& effect : option.effects) {
                    if (effect.type == EventEffectType::GOTO_PAGE) {
                        assert(effect.amount >= 0 && static_cast<size_t>(effect.amount) < def.pages.size());
                    }
                }
            }
        }
        assert(EventManager::find(slot.id) == &def);
    }
    assert(EventManager::find("HUNGRY_FOR_MUSHROOMS") && EventManager::find("NOT_AN_EVENT") == nullptr);
    assert(EventManager::slot_index("NOT_AN_EVENT") == -1);
    std::cout << "[PASS] Event registry: one slot per event, every event file consistent" << std::endl;
}

// Effects: gold, HP, Max HP, cards added / removed / upgraded / transformed / enchanted, curses, relics, potions.
void test_event_effects() {
    {
        RunManager run = fresh_run();
        EventManager::apply_effect(run, { .type = EventEffectType::GAIN_GOLD, .amount = 25 });
        EventManager::apply_effect(run, { .type = EventEffectType::LOSE_GOLD, .amount = 40 });
        assert(run.gold == 85);
        EventManager::apply_effect(run, { .type = EventEffectType::LOSE_GOLD, .amount = 999 });
        assert(run.gold == 0);
        EventManager::apply_effect(run, { .type = EventEffectType::HEAL, .amount = 15 });
        assert(run.hp == 55);
        EventManager::apply_effect(run, { .type = EventEffectType::LOSE_HP, .amount = 10 });
        assert(run.hp == 45);
        EventManager::apply_effect(run, { .type = EventEffectType::GAIN_MAX_HP, .amount = 5 });
        assert(run.max_hp == 85 && run.hp == 50);
        EventManager::apply_effect(run, { .type = EventEffectType::LOSE_MAX_HP, .amount = 10 });
        assert(run.max_hp == 75 && run.hp <= run.max_hp);
    }
    // Cards added, and curses (a given one, or one drawn among the curses).
    {
        RunManager run = fresh_run();
        const size_t deck = run.master_deck.size();
        EventManager::apply_effect(run, { .type = EventEffectType::ADD_CARD, .amount = 2, .card = CardId::BASH, .upgraded = true });
        assert(run.master_deck.size() == deck + 2 && run.master_deck[deck].id == CardId::BASH && run.master_deck[deck].upgraded);
        EventManager::apply_effect(run, { .type = EventEffectType::ADD_CURSE, .card = CardId::INJURY });
        assert(run.master_deck.back().id == CardId::INJURY);
        EventManager::apply_effect(run, { .type = EventEffectType::ADD_CURSE, .amount = 2 });
        assert(run.master_deck.size() == deck + 5);
        assert(card_meta(run.master_deck[deck + 3].id).pool == CardPool::CURSE);
        assert(card_meta(run.master_deck[deck + 4].id).pool == CardPool::CURSE);
    }
    // Chosen by the player: a deck choice of the right kind and size.
    {
        RunManager run = fresh_run();
        EventManager::apply_effect(run, { .type = EventEffectType::REMOVE_CARD, .amount = 2 });
        assert(run.deck_choice.kind == DeckChoiceKind::REMOVE && run.deck_choice.remaining == 2);
        const size_t deck = run.master_deck.size();
        assert(run.resolve_deck_choice(0) && run.resolve_deck_choice(0) && !run.has_deck_choice());
        assert(run.master_deck.size() == deck - 2);

        EventManager::apply_effect(run, { .type = EventEffectType::TRANSFORM_CARD, .upgraded = true });
        assert(run.deck_choice.kind == DeckChoiceKind::TRANSFORM && run.deck_choice.remaining == 1 && run.deck_choice.upgrade_result);
        const CardId before = run.master_deck[0].id;
        assert(run.resolve_deck_choice(0) && run.master_deck[0].id != before && run.master_deck[0].upgraded);

        EventManager::apply_effect(run, { .type = EventEffectType::ENCHANT_CARD, .enchantment = Enchantment::SHARP, .enchant_amount = 3 });
        assert(run.deck_choice.kind == DeckChoiceKind::ENCHANT && run.deck_choice.enchantment == Enchantment::SHARP);
        int attack = -1;
        for (size_t i = 0; i < run.master_deck.size() && attack < 0; ++i) {
            if (run.deck_choice_allows(static_cast<int>(i))) attack = static_cast<int>(i);
        }
        assert(attack >= 0 && run.resolve_deck_choice(attack));
        assert(run.master_deck[attack].enchantment == Enchantment::SHARP && run.master_deck[attack].enchant_amount == 3);
    }
    // Drawn at random: done at once, no choice asked.
    {
        RunManager run = fresh_run();
        const size_t deck = run.master_deck.size();
        EventManager::apply_effect(run, { .type = EventEffectType::REMOVE_CARD, .amount = 2, .pick = EventPick::RANDOM });
        assert(run.master_deck.size() == deck - 2 && !run.has_deck_choice());
        EventManager::apply_effect(run, { .type = EventEffectType::UPGRADE_CARD, .amount = 3, .pick = EventPick::RANDOM });
        int upgraded = 0;
        for (const DeckCard& card : run.master_deck) upgraded += card.upgraded;
        assert(upgraded == 3 && !run.has_deck_choice());
        const int strikes = count_cards(run, CardId::STRIKE) + count_cards(run, CardId::DEFEND) + count_cards(run, CardId::BASH);
        EventManager::apply_effect(run, { .type = EventEffectType::TRANSFORM_CARD, .pick = EventPick::RANDOM });
        assert(count_cards(run, CardId::STRIKE) + count_cards(run, CardId::DEFEND) + count_cards(run, CardId::BASH) <= strikes);
        assert(run.master_deck.size() == deck - 2 && !run.has_deck_choice());
        EventManager::apply_effect(run, { .type = EventEffectType::ENCHANT_CARD, .pick = EventPick::RANDOM,
                                          .enchantment = Enchantment::SHARP, .enchant_amount = 2 });
        int sharp = 0;
        for (const DeckCard& card : run.master_deck) sharp += card.enchantment == Enchantment::SHARP;
        assert(sharp == 1 && !run.has_deck_choice());
    }
    // Relics and potions: a given one, or drawn at random.
    {
        RunManager run = fresh_run();
        const size_t relics = run.relics.size();
        EventManager::apply_effect(run, { .type = EventEffectType::GAIN_RELIC, .ref = "VAJRA" });
        EventManager::apply_effect(run, { .type = EventEffectType::GAIN_RELIC, .ref = "VAJRA" }); // never twice
        assert(run.has_relic("VAJRA") && run.relics.size() == relics + 1);
        EventManager::apply_effect(run, { .type = EventEffectType::GAIN_RELIC });
        assert(run.relics.size() == relics + 2);
        EventManager::apply_effect(run, { .type = EventEffectType::GAIN_POTION, .ref = "FIRE_POTION" });
        assert(run.potions.size() == 1 && run.potions[0] == "FIRE_POTION");
        EventManager::apply_effect(run, { .type = EventEffectType::GAIN_POTION, .amount = 5 });
        assert(run.potions.size() == run.potion_slots);
        EventManager::apply_effect(run, { .type = EventEffectType::LEAVE });
        assert(run.potions.size() == run.potion_slots && run.phase == RunPhase::MAP_NAVIGATION);
    }
    std::cout << "[PASS] Event effects: gold, HP, Max HP, cards added / removed / upgraded / transformed / enchanted, "
                 "curses, relics, potions" << std::endl;
}

// Pages: an option leads to another page, the others end the event; the effects of an option apply in order;
// an option whose condition fails is greyed out and refused.
void test_event_pages_and_conditions() {
    {
        RunManager run = fresh_run();
        open(run, PAGES_EVENT);
        assert(run.current_event.dialogue == "Page 1" && run.current_event.choices.size() == 3);
        char message[64];
        assert(run.choose_event_choice(0, message, sizeof(message)));
        assert(std::string_view(message) == "Vous avancez." && run.gold == 110);
        assert(run.phase == RunPhase::EVENT && run.current_event.page == 1 && run.current_event.dialogue == "Page 2");
        assert(run.current_event.choices.size() == 2 && run.current_event.choices[0].enabled);
        // Chained: 50 gold paid, then 6 Max HP, then the heal sees the new maximum.
        assert(run.choose_event_choice(0));
        assert(run.gold == 60 && run.max_hp == 86 && run.hp == 86 && run.phase == RunPhase::MAP_NAVIGATION);
        assert(!run.choose_event_choice(0)); // the event is over
    }
    {
        RunManager run = fresh_run();
        run.gold = 0;
        open(run, PAGES_EVENT);
        assert(run.choose_event_choice(0) && run.gold == 10 && run.current_event.page == 1);
        // 10 gold: "Payer" needs 50.
        assert(!run.current_event.choices[0].enabled && run.current_event.choices[1].enabled);
        assert(!run.choose_event_choice(0) && run.phase == RunPhase::EVENT && run.gold == 10);
        assert(!run.choose_event_choice(7) && !run.choose_event_choice(-1));
        assert(run.choose_event_choice(1) && run.phase == RunPhase::MAP_NAVIGATION);
    }
    {
        RunManager run = fresh_run();
        open(run, PAGES_EVENT);
        assert(run.choose_event_choice(1) && run.hp == 35 && run.current_event.page == 2);
        assert(run.current_event.dialogue == "Page 3" && run.choose_event_choice(0) && run.phase == RunPhase::MAP_NAVIGATION);
    }
    // A next page is not opened once the event killed the run.
    {
        RunManager run = fresh_run();
        run.hp = 5;
        open(run, PAGES_EVENT);
        assert(run.choose_event_choice(1) && run.hp == 0 && run.current_event.page == 0 && run.phase == RunPhase::EVENT);
    }
    // The conditions.
    {
        RunManager run = fresh_run();
        assert(EventManager::meets(run, {}));
        assert(EventManager::meets(run, { .check = EventCheck::MIN_GOLD, .amount = 100 }));
        assert(!EventManager::meets(run, { .check = EventCheck::MIN_GOLD, .amount = 101 }));
        assert(EventManager::meets(run, { .check = EventCheck::MIN_HP, .amount = 39 }));
        assert(!EventManager::meets(run, { .check = EventCheck::MIN_HP, .amount = 40 }));
        assert(EventManager::meets(run, { .check = EventCheck::HAS_RELIC, .ref = "BURNING_BLOOD" }));
        assert(!EventManager::meets(run, { .check = EventCheck::NO_RELIC, .ref = "BURNING_BLOOD" }));
        assert(EventManager::meets(run, { .check = EventCheck::NO_RELIC, .ref = "VAJRA" }));
        assert(EventManager::meets(run, { .check = EventCheck::MIN_DECK, .amount = 10 }));
        assert(!EventManager::meets(run, { .check = EventCheck::MIN_DECK, .amount = 11 }));
        assert(!EventManager::meets(run, { .check = EventCheck::HAS_POTION }));
        run.potions.push_back("FIRE_POTION");
        assert(EventManager::meets(run, { .check = EventCheck::HAS_POTION }));
    }
    std::cout << "[PASS] Event pages: next page, chained effects, greyed-out options, conditions" << std::endl;
}

// Fights and reward screens: the event is left for them.
void test_event_fights_and_rewards() {
    // A fight in the room of the event, then its rewards.
    {
        RunManager run = fresh_run(7);
        int room = -1;
        for (size_t i = 0; i < run.map.nodes.size() && room < 0; ++i) {
            if (run.map.nodes[i].type == RoomType::EVENT) room = static_cast<int>(i);
        }
        assert(room >= 0);
        run.current_node_index = room;
        run.current_floor = static_cast<uint8_t>(run.map.nodes[room].floor + 1);
        open(run, OUTCOME_EVENT);
        assert(run.choose_event_choice(0));
        assert(run.phase == RunPhase::IN_COMBAT && !run.combat.enemies.empty() && run.combat.player.hp == 40);
        run.combat.state = CombatState::VICTORY;
        run.proceed_after_combat();
        assert(run.phase == RunPhase::REWARD_SCREEN && !run.current_card_rewards.empty());
    }
    {
        RunManager run = fresh_run(7);
        open(run, OUTCOME_EVENT);
        assert(run.choose_event_choice(1) && run.phase == RunPhase::IN_COMBAT && !run.combat.enemies.empty());
        assert(run.combat.enemy_hp_percent == run.get_enemy_hp_multiplier(RoomType::ELITE));
    }
    // A card reward: 1 of 3, then the map.
    {
        RunManager run = fresh_run();
        open(run, OUTCOME_EVENT);
        const size_t deck = run.master_deck.size();
        assert(run.choose_event_choice(2) && run.phase == RunPhase::REWARD_SCREEN);
        assert(run.current_card_rewards.size() == 3 && run.gold_reward == 0 && run.relic_reward.empty());
        run.pick_card_reward(0);
        assert(run.master_deck.size() == deck + 1 && run.phase == RunPhase::MAP_NAVIGATION && run.gold == 100);
    }
    // A relic offered on the reward screen.
    {
        RunManager run = fresh_run();
        open(run, OUTCOME_EVENT);
        const size_t relics = run.relics.size();
        assert(run.choose_event_choice(3) && run.phase == RunPhase::REWARD_SCREEN);
        assert(!run.relic_reward.empty() && run.current_card_rewards.empty() && !run.has_relic(run.relic_reward));
        assert(run.take_relic_reward() && run.relics.size() == relics + 1);
    }
    std::cout << "[PASS] Event fights (normal, elite) and reward screens (card, relic)" << std::endl;
}

// The event of a ? room comes from the act's zone, and an act shows each of its events once before any repeats.
void test_event_draw_per_act() {
    RunManager run = fresh_run(99);
    std::set<std::string_view> act3;
    run.current_act = 3;
    size_t available = 0;
    for (const EventSlot& slot : EventManager::slots()) {
        available += slot.def && (slot.def->acts & EVENT_ACT_3) != 0;
    }
    assert(available >= 2);
    for (size_t n = 0; n < available; ++n) {
        run.trigger_event();
        assert(run.phase == RunPhase::EVENT && !run.current_event.choices.empty());
        assert(act3.insert(run.current_event.id).second); // never twice while one is left
    }
    assert(act3.count("HUNGRY_FOR_MUSHROOMS") == 1);
    // All met: the draw starts again among them.
    run.trigger_event();
    assert(run.phase == RunPhase::EVENT && act3.count(run.current_event.id) == 1);

    // Act 1: never an event of Act 3 only.
    for (uint32_t seed = 1; seed < 60; ++seed) {
        RunManager first = fresh_run(seed);
        first.trigger_event();
        assert(first.phase == RunPhase::EVENT && first.current_event.id != "HUNGRY_FOR_MUSHROOMS");
    }

    // Entering a ? room draws its event.
    RunManager room_run = fresh_run(5);
    for (size_t i = 0; i < room_run.map.nodes.size(); ++i) {
        if (room_run.map.nodes[i].type != RoomType::EVENT) continue;
        room_run.enter_room(static_cast<int>(i));
        break;
    }
    assert(room_run.phase == RunPhase::EVENT && room_run.events_seen != 0);

    // The events met are saved with the run.
    SaveManager& store = SaveManager::instance();
    store.reset_to_defaults();
    run.events_seen |= uint64_t{ 1 } << 57;
    store.autosave_run(run);
    RunManager loaded;
    assert(store.restore_run(loaded) && loaded.events_seen == run.events_seen);
    store.reset_to_defaults();
    std::cout << "[PASS] Event draw: by act zone, no repeat in a run until the act has none left, saved" << std::endl;
}

int main() {
    test_event_registry();
    test_event_effects();
    test_event_pages_and_conditions();
    test_event_fights_and_rewards();
    test_event_draw_per_act();
    // The test of each event that has one (tests/events/<id>_test.h).
    for (const EventTest& test : EVENT_TESTS) {
        // An event file comes with its test.
        assert(!EventManager::find(test.id) || test.run);
        if (test.run) test.run();
    }
    std::cout << "[PASS] All event tests" << std::endl;
    return 0;
}
