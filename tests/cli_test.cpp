#include <iostream>
#include <cassert>
#include <algorithm>
#include <cstring>
#include <vector>
#include "engine/combat.h"
#include "engine/ancient.h"
#include "engine/relic.h"
#include "engine/monster.h"
#include "engine/potion.h"
#include "engine/event.h"
#include "engine/card_db.h"
#include "engine/character.h"
#include "engine/map.h"
#include "engine/run.h"
#include "engine/shop.h"
#include "engine/save_system.h"
#include "engine/title_menu.h"
#include "engine/navigation.h"
#include "engine/event_manager.h"
#include "engine/pokemon_battle_ui.h"

using namespace engine;

namespace {

// Stand-in for the GBA's battery SRAM (erased flash/SRAM reads as 0xFF).
class MemoryStorage final : public SaveStorage {
public:
    std::vector<uint8_t> bytes = std::vector<uint8_t>(32 * 1024, 0xFF);

    void read(void* destination, size_t size, size_t offset) override {
        std::memcpy(destination, bytes.data() + offset, size);
    }

    void write(const void* source, size_t size, size_t offset) override {
        std::memcpy(bytes.data() + offset, source, size);
    }
};

void test_cartridge_save() {
    MemoryStorage storage;
    SaveManager& store = SaveManager::instance();
    store.set_storage(&storage);

    // A blank cartridge has no valid bank: defaults are kept.
    assert(!store.load_from_storage());
    assert(!store.has_active_run());

    RunManager saved;
    saved.start_run(CharacterId::SILENT, 2, PaletteVariant::SHADOW, 999);
    saved.current_node_index = saved.get_available_rooms()[0];
    saved.hp = 40;
    saved.gold = 321;
    saved.add_relic("VAJRA");
    saved.potions.push_back(get_all_potions()[0].id);
    saved.master_deck.push_back(DeckCard(CardId::POISON_STAB, true));
    // What a card gained for good (Genetic Algorithm, The Scythe) is saved with it.
    saved.master_deck.back().bonus = 12;
    // Card ids are stored on 16 bits: one above 255 survives the save, with its upgrade.
    constexpr CardId FAR_CARD = static_cast<CardId>(300);
    saved.master_deck.push_back(DeckCard(FAR_CARD, true));
    saved.master_deck.push_back(DeckCard(static_cast<CardId>(511), false));
    store.settings().music_volume = 3;
    store.autosave_run(saved);

    // Power cycle: memory is wiped, the cartridge is read back.
    store.reset_to_defaults();
    assert(!store.has_active_run());
    assert(store.load_from_storage());
    assert(store.has_active_run());
    assert(store.settings().music_volume == 3);

    RunManager loaded;
    assert(store.restore_run(loaded));
    assert(loaded.character == CharacterId::SILENT);
    assert(loaded.ascension_level == 2);
    assert(loaded.palette == PaletteVariant::SHADOW);
    assert(loaded.hp == 40 && loaded.gold == 321);
    assert(loaded.master_deck.size() == saved.master_deck.size());
    size_t deck_size = loaded.master_deck.size();
    assert(loaded.master_deck[deck_size - 3] == DeckCard(CardId::POISON_STAB, true));
    assert(loaded.master_deck[deck_size - 3].bonus == 12 && loaded.master_deck[deck_size - 2].bonus == 0);
    assert(loaded.master_deck[deck_size - 2].id == FAR_CARD && loaded.master_deck[deck_size - 2].upgraded);
    assert(static_cast<uint16_t>(loaded.master_deck[deck_size - 1].id) == 511 && !loaded.master_deck[deck_size - 1].upgraded);
    assert(sizeof(CardId) == 2 && sizeof(DeckCardSave) == 4);
    assert(loaded.relics.size() == saved.relics.size());
    assert(loaded.relics.back() == "VAJRA");
    assert(loaded.potions.size() == 1 && loaded.potions[0] == saved.potions[0]);
    assert(loaded.current_node_index == saved.current_node_index);
    assert(loaded.rng_state() == saved.rng_state());
    assert(loaded.map.nodes.size() == saved.map.nodes.size());
    for (size_t i = 0; i < saved.map.nodes.size(); ++i) {
        assert(loaded.map.nodes[i].type == saved.map.nodes[i].type);
    }
    assert(loaded.phase == RunPhase::MAP_NAVIGATION);

    // A corrupted newest bank falls back to the previous save instead of loading garbage.
    saved.gold = 500;
    store.autosave_run(saved);
    storage.bytes[SAVE_BANK_SIZE + 64] ^= 0xFF;
    store.reset_to_defaults();
    assert(store.load_from_storage());
    RunManager fallback;
    assert(store.restore_run(fallback));
    assert(fallback.gold == 321);

    // Game over clears the run from the cartridge too.
    store.record_defeat();
    store.reset_to_defaults();
    assert(store.load_from_storage());
    assert(!store.has_active_run());
    assert(store.get_active_profile().total_runs == 1);

    // A cartridge saved with an older layout (8-bit card ids) is not read as if it were the new one:
    // the game starts again from the defaults.
    store.autosave_run(saved);
    for (size_t bank = 0; bank < 2; ++bank) {
        uint16_t old_version = SAVE_FORMAT_VERSION - 1;
        std::memcpy(storage.bytes.data() + bank * SAVE_BANK_SIZE + 8, &old_version, sizeof(old_version));
    }
    store.reset_to_defaults();
    assert(!store.load_from_storage());
    assert(!store.has_active_run());
    assert(store.settings().music_volume == 8);
    assert(store.get_active_profile().total_runs == 0);

    // The next save makes the cartridge valid again.
    store.settings().music_volume = 5;
    store.commit_to_storage();
    store.reset_to_defaults();
    assert(store.load_from_storage() && store.settings().music_volume == 5);

    store.set_storage(nullptr);
    store.reset_to_defaults();
    std::cout << "[PASS] Cartridge Save: SRAM banks, checksum fallback, full run restore and settings verified" << std::endl;
}

// Map layouts are random walks: check the structural rules on many seeds rather than one snapshot.
void test_map_generation() {
    int width_seen[4] = {};
    int right_branches = 0;
    int left_branches = 0;

    for (uint32_t seed = 1; seed <= 500; ++seed) {
        uint8_t act = static_cast<uint8_t>(1 + seed % 3);
        MapGraph map;
        map.generate(act, seed * 2654435761u);

        // Same act and seed: same map (saves regenerate it from the seed).
        MapGraph again;
        again.generate(act, seed * 2654435761u);
        assert(again.nodes.size() == map.nodes.size());
        for (size_t i = 0; i < map.nodes.size(); ++i) {
            assert(again.nodes[i].type == map.nodes[i].type && again.nodes[i].lane == map.nodes[i].lane);
        }

        assert(map.boss_node_index == static_cast<int>(map.nodes.size()) - 1);
        const MapNode& boss = map.nodes[map.boss_node_index];
        assert(boss.type == RoomType::BOSS && boss.floor == 15 && boss.children_count == 0);

        auto starts = map.get_starting_nodes();
        assert(starts.size() >= 2);

        int width[15] = {};
        int parents[MAX_MAP_NODES] = {};
        int events = 0;
        for (size_t i = 0; i < map.nodes.size(); ++i) {
            const MapNode& node = map.nodes[i];
            if (node.type == RoomType::EVENT) ++events;
            if (node.floor == 15) continue;
            ++width[node.floor];
            assert(node.lane < 3);
            assert(node.children_count >= 1);

            if (node.floor == 0) assert(node.type == RoomType::MONSTER);
            if (node.floor == 7) assert(node.type == RoomType::TREASURE);
            if (node.floor == 14) assert(node.type == RoomType::REST_SITE);
            if (node.type == RoomType::ELITE || node.type == RoomType::REST_SITE) assert(node.floor >= 5);
            if (node.type == RoomType::REST_SITE) assert(node.floor != 13);

            for (uint8_t c = 0; c < node.children_count; ++c) {
                const MapNode& child = map.nodes[node.children[c]];
                ++parents[node.children[c]];
                assert(child.floor == node.floor + 1);
                if (child.floor == 15) continue;
                // One lane at most, left or right, listed in screen order.
                assert(child.lane + 1 >= node.lane && child.lane <= node.lane + 1);
                if (c > 0) assert(map.nodes[node.children[c - 1]].lane < child.lane);
                if (child.lane > node.lane) ++right_branches;
                if (child.lane < node.lane) ++left_branches;
                // No two elites, campfires or shops in a row.
                if (node.type == RoomType::ELITE || node.type == RoomType::REST_SITE || node.type == RoomType::SHOP) {
                    assert(child.type != node.type);
                }
                // Paths never cross: no edge from this node swaps sides with an edge from a node to its right.
                for (size_t j = 0; j < map.nodes.size(); ++j) {
                    const MapNode& other = map.nodes[j];
                    if (other.floor != node.floor || other.lane <= node.lane) continue;
                    for (uint8_t oc = 0; oc < other.children_count; ++oc) {
                        assert(map.nodes[other.children[oc]].lane >= child.lane);
                    }
                }
            }
        }
        // Every room above the first floor can be reached.
        for (size_t i = 0; i < map.nodes.size(); ++i) {
            if (map.nodes[i].floor > 0) assert(parents[i] >= 1);
        }
        assert(width[14] == 1);
        assert(events >= 3);
        for (int floor = 1; floor < 14; ++floor) {
            assert(width[floor] >= 1 && width[floor] <= 3);
            ++width_seen[width[floor]];
        }
    }

    // Ascension 1: more elites on the same seeds.
    int elites[2] = {};
    for (uint32_t seed = 1; seed <= 200; ++seed) {
        for (uint8_t ascension = 0; ascension < 2; ++ascension) {
            MapGraph map;
            map.generate(1, seed * 2654435761u, ascension);
            for (const MapNode& node : map.nodes) elites[ascension] += node.type == RoomType::ELITE;
        }
    }
    assert(elites[1] > elites[0] * 5 / 4);

    // Floors of 1, 2 and 3 rooms all occur, and paths branch both ways.
    assert(width_seen[1] > 0 && width_seen[2] > 0 && width_seen[3] > 0);
    assert(right_branches > 0 && left_branches > 0);
    std::cout << "[PASS] Procedural Map: 500 maps with 1-3 rooms per floor, left/right branches, no crossing, no orphan room" << std::endl;
}

} // namespace

