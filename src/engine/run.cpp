#include "engine/run.h"
#include "engine/relic.h"
#include "engine/encounter.h"
#include <algorithm>

namespace engine {

namespace {

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
    const Encounter& encounter = table.size() == 1 ? table[0] : table[next_random() % table.size()];
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
            for (Enemy& foe : combat.enemies) {
                foe.max_hp = static_cast<int16_t>(foe.max_hp * hp_percent / 100);
                foe.hp = foe.max_hp;
                foe.damage_percent = get_enemy_damage_multiplier(node.type);
            }
            combat.enemy = combat.enemies[0];

            // Replace combat piles with current master deck
            combat.draw_pile.clear();
            combat.hand.clear();
            combat.discard_pile.clear();
            combat.exhaust_pile.clear();
            for (size_t i = 0; i < master_deck.size(); ++i) {
                DeckCard card = master_deck[i];
                card.deck_index = static_cast<uint8_t>(i);
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
            gold += 50;
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

void RunManager::rest_at_campfire() {
    if (can_rest()) {
        int16_t heal_amount = max_hp * 3 / 10;
        hp = std::min<int16_t>(max_hp, hp + heal_amount);
    }
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
        master_deck.erase(deck_index);
        return true;
    }
    return false;
}

void RunManager::open_shop() {
    current_shop.generate(character, next_random(), card_removals_count);
    // Relics already owned are shown as sold out: a relic is never bought twice.
    for (size_t i = 0; i < current_shop.relics.size(); ++i) {
        if (has_relic(current_shop.relics[i].relic_id)) {
            current_shop.relics[i].purchased = true;
        }
    }
    phase = RunPhase::SHOP;
}

bool RunManager::buy_shop_card(int index) {
    return current_shop.buy_card(*this, index);
}

bool RunManager::buy_shop_relic(int index) {
    return current_shop.buy_relic(*this, index);
}

bool RunManager::buy_shop_potion(int index) {
    return current_shop.buy_potion(*this, index);
}

bool RunManager::use_potion(int index, int target_index) {
    if (phase != RunPhase::IN_COMBAT) return false;
    if (index < 0 || static_cast<size_t>(index) >= potions.size()) return false;
    if (!combat.use_potion(potions[index], target_index)) return false;
    potions.erase(index);
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
                                                CardId::STONE_ARMOR, CardId::FLAME_BARRIER };
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
                                              CardId::EXPOSE };
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
                                              CardId::SWORD_SAGE, CardId::THE_SMITH };
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
                                                   CardId::SENTRY_MODE };

    switch (char_id) {
        case CharacterId::SILENT: return SILENT_POOL;
        case CharacterId::DEFECT: return DEFECT_POOL;
        case CharacterId::REGENT: return REGENT_POOL;
        case CharacterId::NECROBINDER: return NECROBINDER_POOL;
        default: return IRONCLAD_POOL;
    }
}

void RunManager::generate_rewards() {
    // Offer distinct cards drawn at random from the hero's pool.
    current_card_rewards.clear();
    FixedVector<CardId, 128> candidates;
    for (CardId id : card_reward_pool(character)) {
        candidates.push_back(id);
    }
    while (current_card_rewards.size() < current_card_rewards.capacity() && !candidates.empty()) {
        size_t pick = next_random() % candidates.size();
        current_card_rewards.push_back(DeckCard(candidates[pick], false));
        candidates.erase(pick);
    }

    gold_reward = 15 + static_cast<int16_t>(next_random() % 15);
    phase = RunPhase::REWARD_SCREEN;
}

void RunManager::pick_card_reward(int reward_index) {
    if (reward_index >= 0 && static_cast<size_t>(reward_index) < current_card_rewards.size()) {
        master_deck.push_back(current_card_rewards[reward_index]);
    }
    gold += gold_reward;
    gold_reward = 0;
    phase = RunPhase::MAP_NAVIGATION;
}

void RunManager::skip_card_rewards() {
    gold += gold_reward;
    gold_reward = 0;
    phase = RunPhase::MAP_NAVIGATION;
}

void RunManager::add_relic(std::string_view relic_id) {
    if (relics.size() < relics.capacity()) {
        relics.push_back(relic_id);
    }
}

bool RunManager::has_relic(std::string_view relic_id) const {
    for (size_t i = 0; i < relics.size(); ++i) {
        if (relics[i] == relic_id) return true;
    }
    return false;
}

