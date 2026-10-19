// The registry of the events (#253): one slot per known event, filled when its file src/engine/events/<id>.h
// exists. Adding an event never touches this file (see include/engine/event_script.h). The order of the slots is
// the bit of each event in RunManager::events_seen, saved with the run: never reorder them, only append.
// MYSTIC_ALTAR is not an event of the reference: it is kept from the events the game had before the data.
#include "engine/event_manager.h"

#if __has_include("events/abyssal_baths.h")
#include "events/abyssal_baths.h"
#endif
#if __has_include("events/amalgamator.h")
#include "events/amalgamator.h"
#endif
#if __has_include("events/aroma_of_chaos.h")
#include "events/aroma_of_chaos.h"
#endif
#if __has_include("events/battleworn_dummy.h")
#include "events/battleworn_dummy.h"
#endif
#if __has_include("events/brain_leech.h")
#include "events/brain_leech.h"
#endif
#if __has_include("events/bugslayer.h")
#include "events/bugslayer.h"
#endif
#if __has_include("events/byrdonis_nest.h")
#include "events/byrdonis_nest.h"
#endif
#if __has_include("events/colorful_philosophers.h")
#include "events/colorful_philosophers.h"
#endif
#if __has_include("events/colossal_flower.h")
#include "events/colossal_flower.h"
#endif
#if __has_include("events/crystal_sphere.h")
#include "events/crystal_sphere.h"
#endif
#if __has_include("events/dense_vegetation.h")
#include "events/dense_vegetation.h"
#endif
#if __has_include("events/doll_room.h")
#include "events/doll_room.h"
#endif
#if __has_include("events/doors_of_light_and_dark.h")
#include "events/doors_of_light_and_dark.h"
#endif
#if __has_include("events/drowning_beacon.h")
#include "events/drowning_beacon.h"
#endif
#if __has_include("events/endless_conveyor.h")
#include "events/endless_conveyor.h"
#endif
#if __has_include("events/fake_merchant.h")
#include "events/fake_merchant.h"
#endif
#if __has_include("events/field_of_man_sized_holes.h")
#include "events/field_of_man_sized_holes.h"
#endif
#if __has_include("events/grave_of_the_forgotten.h")
#include "events/grave_of_the_forgotten.h"
#endif
#if __has_include("events/hungry_for_mushrooms.h")
#include "events/hungry_for_mushrooms.h"
#endif
#if __has_include("events/infested_automaton.h")
#include "events/infested_automaton.h"
#endif
#if __has_include("events/jungle_maze_adventure.h")
#include "events/jungle_maze_adventure.h"
#endif
#if __has_include("events/lost_wisp.h")
#include "events/lost_wisp.h"
#endif
#if __has_include("events/luminous_choir.h")
#include "events/luminous_choir.h"
#endif
#if __has_include("events/morphic_grove.h")
#include "events/morphic_grove.h"
#endif
#if __has_include("events/potion_courier.h")
#include "events/potion_courier.h"
#endif
#if __has_include("events/punch_off.h")
#include "events/punch_off.h"
#endif
#if __has_include("events/ranwid_the_elder.h")
#include "events/ranwid_the_elder.h"
#endif
#if __has_include("events/reflections.h")
#include "events/reflections.h"
#endif
#if __has_include("events/relic_trader.h")
#include "events/relic_trader.h"
#endif
#if __has_include("events/room_full_of_cheese.h")
#include "events/room_full_of_cheese.h"
#endif
#if __has_include("events/round_tea_party.h")
#include "events/round_tea_party.h"
#endif
#if __has_include("events/sapphire_seed.h")
#include "events/sapphire_seed.h"
#endif
#if __has_include("events/self_help_book.h")
#include "events/self_help_book.h"
#endif
#if __has_include("events/slippery_bridge.h")
#include "events/slippery_bridge.h"
#endif
#if __has_include("events/spiraling_whirlpool.h")
#include "events/spiraling_whirlpool.h"
#endif
#if __has_include("events/spirit_grafter.h")
#include "events/spirit_grafter.h"
#endif
#if __has_include("events/stone_of_all_time.h")
#include "events/stone_of_all_time.h"
#endif
#if __has_include("events/sunken_statue.h")
#include "events/sunken_statue.h"
#endif
#if __has_include("events/sunken_treasury.h")
#include "events/sunken_treasury.h"
#endif
#if __has_include("events/symbiote.h")
#include "events/symbiote.h"
#endif
#if __has_include("events/tablet_of_truth.h")
#include "events/tablet_of_truth.h"
#endif
#if __has_include("events/tea_master.h")
#include "events/tea_master.h"
#endif
#if __has_include("events/the_future_of_potions.h")
#include "events/the_future_of_potions.h"
#endif
#if __has_include("events/the_lantern_key.h")
#include "events/the_lantern_key.h"
#endif
#if __has_include("events/the_legends_were_true.h")
#include "events/the_legends_were_true.h"
#endif
#if __has_include("events/this_or_that.h")
#include "events/this_or_that.h"
#endif
#if __has_include("events/tinker_time.h")
#include "events/tinker_time.h"
#endif
#if __has_include("events/trash_heap.h")
#include "events/trash_heap.h"
#endif
#if __has_include("events/trial.h")
#include "events/trial.h"
#endif
#if __has_include("events/unrest_site.h")
#include "events/unrest_site.h"
#endif
#if __has_include("events/war_historian_repy.h")
#include "events/war_historian_repy.h"
#endif
#if __has_include("events/waterlogged_scriptorium.h")
#include "events/waterlogged_scriptorium.h"
#endif
#if __has_include("events/welcome_to_wongos.h")
#include "events/welcome_to_wongos.h"
#endif
#if __has_include("events/wellspring.h")
#include "events/wellspring.h"
#endif
#if __has_include("events/whispering_hollow.h")
#include "events/whispering_hollow.h"
#endif
#if __has_include("events/wood_carvings.h")
#include "events/wood_carvings.h"
#endif
#if __has_include("events/zen_weaver.h")
#include "events/zen_weaver.h"
#endif
#if __has_include("events/mystic_altar.h")
#include "events/mystic_altar.h"
#endif

