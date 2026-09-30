#include "engine/run.h"
#include "engine/relic.h"
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

void RunManager::enter_room(int node_index) {
    current_node_index = node_index;
    const auto& node = map.nodes[node_index];
    current_floor = node.floor + 1;

    switch (node.type) {
        case RoomType::MONSTER:
        case RoomType::ELITE:
        case RoomType::BOSS: {
            phase = RunPhase::IN_COMBAT;
            FixedVector<EnemyType, 3> etypes;
            if (node.type == RoomType::BOSS) {
                etypes.push_back(EnemyType::CEREMONIAL_BEAST);
            } else if (node.type == RoomType::ELITE) {
                etypes.push_back(EnemyType::GREMLIN_NOB);
            } else if (current_floor > 3) {
                switch (next_random() % 3) {
                    case 0:
                        etypes.push_back(EnemyType::CULTIST);
                        etypes.push_back(EnemyType::LOUSE);
                        break;
                    case 1:
                        etypes.push_back(EnemyType::JAW_WORM);
                        break;
                    default:
                        etypes.push_back(EnemyType::LOUSE);
                        etypes.push_back(EnemyType::LOUSE);
                        break;
                }
            } else {
                etypes.push_back(EnemyType::CULTIST);
            }

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
                combat.draw_pile.push_back(master_deck[i]);
            }
            combat.shuffle_draw_pile();
            combat.start_player_turn();
            apply_combat_start_relics();
            break;
        }
        case RoomType::REST_SITE:
            phase = RunPhase::CAMPFIRE;
            break;
        case RoomType::TREASURE: {
            gold += 50;
            std::string_view relic = draw_treasure_relic();
            if (!relic.empty()) {
                add_relic(relic);
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
    static constexpr CardId IRONCLAD_POOL[] = { CardId::IRON_WAVE, CardId::CLEAVE, CardId::ANGER };
    static constexpr CardId SILENT_POOL[] = { CardId::POISON_STAB, CardId::DEADLY_POISON, CardId::SURVIVOR };
    static constexpr CardId DEFECT_POOL[] = { CardId::ZAP, CardId::DUALCAST, CardId::STRIKE };
    static constexpr CardId REGENT_POOL[] = { CardId::FALLING_STAR, CardId::VENERATE, CardId::DEFEND };
    static constexpr CardId NECROBINDER_POOL[] = { CardId::UNLEASH, CardId::BODYGUARD, CardId::STRIKE };

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
    FixedVector<CardId, 32> candidates;
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

std::string_view RunManager::draw_treasure_relic() {
    // Relics with an implemented effect come first, then any other common relic; never one already owned.
    constexpr std::string_view ACTIVE_RELICS[] = { "VAJRA", "ANCHOR" };
    FixedVector<std::string_view, 2> active;
    for (std::string_view id : ACTIVE_RELICS) {
        if (!has_relic(id)) active.push_back(id);
    }
    if (!active.empty()) {
        return active[next_random() % active.size()];
    }

    auto is_candidate = [this](const RelicData& relic) {
        return relic.rarity == RelicRarity::COMMON && relic.pool == "Shared" && !has_relic(relic.id);
    };
    std::span<const RelicData> all = get_all_relics();
    uint32_t count = 0;
    for (const RelicData& relic : all) {
        if (is_candidate(relic)) ++count;
    }
    if (count == 0) return {};

    uint32_t pick = next_random() % count;
    for (const RelicData& relic : all) {
        if (is_candidate(relic) && pick-- == 0) return relic.id;
    }
    return {};
}

void RunManager::apply_combat_start_relics() {
    // Vajra: start each combat with 1 Strength.
    if (has_relic("VAJRA")) {
        combat.player.add_status(StatusType::STRENGTH, 1);
    }
    // Ring of the Snake (Silent): draw 2 additional cards on the first turn.
    if (has_relic("RING_OF_THE_SNAKE")) {
        combat.draw_cards(2);
    }
    // Cracked Core (Defect): channel 1 Lightning.
    if (has_relic("CRACKED_CORE")) {
        combat.player.channel_orb(OrbType::LIGHTNING);
    }
    // Anchor: start each combat with 10 Block, added after the turn reset and unaffected by Dexterity.
    if (has_relic("ANCHOR")) {
        combat.player.block += 10;
    }
}

void RunManager::proceed_after_combat() {
    hp = combat.player.hp;
    // Check Ironclad Burning blood passive
    for (size_t i = 0; i < relics.size(); ++i) {
        if (relics[i] == "BURNING_BLOOD") {
            hp = std::min<int16_t>(max_hp, hp + 6);
        }
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
