#include "engine/run.h"
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
    map.generate(current_act, next_random());
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
                etypes.push_back(EnemyType::CULTIST);
            } else if (node.type == RoomType::ELITE) {
                etypes.push_back(EnemyType::JAW_WORM);
                etypes.push_back(EnemyType::LOUSE);
            } else {
                if (current_floor > 3 && (next_random() % 2 == 0)) {
                    etypes.push_back(EnemyType::CULTIST);
                    etypes.push_back(EnemyType::LOUSE);
                } else {
                    etypes.push_back(EnemyType::CULTIST);
                }
            }

            // Start combat with current HP and master deck
            combat.start_combat_multi(etypes, character);
            combat.player.hp = hp;
            combat.player.max_hp = max_hp;

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
            break;
        }
        case RoomType::REST_SITE:
            phase = RunPhase::CAMPFIRE;
            break;
        case RoomType::TREASURE:
            gold += 50;
            add_relic("VAJRA"); // Gain powerful relic in chest
            phase = RunPhase::TREASURE;
            break;
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

void RunManager::generate_rewards() {
    current_card_rewards.clear();
    switch (character) {
        case CharacterId::SILENT:
            current_card_rewards.push_back(DeckCard(CardId::POISON_STAB, false));
            current_card_rewards.push_back(DeckCard(CardId::DEADLY_POISON, false));
            current_card_rewards.push_back(DeckCard(CardId::SURVIVOR, false));
            break;
        case CharacterId::DEFECT:
            current_card_rewards.push_back(DeckCard(CardId::ZAP, false));
            current_card_rewards.push_back(DeckCard(CardId::DUALCAST, false));
            current_card_rewards.push_back(DeckCard(CardId::STRIKE, false));
            break;
        case CharacterId::REGENT:
            current_card_rewards.push_back(DeckCard(CardId::FALLING_STAR, false));
            current_card_rewards.push_back(DeckCard(CardId::VENERATE, false));
            current_card_rewards.push_back(DeckCard(CardId::DEFEND, false));
            break;
        case CharacterId::NECROBINDER:
            current_card_rewards.push_back(DeckCard(CardId::UNLEASH, false));
            current_card_rewards.push_back(DeckCard(CardId::BODYGUARD, false));
            current_card_rewards.push_back(DeckCard(CardId::STRIKE, false));
            break;
        default: // IRONCLAD
            current_card_rewards.push_back(DeckCard(CardId::IRON_WAVE, false));
            current_card_rewards.push_back(DeckCard(CardId::CLEAVE, false));
            current_card_rewards.push_back(DeckCard(CardId::ANGER, false));
            break;
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
    map.generate(current_act, next_random());
    phase = RunPhase::ANCIENT_BLESSING; // Transition with Ancient encounter!
}

bool RunManager::is_run_completed() const {
    return phase == RunPhase::VICTORY_ACT;
}

std::span<const AscensionModifier> RunManager::get_ascension_modifiers() {
    return std::span<const AscensionModifier>(ASCENSION_MODIFIERS);
}

int16_t RunManager::get_enemy_damage_multiplier() const {
    return (ascension_level >= 2) ? 120 : 100;
}

int16_t RunManager::get_enemy_hp_multiplier() const {
    return (ascension_level >= 7) ? 115 : 100;
}

} // namespace engine