namespace engine {

namespace {

constexpr EventSlot EVENT_SLOTS[] = {
#if __has_include("events/abyssal_baths.h")
    { "ABYSSAL_BATHS", &events::ABYSSAL_BATHS },
#else
    { "ABYSSAL_BATHS", nullptr },
#endif
#if __has_include("events/amalgamator.h")
    { "AMALGAMATOR", &events::AMALGAMATOR },
#else
    { "AMALGAMATOR", nullptr },
#endif
#if __has_include("events/aroma_of_chaos.h")
    { "AROMA_OF_CHAOS", &events::AROMA_OF_CHAOS },
#else
    { "AROMA_OF_CHAOS", nullptr },
#endif
#if __has_include("events/battleworn_dummy.h")
    { "BATTLEWORN_DUMMY", &events::BATTLEWORN_DUMMY },
#else
    { "BATTLEWORN_DUMMY", nullptr },
#endif
#if __has_include("events/brain_leech.h")
    { "BRAIN_LEECH", &events::BRAIN_LEECH },
#else
    { "BRAIN_LEECH", nullptr },
#endif
#if __has_include("events/bugslayer.h")
    { "BUGSLAYER", &events::BUGSLAYER },
#else
    { "BUGSLAYER", nullptr },
#endif
#if __has_include("events/byrdonis_nest.h")
    { "BYRDONIS_NEST", &events::BYRDONIS_NEST },
#else
    { "BYRDONIS_NEST", nullptr },
#endif
#if __has_include("events/colorful_philosophers.h")
    { "COLORFUL_PHILOSOPHERS", &events::COLORFUL_PHILOSOPHERS },
#else
    { "COLORFUL_PHILOSOPHERS", nullptr },
#endif
#if __has_include("events/colossal_flower.h")
    { "COLOSSAL_FLOWER", &events::COLOSSAL_FLOWER },
#else
    { "COLOSSAL_FLOWER", nullptr },
#endif
#if __has_include("events/crystal_sphere.h")
    { "CRYSTAL_SPHERE", &events::CRYSTAL_SPHERE },
#else
    { "CRYSTAL_SPHERE", nullptr },
#endif
#if __has_include("events/dense_vegetation.h")
    { "DENSE_VEGETATION", &events::DENSE_VEGETATION },
#else
    { "DENSE_VEGETATION", nullptr },
#endif
#if __has_include("events/doll_room.h")
    { "DOLL_ROOM", &events::DOLL_ROOM },
#else
    { "DOLL_ROOM", nullptr },
#endif
#if __has_include("events/doors_of_light_and_dark.h")
    { "DOORS_OF_LIGHT_AND_DARK", &events::DOORS_OF_LIGHT_AND_DARK },
#else
    { "DOORS_OF_LIGHT_AND_DARK", nullptr },
#endif
#if __has_include("events/drowning_beacon.h")
    { "DROWNING_BEACON", &events::DROWNING_BEACON },
#else
    { "DROWNING_BEACON", nullptr },
#endif
#if __has_include("events/endless_conveyor.h")
    { "ENDLESS_CONVEYOR", &events::ENDLESS_CONVEYOR },
#else
    { "ENDLESS_CONVEYOR", nullptr },
#endif
#if __has_include("events/fake_merchant.h")
    { "FAKE_MERCHANT", &events::FAKE_MERCHANT },
#else
    { "FAKE_MERCHANT", nullptr },
#endif
#if __has_include("events/field_of_man_sized_holes.h")
    { "FIELD_OF_MAN_SIZED_HOLES", &events::FIELD_OF_MAN_SIZED_HOLES },
#else
    { "FIELD_OF_MAN_SIZED_HOLES", nullptr },
#endif
#if __has_include("events/grave_of_the_forgotten.h")
    { "GRAVE_OF_THE_FORGOTTEN", &events::GRAVE_OF_THE_FORGOTTEN },
#else
    { "GRAVE_OF_THE_FORGOTTEN", nullptr },
#endif
#if __has_include("events/hungry_for_mushrooms.h")
    { "HUNGRY_FOR_MUSHROOMS", &events::HUNGRY_FOR_MUSHROOMS },
#else
    { "HUNGRY_FOR_MUSHROOMS", nullptr },
#endif
#if __has_include("events/infested_automaton.h")
    { "INFESTED_AUTOMATON", &events::INFESTED_AUTOMATON },
#else
    { "INFESTED_AUTOMATON", nullptr },
#endif
#if __has_include("events/jungle_maze_adventure.h")
    { "JUNGLE_MAZE_ADVENTURE", &events::JUNGLE_MAZE_ADVENTURE },
#else
    { "JUNGLE_MAZE_ADVENTURE", nullptr },
#endif
#if __has_include("events/lost_wisp.h")
    { "LOST_WISP", &events::LOST_WISP },
#else
    { "LOST_WISP", nullptr },
#endif
#if __has_include("events/luminous_choir.h")
    { "LUMINOUS_CHOIR", &events::LUMINOUS_CHOIR },
#else
    { "LUMINOUS_CHOIR", nullptr },
#endif
#if __has_include("events/morphic_grove.h")
    { "MORPHIC_GROVE", &events::MORPHIC_GROVE },
#else
    { "MORPHIC_GROVE", nullptr },
#endif
#if __has_include("events/potion_courier.h")
    { "POTION_COURIER", &events::POTION_COURIER },
#else
    { "POTION_COURIER", nullptr },
#endif
#if __has_include("events/punch_off.h")
    { "PUNCH_OFF", &events::PUNCH_OFF },
#else
    { "PUNCH_OFF", nullptr },
#endif
#if __has_include("events/ranwid_the_elder.h")
    { "RANWID_THE_ELDER", &events::RANWID_THE_ELDER },
#else
    { "RANWID_THE_ELDER", nullptr },
#endif
#if __has_include("events/reflections.h")
    { "REFLECTIONS", &events::REFLECTIONS },
#else
    { "REFLECTIONS", nullptr },
#endif
#if __has_include("events/relic_trader.h")
    { "RELIC_TRADER", &events::RELIC_TRADER },
#else
    { "RELIC_TRADER", nullptr },
#endif
#if __has_include("events/room_full_of_cheese.h")
    { "ROOM_FULL_OF_CHEESE", &events::ROOM_FULL_OF_CHEESE },
#else
    { "ROOM_FULL_OF_CHEESE", nullptr },
#endif
#if __has_include("events/round_tea_party.h")
    { "ROUND_TEA_PARTY", &events::ROUND_TEA_PARTY },
#else
    { "ROUND_TEA_PARTY", nullptr },
#endif
#if __has_include("events/sapphire_seed.h")
    { "SAPPHIRE_SEED", &events::SAPPHIRE_SEED },
#else
    { "SAPPHIRE_SEED", nullptr },
#endif
#if __has_include("events/self_help_book.h")
    { "SELF_HELP_BOOK", &events::SELF_HELP_BOOK },
#else
    { "SELF_HELP_BOOK", nullptr },
#endif
#if __has_include("events/slippery_bridge.h")
    { "SLIPPERY_BRIDGE", &events::SLIPPERY_BRIDGE },
#else
    { "SLIPPERY_BRIDGE", nullptr },
#endif
#if __has_include("events/spiraling_whirlpool.h")
    { "SPIRALING_WHIRLPOOL", &events::SPIRALING_WHIRLPOOL },
#else
    { "SPIRALING_WHIRLPOOL", nullptr },
#endif
#if __has_include("events/spirit_grafter.h")
    { "SPIRIT_GRAFTER", &events::SPIRIT_GRAFTER },
#else
    { "SPIRIT_GRAFTER", nullptr },
#endif
#if __has_include("events/stone_of_all_time.h")
    { "STONE_OF_ALL_TIME", &events::STONE_OF_ALL_TIME },
#else
    { "STONE_OF_ALL_TIME", nullptr },
#endif
#if __has_include("events/sunken_statue.h")
    { "SUNKEN_STATUE", &events::SUNKEN_STATUE },
#else
    { "SUNKEN_STATUE", nullptr },
#endif
#if __has_include("events/sunken_treasury.h")
    { "SUNKEN_TREASURY", &events::SUNKEN_TREASURY },
#else
    { "SUNKEN_TREASURY", nullptr },
#endif
#if __has_include("events/symbiote.h")
    { "SYMBIOTE", &events::SYMBIOTE },
#else
    { "SYMBIOTE", nullptr },
#endif
#if __has_include("events/tablet_of_truth.h")
    { "TABLET_OF_TRUTH", &events::TABLET_OF_TRUTH },
#else
    { "TABLET_OF_TRUTH", nullptr },
#endif
#if __has_include("events/tea_master.h")
    { "TEA_MASTER", &events::TEA_MASTER },
#else
    { "TEA_MASTER", nullptr },
#endif
#if __has_include("events/the_future_of_potions.h")
    { "THE_FUTURE_OF_POTIONS", &events::THE_FUTURE_OF_POTIONS },
#else
    { "THE_FUTURE_OF_POTIONS", nullptr },
#endif
#if __has_include("events/the_lantern_key.h")
    { "THE_LANTERN_KEY", &events::THE_LANTERN_KEY },
#else
    { "THE_LANTERN_KEY", nullptr },
#endif
#if __has_include("events/the_legends_were_true.h")
    { "THE_LEGENDS_WERE_TRUE", &events::THE_LEGENDS_WERE_TRUE },
#else
    { "THE_LEGENDS_WERE_TRUE", nullptr },
#endif
#if __has_include("events/this_or_that.h")
    { "THIS_OR_THAT", &events::THIS_OR_THAT },
#else
    { "THIS_OR_THAT", nullptr },
#endif
#if __has_include("events/tinker_time.h")
    { "TINKER_TIME", &events::TINKER_TIME },
#else
    { "TINKER_TIME", nullptr },
#endif
#if __has_include("events/trash_heap.h")
    { "TRASH_HEAP", &events::TRASH_HEAP },
#else
    { "TRASH_HEAP", nullptr },
#endif
#if __has_include("events/trial.h")
    { "TRIAL", &events::TRIAL },
#else
    { "TRIAL", nullptr },
#endif
#if __has_include("events/unrest_site.h")
    { "UNREST_SITE", &events::UNREST_SITE },
#else
    { "UNREST_SITE", nullptr },
#endif
#if __has_include("events/war_historian_repy.h")
    { "WAR_HISTORIAN_REPY", &events::WAR_HISTORIAN_REPY },
#else
    { "WAR_HISTORIAN_REPY", nullptr },
#endif
#if __has_include("events/waterlogged_scriptorium.h")
    { "WATERLOGGED_SCRIPTORIUM", &events::WATERLOGGED_SCRIPTORIUM },
#else
    { "WATERLOGGED_SCRIPTORIUM", nullptr },
#endif
#if __has_include("events/welcome_to_wongos.h")
    { "WELCOME_TO_WONGOS", &events::WELCOME_TO_WONGOS },
#else
    { "WELCOME_TO_WONGOS", nullptr },
#endif
#if __has_include("events/wellspring.h")
    { "WELLSPRING", &events::WELLSPRING },
#else
    { "WELLSPRING", nullptr },
#endif
#if __has_include("events/whispering_hollow.h")
    { "WHISPERING_HOLLOW", &events::WHISPERING_HOLLOW },
#else
    { "WHISPERING_HOLLOW", nullptr },
#endif
#if __has_include("events/wood_carvings.h")
    { "WOOD_CARVINGS", &events::WOOD_CARVINGS },
#else
    { "WOOD_CARVINGS", nullptr },
#endif
#if __has_include("events/zen_weaver.h")
    { "ZEN_WEAVER", &events::ZEN_WEAVER },
#else
    { "ZEN_WEAVER", nullptr },
#endif
#if __has_include("events/mystic_altar.h")
    { "MYSTIC_ALTAR", &events::MYSTIC_ALTAR },
#else
    { "MYSTIC_ALTAR", nullptr },
#endif
};

// Bits of a uint64_t in RunManager::events_seen.
static_assert(sizeof(EVENT_SLOTS) / sizeof(EVENT_SLOTS[0]) <= 64, "events_seen holds 64 events");

} // namespace

std::span<const EventSlot> event_slots() {
    return EVENT_SLOTS;
}

} // namespace engine