std::span<const std::string_view> RunManager::active_relics() {
    // Every implemented relic that drops normally (Common, Uncommon, Rare, Shop); Starter, Boss, Event and Ancient
    // relics come from elsewhere. Shop relics are in the list too: chests give them until the GBA shop sells relics (#184).
    static constexpr std::string_view ACTIVE_RELICS[] = {
        "VAJRA", "ANCHOR", "BLOOD_VIAL", "LANTERN", "RED_MASK", "ODDLY_SMOOTH_STONE", "FESTIVE_POPPER", "BAG_OF_MARBLES",
        "STONE_CRACKER", "PANTOGRAPH", "SLING_OF_COURAGE",
        // Start and end of turn
        "CAPTAINS_WHEEL", "HORN_CLEAT", "CANDELABRA", "CHANDELIER", "SPARKLING_ROUGE", "HAPPY_FLOWER", "PENDULUM", "BREAD",
        "MERCURY_HOURGLASS", "CLOAK_CLASP", "POCKETWATCH", "PARRYING_SHIELD", "STONE_CALENDAR", "ART_OF_WAR", "ORICHALCUM",
        "SCREAMING_FLAGON", "ICE_CREAM", "STURDY_CLAMP",
        // Combat events
        "GREMLIN_HORN", "RAZOR_TOOTH", "THE_ABACUS", "PERMAFROST", "GAME_PIECE", "CENTENNIAL_PUZZLE", "LIZARD_TAIL",
        "BELLOWS", "UNCEASING_TOP", "TOUGH_BANDAGES", "TINGSHA", "GAMBLING_CHIP", "HELICAL_DART", "NINJA_SCROLL",
        // Defect and Necrobinder
        "DATA_DISK", "SYMBIOTIC_VIRUS", "GOLD_PLATED_CABLES", "METRONOME", "RUNIC_CAPACITOR", "POWER_CELL", "EMOTION_CHIP",
        "BOOK_REPAIR_KNIFE", "BONE_FLUTE",
        // On the combat hooks (#299)
        "BAG_OF_PREPARATION", "BURNING_STICKS", "INTIMIDATING_HELMET", "JOSS_PAPER", "KUNAI", "KUSARIGAMA", "LETTER_OPENER",
        "MEAT_ON_THE_BONE", "MUMMIFIED_HAND", "NUNCHAKU", "ORNAMENTAL_FAN", "RAINBOW_RING", "RIPPLE_BASIN", "SHURIKEN",
        "TUNING_FORK", "VEXING_PUZZLEBOX",
        "BRIMSTONE", "CHARONS_ASHES", "DEMON_TONGUE", "RED_SKULL", "SELF_FORMING_CLAY", // Ironclad
        "SNECKO_SKULL", "TWISTED_FUNNEL",                                                // Silent
        "GALACTIC_DUST", "LUNAR_PASTRY", "MINI_REGENT", "REGALITE", "FENCING_MANUAL",   // Regent
        "BIG_HAT", "IVORY_TILE",                                                         // Necrobinder
        // Statuses given at the start of the combat (#91, #93)
        "BRONZE_SCALES", "GORGET", "AKABEKO"
    };
    return ACTIVE_RELICS;
}

