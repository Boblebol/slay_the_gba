#include "engine/combat.h"
#include <algorithm>

namespace engine {

uint32_t CombatEngine::next_random() {
    // 32-bit XorShift
    rng_state_ ^= rng_state_ << 13;
    rng_state_ ^= rng_state_ >> 17;
    rng_state_ ^= rng_state_ << 5;
    return rng_state_;
}

void CombatEngine::set_log(std::string_view msg) {
    size_t i = 0;
    for (; i < msg.size() && i + 1 < sizeof(combat_log); ++i) {
        combat_log[i] = msg[i];
    }
    combat_log[i] = '\0';
}

bool CombatEngine::has_alive_enemies() const {
    for (size_t i = 0; i < enemies.size(); ++i) {
        if (enemies[i].is_alive()) return true;
    }
    return false;
}

int CombatEngine::count_alive_enemies() const {
    int count = 0;
    for (size_t i = 0; i < enemies.size(); ++i) {
        if (enemies[i].is_alive()) ++count;
    }
    return count;
}

int CombatEngine::get_first_alive_enemy_index() const {
    for (size_t i = 0; i < enemies.size(); ++i) {
        if (enemies[i].is_alive()) return static_cast<int>(i);
    }
    return 0;
}

void CombatEngine::init_starter_deck(CharacterId character) {
    draw_pile.clear();
    hand.clear();
    discard_pile.clear();
    exhaust_pile.clear();

    switch (character) {
        case CharacterId::IRONCLAD:
            for (int i = 0; i < 5; ++i) draw_pile.push_back(CardId::STRIKE);
            for (int i = 0; i < 4; ++i) draw_pile.push_back(CardId::DEFEND);
            draw_pile.push_back(CardId::BASH);
            break;
        case CharacterId::SILENT:
            for (int i = 0; i < 5; ++i) draw_pile.push_back(CardId::STRIKE);
            for (int i = 0; i < 5; ++i) draw_pile.push_back(CardId::DEFEND);
            draw_pile.push_back(CardId::NEUTRALIZE);
            draw_pile.push_back(CardId::SURVIVOR);
            break;
        case CharacterId::DEFECT:
            for (int i = 0; i < 4; ++i) draw_pile.push_back(CardId::STRIKE);
            for (int i = 0; i < 4; ++i) draw_pile.push_back(CardId::DEFEND);
            draw_pile.push_back(CardId::ZAP);
            draw_pile.push_back(CardId::DUALCAST);
            break;
        case CharacterId::REGENT:
            for (int i = 0; i < 4; ++i) draw_pile.push_back(CardId::STRIKE);
            for (int i = 0; i < 4; ++i) draw_pile.push_back(CardId::DEFEND);
            draw_pile.push_back(CardId::FALLING_STAR);
            draw_pile.push_back(CardId::VENERATE);
            break;
        case CharacterId::NECROBINDER:
            for (int i = 0; i < 4; ++i) draw_pile.push_back(CardId::STRIKE);
            for (int i = 0; i < 4; ++i) draw_pile.push_back(CardId::DEFEND);
            draw_pile.push_back(CardId::UNLEASH);
            draw_pile.push_back(CardId::BODYGUARD);
            break;
        default:
            break;
    }

    shuffle_draw_pile();
}

void CombatEngine::shuffle_draw_pile() {
    if (draw_pile.size() <= 1) return;
    for (size_t i = draw_pile.size() - 1; i > 0; --i) {
        size_t j = next_random() % (i + 1);
        DeckCard temp = draw_pile[i];
        draw_pile[i] = draw_pile[j];
        draw_pile[j] = temp;
    }
}

void CombatEngine::shuffle_discard_into_draw() {
    while (!discard_pile.empty()) {
        draw_pile.push_back(discard_pile.back());
        discard_pile.pop_back();
    }
    shuffle_draw_pile();
}

void CombatEngine::draw_card() {
    if (hand.size() >= hand.capacity()) return; // Hand limit

    if (draw_pile.empty()) {
        shuffle_discard_into_draw();
    }

    if (!draw_pile.empty()) {
        DeckCard card = draw_pile.back();
        draw_pile.pop_back();
        hand.push_back(card);
    }
}

void CombatEngine::draw_cards(int count) {
    for (int i = 0; i < count; ++i) {
        draw_card();
    }
}

void CombatEngine::start_combat(EnemyType enemy_type, CharacterId character) {
    FixedVector<EnemyType, 3> single_list;
    single_list.push_back(enemy_type);
    start_combat_multi(single_list, character);
}

void CombatEngine::start_combat_multi(FixedVector<EnemyType, 3> enemy_types, CharacterId character) {
    current_character = character;
    const auto& cdef = get_character(character);
    player.init();
    player.hp = cdef.starting_hp;
    player.max_hp = cdef.starting_hp;

    enemies.clear();
    for (size_t i = 0; i < enemy_types.size(); ++i) {
        Enemy e;
        e.init(enemy_types[i]);
        enemies.push_back(e);
    }
    if (enemies.empty()) {
        Enemy e;
        e.init(EnemyType::CULTIST);
        enemies.push_back(e);
    }
    enemy = enemies[0];
    selected_target_index = 0;

    turn_number = 1;
    selected_card_index = 0;
    init_starter_deck(character);
    state = CombatState::PLAYER_TURN;
    
    start_player_turn();
    set_log("Combat started!");
}

void CombatEngine::start_player_turn() {
    state = CombatState::PLAYER_TURN;
    player.apply_poison_tick();
    if (!player.is_alive()) {
        state = CombatState::DEFEAT;
        set_log("You were defeated!");
        return;
    }
    player.reset_turn();
    player.trigger_start_of_turn_orbs();
    draw_cards(5);
    selected_card_index = 0;
    selected_target_index = get_first_alive_enemy_index();
}

bool CombatEngine::can_play_card(int index) const {
    if (state != CombatState::PLAYER_TURN) return false;
    if (index < 0 || static_cast<size_t>(index) >= hand.size()) return false;

    uint8_t cost = CardDef::get_cost(hand[index].id, hand[index].upgraded);
    return player.energy >= cost;
}

bool CombatEngine::play_card(int index, int target_index) {
    if (!can_play_card(index)) return false;

    DeckCard played_card = hand[index];
    CardId card_id = played_card.id;
    bool upgraded = played_card.upgraded;
    uint8_t cost = CardDef::get_cost(card_id, upgraded);
    uint8_t base_value = CardDef::get_value(card_id, upgraded);
    uint8_t secondary_value = CardDef::get_secondary(card_id, upgraded);

    // Resolve target
    int target_idx = target_index;
    if (target_idx < 0 || target_idx >= static_cast<int>(enemies.size()) || !enemies[target_idx].is_alive()) {
        target_idx = selected_target_index;
        if (target_idx < 0 || target_idx >= static_cast<int>(enemies.size()) || !enemies[target_idx].is_alive()) {
            target_idx = get_first_alive_enemy_index();
        }
    }
    Enemy& current_target = (target_idx >= 0 && target_idx < static_cast<int>(enemies.size())) ? enemies[target_idx] : enemy;

    // Pay energy
    player.energy -= cost;

    // Remove from hand
    hand.erase(index);

    // Apply effects
    switch (card_id) {
        case CardId::STRIKE: {
            current_target.take_damage(attack_damage(base_value + player.sovereign_stars, player, current_target));
            set_log(upgraded ? "Played Strike+!" : "Played Strike!");
            break;
        }
        case CardId::DEFEND: {
            player.gain_block(base_value + player.sovereign_stars);
            set_log(upgraded ? "Played Defend+!" : "Played Defend!");
            break;
        }
        case CardId::BASH: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Bash+!" : "Played Bash!");
            break;
        }
        case CardId::IRON_WAVE: {
            player.gain_block(base_value);
            current_target.take_damage(attack_damage(secondary_value, player, current_target));
            set_log(upgraded ? "Played Iron Wave+!" : "Played Iron Wave!");
            break;
        }
        case CardId::CLEAVE: {
            // AoE: attacks ALL alive enemies!
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    enemies[i].take_damage(attack_damage(base_value, player, enemies[i]));
                }
            }
            set_log(upgraded ? "Played Cleave+!" : "Played Cleave!");
            break;
        }
        case CardId::ANGER: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            discard_pile.push_back(DeckCard(CardId::ANGER, upgraded)); // Extra copy!
            set_log(upgraded ? "Played Anger+!" : "Played Anger!");
            break;
        }
        case CardId::METALLICIZE: {
            player.add_status(StatusType::METALLICIZE, base_value);
            set_log(upgraded ? "Played Metallicize+!" : "Played Metallicize!");
            break;
        }
        case CardId::NEUTRALIZE: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::WEAK, secondary_value);
            set_log(upgraded ? "Played Neutralize+!" : "Played Neutralize!");
            break;
        }
        case CardId::SURVIVOR: {
            player.gain_block(base_value);
            set_log(upgraded ? "Played Survivor+!" : "Played Survivor!");
            break;
        }
        case CardId::POISON_STAB: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::POISON, secondary_value);
            set_log(upgraded ? "Played Poison Stab+!" : "Played Poison Stab!");
            break;
        }
        case CardId::DEADLY_POISON: {
            current_target.add_status(StatusType::POISON, secondary_value);
            set_log(upgraded ? "Played Deadly Poison+!" : "Played Deadly Poison!");
            break;
        }
        case CardId::ZAP: {
            player.channel_orb(OrbType::LIGHTNING, &current_target);
            set_log(upgraded ? "Channeled Lightning (Zap+)!" : "Channeled Lightning (Zap)!");
            break;
        }
        case CardId::DUALCAST: {
            player.evoke_oldest_orb(&current_target, 2);
            set_log(upgraded ? "Triggered Dualcast+!" : "Triggered Dualcast!");
            break;
        }
        case CardId::FALLING_STAR: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            player.add_stars(secondary_value);
            set_log(upgraded ? "Cast Falling Star+!" : "Cast Falling Star!");
            break;
        }
        case CardId::VENERATE: {
            player.gain_block(base_value);
            player.add_stars(secondary_value);
            set_log(upgraded ? "Cast Venerate+!" : "Cast Venerate!");
            break;
        }
        case CardId::UNLEASH: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            player.souls += secondary_value;
            set_log(upgraded ? "Cast Unleash+!" : "Cast Unleash!");
            break;
        }
        case CardId::BODYGUARD: {
            player.gain_block(base_value);
            player.summon_minion(secondary_value, 0);
            set_log(upgraded ? "Summoned Osty (Bodyguard+)!" : "Summoned Osty (Bodyguard)!");
            break;
        }
        case CardId::TWIN_STRIKE: {
            for (uint8_t hit = 0; hit < secondary_value; ++hit) {
                current_target.take_damage(attack_damage(base_value, player, current_target));
            }
            set_log(upgraded ? "Played Twin Strike+!" : "Played Twin Strike!");
            break;
        }
        case CardId::POMMEL_STRIKE: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Pommel Strike+!" : "Played Pommel Strike!");
            break;
        }
        case CardId::SHRUG_IT_OFF: {
            player.gain_block(base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Shrug It Off+!" : "Played Shrug It Off!");
            break;
        }
        case CardId::TAUNT: {
            player.gain_block(base_value);
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Taunt+!" : "Played Taunt!");
            break;
        }
        default:
            set_log("Played card!");
            break;
    }

    // Keep primary enemy in sync
    if (!enemies.empty()) {
        enemy = enemies[0];
    }

    // Powers stay in play for the rest of the combat: they never return to the piles.
    if (CardDef::get(card_id).type != CardType::POWER) {
        discard_pile.push_back(played_card);
    }

    // Check victory
    if (!has_alive_enemies()) {
        state = CombatState::VICTORY;
        set_log("Victory!");
    } else {
        selected_target_index = get_first_alive_enemy_index();
    }

    return true;
}