int main() {
    std::cout << "=== Slay The GBA Engine Test ===" << std::endl;

    CombatEngine combat;
    combat.start_combat(EnemyType::CULTIST);

    assert(combat.player.hp == 80);
    assert(combat.player.energy == 3);
    assert(combat.hand.size() == 5);
    assert(combat.enemy.hp == 50);
    std::cout << "[PASS] Combat initialized properly" << std::endl;

    // Test playing Strike or Bash if in hand
    bool played = false;
    for (size_t i = 0; i < combat.hand.size(); ++i) {
        if (combat.hand[i] == CardId::STRIKE) {
            int16_t old_hp = combat.enemy.hp;
            bool ok = combat.play_card(i);
            assert(ok);
            assert(combat.enemy.hp == old_hp - 6);
            assert(combat.player.energy == 2);
            played = true;
            std::cout << "[PASS] Played Strike! Cultist HP: " << combat.enemy.hp << std::endl;
            break;
        }
    }
    assert(played);

    // End turn
    combat.end_player_turn();
    assert(combat.turn_number == 2);
    assert(combat.player.energy == 3);
    assert(combat.hand.size() == 5);
    // Cultist cast Ritual +3 on turn 1: it only starts granting Strength at the end of turn 2.
    assert(combat.enemy.get_status(StatusType::RITUAL) == 3);
    assert(combat.enemy.get_status(StatusType::STRENGTH) == 0);
    combat.end_player_turn();
    assert(combat.enemy.get_status(StatusType::STRENGTH) == 3);
    std::cout << "[PASS] Turn ended, Cultist executed Ritual! Strength is now: "
              << combat.enemy.get_status(StatusType::STRENGTH) << std::endl;

    // Test Ancients System (Patch v0.111.0 Beta)
    auto ancients = get_all_ancients();
    assert(ancients.size() == 8);
    std::cout << "[PASS] Ancients loaded: " << ancients.size() << " ancients registered" << std::endl;

    const auto& neow = get_ancient(AncientId::NEOW);
    assert(neow.name == "Neow");
    assert(neow.slots.size() == 2);
    assert(neow.slots[0].options.size() == 20);
    assert(neow.slots[1].options.size() == 10);
    std::cout << "[PASS] Neow validated with " << (neow.slots[0].options.size() + neow.slots[1].options.size()) << " relics" << std::endl;

    const auto& darv = get_ancient(AncientId::DARV);
    assert(darv.name == "Darv");
    assert(darv.slots[0].options.size() == 12);
    std::cout << "[PASS] Darv validated with " << darv.slots[0].options.size() << " relics" << std::endl;

    const auto& vakuu = get_ancient(AncientId::VAKUU);
    assert(vakuu.name == "Vakuu");
    assert(vakuu.slots.size() == 3);
    std::cout << "[PASS] Vakuu validated with 3 slots" << std::endl;

    // Test Complete Cards Database (569 cards)
    auto all_cards = get_all_card_records();
    assert(all_cards.size() == 569);
    const auto* abrasive = find_card_record("ABRASIVE");
    assert(abrasive != nullptr && abrasive->name == "Abrasif");
    std::cout << "[PASS] Cards database: 569 cards registered (validated: " << abrasive->name << ")" << std::endl;

    // Test Complete Relics Database (298 relics)
    auto all_relics = get_all_relics();
    assert(all_relics.size() == 298);
    const auto* akabeko = find_relic("AKABEKO");
    assert(akabeko != nullptr && akabeko->name == "Akabeko");
    std::cout << "[PASS] Relics database: 298 relics registered (validated: " << akabeko->name << ")" << std::endl;

    // Test Complete Monsters Database (112 monsters)
    auto all_monsters = get_all_monsters();
    assert(all_monsters.size() == 112);
    const auto* aeonglass = find_monster("AEONGLASS");
    assert(aeonglass != nullptr && aeonglass->name == "Clepséon");
    assert(aeonglass->min_hp == 512);
    std::cout << "[PASS] Monsters database: 112 monsters registered (validated: " << aeonglass->name << " HP: " << aeonglass->min_hp << ")" << std::endl;

    // Test Complete Potions Database (64 potions)
    auto all_potions = get_all_potions();
    assert(all_potions.size() == 64);
    const auto* ambergris = find_potion("AMBERGRIS");
    assert(ambergris != nullptr && ambergris->name == "Ambre gris");
    std::cout << "[PASS] Potions database: 64 potions registered (validated: " << ambergris->name << ")" << std::endl;

    // Test Complete Events Database (57 events)
    auto all_events = get_all_events();
    assert(all_events.size() == 57);
    const auto* abyssal_baths = find_event("ABYSSAL_BATHS");
    assert(abyssal_baths != nullptr && abyssal_baths->name == "Bains abyssaux");
    std::cout << "[PASS] Events database: 57 events registered (validated: " << abyssal_baths->name << ")" << std::endl;

    // Test Playable Characters & Starter Decks (5 characters)
    auto all_characters = get_all_characters();
    assert(all_characters.size() == 5);
    const auto& ironclad = get_character(CharacterId::IRONCLAD);
    assert(ironclad.starting_hp == 80);
    assert(ironclad.starting_relic_id == "BURNING_BLOOD");
    assert(ironclad.starter_deck.size() == 3);

    const auto& silent = get_character(CharacterId::SILENT);
    assert(silent.starting_hp == 70);
    assert(silent.starting_relic_id == "RING_OF_THE_SNAKE");
    assert(silent.starter_deck.size() == 4);

    const auto& defect = get_character(CharacterId::DEFECT);
    assert(defect.starting_hp == 75);
    assert(defect.starting_relic_id == "CRACKED_CORE");

    const auto& regent = get_character(CharacterId::REGENT);
    assert(regent.starting_hp == 75);
    assert(regent.starting_relic_id == "DIVINE_RIGHT");

    const auto& necrobinder = get_character(CharacterId::NECROBINDER);
    assert(necrobinder.starting_hp == 65);
    assert(necrobinder.starting_relic_id == "BOUND_PHYLACTERY");
    std::cout << "[PASS] Characters database: 5 characters registered (Ironclad 80HP, Silent 70HP, Defect 75HP, Regent 75HP, Necrobinder 65HP)" << std::endl;

    // Test Character-specific and Common Cards distribution
    assert(count_cards_by_pool("Ironclad") == 90);
    assert(count_cards_by_pool("Silent") == 91);
    assert(count_cards_by_pool("Defect") == 91);
    assert(count_cards_by_pool("Regent") == 91);
    assert(count_cards_by_pool("Necrobinder") == 91);

    // Verify exactly 20 Common cards for EVERY character
    assert(count_cards_by_pool_and_rarity("Ironclad", "Common") == 20);
    assert(count_cards_by_pool_and_rarity("Silent", "Common") == 20);
    assert(count_cards_by_pool_and_rarity("Defect", "Common") == 20);
    assert(count_cards_by_pool_and_rarity("Regent", "Common") == 20);
    assert(count_cards_by_pool_and_rarity("Necrobinder", "Common") == 20);
    std::cout << "[PASS] Common cards validated: exactly 20 common cards for each of the 5 characters!" << std::endl;

    test_map_generation();

    // Test Full Roguelike Run Loop (RunManager)
    RunManager run;
    run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    assert(run.character == CharacterId::IRONCLAD);
    assert(run.hp == 80 && run.max_hp == 80);
    assert(run.gold == 99);
    assert(run.master_deck.size() == 10); // 5 Strike, 4 Defend, 1 Bash
    assert(run.relics.size() == 1 && run.relics[0] == "BURNING_BLOOD");
    assert(run.current_act == 1 && run.current_floor == 1);
    assert(run.phase == RunPhase::MAP_NAVIGATION);

    // Test Room Entry & Combat initialization
    auto available = run.get_available_rooms();
    assert(available.size() >= 2);
    run.enter_room(available[0]);
    assert(run.phase == RunPhase::IN_COMBAT);
    assert(run.combat.player.hp == 80);
    assert(run.combat.draw_pile.size() + run.combat.hand.size() == 10);

    // Simulate winning combat & reward screen
    run.combat.player.hp = 70; // took 10 dmg
    run.proceed_after_combat();
    // Burning blood heals 6 HP -> 76 HP
    assert(run.hp == 76);
    assert(run.phase == RunPhase::REWARD_SCREEN);
    assert(run.current_card_rewards.size() == 3);
    int prev_deck_size = static_cast<int>(run.master_deck.size());
    int prev_gold = run.gold;
    run.pick_card_reward(0);
    assert(run.master_deck.size() == prev_deck_size + 1);
    assert(run.gold > prev_gold);
    assert(run.phase == RunPhase::MAP_NAVIGATION);
    std::cout << "[PASS] Run Loop: Deck, Combat, Burning Blood (+6 HP), Card Reward & Gold picked" << std::endl;

    // Vajra: every combat starts with 1 Strength, and only with the relic.
    RunManager plain_run;
    plain_run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    plain_run.enter_room(plain_run.get_available_rooms()[0]);
    assert(!plain_run.has_relic("VAJRA"));
    assert(plain_run.combat.player.get_status(StatusType::STRENGTH) == 0);

    RunManager vajra_run;
    vajra_run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    vajra_run.add_relic("VAJRA");
    assert(vajra_run.has_relic("VAJRA"));
    vajra_run.enter_room(vajra_run.get_available_rooms()[0]);
    assert(vajra_run.phase == RunPhase::IN_COMBAT);
    assert(vajra_run.combat.player.get_status(StatusType::STRENGTH) == 1);
    std::cout << "[PASS] Vajra: +1 Strength at the start of each combat" << std::endl;

    // Starter relics: Ring of the Snake draws 2 more cards, Cracked Core channels a Lightning orb.
    assert(plain_run.combat.hand.size() == 5);
    assert(plain_run.combat.player.orbs.empty());

    RunManager silent_run;
    silent_run.start_run(CharacterId::SILENT, 0, PaletteVariant::CLASSIC, 777);
    assert(silent_run.has_relic("RING_OF_THE_SNAKE"));
    silent_run.enter_room(silent_run.get_available_rooms()[0]);
    assert(silent_run.combat.hand.size() == 7);

    RunManager defect_run;
    defect_run.start_run(CharacterId::DEFECT, 0, PaletteVariant::CLASSIC, 777);
    assert(defect_run.has_relic("CRACKED_CORE"));
    defect_run.enter_room(defect_run.get_available_rooms()[0]);
    assert(defect_run.combat.player.orbs.size() == 1);
    assert(defect_run.combat.player.orbs[0] == OrbType::LIGHTNING);
    assert(defect_run.combat.hand.size() == 5);
    std::cout << "[PASS] Starter relics: Ring of the Snake (+2 cards) & Cracked Core (Lightning orb)" << std::endl;

    // Anchor (bought at the shop): every combat starts with 10 Block.
    assert(plain_run.combat.player.block == 0);
    RunManager anchor_run;
    anchor_run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    anchor_run.gold = 150;
    // The stock of the merchant is drawn at random: the Anchor is put on its shelf by hand.
    ShopInventory anchor_shop;
    anchor_shop.generate(CharacterId::IRONCLAD, 1);
    anchor_shop.relics.clear();
    anchor_shop.relics.push_back({ "ANCHOR", 150, false });
    assert(anchor_shop.buy_relic(anchor_run, 0));
    assert(anchor_run.has_relic("ANCHOR"));
    anchor_run.enter_room(anchor_run.get_available_rooms()[0]);
    assert(anchor_run.combat.player.block == 10);
    std::cout << "[PASS] Anchor: 10 Block at the start of each combat" << std::endl;

    // Relics acting when a combat starts. The first room is a normal fight against one enemy.
    auto start_with = [](std::initializer_list<std::string_view> relic_ids, RoomType room = RoomType::MONSTER) {
        RunManager run;
        run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
        run.hp = 50;
        for (std::string_view id : relic_ids) run.add_relic(id);
        int first_room = run.get_available_rooms()[0];
        run.map.nodes[first_room].type = room;
        run.enter_room(first_room);
        assert(run.phase == RunPhase::IN_COMBAT);
        return run;
    };
    RunManager bare = start_with({});
    assert(bare.combat.player.hp == 50 && bare.combat.player.energy == 3);

    RunManager stone = start_with({ "ODDLY_SMOOTH_STONE", "LANTERN", "BLOOD_VIAL" });
    assert(stone.combat.player.get_status(StatusType::DEXTERITY) == 1);
    assert(stone.combat.player.energy == 4);
    assert(stone.combat.player.hp == 52);
    stone.combat.end_player_turn();
    assert(stone.combat.player.energy == 3); // the Lantern only lights the first turn

    RunManager mask = start_with({ "RED_MASK", "BAG_OF_MARBLES", "FESTIVE_POPPER" });
    for (const Enemy& foe : mask.combat.enemies) {
        assert(foe.get_status(StatusType::WEAK) == 1 && foe.get_status(StatusType::VULNERABLE) == 1);
        assert(foe.hp == foe.max_hp - 9);
    }
    assert(mask.combat.enemy.hp == mask.combat.enemies[0].hp);

    // Sling of Courage and Pantograph depend on the room.
    assert(start_with({ "SLING_OF_COURAGE", "PANTOGRAPH" }).combat.player.get_status(StatusType::STRENGTH) == 0);
    assert(start_with({ "SLING_OF_COURAGE" }, RoomType::ELITE).combat.player.get_status(StatusType::STRENGTH) == 2);
    assert(start_with({ "PANTOGRAPH" }, RoomType::ELITE).combat.player.hp == 50);
    assert(start_with({ "PANTOGRAPH" }, RoomType::BOSS).combat.player.hp == 75);

    // Stone Cracker upgrades 2 cards of the draw pile for the combat, not in the deck.
    RunManager cracker = start_with({ "STONE_CRACKER" });
    int upgraded_in_pile = 0;
    for (const DeckCard& card : cracker.combat.draw_pile) upgraded_in_pile += card.upgraded;
    assert(upgraded_in_pile == 2);
    for (const DeckCard& card : cracker.master_deck) assert(!card.upgraded);
    for (const DeckCard& card : bare.combat.draw_pile) assert(!card.upgraded);

    ShopInventory sling_shop;
    sling_shop.generate(CharacterId::IRONCLAD, 1);
    // The third relic of the merchant is one of its own (Shop rarity), as the Sling of Courage is.
    assert(sling_shop.relics.size() == 3 && find_relic(sling_shop.relics[2].relic_id)->rarity == RelicRarity::SHOP);
    assert(find_relic("SLING_OF_COURAGE")->rarity == RelicRarity::SHOP);
    std::cout << "[PASS] Combat-start relics: Oddly Smooth Stone, Lantern, Blood Vial, Red Mask, Bag of Marbles, "
                 "Festive Popper, Sling of Courage, Pantograph, Stone Cracker" << std::endl;

    // Potions are drunk through the engine: effect applied, potion removed from the belt.
    RunManager potion_run;
    potion_run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    potion_run.potions.push_back("FIRE_POTION");
    potion_run.potions.push_back("BLOCK_POTION");
    potion_run.potions.push_back("NOT_A_POTION");
    assert(!potion_run.use_potion(0)); // not in combat yet
    potion_run.enter_room(potion_run.get_available_rooms()[0]);
    CombatEngine& potion_combat = potion_run.combat;
    int16_t foe_hp = potion_combat.enemies[0].hp;
    assert(potion_run.use_potion(0, 0));
    assert(potion_combat.enemies[0].hp == foe_hp - 20);
    assert(potion_combat.enemy.hp == potion_combat.enemies[0].hp);
    assert(potion_run.potions.size() == 2 && potion_run.potions[0] == "BLOCK_POTION");
    assert(potion_run.use_potion(0));
    assert(potion_combat.player.block == 12);
    assert(!potion_run.use_potion(0)); // unknown potion: kept in the belt
    assert(potion_run.potions.size() == 1);
    assert(!potion_run.use_potion(5));

    int8_t energy_before = potion_combat.player.energy;
    assert(potion_combat.use_potion("ENERGY_POTION"));
    assert(potion_combat.player.energy == energy_before + 2);
    potion_combat.player.hp = 40;
    assert(potion_combat.use_potion("BLOOD_POTION"));
    assert(potion_combat.player.hp == 40 + 80 / 5);

    // A Fire Potion that kills the last enemy wins the fight; nothing can be drunk once it is over.
    for (size_t i = 0; i < potion_combat.enemies.size(); ++i) potion_combat.enemies[i].hp = 5;
    for (size_t i = 0; i < potion_combat.enemies.size(); ++i) assert(potion_combat.use_potion("FIRE_POTION"));
    assert(potion_combat.state == CombatState::VICTORY);
    assert(!potion_combat.use_potion("BLOCK_POTION"));
    std::cout << "[PASS] Potions: Fire, Block, Energy & Blood drunk in combat through the engine" << std::endl;

    // Treasure chests give relics with an implemented effect, shared or of the hero, never twice.
    RunManager chest_run;
    chest_run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 777);
    int chest_room = chest_run.get_available_rooms()[0];
    chest_run.map.nodes[chest_room].type = RoomType::TREASURE;
    int chest_gold = chest_run.gold;
    chest_run.enter_room(chest_room);
    assert(chest_run.phase == RunPhase::TREASURE);
    assert(chest_run.gold == chest_gold + 50);
    assert(chest_run.relics.size() == 2);
    std::span<const std::string_view> chest_relics = RunManager::active_relics();
    auto can_hold = [&](std::string_view id) {
        const RelicData* relic = find_relic(id);
        bool listed = std::find(chest_relics.begin(), chest_relics.end(), id) != chest_relics.end();
        return listed && relic && (relic->pool == "Shared" || relic->pool == "Ironclad");
    };
    for (std::string_view id : chest_relics) assert(find_relic(id) != nullptr);
    assert(!can_hold("TINGSHA") && can_hold("HAPPY_FLOWER")); // Tingsha belongs to the Silent
    assert(can_hold(chest_run.relics[1]) && chest_run.last_treasure_relic == chest_run.relics[1]);
    while (chest_run.relics.size() < chest_run.relics.capacity()) {
        std::string_view relic_id = chest_run.draw_treasure_relic();
        assert(can_hold(relic_id) && !chest_run.has_relic(relic_id));
        chest_run.add_relic(relic_id);
    }

    // The merchant never stocks a relic the hero already owns.
    chest_run.open_shop();
    for (const ShopRelic& item : chest_run.current_shop.relics) assert(!chest_run.has_relic(item.relic_id));
    potion_run.open_shop();
    assert(potion_run.current_shop.relics.size() == 3);
    for (const ShopRelic& item : potion_run.current_shop.relics) {
        assert(!item.purchased && !potion_run.has_relic(item.relic_id));
    }
    std::cout << "[PASS] Relics: chests and the shop never give a relic twice" << std::endl;

    // Card rewards: distinct cards drawn at random from the hero's pool.
    for (CharacterId hero : { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT, CharacterId::REGENT,
                              CharacterId::NECROBINDER }) {
        std::span<const CardId> pool = RunManager::card_reward_pool(hero);
        assert(pool.size() >= 3);
        RunManager reward_run;
        reward_run.start_run(hero, 0, PaletteVariant::CLASSIC, 4242);
        for (int round = 0; round < 20; ++round) {
            reward_run.generate_rewards();
            assert(reward_run.phase == RunPhase::REWARD_SCREEN);
            assert(reward_run.current_card_rewards.size() == 3);
            for (size_t i = 0; i < 3; ++i) {
                const DeckCard& card = reward_run.current_card_rewards[i];
                assert(!card.upgraded);
                assert(std::find(pool.begin(), pool.end(), card.id) != pool.end());
                for (size_t j = 0; j < i; ++j) assert(reward_run.current_card_rewards[j].id != card.id);
            }
        }
    }
    std::cout << "[PASS] Card rewards: 3 distinct cards drawn from each hero's pool" << std::endl;

    // Basic Strike and Defend are never offered: neither as a combat reward nor at the merchant.
    for (CharacterId hero : { CharacterId::IRONCLAD, CharacterId::SILENT, CharacterId::DEFECT, CharacterId::REGENT,
                              CharacterId::NECROBINDER }) {
        for (CardId id : RunManager::card_reward_pool(hero)) {
            assert(id != CardId::STRIKE && id != CardId::DEFEND);
        }
        ShopInventory shop;
        shop.generate(hero, 1);
        for (const auto& item : shop.cards) {
            assert(item.card.id != CardId::STRIKE && item.card.id != CardId::DEFEND);
        }
    }
    std::cout << "[PASS] Rewards & shop: no basic Strike or Defend offered to any hero" << std::endl;

    // Test Campfire Rest
    assert(run.can_rest());
    int hp_before_rest = run.hp;
    run.rest_at_campfire();
    // Rest heals 30% of max_hp (80 * 0.30 = 24) -> 76 + 24 = 100 capped at 80
    assert(run.hp == 80);
    assert(run.hp > hp_before_rest);
    std::cout << "[PASS] Campfire: Rest healed HP back to max" << std::endl;

    // Test Ascension Modifiers (Ascension 0 to 10)
    auto asc_mods = RunManager::get_ascension_modifiers();
    assert(asc_mods.size() == 11);
    assert(asc_mods[1].level == 1);
    assert(asc_mods[10].level == 10);
    assert(asc_mods[10].title == "Fléau de l'Ascension");

    // Ascension 6: Player starts with 10% less HP (80 -> 72)
    RunManager run_asc6;
    run_asc6.start_run(CharacterId::IRONCLAD, 6, PaletteVariant::CLASSIC, 777);
    assert(run_asc6.hp == 72);
    assert(run_asc6.max_hp == 80);

    // Ascension 2: Enemy damage multiplier (+20%)
    assert(run.get_enemy_damage_multiplier() == 100);
    assert(run_asc6.get_enemy_damage_multiplier() == 120);

    // Ascension 7 & 10: Enemy HP multiplier (+15%)
    RunManager run_asc10;
    run_asc10.start_run(CharacterId::NECROBINDER, 10, PaletteVariant::CLASSIC, 777);
    assert(run_asc10.hp == 58); // 65 * 9 / 10 = 58
    assert(run_asc10.get_enemy_damage_multiplier() == 120);
    assert(run_asc10.get_enemy_hp_multiplier() == 115);
    std::cout << "[PASS] Ascension 0-10: Levels, handicap HP (A6/A10), dmg mult (A2), and HP mult (A7) verified" << std::endl;

    // Test Act Progression & Victory
    run.current_act = 3;
    run.current_node_index = run.map.boss_node_index;
    run.proceed_after_combat();
    assert(run.is_run_completed());
    std::cout << "[PASS] Act Progression: Act 3 Boss victory triggers Run Completed" << std::endl;

    // Test 3 Save Profiles & Autosave System
    SaveManager& sm = SaveManager::instance();
    sm.init();
    sm.select_profile(0);
    assert(sm.get_active_profile_index() == 0);
    sm.create_profile(1, "Alexandre");
    assert(std::string_view(sm.get_profile(1).name) == "Alexandre");
    assert(sm.get_profile(1).is_created);

    // Test Autosave & Run Restoring
    run.hp = 64;
    run.gold = 188;
    sm.autosave_run(run);
    assert(sm.get_active_profile().active_run.has_active_run);
    assert(sm.get_active_profile().active_run.hp == 64);
    assert(sm.get_active_profile().active_run.gold == 188);

    RunManager restored_run;
    bool ok_restore = sm.restore_run(restored_run);
    assert(ok_restore);
    assert(restored_run.hp == 64);
    assert(restored_run.gold == 188);
    assert(restored_run.master_deck.size() == run.master_deck.size());
    std::cout << "[PASS] Save System: 3 Profiles, Autosave checkpoint and Run Restoration verified" << std::endl;

    // Test Compendium / Codex Progressive Unlocks
    sm.discover_card_by_id("ABRASIVE");
    sm.discover_relic_by_id("AKABEKO");
    sm.discover_monster_by_id("AEONGLASS");
    sm.discover_potion_by_id("AMBERGRIS");
    sm.discover_ancient_by_id(0); // Neow
    assert(sm.get_active_profile().count_discovered_cards() > 0);
    assert(sm.get_active_profile().count_discovered_relics() > 0);
    assert(sm.get_active_profile().count_discovered_monsters() > 0);
    assert(sm.get_active_profile().count_discovered_potions() > 0);
    assert(sm.get_active_profile().count_discovered_ancients() > 0);
    std::cout << "[PASS] Compendium: Progressive discovery tracking verified for Cards, Relics, Monsters, Potions, Ancients" << std::endl;

    // Test Campfire Smithing / Upgrading Cards
    assert(restored_run.can_smith());
    assert(!restored_run.master_deck[0].upgraded);
    restored_run.smith_at_campfire(0); // Upgrade first card (Strike -> Strike+)
    assert(restored_run.master_deck[0].upgraded);
    assert(restored_run.phase == RunPhase::MAP_NAVIGATION);

    // In combat, verify Strike+ deals 9 damage instead of 6
    CombatEngine up_combat;
    up_combat.start_combat(EnemyType::CULTIST);
    up_combat.hand.clear();
    up_combat.hand.push_back(DeckCard(CardId::STRIKE, true)); // Strike+
    int old_enemy_hp = up_combat.enemy.hp;
    up_combat.play_card(0);
    assert(up_combat.enemy.hp == old_enemy_hp - 9);
    std::cout << "[PASS] Campfire & Card Upgrades: Card smithing to '+' and Strike+ 9 dmg verified" << std::endl;

    // Test Merchant / Shop & Gold economy
    restored_run.gold = 300;
    restored_run.open_shop();
    assert(restored_run.phase == RunPhase::SHOP);
    assert(restored_run.current_shop.cards.size() == 7);
    assert(restored_run.current_shop.relics.size() == 3);
    prev_gold = restored_run.gold;
    size_t prev_deck_sz = restored_run.master_deck.size();
    bool buy_c_ok = restored_run.buy_shop_card(0);
    assert(buy_c_ok);
    assert(restored_run.gold < prev_gold);
    assert(restored_run.master_deck.size() == prev_deck_sz + 1);

    // Test Card Removal at shop (Base cost 75 Or)
    prev_gold = restored_run.gold;
    prev_deck_sz = restored_run.master_deck.size();
    bool remove_ok = restored_run.remove_card_at_shop(0);
    assert(remove_ok);
    assert(restored_run.gold == prev_gold - 75);
    assert(restored_run.master_deck.size() == prev_deck_sz - 1);
    assert(restored_run.card_removals_count == 1);
    restored_run.leave_shop();
    assert(restored_run.phase == RunPhase::MAP_NAVIGATION);
    std::cout << "[PASS] Merchant: Catalog generation, buying cards/relics and Card Removal service verified" << std::endl;

    // Test Character-Specific Resources & Combat Modifiers
    // 1. Silent Poison
    CombatEngine silent_c;
    silent_c.start_combat(EnemyType::CULTIST, CharacterId::SILENT);
    silent_c.enemy.add_status(StatusType::POISON, 5);
    assert(silent_c.enemy.get_status(StatusType::POISON) == 5);
    int16_t ptick = silent_c.enemy.apply_poison_tick();
    assert(ptick == 5);
    assert(silent_c.enemy.hp == 45); // Direct HP damage (50 -> 45)
    assert(silent_c.enemy.get_status(StatusType::POISON) == 4);

    // 2. Defect Orbs (Lightning channel, passive & evoke)
    CombatEngine defect_c;
    defect_c.start_combat(EnemyType::CULTIST, CharacterId::DEFECT);
    defect_c.channel_orb(OrbType::LIGHTNING);
    assert(defect_c.player.orbs.size() == 1);
    defect_c.trigger_orb_passive(0);
    assert(defect_c.enemies[0].hp == 47); // 3 passive lightning damage
    defect_c.evoke_orb(0);
    assert(defect_c.enemies[0].hp == 39); // 8 evoke lightning damage
    assert(defect_c.player.orbs.empty());

    // 3. Regent Sovereign Stars
    CombatEngine regent_c;
    regent_c.start_combat(EnemyType::CULTIST, CharacterId::REGENT);
    regent_c.player.add_stars(3);
    assert(regent_c.player.sovereign_stars == 3);

    // 4. Necrobinder Souls & Minion (Osty)
    CombatEngine necro_c;
    necro_c.start_combat(EnemyType::CULTIST, CharacterId::NECROBINDER);
    necro_c.player.summon_minion(10, 0); // Summon minion with 10 HP
    assert(necro_c.player.minion_hp == 10);
    necro_c.player.take_damage_with_minion(6); // Enemy hits 6 dmg
    assert(necro_c.player.minion_hp == 4);     // Minion took the 6 damage!
    assert(necro_c.player.hp == 65);          // Necrobinder HP untouched!

    // 5. Ironclad Metallicize
    CombatEngine iron_c;
    iron_c.start_combat(EnemyType::CULTIST, CharacterId::IRONCLAD);
    iron_c.player.add_status(StatusType::METALLICIZE, 4);
    iron_c.player.apply_metallicize_tick();
    assert(iron_c.player.block == 4);
    std::cout << "[PASS] Character Resources: Silent Poison, Defect Orbs, Regent Stars, Necro Minion & Ironclad Metallicize verified" << std::endl;

    // Test Title Screen & Menuing Navigation
    TitleMenuManager menu;
    menu.init();
    assert(menu.current_screen == MenuScreen::TITLE_SCREEN);
    menu.press_confirm(); // Title -> Profile Select
    assert(menu.current_screen == MenuScreen::PROFILE_SELECT);
    menu.press_confirm(); // Select Profile 0 -> Main Menu
    assert(menu.current_screen == MenuScreen::MAIN_MENU);
    menu.press_back();    // Main Menu -> Profile Select
    assert(menu.current_screen == MenuScreen::PROFILE_SELECT);
    std::cout << "[PASS] Title Screen & Menuing: Splash, Profile Select, Main Menu navigation verified" << std::endl;

    // Test MapNavigator: D-Pad floor scrolling, route inspection & Deck viewing
    RunManager nav_run;
    nav_run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::SHINY, 1001);
    MapNavigator nav(nav_run);

    assert(nav.scroll_floor == 0);
    nav.scroll_up();
    assert(nav.scroll_floor == 1);
    nav.scroll_up();
    assert(nav.scroll_floor == 2);
    nav.scroll_down();
    assert(nav.scroll_floor == 1);

    // Starting routes
    auto routes = nav.get_available_routes();
    assert(routes.size() >= 2); // the first floor always offers a choice
    assert(nav.get_selected_target_node() == routes[0]);
    nav.next_route();
    assert(nav.get_selected_target_node() == routes[1]);
    nav.prev_route();
    assert(nav.get_selected_target_node() == routes[0]);

    // Enter chosen room
    bool entered = nav.select_current_route();
    assert(entered);
    assert(nav_run.current_floor == 1);
    assert(nav_run.current_node_index == routes[0]);

    // Test Deck Viewing overlay
    assert(!nav.deck_view_active);
    nav.toggle_deck_view();
    assert(nav.deck_view_active);
    assert(nav.deck_scroll_index == 0);
    nav.deck_scroll_down();
    assert(nav.deck_scroll_index == 1);
    nav.deck_scroll_up();
    assert(nav.deck_scroll_index == 0);
    nav.toggle_deck_view();
    assert(!nav.deck_view_active);
    std::cout << "[PASS] Map Navigation & Deck Viewer: Floor scrolling, route inspection & Deck view verified" << std::endl;

    // Test Pokémon Battle UI: 4-Way Menu, Moves, Animations & Layout
    PokemonBattleUI pui;
    pui.init(CharacterId::IRONCLAD, PaletteVariant::SHINY);
    assert(pui.current_tab == BattleMenuTab::MAIN_MENU);
    assert(pui.selected_option == MainMenuOption::CARDS);

    // 4-Way navigation
    pui.cursor_right();
    assert(pui.selected_option == MainMenuOption::BAG);
    pui.cursor_down();
    assert(pui.selected_option == MainMenuOption::END_TURN);
    pui.cursor_left();
    assert(pui.selected_option == MainMenuOption::DECK);
    pui.cursor_up();
    assert(pui.selected_option == MainMenuOption::CARDS);

    // Open CARDS submenu (Move selection)
    CombatEngine p_combat;
    p_combat.start_combat(EnemyType::CULTIST, CharacterId::IRONCLAD);
    pui.button_a_pressed(p_combat, &nav_run);
    assert(pui.current_tab == BattleMenuTab::CARDS_MENU);

    // Play card (Strike)
    int cultist_initial_hp = p_combat.enemy.hp;
    bool card_played = pui.button_a_pressed(p_combat, &nav_run);
    assert(card_played);
    assert(p_combat.enemy.hp < cultist_initial_hp);
    assert(pui.anim.player_attack_timer > 0);
    assert(pui.anim.enemy_hurt_timer > 0);
    assert(pui.anim.last_damage_dealt == (cultist_initial_hp - p_combat.enemy.hp));

    // Cancel back to main menu
    pui.button_b_pressed();
    assert(pui.current_tab == BattleMenuTab::MAIN_MENU);

    // Test HP Bar formatting & Hero resource labels
    char hp_bar[16];
    PokemonBattleUI::format_hp_bar(40, 80, hp_bar, 10);
    assert(std::string_view(hp_bar) == "[=====-----]");

    char res_label[48];
    PokemonBattleUI::get_hero_resource_label(p_combat.player, CharacterId::IRONCLAD, res_label, sizeof(res_label));
    assert(std::string_view(res_label).find("STR") != std::string_view::npos);

    Player silent_pl;
    PokemonBattleUI::get_hero_resource_label(silent_pl, CharacterId::SILENT, res_label, sizeof(res_label));
    assert(std::string_view(res_label) == "POISON");

    Player defect_pl;
    defect_pl.orbs.push_back(OrbType::LIGHTNING);
    PokemonBattleUI::get_hero_resource_label(defect_pl, CharacterId::DEFECT, res_label, sizeof(res_label));
    assert(std::string_view(res_label) == "ORBES: 1/3");

    Player regent_pl;
    regent_pl.add_stars(4);
    PokemonBattleUI::get_hero_resource_label(regent_pl, CharacterId::REGENT, res_label, sizeof(res_label));
    assert(std::string_view(res_label) == "ETOILES: 4");

    Player necro_pl;
    necro_pl.summon_minion(12, 0);
    PokemonBattleUI::get_hero_resource_label(necro_pl, CharacterId::NECROBINDER, res_label, sizeof(res_label));
    assert(std::string_view(res_label).find("OSTY: 12 PV") != std::string_view::npos);
    std::cout << "[PASS] Pokémon Battle UI: 4-Way Menu, Move Selection, Attack feedback, HP bar & Resource HUD verified" << std::endl;

    // Test Event Manager: Encounters, Choices & Outcomes
    RunManager ev_run;
    ev_run.start_run(CharacterId::IRONCLAD, 0, PaletteVariant::CLASSIC, 42);
    EventEncounter enc = EventManager::get_encounter_for_floor(3, 42);
    assert(!enc.title.empty());
    assert(enc.choices.size() >= 2);

    ev_run.hp = 50;
    int hp_before_ev = ev_run.hp;
    int gold_before_ev = ev_run.gold;
    char outcome_msg[128];

    // Pick choice 0 (Gain gold / Heal / etc.)
    EventManager::apply_choice(ev_run, enc.choices[0], outcome_msg, sizeof(outcome_msg));
    assert(outcome_msg[0] != '\0');
    assert(ev_run.gold != gold_before_ev || ev_run.hp != hp_before_ev);
    std::cout << "[PASS] Pokémon-style Events: Encounters, Choice Dialogues, and Run State Modifications verified" << std::endl;

    // Act 1 boss: opens with a buff, then attacks; each enemy action is recorded for replay.
    CombatEngine boss_combat;
    boss_combat.start_combat(EnemyType::CEREMONIAL_BEAST, CharacterId::IRONCLAD);
    assert(boss_combat.enemies[0].max_hp == 150);
    assert(boss_combat.enemies[0].current_intent.type == EnemyIntentType::BUFF);
    boss_combat.end_player_turn();
    assert(boss_combat.last_enemy_actions.size() == 1);
    assert(boss_combat.last_enemy_actions[0].intent_type == EnemyIntentType::BUFF);
    assert(boss_combat.last_enemy_actions[0].damage_to_player == 0);
    assert(boss_combat.enemies[0].get_status(StatusType::STRENGTH) == 2);
    int boss_hp_before = boss_combat.player.hp;
    boss_combat.end_player_turn();
    const auto& trample = boss_combat.last_enemy_actions[0];
    assert(trample.intent_type == EnemyIntentType::ATTACK);
    assert(trample.damage_to_player + trample.damage_blocked == 14);
    assert(trample.player_hp_after == boss_combat.player.hp);
    assert(boss_combat.player.hp == boss_hp_before - trample.damage_to_player);
    std::cout << "[PASS] Ceremonial Beast boss & per-enemy action records verified" << std::endl;

    test_cartridge_save();

    std::cout << "=== All Engine Tests Passed Successfully! ===" << std::endl;
    return 0;
}
