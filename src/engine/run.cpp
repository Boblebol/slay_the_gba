#include "engine/run.h"
#include "engine/relic.h"
#include "engine/encounter.h"
#include "engine/rewards.h"
#include "engine/potion.h"
#include "engine/ancient.h"
#include <algorithm>

namespace engine {

namespace {

// Campfire actions given by a relic, in the order of the menu.
struct CampfireRelic {
    std::string_view relic_id;
    CampfireAction action;
};

constexpr CampfireRelic CAMPFIRE_RELICS[] = {
    { "SHOVEL", CampfireAction::DIG },
    { "GIRYA", CampfireAction::LIFT },
    { "MEAT_CLEAVER", CampfireAction::COOK }
};

constexpr int MEMBERSHIP_CARD_DISCOUNT = 50;
constexpr int THE_COURIER_DISCOUNT = 20;
constexpr int COOK_REMOVALS = 2; // cards removed by Cook (Meat Cleaver)
constexpr int COOK_MAX_HP = 5;   // Max HP given by Cook
constexpr int LAVA_ROCK_RELICS = 2; // relics the Act 1 Boss gives to Lava Rock when defeated
constexpr uint8_t GIRYA_LIFTS = 3;  // most Strength Girya gives, one lift each

constexpr AscensionModifier ASCENSION_MODIFIERS[] = {
    { 0, "Normal", "Difficulté standard de l'Ascension." },
    { 1, "Élites Renforcés", "Les monstres Élites apparaissent plus fréquemment sur la carte." },
    { 2, "Monstres Meurtris", "Les attaques des monstres normaux infligent plus de dégâts." },
    { 3, "Élites Féroces", "Les attaques des monstres Élites infligent plus de dégâts." },
    { 4, "Boss Impitoyables", "Les attaques des Boss infligent plus de dégâts." },
    { 5, "Soin Réduit", "Vous ne récupérez que 75% de vos PV perdus après un Boss." },
    { 6, "Départ Éprouvé", "Vous commencez la tentative avec 10% de PV en moins." },
    { 7, "Monstres Robustes", "Les monstres normaux ont plus de PV." },
    { 8, "Élites Robustes", "Les monstres Élites ont plus de PV." },
    { 9, "Boss Colossaux", "Les Boss ont plus de PV." },
    { 10, "Fléau de l'Ascension", "Vous commencez la tentative avec une malédiction inépuisable dans votre deck." }
};

// Hand-written search: string_view::find calls memchr and substr can throw, neither links in the GBA build.
bool contains(std::string_view text, std::string_view word) {
    for (size_t start = 0; start + word.size() <= text.size(); ++start) {
        size_t i = 0;
        while (i < word.size() && text[start + i] == word[i]) ++i;
        if (i == word.size()) return true;
    }
    return false;
}

// Strikes and Defends, by their English name (Perfected Strike, Ultimate Defend...).
bool is_strike_or_defend(CardId id) {
    const std::string_view name = CardDef::get(id).name;
    return contains(name, "Strike") || contains(name, "Defend");
}

} // namespace

uint32_t RunManager::next_random() {
    rng_state_ ^= rng_state_ << 13;
    rng_state_ ^= rng_state_ >> 17;
    rng_state_ ^= rng_state_ << 5;
    return rng_state_;
}

void RunManager::start_run(CharacterId char_id, uint8_t ascension, PaletteVariant pal, uint32_t seed) {
    character = char_id;
    ascension_level = std::min<uint8_t>(ascension, 10);
    palette = pal;
    current_act = 1;
    current_floor = 1;
    current_node_index = -1;
    current_encounter = {};
    cook_pending = false;
    rng_state_ = seed ? seed : 12345;

    const auto& cdef = get_character(character);
    max_hp = cdef.starting_hp;
    hp = max_hp;
    gold = cdef.starting_gold;

    // Ascension 6 handicap: start damaged
    if (ascension_level >= 6) {
        hp = (max_hp * 9) / 10;
    }

    // Build Master Deck from Starter Deck
    master_deck.clear();
    for (const auto& item : cdef.starter_deck) {
        CardId cid = CardId::STRIKE;
        if (item.card_id == "DEFEND_IRONCLAD" || item.card_id == "DEFEND_SILENT" ||
            item.card_id == "DEFEND_DEFECT" || item.card_id == "DEFEND_REGENT" ||
            item.card_id == "DEFEND_NECROBINDER") cid = CardId::DEFEND;
        else if (item.card_id == "BASH") cid = CardId::BASH;
        else if (item.card_id == "NEUTRALIZE") cid = CardId::NEUTRALIZE;
        else if (item.card_id == "SURVIVOR") cid = CardId::SURVIVOR;
        else if (item.card_id == "ZAP") cid = CardId::ZAP;
        else if (item.card_id == "DUALCAST") cid = CardId::DUALCAST;
        else if (item.card_id == "FALLING_STAR") cid = CardId::FALLING_STAR;
        else if (item.card_id == "VENERATE") cid = CardId::VENERATE;
        else if (item.card_id == "UNLEASH") cid = CardId::UNLEASH;
        else if (item.card_id == "BODYGUARD") cid = CardId::BODYGUARD;

        for (uint8_t i = 0; i < item.count; ++i) {
            master_deck.push_back(cid);
        }
    }

    // Starting relic
    relics.clear();
    relics.push_back(cdef.starting_relic_id);
    potions.clear();
    potion_slots = BASE_POTION_SLOTS;
    deck_choice = {};
    relic_counters.clear();
    for (uint16_t& runs : hook_runs) runs = 0;

    // Generate Act 1 Map
    map_seed = next_random();
    map.generate(current_act, map_seed, ascension_level);
    phase = RunPhase::MAP_NAVIGATION;
}

FixedVector<int, 3> RunManager::get_available_rooms() const {
    if (current_node_index < 0) {
        return map.get_starting_nodes();
    }
    return map.get_next_nodes(current_node_index);
}

FixedVector<EnemyType, 3> RunManager::draw_encounter(RoomType room) {
    std::span<const Encounter> table = encounter_table(act_zone(current_act, map_seed),
                                                       encounter_tier(room, current_floor));
    // A list of one draws nothing, as the rooms with a single choice did before the tables.
    // The high bits: the low ones of two draws in a row are too alike, and would tie the monsters drawn
    // from a pool to the encounter picked.
    const Encounter& encounter = table.size() == 1 ? table[0] : table[(next_random() >> 16) % table.size()];
    current_encounter = encounter.id;

    FixedVector<EnemyType, 3> monsters;
    if (encounter.random_pool.empty()) {
        for (uint8_t i = 0; i < encounter.count && i < 3; ++i) {
            monsters.push_back(encounter.monsters[i]);
        }
        return monsters;
    }

    // Different monsters drawn from the pool: the first steps of a Fisher-Yates shuffle.
    FixedVector<EnemyType, 8> pool;
    for (EnemyType type : encounter.random_pool) {
        pool.push_back(type);
    }
    for (size_t i = 0; i < encounter.count && i < pool.size(); ++i) {
        size_t pick = i + next_random() % (pool.size() - i);
        std::swap(pool[i], pool[pick]);
        monsters.push_back(pool[i]);
    }
    return monsters;
}

void RunManager::enter_room(int node_index) {
    current_node_index = node_index;
    const auto& node = map.nodes[node_index];
    current_floor = node.floor + 1;
    on_room_entered(node.type);

    switch (node.type) {
        case RoomType::MONSTER:
        case RoomType::ELITE:
        case RoomType::BOSS: {
            phase = RunPhase::IN_COMBAT;
            FixedVector<EnemyType, 3> etypes = draw_encounter(node.type);

            // Start combat with current HP and master deck
            combat.start_combat_multi(etypes, character);
            combat.player.hp = hp;
            combat.player.max_hp = max_hp;

            // Ascension: tougher and harder-hitting enemies depending on the room.
            int16_t hp_percent = get_enemy_hp_multiplier(node.type);
            int16_t damage_percent = get_enemy_damage_multiplier(node.type);
            for (Enemy& foe : combat.enemies) {
                if (foe.ascension_hp() > 0) {
                    // A monster with its own Ascension values takes them instead of the percentages.
                    foe.ascend(hp_percent > 100, damage_percent > 100);
                    continue;
                }
                foe.max_hp = static_cast<int16_t>(foe.max_hp * hp_percent / 100);
                foe.hp = foe.max_hp;
                foe.damage_percent = damage_percent;
            }
            combat.plan_enemies(); // the moves follow the Ascension values just given

            // Replace combat piles with current master deck
            combat.draw_pile.clear();
            combat.hand.clear();
            combat.discard_pile.clear();
            combat.exhaust_pile.clear();
            for (size_t i = 0; i < master_deck.size(); ++i) {
                DeckCard card = master_deck[i];
                card.deck_index = static_cast<uint8_t>(i);
                // Ghost Seed: the Strikes and Defends of the fight are Ethereal.
                if (has_relic("GHOST_SEED") && is_strike_or_defend(card.id)) card.added_keywords |= KEYWORD_ETHEREAL;
                combat.draw_pile.push_back(card);
            }
            combat.shuffle_draw_pile();
            combat.reset_combat_state();
            register_relic_powers();
            combat.begin_first_turn();
            apply_combat_start_relics();
            break;
        }
        case RoomType::REST_SITE:
            phase = RunPhase::CAMPFIRE;
            break;
        case RoomType::TREASURE: {
            gain_gold(50);
            last_treasure_relic = draw_treasure_relic();
            if (!last_treasure_relic.empty()) {
                add_relic(last_treasure_relic);
            }
            phase = RunPhase::TREASURE;
            break;
        }
        case RoomType::SHOP:
            open_shop();
            break;
        case RoomType::EVENT:
            trigger_event();
            phase = RunPhase::EVENT;
            break;
        default:
            break;
    }
}

FixedVector<CampfireAction, 5> RunManager::campfire_actions() const {
    FixedVector<CampfireAction, 5> actions;
    actions.push_back(CampfireAction::REST);
    actions.push_back(CampfireAction::SMITH);
    for (const CampfireRelic& entry : CAMPFIRE_RELICS) {
        if (has_relic(entry.relic_id)) actions.push_back(entry.action);
    }
    return actions;
}

void RunManager::rest_at_campfire() {
    if (can_rest()) {
        int16_t heal_amount = max_hp * 3 / 10;
        // Regal Pillow: each Rest heals 15 more HP.
        if (has_relic("REGAL_PILLOW")) heal_amount += 15;
        hp = std::min<int16_t>(max_hp, hp + heal_amount);
    }
    on_rested();
    phase = RunPhase::MAP_NAVIGATION;
}

bool RunManager::can_rest() const {
    return hp < max_hp;
}

bool RunManager::smith_at_campfire(int card_index) {
    if (upgrade_card_in_deck(card_index)) {
        phase = RunPhase::MAP_NAVIGATION;
        return true;
    }
    return false;
}

bool RunManager::cook_at_campfire() {
    if (!has_relic("MEAT_CLEAVER")) return false;
    request_deck_choice(DeckChoiceKind::REMOVE, COOK_REMOVALS);
    cook_pending = true;
    finish_cook(); // no removable card: the Max HP comes at once
    phase = RunPhase::MAP_NAVIGATION;
    return true;
}

bool RunManager::lift_at_campfire() {
    if (!has_relic("GIRYA") || relic_counter("GIRYA") >= GIRYA_LIFTS) return false;
    set_relic_counter("GIRYA", static_cast<uint8_t>(relic_counter("GIRYA") + 1));
    phase = RunPhase::MAP_NAVIGATION;
    return true;
}

void RunManager::finish_cook() {
    if (!cook_pending || has_deck_choice()) return;
    cook_pending = false;
    gain_max_hp(COOK_MAX_HP);
}

bool RunManager::dig_at_campfire() {
    if (!has_relic("SHOVEL")) return false;
    if (gain_random_relics(1) == 0) return false;
    phase = RunPhase::MAP_NAVIGATION;
    return true;
}

bool RunManager::can_smith() const {
    for (size_t i = 0; i < master_deck.size(); ++i) {
        if (!master_deck[i].upgraded) return true;
    }
    return false;
}

bool RunManager::upgrade_card_in_deck(int deck_index) {
    if (deck_index >= 0 && static_cast<size_t>(deck_index) < master_deck.size()) {
        if (!master_deck[deck_index].upgraded) {
            master_deck[deck_index].upgraded = true;
            return true;
        }
    }
    return false;
}

bool RunManager::remove_card_from_deck(int deck_index) {
    if (deck_index >= 0 && static_cast<size_t>(deck_index) < master_deck.size()) {
        if (CardDef::is_eternal(master_deck[deck_index])) return false;
        master_deck.erase(deck_index);
        return true;
    }
    return false;
}

void RunManager::open_shop() {
    current_shop.generate(character, next_random(), card_removals_count,
                          std::span<const std::string_view>(relics.begin(), relics.size()));
    // Relics already owned are shown as sold out: a relic is never bought twice.
    for (size_t i = 0; i < current_shop.relics.size(); ++i) {
        if (has_relic(current_shop.relics[i].relic_id)) {
            current_shop.relics[i].purchased = true;
        }
    }
    phase = RunPhase::SHOP;
    on_shop_opened();
}

bool RunManager::buy_shop_card(int index) {
    if (!current_shop.buy_card(*this, index)) return false;
    // The Courier: the card bought is back on the shelf at once.
    if (has_relic("THE_COURIER")) current_shop.cards[index].purchased = false;
    return true;
}

bool RunManager::buy_shop_relic(int index) {
    if (!current_shop.buy_relic(*this, index)) return false;
    // The Courier: the relic bought is replaced by another one, priced after the discounts in force.
    if (has_relic("THE_COURIER")) {
        RewardRng rng(next_random());
        FixedVector<std::string_view, 3> stocked;
        for (const ShopRelic& item : current_shop.relics) {
            if (&item != &current_shop.relics[index]) stocked.push_back(item.relic_id);
        }
        std::string_view id = draw_relic(rng, character, RelicSource::SHOP, std::span<const std::string_view>(relics.begin(), relics.size()),
                                         std::span<const std::string_view>(stocked.begin(), stocked.size()));
        if (!id.empty()) {
            int cost = vary_price(rng, relic_base_price(find_relic(id)->rarity));
            cost = (cost * (100 - THE_COURIER_DISCOUNT) + 50) / 100;
            if (has_relic("MEMBERSHIP_CARD")) cost = (cost * (100 - MEMBERSHIP_CARD_DISCOUNT) + 50) / 100;
            current_shop.relics[index] = ShopRelic{ id, static_cast<int16_t>(cost), false };
        }
    }
    return true;
}

bool RunManager::buy_shop_potion(int index, int replace_index) {
    if (!current_shop.buy_potion(*this, index, replace_index)) return false;
    // The Courier: the potion bought is replaced by another one, priced after the discounts in force.
    if (has_relic("THE_COURIER")) {
        RewardRng rng(next_random());
        FixedVector<std::string_view, 3> stocked;
        for (const ShopPotion& item : current_shop.potions) {
            if (&item != &current_shop.potions[index]) stocked.push_back(item.potion_id);
        }
        std::string_view id = draw_potion(rng, character, std::span<const std::string_view>(stocked.begin(), stocked.size()));
        if (!id.empty()) {
            int cost = vary_price(rng, potion_base_price(find_potion(id)->rarity));
            cost = (cost * (100 - THE_COURIER_DISCOUNT) + 50) / 100;
            if (has_relic("MEMBERSHIP_CARD")) cost = (cost * (100 - MEMBERSHIP_CARD_DISCOUNT) + 50) / 100;
            current_shop.potions[index] = ShopPotion{ id, static_cast<int16_t>(cost), false };
        }
    }
    return true;
}

bool RunManager::use_potion(int index, int target_index) {
    if (phase != RunPhase::IN_COMBAT) return false;
    if (index < 0 || static_cast<size_t>(index) >= potions.size()) return false;
    if (!combat.use_potion(potions[index], target_index)) return false;
    potions.erase(index);
    refresh_belt_buckle();
    return true;
}

bool RunManager::remove_card_at_shop(int deck_index) {
    return current_shop.remove_card(*this, deck_index);
}

void RunManager::leave_shop() {
    phase = RunPhase::MAP_NAVIGATION;
}

void RunManager::trigger_event() {
    current_event = EventManager::get_encounter_for_floor(current_floor, next_random());
}

bool RunManager::choose_event_choice(int choice_index, char* out_msg, size_t max_len) {
    if (choice_index >= 0 && static_cast<size_t>(choice_index) < current_event.choices.size()) {
        EventManager::apply_choice(*this, current_event.choices[choice_index], out_msg, max_len);
        phase = RunPhase::MAP_NAVIGATION;
        return true;
    }
    return false;
}

std::span<const CardId> RunManager::card_reward_pool(CharacterId char_id) {
    // Left out on purpose: Cleave (replaced by Thunderclap, whose art and name it borrowed) and Break (Ancient
    // rarity: given by an Ancient, never by a combat).
    static constexpr CardId IRONCLAD_POOL[] = { CardId::IRON_WAVE, CardId::ANGER, CardId::TWIN_STRIKE,
                                                CardId::POMMEL_STRIKE, CardId::SHRUG_IT_OFF, CardId::TAUNT,
                                                CardId::BODY_SLAM, CardId::THUNDERCLAP, CardId::DISMANTLE,
                                                CardId::INFLAME, CardId::PERFECTED_STRIKE, CardId::BLUDGEON,
                                                CardId::BULLY, CardId::SWORD_BOOMERANG, CardId::CONFLAGRATION,
                                                CardId::EXPECT_A_FIGHT, CardId::PILLAGE, CardId::UPPERCUT,
                                                CardId::FIGHT_ME, CardId::ASHEN_STRIKE, CardId::TREMBLE,
                                                CardId::EVIL_EYE, CardId::MOLTEN_FIST, CardId::CINDER,
                                                CardId::SECOND_WIND, CardId::TRUE_GRIT, CardId::DOMINATE,
                                                CardId::FIEND_FIRE, CardId::THRASH, CardId::IMPERVIOUS,
                                                CardId::PACTS_END, CardId::NOT_YET, CardId::BLOOD_WALL,
                                                CardId::BLOODLETTING, CardId::BREAKTHROUGH, CardId::FORGOTTEN_RITUAL,
                                                CardId::HEMOKINESIS, CardId::MANGLE, CardId::OFFERING,
                                                CardId::SETUP_STRIKE, CardId::SPITE, CardId::WHIRLWIND, CardId::FEED,
                                                CardId::BATTLE_TRANCE, CardId::FEEL_NO_PAIN, CardId::DARK_EMBRACE,
                                                CardId::DRUM_OF_BATTLE, CardId::BARRICADE, CardId::INFERNO,
                                                CardId::CRIMSON_MANTLE, CardId::RUPTURE, CardId::AGGRESSION,
                                                CardId::PYRE, CardId::DEMON_FORM, CardId::JUGGERNAUT, CardId::RAGE,
                                                CardId::VICIOUS, CardId::HOWL_FROM_BEYOND, CardId::HELLRAISER,
                                                CardId::STAMPEDE, CardId::CASCADE, CardId::ONE_TWO_PUNCH,
                                                CardId::TEAR_ASUNDER, CardId::HAVOC, CardId::UNRELENTING,
                                                CardId::MIDNIGHT, CardId::STOMP, CardId::RAMPAGE, CardId::ARMAMENTS,
                                                CardId::BRAND, CardId::BURNING_PACT,
                                                CardId::JUGGLING,
                                                CardId::HEADBUTT,
                                                CardId::STONE_ARMOR, CardId::FLAME_BARRIER, CardId::UNMOVABLE, CardId::COLOSSUS, CardId::CRUELTY };
    static constexpr CardId SILENT_POOL[] = { CardId::POISON_STAB, CardId::DEADLY_POISON, CardId::SURVIVOR,
                                              CardId::SLICE, CardId::BACKFLIP, CardId::SUCKER_PUNCH,
                                              CardId::DAGGER_SPRAY, CardId::DEFLECT, CardId::LEG_SWEEP, CardId::HAZE,
                                              CardId::PRECISE_CUT, CardId::BOUNCING_FLASK, CardId::FLECHETTES,
                                              CardId::FOOTWORK, CardId::ESCAPE_PLAN, CardId::DASH,
                                              CardId::BUBBLE_BUBBLE, CardId::OUTBREAK, CardId::ADRENALINE,
                                              CardId::CALCULATED_GAMBLE, CardId::MIRAGE, CardId::ANTICIPATE,
                                              CardId::ASSASSINATE, CardId::BACKSTAB, CardId::BLUR,
                                              CardId::DODGE_AND_ROLL, CardId::FINISHER, CardId::MALAISE,
                                              CardId::MEMENTO_MORI, CardId::MURDER, CardId::PIERCING_WAIL,
                                              CardId::PREDATOR, CardId::SIDESTEP, CardId::SKEWER, CardId::EXPERTISE,
                                              CardId::SNAKEBITE, CardId::ECHOING_SLASH, CardId::GRAND_FINALE,
                                              CardId::ACCELERANT, CardId::SPEEDSTER, CardId::ENVENOM,
                                              CardId::SERPENT_FORM, CardId::AFTERIMAGE, CardId::WELL_LAID_PLANS,
                                              CardId::STRANGLE, CardId::CORROSIVE_WAVE, CardId::NOXIOUS_FUMES,
                                              CardId::BURST, CardId::BULLET_TIME, CardId::PINPOINT, CardId::POUNCE,
                                              CardId::BLADE_DANCE, CardId::CLOAK_AND_DAGGER, CardId::LEADING_STRIKE,
                                              CardId::BLADE_OF_INK, CardId::INFINITE_BLADES, CardId::STORM_OF_STEEL,
                                              CardId::FAN_OF_KNIVES, CardId::PHANTOM_BLADES, CardId::ACCURACY,
                                              CardId::UP_MY_SLEEVE, CardId::KNIFE_TRAP, CardId::ACROBATICS,
                                              CardId::DAGGER_THROW, CardId::PREPARED, CardId::HIDDEN_DAGGERS,
                                              CardId::EXPOSE, CardId::TACTICIAN, CardId::ABRASIVE,
                                              CardId::MASTER_PLANNER, CardId::HAND_TRICK, CardId::UNTOUCHABLE,
                                              CardId::RICOCHET, CardId::REFLEX, CardId::FLICK_FLACK, CardId::SHADOWMELD, CardId::TRACKING, CardId::SHADOW_STEP, CardId::THE_HUNT };
    static constexpr CardId DEFECT_POOL[] = { CardId::ZAP, CardId::DUALCAST, CardId::BEAM_CELL,
                                              CardId::BALL_LIGHTNING, CardId::COLD_SNAP, CardId::LEAP,
                                              CardId::COOLHEADED, CardId::SWEEPING_BEAM, CardId::SHADOW_SHIELD,
                                              CardId::GLACIER, CardId::METEOR_STRIKE, CardId::ICE_LANCE,
                                              CardId::SKIM, CardId::ALL_FOR_ONE, CardId::NULL_CARD,
                                              CardId::GO_FOR_THE_EYES, CardId::SCRAPE, CardId::RAINBOW,
                                              CardId::CHILL, CardId::FUSION, CardId::REBOOT, CardId::SUPERCRITICAL,
                                              CardId::DOUBLE_ENERGY, CardId::BOOT_SEQUENCE, CardId::CHARGE_BATTERY,
                                              CardId::FTL, CardId::HELIX_DRILL, CardId::LIGHTNING_ROD,
                                              CardId::TEMPEST, CardId::SUNDER, CardId::MACHINE_LEARNING,
                                              CardId::STORM, CardId::FERAL, CardId::SUBROUTINE, CardId::SHATTER,
                                              CardId::VOLTAIC, CardId::BARRAGE, CardId::TESLA_COIL, CardId::CHAOS,
                                              CardId::COMPILE_DRIVER, CardId::DARKNESS, CardId::MULTI_CAST,
                                              CardId::LOOP, CardId::CONSUMING_SHADOW, CardId::HAILSTORM,
                                              CardId::THUNDER, CardId::COOLANT, CardId::HOTFIX, CardId::DEFRAGMENT,
                                              CardId::FOCUSED_STRIKE, CardId::HYPERBEAM, CardId::CAPACITOR,
                                              CardId::BULK_UP, CardId::SYNCHRONIZE, CardId::ECHO_FORM,
                                              CardId::SIGNAL_BOOST, CardId::UPROAR, CardId::ADAPTIVE_STRIKE,
                                              CardId::SYNTHESIS, CardId::GENETIC_ALGORITHM, CardId::MOMENTUM_STRIKE,
                                              CardId::CLAW, CardId::MODDED, CardId::BOOST_AWAY, CardId::FLAK_CANNON,
                                              CardId::SMOKESTACK, CardId::COMPACT, CardId::GUNK_UP,
                                              CardId::ITERATION, CardId::FIGHT_THROUGH, CardId::OVERCLOCK,
                                              CardId::ROCKET_PUNCH, CardId::TRASH_TO_TREASURE, CardId::TURBO,
                                              CardId::SCAVENGE,
                                              CardId::HOLOGRAM,
                                              CardId::BUFFER };
    static constexpr CardId REGENT_POOL[] = { CardId::FALLING_STAR, CardId::VENERATE, CardId::SOLAR_STRIKE,
                                              CardId::GATHER_LIGHT, CardId::CELESTIAL_MIGHT, CardId::CRUSH_UNDER,
                                              CardId::GLITTERSTREAM, CardId::HEAVENLY_DRILL, CardId::HEGEMONY,
                                              CardId::KNOW_THY_PLACE, CardId::LUNAR_BLAST, CardId::PROPHESIZE,
                                              CardId::SUPERMASSIVE, CardId::CONVERGENCE, CardId::ALIGNMENT,
                                              CardId::ASTRAL_PULSE, CardId::CLOAK_OF_STARS, CardId::COMET,
                                              CardId::CRESCENT_SPEAR, CardId::DEVASTATE, CardId::DYING_STAR,
                                              CardId::GAMMA_BLAST, CardId::GLOW, CardId::GUIDING_STAR,
                                              CardId::HIDDEN_CACHE, CardId::PARTICLE_WALL, CardId::RADIATE,
                                              CardId::RESONANCE, CardId::ROYAL_GAMBLE, CardId::SEVEN_STARS,
                                              CardId::SHINING_STRIKE, CardId::STARDUST, CardId::KNOCKOUT_BLOW,
                                              CardId::CHILD_OF_THE_STARS, CardId::GENESIS, CardId::BLACK_HOLE,
                                              CardId::MAKE_IT_SO, CardId::PALE_BLUE_DOT, CardId::ARSENAL,
                                              CardId::MONOLOGUE, CardId::ORBIT, CardId::PILLAR_OF_CREATION,
                                              CardId::MONARCHS_GAZE, CardId::BOMBARDMENT, CardId::I_AM_INVINCIBLE,
                                              CardId::VOID_FORM, CardId::KINGLY_KICK, CardId::KINGLY_PUNCH,
                                              CardId::COLLISION_COURSE, CardId::CRASH_LANDING, CardId::PHOTON_CUT,
                                              CardId::GLIMMER, CardId::TYRANNY,
                                              CardId::COSMIC_INDIFFERENCE,
                                              CardId::FOREGONE_CONCLUSION,
                                              CardId::NEUTRON_AEGIS, CardId::REFLECT, CardId::PATTER,
                                              CardId::TERRAFORMING,
                                              CardId::REFINE_BLADE, CardId::SPOILS_OF_BATTLE,
                                              CardId::WROUGHT_IN_WAR, CardId::BULWARK, CardId::CONQUEROR,
                                              CardId::FURNACE, CardId::PARRY, CardId::SUMMON_FORTH,
                                              CardId::BEAT_INTO_SHAPE, CardId::BIG_BANG, CardId::SEEKING_EDGE,
                                              CardId::SWORD_SAGE, CardId::THE_SMITH,
                                              CardId::MANIFEST_AUTHORITY, CardId::ROYALTIES };
    static constexpr CardId NECROBINDER_POOL[] = { CardId::UNLEASH, CardId::BODYGUARD, CardId::PULL_AGGRO,
                                                   CardId::DRAIN_POWER, CardId::BURY, CardId::MISERY,
                                                   CardId::UNDEATH, CardId::SHARED_FATE, CardId::WISP,
                                                   CardId::PUTREFY, CardId::REANIMATE, CardId::AFTERLIFE,
                                                   CardId::DEATH_MARCH, CardId::DEFILE, CardId::DEFY, CardId::DELAY,
                                                   CardId::ENFEEBLING_TOUCH, CardId::FEAR, CardId::INVOKE,
                                                   CardId::PARSE, CardId::PULL_FROM_BELOW, CardId::REAP, CardId::SOW,
                                                   CardId::ERADICATE, CardId::FRIENDSHIP, CardId::DANSE_MACABRE,
                                                   CardId::DEMESNE, CardId::SPIRIT_OF_ASH, CardId::PAGESTORM,
                                                   CardId::SLEIGHT_OF_FLESH, CardId::NEGATIVE_PULSE,
                                                   CardId::END_OF_DAYS, CardId::BLIGHT_STRIKE, CardId::NO_ESCAPE,
                                                   CardId::SCOURGE, CardId::DEATHBRINGER, CardId::DEATHS_DOOR,
                                                   CardId::COUNTDOWN, CardId::REAPER_FORM, CardId::NEUROSURGE,
                                                   CardId::TIMES_UP, CardId::OBLIVION, CardId::SHROUD,
                                                   CardId::EIDOLON, CardId::BANSHEES_CRY, CardId::VEILPIERCER,
                                                   CardId::BORROWED_TIME, CardId::THE_SCYTHE, CardId::MELANCHOLY,
                                                   CardId::HANG, CardId::SCULPTING_STRIKE,
                                                   CardId::GRAVEBLAST,
                                                   CardId::DREDGE,
                                                   CardId::CLEANSE,
                                                   CardId::POKE, CardId::SNAP, CardId::RATTLE, CardId::BONE_SHARDS,
                                                   CardId::SQUEEZE, CardId::FLATTEN, CardId::HIGH_FIVE, CardId::FETCH,
                                                   CardId::CALCIFY, CardId::RIGHT_HAND_HAND, CardId::NECRO_MASTERY,
                                                   CardId::SACRIFICE, CardId::SPUR, CardId::SIC_EM,
                                                   CardId::SENTRY_MODE,
                                                   CardId::REAVE, CardId::GRAVE_WARDEN, CardId::CAPTURE_SPIRIT,
                                                   CardId::SEVERANCE, CardId::DIRGE, CardId::HAUNT,
                                                   CardId::DEVOUR_LIFE, CardId::SOUL_STORM,
                                                   CardId::GLIMPSE_BEYOND, CardId::LETHALITY, CardId::DEBILITATE, CardId::FORBIDDEN_GRIMOIRE };

    switch (char_id) {
        case CharacterId::SILENT: return SILENT_POOL;
        case CharacterId::DEFECT: return DEFECT_POOL;
        case CharacterId::REGENT: return REGENT_POOL;
        case CharacterId::NECROBINDER: return NECROBINDER_POOL;
        default: return IRONCLAD_POOL;
    }
}

uint32_t RunManager::reward_seed() const {
    return map_seed ^ (current_act * 0x9E3779B1u) ^ (current_floor * 0x85EBCA6Bu) ^
           (static_cast<uint32_t>(current_node_index + 1) * 0xC2B2AE35u);
}

void RunManager::draw_card_rewards(bool boss, RewardRng& rng, CardRarity rarity) {
    // Offer distinct cards drawn at random from the hero's pool, of `rarity` when one is asked.
    current_card_rewards.clear();
    FixedVector<CardId, 512> candidates;
    auto keep = [rarity](CardId id) { return rarity == CardRarity::NONE || card_meta(id).rarity == rarity; };
    for (CardId id : card_reward_pool(character)) {
        if (keep(id)) candidates.push_back(id);
    }
    // Prismatic Gem: the other heroes' pools are added too (the pools do not overlap).
    if (has_relic("PRISMATIC_GEM")) {
        for (uint8_t hero = 0; hero < static_cast<uint8_t>(CharacterId::COUNT); ++hero) {
            if (static_cast<CharacterId>(hero) == character) continue;
            for (CardId id : card_reward_pool(static_cast<CharacterId>(hero))) {
                if (keep(id)) candidates.push_back(id);
            }
        }
    }
    // Dingy Rug: the Colorless cards are offered too.
    if (has_relic("DINGY_RUG")) {
        for (uint16_t id = 0; id < static_cast<uint16_t>(CardId::COUNT); ++id) {
            if (card_meta(static_cast<CardId>(id)).pool == CardPool::COLORLESS) candidates.push_back(static_cast<CardId>(id));
        }
    }
    while (current_card_rewards.size() < current_card_rewards.capacity() && !candidates.empty()) {
        size_t pick = (boss ? rng.next() : next_random()) % candidates.size();
        current_card_rewards.push_back(DeckCard(candidates[pick], card_rewards_upgraded));
        candidates.erase(pick);
    }
}

void RunManager::generate_rewards() {
    RoomType room = current_node_index >= 0 ? map.nodes[current_node_index].type : RoomType::MONSTER;
    // Potions, relics and what a boss gives are drawn from the stream of the room, so the run's own sequence
    // is the one it was before fights gave them.
    RewardRng rng(reward_seed());
    bool boss = room == RoomType::BOSS;

    draw_card_rewards(boss, rng);
    // Prayer Wheel: a normal enemy gives one more set of cards, after the first one (as The Hunt does).
    if (room == RoomType::MONSTER && has_relic("PRAYER_WHEEL")) extra_card_rewards = static_cast<uint8_t>(std::min(255, extra_card_rewards + 1));

    // A boss gives 95 to 105 gold.
    gold_reward = boss ? static_cast<int16_t>(95 + rng.below(11)) : 15 + static_cast<int16_t>(next_random() % 15);
    // Amethyst Aubergine: the enemies drop 15 more Gold.
    if (has_relic("AMETHYST_AUBERGINE")) gold_reward = static_cast<int16_t>(gold_reward + 15);

    // A potion the hero can use, in 40 % of the fights; White Beast Statue makes it certain.
    potion_reward = {};
    bool potion_roll = rng.below(100) < POTION_REWARD_CHANCE;
    if (potion_roll || has_relic("WHITE_BEAST_STATUE")) {
        potion_reward = draw_potion(rng, character);
    }

    // A relic after an elite and after a boss, as long as there is room for one more.
    relic_reward = {};
    if ((room == RoomType::ELITE || boss) && relics.size() < relics.capacity()) {
        relic_reward = draw_relic(rng, character, boss ? RelicSource::BOSS : RelicSource::ELITE,
                                  std::span<const std::string_view>(relics.begin(), relics.size()));
    }
    // White Star: an Elite gives one more set of Rare cards, offered once the first set is taken or skipped.
    extra_rare_card_rewards = room == RoomType::ELITE && has_relic("WHITE_STAR") ? 1 : 0;
    on_rewards_generated(room);
    phase = RunPhase::REWARD_SCREEN;
}

bool RunManager::take_potion_reward(int replace_index) {
    if (phase != RunPhase::REWARD_SCREEN || potion_reward.empty()) return false;
    if (!potion_belt_full()) {
        potions.push_back(potion_reward);
    } else if (replace_index >= 0 && static_cast<size_t>(replace_index) < potions.size()) {
        potions[replace_index] = potion_reward;
    } else {
        return false;
    }
    potion_reward = {};
    return true;
}

bool RunManager::take_relic_reward() {
    if (phase != RunPhase::REWARD_SCREEN || relic_reward.empty()) return false;
    if (has_relic(relic_reward) || relics.size() >= relics.capacity()) return false;
    add_relic(relic_reward);
    relic_reward = {};
    return true;
}

void RunManager::finish_rewards() {
    // Bowler Hat: 25 % more gold from the rewards of the fight.
    const int bonus = has_relic("BOWLER_HAT") ? gold_reward / 4 : 0;
    gain_gold(static_cast<int16_t>(gold_reward + bonus));
    gold_reward = 0;
    potion_reward = {};
    relic_reward = {};
    extra_rare_card_rewards = 0;
    // The boss of act 1 or 2: its rewards are taken, the next act opens.
    if (current_node_index >= 0 && map.nodes[current_node_index].type == RoomType::BOSS && current_act < 3) {
        advance_act();
    } else {
        phase = RunPhase::MAP_NAVIGATION;
    }
}

void RunManager::pick_card_reward(int reward_index) {
    if (reward_index >= 0 && static_cast<size_t>(reward_index) < current_card_rewards.size()) {
        add_card_to_deck(current_card_rewards[reward_index]);
    }
    next_card_reward_or_finish();
}

void RunManager::skip_card_rewards() {
    next_card_reward_or_finish();
}

void RunManager::next_card_reward_or_finish() {
    // White Star: one more set of Rare cards after an Elite.
    if (extra_rare_card_rewards > 0) {
        --extra_rare_card_rewards;
        RewardRng rng(next_random());
        draw_card_rewards(false, rng, CardRarity::RARE);
        return;
    }
    // The Hunt: one more set of cards is offered before the rewards are over.
    if (extra_card_rewards > 0) {
        --extra_card_rewards;
        RewardRng rng(next_random());
        draw_card_rewards(false, rng);
        return;
    }
    finish_rewards();
}

void RunManager::add_relic(std::string_view relic_id) {
    if (relics.push_back(relic_id)) {
        on_relic_obtained(relic_id);
    }
}

void RunManager::on_relic_obtained(std::string_view relic_id) {
    // The relic is already stored: `has_relic` is true for it here.
    if (relic_id == "SMALL_CAPSULE" && has_relic("SMALL_CAPSULE")) gain_random_relics(1);
    if (relic_id == "STRAWBERRY" && has_relic("STRAWBERRY")) gain_max_hp(7);
    if (relic_id == "LOOMING_FRUIT" && has_relic("LOOMING_FRUIT")) gain_max_hp(31);
    if (relic_id == "NUTRITIOUS_OYSTER" && has_relic("NUTRITIOUS_OYSTER")) gain_max_hp(11);
    // Shared shop relic: 7 more maximum HP, then every point of HP is restored.
    if (relic_id == "LEES_WAFFLE" && has_relic("LEES_WAFFLE")) {
        gain_max_hp(7);
        hp = max_hp;
    }
    if (relic_id == "MANGO" && has_relic("MANGO")) gain_max_hp(14);
    if (relic_id == "PEAR" && has_relic("PEAR")) gain_max_hp(10);
    if (relic_id == "LARGE_CAPSULE" && has_relic("LARGE_CAPSULE")) {
        gain_random_relics(2);
        add_card_to_deck(DeckCard{ CardId::STRIKE, false });
        add_card_to_deck(DeckCard{ CardId::DEFEND, false });
    }
    if (relic_id == "YUMMY_COOKIE" && has_relic("YUMMY_COOKIE")) upgrade_random_cards(4);
    if (relic_id == "SAND_CASTLE" && has_relic("SAND_CASTLE")) upgrade_random_cards(6);
    if (relic_id == "WHETSTONE" && has_relic("WHETSTONE")) upgrade_random_cards(2, CardType::ATTACK);
    // War Paint: upgrades 2 random Skills when obtained.
    if (relic_id == "WAR_PAINT" && has_relic("WAR_PAINT")) upgrade_random_cards(2, CardType::SKILL);
    // The Act 2 map is already drawn when the Ancient offers it: draw it again as a single path.
    if (relic_id == "GOLDEN_COMPASS" && has_relic("GOLDEN_COMPASS") && current_act == 2 && current_node_index < 0) {
        regenerate_map();
    }
    // Bought from the merchant himself, the card lowers what is still on his shelves.
    if (relic_id == "SIGNET_RING" && has_relic("SIGNET_RING")) gain_gold(888);
    // Relic of the Ancients: the gold comes once, when the pearl is obtained.
    if (relic_id == "GOLDEN_PEARL" && has_relic("GOLDEN_PEARL")) gain_gold(150);
    // Rare relic: the gold comes once, when the coin is obtained.
    if (relic_id == "OLD_COIN" && has_relic("OLD_COIN")) gain_gold(300);
    // Relic of the Ancients: a Greed curse joins the deck and the gold comes once, when the pearl is obtained.
    if (relic_id == "CURSED_PEARL" && has_relic("CURSED_PEARL")) {
        add_card_to_deck(DeckCard{ CardId::GREED, false });
        gain_gold(333);
    }
    // Relic of the Ancients: one Ancient card of the hero, drawn at random with the run's generator.
    if (relic_id == "DUSTY_TOME" && has_relic("DUSTY_TOME")) {
        const auto hero_pool = static_cast<CardPool>(static_cast<uint8_t>(character) + 1);
        const auto is_ancient = [&](uint16_t id) {
            const CardMeta& meta = card_meta(static_cast<CardId>(id));
            return meta.rarity == CardRarity::ANCIENT && meta.pool == hero_pool;
        };
        uint16_t count = 0;
        for (uint16_t id = 0; id < static_cast<uint16_t>(CardId::COUNT); ++id) {
            if (is_ancient(id)) ++count;
        }
        if (count > 0) {
            uint16_t pick = next_random() % count;
            for (uint16_t id = 0; id < static_cast<uint16_t>(CardId::COUNT); ++id) {
                if (!is_ancient(id)) continue;
                if (pick-- == 0) {
                    add_card_to_deck(DeckCard{ static_cast<CardId>(id), false });
                    break;
                }
            }
        }
    }
    // Glass Eye (Ancient): 2 random Common, 2 Uncommon and 1 Rare cards of the hero's pool, all distinct.
    if (relic_id == "GLASS_EYE" && has_relic("GLASS_EYE")) {
        struct EyeDraw {
            CardRarity rarity;
            int count;
        };
        for (EyeDraw draw : { EyeDraw{ CardRarity::COMMON, 2 }, EyeDraw{ CardRarity::UNCOMMON, 2 }, EyeDraw{ CardRarity::RARE, 1 } }) {
            FixedVector<CardId, 128> candidates;
            for (CardId id : card_reward_pool(character)) {
                if (card_meta(id).rarity == draw.rarity) candidates.push_back(id);
            }
            for (int n = 0; n < draw.count && !candidates.empty(); ++n) {
                size_t pick = next_random() % candidates.size();
                add_card_to_deck(DeckCard{ candidates[pick], false });
                candidates.erase(pick);
            }
        }
    }
    if (relic_id == "SERE_TALON" && has_relic("SERE_TALON")) {
        lose_max_hp(9);
        for (int i = 0; i < 3; ++i) add_card_to_deck(DeckCard{ CardId::WISH, false });
    }
    if (relic_id == "BLOOD_SOAKED_ROSE" && has_relic("BLOOD_SOAKED_ROSE")) add_card_to_deck(DeckCard{ CardId::ENTHRALLED, false });
    if (relic_id == "JEWELRY_BOX" && has_relic("JEWELRY_BOX")) add_card_to_deck(DeckCard{ CardId::APOTHEOSIS, false });
    // Lead Paperweight: choose 1 Colorless card out of 2. The offer is the card reward screen with two cards.
    if (relic_id == "LEAD_PAPERWEIGHT" && has_relic("LEAD_PAPERWEIGHT")) {
        FixedVector<CardId, 64> colorless;
        for (uint16_t id = 0; id < static_cast<uint16_t>(CardId::COUNT); ++id) {
            if (card_meta(static_cast<CardId>(id)).pool == CardPool::COLORLESS) colorless.push_back(static_cast<CardId>(id));
        }
        current_card_rewards.clear();
        for (int n = 0; n < 2 && !colorless.empty(); ++n) {
            size_t pick = next_random() % colorless.size();
            current_card_rewards.push_back(DeckCard(colorless[pick], false));
            colorless.erase(pick);
        }
        phase = RunPhase::REWARD_SCREEN;
    }
    if (relic_id == "DISTINGUISHED_CAPE" && has_relic("DISTINGUISHED_CAPE")) {
        // Two random curses (repetition allowed) drawn from the whole curse pool, then three Apparitions.
        const auto is_curse = [](uint16_t id) { return card_meta(static_cast<CardId>(id)).pool == CardPool::CURSE; };
        uint16_t curse_count = 0;
        for (uint16_t id = 0; id < static_cast<uint16_t>(CardId::COUNT); ++id) {
            if (is_curse(id)) ++curse_count;
        }
        for (int i = 0; i < 2 && curse_count > 0; ++i) {
            uint16_t pick = next_random() % curse_count;
            for (uint16_t id = 0; id < static_cast<uint16_t>(CardId::COUNT); ++id) {
                if (!is_curse(id)) continue;
                if (pick-- == 0) {
                    add_card_to_deck(DeckCard{ static_cast<CardId>(id), false });
                    break;
                }
            }
        }
        for (int i = 0; i < 3; ++i) add_card_to_deck(DeckCard{ CardId::APPARITION, false });
    }
    if (relic_id == "ARCANE_SCROLL" && has_relic("ARCANE_SCROLL")) {
        // One random Rare card of the hero's pool, drawn the way a card reward is.
        FixedVector<CardId, 128> rares;
        for (CardId id : card_reward_pool(character)) {
            if (card_meta(id).rarity == CardRarity::RARE) rares.push_back(id);
        }
        if (!rares.empty()) add_card_to_deck(DeckCard{ rares[next_random() % rares.size()], false });
    }
    if (relic_id == "NEOWS_BONES" && has_relic("NEOWS_BONES")) {
        // Two random Neow relics among the ones Neow offers: a relic already owned (this one included) is never drawn.
        const Ancient& neow = get_ancient(AncientId::NEOW);
        for (int i = 0; i < 2 && relics.size() < relics.capacity(); ++i) {
            uint32_t count = 0;
            for (const AncientSlot& slot : neow.slots) {
                for (const AncientRelic& option : slot.options) {
                    if (!has_relic(option.id)) ++count;
                }
            }
            if (count == 0) break;
            uint32_t pick = next_random() % count;
            std::string_view chosen;
            for (const AncientSlot& slot : neow.slots) {
                for (const AncientRelic& option : slot.options) {
                    if (has_relic(option.id)) continue;
                    if (pick-- == 0) chosen = option.id;
                }
            }
            add_relic(chosen);
        }
        // Then one random curse, drawn from the whole curse pool like Distinguished Cape.
        const auto is_curse = [](uint16_t id) { return card_meta(static_cast<CardId>(id)).pool == CardPool::CURSE; };
        uint16_t curse_count = 0;
        for (uint16_t id = 0; id < static_cast<uint16_t>(CardId::COUNT); ++id) {
            if (is_curse(id)) ++curse_count;
        }
        if (curse_count > 0) {
            uint16_t pick = next_random() % curse_count;
            for (uint16_t id = 0; id < static_cast<uint16_t>(CardId::COUNT); ++id) {
                if (!is_curse(id)) continue;
                if (pick-- == 0) {
                    add_card_to_deck(DeckCard{ static_cast<CardId>(id), false });
                    break;
                }
            }
        }
    }
    if (relic_id == "PAELS_HORN" && has_relic("PAELS_HORN")) {
        for (int i = 0; i < 2; ++i) add_card_to_deck(DeckCard{ CardId::RELAX, false });
    }
    // Calling Bell (Ancient): the unique Curse of the Bell joins the deck, then 3 random relics.
    if (relic_id == "CAULDRON" && has_relic("CAULDRON")) gain_random_potions(5);
    if (relic_id == "CALLING_BELL" && has_relic("CALLING_BELL")) {
        add_card_to_deck(DeckCard{ CardId::CURSE_OF_THE_BELL, false });
        gain_random_relics(3);
    }
    if (relic_id == "TANXS_WHISTLE" && has_relic("TANXS_WHISTLE")) add_card_to_deck(DeckCard{ CardId::WHISTLE, false });
    if (relic_id == "STORYBOOK" && has_relic("STORYBOOK")) add_card_to_deck(DeckCard{ CardId::BRIGHTEST_FLAME, false });
    if (relic_id == "NEOWS_TORMENT" && has_relic("NEOWS_TORMENT")) add_card_to_deck(DeckCard{ CardId::NEOWS_FURY, false });
    if (relic_id == "PAELS_TOOTH" && has_relic("PAELS_TOOTH")) {
        request_deck_choice(DeckChoiceKind::REMOVE, 5);
        paels_tooth_picks = deck_choice.remaining;
    }
    if (relic_id == "CLAWS" && has_relic("CLAWS")) request_deck_choice(DeckChoiceKind::TRANSFORM, 6, true, false, CardId::MAUL);
    if (relic_id == "NEW_LEAF" && has_relic("NEW_LEAF")) request_deck_choice(DeckChoiceKind::TRANSFORM, 1);
    if (relic_id == "POMANDER" && has_relic("POMANDER")) request_deck_choice(DeckChoiceKind::UPGRADE, 1);
    if (relic_id == "ASTROLABE" && has_relic("ASTROLABE")) request_deck_choice(DeckChoiceKind::TRANSFORM, 3, false, true);
    if (relic_id == "PANDORAS_BOX" && has_relic("PANDORAS_BOX")) {
        // Every Strike and Defend of the deck becomes a random card of the hero's pool, in place.
        for (size_t i = 0; i < master_deck.size(); ++i) {
            const CardId id = master_deck[i].id;
            if (id == CardId::STRIKE || id == CardId::DEFEND) transform_card_in_deck(static_cast<int>(i));
        }
    }
    if (relic_id == "PRESERVED_FOG" && has_relic("PRESERVED_FOG")) {
        // Hubris joins only after the removals, so the Eternal curse can never be picked.
        request_deck_choice(DeckChoiceKind::REMOVE, 3);
        preserved_fog_pending = true;
        finish_preserved_fog();
    }
    if (relic_id == "PRECISE_SCISSORS" && has_relic("PRECISE_SCISSORS")) request_deck_choice(DeckChoiceKind::REMOVE, 1);
    if (relic_id == "NEOWS_TALISMAN" && has_relic("NEOWS_TALISMAN")) {
        // One random Strike and one random Defend, not yet upgraded, are upgraded.
        const CardId targets[] = { CardId::STRIKE, CardId::DEFEND };
        for (CardId target : targets) {
            FixedVector<uint8_t, 50> candidates;
            for (size_t i = 0; i < master_deck.size(); ++i) {
                if (!master_deck[i].upgraded && master_deck[i].id == target) candidates.push_back(static_cast<uint8_t>(i));
            }
            if (!candidates.empty()) master_deck[candidates[next_random() % candidates.size()]].upgraded = true;
        }
    }
    if (relic_id == "EMPTY_CAGE" && has_relic("EMPTY_CAGE")) request_deck_choice(DeckChoiceKind::REMOVE, 2);
    if (relic_id == "MEMBERSHIP_CARD" && phase == RunPhase::SHOP) current_shop.apply_discount(MEMBERSHIP_CARD_DISCOUNT);
    if (relic_id == "THE_COURIER" && phase == RunPhase::SHOP) current_shop.apply_discount(THE_COURIER_DISCOUNT);
    // Circlet (fallback relic): no effect in game, it is only a circlet.
    if (has_relic("CIRCLET")) {}
    // Le tapis du Marchand ??? (event relic): no effect in game, a poor imitation.
    if (has_relic("FAKE_MERCHANTS_RUG")) {}
}

void RunManager::count_hook(RunHook hook) {
    uint16_t& runs = hook_runs[static_cast<size_t>(hook)];
    if (runs < UINT16_MAX) ++runs;
}

void RunManager::on_room_entered(RoomType room) {
    count_hook(RunHook::ROOM_ENTERED);
    // Eternal Feather: entering a Rest Site heals 3 HP for every 5 cards of the deck.
    if (room == RoomType::REST_SITE && has_relic("ETERNAL_FEATHER")) {
        heal(static_cast<int16_t>(master_deck.size() / 5 * 3));
    }
    // Venerable Tea Set: each Rest Site entered adds one count, spent at the start of the next combat.
    if (room == RoomType::REST_SITE && has_relic("VENERABLE_TEA_SET") && relic_counter("VENERABLE_TEA_SET") < UINT8_MAX) {
        set_relic_counter("VENERABLE_TEA_SET", static_cast<uint8_t>(relic_counter("VENERABLE_TEA_SET") + 1));
    }
    // Planisphere: each ? room entered heals 5 HP.
    if (room == RoomType::EVENT && has_relic("PLANISPHERE")) heal(5);
    // Meal Ticket: entering a shop heals 15 HP.
    if (room == RoomType::SHOP && has_relic("MEAL_TICKET")) heal(15);
}

void RunManager::on_rested() {
    count_hook(RunHook::RESTED);
    // Stone Humidifier: each Rest at a Rest Site raises Max HP by 5.
    if (has_relic("STONE_HUMIDIFIER")) gain_max_hp(5);
    // Tiny Mailbox: each Rest procures 2 random potions (those that do not fit the full belt are lost).
    if (has_relic("TINY_MAILBOX")) gain_random_potions(2);
}

void RunManager::on_shop_opened() {
    count_hook(RunHook::SHOP_OPENED);
    if (has_relic("MEMBERSHIP_CARD")) current_shop.apply_discount(MEMBERSHIP_CARD_DISCOUNT);
    if (has_relic("THE_COURIER")) current_shop.apply_discount(THE_COURIER_DISCOUNT);
    if (has_relic("LORDS_PARASOL")) {
        // Everything on sale is obtained for free; a potion that does not fit the full belt is left behind.
        current_shop.apply_discount(100);
        for (size_t i = 0; i < current_shop.cards.size(); ++i) current_shop.buy_card(*this, static_cast<int>(i));
        for (size_t i = 0; i < current_shop.relics.size(); ++i) current_shop.buy_relic(*this, static_cast<int>(i));
        for (size_t i = 0; i < current_shop.potions.size(); ++i) current_shop.buy_potion(*this, static_cast<int>(i));
    }
}

void RunManager::on_combat_won(RoomType room) {
    count_hook(RunHook::COMBAT_WON);
    // War Hammer: killing an Elite upgrades 4 random cards of the deck.
    if (room == RoomType::ELITE && has_relic("WAR_HAMMER")) upgrade_random_cards(4);
    // Paël's Tooth gives back one devoured card, upgraded, per fight (chosen at random).
    if (has_relic("PAELS_TOOTH") && !paels_tooth_cards.empty()) {
        size_t pick = next_random() % paels_tooth_cards.size();
        CardId id = paels_tooth_cards[pick];
        paels_tooth_cards.erase(pick);
        add_card_to_deck(DeckCard{ id, true });
    }
    // Lava Rock: the Act 1 Boss gives 2 relics when it is defeated, drawn as a boss relic is.
    if (room == RoomType::BOSS && current_act == 1 && has_relic("LAVA_ROCK")) {
        RewardRng rng(reward_seed() ^ 0x4C41A0C4u); // own salt, apart from the boss rewards of the same room
        for (int i = 0; i < LAVA_ROCK_RELICS && relics.size() < relics.capacity(); ++i) {
            std::string_view id = draw_relic(rng, character, RelicSource::BOSS,
                                             std::span<const std::string_view>(relics.begin(), relics.size()));
            if (id.empty()) break;
            add_relic(id);
        }
    }
}

void RunManager::on_rewards_generated(RoomType) {
    count_hook(RunHook::REWARDS_GENERATED);
}

void RunManager::on_card_added(DeckCard& card) {
    count_hook(RunHook::CARD_ADDED);
    // Frozen Egg: a Power added to the Deck is upgraded.
    if (has_relic("FROZEN_EGG") && CardDef::get(card.id).type == CardType::POWER) card.upgraded = true;
    // Toxic Egg: a Skill added to the Deck is Upgraded.
    if (has_relic("TOXIC_EGG") && CardDef::get(card.id).type == CardType::SKILL) card.upgraded = true;
}

void RunManager::on_gold_gained(int16_t) {
    count_hook(RunHook::GOLD_GAINED);
}

uint8_t RunManager::relic_counter(std::string_view relic_id) const {
    for (const RelicCounter& counter : relic_counters) {
        if (counter.relic_id == relic_id) return counter.value;
    }
    return 0;
}

bool RunManager::set_relic_counter(std::string_view relic_id, uint8_t value) {
    for (RelicCounter& counter : relic_counters) {
        if (counter.relic_id == relic_id) {
            counter.value = value;
            return true;
        }
    }
    return relic_counters.push_back(RelicCounter{ relic_id, value });
}

bool RunManager::tick_relic_counter(std::string_view relic_id, uint8_t every) {
    uint8_t next = static_cast<uint8_t>(relic_counter(relic_id) + 1);
    bool reached = every > 0 && next >= every;
    return set_relic_counter(relic_id, reached ? 0 : next) && reached;
}

void RunManager::gain_max_hp(int16_t amount) {
    if (amount <= 0) return;
    max_hp = static_cast<int16_t>(std::min<int>(999, max_hp + amount));
    heal(amount);
}

void RunManager::lose_max_hp(int16_t amount) {
    if (amount <= 0) return;
    max_hp = static_cast<int16_t>(std::max<int>(1, max_hp - amount));
    hp = std::min(hp, max_hp);
}

void RunManager::heal(int16_t amount) {
    if (amount <= 0) return;
    hp = static_cast<int16_t>(std::min<int>(max_hp, hp + amount));
}

void RunManager::gain_gold(int16_t amount) {
    if (amount <= 0) return;
    // Ectoplasm: no gold can be gained, from a fight, a treasure, an event or a relic.
    if (has_relic("ECTOPLASM")) return;
    gold = static_cast<int16_t>(std::min<int>(9999, gold + amount));
    on_gold_gained(amount);
    // Dragon Fruit: each gain of Gold raises the Max HP by 1.
    if (has_relic("DRAGON_FRUIT")) gain_max_hp(1);
}

int RunManager::upgrade_random_cards(int count) {
    return upgrade_random_cards(count, true, CardType::ATTACK);
}

int RunManager::upgrade_random_cards(int count, CardType type) {
    return upgrade_random_cards(count, false, type);
}

int RunManager::upgrade_random_cards(int count, bool any_type, CardType type) {
    int upgraded = 0;
    for (; upgraded < count; ++upgraded) {
        FixedVector<uint8_t, 50> candidates;
        for (size_t i = 0; i < master_deck.size(); ++i) {
            const DeckCard& card = master_deck[i];
            if (!card.upgraded && (any_type || CardDef::get(card.id).type == type)) {
                candidates.push_back(static_cast<uint8_t>(i));
            }
        }
        if (candidates.empty()) break;
        master_deck[candidates[next_random() % candidates.size()]].upgraded = true;
    }
    return upgraded;
}

// Book of Five Rings: every 5th card added to the Deck heals 20 HP.
static constexpr uint8_t BOOK_OF_FIVE_RINGS_CARDS = 5;
static constexpr int16_t BOOK_OF_FIVE_RINGS_HEAL = 20;

bool RunManager::add_card_to_deck(DeckCard card) {
    if (master_deck.size() >= master_deck.capacity()) return false;
    // Molten Egg: an Attack added to the deck comes in upgraded.
    if (has_relic("MOLTEN_EGG") && CardDef::get(card.id).type == CardType::ATTACK) card.upgraded = true;
    on_card_added(card);
    if (!master_deck.push_back(card)) return false;
    // Lucky Fysh: each card that joins the deck gains gold, whatever the source (reward, shop, event).
    if (has_relic("LUCKY_FYSH")) gain_gold(15);
    if (has_relic("BOOK_OF_FIVE_RINGS") && tick_relic_counter("BOOK_OF_FIVE_RINGS", BOOK_OF_FIVE_RINGS_CARDS)) {
        heal(BOOK_OF_FIVE_RINGS_HEAL);
    }
    return true;
}

bool RunManager::transform_card_in_deck(int deck_index, bool upgraded, CardId into) {
    if (deck_index < 0 || static_cast<size_t>(deck_index) >= master_deck.size()) return false;
    if (CardDef::is_eternal(master_deck[deck_index])) return false;
    if (into != CardId::NONE) {
        master_deck[deck_index] = DeckCard(into, upgraded);
        return true;
    }
    std::span<const CardId> pool = card_reward_pool(character);
    CardId old_id = master_deck[deck_index].id;
    size_t candidates = 0;
    for (CardId id : pool) {
        if (id != old_id) ++candidates;
    }
    if (candidates == 0) return false;

    size_t pick = next_random() % candidates;
    for (CardId id : pool) {
        if (id != old_id && pick-- == 0) {
            master_deck[deck_index] = DeckCard(id, upgraded);
            return true;
        }
    }
    return false;
}

int RunManager::gain_random_relics(int count) {
    int gained = 0;
    while (gained < count && relics.size() < relics.capacity()) {
        std::string_view id = draw_treasure_relic();
        if (id.empty()) break;
        add_relic(id);
        ++gained;
    }
    return gained;
}

int RunManager::gain_random_potions(int count) {
    RewardRng rng(next_random());
    int gained = 0;
    while (gained < count && !potion_belt_full()) {
        std::string_view id = draw_potion(rng, character);
        if (id.empty()) break;
        potions.push_back(id);
        ++gained;
    }
    return gained;
}

void RunManager::add_potion_slots(int count) {
    if (count <= 0) return;
    potion_slots = static_cast<uint8_t>(std::min<int>(MAX_POTION_SLOTS, potion_slots + count));
}

void RunManager::request_deck_choice(DeckChoiceKind kind, int count, bool optional, bool upgrade_result, CardId transform_into) {
    deck_choice = {};
    if (kind == DeckChoiceKind::NONE || count <= 0) return;
    deck_choice.kind = kind;
    int candidates = 0;
    for (size_t i = 0; i < master_deck.size(); ++i) {
        if (deck_choice_allows(static_cast<int>(i))) ++candidates;
    }
    if (candidates == 0) {
        deck_choice = {};
        return;
    }
    deck_choice.remaining = static_cast<uint8_t>(std::min(count, candidates));
    deck_choice.optional = optional;
    deck_choice.upgrade_result = upgrade_result;
    deck_choice.transform_into = transform_into;
}

void RunManager::finish_preserved_fog() {
    if (!preserved_fog_pending || has_deck_choice()) return;
    preserved_fog_pending = false;
    add_card_to_deck(DeckCard{ CardId::FOLLY, false });
}

bool RunManager::deck_choice_allows(int deck_index) const {
    if (deck_index < 0 || static_cast<size_t>(deck_index) >= master_deck.size()) return false;
    switch (deck_choice.kind) {
        case DeckChoiceKind::UPGRADE: return !master_deck[deck_index].upgraded;
        case DeckChoiceKind::REMOVE:
        case DeckChoiceKind::TRANSFORM: return !CardDef::is_eternal(master_deck[deck_index]);
        default: return false;
    }
}

bool RunManager::resolve_deck_choice(int deck_index) {
    if (!deck_choice_allows(deck_index)) return false;
    bool done = false;
    switch (deck_choice.kind) {
        case DeckChoiceKind::UPGRADE: done = upgrade_card_in_deck(deck_index); break;
        case DeckChoiceKind::REMOVE: {
            CardId devoured = master_deck[deck_index].id;
            done = remove_card_from_deck(deck_index);
            if (done && paels_tooth_picks > 0) {
                --paels_tooth_picks;
                paels_tooth_cards.push_back(devoured);
            }
            break;
        }
        case DeckChoiceKind::TRANSFORM: done = transform_card_in_deck(deck_index, deck_choice.upgrade_result, deck_choice.transform_into); break;
        default: break;
    }
    if (!done) return false;

    // The cards left may not allow as many picks as asked (deck emptied, everything upgraded).
    DeckChoice next = deck_choice;
    request_deck_choice(next.kind, next.remaining - 1, next.optional, next.upgrade_result, next.transform_into);
    finish_preserved_fog();
    finish_cook();
    return true;
}

bool RunManager::skip_deck_choice() {
    if (!has_deck_choice() || !deck_choice.optional) return false;
    deck_choice = {};
    return true;
}

bool RunManager::has_relic(std::string_view relic_id) const {
    for (size_t i = 0; i < relics.size(); ++i) {
        if (relics[i] == relic_id) return true;
    }
    return false;
}

std::span<const std::string_view> RunManager::active_relics() {
    // Every implemented relic that drops normally (Common, Uncommon, Rare, Shop); Starter, Boss, Event and Ancient
    // relics come from elsewhere. Shop relics are in the list too: only the merchant gives them.
    static constexpr std::string_view ACTIVE_RELICS[] = {
        "VAJRA", "STRAWBERRY", "ANCHOR", "BLOOD_VIAL", "LANTERN", "RED_MASK", "ODDLY_SMOOTH_STONE", "FESTIVE_POPPER", "BAG_OF_MARBLES",
        "STONE_CRACKER", "PANTOGRAPH", "SLING_OF_COURAGE",
        // Start and end of turn
        "CAPTAINS_WHEEL", "HORN_CLEAT", "CANDELABRA", "CHANDELIER", "SPARKLING_ROUGE", "HAPPY_FLOWER", "PENDULUM", "BREAD", "BOOKMARK",
        "MERCURY_HOURGLASS", "CLOAK_CLASP", "POCKETWATCH", "PARRYING_SHIELD", "STONE_CALENDAR", "ART_OF_WAR", "ORICHALCUM",
        "SCREAMING_FLAGON", "ICE_CREAM", "STURDY_CLAMP", "MEMBERSHIP_CARD", "CAULDRON",
        // Combat events
        "GREMLIN_HORN", "RAZOR_TOOTH", "THE_ABACUS", "PERMAFROST", "GAME_PIECE", "REPTILE_TRINKET", "CENTENNIAL_PUZZLE", "LIZARD_TAIL",
        "BELLOWS", "UNCEASING_TOP", "TOUGH_BANDAGES", "TINGSHA", "GAMBLING_CHIP", "HELICAL_DART", "NINJA_SCROLL",
        // Defect and Necrobinder
        "DATA_DISK", "SYMBIOTIC_VIRUS", "GOLD_PLATED_CABLES", "METRONOME", "RUNIC_CAPACITOR", "POWER_CELL", "EMOTION_CHIP",
        "BOOK_REPAIR_KNIFE", "BONE_FLUTE",
        // On the combat hooks (#299)
        "BAG_OF_PREPARATION", "BURNING_STICKS", "INTIMIDATING_HELMET", "JOSS_PAPER", "KUNAI", "KUSARIGAMA", "LETTER_OPENER",
        "MEAT_ON_THE_BONE", "MUMMIFIED_HAND", "NUNCHAKU", "ORNAMENTAL_FAN", "PEN_NIB", "RAINBOW_RING", "RIPPLE_BASIN", "SHURIKEN", "STRIKE_DUMMY", "MINIATURE_CANNON",
        "TUNGSTEN_ROD", "TUNING_FORK", "VEXING_PUZZLEBOX",
        "BRIMSTONE", "CHARONS_ASHES", "DEMON_TONGUE", "RED_SKULL", "SELF_FORMING_CLAY", // Ironclad
        "SNECKO_SKULL", "TWISTED_FUNNEL",                                                // Silent
        "WHITE_BEAST_STATUE", "BELT_BUCKLE", "TINY_MAILBOX", "REGAL_PILLOW", "AMETHYST_AUBERGINE", "BOWLER_HAT", "ETERNAL_FEATHER", "VENERABLE_TEA_SET", "PLANISPHERE", "MEAL_TICKET", "SHOVEL", // Shared, out of combat
        "WHITE_STAR",                         // Shared, rare: an Elite gives one more set of Rare cards
        "VAMBRACE",                           // Shared, uncommon: the first Block of a card each combat is doubled
        "BEATING_REMNANT", "UNSETTLING_LAMP", // Shared, rare: caps the HP lost each turn, doubles the first debuff of the fight
        "GHOST_SEED",                        // Shared, the Strikes and Defends of the fight
        "LEES_WAFFLE", "MANGO", "OLD_COIN", "PEAR", "LUCKY_FYSH",                 // Shared, out of combat
        "BOOK_OF_FIVE_RINGS",                 // Shared, out of combat: heals each 5th card added to the Deck
        "GALACTIC_DUST", "LUNAR_PASTRY", "MINI_REGENT", "REGALITE", "FENCING_MANUAL",   // Regent
        "BIG_HAT", "IVORY_TILE", "FUNERARY_MASK",                                        // Necrobinder
        // Statuses given at the start of the combat (#91, #93)
        "BRONZE_SCALES", "GORGET", "AKABEKO",
        // At the merchant (#487)
        "THE_COURIER",
        // Max HP with each gain of Gold (#474)
        "DRAGON_FRUIT",
        // Retained hand on the first turn (#509)
        "RINGING_TRIANGLE",
        // Upgrades on pickup (#465, #464)
        "WHETSTONE", "WAR_PAINT",
        // Colorless cards in the card rewards (#486)
        "DINGY_RUG",
        // Out of combat, at the end of a fight without HP lost (#476)
        "LAVA_LAMP",
        // Extra card reward after a normal fight (#479)
        "PRAYER_WHEEL",
        // Upgrades the Attacks added to the deck (#493)
        "MOLTEN_EGG",
        // Upgrades a Power added to the Deck (#494)
        "FROZEN_EGG",
        // A Colorless card offered at the start of the combat (#496)
        "TOOLBOX",
        // Out of combat, at the rest sites (#475)
        "GIRYA",
        // Potion at the start of each combat (#497)
        "PETRIFIED_TOAD",
        // Skills added to the Deck are upgraded (#495)
        "TOXIC_EGG",
        // At the merchant (#503)
        "CHEMICAL_X"
    };
    return ACTIVE_RELICS;
}

std::string_view RunManager::draw_treasure_relic() {
    // A relic with an implemented effect, shared or of this hero, never one already owned, never one of the
    // merchant's own.
    std::string_view hero_pool = get_character(character).english_name;
    auto is_candidate = [&](std::string_view id) {
        const RelicData* relic = find_relic(id);
        return relic && relic->rarity != RelicRarity::SHOP && (relic->pool == "Shared" || relic->pool == hero_pool) &&
               !has_relic(id);
    };
    uint32_t count = 0;
    for (std::string_view id : active_relics()) {
        if (is_candidate(id)) ++count;
    }
    if (count == 0) return {};

    uint32_t pick = next_random() % count;
    for (std::string_view id : active_relics()) {
        if (is_candidate(id) && pick-- == 0) return id;
    }
    return {};
}

static constexpr uint8_t PUMPKIN_CANDLE_COMBATS = 5;

void RunManager::register_relic_powers() {
    // Blessed Antler: one more energy each turn, paid for with three Dazed shuffled into the draw pile.
    if (has_relic("BLESSED_ANTLER")) {
        combat.add_power(PowerId::TURN_ENERGY, 1);
        for (int copy = 0; copy < 3; ++copy) combat.create_card(CardId::DAZED, Pile::DRAW);
    }
    if (has_relic("BLOOD_SOAKED_ROSE")) combat.add_power(PowerId::TURN_ENERGY, 1);
    // Ectoplasm: one more energy each turn.
    if (has_relic("ECTOPLASM")) combat.add_power(PowerId::TURN_ENERGY, 1);
    // Philosopher's Stone (ancient): one more energy each turn.
    if (has_relic("PHILOSOPHERS_STONE")) combat.add_power(PowerId::TURN_ENERGY, 1);
    if (has_relic("PRISMATIC_GEM")) combat.add_power(PowerId::TURN_ENERGY, 1);
    // Whispering Earring: one more energy each turn, and Vakuu plays the first turn of the combat.
    if (has_relic("WHISPERING_EARRING")) {
        combat.add_power(PowerId::TURN_ENERGY, 1);
        combat.add_power(PowerId::WHISPERING_EARRING, 1);
    }
    // Velvet Choker: one more energy each turn, and no more than VELVET_CHOKER_CARDS cards played each turn.
    if (has_relic("VELVET_CHOKER")) {
        combat.add_power(PowerId::TURN_ENERGY, 1);
        combat.add_power(PowerId::VELVET_CHOKER, VELVET_CHOKER_CARDS);
    }
    // Pael's Eye: the first turn ended without playing a card is followed by an extra turn.
    if (has_relic("PAELS_EYE")) combat.add_power(PowerId::PAELS_EYE, 1);
    // Start of turn
    if (has_relic("CAPTAINS_WHEEL")) combat.add_power(PowerId::CAPTAINS_WHEEL, 18);
    if (has_relic("HORN_CLEAT")) combat.add_power(PowerId::HORN_CLEAT, 14);
    if (has_relic("CANDELABRA")) combat.add_power(PowerId::CANDELABRA, 2);
    if (has_relic("CHANDELIER")) combat.add_power(PowerId::CHANDELIER, 3);
    if (has_relic("SPARKLING_ROUGE")) combat.add_power(PowerId::SPARKLING_ROUGE, 1);
    if (has_relic("HAPPY_FLOWER")) combat.add_power(PowerId::HAPPY_FLOWER, 1);
    if (has_relic("PENDULUM")) combat.add_power(PowerId::PENDULUM, 1);
    if (has_relic("BREAD")) combat.add_power(PowerId::BREAD, 1);
    if (has_relic("BOOKMARK")) combat.add_power(PowerId::BOOKMARK, 1);
    if (has_relic("VERY_HOT_COCOA")) combat.add_power(PowerId::VERY_HOT_COCOA, 4);
    // Paël's Blood: one more card in every opening draw, the first one included.
    // Fiddle: two more cards in every opening draw, then no draw until the next turn.
    if (has_relic("FIDDLE")) {
        combat.add_power(PowerId::TURN_DRAW, 2);
        combat.add_power(PowerId::FIDDLE, 1);
    }
    if (has_relic("PAELS_BLOOD")) combat.add_power(PowerId::TURN_DRAW, 1);
    if (has_relic("MERCURY_HOURGLASS")) combat.add_power(PowerId::MERCURY_HOURGLASS, 3);
    // End of turn
    if (has_relic("CLOAK_CLASP")) combat.add_power(PowerId::CLOAK_CLASP, 1);
    if (has_relic("POCKETWATCH")) combat.add_power(PowerId::POCKETWATCH, 3);
    if (has_relic("PARRYING_SHIELD")) combat.add_power(PowerId::PARRYING_SHIELD, 6);
    if (has_relic("STONE_CALENDAR")) combat.add_power(PowerId::STONE_CALENDAR, 52);
    if (has_relic("PAELS_TEARS")) combat.add_power(PowerId::PAELS_TEARS, 2);
    if (has_relic("ART_OF_WAR")) combat.add_power(PowerId::ART_OF_WAR, 1);
    if (has_relic("ORICHALCUM")) combat.add_power(PowerId::ORICHALCUM, 6);
    if (has_relic("SCREAMING_FLAGON")) combat.add_power(PowerId::SCREAMING_FLAGON, 20);
    // Runic Pyramid: the hand is not discarded at the end of the turn (KEEP_HAND, the Well-Laid Plans rule).
    if (has_relic("RUNIC_PYRAMID")) combat.add_power(PowerId::KEEP_HAND);
    // Kept from one turn to the next
    // Ancient relic: the combat borrows the gold it spends, paid back when the fight ends.
    if (has_relic("SEAL_OF_GOLD")) {
        combat.gold = gold;
        combat.add_power(PowerId::SEAL_OF_GOLD, 1);
    }
    // Pumpkin Candle: one more energy each turn, for the first 5 combats; the counter holds the combats lit so far.
    if (has_relic("PUMPKIN_CANDLE") && relic_counter("PUMPKIN_CANDLE") < PUMPKIN_CANDLE_COMBATS) {
        combat.add_power(PowerId::TURN_ENERGY, 1);
        set_relic_counter("PUMPKIN_CANDLE", static_cast<uint8_t>(relic_counter("PUMPKIN_CANDLE") + 1));
    }
    if (has_relic("SAI")) combat.add_power(PowerId::SAI, 7);
    if (has_relic("ICE_CREAM")) combat.add_power(PowerId::ICE_CREAM);
    if (has_relic("STURDY_CLAMP")) combat.add_power(PowerId::STURDY_CLAMP, 10);
    // Combat events
    if (has_relic("GREMLIN_HORN")) combat.add_power(PowerId::GREMLIN_HORN, 1);
    if (has_relic("RAZOR_TOOTH")) combat.add_power(PowerId::RAZOR_TOOTH);
    if (has_relic("THE_ABACUS")) combat.add_power(PowerId::THE_ABACUS, 6);
    if (has_relic("PERMAFROST")) combat.add_power(PowerId::PERMAFROST, 7);
    if (has_relic("GAME_PIECE")) combat.add_power(PowerId::GAME_PIECE, 1);
    if (has_relic("REPTILE_TRINKET")) combat.add_power(PowerId::REPTILE_TRINKET, 3);
    if (has_relic("CENTENNIAL_PUZZLE")) combat.add_power(PowerId::CENTENNIAL_PUZZLE, 3);
    if (has_relic("LIZARD_TAIL")) combat.add_power(PowerId::LIZARD_TAIL, 50);
    if (has_relic("UNCEASING_TOP")) combat.add_power(PowerId::UNCEASING_TOP);
    if (has_relic("TOUGH_BANDAGES")) combat.add_power(PowerId::TOUGH_BANDAGES, 3);
    if (has_relic("TINGSHA")) combat.add_power(PowerId::TINGSHA, 3);
    // Hero relics
    if (has_relic("BOUND_PHYLACTERY")) combat.add_power(PowerId::TURN_SUMMON, 1);
    // Phylactery Unbound: Osty is there from the start of the combat, and grows faster.
    if (has_relic("PHYLACTERY_UNBOUND")) {
        combat.player.summon_minion(5);
        combat.add_power(PowerId::TURN_SUMMON, 2);
    }
    if (has_relic("BONE_FLUTE")) combat.add_power(PowerId::BONE_FLUTE, 2);
    // Funerary Mask (Necrobinder): 3 Souls shuffled into the draw pile before the first hand is drawn.
    if (has_relic("FUNERARY_MASK")) combat.add_souls(3);
    if (has_relic("GOLD_PLATED_CABLES")) combat.add_power(PowerId::GOLD_PLATED_CABLES);
    if (has_relic("METRONOME")) combat.add_power(PowerId::METRONOME, 30);
    if (has_relic("EMOTION_CHIP")) combat.add_power(PowerId::EMOTION_CHIP);
    if (has_relic("BOOK_REPAIR_KNIFE")) combat.add_power(PowerId::BOOK_REPAIR_KNIFE, 3);
    if (has_relic("HELICAL_DART")) combat.add_power(PowerId::HELICAL_DART, 1);
    // Cards played, exhausted or created
    if (has_relic("JOSS_PAPER")) combat.add_power(PowerId::JOSS_PAPER, 1);
    if (has_relic("LETTER_OPENER")) combat.add_power(PowerId::LETTER_OPENER, 5);
    if (has_relic("TUNING_FORK")) combat.add_power(PowerId::TUNING_FORK, 7);
    if (has_relic("KUNAI")) combat.add_power(PowerId::KUNAI, 1);
    if (has_relic("KUSARIGAMA")) combat.add_power(PowerId::KUSARIGAMA, 6);
    if (has_relic("MUMMIFIED_HAND")) combat.add_power(PowerId::MUMMIFIED_HAND);
    if (has_relic("NUNCHAKU")) combat.add_power(PowerId::NUNCHAKU, 1);
    if (has_relic("PEN_NIB")) combat.add_power(PowerId::PEN_NIB);
    if (has_relic("STRIKE_DUMMY")) combat.add_power(PowerId::STRIKE_DUMMY, 3);
    if (has_relic("MINIATURE_CANNON")) combat.add_power(PowerId::MINIATURE_CANNON, 3);
    // Tungsten Rod: whenever the player would lose HP, 1 less.
    if (has_relic("TUNGSTEN_ROD")) combat.player.hp_loss_reduction = 1;
    if (has_relic("RAINBOW_RING")) combat.add_power(PowerId::RAINBOW_RING, 1);
    if (has_relic("RIPPLE_BASIN")) combat.add_power(PowerId::RIPPLE_BASIN, 4);
    if (has_relic("BURNING_STICKS")) combat.add_power(PowerId::BURNING_STICKS);
    // Intimidating Helmet does what Danse Macabre does: Block for a card that costs 2 or more.
    if (has_relic("INTIMIDATING_HELMET")) combat.add_power(PowerId::DANSE_MACABRE, 4);
    if (has_relic("SHURIKEN")) combat.add_power(PowerId::SHURIKEN, 1);
    if (has_relic("ORNAMENTAL_FAN")) combat.add_power(PowerId::ORNAMENTAL_FAN, 4);
    // More hero relics
    if (has_relic("SELF_FORMING_CLAY")) combat.add_power(PowerId::SELF_FORMING_CLAY, 3);
    if (has_relic("CHARONS_ASHES")) combat.add_power(PowerId::CHARONS_ASHES, 3);
    if (has_relic("RED_SKULL")) combat.add_power(PowerId::RED_SKULL, 3);
    if (has_relic("DEMON_TONGUE")) combat.add_power(PowerId::DEMON_TONGUE);
    if (has_relic("BEATING_REMNANT")) combat.add_power(PowerId::BEATING_REMNANT, BEATING_REMNANT_HP_LOSS);
    // Vambrace: the first Block a card gives each combat is doubled.
    if (has_relic("VAMBRACE")) combat.add_power(PowerId::VAMBRACE, 1);
    if (has_relic("CHEMICAL_X")) combat.add_power(PowerId::CHEMICAL_X, CHEMICAL_X_INCREASE);
    // Unsettling Lamp: the first card of the fight that debuffs an enemy has its debuffs doubled.
    if (has_relic("UNSETTLING_LAMP")) combat.add_power(PowerId::UNSETTLING_LAMP, 1);
    if (has_relic("BRIMSTONE")) combat.add_power(PowerId::BRIMSTONE, 2);
    if (has_relic("SNECKO_SKULL")) combat.add_power(PowerId::SNECKO_SKULL, 1);
    if (has_relic("MINI_REGENT")) combat.add_power(PowerId::MINI_REGENT, 1);
    if (has_relic("GALACTIC_DUST")) combat.add_power(PowerId::GALACTIC_DUST, 10);
    if (has_relic("LUNAR_PASTRY")) combat.add_power(PowerId::LUNAR_PASTRY, 1);
    if (has_relic("REGALITE")) combat.add_power(PowerId::REGALITE, 4);
    if (has_relic("IVORY_TILE")) combat.add_power(PowerId::IVORY_TILE, 1);
    if (has_relic("PAELS_LEGION")) combat.add_power(PowerId::PAELS_LEGION, 2);
    if (has_relic("RINGING_TRIANGLE")) combat.add_power(PowerId::RINGING_TRIANGLE, 1);
    // Confused from the start: the opening hand already gets random costs.
    if (has_relic("FAKE_SNECKO_EYE")) combat.player.add_status(StatusType::CONFUSED, 1);
    if (has_relic("SNECKO_EYE")) {
        combat.add_power(PowerId::TURN_DRAW, 2);
        combat.player.add_status(StatusType::CONFUSED, 1);
    }
}

void RunManager::apply_combat_start_relics() {
    // Delicate Frond: fill every empty potion slot with a random potion.
    if (has_relic("DELICATE_FROND")) gain_random_potions(MAX_POTION_SLOTS);
    // Petrified Toad: procure a Potion-Shaped Rock at the start of each combat (lost if the belt is full).
    if (has_relic("PETRIFIED_TOAD") && !potion_belt_full()) potions.push_back("POTION_SHAPED_ROCK");
    // Vajra: start each combat with 1 Strength.
    if (has_relic("VAJRA")) {
        combat.player.add_status(StatusType::STRENGTH, 1);
    }
    // Girya: the Strength lifted at the rest sites, one point per lift.
    if (has_relic("GIRYA") && relic_counter("GIRYA") > 0) {
        combat.player.add_status(StatusType::STRENGTH, relic_counter("GIRYA"));
    }
    // Statuses to start the combat with. Given once the first turn has started, so that Plating does not lose a
    // point before it has given any Block.
    if (has_relic("BRONZE_SCALES")) combat.player.add_status(StatusType::THORNS, 3);
    if (has_relic("GORGET")) combat.player.add_status(StatusType::PLATED_ARMOR, 4);
    if (has_relic("AKABEKO")) combat.player.add_status(StatusType::VIGOR, 8);
    // Ring of the Snake (Silent): draw 2 additional cards on the first turn.
    if (has_relic("RING_OF_THE_SNAKE")) {
        combat.draw_cards(2);
    }
    // Bag of Preparation: 2 additional cards as well.
    if (has_relic("BAG_OF_PREPARATION")) {
        combat.draw_cards(2);
    }
    // Booming Conch (Ancient): in an Elite room, 2 additional cards on the first turn and 1 more energy.
    if (has_relic("BOOMING_CONCH") &&
        (current_node_index >= 0 ? map.nodes[current_node_index].type : RoomType::MONSTER) == RoomType::ELITE) {
        combat.draw_cards(2);
        combat.player.energy = static_cast<int8_t>(combat.player.energy + 1);
    }
    // Bellows: the first hand of the combat is upgraded, the cards of Ring of the Snake included.
    if (has_relic("BELLOWS")) {
        for (DeckCard& card : combat.hand) card.upgraded = true;
    }
    // Gambling Chip: any number of cards of the first hand are discarded, and as many are drawn.
    if (has_relic("GAMBLING_CHIP")) {
        combat.ask_choice(CardChoice{ ChoiceKind::DISCARD, 10, true, ChoiceThen::DRAW_PER_CARD });
    }
    // Ninja Scroll (Silent): 3 Shivs.
    if (has_relic("NINJA_SCROLL")) {
        combat.add_shivs(3);
    }
    // Vexing Puzzlebox: a random card of the hero's pool, free this turn unless a full hand sends it to the
    // discard pile.
    if (has_relic("VEXING_PUZZLEBOX")) {
        std::span<const CardId> pool = card_reward_pool(character);
        DeckCard gift(pool[next_random() % pool.size()]);
        gift.free_this_turn = combat.hand.size() < combat.hand.capacity();
        combat.create_card(gift, Pile::HAND);
    }
    // Toolbox: 3 random Colorless cards are offered, the one picked goes to the hand.
    if (has_relic("TOOLBOX")) {
        combat.offer_random_cards(CardFilter(CardPool::COLORLESS), 3);
    }
    // Jeweled Mask (ancient): a random Power of the draw pile goes to the hand and costs 0 for the combat.
    if (has_relic("JEWELED_MASK") && combat.hand.size() < combat.hand.capacity()) {
        FixedVector<uint8_t, 50> powers;
        for (size_t i = 0; i < combat.draw_pile.size(); ++i) {
            if (CardDef::get(combat.draw_pile[i].id).type == CardType::POWER) powers.push_back(static_cast<uint8_t>(i));
        }
        if (!powers.empty()) {
            size_t pick = powers[next_random() % powers.size()];
            DeckCard card = combat.draw_pile[pick];
            combat.draw_pile.erase(pick);
            uint8_t printed = CardDef::get_cost(card.id, card.upgraded);
            if (printed != COST_X) card.cost_delta = static_cast<int8_t>(card.cost_delta - printed);
            combat.hand.push_back(card);
        }
    }
    // Big Hat (Necrobinder): 2 random Ethereal cards of the hero's pool.
    if (has_relic("BIG_HAT")) {
        FixedVector<CardId, 128> ethereal;
        for (CardId id : card_reward_pool(character)) {
            if (CardDef::has_keyword(id, false, KEYWORD_ETHEREAL)) ethereal.push_back(id);
        }
        for (int n = 0; n < 2 && !ethereal.empty(); ++n) {
            combat.create_card(ethereal[next_random() % ethereal.size()], Pile::HAND);
        }
    }
    // Radiant Pearl (Orobas): 1 Luminesce in hand.
    if (has_relic("RADIANT_PEARL")) {
        combat.create_card(DeckCard(CardId::LUMINESCE), Pile::HAND);
    }
    // Fencing Manual (Regent): Forge 10.
    if (has_relic("FENCING_MANUAL")) {
        combat.forge(10);
    }
    // Divine Right (Regent): 3 Stars.
    if (has_relic("DIVINE_RIGHT")) {
        combat.gain_stars(3);
    }
    // Data Disk: 1 Focus. Runic Capacitor: 3 more orb slots. Symbiotic Virus: 1 Dark orb.
    if (has_relic("DATA_DISK")) {
        combat.player.add_status(StatusType::FOCUS, 1);
    }
    if (has_relic("RUNIC_CAPACITOR")) {
        combat.change_orb_slots(3);
    }
    if (has_relic("SYMBIOTIC_VIRUS")) {
        combat.channel_orb(OrbType::DARK);
    }
    // Power Cell: 2 random cards that cost 0 go from the draw pile to the hand.
    if (has_relic("POWER_CELL")) {
        for (int n = 0; n < 2 && combat.hand.size() < combat.hand.capacity(); ++n) {
            FixedVector<uint8_t, 50> free_cards;
            for (size_t i = 0; i < combat.draw_pile.size(); ++i) {
                const DeckCard& card = combat.draw_pile[i];
                if (CardDef::get_cost(card.id, card.upgraded) == 0) free_cards.push_back(static_cast<uint8_t>(i));
            }
            if (free_cards.empty()) break;
            size_t pick = free_cards[next_random() % free_cards.size()];
            combat.hand.push_back(combat.draw_pile[pick]);
            combat.draw_pile.erase(pick);
        }
    }
    // Cracked Core (Defect): channel 1 Lightning.
    if (has_relic("CRACKED_CORE")) {
        combat.channel_orb(OrbType::LIGHTNING);
    }
    // Anchor: start each combat with 10 Block, added after the turn reset and unaffected by Dexterity.
    if (has_relic("ANCHOR")) {
        combat.player.block += 10;
    }
    // Oddly Smooth Stone: start each combat with 1 Dexterity.
    if (has_relic("ODDLY_SMOOTH_STONE")) {
        combat.player.add_status(StatusType::DEXTERITY, 1);
    }
    // Lantern: 1 additional energy on the first turn.
    if (has_relic("LANTERN")) {
        combat.player.energy = static_cast<int8_t>(combat.player.energy + 1);
    }
    // Venerable Tea Set: 2 additional energy on the first turn for each Rest Site entered since the last combat.
    if (has_relic("VENERABLE_TEA_SET") && relic_counter("VENERABLE_TEA_SET") > 0) {
        combat.player.energy = static_cast<int8_t>(combat.player.energy + 2 * relic_counter("VENERABLE_TEA_SET"));
        set_relic_counter("VENERABLE_TEA_SET", 0);
    }
    // Blood Vial: heal 2 HP.
    if (has_relic("BLOOD_VIAL")) {
        combat.player.heal(2);
    }
    // Sling of Courage and Pantograph only act in Elite and Boss rooms.
    RoomType room = current_node_index >= 0 ? map.nodes[current_node_index].type : RoomType::MONSTER;
    if (has_relic("SLING_OF_COURAGE") && room == RoomType::ELITE) {
        combat.player.add_status(StatusType::STRENGTH, 2);
    }
    if (has_relic("PANTOGRAPH") && room == RoomType::BOSS) {
        combat.player.heal(25);
    }
    // Stone Cracker: upgrade 2 random cards of the draw pile for this combat (the piles are copies of the deck).
    if (has_relic("STONE_CRACKER")) {
        for (int n = 0; n < 2; ++n) {
            FixedVector<uint8_t, 50> plain;
            for (size_t i = 0; i < combat.draw_pile.size(); ++i) {
                if (!combat.draw_pile[i].upgraded) plain.push_back(static_cast<uint8_t>(i));
            }
            if (plain.empty()) break;
            combat.draw_pile[plain[next_random() % plain.size()]].upgraded = true;
        }
    }
    // Red Mask, Bag of Marbles and Festive Popper hit every enemy: 1 Weak, 1 Vulnerable, 9 damage.
    // So does Twisted Funnel (Silent): 4 Poison, which the powers hear about (Snecko Skull adds its own).
    for (Enemy& foe : combat.enemies) {
        if (has_relic("RED_MASK")) foe.add_status(StatusType::WEAK, 1);
        if (has_relic("BAG_OF_MARBLES")) foe.add_status(StatusType::VULNERABLE, 1);
        if (has_relic("TWISTED_FUNNEL")) combat.afflict(foe, StatusType::POISON, 4);
        if (has_relic("FESTIVE_POPPER")) foe.take_damage(9);
        // Philosopher's Stone (ancient): every enemy starts the combat with 1 Strength.
        if (has_relic("PHILOSOPHERS_STONE")) foe.add_status(StatusType::STRENGTH, 1);
    }
    combat.enemy = combat.enemies[0];
    if (!combat.has_alive_enemies()) {
        combat.state = CombatState::VICTORY;
    }
    belt_buckle_active = false;
    refresh_belt_buckle();
}

void RunManager::refresh_belt_buckle() {
    // Belt Buckle: 2 additional Dexterity while the potion belt is empty.
    if (!has_relic("BELT_BUCKLE")) return;
    bool should_be_active = potions.empty();
    if (should_be_active == belt_buckle_active) return;
    combat.player.add_status(StatusType::DEXTERITY, should_be_active ? 2 : -2);
    belt_buckle_active = should_be_active;
}

void RunManager::proceed_after_combat() {
    // Lizard Tail only works once: it leaves the run when it saved its owner.
    if (combat.power_counter(PowerId::LIZARD_TAIL) > 0) {
        for (size_t i = 0; i < relics.size(); ++i) {
            if (relics[i] == "LIZARD_TAIL") {
                relics.erase(i);
                break;
            }
        }
    }
    // Bonuses gained "permanently" during the fight go to the cards of the deck (Genetic Algorithm, The Scythe).
    for (const PermanentGain& gain : combat.permanent_gains) {
        if (gain.deck_index < master_deck.size()) {
            DeckCard& card = master_deck[gain.deck_index];
            card.bonus = static_cast<uint8_t>(std::min(255, card.bonus + gain.amount));
        }
    }
    combat.permanent_gains.clear();
    if (has_relic("SEAL_OF_GOLD")) gold = combat.gold;
    // What the cards of the fight leave to the run: gold (Hand of Greed, Royalties), potions (Alchemize), an extra
    // card reward (The Hunt), a card to remove from the deck (Forbidden Grimoire).
    const bool won = combat.state == CombatState::VICTORY;
    if (won) {
        gain_gold(static_cast<int16_t>(combat.gold_gained + combat.power_amount(PowerId::ROYALTIES)));
        gain_random_potions(combat.potions_gained);
        extra_card_rewards = combat.extra_card_rewards;
        if (combat.has_power(PowerId::FORBIDDEN_GRIMOIRE)) request_deck_choice(DeckChoiceKind::REMOVE, 1, true);
    }
    combat.gold_gained = 0;
    combat.potions_gained = 0;
    combat.extra_card_rewards = 0;
    // Gold taken by the cards left in hand at the end of a turn (curses).
    gold = static_cast<decltype(gold)>(std::max(0, static_cast<int>(gold) - combat.gold_lost));
    combat.gold_lost = 0;
    hp = combat.player.hp;
    max_hp = combat.player.max_hp; // cards can raise it for the rest of the run (Feed)
    // Check Ironclad Burning blood passive
    for (size_t i = 0; i < relics.size(); ++i) {
        if (relics[i] == "BURNING_BLOOD") {
            hp = std::min<int16_t>(max_hp, hp + 6);
        }
    }
    // Meat on the Bone: 12 HP for whoever leaves the fight with half their HP or less.
    if (has_relic("MEAT_ON_THE_BONE") && hp * 2 <= max_hp) {
        hp = std::min<int16_t>(max_hp, hp + 12);
    }

    // Every enemy ran away: the fight is over, but there is nothing to take.
    if (combat.every_enemy_escaped()) {
        phase = RunPhase::MAP_NAVIGATION;
        return;
    }

    // Lava Lamp: a fight won without losing HP upgrades all the card rewards it gives.
    card_rewards_upgraded = won && has_relic("LAVA_LAMP") && combat.counters.combat_hp_losses == 0;
    on_combat_won(map.nodes[current_node_index].type);

    if (map.nodes[current_node_index].type == RoomType::BOSS) {
        if (current_act < 3) {
            // Between acts the hero recovers every lost HP, only 75 % of them from Ascension 5.
            int16_t missing = max_hp - hp;
            hp += ascension_level >= 5 ? static_cast<int16_t>(missing * 3 / 4) : missing;
            // The next act opens once the rewards of the boss are taken (see finish_rewards).
            generate_rewards();
        } else {
            phase = RunPhase::VICTORY_ACT;
        }
    } else {
        generate_rewards();
    }
}

void RunManager::regenerate_map() {
    map.generate(current_act, map_seed, ascension_level, current_act == 2 && has_relic("GOLDEN_COMPASS"));
}

void RunManager::advance_act() {
    current_act++;
    current_floor = 1;
    current_node_index = -1;
    map_seed = next_random();
    regenerate_map();
    phase = RunPhase::ANCIENT_BLESSING; // Transition with Ancient encounter!
}

bool RunManager::is_run_completed() const {
    return phase == RunPhase::VICTORY_ACT;
}

std::span<const AscensionModifier> RunManager::get_ascension_modifiers() {
    return std::span<const AscensionModifier>(ASCENSION_MODIFIERS);
}

int16_t RunManager::get_enemy_damage_multiplier(RoomType room) const {
    uint8_t from = room == RoomType::BOSS ? 4 : (room == RoomType::ELITE ? 3 : 2);
    return ascension_level >= from ? 120 : 100;
}

int16_t RunManager::get_enemy_hp_multiplier(RoomType room) const {
    uint8_t from = room == RoomType::BOSS ? 9 : (room == RoomType::ELITE ? 8 : 7);
    return ascension_level >= from ? 115 : 100;
}

} // namespace engine