std::string_view RunManager::draw_treasure_relic() {
    // A relic with an implemented effect, shared or of this hero, never one already owned.
    std::string_view hero_pool = get_character(character).english_name;
    auto is_candidate = [&](std::string_view id) {
        const RelicData* relic = find_relic(id);
        return relic && (relic->pool == "Shared" || relic->pool == hero_pool) && !has_relic(id);
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

void RunManager::register_relic_powers() {
    // Blessed Antler: one more energy each turn, paid for with three Dazed shuffled into the draw pile.
    if (has_relic("BLESSED_ANTLER")) {
        combat.add_power(PowerId::TURN_ENERGY, 1);
        for (int copy = 0; copy < 3; ++copy) combat.create_card(CardId::DAZED, Pile::DRAW);
    }
    // Start of turn
    if (has_relic("CAPTAINS_WHEEL")) combat.add_power(PowerId::CAPTAINS_WHEEL, 18);
    if (has_relic("HORN_CLEAT")) combat.add_power(PowerId::HORN_CLEAT, 14);
    if (has_relic("CANDELABRA")) combat.add_power(PowerId::CANDELABRA, 2);
    if (has_relic("CHANDELIER")) combat.add_power(PowerId::CHANDELIER, 3);
    if (has_relic("SPARKLING_ROUGE")) combat.add_power(PowerId::SPARKLING_ROUGE, 1);
    if (has_relic("HAPPY_FLOWER")) combat.add_power(PowerId::HAPPY_FLOWER, 1);
    if (has_relic("PENDULUM")) combat.add_power(PowerId::PENDULUM, 1);
    if (has_relic("BREAD")) combat.add_power(PowerId::BREAD, 1);
    if (has_relic("MERCURY_HOURGLASS")) combat.add_power(PowerId::MERCURY_HOURGLASS, 3);
    // End of turn
    if (has_relic("CLOAK_CLASP")) combat.add_power(PowerId::CLOAK_CLASP, 1);
    if (has_relic("POCKETWATCH")) combat.add_power(PowerId::POCKETWATCH, 3);
    if (has_relic("PARRYING_SHIELD")) combat.add_power(PowerId::PARRYING_SHIELD, 6);
    if (has_relic("STONE_CALENDAR")) combat.add_power(PowerId::STONE_CALENDAR, 52);
    if (has_relic("ART_OF_WAR")) combat.add_power(PowerId::ART_OF_WAR, 1);
    if (has_relic("ORICHALCUM")) combat.add_power(PowerId::ORICHALCUM, 6);
    if (has_relic("SCREAMING_FLAGON")) combat.add_power(PowerId::SCREAMING_FLAGON, 20);
    // Kept from one turn to the next
    if (has_relic("ICE_CREAM")) combat.add_power(PowerId::ICE_CREAM);
    if (has_relic("STURDY_CLAMP")) combat.add_power(PowerId::STURDY_CLAMP, 10);
    // Combat events
    if (has_relic("GREMLIN_HORN")) combat.add_power(PowerId::GREMLIN_HORN, 1);
    if (has_relic("RAZOR_TOOTH")) combat.add_power(PowerId::RAZOR_TOOTH);
    if (has_relic("THE_ABACUS")) combat.add_power(PowerId::THE_ABACUS, 6);
    if (has_relic("PERMAFROST")) combat.add_power(PowerId::PERMAFROST, 7);
    if (has_relic("GAME_PIECE")) combat.add_power(PowerId::GAME_PIECE, 1);
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
    if (has_relic("BRIMSTONE")) combat.add_power(PowerId::BRIMSTONE, 2);
    if (has_relic("SNECKO_SKULL")) combat.add_power(PowerId::SNECKO_SKULL, 1);
    if (has_relic("MINI_REGENT")) combat.add_power(PowerId::MINI_REGENT, 1);
    if (has_relic("GALACTIC_DUST")) combat.add_power(PowerId::GALACTIC_DUST, 10);
    if (has_relic("LUNAR_PASTRY")) combat.add_power(PowerId::LUNAR_PASTRY, 1);
    if (has_relic("REGALITE")) combat.add_power(PowerId::REGALITE, 4);
    if (has_relic("IVORY_TILE")) combat.add_power(PowerId::IVORY_TILE, 1);
    // Confused from the start: the opening hand already gets random costs.
    if (has_relic("FAKE_SNECKO_EYE")) combat.player.add_status(StatusType::CONFUSED, 1);
    if (has_relic("SNECKO_EYE")) {
        combat.add_power(PowerId::TURN_DRAW, 2);
        combat.player.add_status(StatusType::CONFUSED, 1);
    }
}

void RunManager::apply_combat_start_relics() {
    // Vajra: start each combat with 1 Strength.
    if (has_relic("VAJRA")) {
        combat.player.add_status(StatusType::STRENGTH, 1);
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
    }
    combat.enemy = combat.enemies[0];
    if (!combat.has_alive_enemies()) {
        combat.state = CombatState::VICTORY;
    }
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

    if (map.nodes[current_node_index].type == RoomType::BOSS) {
        if (current_act < 3) {
            // Between acts the hero recovers every lost HP, only 75 % of them from Ascension 5.
            int16_t missing = max_hp - hp;
            hp += ascension_level >= 5 ? static_cast<int16_t>(missing * 3 / 4) : missing;
            advance_act();
        } else {
            phase = RunPhase::VICTORY_ACT;
        }
    } else {
        generate_rewards();
    }
}

void RunManager::advance_act() {
    current_act++;
    current_floor = 1;
    current_node_index = -1;
    map_seed = next_random();
    map.generate(current_act, map_seed, ascension_level);
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