void CombatEngine::end_player_turn() {
    if (state != CombatState::PLAYER_TURN) return;

    // Discard hand
    while (!hand.empty()) {
        discard_pile.push_back(hand.back());
        hand.pop_back();
    }

    // End of turn effects: Metallicize block, then orb passives on the first living enemy.
    player.apply_metallicize_tick();
    if (has_alive_enemies()) {
        player.trigger_passive_orbs(&enemies[get_first_alive_enemy_index()]);
    }
    if (!enemies.empty()) {
        enemy = enemies[0];
    }
    if (!has_alive_enemies()) {
        state = CombatState::VICTORY;
        set_log("Victory!");
        return;
    }

    state = CombatState::ENEMY_TURN;
    set_log("Enemy Turn...");

    execute_enemy_turn();
}

void CombatEngine::execute_enemy_turn() {
    last_enemy_actions.clear();
    for (size_t i = 0; i < enemies.size(); ++i) {
        // Poison hits at the start of the poisoned enemy's turn and can kill it before it acts.
        if (enemies[i].is_alive()) {
            enemies[i].apply_poison_tick();
        }
        if (enemies[i].is_alive()) {
            EnemyActionRecord record;
            record.enemy_index = static_cast<int8_t>(i);
            record.intent_type = enemies[i].current_intent.type;
            const int16_t hp_before = player.hp;
            const int16_t shield_before = player.block + player.minion_hp + player.minion_block;

            enemies[i].execute_turn(player);

            record.damage_to_player = hp_before - player.hp;
            record.damage_blocked = shield_before - (player.block + player.minion_hp + player.minion_block);
            record.block_gained = enemies[i].block; // execute_turn() clears block before gaining it
            record.player_hp_after = player.hp;
            last_enemy_actions.push_back(record);

            if (!player.is_alive()) {
                state = CombatState::DEFEAT;
                set_log("You were defeated!");
                return;
            }
        }
    }

    // Keep primary enemy in sync
    if (!enemies.empty()) {
        enemy = enemies[0];
    }

    if (!has_alive_enemies()) {
        state = CombatState::VICTORY;
        set_log("Victory!");
        return;
    }

    // Player debuffs (Weak, Vulnerable) wear off at the end of the round, after the enemies acted.
    player.tick_end_of_turn_statuses();

    // Plan next intents for all alive enemies
    turn_number++;
    for (size_t i = 0; i < enemies.size(); ++i) {
        if (enemies[i].is_alive()) {
            enemies[i].plan_turn(turn_number);
        }
    }

    // Start player turn again
    start_player_turn();
    set_log("Your Turn!");
}

