#include "engine/combat.h"
#include <algorithm>

namespace engine {

// Log messages are picked at run time between two literals: the "sv" suffix keeps their length a compile-time
// constant, where a plain literal would call strlen, which the GBA build does not link.
using namespace std::string_view_literals;

namespace {

// Cards whose French name contains "Frappe" (Perfected Strike counts them).
bool is_strike(CardId id) {
    switch (id) {
        case CardId::STRIKE:
        case CardId::TWIN_STRIKE:
        case CardId::POMMEL_STRIKE:
        case CardId::SOLAR_STRIKE:
        case CardId::PERFECTED_STRIKE:
        case CardId::METEOR_STRIKE:
            return true;
        default:
            return false;
    }
}

template <size_t Capacity>
int count_strikes(const FixedVector<DeckCard, Capacity>& pile) {
    int count = 0;
    for (const DeckCard& card : pile) count += is_strike(card.id);
    return count;
}

bool retains(const DeckCard& card) {
    return CardDef::has_keyword(card.id, card.upgraded, KEYWORD_RETAIN);
}

} // namespace

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

int CombatEngine::random_alive_enemy_index() {
    int alive = count_alive_enemies();
    if (alive == 0) return 0;
    int pick = static_cast<int>(next_random() % static_cast<uint32_t>(alive));
    for (size_t i = 0; i < enemies.size(); ++i) {
        if (enemies[i].is_alive() && pick-- == 0) return static_cast<int>(i);
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

bool CombatEngine::draw_card() {
    if (hand.size() >= hand.capacity()) return false; // Hand limit

    if (draw_pile.empty()) {
        shuffle_discard_into_draw();
    }

    if (draw_pile.empty()) return false;

    DeckCard card = draw_pile.back();
    draw_pile.pop_back();
    hand.push_back(card);
    return true;
}

void CombatEngine::exhaust_card(DeckCard card) {
    if (exhaust_pile.size() < exhaust_pile.capacity()) {
        exhaust_pile.push_back(card);
    }
    if (exhausted_this_turn < UINT8_MAX) ++exhausted_this_turn;
}

DeckCard CombatEngine::exhaust_from_hand(size_t index) {
    DeckCard card = hand[index];
    hand.erase(index);
    exhaust_card(card);
    return card;
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
    exhausted_this_turn = 0;
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
            set_log(upgraded ? "Played Strike+!"sv : "Played Strike!"sv);
            break;
        }
        case CardId::DEFEND: {
            player.gain_block(base_value + player.sovereign_stars);
            set_log(upgraded ? "Played Defend+!"sv : "Played Defend!"sv);
            break;
        }
        case CardId::BASH: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Bash+!"sv : "Played Bash!"sv);
            break;
        }
        case CardId::IRON_WAVE: {
            player.gain_block(base_value);
            current_target.take_damage(attack_damage(secondary_value, player, current_target));
            set_log(upgraded ? "Played Iron Wave+!"sv : "Played Iron Wave!"sv);
            break;
        }
        case CardId::CLEAVE: {
            // AoE: attacks ALL alive enemies!
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    enemies[i].take_damage(attack_damage(base_value, player, enemies[i]));
                }
            }
            set_log(upgraded ? "Played Cleave+!"sv : "Played Cleave!"sv);
            break;
        }
        case CardId::ANGER: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            discard_pile.push_back(DeckCard(CardId::ANGER, upgraded)); // Extra copy!
            set_log(upgraded ? "Played Anger+!"sv : "Played Anger!"sv);
            break;
        }
        case CardId::METALLICIZE: {
            player.add_status(StatusType::METALLICIZE, base_value);
            set_log(upgraded ? "Played Metallicize+!"sv : "Played Metallicize!"sv);
            break;
        }
        case CardId::NEUTRALIZE: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::WEAK, secondary_value);
            set_log(upgraded ? "Played Neutralize+!"sv : "Played Neutralize!"sv);
            break;
        }
        case CardId::SURVIVOR: {
            player.gain_block(base_value);
            set_log(upgraded ? "Played Survivor+!"sv : "Played Survivor!"sv);
            break;
        }
        case CardId::POISON_STAB: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::POISON, secondary_value);
            set_log(upgraded ? "Played Poison Stab+!"sv : "Played Poison Stab!"sv);
            break;
        }
        case CardId::DEADLY_POISON: {
            current_target.add_status(StatusType::POISON, secondary_value);
            set_log(upgraded ? "Played Deadly Poison+!"sv : "Played Deadly Poison!"sv);
            break;
        }
        case CardId::ZAP: {
            player.channel_orb(OrbType::LIGHTNING, &current_target);
            set_log(upgraded ? "Channeled Lightning (Zap+)!"sv : "Channeled Lightning (Zap)!"sv);
            break;
        }
        case CardId::DUALCAST: {
            player.evoke_oldest_orb(&current_target, 2);
            set_log(upgraded ? "Triggered Dualcast+!"sv : "Triggered Dualcast!"sv);
            break;
        }
        case CardId::FALLING_STAR: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            player.add_stars(secondary_value);
            set_log(upgraded ? "Cast Falling Star+!"sv : "Cast Falling Star!"sv);
            break;
        }
        case CardId::VENERATE: {
            player.gain_block(base_value);
            player.add_stars(secondary_value);
            set_log(upgraded ? "Cast Venerate+!"sv : "Cast Venerate!"sv);
            break;
        }
        case CardId::UNLEASH: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            player.souls += secondary_value;
            set_log(upgraded ? "Cast Unleash+!"sv : "Cast Unleash!"sv);
            break;
        }
        case CardId::BODYGUARD: {
            player.gain_block(base_value);
            player.summon_minion(secondary_value, 0);
            set_log(upgraded ? "Summoned Osty (Bodyguard+)!"sv : "Summoned Osty (Bodyguard)!"sv);
            break;
        }
        case CardId::TWIN_STRIKE: {
            for (uint8_t hit = 0; hit < secondary_value; ++hit) {
                current_target.take_damage(attack_damage(base_value, player, current_target));
            }
            set_log(upgraded ? "Played Twin Strike+!"sv : "Played Twin Strike!"sv);
            break;
        }
        case CardId::POMMEL_STRIKE: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Pommel Strike+!"sv : "Played Pommel Strike!"sv);
            break;
        }
        case CardId::SHRUG_IT_OFF: {
            player.gain_block(base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Shrug It Off+!"sv : "Played Shrug It Off!"sv);
            break;
        }
        case CardId::TAUNT: {
            player.gain_block(base_value);
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Taunt+!"sv : "Played Taunt!"sv);
            break;
        }
        case CardId::SLICE: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            set_log(upgraded ? "Played Slice+!"sv : "Played Slice!"sv);
            break;
        }
        case CardId::BACKFLIP: {
            player.gain_block(base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Backflip+!"sv : "Played Backflip!"sv);
            break;
        }
        case CardId::SUCKER_PUNCH: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::WEAK, secondary_value);
            set_log(upgraded ? "Played Sucker Punch+!"sv : "Played Sucker Punch!"sv);
            break;
        }
        case CardId::DAGGER_SPRAY: {
            for (uint8_t hit = 0; hit < secondary_value; ++hit) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        enemies[i].take_damage(attack_damage(base_value, player, enemies[i]));
                    }
                }
            }
            set_log(upgraded ? "Played Dagger Spray+!"sv : "Played Dagger Spray!"sv);
            break;
        }
        case CardId::BEAM_CELL: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Beam Cell+!"sv : "Played Beam Cell!"sv);
            break;
        }
        case CardId::BALL_LIGHTNING: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            player.channel_orb(OrbType::LIGHTNING, &current_target);
            set_log(upgraded ? "Played Ball Lightning+!"sv : "Played Ball Lightning!"sv);
            break;
        }
        case CardId::COLD_SNAP: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            player.channel_orb(OrbType::FROST, &current_target);
            set_log(upgraded ? "Played Cold Snap+!"sv : "Played Cold Snap!"sv);
            break;
        }
        case CardId::LEAP: {
            player.gain_block(base_value);
            set_log(upgraded ? "Played Leap+!"sv : "Played Leap!"sv);
            break;
        }
        case CardId::SOLAR_STRIKE: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            player.add_stars(secondary_value);
            set_log(upgraded ? "Played Solar Strike+!"sv : "Played Solar Strike!"sv);
            break;
        }
        case CardId::GATHER_LIGHT: {
            player.gain_block(base_value);
            player.add_stars(secondary_value);
            set_log(upgraded ? "Played Gather Light+!"sv : "Played Gather Light!"sv);
            break;
        }
        case CardId::CELESTIAL_MIGHT: {
            for (uint8_t hit = 0; hit < secondary_value; ++hit) {
                current_target.take_damage(attack_damage(base_value, player, current_target));
            }
            set_log(upgraded ? "Played Celestial Might+!"sv : "Played Celestial Might!"sv);
            break;
        }
        case CardId::PULL_AGGRO: {
            player.summon_minion(secondary_value, 0);
            player.gain_block(base_value);
            set_log(upgraded ? "Played Pull Aggro+!"sv : "Played Pull Aggro!"sv);
            break;
        }
        case CardId::DRAIN_POWER: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            // Upgrades random cards of the discard pile for the rest of the combat.
            for (uint8_t n = 0; n < secondary_value; ++n) {
                FixedVector<uint8_t, 50> upgradable;
                for (size_t i = 0; i < discard_pile.size(); ++i) {
                    if (!discard_pile[i].upgraded) upgradable.push_back(static_cast<uint8_t>(i));
                }
                if (upgradable.empty()) break;
                discard_pile[upgradable[next_random() % upgradable.size()]].upgraded = true;
            }
            set_log(upgraded ? "Played Drain Power+!"sv : "Played Drain Power!"sv);
            break;
        }
        case CardId::BODY_SLAM: {
            current_target.take_damage(attack_damage(player.block, player, current_target));
            set_log(upgraded ? "Played Body Slam+!"sv : "Played Body Slam!"sv);
            break;
        }
        case CardId::DEFLECT: {
            player.gain_block(base_value);
            set_log(upgraded ? "Played Deflect+!"sv : "Played Deflect!"sv);
            break;
        }
        case CardId::COOLHEADED: {
            player.channel_orb(OrbType::FROST, &current_target);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Coolheaded+!"sv : "Played Coolheaded!"sv);
            break;
        }
        case CardId::SWEEPING_BEAM: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    enemies[i].take_damage(attack_damage(base_value, player, enemies[i]));
                }
            }
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Sweeping Beam+!"sv : "Played Sweeping Beam!"sv);
            break;
        }
        case CardId::THUNDERCLAP: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    enemies[i].take_damage(attack_damage(base_value, player, enemies[i]));
                    enemies[i].add_status(StatusType::VULNERABLE, secondary_value);
                }
            }
            set_log(upgraded ? "Played Thunderclap+!"sv : "Played Thunderclap!"sv);
            break;
        }
        case CardId::DISMANTLE: {
            // The second hit is decided before the first one lands.
            int hits = current_target.get_status(StatusType::VULNERABLE) > 0 ? 2 : 1;
            for (int hit = 0; hit < hits; ++hit) {
                current_target.take_damage(attack_damage(base_value, player, current_target));
            }
            set_log(upgraded ? "Played Dismantle+!"sv : "Played Dismantle!"sv);
            break;
        }
        case CardId::INFLAME: {
            player.add_status(StatusType::STRENGTH, base_value);
            set_log(upgraded ? "Played Inflame+!"sv : "Played Inflame!"sv);
            break;
        }
        case CardId::PERFECTED_STRIKE: {
            // Counts every card named "Frappe" in the deck, this one included (it already left the hand).
            int strikes = 1 + count_strikes(hand) + count_strikes(draw_pile) + count_strikes(discard_pile);
            current_target.take_damage(attack_damage(base_value + secondary_value * strikes, player, current_target));
            set_log(upgraded ? "Played Perfected Strike+!"sv : "Played Perfected Strike!"sv);
            break;
        }
        case CardId::BLUDGEON: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            set_log(upgraded ? "Played Bludgeon+!"sv : "Played Bludgeon!"sv);
            break;
        }
        case CardId::BULLY: {
            current_target.take_damage(attack_damage(base_value + secondary_value * current_target.get_status(StatusType::VULNERABLE), player, current_target));
            set_log(upgraded ? "Played Bully+!"sv : "Played Bully!"sv);
            break;
        }
        case CardId::SWORD_BOOMERANG: {
            for (uint8_t hit = 0; hit < secondary_value && has_alive_enemies(); ++hit) {
                Enemy& victim = enemies[random_alive_enemy_index()];
                victim.take_damage(attack_damage(base_value, player, victim));
            }
            set_log(upgraded ? "Played Sword Boomerang+!"sv : "Played Sword Boomerang!"sv);
            break;
        }
        case CardId::BREAK: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Break+!"sv : "Played Break!"sv);
            break;
        }
        case CardId::CONFLAGRATION: {
            for (uint8_t hit = 0; hit < secondary_value; ++hit) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        enemies[i].take_damage(attack_damage(base_value, player, enemies[i]));
                    }
                }
            }
            set_log(upgraded ? "Played Conflagration+!"sv : "Played Conflagration!"sv);
            break;
        }
        case CardId::EXPECT_A_FIGHT: {
            int16_t strength = std::max<int16_t>(0, player.get_status(StatusType::STRENGTH));
            player.gain_block(static_cast<int16_t>(base_value + secondary_value * strength));
            set_log(upgraded ? "Played Expect a Fight+!"sv : "Played Expect a Fight!"sv);
            break;
        }
        case CardId::PILLAGE: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            // Stops on the first non-Attack, a full hand or empty piles.
            while (draw_card() && CardDef::get(hand.back().id).type == CardType::ATTACK) {
            }
            set_log(upgraded ? "Played Pillage+!"sv : "Played Pillage!"sv);
            break;
        }
        case CardId::UPPERCUT: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::WEAK, secondary_value);
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Uppercut+!"sv : "Played Uppercut!"sv);
            break;
        }
        case CardId::FIGHT_ME: {
            for (int hit = 0; hit < 2; ++hit) {
                current_target.take_damage(attack_damage(base_value, player, current_target));
            }
            player.add_status(StatusType::STRENGTH, secondary_value);
            if (current_target.is_alive()) current_target.add_status(StatusType::STRENGTH, 1);
            set_log(upgraded ? "Played Fight Me+!"sv : "Played Fight Me!"sv);
            break;
        }
        case CardId::LEG_SWEEP: {
            current_target.add_status(StatusType::WEAK, secondary_value);
            player.gain_block(base_value);
            set_log(upgraded ? "Played Leg Sweep+!"sv : "Played Leg Sweep!"sv);
            break;
        }
        case CardId::HAZE: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    enemies[i].add_status(StatusType::POISON, base_value);
                    enemies[i].add_status(StatusType::WEAK, secondary_value);
                }
            }
            set_log(upgraded ? "Played Haze+!"sv : "Played Haze!"sv);
            break;
        }
        case CardId::PRECISE_CUT: {
            // The card already left the hand: what remains are the other cards.
            current_target.take_damage(attack_damage(static_cast<int16_t>(base_value - secondary_value * static_cast<int>(hand.size())), player, current_target));
            set_log(upgraded ? "Played Precise Cut+!"sv : "Played Precise Cut!"sv);
            break;
        }
        case CardId::BOUNCING_FLASK: {
            for (uint8_t bounce = 0; bounce < secondary_value; ++bounce) {
                enemies[random_alive_enemy_index()].add_status(StatusType::POISON, base_value);
            }
            set_log(upgraded ? "Played Bouncing Flask+!"sv : "Played Bouncing Flask!"sv);
            break;
        }
        case CardId::FLECHETTES: {
            int skills = 0;
            for (const DeckCard& card : hand) skills += CardDef::get(card.id).type == CardType::SKILL;
            for (int hit = 0; hit < skills; ++hit) {
                current_target.take_damage(attack_damage(base_value, player, current_target));
            }
            set_log(upgraded ? "Played Flechettes+!"sv : "Played Flechettes!"sv);
            break;
        }
        case CardId::FOOTWORK: {
            player.add_status(StatusType::DEXTERITY, base_value);
            set_log(upgraded ? "Played Footwork+!"sv : "Played Footwork!"sv);
            break;
        }
        case CardId::ESCAPE_PLAN: {
            if (draw_card() && CardDef::get(hand.back().id).type == CardType::SKILL) {
                player.gain_block(base_value);
            }
            set_log(upgraded ? "Played Escape Plan+!"sv : "Played Escape Plan!"sv);
            break;
        }
        case CardId::DASH: {
            player.gain_block(secondary_value);
            current_target.take_damage(attack_damage(base_value, player, current_target));
            set_log(upgraded ? "Played Dash+!"sv : "Played Dash!"sv);
            break;
        }
        case CardId::BUBBLE_BUBBLE: {
            if (current_target.get_status(StatusType::POISON) > 0) {
                current_target.add_status(StatusType::POISON, base_value);
            }
            set_log(upgraded ? "Played Bubble Bubble+!"sv : "Played Bubble Bubble!"sv);
            break;
        }
        case CardId::OUTBREAK: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    enemies[i].add_status(StatusType::POISON, base_value);
                    enemies[i].apply_poison_tick();
                }
            }
            set_log(upgraded ? "Played Outbreak+!"sv : "Played Outbreak!"sv);
            break;
        }
        case CardId::SHADOW_SHIELD: {
            player.gain_block(base_value);
            player.channel_orb(OrbType::DARK, &current_target);
            set_log(upgraded ? "Played Shadow Shield+!"sv : "Played Shadow Shield!"sv);
            break;
        }
        case CardId::METEOR_STRIKE: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            for (uint8_t n = 0; n < secondary_value; ++n) player.channel_orb(OrbType::PLASMA, &current_target);
            set_log(upgraded ? "Played Meteor Strike+!"sv : "Played Meteor Strike!"sv);
            break;
        }
        case CardId::GLACIER: {
            player.gain_block(base_value);
            for (uint8_t n = 0; n < secondary_value; ++n) player.channel_orb(OrbType::FROST, &current_target);
            set_log(upgraded ? "Played Glacier+!"sv : "Played Glacier!"sv);
            break;
        }
        case CardId::ICE_LANCE: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            for (uint8_t n = 0; n < secondary_value; ++n) player.channel_orb(OrbType::FROST, &current_target);
            set_log(upgraded ? "Played Ice Lance+!"sv : "Played Ice Lance!"sv);
            break;
        }
        case CardId::SKIM: {
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Skim+!"sv : "Played Skim!"sv);
            break;
        }
        case CardId::ALL_FOR_ONE: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            for (size_t i = 0; i < discard_pile.size() && hand.size() < hand.capacity();) {
                DeckCard card = discard_pile[i];
                if (CardDef::get_cost(card.id, card.upgraded) == 0) {
                    discard_pile.erase(i);
                    hand.push_back(card);
                } else {
                    ++i;
                }
            }
            set_log(upgraded ? "Played All for One+!"sv : "Played All for One!"sv);
            break;
        }
        case CardId::NULL_CARD: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::WEAK, secondary_value);
            player.channel_orb(OrbType::DARK, &current_target);
            set_log(upgraded ? "Played Null+!"sv : "Played Null!"sv);
            break;
        }
        case CardId::GO_FOR_THE_EYES: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            if (current_target.current_intent.base_damage > 0) {
                current_target.add_status(StatusType::WEAK, secondary_value);
            }
            set_log(upgraded ? "Played Go for the Eyes+!"sv : "Played Go for the Eyes!"sv);
            break;
        }
        case CardId::SCRAPE: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            size_t kept = hand.size();
            draw_cards(secondary_value);
            for (size_t i = hand.size(); i-- > kept;) {
                DeckCard drawn = hand[i];
                if (CardDef::get_cost(drawn.id, drawn.upgraded) != 0) {
                    hand.erase(i);
                    discard_pile.push_back(drawn);
                }
            }
            set_log(upgraded ? "Played Scrape+!"sv : "Played Scrape!"sv);
            break;
        }
        case CardId::BURY: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            set_log(upgraded ? "Played Bury+!"sv : "Played Bury!"sv);
            break;
        }
        case CardId::MISERY: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            for (StatusType debuff : { StatusType::VULNERABLE, StatusType::WEAK, StatusType::POISON }) {
                int16_t amount = current_target.get_status(debuff);
                for (size_t i = 0; amount > 0 && i < enemies.size(); ++i) {
                    if (static_cast<int>(i) != target_idx && enemies[i].is_alive()) enemies[i].add_status(debuff, amount);
                }
            }
            set_log(upgraded ? "Played Misery+!"sv : "Played Misery!"sv);
            break;
        }
        case CardId::UNDEATH: {
            player.gain_block(base_value);
            discard_pile.push_back(DeckCard(CardId::UNDEATH, upgraded));
            set_log(upgraded ? "Played Undeath+!"sv : "Played Undeath!"sv);
            break;
        }
        case CardId::ASHEN_STRIKE: {
            current_target.take_damage(attack_damage(base_value + secondary_value * static_cast<int>(exhaust_pile.size()), player, current_target));
            set_log(upgraded ? "Played Ashen Strike+!"sv : "Played Ashen Strike!"sv);
            break;
        }
        case CardId::TREMBLE: {
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Tremble+!"sv : "Played Tremble!"sv);
            break;
        }
        case CardId::EVIL_EYE: {
            player.gain_block(base_value);
            if (exhausted_this_turn > 0) player.gain_block(base_value);
            set_log(upgraded ? "Played Evil Eye+!"sv : "Played Evil Eye!"sv);
            break;
        }
        case CardId::MOLTEN_FIST: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            current_target.add_status(StatusType::VULNERABLE, current_target.get_status(StatusType::VULNERABLE));
            set_log(upgraded ? "Played Molten Fist+!"sv : "Played Molten Fist!"sv);
            break;
        }
        case CardId::CINDER: {
            current_target.take_damage(attack_damage(base_value, player, current_target));
            if (!hand.empty()) {
                exhaust_from_hand(next_random() % hand.size());
            }
            set_log(upgraded ? "Played Cinder+!"sv : "Played Cinder!"sv);
            break;
        }
        case CardId::SECOND_WIND: {
            for (size_t i = hand.size(); i-- > 0;) {
                if (CardDef::get(hand[i].id).type != CardType::ATTACK) {
                    exhaust_from_hand(i);
                    player.gain_block(base_value);
                }
            }
            set_log(upgraded ? "Played Second Wind+!"sv : "Played Second Wind!"sv);
            break;
        }
        case CardId::TRUE_GRIT: {
            // The upgraded card lets the player choose the card: random as well until hand choices exist (#31).
            player.gain_block(base_value);
            if (!hand.empty()) {
                exhaust_from_hand(next_random() % hand.size());
            }
            set_log(upgraded ? "Played True Grit+!"sv : "Played True Grit!"sv);
            break;
        }
        case CardId::DOMINATE: {
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            player.add_status(StatusType::STRENGTH, current_target.get_status(StatusType::VULNERABLE));
            set_log(upgraded ? "Played Dominate+!"sv : "Played Dominate!"sv);
            break;
        }
        case CardId::FIEND_FIRE: {
            size_t burnt = hand.size();
            while (!hand.empty()) exhaust_from_hand(hand.size() - 1);
            for (size_t hit = 0; hit < burnt; ++hit) {
                current_target.take_damage(attack_damage(base_value, player, current_target));
            }
            set_log(upgraded ? "Played Fiend Fire+!"sv : "Played Fiend Fire!"sv);
            break;
        }
        case CardId::THRASH: {
            for (int hit = 0; hit < 2; ++hit) {
                current_target.take_damage(attack_damage(base_value + played_card.bonus, player, current_target));
            }
            // Exhausts a random Attack of the hand; this copy keeps its damage for the rest of the combat.
            FixedVector<uint8_t, 10> attacks;
            for (size_t i = 0; i < hand.size(); ++i) {
                if (CardDef::get(hand[i].id).type == CardType::ATTACK) attacks.push_back(static_cast<uint8_t>(i));
            }
            if (!attacks.empty()) {
                DeckCard eaten = exhaust_from_hand(attacks[next_random() % attacks.size()]);
                int gained = CardDef::get_value(eaten.id, eaten.upgraded) + eaten.bonus;
                played_card.bonus = static_cast<uint8_t>(std::min(255, played_card.bonus + gained));
            }
            set_log(upgraded ? "Played Thrash+!"sv : "Played Thrash!"sv);
            break;
        }
        case CardId::IMPERVIOUS: {
            player.gain_block(base_value);
            set_log(upgraded ? "Played Impervious+!"sv : "Played Impervious!"sv);
            break;
        }
        case CardId::PACTS_END: {
            if (exhaust_pile.size() >= secondary_value) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        enemies[i].take_damage(attack_damage(base_value, player, enemies[i]));
                    }
                }
            }
            set_log(upgraded ? "Played Pact's End+!"sv : "Played Pact's End!"sv);
            break;
        }
        case CardId::NOT_YET: {
            player.heal(base_value);
            set_log(upgraded ? "Played Not Yet+!"sv : "Played Not Yet!"sv);
            break;
        }
        case CardId::ADRENALINE: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Adrenaline+!"sv : "Played Adrenaline!"sv);
            break;
        }
        case CardId::CALCULATED_GAMBLE: {
            int swapped = static_cast<int>(hand.size());
            while (!hand.empty()) {
                discard_pile.push_back(hand.back());
                hand.pop_back();
            }
            draw_cards(swapped);
            set_log(upgraded ? "Played Calculated Gamble+!"sv : "Played Calculated Gamble!"sv);
            break;
        }
        case CardId::MIRAGE: {
            int16_t poison = 0;
            for (const Enemy& foe : enemies) {
                if (foe.is_alive()) poison += foe.get_status(StatusType::POISON);
            }
            player.gain_block(poison);
            set_log(upgraded ? "Played Mirage+!"sv : "Played Mirage!"sv);
            break;
        }
        case CardId::RAINBOW: {
            for (OrbType orb : { OrbType::LIGHTNING, OrbType::FROST, OrbType::DARK }) player.channel_orb(orb, &current_target);
            set_log(upgraded ? "Played Rainbow+!"sv : "Played Rainbow!"sv);
            break;
        }
        case CardId::CHILL: {
            for (int foes = count_alive_enemies(); foes > 0; --foes) player.channel_orb(OrbType::FROST, &current_target);
            set_log(upgraded ? "Played Chill+!"sv : "Played Chill!"sv);
            break;
        }
        case CardId::FUSION: {
            player.channel_orb(OrbType::PLASMA, &current_target);
            set_log(upgraded ? "Played Fusion+!"sv : "Played Fusion!"sv);
            break;
        }
        case CardId::REBOOT: {
            while (!hand.empty()) {
                draw_pile.push_back(hand.back());
                hand.pop_back();
            }
            shuffle_discard_into_draw();
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Reboot+!"sv : "Played Reboot!"sv);
            break;
        }
        case CardId::SUPERCRITICAL: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            set_log(upgraded ? "Played Supercritical+!"sv : "Played Supercritical!"sv);
            break;
        }
        case CardId::DOUBLE_ENERGY: {
            player.energy = static_cast<int8_t>(player.energy * 2);
            set_log(upgraded ? "Played Double Energy+!"sv : "Played Double Energy!"sv);
            break;
        }
        case CardId::SHARED_FATE: {
            player.add_status(StatusType::STRENGTH, static_cast<int16_t>(-base_value));
            current_target.add_status(StatusType::STRENGTH, static_cast<int16_t>(-secondary_value));
            set_log(upgraded ? "Played Shared Fate+!"sv : "Played Shared Fate!"sv);
            break;
        }
        case CardId::WISP: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            set_log(upgraded ? "Played Wisp+!"sv : "Played Wisp!"sv);
            break;
        }
        case CardId::PUTREFY: {
            current_target.add_status(StatusType::WEAK, secondary_value);
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Putrefy+!"sv : "Played Putrefy!"sv);
            break;
        }
        case CardId::REANIMATE: {
            player.summon_minion(secondary_value, 0);
            set_log(upgraded ? "Played Reanimate+!"sv : "Played Reanimate!"sv);
            break;
        }
        case CardId::AFTERLIFE: {
            player.summon_minion(secondary_value, 0);
            set_log(upgraded ? "Played Afterlife+!"sv : "Played Afterlife!"sv);
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
        if (CardDef::has_keyword(card_id, upgraded, KEYWORD_EXHAUST)) {
            exhaust_card(played_card);
        } else {
            discard_pile.push_back(played_card);
        }
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

    // Discard the hand, except the cards with Retain
    for (size_t i = hand.size(); i-- > 0;) {
        if (retains(hand[i])) continue;
        discard_pile.push_back(hand[i]);
        hand.erase(i);
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