bool CombatEngine::use_potion(std::string_view potion_id, int target_index) {
    if (state != CombatState::PLAYER_TURN) return false;

    if (potion_id == "FIRE_POTION") {
        if (target_index < 0 || target_index >= static_cast<int>(enemies.size()) || !enemies[target_index].is_alive()) {
            target_index = get_first_alive_enemy_index();
        }
        enemies[target_index].take_damage(20);
        set_log("Fire Potion!");
    } else if (potion_id == "BLOCK_POTION") {
        player.block += 12; // Potion Block is not modified by Dexterity
        set_log("Block Potion!");
    } else if (potion_id == "ENERGY_POTION") {
        player.energy = static_cast<int8_t>(player.energy + 2);
        set_log("Energy Potion!");
    } else if (potion_id == "BLOOD_POTION") {
        player.heal(player.max_hp / 5);
        set_log("Blood Potion!");
    } else {
        return false;
    }

    // Keep primary enemy in sync
    if (!enemies.empty()) {
        enemy = enemies[0];
    }

    if (!has_alive_enemies()) {
        state = CombatState::VICTORY;
        set_log("Victory!");
    } else if (selected_target_index < 0 || selected_target_index >= static_cast<int>(enemies.size()) ||
               !enemies[selected_target_index].is_alive()) {
        selected_target_index = get_first_alive_enemy_index();
    }

    return true;
}

} // namespace engine
