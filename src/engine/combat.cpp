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
        case CardId::ASHEN_STRIKE:
        case CardId::SETUP_STRIKE:
        case CardId::SHINING_STRIKE:
        case CardId::FOCUSED_STRIKE:
        case CardId::BLIGHT_STRIKE:
        case CardId::ADAPTIVE_STRIKE:
        case CardId::MOMENTUM_STRIKE:
        case CardId::LEADING_STRIKE:
        case CardId::SCULPTING_STRIKE:
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

// Attacks dealt by Osty (Squeeze counts them).
bool is_osty_attack(CardId id) {
    switch (id) {
        case CardId::UNLEASH:
        case CardId::POKE:
        case CardId::SNAP:
        case CardId::RATTLE:
        case CardId::BONE_SHARDS:
        case CardId::SQUEEZE:
        case CardId::FLATTEN:
        case CardId::HIGH_FIVE:
        case CardId::FETCH:
        case CardId::RIGHT_HAND_HAND:
        case CardId::PROTECTOR:
        case CardId::SIC_EM:
        case CardId::SWEEPING_GAZE:
            return true;
        default:
            return false;
    }
}

template <size_t Capacity>
int count_osty_attacks(const FixedVector<DeckCard, Capacity>& pile) {
    int count = 0;
    for (const DeckCard& card : pile) count += is_osty_attack(card.id);
    return count;
}

uint8_t bump(uint8_t value, int amount = 1) {
    return static_cast<uint8_t>(std::min(255, value + amount));
}

// Reactions that trigger reactions stop here: no chain of real cards goes that deep.
constexpr uint8_t MAX_EVENT_DEPTH = 6;

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

bool CombatEngine::every_enemy_escaped() const {
    for (const Enemy& foe : enemies) {
        if (!foe.escaped) return false;
    }
    return !enemies.empty();
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

void CombatEngine::add_power(PowerId id, int16_t amount, bool turn_only, int8_t target) {
    for (ActivePower& power : powers) {
        if (power.id == id && power.turn_only == turn_only && power.target == target) {
            power.amount = static_cast<int16_t>(power.amount + amount);
            power.stacks = bump(power.stacks);
            return;
        }
    }
    ActivePower power;
    power.id = id;
    power.amount = amount;
    power.stacks = 1;
    power.turn_only = turn_only;
    power.target = target;
    powers.push_back(power);
}

int16_t CombatEngine::power_amount(PowerId id) const {
    int16_t total = 0;
    for (const ActivePower& power : powers) {
        if (power.id == id) total = static_cast<int16_t>(total + power.amount);
    }
    return total;
}

void CombatEngine::add_enemy_power(int enemy_index, PowerId id, int16_t amount) {
    if (enemy_index < 0 || static_cast<size_t>(enemy_index) >= enemies.size() || id == PowerId::NONE) return;
    if (!enemies[enemy_index].is_alive()) return;
    for (ActivePower& power : powers) {
        if (power.id == id && power.holder == enemy_index) {
            power.amount = static_cast<int16_t>(power.amount + amount);
            power.stacks = bump(power.stacks);
            return;
        }
    }
    ActivePower power;
    power.id = id;
    power.amount = amount;
    power.stacks = 1;
    power.holder = static_cast<int8_t>(enemy_index);
    powers.push_back(power);
}

int16_t CombatEngine::enemy_power_amount(int enemy_index, PowerId id) const {
    for (const ActivePower& power : powers) {
        if (power.id == id && power.holder == enemy_index) return power.amount;
    }
    return 0;
}

bool CombatEngine::afflict_player(PowerId id, int16_t amount, bool turn_only) {
    if (id == PowerId::NONE || amount <= 0) return false;
    if (is_player_debuff_power(id) && player.get_status(StatusType::ARTIFACT) > 0) {
        player.add_status(StatusType::ARTIFACT, -1);
        return false;
    }
    add_power(id, amount, turn_only);
    return true;
}

uint8_t CombatEngine::power_counter(PowerId id) const {
    for (const ActivePower& power : powers) {
        if (power.id == id) return power.counter;
    }
    return 0;
}

void CombatEngine::settle_deaths() {
    for (size_t i = 0; i < enemies.size(); ++i) {
        uint8_t bit = static_cast<uint8_t>(1u << i);
        if ((alive_mask_ & bit) && !enemies[i].is_alive()) {
            alive_mask_ = static_cast<uint8_t>(alive_mask_ & ~bit);
            // Its powers leave with it, whether it died or ran away.
            for (size_t p = powers.size(); p-- > 0;) {
                if (powers[p].holder == static_cast<int8_t>(i)) powers.erase(p);
            }
            // An enemy that ran away is not a kill: it keeps what it stole and nothing reacts to its death.
            if (enemies[i].escaped) continue;
            // What it stole from the player comes back.
            enemies[i].return_stolen(player);
            // Melancholy gets cheaper each time someone dies.
            change_cost_of_copies(CardId::MELANCHOLY, -1);
            EventInfo info;
            info.enemy = static_cast<int8_t>(i);
            emit(CombatEvent::ENEMY_KILLED, info);
        }
    }
}

ActivePower* CombatEngine::find_power(PowerId id) {
    for (ActivePower& power : powers) {
        if (power.id == id) return &power;
    }
    return nullptr;
}

int8_t CombatEngine::index_of(const Enemy& target) const {
    for (size_t i = 0; i < enemies.size(); ++i) {
        if (&enemies[i] == &target) return static_cast<int8_t>(i);
    }
    return -1;
}

void CombatEngine::emit(CombatEvent event, const EventInfo& info) {
    if (powers.empty() || event_depth_ >= MAX_EVENT_DEPTH) return;
    ++event_depth_;
    // A power gained while the others react only hears the next event.
    size_t listeners = powers.size();
    for (size_t i = 0; i < listeners && i < powers.size(); ++i) {
        react(powers[i], event, info);
    }
    --event_depth_;
}

void CombatEngine::react(ActivePower& power, CombatEvent event, const EventInfo& info) {
    const bool my_turn = state == CombatState::PLAYER_TURN;
    const bool turn_start = event == CombatEvent::TURN_START;
    const bool card_played = event == CombatEvent::CARD_PLAYED;
    Enemy* foe = (info.enemy >= 0 && static_cast<size_t>(info.enemy) < enemies.size()) ? &enemies[info.enemy] : nullptr;
    // Whether the card being played completes a group of `every` cards of its type this turn: `tally` does
    // not count it yet.
    const auto completes = [&](CardType type, uint8_t tally, uint8_t every) {
        return card_played && CardDef::get(info.card.id).type == type && (tally + 1) % every == 0;
    };

    if (power.holder >= 0) {
        // A power held by an enemy: silent once its holder is out, and it counts its holder's turns.
        if (static_cast<size_t>(power.holder) >= enemies.size() || !enemies[power.holder].is_alive()) return;
        if (event == CombatEvent::ENEMY_TURN_START && info.enemy == power.holder) power.turns = bump(power.turns);
    } else if (turn_start) {
        power.turns = bump(power.turns);
    }

    switch (power.id) {
        case PowerId::TENDER:
            if (card_played) {
                gain_temporary(StatusType::STRENGTH, static_cast<int8_t>(-power.amount));
                gain_temporary(StatusType::DEXTERITY, static_cast<int8_t>(-power.amount));
            }
            break;
        case PowerId::BURROWED:
            // Spent when its Block is broken (the holder loses its turn) or when its holder acts again.
            if (event == CombatEvent::BLOCK_BROKEN && info.enemy == power.holder && power.amount > 0) {
                power.amount = 0;
                enemies[power.holder].break_burrow();
            } else if (event == CombatEvent::ENEMY_TURN_START && info.enemy == power.holder) {
                power.amount = 0;
            }
            break;
        case PowerId::PERSONAL_HIVE:
            if (event == CombatEvent::UNBLOCKED_DAMAGE_DEALT && info.enemy == power.holder) {
                for (int16_t i = 0; i < power.amount; ++i) create_card(CardId::DAZED, Pile::DRAW);
            }
            break;
        case PowerId::VITAL_SPARK:
            if (turn_start) {
                power.counter = 0;
            } else if (event == CombatEvent::UNBLOCKED_DAMAGE_DEALT && info.enemy == power.holder &&
                       power.counter < power.amount) {
                power.counter = bump(power.counter);
                player.energy = static_cast<int8_t>(player.energy + 1);
            }
            break;
        case PowerId::TURN_ENERGY:
            if (turn_start) player.energy = static_cast<int8_t>(player.energy + power.amount);
            break;
        case PowerId::TURN_DRAW:
        case PowerId::TYRANNY: // the exhaust that follows is asked once the hand is drawn
            if (turn_start) turn_draw_bonus_ = static_cast<int16_t>(turn_draw_bonus_ + power.amount);
            break;
        case PowerId::TURN_STRENGTH:
        case PowerId::TOASTY_MITTENS: // the exhaust that follows is asked once the hand is drawn
            if (turn_start) player.add_status(StatusType::STRENGTH, power.amount);
            break;
        case PowerId::TURN_STARS:
            if (turn_start) gain_stars(power.amount);
            break;
        case PowerId::TURN_POISON_ALL:
            if (turn_start) {
                for (Enemy& enemy_hit : enemies) {
                    if (enemy_hit.is_alive()) afflict(enemy_hit, StatusType::POISON, power.amount);
                }
            }
            break;
        case PowerId::CRIMSON_MANTLE:
            if (turn_start) {
                lose_hp(power.stacks);
                add_block(power.amount);
            }
            break;
        case PowerId::INFERNO:
            if (turn_start) {
                lose_hp(power.stacks);
            } else if (event == CombatEvent::HP_LOST && my_turn) {
                damage_all_enemies(power.amount);
            }
            break;
        case PowerId::AGGRESSION:
            if (turn_start) {
                for (uint8_t copy = 0; copy < power.stacks && hand.size() < hand.capacity(); ++copy) {
                    FixedVector<uint8_t, 50> attacks;
                    for (size_t i = 0; i < discard_pile.size(); ++i) {
                        if (CardDef::get(discard_pile[i].id).type == CardType::ATTACK) attacks.push_back(static_cast<uint8_t>(i));
                    }
                    if (attacks.empty()) break;
                    size_t pick = attacks[next_random() % attacks.size()];
                    DeckCard card = discard_pile[pick];
                    card.upgraded = true;
                    discard_pile.erase(pick);
                    hand.push_back(card);
                }
            }
            break;
        case PowerId::FERAL:
        case PowerId::ECHO_FORM:
        case PowerId::VOID_FORM:
            if (turn_start) power.counter = 0;
            break;
        case PowerId::HISTORY_COURSE:
            if (card_played && CardDef::get(info.card.id).type == CardType::ATTACK) {
                power.amount = static_cast<int16_t>(static_cast<int>(info.card.id) * 2 + (info.card.upgraded ? 1 : 0) + 1);
            } else if (turn_start && my_turn && power.amount > 0) {
                DeckCard copy(static_cast<CardId>((power.amount - 1) / 2));
                copy.upgraded = (power.amount - 1) % 2 == 1;
                autoplay(copy, -1, AfterPlay::NOWHERE);
            }
            break;
        case PowerId::CROSSBOW:
            if (turn_start && my_turn) {
                const CardId id = random_card(CardFilter(CardType::ATTACK));
                if (id != CardId::NONE) {
                    DeckCard gift(id);
                    gift.free_this_turn = hand.size() < hand.capacity();
                    create_card(gift, Pile::HAND);
                }
            }
            break;
        case PowerId::HELLRAISER:
            // The card just drawn is the last one of the hand.
            if (event == CombatEvent::CARD_DRAWN && is_strike(info.card.id) && !hand.empty() && hand.back().id == info.card.id) {
                DeckCard strike = hand.back();
                hand.pop_back();
                autoplay(strike);
            }
            break;
        case PowerId::STAMPEDE:
            if (event == CombatEvent::TURN_END) {
                for (uint8_t copy = 0; copy < power.stacks; ++copy) {
                    FixedVector<uint8_t, 10> attacks;
                    for (size_t i = 0; i < hand.size(); ++i) {
                        if (CardDef::get(hand[i].id).type == CardType::ATTACK) attacks.push_back(static_cast<uint8_t>(i));
                    }
                    if (attacks.empty() || !has_alive_enemies()) break;
                    size_t pick = attacks[next_random() % attacks.size()];
                    DeckCard attack = hand[pick];
                    hand.erase(pick);
                    autoplay(attack);
                }
            }
            break;
        case PowerId::INFINITE_BLADES:
            if (turn_start) add_shivs(power.amount);
            break;
        case PowerId::FURNACE:
            if (turn_start) forge(power.amount);
            break;
        case PowerId::PARRY:
            if (card_played && info.card.id == CardId::SOVEREIGN_BLADE) add_block(power.amount);
            break;
        case PowerId::HELICAL_DART:
            if (card_played && info.card.id == CardId::SHIV) gain_temporary(StatusType::DEXTERITY, static_cast<int8_t>(power.amount));
            break;
        case PowerId::SMOKESTACK:
            if (event == CombatEvent::CARD_CREATED && CardDef::get(info.card.id).type == CardType::STATUS) {
                damage_all_enemies(power.amount);
            }
            break;
        case PowerId::TRASH_TO_TREASURE:
            if (event == CombatEvent::CARD_CREATED && CardDef::get(info.card.id).type == CardType::STATUS) {
                constexpr OrbType KINDS[] = { OrbType::LIGHTNING, OrbType::FROST, OrbType::DARK, OrbType::PLASMA };
                for (uint8_t copy = 0; copy < power.stacks; ++copy) channel_orb(KINDS[next_random() % 4]);
            }
            break;
        case PowerId::ITERATION:
            if (turn_start) power.counter = 0;
            if (event == CombatEvent::CARD_DRAWN && CardDef::get(info.card.id).type == CardType::STATUS && power.counter == 0) {
                power.counter = 1;
                draw_cards(power.amount);
            }
            break;
        case PowerId::FEEL_NO_PAIN:
            if (event == CombatEvent::CARD_EXHAUSTED) add_block(power.amount);
            break;
        case PowerId::DARK_EMBRACE:
            if (event == CombatEvent::CARD_EXHAUSTED) draw_cards(power.amount);
            break;
        case PowerId::RUPTURE:
            if (event == CombatEvent::HP_LOST && my_turn) player.add_status(StatusType::STRENGTH, power.amount);
            break;
        case PowerId::JUGGERNAUT:
            if (event == CombatEvent::BLOCK_GAINED && has_alive_enemies()) {
                damage_enemy(enemies[random_alive_enemy_index()], power.amount);
            }
            break;
        case PowerId::RAGE:
            if (card_played && CardDef::get(info.card.id).type == CardType::ATTACK) add_block(power.amount);
            break;
        case PowerId::DAUGHTER_OF_THE_WIND:
            if (card_played && CardDef::get(info.card.id).type == CardType::ATTACK) add_block(power.amount);
            break;
        case PowerId::VICIOUS:
            if (event == CombatEvent::DEBUFF_APPLIED && info.status == StatusType::VULNERABLE) draw_cards(power.amount);
            break;
        case PowerId::SPEEDSTER:
            if (event == CombatEvent::CARD_DRAWN && my_turn && !info.opening_draw) damage_all_enemies(power.amount);
            break;
        case PowerId::ENVENOM:
            if (event == CombatEvent::UNBLOCKED_DAMAGE_DEALT && foe && foe->is_alive()) {
                afflict(*foe, StatusType::POISON, power.amount);
            }
            break;
        case PowerId::SERPENT_FORM:
            if (card_played && has_alive_enemies()) damage_enemy(enemies[random_alive_enemy_index()], power.amount);
            break;
        case PowerId::AFTERIMAGE:
            if (card_played) add_block(power.amount);
            break;
        case PowerId::STRANGLE:
            if (card_played && power.target >= 0 && static_cast<size_t>(power.target) < enemies.size()) {
                // HP loss: the enemy's Block does not absorb it.
                Enemy& victim = enemies[power.target];
                victim.hp = std::max<int16_t>(0, static_cast<int16_t>(victim.hp - power.amount));
            }
            break;
        case PowerId::HAUNT:
            // HP loss: the enemy's Block does not absorb it.
            if (card_played && is_soul(info.card.id) && has_alive_enemies()) {
                enemies[random_alive_enemy_index()].lose_hp(power.amount);
            }
            break;
        case PowerId::DEVOUR_LIFE:
            if (card_played && is_soul(info.card.id)) player.summon_minion(power.amount, 0);
            break;
        case PowerId::CORROSIVE_WAVE:
            if (event == CombatEvent::CARD_DRAWN && !info.opening_draw) {
                for (Enemy& enemy_hit : enemies) {
                    if (enemy_hit.is_alive()) afflict(enemy_hit, StatusType::POISON, power.amount);
                }
            }
            break;
        case PowerId::STORM:
            if (card_played && CardDef::get(info.card.id).type == CardType::POWER) {
                for (int16_t orb = 0; orb < power.amount; ++orb) {
                    channel_orb(OrbType::LIGHTNING);
                }
            }
            break;
        case PowerId::SUBROUTINE:
            if (card_played && CardDef::get(info.card.id).type == CardType::POWER) {
                player.energy = static_cast<int8_t>(player.energy + power.amount);
            }
            break;
        case PowerId::CHILD_OF_THE_STARS:
            if (event == CombatEvent::STARS_SPENT) add_block(static_cast<int16_t>(power.amount * info.amount));
            break;
        case PowerId::SEALED_THRONE:
            if (card_played) gain_stars(power.amount);
            break;
        case PowerId::BLACK_HOLE:
            if (event == CombatEvent::STARS_GAINED || event == CombatEvent::STARS_SPENT) damage_all_enemies(power.amount);
            break;
        case PowerId::PALE_BLUE_DOT:
            if (event == CombatEvent::TURN_END && counters.cards_played() >= PALE_BLUE_DOT_CARDS) {
                schedule(DelayedType::DRAW, power.amount);
            }
            break;
        case PowerId::ARSENAL:
            if (event == CombatEvent::CARD_CREATED) player.add_status(StatusType::STRENGTH, power.amount);
            break;
        case PowerId::MONOLOGUE:
            if (card_played) gain_temporary(StatusType::STRENGTH, static_cast<int8_t>(power.amount));
            break;
        case PowerId::ORBIT:
            if (card_played) {
                int spent = power.counter + info.cost;
                player.energy = static_cast<int8_t>(player.energy + power.amount * (spent / ORBIT_ENERGY));
                power.counter = static_cast<uint8_t>(spent % ORBIT_ENERGY);
            }
            break;
        case PowerId::PILLAR_OF_CREATION:
            if (event == CombatEvent::CARD_CREATED) add_block(power.amount);
            break;
        case PowerId::MONARCHS_GAZE:
            if (event == CombatEvent::ATTACK_HIT && foe && foe->is_alive()) sap_strength(*foe, power.amount);
            break;
        case PowerId::DANSE_MACABRE:
            if (card_played && info.cost >= DANSE_MACABRE_COST) add_block(power.amount);
            break;
        case PowerId::SPIRIT_OF_ASH:
            if (card_played && CardDef::has_keyword(info.card, KEYWORD_ETHEREAL)) add_block(power.amount);
            break;
        case PowerId::PAGESTORM:
            if (event == CombatEvent::CARD_DRAWN && CardDef::has_keyword(info.card, KEYWORD_ETHEREAL)) draw_cards(power.amount);
            break;
        case PowerId::SLEIGHT_OF_FLESH:
            if (event == CombatEvent::DEBUFF_APPLIED && foe && foe->is_alive()) damage_enemy(*foe, power.amount);
            break;
        // Orbs and Focus
        case PowerId::LOOP:
            if (turn_start) {
                for (int16_t repeat = 0; repeat < power.amount; ++repeat) trigger_orb_passive(0);
            }
            break;
        case PowerId::COOLANT:
            if (turn_start) add_block(static_cast<int16_t>(power.amount * unique_orbs()));
            break;
        case PowerId::BIASED_COGNITION:
            if (turn_start) player.receive_status(StatusType::FOCUS, static_cast<int16_t>(-power.stacks));
            break;
        case PowerId::CONSUMING_SHADOW:
            if (event == CombatEvent::TURN_END) {
                for (uint8_t copy = 0; copy < power.stacks && !player.orbs.empty(); ++copy) {
                    evoke_orb(player.orbs.size() - 1);
                }
            }
            break;
        case PowerId::HAILSTORM:
            if (event == CombatEvent::TURN_END) {
                for (const Orb& orb : player.orbs) {
                    if (orb == OrbType::FROST) {
                        damage_all_enemies(power.amount);
                        break;
                    }
                }
            }
            break;
        // Doom
        case PowerId::COUNTDOWN:
            if (turn_start && has_alive_enemies()) afflict(enemies[random_alive_enemy_index()], StatusType::DOOM, power.amount);
            break;
        case PowerId::NEUROSURGE:
            if (turn_start) player.receive_status(StatusType::DOOM, power.amount);
            break;
        case PowerId::HAND_DRILL:
            if (event == CombatEvent::BLOCK_BROKEN && foe && foe->is_alive()) afflict(*foe, StatusType::VULNERABLE, power.amount);
            break;
        case PowerId::REAPER_FORM:
            if (event == CombatEvent::UNBLOCKED_DAMAGE_DEALT && foe && foe->is_alive()) {
                afflict(*foe, StatusType::DOOM, info.amount);
            }
            break;
        case PowerId::OBLIVION:
            if (card_played && power.target >= 0 && static_cast<size_t>(power.target) < enemies.size() &&
                enemies[power.target].is_alive()) {
                afflict(enemies[power.target], StatusType::DOOM, power.amount);
            }
            break;
        case PowerId::SHROUD:
            if (event == CombatEvent::DEBUFF_APPLIED && info.status == StatusType::DOOM) add_block(power.amount);
            break;
        // Attacks played
        case PowerId::JUGGLING:
            // The tally of the turn does not count the card being played yet.
            if (card_played && CardDef::get(info.card.id).type == CardType::ATTACK &&
                counters.attacks_played + 1 == JUGGLING_ATTACK) {
                DeckCard copy = info.card;
                copy.free_this_turn = false;
                copy.deck_index = NOT_IN_DECK; // a gain on the copy does not reach the deck
                create_card(copy, Pile::HAND);
            }
            break;
        // Relics
        case PowerId::CAPTAINS_WHEEL:
            if (turn_start && power.turns == 3) add_block(power.amount);
            break;
        case PowerId::HORN_CLEAT:
            if (turn_start && power.turns == 2) add_block(power.amount);
            break;
        case PowerId::CANDELABRA:
            if (turn_start && power.turns == 2) player.energy = static_cast<int8_t>(player.energy + power.amount);
            break;
        case PowerId::CHANDELIER:
            if (turn_start && power.turns == 3) player.energy = static_cast<int8_t>(player.energy + power.amount);
            break;
        case PowerId::SPARKLING_ROUGE:
            if (turn_start && power.turns == 3) {
                player.add_status(StatusType::STRENGTH, power.amount);
                player.add_status(StatusType::DEXTERITY, power.amount);
            }
            break;
        case PowerId::SEAL_OF_GOLD:
            if (turn_start && gold >= 3) {
                gold = static_cast<int16_t>(gold - 3);
                player.energy = static_cast<int8_t>(player.energy + power.amount);
            }
            break;
        case PowerId::SAI:
            if (turn_start) add_block(power.amount);
            break;
        case PowerId::HAPPY_FLOWER:
            if (turn_start && power.turns % 3 == 0) player.energy = static_cast<int8_t>(player.energy + power.amount);
            break;
        case PowerId::FAKE_HAPPY_FLOWER:
            if (turn_start && power.turns % 5 == 0) player.energy = static_cast<int8_t>(player.energy + power.amount);
            break;
        case PowerId::PENDULUM:
            if (turn_start && power.turns % 3 == 0) turn_draw_bonus_ = static_cast<int16_t>(turn_draw_bonus_ + power.amount);
            break;
        case PowerId::RING_OF_THE_DRAKE:
            if (turn_start && power.turns <= RING_OF_THE_DRAKE_TURNS) turn_draw_bonus_ = static_cast<int16_t>(turn_draw_bonus_ + power.amount);
            break;
        case PowerId::POLLINOUS_CORE:
            if (turn_start && power.turns % 4 == 0) turn_draw_bonus_ = static_cast<int16_t>(turn_draw_bonus_ + power.amount);
            break;
        case PowerId::RINGING_TRIANGLE:
            // Set after the turn start has cleared retain_hand: the hand is kept at the end of the first turn only.
            if (turn_start && power.turns == 1) retain_hand = true;
            break;
        case PowerId::VERY_HOT_COCOA:
            if (turn_start && power.turns == 1) player.energy = static_cast<int8_t>(player.energy + power.amount);
            break;
        case PowerId::BREAD:
            if (turn_start) {
                int change = power.turns == 1 ? -BREAD_ENERGY_LOST : power.amount;
                player.energy = static_cast<int8_t>(std::max(0, player.energy + change));
            }
            break;
        case PowerId::MERCURY_HOURGLASS:
            if (turn_start) damage_all_enemies(power.amount);
            break;
        case PowerId::PAELS_FLESH:
            if (turn_start && turn_number >= 3) player.energy = static_cast<int8_t>(player.energy + power.amount);
            break;
        case PowerId::MR_STRUGGLES:
            if (turn_start) damage_all_enemies(static_cast<int16_t>(turn_number));
            break;
        case PowerId::CLOAK_CLASP:
            if (event == CombatEvent::TURN_END) add_block(static_cast<int16_t>(power.amount * hand.size()));
            break;
        case PowerId::POCKETWATCH:
            if (event == CombatEvent::TURN_END && counters.cards_played() <= POCKETWATCH_CARDS) {
                schedule(DelayedType::DRAW, power.amount);
            }
            break;
        case PowerId::PARRYING_SHIELD:
            if (event == CombatEvent::TURN_END && player.block >= PARRYING_SHIELD_BLOCK && has_alive_enemies()) {
                damage_enemy(enemies[random_alive_enemy_index()], power.amount);
            }
            break;
        case PowerId::STONE_CALENDAR:
            if (event == CombatEvent::TURN_END && power.turns == STONE_CALENDAR_TURN) damage_all_enemies(power.amount);
            break;
        case PowerId::ART_OF_WAR:
            if (event == CombatEvent::TURN_END && counters.attacks_played == 0) schedule(DelayedType::ENERGY, power.amount);
            break;
        case PowerId::ROLLING_BOULDER:
            if (turn_start) {
                damage_all_enemies(static_cast<int16_t>(power.amount + power.counter));
                power.counter = bump(power.counter, ROLLING_BOULDER_GROWTH * power.stacks);
            }
            break;
        case PowerId::PANACHE:
            if (turn_start) {
                power.counter = 0;
            } else if (card_played) {
                power.counter = bump(power.counter);
                if (power.counter >= PANACHE_CARDS) {
                    power.counter = 0;
                    damage_all_enemies(power.amount);
                }
            }
            break;
        case PowerId::FASTEN:
            break; // read when a Defend card is played
        case PowerId::MASTER_PLANNER:
            break; // read when a Skill is placed after it was played
        case PowerId::NOSTALGIA:
            if (turn_start) power.counter = 0;
            break;
        case PowerId::HELLO_WORLD:
            if (turn_start) {
                for (int16_t copy = 0; copy < power.amount; ++copy) {
                    create_random_card(CardFilter(CardRarity::COMMON), Pile::HAND);
                }
            }
            break;
        case PowerId::MAYHEM:
            if (turn_start) {
                for (int16_t copy = 0; copy < power.amount && has_alive_enemies(); ++copy) {
                    if (draw_pile.empty()) shuffle_discard_into_draw();
                    if (draw_pile.empty()) break;
                    const DeckCard top = draw_pile.back();
                    draw_pile.pop_back();
                    autoplay(top);
                }
            }
            break;
        case PowerId::PREP_TIME:
            if (turn_start) player.add_status(StatusType::VIGOR, power.amount);
            break;
        case PowerId::STRATAGEM:
            if (event == CombatEvent::DRAW_PILE_SHUFFLED) {
                ask_pile_choice(Pile::DRAW, ChoiceKind::TO_HAND, static_cast<uint8_t>(power.amount));
            }
            break;
        case PowerId::CALAMITY:
            if (card_played && CardDef::get(info.card.id).type == CardType::ATTACK) {
                for (int16_t copy = 0; copy < power.amount; ++copy) {
                    create_random_card(CardFilter(CardType::ATTACK), Pile::HAND);
                }
            }
            break;
        case PowerId::CALL_OF_THE_VOID:
            if (turn_start) {
                for (int16_t copy = 0; copy < power.amount; ++copy) {
                    const CardId id = random_card();
                    if (id == CardId::NONE) break;
                    DeckCard card(id);
                    card.added_keywords |= KEYWORD_ETHEREAL;
                    create_card(card, Pile::HAND);
                }
            }
            break;
        case PowerId::SPECTRUM_SHIFT:
            if (turn_start) {
                for (int16_t copy = 0; copy < power.amount; ++copy) {
                    create_random_card(CardFilter(CardPool::COLORLESS), Pile::HAND);
                }
            }
            break;
        case PowerId::CREATIVE_AI:
            if (turn_start) {
                for (int16_t copy = 0; copy < power.amount; ++copy) {
                    create_random_card(CardFilter(CardType::POWER), Pile::HAND);
                }
            }
            break;
        case PowerId::AUTOMATION:
            if (event == CombatEvent::CARD_DRAWN) {
                power.counter = static_cast<uint8_t>(power.counter + 1);
                if (power.counter >= AUTOMATION_CARDS) {
                    power.counter = 0;
                    player.energy = static_cast<int8_t>(player.energy + power.amount);
                }
            }
            break;
        case PowerId::PAELS_TEARS:
            if (event == CombatEvent::TURN_END && player.energy > 0) schedule(DelayedType::ENERGY, power.amount);
            break;
        case PowerId::ORICHALCUM:
            if (event == CombatEvent::TURN_END && player.block == 0) add_block(power.amount);
            break;
        case PowerId::SCREAMING_FLAGON:
            if (event == CombatEvent::TURN_END && hand.empty()) damage_all_enemies(power.amount);
            break;
        case PowerId::GREMLIN_HORN:
            if (event == CombatEvent::ENEMY_KILLED) {
                player.energy = static_cast<int8_t>(player.energy + power.amount);
                draw_cards(1);
            }
            break;
        case PowerId::THE_ABACUS:
            if (event == CombatEvent::DRAW_PILE_SHUFFLED) add_block(power.amount);
            break;
        case PowerId::BIIIG_HUG:
            if (event == CombatEvent::DRAW_PILE_SHUFFLED) {
                for (int16_t copy = 0; copy < power.amount; ++copy) create_card(CardId::SOOT, Pile::DRAW);
            }
            break;
        case PowerId::PERMAFROST:
            if (card_played && CardDef::get(info.card.id).type == CardType::POWER && power.counter == 0) {
                power.counter = 1;
                add_block(power.amount);
            }
            break;
        case PowerId::GAME_PIECE:
            if (card_played && CardDef::get(info.card.id).type == CardType::POWER) draw_cards(power.amount);
            break;
        case PowerId::LOST_WISP:
            if (card_played && CardDef::get(info.card.id).type == CardType::POWER) damage_all_enemies(power.amount);
            break;
        case PowerId::CENTENNIAL_PUZZLE:
            if (event == CombatEvent::HP_LOST && power.counter == 0) {
                power.counter = 1;
                draw_cards(power.amount);
            }
            break;
        case PowerId::TURN_SUMMON:
            if (turn_start) player.summon_minion(power.amount, 0);
            break;
        case PowerId::BONE_FLUTE:
            if (event == CombatEvent::OSTY_ATTACKED) add_block(power.amount);
            break;
        case PowerId::SENTRY_MODE:
            if (turn_start) {
                for (int16_t copy = 0; copy < power.amount; ++copy) create_card(CardId::SWEEPING_GAZE, Pile::HAND);
            }
            break;
        case PowerId::NECRO_MASTERY:
            if (event == CombatEvent::OSTY_HP_LOST) {
                for (Enemy& other : enemies) {
                    if (other.is_alive()) other.lose_hp(info.amount);
                }
                settle_deaths();
            }
            break;
        case PowerId::EMOTION_CHIP:
            if (turn_start && lost_hp_last_turn_) {
                for (size_t slot = 0; slot < player.orbs.size(); ++slot) trigger_orb_passive(slot);
            }
            break;
        case PowerId::TOUGH_BANDAGES:
            if (event == CombatEvent::CARD_DISCARDED && my_turn) add_block(power.amount);
            break;
        case PowerId::TINGSHA:
            if (event == CombatEvent::CARD_DISCARDED && my_turn && has_alive_enemies()) {
                damage_enemy(enemies[random_alive_enemy_index()], power.amount);
            }
            break;
        case PowerId::JOSS_PAPER:
            if (event == CombatEvent::CARD_EXHAUSTED && ++power.counter == JOSS_PAPER_CARDS) {
                power.counter = 0;
                draw_cards(power.amount);
            }
            break;
        case PowerId::LETTER_OPENER:
            if (completes(CardType::SKILL, counters.skills_played, LETTER_OPENER_SKILLS)) damage_all_enemies(power.amount);
            break;
        case PowerId::TUNING_FORK:
            if (card_played && CardDef::get(info.card.id).type == CardType::SKILL && ++power.counter == TUNING_FORK_SKILLS) {
                power.counter = 0;
                add_block(power.amount);
            }
            break;
        case PowerId::KUNAI:
            if (completes(CardType::ATTACK, counters.attacks_played, KUNAI_ATTACKS)) {
                player.add_status(StatusType::DEXTERITY, power.amount);
            }
            break;
        case PowerId::KUSARIGAMA:
            if (completes(CardType::ATTACK, counters.attacks_played, KUSARIGAMA_ATTACKS) && has_alive_enemies()) {
                damage_enemy(enemies[random_alive_enemy_index()], power.amount);
            }
            break;
        case PowerId::MUMMIFIED_HAND:
            if (card_played && CardDef::get(info.card.id).type == CardType::POWER) {
                // Only a card that still costs something is worth picking.
                FixedVector<uint8_t, 10> paid;
                for (size_t i = 0; i < hand.size(); ++i) {
                    int cost = effective_cost(hand[i]);
                    if (cost > 0 && cost != COST_X && !CardDef::has_keyword(hand[i], KEYWORD_UNPLAYABLE)) {
                        paid.push_back(static_cast<uint8_t>(i));
                    }
                }
                if (!paid.empty()) hand[paid[next_random() % paid.size()]].free_this_turn = true;
            }
            break;
        case PowerId::IRON_CLUB:
            if (card_played && ++power.counter == IRON_CLUB_CARDS) {
                power.counter = 0;
                draw_cards(power.amount);
            }
            break;
        case PowerId::NUNCHAKU:
            if (card_played && CardDef::get(info.card.id).type == CardType::ATTACK && ++power.counter == NUNCHAKU_ATTACKS) {
                power.counter = 0;
                player.energy = static_cast<int8_t>(player.energy + power.amount);
            }
            break;
        case PowerId::RAINBOW_RING:
            if (turn_start) power.counter = 0;
            if (card_played && power.counter == 0) {
                // The tallies of the turn do not count the card being played yet.
                const CardType type = CardDef::get(info.card.id).type;
                if (counters.attacks_played + (type == CardType::ATTACK) > 0 &&
                    counters.skills_played + (type == CardType::SKILL) > 0 &&
                    counters.powers_played + (type == CardType::POWER) > 0) {
                    power.counter = 1;
                    player.add_status(StatusType::STRENGTH, power.amount);
                    player.add_status(StatusType::DEXTERITY, power.amount);
                }
            }
            break;
        case PowerId::RIPPLE_BASIN:
            if (event == CombatEvent::TURN_END && counters.attacks_played == 0) add_block(power.amount);
            break;
        case PowerId::BURNING_STICKS:
            if (event == CombatEvent::CARD_EXHAUSTED && power.counter == 0 &&
                CardDef::get(info.card.id).type == CardType::SKILL) {
                power.counter = 1;
                DeckCard copy = info.card;
                copy.free_this_turn = false;
                copy.deck_index = NOT_IN_DECK; // a gain on the copy does not reach the deck
                create_card(copy, Pile::HAND);
            }
            break;
        case PowerId::SHURIKEN:
            if (completes(CardType::ATTACK, counters.attacks_played, SHURIKEN_ATTACKS)) {
                player.add_status(StatusType::STRENGTH, power.amount);
            }
            break;
        case PowerId::PEN_NIB:
            if (card_played && CardDef::get(info.card.id).type == CardType::ATTACK && ++power.counter == PEN_NIB_ATTACKS) {
                power.counter = 0;
                pen_nib_now = true;
            }
            break;
        case PowerId::VITRUVIAN_MINION:
            if (card_played && is_minion_card(info.card.id)) minion_played_now = true;
            break;
        case PowerId::STRIKE_DUMMY:
        case PowerId::FAKE_STRIKE_DUMMY:
            if (card_played && is_strike(info.card.id)) strike_played_now = true;
            break;
        case PowerId::MINIATURE_CANNON:
            if (card_played && info.card.upgraded && CardDef::get(info.card.id).type == CardType::ATTACK) upgraded_attack_now = true;
            break;
        case PowerId::ORNAMENTAL_FAN:
            if (completes(CardType::ATTACK, counters.attacks_played, ORNAMENTAL_FAN_ATTACKS)) add_block(power.amount);
            break;
        case PowerId::SELF_FORMING_CLAY:
            // Counted when the HP are lost, given when the next turn starts.
            if (turn_start) {
                add_block(static_cast<int16_t>(power.amount * power.counter));
                power.counter = 0;
            } else if (event == CombatEvent::HP_LOST) {
                power.counter = bump(power.counter);
            }
            break;
        case PowerId::CHARONS_ASHES:
            if (event == CombatEvent::CARD_EXHAUSTED) damage_all_enemies(power.amount);
            break;
        case PowerId::FORGOTTEN_SOUL:
            if (event == CombatEvent::CARD_EXHAUSTED && has_alive_enemies()) damage_enemy(enemies[random_alive_enemy_index()], power.amount);
            break;
        case PowerId::RED_SKULL: {
            // Looked at on every event, a heal being none of them: the Strength follows the HP at the next one.
            const int wanted = player.hp * 100 <= player.max_hp * RED_SKULL_HP_PERCENT ? power.amount : 0;
            if (wanted != power.counter) {
                player.add_status(StatusType::STRENGTH, static_cast<int16_t>(wanted - power.counter));
                power.counter = static_cast<uint8_t>(wanted);
            }
            break;
        }
        case PowerId::PAELS_LEGION:
            // Asleep: one turn less each time the player's turn starts.
            if (turn_start && power.counter > 0) --power.counter;
            break;
        case PowerId::DEMON_TONGUE:
            // Poison takes its HP before the turn start is announced: the turn number tells the turns apart.
            if (event == CombatEvent::HP_LOST && my_turn && player.is_alive() &&
                power.counter != static_cast<uint8_t>(turn_number)) {
                power.counter = static_cast<uint8_t>(turn_number);
                player.heal(info.amount);
            }
            break;
        case PowerId::BRIMSTONE:
            if (turn_start) {
                player.add_status(StatusType::STRENGTH, power.amount);
                for (Enemy& enemy_hit : enemies) {
                    if (enemy_hit.is_alive()) enemy_hit.add_status(StatusType::STRENGTH, power.stacks);
                }
            }
            break;
        case PowerId::SNECKO_SKULL:
            // Added to the Poison just applied, without a second event: it is the same application.
            if (event == CombatEvent::DEBUFF_APPLIED && info.status == StatusType::POISON && info.amount > 0 && foe &&
                foe->is_alive()) {
                foe->add_status(StatusType::POISON, power.amount);
            }
            break;
        case PowerId::MINI_REGENT:
            if (turn_start) power.counter = 0;
            if (event == CombatEvent::STARS_SPENT && power.counter == 0) {
                power.counter = 1;
                player.add_status(StatusType::STRENGTH, power.amount);
            }
            break;
        case PowerId::GALACTIC_DUST:
            if (event == CombatEvent::STARS_SPENT) {
                int spent = power.counter + info.amount;
                add_block(static_cast<int16_t>(power.amount * (spent / GALACTIC_DUST_STARS)));
                power.counter = static_cast<uint8_t>(spent % GALACTIC_DUST_STARS);
            }
            break;
        case PowerId::LUNAR_PASTRY:
            if (event == CombatEvent::TURN_END) gain_stars(power.amount);
            break;
        case PowerId::REGALITE:
            if (turn_start) power.counter = 0;
            if (event == CombatEvent::CARD_CREATED && power.counter == 0) {
                power.counter = 1;
                add_block(power.amount);
            }
            break;
        case PowerId::IVORY_TILE:
            if (card_played && info.cost >= IVORY_TILE_COST) player.energy = static_cast<int8_t>(player.energy + power.amount);
            break;
        case PowerId::WRAITH_FORM:
            // A debuff the player gives themselves: Artifact cancels it.
            if (turn_start) player.receive_status(StatusType::DEXTERITY, static_cast<int16_t>(-power.amount));
            break;
        default:
            // Rules read where they apply: BARRICADE, KEEP_HAND, ACCELERANT, THUNDER, ICE_CREAM, STURDY_CLAMP,
            // RAZOR_TOOTH, LIZARD_TAIL, UNCEASING_TOP, GOLD_PLATED_CABLES, METRONOME, BOOK_REPAIR_KNIFE,
            // FLAME_BARRIER, REFLECT, CALCIFY, SIC_EM, DEMISE, DUPLICATION, STABLE_SERUM.
            break;
    }
}

void CombatEngine::gain_block(int16_t amount) {
    // Panic Button: a card gives no Block.
    if (resolving_card && no_card_block_turns > 0) return;
    int16_t before = player.block;
    player.gain_block(amount);
    // Paël's Legion doubles the Block a card gives, then sleeps; the Block gained is doubled, Dexterity included.
    if (resolving_card && player.block > before) {
        ActivePower* legion = find_power(PowerId::PAELS_LEGION);
        if (legion != nullptr && legion->counter == 0) {
            player.block = static_cast<int16_t>(player.block + (player.block - before));
            legion->counter = static_cast<uint8_t>(legion->amount + 1);
        }
    }
    // Vambrace: the first Block a card gives each combat is doubled.
    if (resolving_card && player.block > before) {
        ActivePower* vambrace = find_power(PowerId::VAMBRACE);
        if (vambrace != nullptr && vambrace->counter == 0) {
            player.block = static_cast<int16_t>(player.block + (player.block - before));
            vambrace->counter = 1;
        }
    }
    // Unmovable doubles the first Block a card gives each turn, Shadowmeld every one of the turn.
    if (player.block > before) {
        int factor = 1;
        if (resolving_card && !unmovable_used && has_power(PowerId::UNMOVABLE)) {
            unmovable_used = true;
            factor *= 2;
        }
        if (double_block_turn) factor *= 2;
        if (resolving_card && minion_played_now) factor *= 2;
        if (factor > 1) player.block = static_cast<int16_t>(before + (player.block - before) * factor);
    }
    if (player.block > before) {
        EventInfo info;
        info.amount = static_cast<int16_t>(player.block - before);
        emit(CombatEvent::BLOCK_GAINED, info);
    }
}

void CombatEngine::add_block(int16_t amount) {
    if (amount <= 0) return;
    player.block = static_cast<int16_t>(player.block + amount);
    EventInfo info;
    info.amount = amount;
    emit(CombatEvent::BLOCK_GAINED, info);
}

void CombatEngine::afflict(Enemy& target, StatusType type, int16_t amount) {
    if (amount == 0) return;
    // Unsettling Lamp: the first card of the fight that debuffs an enemy has every debuff it applies doubled.
    if (resolving_card && is_debuff(type, amount)) {
        ActivePower* lamp = find_power(PowerId::UNSETTLING_LAMP);
        if (lamp != nullptr && lamp->counter == 0) {
            lamp->counter = 1;
            lamp_doubling = true;
        }
        if (lamp_doubling) amount = static_cast<int16_t>(amount * 2);
    }
    // Artifact cancels the debuff: nothing was applied, nothing reacts.
    if (!target.receive_status(type, amount)) return;
    if (type == StatusType::DOOM) counters.doom_applied = bump(counters.doom_applied);
    if (is_debuff(type, amount)) {
        EventInfo info;
        info.enemy = index_of(target);
        info.status = type;
        info.amount = amount;
        emit(CombatEvent::DEBUFF_APPLIED, info);
    }
}

void CombatEngine::damage_enemy(Enemy& target, int16_t amount) {
    if (!target.is_alive()) return;
    const int16_t block_before = target.block;
    target.take_damage(amount);
    if (block_before > 0 && target.block == 0) {
        EventInfo info;
        info.enemy = index_of(target);
        emit(CombatEvent::BLOCK_BROKEN, info);
    }
    settle_deaths();
}

void CombatEngine::damage_all_enemies(int16_t amount) {
    for (Enemy& target : enemies) damage_enemy(target, amount);
}

int16_t CombatEngine::with_focus(int16_t base) const {
    return std::max<int16_t>(0, static_cast<int16_t>(base + player.get_status(StatusType::FOCUS)));
}

int CombatEngine::unique_orbs() const {
    int count = 0;
    for (OrbType type : { OrbType::LIGHTNING, OrbType::FROST, OrbType::DARK, OrbType::PLASMA }) {
        for (const Orb& orb : player.orbs) {
            if (orb == type) {
                ++count;
                break;
            }
        }
    }
    return count;
}

void CombatEngine::channel_orb(OrbType type) {
    if (type == OrbType::NONE || player.orb_slots == 0) return;
    // No free slot: the oldest orb is evoked to make room.
    if (player.orbs.size() >= player.orb_slots) evoke_orb(0);
    Orb orb(type);
    if (type == OrbType::DARK) orb.charge = DARK_PASSIVE;
    player.orbs.push_back(orb);
    counters.combat_orbs_channeled = bump(counters.combat_orbs_channeled);
    if (type == OrbType::LIGHTNING) counters.combat_lightning_channeled = bump(counters.combat_lightning_channeled);
    ActivePower* metronome = find_power(PowerId::METRONOME);
    if (metronome && metronome->counter == 0 && counters.combat_orbs_channeled >= METRONOME_ORBS) {
        metronome->counter = 1;
        damage_all_enemies(metronome->amount);
    }
}

void CombatEngine::change_orb_slots(int delta) {
    int slots = std::clamp(player.orb_slots + delta, 0, static_cast<int>(MAX_ORB_SLOTS));
    player.orb_slots = static_cast<uint8_t>(slots);
    while (player.orbs.size() > player.orb_slots) player.orbs.pop_back();
}

void CombatEngine::evoke_orb(size_t slot, int times) {
    if (slot >= player.orbs.size()) return;
    const Orb orb = player.orbs[slot];
    player.orbs.erase(slot);

    for (int i = 0; i < times; ++i) {
        switch (orb.type) {
            case OrbType::LIGHTNING:
                if (has_alive_enemies()) {
                    Enemy& struck = enemies[random_alive_enemy_index()];
                    damage_enemy(struck, static_cast<int16_t>(with_focus(LIGHTNING_EVOKE) + power_amount(PowerId::THUNDER) + power_amount(PowerId::INFUSED_CORE)));
                }
                break;
            case OrbType::FROST:
                add_block(with_focus(FROST_EVOKE)); // orb Block ignores Dexterity
                break;
            case OrbType::DARK: {
                // The stored charge goes to the weakest enemy.
                Enemy* weakest = nullptr;
                for (Enemy& foe : enemies) {
                    if (foe.is_alive() && (!weakest || foe.hp < weakest->hp)) weakest = &foe;
                }
                if (weakest) damage_enemy(*weakest, orb.charge);
                break;
            }
            case OrbType::PLASMA:
                player.energy = static_cast<int8_t>(player.energy + PLASMA_EVOKE);
                break;
            default:
                break;
        }
    }
}

void CombatEngine::trigger_orb_passive(size_t slot, Enemy* target) {
    if (slot >= player.orbs.size()) return;
    Orb& orb = player.orbs[slot];
    switch (orb.type) {
        case OrbType::LIGHTNING:
            if (!target && has_alive_enemies()) target = &enemies[random_alive_enemy_index()];
            if (target) damage_enemy(*target, static_cast<int16_t>(with_focus(LIGHTNING_PASSIVE) + power_amount(PowerId::INFUSED_CORE)));
            break;
        case OrbType::FROST:
            add_block(with_focus(FROST_PASSIVE));
            break;
        case OrbType::DARK:
            orb.charge = static_cast<int16_t>(orb.charge + with_focus(DARK_PASSIVE));
            break;
        case OrbType::PLASMA:
            player.energy = static_cast<int8_t>(player.energy + PLASMA_PASSIVE);
            break;
        default:
            break;
    }
}

void CombatEngine::trigger_turn_passives(bool turn_start) {
    // Plasma gives its energy when the turn starts; the other orbs act when it ends.
    for (size_t slot = 0; slot < player.orbs.size(); ++slot) {
        if ((player.orbs[slot] == OrbType::PLASMA) == turn_start) trigger_orb_passive(slot);
    }
    if (has_power(PowerId::GOLD_PLATED_CABLES) && !player.orbs.empty() &&
        (player.orbs[0] == OrbType::PLASMA) == turn_start) {
        trigger_orb_passive(0);
    }
}

void CombatEngine::reap(Enemy& target) {
    if (target.is_alive() && target.get_status(StatusType::DOOM) >= target.hp) {
        target.hp = 0;
        if (!target.minion) player.heal(power_amount(PowerId::BOOK_REPAIR_KNIFE));
        settle_deaths();
    }
}

void CombatEngine::recall_to_hand(CardId id) {
    for (size_t i = discard_pile.size(); i-- > 0 && hand.size() < hand.capacity();) {
        if (discard_pile[i].id == id) {
            hand.push_back(discard_pile[i]);
            discard_pile.erase(i);
        }
    }
    for (size_t i = draw_pile.size(); i-- > 0 && hand.size() < hand.capacity();) {
        if (draw_pile[i].id == id) {
            hand.push_back(draw_pile[i]);
            draw_pile.erase(i);
        }
    }
}

void CombatEngine::shuffle_discard_into_draw() {
    while (!discard_pile.empty()) {
        draw_pile.push_back(discard_pile.back());
        discard_pile.pop_back();
    }
    shuffle_draw_pile();
    if (!draw_pile.empty()) emit(CombatEvent::DRAW_PILE_SHUFFLED);
}

bool CombatEngine::draw_card() {
    if (no_draw_this_turn) return false;
    if (hand.size() >= hand.capacity()) return false; // Hand limit

    if (draw_pile.empty()) {
        shuffle_discard_into_draw();
    }

    if (draw_pile.empty()) return false;

    DeckCard card = draw_pile.back();
    draw_pile.pop_back();
    // Confused: the card drawn costs 0 to 3 at random, until it is drawn again. X and Unplayable cards keep theirs.
    if (player.get_status(StatusType::CONFUSED) > 0 && !CardDef::is_x_cost(card.id, card.upgraded) &&
        !CardDef::has_keyword(card, KEYWORD_UNPLAYABLE)) {
        const int printed = CardDef::get_cost(card.id, card.upgraded);
        card.cost_delta = static_cast<int8_t>(static_cast<int>(next_random() % 4) - printed);
    }
    // Cards that change when they are drawn.
    if (card.id == CardId::KINGLY_KICK) {
        card.cost_delta = static_cast<int8_t>(card.cost_delta - 1);
    } else if (card.id == CardId::KINGLY_PUNCH) {
        card.bonus = bump(card.bonus, CardDef::get_secondary(card.id, card.upgraded));
    } else if (card.id == CardId::VOID_CARD) {
        player.energy = static_cast<int8_t>(std::max(0, player.energy - CardDef::get_value(card.id, card.upgraded)));
    }
    hand.push_back(card);
    counters.cards_drawn = bump(counters.cards_drawn);
    counters.combat_cards_drawn = bump(counters.combat_cards_drawn);
    EventInfo info;
    info.card = card;
    info.opening_draw = opening_draw_;
    emit(CombatEvent::CARD_DRAWN, info);
    return true;
}

void CombatEngine::exhaust_card(DeckCard card) {
    if (exhaust_pile.size() < exhaust_pile.capacity()) {
        exhaust_pile.push_back(card);
    }
    if (exhausted_this_turn < UINT8_MAX) ++exhausted_this_turn;
    if (card.id == CardId::DRUM_OF_BATTLE) {
        player.energy = static_cast<int8_t>(player.energy + CardDef::get_secondary(card.id, card.upgraded));
    }
    EventInfo info;
    info.card = card;
    emit(CombatEvent::CARD_EXHAUSTED, info);
}

DeckCard CombatEngine::exhaust_from_hand(size_t index) {
    DeckCard card = hand[index];
    hand.erase(index);
    exhaust_card(card);
    return card;
}

void CombatEngine::discard_from_hand(size_t index) {
    DeckCard card = hand[index];
    discard_pile.push_back(card);
    hand.erase(index);
    counters.cards_discarded = bump(counters.cards_discarded);
    EventInfo info;
    info.card = card;
    emit(CombatEvent::CARD_DISCARDED, info);
    // Sly: discarded from the hand during the player's turn, the card is played for free once the discard is over.
    if (state == CombatState::PLAYER_TURN && CardDef::is_sly(card) && !discard_pile.empty() &&
        discard_pile.back().id == card.id && sly_pending.push_back(card)) {
        discard_pile.pop_back();
    }
}

void CombatEngine::play_sly_pending() {
    // A Sly card played by this loop may discard another one: the queue is read again until it is empty.
    for (int guard = 0; !sly_pending.empty() && guard < 24; ++guard) {
        DeckCard card = sly_pending[0];
        sly_pending.erase(0);
        card.sly_this_turn = false;
        if (state != CombatState::PLAYER_TURN || !has_alive_enemies()) {
            discard_pile.push_back(card);
            continue;
        }
        autoplay(card, -1, AfterPlay::DISCARD);
    }
    while (!sly_pending.empty()) {
        discard_pile.push_back(sly_pending[0]);
        sly_pending.erase(0);
    }
}

bool CombatEngine::card_fits(CardId id, const CardFilter& filter) const {
    if (id == CardId::NONE || id >= CardId::COUNT) return false;
    const CardMeta& meta = card_meta(id);
    // The heroes follow each other in the same order in CharacterId and in CardPool.
    const CardPool hero_pool = static_cast<CardPool>(static_cast<uint8_t>(current_character) + 1);
    if (meta.pool != (filter.pool == CardPool::NONE ? hero_pool : filter.pool)) return false;
    if (filter.rarity == CardRarity::NONE) {
        if (meta.rarity != CardRarity::COMMON && meta.rarity != CardRarity::UNCOMMON && meta.rarity != CardRarity::RARE) {
            return false;
        }
    } else if (meta.rarity != filter.rarity) {
        return false;
    }
    if (filter.cost >= 0 && CardDef::get_cost(id, filter.cost_upgraded) != filter.cost) return false;
    return filter.any_type || CardDef::get(id).type == filter.type;
}

CardId CombatEngine::random_card(const CardFilter& filter) {
    uint16_t count = 0;
    for (uint16_t i = 1; i < static_cast<uint16_t>(CardId::COUNT); ++i) {
        if (card_fits(static_cast<CardId>(i), filter)) ++count;
    }
    // No number is drawn for nothing: the sequence of the fight only moves when a card comes out.
    if (count == 0) return CardId::NONE;

    uint16_t pick = static_cast<uint16_t>((next_random() >> 8) % count);
    for (uint16_t i = 1; i < static_cast<uint16_t>(CardId::COUNT); ++i) {
        if (card_fits(static_cast<CardId>(i), filter) && pick-- == 0) return static_cast<CardId>(i);
    }
    return CardId::NONE;
}

bool CombatEngine::create_random_card(const CardFilter& filter, Pile pile, bool upgraded) {
    CardId id = random_card(filter);
    if (id == CardId::NONE) return false;
    create_card(DeckCard(id, upgraded), pile);
    return true;
}

void CombatEngine::create_card(DeckCard card, Pile pile, bool by_player) {
    if (pile == Pile::HAND && hand.push_back(card)) {
        // in hand
    } else if (pile == Pile::DRAW) {
        if (draw_pile.push_back(card)) {
            size_t place = next_random() % draw_pile.size();
            for (size_t i = draw_pile.size() - 1; i > place; --i) draw_pile[i] = draw_pile[i - 1];
            draw_pile[place] = card;
        }
    } else {
        discard_pile.push_back(card);
    }
    if (!by_player) return;
    counters.combat_cards_created = bump(counters.combat_cards_created);
    // Rocket Punch gets cheaper with each Status card created, until it is played.
    if (CardDef::get(card.id).type == CardType::STATUS) change_cost_of_copies(CardId::ROCKET_PUNCH, -1);
    EventInfo info;
    info.card = card;
    emit(CombatEvent::CARD_CREATED, info);
}

void CombatEngine::add_shivs(int count, bool upgraded, Enchantment enchantment) {
    DeckCard shiv(CardId::SHIV, upgraded);
    shiv.enchantment = enchantment;
    for (int copy = 0; copy < count; ++copy) create_card(shiv, Pile::HAND);
}

void CombatEngine::add_souls(int count, Pile pile, bool upgraded) {
    for (int copy = 0; copy < count; ++copy) create_card(DeckCard(SOUL_CARD, upgraded), pile);
}

int CombatEngine::exhausted_souls() const {
    int count = 0;
    for (const DeckCard& card : exhaust_pile) count += is_soul(card.id);
    return count;
}

void CombatEngine::forge(int amount) {
    if (amount <= 0) return;
    if (!blade_forged_) {
        blade_forged_ = true;
        create_card(CardId::SOVEREIGN_BLADE, Pile::HAND);
    }
    forged = static_cast<int16_t>(std::min(forged + amount, 9999));
    // The damage is kept by each copy, like the other gains of a combat: the text of the card shows it.
    boost_copies(CardId::SOVEREIGN_BLADE, amount);
    for (DeckCard& card : returning_cards) {
        if (card.id == CardId::SOVEREIGN_BLADE) card.bonus = bump(card.bonus, amount);
    }
}

int16_t CombatEngine::hit(Enemy& target, int16_t base) {
    counters.hits_dealt = bump(counters.hits_dealt);
    if (int8_t at = index_of(target); at >= 0 && at < 3) counters.hits_on_enemy[at] = bump(counters.hits_on_enemy[at]);
    bool was_alive = target.is_alive();
    // Percentage modifiers add up; Shadow Step then doubles the total.
    int32_t percent = 100;
    if (target.get_status(StatusType::VULNERABLE) > 0) {
        percent += power_amount(PowerId::CRUELTY);
        percent += power_amount(PowerId::PAPER_PHROG);
    }
    if (target.get_status(StatusType::WEAK) > 0) percent += power_amount(PowerId::TRACKING);
    if (resolving_card && counters.attacks_played == 0) percent += power_amount(PowerId::LETHALITY);
    if (attack_double_now) percent *= 2;
    if (resolving_card && triple_next_attack) percent *= 3;
    if (pen_nib_now && resolving_card) percent *= 2;
    if (minion_played_now && resolving_card) percent *= 2;
    if (strike_played_now && resolving_card) {
        base = static_cast<int16_t>(base + power_amount(PowerId::STRIKE_DUMMY) + power_amount(PowerId::FAKE_STRIKE_DUMMY));
    }
    if (upgraded_attack_now && resolving_card) base = static_cast<int16_t>(base + power_amount(PowerId::MINIATURE_CANNON));
    int16_t damage = attack_damage(base, player, target, percent);
    // The Boot: an Attack hit that would deal 1 to 4 damage (before Block) deals 5 instead.
    if (damage > 0 && damage <= THE_BOOT_THRESHOLD && has_power(PowerId::THE_BOOT)) damage = THE_BOOT_MINIMUM;
    const int16_t block_before = target.block;
    int16_t lost = target.take_damage(damage);
    if (was_alive && !target.is_alive() && !target.minion) fatal_kills = bump(fatal_kills);
    if (was_alive) {
        EventInfo info;
        info.enemy = index_of(target);
        info.amount = lost;
        emit(CombatEvent::ATTACK_HIT, info);
        if (lost > 0) emit(CombatEvent::UNBLOCKED_DAMAGE_DEALT, info);
        if (block_before > 0 && target.block == 0) emit(CombatEvent::BLOCK_BROKEN, info);
        // The enemy's Thorns hurt the player, once per hit.
        damage_player(target.get_status(StatusType::THORNS));
        settle_deaths();
    }
    return lost;
}

int16_t CombatEngine::osty_hit(Enemy& target, int16_t base) {
    const bool was_alive = target.is_alive();
    int16_t lost = hit(target, static_cast<int16_t>(base + power_amount(PowerId::CALCIFY)));
    if (was_alive) {
        // Sic 'Em: Summon for each hit on the enemy it marked this turn.
        const int8_t at = index_of(target);
        int16_t summoned = 0;
        for (const ActivePower& power : powers) {
            if (power.id == PowerId::SIC_EM && power.target == at) summoned = static_cast<int16_t>(summoned + power.amount);
        }
        if (summoned > 0) player.summon_minion(summoned, 0);
    }
    return lost;
}

int16_t CombatEngine::osty_attack(Enemy& target, int16_t base, int hits) {
    if (!player.minion_alive()) return 0;
    int16_t lost = 0;
    for (int blow = 0; blow < hits && target.is_alive(); ++blow) lost = osty_hit(target, base);
    note_osty_attack();
    return lost;
}

void CombatEngine::osty_attack_all(int16_t base) {
    if (!player.minion_alive()) return;
    for (Enemy& target : enemies) {
        if (target.is_alive()) osty_hit(target, base);
    }
    note_osty_attack();
}

void CombatEngine::heal_osty(int16_t amount) {
    player.heal_minion(amount);
}

void CombatEngine::kill_osty() {
    if (!player.minion_alive()) return;
    note_osty_hp_lost(player.kill_minion());
}

void CombatEngine::note_osty_attack() {
    counters.osty_attacks = bump(counters.osty_attacks);
    emit(CombatEvent::OSTY_ATTACKED);
}

void CombatEngine::note_osty_hp_lost(int16_t amount) {
    if (amount <= 0) return;
    EventInfo info;
    info.amount = amount;
    emit(CombatEvent::OSTY_HP_LOST, info);
}

void CombatEngine::gain_stars(int amount) {
    if (amount <= 0) return;
    player.add_stars(static_cast<uint8_t>(std::min(amount, 99)));
    counters.stars_gained = bump(counters.stars_gained, amount);
    EventInfo info;
    info.amount = static_cast<int16_t>(amount);
    emit(CombatEvent::STARS_GAINED, info);
}

void CombatEngine::spend_stars(int amount) {
    amount = std::min<int>(amount, player.sovereign_stars);
    if (amount <= 0) return;
    player.sovereign_stars = static_cast<uint8_t>(player.sovereign_stars - amount);
    counters.stars_spent = bump(counters.stars_spent, amount);
    EventInfo info;
    info.amount = static_cast<int16_t>(amount);
    emit(CombatEvent::STARS_SPENT, info);
}

void CombatEngine::note_player_hp_lost(int16_t amount) {
    if (amount <= 0) return;
    counters.hp_lost = static_cast<int16_t>(counters.hp_lost + amount);
    counters.combat_hp_losses = bump(counters.combat_hp_losses);
    if (player.hp <= 0) {
        ActivePower* tail = find_power(PowerId::LIZARD_TAIL);
        if (tail && tail->counter == 0) {
            tail->counter = 1;
            player.hp = static_cast<int16_t>(player.max_hp * tail->amount / 100);
        }
    }
    if (player.hp <= 0 && fairy_bottles > 0) {
        // Fairy in a Bottle: the potion is discarded and heals to 30 % of Max HP.
        --fairy_bottles;
        fairy_used = static_cast<uint8_t>(fairy_used + 1);
        player.hp = static_cast<int16_t>(std::max(1, player.max_hp * 30 / 100));
    }
    EventInfo info;
    info.amount = amount;
    emit(CombatEvent::HP_LOST, info);
}

void CombatEngine::damage_player(int16_t amount) {
    if (amount <= 0) return;
    int16_t hp_before = player.hp;
    player.take_damage(amount);
    note_player_hp_lost(static_cast<int16_t>(hp_before - player.hp));
    if (!player.is_alive()) {
        state = CombatState::DEFEAT;
    }
}

void CombatEngine::lose_hp(int16_t amount) {
    if (amount <= 0) return;
    int16_t hp_before = player.hp;
    player.lose_hp(amount); // Intangible and Buffer apply
    note_player_hp_lost(static_cast<int16_t>(hp_before - player.hp));
    if (!player.is_alive()) {
        state = CombatState::DEFEAT;
    }
}

void CombatEngine::gain_temporary(StatusType type, int8_t amount) {
    if (type == StatusType::STRENGTH) {
        temp_strength = static_cast<int8_t>(temp_strength + amount);
    } else if (type == StatusType::DEXTERITY) {
        temp_dexterity = static_cast<int8_t>(temp_dexterity + amount);
    } else if (type == StatusType::FOCUS) {
        temp_focus = static_cast<int8_t>(temp_focus + amount);
    } else {
        return;
    }
    player.add_status(type, amount);
}

void CombatEngine::sap_strength(Enemy& target, int16_t amount) {
    // Measured rather than assumed: the status list can be full.
    int16_t before = target.get_status(StatusType::STRENGTH);
    afflict(target, StatusType::STRENGTH, static_cast<int16_t>(-amount));
    target.temp_strength_loss = static_cast<int16_t>(target.temp_strength_loss + before - target.get_status(StatusType::STRENGTH));
}

void CombatEngine::schedule(DelayedType type, int16_t amount, uint8_t delay, uint8_t repeat) {
    if (amount <= 0 || repeat == 0) return;
    delayed_effects.push_back(DelayedEffect{ type, amount, std::max<uint8_t>(delay, 1), repeat });
}

int CombatEngine::run_delayed_effects(bool end_of_turn) {
    int extra_draw = 0;
    for (size_t i = 0; i < delayed_effects.size();) {
        DelayedEffect& effect = delayed_effects[i];
        if ((effect.type == DelayedType::DAMAGE_ALL) != end_of_turn) {
            ++i;
            continue;
        }
        if (effect.delay > 1) {
            --effect.delay;
            ++i;
            continue;
        }
        switch (effect.type) {
            case DelayedType::ENERGY:
                player.energy = static_cast<int8_t>(player.energy + effect.amount);
                break;
            case DelayedType::BLOCK:
                add_block(effect.amount);
                break;
            case DelayedType::DRAW:
                extra_draw += effect.amount;
                break;
            case DelayedType::STARS:
                gain_stars(effect.amount);
                break;
            case DelayedType::SUMMON:
                player.summon_minion(effect.amount, 0);
                break;
            case DelayedType::CHANNEL_LIGHTNING:
                for (int16_t orb = 0; orb < effect.amount; ++orb) {
                    channel_orb(OrbType::LIGHTNING);
                }
                break;
            case DelayedType::DAMAGE_ALL:
                damage_all_enemies(effect.amount);
                break;
            case DelayedType::PICK_FROM_DRAW:
                turn_picks_from_draw_ = bump(turn_picks_from_draw_, effect.amount);
                break;
            case DelayedType::COPIES_TO_HAND:
                for (uint8_t copy = 0; copy < NIGHTMARE_COPIES; ++copy) {
                    create_card(DeckCard(static_cast<CardId>(effect.amount / 2), (effect.amount % 2) != 0), Pile::HAND);
                }
                break;
        }
        if (--effect.repeat == 0) {
            delayed_effects.erase(i);
        } else {
            ++i;
        }
    }
    return extra_draw;
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
        e.roll_hp(rng_state_ ^ (0x85EBCA6Bu * static_cast<uint32_t>(i + 1)));
        enemies.push_back(e);
    }
    if (enemies.empty()) {
        Enemy e;
        e.init(EnemyType::CULTIST);
        enemies.push_back(e);
    }
    selected_target_index = 0;

    turn_number = 1;
    plan_enemies();
    selected_card_index = 0;
    init_starter_deck(character);
    state = CombatState::PLAYER_TURN;

    open_first_turn();
    set_log("Combat started!");
}

void CombatEngine::open_first_turn() {
    reset_combat_state();
    begin_first_turn();
}

void CombatEngine::plan_enemies() {
    uint8_t alive = 0;
    for (const Enemy& foe : enemies) alive += foe.is_alive();
    for (size_t i = 0; i < enemies.size(); ++i) {
        Enemy& foe = enemies[i];
        if (!foe.is_alive()) continue;
        // A fight starts every pattern again, each monster drawing from a stream of its own: the draws of the
        // monsters never move the sequence that shuffles the piles.
        if (turn_number == 1) foe.reset_pattern(rng_state_ ^ (0x9E3779B1u * static_cast<uint32_t>(i + 1)));
        PlanContext context;
        context.allies = static_cast<uint8_t>(alive - 1);
        context.slot = static_cast<uint8_t>(i);
        foe.plan_turn(turn_number, context);
    }
    if (!enemies.empty()) enemy = enemies[0];
}

void CombatEngine::reset_combat_state() {
    counters = CombatCounters();
    delayed_effects.clear();
    returning_cards.clear();
    keep_block_turns = 0;
    retain_hand = false;
    no_draw_this_turn = false;
    double_block_turn = false;
    unmovable_used = false;
    attack_double_next = false;
    attack_double_now = false;
    triple_next_attack = false;
    no_card_block_turns = 0;
    rebound_pending = false;
    fatal_kills = 0;
    powers.clear();
    powers_in_play.clear();
    permanent_gains.clear();
    pending_choices.clear();
    clear_offers();
    turn_picks_from_draw_ = 0;
    end_turn_requested = false;
    extra_turn_pending = false;
    event_depth_ = 0;
    autoplay_depth_ = 0;
    vakuu_turn_active = false;
    vakuu_plays_ = 0;
    temp_strength = 0;
    temp_dexterity = 0;
    temp_focus = 0;
    forged = 0;
    blade_forged_ = false;
    alive_mask_ = 0;
    for (size_t i = 0; i < enemies.size(); ++i) {
        if (enemies[i].is_alive()) alive_mask_ = static_cast<uint8_t>(alive_mask_ | (1u << i));
    }
}

void CombatEngine::begin_first_turn() {
    // Innate cards go on top of the draw pile (its back), in their shuffled order.
    FixedVector<DeckCard, 50> innate;
    for (size_t i = draw_pile.size(); i-- > 0;) {
        if (CardDef::has_keyword(draw_pile[i], KEYWORD_INNATE)) {
            innate.push_back(draw_pile[i]);
            draw_pile.erase(i);
        }
    }
    for (const DeckCard& card : innate) draw_pile.push_back(card);

    start_player_turn();

    // More Innate cards than the opening draw: they are all drawn, up to the hand limit.
    for (size_t extra = 5; extra < innate.size(); ++extra) {
        if (!draw_card()) break;
    }
    counters.start_turn();
    // Whispering Earring: this turn is Vakuu's.
    vakuu_plays_ = 0;
    vakuu_turn_active = has_power(PowerId::WHISPERING_EARRING) && state == CombatState::PLAYER_TURN;
}

void CombatEngine::start_player_turn() {
    state = CombatState::PLAYER_TURN;
    // Beating Remnant: the HP lost from this turn on, until the next one, is capped.
    player.hp_loss_budget = has_power(PowerId::BEATING_REMNANT) ? power_amount(PowerId::BEATING_REMNANT) : -1;
    int16_t hp_before = player.hp;
    player.apply_poison_tick();
    lost_hp_last_turn_ = counters.hp_lost > 0;
    counters.start_turn();
    note_player_hp_lost(static_cast<int16_t>(hp_before - player.hp));
    if (!player.is_alive()) {
        state = CombatState::DEFEAT;
        set_log("You were defeated!");
        return;
    }
    int16_t kept_block = std::min(player.block, power_amount(PowerId::STURDY_CLAMP));
    if (has_power(PowerId::BARRICADE)) kept_block = player.block;
    if (keep_block_turns > 0) {
        --keep_block_turns;
        kept_block = player.block;
    }
    // Ice Cream: the energy left from the previous turn is added to the new one.
    int8_t kept_energy = (has_power(PowerId::ICE_CREAM) && turn_number > 1) ? std::max<int8_t>(player.energy, 0) : 0;
    player.reset_turn();
    player.block = kept_block;
    player.energy = static_cast<int8_t>(player.energy + kept_energy);
    player.decay_plating();
    // Flame Barrier and Reflect answered the attacks until this turn.
    for (size_t i = powers.size(); i-- > 0;) {
        if (powers[i].id == PowerId::FLAME_BARRIER || powers[i].id == PowerId::REFLECT) powers.erase(i);
    }
    exhausted_this_turn = 0;
    retain_hand = false;
    no_draw_this_turn = false;
    rebound_pending = false;
    unmovable_used = false;
    attack_double_now = attack_double_next;
    attack_double_next = false;
    player.halve_vulnerable_damage = false;
    for (Enemy& foe : enemies) {
        if (foe.debilitate_turns > 0) --foe.debilitate_turns;
        if (foe.damage_down_turns > 0) --foe.damage_down_turns;
    }
    if (no_card_block_turns > 0) --no_card_block_turns;
    trigger_turn_passives(true);
    int extra_draw = run_delayed_effects(false);
    turn_draw_bonus_ = 0;
    emit(CombatEvent::TURN_START);
    extra_draw += turn_draw_bonus_;

    // Bombardment plays itself from the exhaust pile at the start of each turn, and stays there.
    for (size_t i = 0; i < exhaust_pile.size() && has_alive_enemies(); ++i) {
        if (exhaust_pile[i].id == CardId::BOMBARDMENT) autoplay(exhaust_pile[i], -1, AfterPlay::NOWHERE);
    }

    // Cards played with Return come back before the draw.
    for (const DeckCard& card : returning_cards) {
        if (!hand.push_back(card)) discard_pile.push_back(card);
    }
    returning_cards.clear();

    opening_draw_ = true;
    draw_cards(5 + extra_draw);
    opening_draw_ = false;
    // Fiddle: the opening draw is the last one of the turn.
    if (has_power(PowerId::FIDDLE)) no_draw_this_turn = true;
    // Foregone Conclusion: cards of the draw pile, picked once the hand is drawn.
    if (turn_picks_from_draw_ > 0) {
        ask_pile_choice(Pile::DRAW, ChoiceKind::TO_HAND, turn_picks_from_draw_);
        turn_picks_from_draw_ = 0;
    }
    // Tools of the Trade: once the hand is drawn, draw then discard.
    if (int16_t tools = power_amount(PowerId::TOOLS_OF_THE_TRADE); tools > 0) {
        draw_cards(tools);
        CardChoice choice;
        choice.count = static_cast<uint8_t>(tools);
        ask_choice(choice);
    }
    // Entropy: one card of the new hand is transformed.
    if (int16_t entropy = power_amount(PowerId::ENTROPY); entropy > 0) {
        CardChoice choice;
        choice.kind = ChoiceKind::TRANSFORM;
        choice.count = static_cast<uint8_t>(entropy);
        ask_choice(choice);
    }
    // Tyranny: one card of the new hand is exhausted for each card it drew.
    if (int16_t tyranny = power_amount(PowerId::TYRANNY); tyranny > 0) {
        CardChoice choice;
        choice.kind = ChoiceKind::EXHAUST;
        choice.count = static_cast<uint8_t>(tyranny);
        ask_choice(choice);
    }
    // Toasty Mittens: one card of the new hand is exhausted.
    if (has_power(PowerId::TOASTY_MITTENS)) ask_choice(CardChoice{ ChoiceKind::EXHAUST, 1 });
    // Only the cards drawn by effects during the turn count as drawn this turn.
    counters.cards_drawn = 0;
    selected_card_index = 0;
    settle_deaths();
    selected_target_index = get_first_alive_enemy_index();

    // Start-of-turn powers can end the fight either way.
    if (!enemies.empty()) {
        enemy = enemies[0];
    }
    if (!player.is_alive()) {
        state = CombatState::DEFEAT;
        set_log("You were defeated!");
    } else if (!has_alive_enemies()) {
        state = CombatState::VICTORY;
        set_log("Victory!");
    }
}

bool CombatEngine::play_condition_met(PlayCondition condition) const {
    switch (condition) {
        case PlayCondition::DRAW_PILE_EMPTY:
            return draw_pile.empty();
        case PlayCondition::HAND_ALL_ATTACKS:
            for (const DeckCard& card : hand) {
                if (CardDef::get(card.id).type != CardType::ATTACK) return false;
            }
            return true;
        case PlayCondition::OSTY_ALIVE:
            return player.minion_alive();
        default:
            return true;
    }
}

PlayBlocker CombatEngine::play_blocker(int index) const {
    if (state != CombatState::PLAYER_TURN || end_turn_requested) return PlayBlocker::NOT_PLAYABLE;
    if (index < 0 || static_cast<size_t>(index) >= hand.size()) return PlayBlocker::NOT_PLAYABLE;

    if (choice_pending()) return PlayBlocker::CHOICE;

    const DeckCard& card = hand[index];
    if (CardDef::has_keyword(card, KEYWORD_UNPLAYABLE)) return PlayBlocker::NOT_PLAYABLE;
    // Restrictions set by the cards held in hand.
    bool plays_first = CardDef::has_keyword(card, KEYWORD_PLAY_FIRST);
    for (const DeckCard& held : hand) {
        if (!plays_first && CardDef::has_keyword(held, KEYWORD_PLAY_FIRST)) return PlayBlocker::PLAY_FIRST;
        if (CardDef::has_keyword(held, KEYWORD_LIMIT_PLAYS) && counters.cards_played() >= MAX_PLAYS_WHEN_LIMITED) {
            return PlayBlocker::PLAY_LIMIT;
        }
    }
    // Velvet Choker: the card limit of the turn.
    if (has_power(PowerId::VELVET_CHOKER) && counters.cards_played() >= power_amount(PowerId::VELVET_CHOKER)) {
        return PlayBlocker::PLAY_LIMIT;
    }
    if (!play_condition_met(CardDef::get(card.id).condition)) return PlayBlocker::CONDITION;

    // An X card can be played for X = 0, in energy as in Stars.
    int cost = effective_cost(card);
    if (cost != COST_X && player.energy < cost) return PlayBlocker::ENERGY;
    uint8_t star_cost = CardDef::get_star_cost(card.id, card.upgraded);
    if (star_cost != COST_X && player.sovereign_stars < star_cost) return PlayBlocker::STARS;
    return PlayBlocker::NONE;
}

bool CombatEngine::costs_nothing(const DeckCard& card) const {
    if (card.free_this_turn) return true;
    // Brilliant Scarf: the 5th card played each turn is free.
    if (counters.cards_played() == BRILLIANT_SCARF_CARD - 1 && has_power(PowerId::BRILLIANT_SCARF)) return true;
    CardType type = CardDef::get(card.id).type;
    if (type == CardType::SKILL && has_power(PowerId::CORRUPTION)) return true;
    if (type == CardType::ATTACK && has_power(PowerId::FREE_ATTACK)) return true;
    if (type == CardType::SKILL && has_power(PowerId::FREE_SKILL)) return true;
    if (type == CardType::POWER && has_power(PowerId::FREE_POWER)) return true;
    if (CardDef::has_keyword(card, KEYWORD_ETHEREAL) && has_power(PowerId::FREE_ETHEREAL)) return true;
    return power_counter(PowerId::VOID_FORM) < power_amount(PowerId::VOID_FORM);
}

int CombatEngine::effective_cost(const DeckCard& card) const {
    uint8_t printed = CardDef::get_cost(card.id, card.upgraded);
    if (printed == COST_X) return COST_X;
    if (costs_nothing(card)) return 0;

    int cost = printed + card.cost_delta - card.bookmark_discount;
    switch (card.id) {
        case CardId::MIDNIGHT:
            cost -= static_cast<int>(exhaust_pile.size());
            break;
        case CardId::STOMP:
            cost -= counters.attacks_played;
            break;
        case CardId::PINPOINT:
            cost -= counters.skills_played;
            break;
        case CardId::BANSHEES_CRY:
            cost -= 2 * counters.combat_ethereal_played;
            break;
        case CardId::FLATTEN:
            if (counters.osty_attacks > 0) cost = 0;
            break;
        default:
            break;
    }
    cost += power_amount(PowerId::SURCHARGE);
    if (CardDef::get(card.id).type == CardType::POWER) cost += power_amount(PowerId::SPIKED_GAUNTLETS);
    if (CardDef::get(card.id).type == CardType::ATTACK) cost += power_amount(PowerId::TANGLED);
    if (card.cost_one_this_turn) cost = std::min(cost, 1);
    return std::clamp(cost, 0, 99);
}

int CombatEngine::consume_play_modifiers(const DeckCard& card) {
    CardType type = CardDef::get(card.id).type;
    auto use_one = [this](PowerId id) {
        for (size_t i = 0; i < powers.size(); ++i) {
            if (powers[i].id == id) {
                if (--powers[i].amount <= 0) powers.erase(i);
                return true;
            }
        }
        return false;
    };

    // "The next ... costs 0": used by the card whether it needed it or not.
    if (type == CardType::ATTACK) use_one(PowerId::FREE_ATTACK);
    if (type == CardType::SKILL) use_one(PowerId::FREE_SKILL);
    if (type == CardType::POWER) use_one(PowerId::FREE_POWER);
    if (CardDef::has_keyword(card, KEYWORD_ETHEREAL)) use_one(PowerId::FREE_ETHEREAL);

    int extra = 0;
    if (use_one(PowerId::DUPLICATION)) ++extra;
    if (use_one(PowerId::THROWING_AXE)) ++extra; // Throwing Axe: the first card of the combat
    if (type == CardType::ATTACK && use_one(PowerId::REPLAY_ATTACK)) ++extra;
    if (type == CardType::SKILL && use_one(PowerId::REPLAY_SKILL)) ++extra;
    if (type == CardType::POWER && use_one(PowerId::REPLAY_POWER)) ++extra;
    // Sword Sage: Replay on the Sovereign Blade, every time it is played.
    if (card.id == CardId::SOVEREIGN_BLADE) extra += power_amount(PowerId::SWORD_SAGE);
    // Echo N on this copy of the card.
    extra += card.echo;
    ActivePower* echo = find_power(PowerId::ECHO_FORM);
    if (echo && echo->counter < echo->amount) {
        ++echo->counter;
        ++extra;
    }
    return extra;
}

bool CombatEngine::play_card(int index, int target_index) {
    if (!can_play_card(index)) return false;

    DeckCard played_card = hand[index];
    int cost = effective_cost(played_card);

    // Pay energy. An X card spends everything left: `x_value` is what it spent, or what is there when it is free.
    int x_value = 0;
    const bool x_card = cost == COST_X || CardDef::get_star_cost(played_card.id, played_card.upgraded) == COST_X;
    if (cost == COST_X) {
        x_value = std::max<int>(player.energy, 0);
        cost = costs_nothing(played_card) ? 0 : x_value;
    }
    player.energy = static_cast<int8_t>(player.energy - cost);
    // Stars are paid the same way; an X card in Stars spends them all.
    uint8_t star_cost = CardDef::get_star_cost(played_card.id, played_card.upgraded);
    if (star_cost == COST_X) {
        x_value = player.sovereign_stars;
        star_cost = player.sovereign_stars;
    }
    spend_stars(star_cost);
    // Chemical X: the effect of an X card is raised, the energy it spends is not.
    if (x_card) x_value += power_amount(PowerId::CHEMICAL_X);

    hand.erase(index);
    // Void Form counts the cards played from the hand, not their replays.
    if (ActivePower* void_form = find_power(PowerId::VOID_FORM)) void_form->counter = bump(void_form->counter);

    AfterPlay after_play = AfterPlay::DISCARD;
    run_card(played_card, target_index, false, x_value, static_cast<uint8_t>(cost), after_play);
    place_played_card(played_card, after_play);
    finish_play();
    return true;
}

void CombatEngine::autoplay(DeckCard card, int target_index, AfterPlay destination) {
    // Cards that play cards that play cards: the chain is cut before it gets out of hand.
    if (autoplay_depth_ >= 4 || state != CombatState::PLAYER_TURN) {
        if (destination != AfterPlay::NOWHERE) discard_pile.push_back(card);
        return;
    }
    ++autoplay_depth_;
    AfterPlay after_play = destination;
    // An X card played for free takes the energy that is there without spending it.
    int x_value = CardDef::is_x_cost(card.id, card.upgraded) ? std::max<int>(player.energy, 0) : 0;
    const bool star_x = CardDef::get_star_cost(card.id, card.upgraded) == COST_X;
    if (star_x) x_value = player.sovereign_stars;
    if (CardDef::is_x_cost(card.id, card.upgraded) || star_x) x_value += power_amount(PowerId::CHEMICAL_X);
    run_card(card, target_index, target_index < 0, x_value, 0, after_play);
    // The destination asked by whoever plays the card wins over the card's own (Havoc exhausts).
    if (destination != AfterPlay::DISCARD) after_play = destination;
    place_played_card(card, after_play);
    --autoplay_depth_;
}

bool CombatEngine::opens_choice(const DeckCard& card) {
    switch (card.id) {
        case CardId::SURVIVOR:
        case CardId::HAND_TRICK:
        case CardId::SEANCE:
        case CardId::TRANSFIGURE:
        case CardId::BEGONE:
        case CardId::CHARGE:
        case CardId::GUARDS:
        case CardId::NEOWS_FURY:
        case CardId::THINKING_AHEAD:
        case CardId::PURITY:
        case CardId::SECRET_TECHNIQUE:
        case CardId::SECRET_WEAPON:
        case CardId::SEEKER_STRIKE:
        case CardId::SPLASH:
        case CardId::DISCOVERY:
        case CardId::DUAL_WIELD:
        case CardId::HEIRLOOM_HAMMER:
        case CardId::NIGHTMARE:
        case CardId::QUASAR:
        case CardId::DECISIONS_DECISIONS:
        case CardId::BRAND:
        case CardId::BURNING_PACT:
        case CardId::ACROBATICS:
        case CardId::DAGGER_THROW:
        case CardId::PREPARED:
        case CardId::HIDDEN_DAGGERS:
        case CardId::SCAVENGE:
        case CardId::PHOTON_CUT:
        case CardId::GLIMMER:
        case CardId::SCULPTING_STRIKE:
        case CardId::HEADBUTT:
        case CardId::WISH:
        case CardId::ABUNDANCE:
        case CardId::HOLOGRAM:
        case CardId::COSMIC_INDIFFERENCE:
        case CardId::GRAVEBLAST:
        case CardId::DREDGE:
        case CardId::CLEANSE:
        case CardId::SNAP:
            return true;
        // True Grit picks at random until it is upgraded; Armaments upgrades the whole hand once it is.
        case CardId::TRUE_GRIT:
            return card.upgraded;
        case CardId::ARMAMENTS:
            return !card.upgraded;
        default:
            return false;
    }
}

int CombatEngine::vakuu_next_card() {
    if (!vakuu_turn_active || state != CombatState::PLAYER_TURN) return -1;
    while (choice_pending()) resolve_choice_default();
    if (state != CombatState::PLAYER_TURN || vakuu_plays_ >= VAKUU_MAX_PLAYS) return -1;

    for (size_t i = 0; i < hand.size(); ++i) {
        const DeckCard& card = hand[i];
        if (opens_choice(card) || !can_play_card(static_cast<int>(i))) continue;
        // An X card with nothing to spend would be played for nothing.
        if (CardDef::is_x_cost(card.id, card.upgraded) && player.energy <= 0) continue;
        if (CardDef::get_star_cost(card.id, card.upgraded) == COST_X && player.sovereign_stars == 0) continue;
        return static_cast<int>(i);
    }
    return -1;
}

bool CombatEngine::vakuu_step() {
    const int index = vakuu_next_card();
    if (index < 0) return false;
    ++vakuu_plays_;
    return play_card(index, get_first_alive_enemy_index());
}

int CombatEngine::vakuu_play_turn() {
    if (!vakuu_turn_active) return 0;
    int played = 0;
    while (vakuu_step()) ++played;
    end_player_turn();
    return played;
}

void CombatEngine::run_card(DeckCard& card, int target_index, bool random_target, int x_value, uint8_t energy_paid,
                            AfterPlay& after_play) {
    const CardType played_type = CardDef::get(card.id).type;
    const int plays = 1 + consume_play_modifiers(card);
    // Rebound: the card played right after it goes back on top of the draw pile.
    const bool rebounded = rebound_pending;
    rebound_pending = false;
    // Nostalgia: the first Attack or Skill of the turn goes back on top of the draw pile too.
    bool nostalgic = false;
    ActivePower* nostalgia = find_power(PowerId::NOSTALGIA);
    if (nostalgia && nostalgia->counter == 0 && (played_type == CardType::ATTACK || played_type == CardType::SKILL)) {
        nostalgia->counter = 1;
        nostalgic = true;
    }

    for (int play = 0; play < plays; ++play) {
        fatal_kills = 0;
        const uint8_t paid = play == 0 ? energy_paid : 0;

        // Powers react before the card resolves, so a power gained from this card does not hear it.
        EventInfo played;
        played.card = card;
        played.cost = paid;
        emit(CombatEvent::CARD_PLAYED, played);

        // Resolve target: a reaction may have killed the one that was aimed at
        int target_idx = random_target ? random_alive_enemy_index() : target_index;
        if (target_idx < 0 || target_idx >= static_cast<int>(enemies.size()) || !enemies[target_idx].is_alive()) {
            target_idx = selected_target_index;
            if (target_idx < 0 || target_idx >= static_cast<int>(enemies.size()) || !enemies[target_idx].is_alive()) {
                target_idx = get_first_alive_enemy_index();
            }
        }

        // The counters are updated once the effect is resolved: inside the effect they still describe what
        // happened before this card ("already played this turn", "spent on other cards").
        resolving_card = true;
        resolve_effect(card, target_idx, x_value, after_play);
        resolving_card = false;
        lamp_doubling = false;
        pen_nib_now = false;
        strike_played_now = false;
        minion_played_now = false;
        upgraded_attack_now = false;
        // Vigor goes to the first Attack played, all its hits included.
        if (played_type == CardType::ATTACK) {
            player.clear_status(StatusType::VIGOR);
            triple_next_attack = false;
        }

        // Keep primary enemy in sync
        if (!enemies.empty()) {
            enemy = enemies[0];
        }

        // Music Box: the first Attack of the turn leaves an Ethereal copy in the hand.
        if (played_type == CardType::ATTACK && play == 0 && counters.attacks_played == 0 && has_power(PowerId::MUSIC_BOX)) {
            DeckCard copy = card;
            copy.added_keywords |= KEYWORD_ETHEREAL;
            create_card(copy, Pile::HAND);
        }

        // Tallies of the card just played
        if (played_type == CardType::ATTACK) {
            counters.attacks_played = bump(counters.attacks_played);
        } else if (played_type == CardType::SKILL) {
            counters.skills_played = bump(counters.skills_played);
        } else if (played_type == CardType::POWER) {
            counters.powers_played = bump(counters.powers_played);
        } else {
            counters.statuses_played = bump(counters.statuses_played);
        }
        if (card.id == CardId::SHIV) counters.shivs_played = bump(counters.shivs_played);
        counters.energy_spent = bump(counters.energy_spent, paid);
        counters.combat_cards_played = bump(counters.combat_cards_played);
        if (CardDef::has_keyword(card, KEYWORD_ETHEREAL)) {
            counters.combat_ethereal_played = bump(counters.combat_ethereal_played);
        }
        if (played_type == CardType::SKILL && counters.skills_played % CardDef::get_secondary(CardId::MAKE_IT_SO, false) == 0) {
            recall_to_hand(CardId::MAKE_IT_SO);
        }
        // Right Hand Hand comes back from the discard pile when a card that cost 2 energy or more is played.
        if (paid >= CardDef::get_secondary(CardId::RIGHT_HAND_HAND, false)) {
            for (size_t i = discard_pile.size(); i-- > 0 && hand.size() < hand.capacity();) {
                if (discard_pile[i].id == CardId::RIGHT_HAND_HAND) {
                    hand.push_back(discard_pile[i]);
                    discard_pile.erase(i);
                }
            }
        }
        if (!player.is_alive() || !has_alive_enemies()) break;
    }

    if (played_type == CardType::POWER) {
        powers_in_play.push_back(card);
    } else if (played_type == CardType::ATTACK && CardDef::get_cost(card.id, card.upgraded) == 0 && after_play == AfterPlay::DISCARD) {
        ActivePower* feral = find_power(PowerId::FERAL);
        if (feral && feral->counter < feral->amount) {
            ++feral->counter;
            after_play = AfterPlay::HAND;
        }
    }
    if ((rebounded || nostalgic) && after_play == AfterPlay::DISCARD) after_play = AfterPlay::DRAW_TOP;
}

void CombatEngine::place_played_card(const DeckCard& card, AfterPlay after_play) {
    const CardType played_type = CardDef::get(card.id).type;
    // Powers stay in play for the rest of the combat: they never return to the piles.
    if (played_type == CardType::POWER || after_play == AfterPlay::NOWHERE) return;

    // Where it goes is decided by the card as it was played; Razor Tooth upgrades the copy that comes back.
    DeckCard kept_card = card;
    kept_card.free_this_turn = false;
    kept_card.bookmark_discount = 0;
    kept_card.cost_one_this_turn = false;
    kept_card.sly_this_turn = false;
    if (played_type == CardType::SKILL && has_power(PowerId::MASTER_PLANNER)) kept_card.sly_combat = true;
    if (has_power(PowerId::RAZOR_TOOTH)) kept_card.upgraded = true;
    const bool corrupted = played_type == CardType::SKILL && has_power(PowerId::CORRUPTION);
    if (CardDef::has_keyword(card, KEYWORD_EXHAUST) || after_play == AfterPlay::EXHAUST || corrupted) {
        exhaust_card(kept_card);
    } else if (CardDef::has_keyword(card, KEYWORD_RETURN)) {
        if (!returning_cards.push_back(kept_card)) discard_pile.push_back(kept_card);
    } else if (after_play == AfterPlay::HAND && hand.push_back(kept_card)) {
        // back in hand
    } else if (after_play == AfterPlay::DRAW_TOP) {
        draw_pile.push_back(kept_card);
    } else {
        discard_pile.push_back(kept_card);
    }
}

void CombatEngine::finish_play() {
    play_sly_pending();
    // Deaths not caused by a hit (orbs, HP loss) are noticed here.
    settle_deaths();
    if (hand.empty() && has_power(PowerId::UNCEASING_TOP) && player.is_alive()) {
        draw_card();
    }
    if (!enemies.empty()) {
        enemy = enemies[0];
    }

    // A card that costs HP can kill its player.
    if (!player.is_alive()) {
        state = CombatState::DEFEAT;
        set_log("You were defeated!");
        return;
    }

    // Check victory
    if (!has_alive_enemies()) {
        state = CombatState::VICTORY;
        set_log("Victory!");
    } else {
        selected_target_index = get_first_alive_enemy_index();
    }
}

bool CombatEngine::can_be_picked(const DeckCard& card, ChoiceKind kind) const {
    switch (kind) {
        case ChoiceKind::UPGRADE:
            return !card.upgraded && CardDef::get(card.id).type != CardType::STATUS;
        case ChoiceKind::MAKE_ETHEREAL:
            return !CardDef::has_keyword(card, KEYWORD_ETHEREAL);
        case ChoiceKind::MAKE_RETAIN:
            return !CardDef::has_keyword(card, KEYWORD_RETAIN);
        case ChoiceKind::MAKE_SLY:
            return !CardDef::is_sly(card);
        case ChoiceKind::TRANSFORM:
            return true;
        case ChoiceKind::ADD_ECHO:
            return card.echo == 0;
        case ChoiceKind::MAKE_FREE: {
            const int cost = effective_cost(card);
            return cost != COST_X && cost > 0;
        }
        case ChoiceKind::COPY_ATTACK_OR_POWER:
            return CardDef::get(card.id).type == CardType::ATTACK || CardDef::get(card.id).type == CardType::POWER;
        case ChoiceKind::COPY_COLORLESS:
            return card_meta(card.id).pool == CardPool::COLORLESS;
        default:
            return true;
    }
}

bool CombatEngine::can_be_picked(const DeckCard& card, const CardChoice& choice) const {
    if (!choice.any_type && CardDef::get(card.id).type != choice.type) return false;
    return can_be_picked(card, choice.kind);
}

size_t CombatEngine::pile_size(Pile pile) const {
    switch (pile) {
        case Pile::HAND: return hand.size();
        case Pile::DRAW: return draw_pile.size();
        case Pile::OFFERS: return offers.size();
        default: return discard_pile.size();
    }
}

const DeckCard& CombatEngine::pile_card(Pile pile, size_t index) const {
    switch (pile) {
        case Pile::HAND: return hand[index];
        case Pile::DRAW: return draw_pile[index];
        case Pile::OFFERS: return offers[index];
        default: return discard_pile[index];
    }
}

size_t CombatEngine::choice_pool(const CardChoice& choice) const {
    return std::min<size_t>(pile_size(choice.source), choice.pool);
}

bool CombatEngine::choice_allows(size_t index) const {
    if (!choice_pending()) return false;
    const CardChoice& choice = pending_choices[0];
    return index < choice_pool(choice) && can_be_picked(pile_card(choice.source, index), choice);
}

int CombatEngine::next_pickable_choice(std::span<const uint8_t> order, int from, int step, bool wrap) const {
    const int count = order.empty() ? choice_pool_size() : static_cast<int>(order.size());
    if (count <= 0 || (step != 1 && step != -1)) return -1;
    for (int tried = 0, position = from; tried < count; ++tried, position += step) {
        if (position < 0 || position >= count) {
            if (!wrap) return -1;
            position = (position + count) % count;
        }
        const size_t index = order.empty() ? static_cast<size_t>(position) : order[static_cast<size_t>(position)];
        if (choice_allows(index)) return position;
    }
    return -1;
}

int CombatEngine::choice_pool_size() const {
    return choice_pending() ? static_cast<int>(choice_pool(pending_choices[0])) : 0;
}

int CombatEngine::choice_size() const {
    if (!choice_pending()) return 0;
    int pickable = 0;
    for (size_t i = 0, pool = choice_pool(pending_choices[0]); i < pool; ++i) pickable += choice_allows(i);
    return std::min<int>(pending_choices[0].count, pickable);
}

void CombatEngine::ask_choice(const CardChoice& choice) {
    CardChoice asked = choice;
    // The card that asks is not in the discard pile yet: what is there now is what can be picked.
    if (asked.source == Pile::DISCARD) asked.pool = static_cast<uint8_t>(std::min<size_t>(discard_pile.size(), WHOLE_PILE));
    if (!pending_choices.push_back(asked)) return; // more choices than any card combination asks for
    settle_choices();
}

void CombatEngine::transform_card(DeckCard& card, CardId into, bool upgraded) {
    if (into == CardId::NONE) into = random_card(CardFilter());
    if (into == CardId::NONE) return;
    // A new card: what the old one gained goes with it, and it no longer stands for a card of the deck.
    card = DeckCard(into, upgraded);
}

void CombatEngine::ask_pile_choice(Pile source, ChoiceKind kind, uint8_t count) {
    CardChoice choice;
    choice.kind = kind;
    choice.count = count;
    choice.source = source;
    ask_choice(choice);
}

void CombatEngine::offer_from_draw(uint8_t count) {
    CardChoice choice;
    choice.kind = ChoiceKind::TO_HAND;
    choice.source = Pile::OFFERS;
    choice.amount = count;
    ask_choice(choice);
}

void CombatEngine::offer_random_cards(const CardFilter& filter, uint8_t count, bool upgraded, bool free_this_turn, bool retain) {
    CardChoice choice;
    choice.kind = ChoiceKind::TO_HAND;
    choice.source = Pile::OFFERS;
    choice.amount = count;
    choice.upgraded = upgraded;
    choice.offer_from_draw = false;
    choice.offer_free = free_this_turn;
    choice.offer_retain = retain;
    choice.offer_filter = filter;
    ask_choice(choice);
}

void CombatEngine::clear_offers() {
    offers.clear();
    offer_origins.clear();
}

void CombatEngine::deal_offers(const CardChoice& choice) {
    clear_offers();
    const size_t wanted = std::min<size_t>(choice.amount, offers.capacity());
    if (choice.offer_from_draw) {
        // Different cards of the draw pile, at random: its order stays hidden.
        FixedVector<uint8_t, 50> left;
        for (size_t i = 0; i < draw_pile.size(); ++i) left.push_back(static_cast<uint8_t>(i));
        while (offers.size() < wanted && !left.empty()) {
            const size_t pick = next_random() % left.size();
            offers.push_back(draw_pile[left[pick]]);
            offer_origins.push_back(left[pick]);
            left.erase(pick);
        }
        return;
    }
    // Different cards: a card drawn twice is drawn again, a few times at most when the filter has few cards.
    for (size_t tries = 0; offers.size() < wanted && tries < wanted * 8; ++tries) {
        const CardId id = random_card(choice.offer_filter);
        if (id == CardId::NONE) break;
        bool offered = false;
        for (const DeckCard& card : offers) offered = offered || card.id == id;
        if (offered) continue;
        DeckCard card(id, choice.upgraded);
        card.free_this_turn = choice.offer_free;
        if (choice.offer_retain) card.added_keywords |= KEYWORD_RETAIN;
        offers.push_back(card);
        offer_origins.push_back(NOT_IN_PILE);
    }
}

void CombatEngine::settle_choices() {
    while (choice_pending()) {
        if (pending_choices[0].source == Pile::OFFERS && offers.empty()) deal_offers(pending_choices[0]);
        const CardChoice choice = pending_choices[0];
        FixedVector<uint8_t, 50> pickable;
        for (size_t i = 0, pool = choice_pool(choice); i < pool; ++i) {
            if (can_be_picked(pile_card(choice.source, i), choice)) pickable.push_back(static_cast<uint8_t>(i));
        }
        // Something is left to decide when a card can be left out, or when picking is optional.
        if (!pickable.empty() && (choice.optional || pickable.size() > choice.count)) return;
        pending_choices.erase(0);
        apply_choice(choice, std::span<const uint8_t>(pickable.begin(), choice.optional ? 0 : pickable.size()));
    }
}

void CombatEngine::apply_choice(const CardChoice& choice, std::span<const uint8_t> indices) {
    // From the last card of the pile to the first: taking a card out does not move the ones still to take.
    FixedVector<uint8_t, 50> picks;
    for (uint8_t index : indices) picks.push_back(index);
    std::sort(picks.begin(), picks.end(), [](uint8_t a, uint8_t b) { return a > b; });

    uint8_t taken_from_discard = 0;
    for (uint8_t index : picks) {
        if (index >= pile_size(choice.source)) continue;
        if (choice.source == Pile::OFFERS) {
            const uint8_t origin = offer_origins[index];
            if (origin == NOT_IN_PILE) {
                create_card(offers[index], Pile::HAND); // to the discard pile when the hand is full
            } else if (origin < draw_pile.size() && hand.size() < hand.capacity()) {
                // A card of the draw pile stays there when the hand is full.
                hand.push_back(draw_pile[origin]);
                draw_pile.erase(origin);
                for (uint8_t& other : offer_origins) {
                    if (other != NOT_IN_PILE && other > origin) --other;
                }
            }
            continue;
        }
        if (choice.source == Pile::HAND) {
            switch (choice.kind) {
                case ChoiceKind::DISCARD:
                    discard_from_hand(index);
                    break;
                case ChoiceKind::EXHAUST:
                    exhaust_from_hand(index);
                    break;
                case ChoiceKind::TOP_OF_DRAW: {
                    DeckCard card = hand[index];
                    hand.erase(index);
                    draw_pile.push_back(card);
                    break;
                }
                case ChoiceKind::UPGRADE:
                    hand[index].upgraded = true;
                    break;
                case ChoiceKind::MAKE_ETHEREAL:
                    hand[index].added_keywords |= KEYWORD_ETHEREAL;
                    break;
                case ChoiceKind::MAKE_RETAIN:
                    hand[index].added_keywords |= KEYWORD_RETAIN;
                    break;
                case ChoiceKind::MAKE_SLY:
                    hand[index].sly_this_turn = true;
                    break;
                case ChoiceKind::ADD_ECHO:
                    hand[index].echo = 1;
                    hand[index].cost_delta = static_cast<int8_t>(hand[index].cost_delta + 1);
                    break;
                case ChoiceKind::TRANSFORM:
                    transform_card(hand[index], choice.into, choice.upgraded);
                    break;
                case ChoiceKind::MAKE_FREE: {
                    const int cost = effective_cost(hand[index]);
                    if (cost != COST_X) hand[index].cost_delta = static_cast<int8_t>(hand[index].cost_delta - cost);
                    break;
                }
                case ChoiceKind::COPY_ATTACK_OR_POWER:
                case ChoiceKind::COPY_COLORLESS: {
                    DeckCard copy = hand[index];
                    copy.free_this_turn = false;
                    copy.cost_one_this_turn = false;
                    copy.deck_index = NOT_IN_DECK; // a gain on the copy does not reach the deck
                    for (uint8_t made = 0; made < choice.amount; ++made) create_card(copy, Pile::HAND);
                    break;
                }
                case ChoiceKind::COPY_NEXT_TURN:
                    schedule(DelayedType::COPIES_TO_HAND,
                             static_cast<int16_t>(static_cast<int>(hand[index].id) * 2 + (hand[index].upgraded ? 1 : 0)));
                    break;
                case ChoiceKind::PLAY_REPEATED: {
                    DeckCard card = hand[index];
                    hand.erase(index);
                    // Every play but the last leaves the card out of the piles; the last one sends it where it goes.
                    bool placed = false;
                    for (uint8_t play = 0; play < choice.amount && has_alive_enemies(); ++play) {
                        placed = play + 1 == choice.amount;
                        autoplay(card, -1, placed ? AfterPlay::DISCARD : AfterPlay::NOWHERE);
                    }
                    if (!placed) discard_pile.push_back(card);
                    break;
                }
                case ChoiceKind::TO_HAND:
                    break;
            }
            continue;
        }

        FixedVector<DeckCard, 50>& pile = choice.source == Pile::DRAW ? draw_pile : discard_pile;
        if (choice.kind == ChoiceKind::UPGRADE) {
            pile[index].upgraded = true;
            continue;
        }
        if (choice.kind == ChoiceKind::TRANSFORM) {
            transform_card(pile[index], choice.into, choice.upgraded);
            continue;
        }
        if (choice.kind == ChoiceKind::MAKE_ETHEREAL) {
            pile[index].added_keywords |= KEYWORD_ETHEREAL;
            continue;
        }
        if (choice.kind == ChoiceKind::MAKE_RETAIN) {
            pile[index].added_keywords |= KEYWORD_RETAIN;
            continue;
        }
        if (choice.kind == ChoiceKind::TO_HAND && hand.size() >= hand.capacity()) continue;

        DeckCard card = pile[index];
        pile.erase(index);
        if (choice.source == Pile::DISCARD) ++taken_from_discard;
        switch (choice.kind) {
            case ChoiceKind::TO_HAND:
                if (choice.free_this_turn) card.free_this_turn = true;
                hand.push_back(card);
                break;
            case ChoiceKind::TOP_OF_DRAW:
                draw_pile.push_back(card);
                break;
            case ChoiceKind::EXHAUST:
                exhaust_card(card);
                break;
            default:
                discard_pile.push_back(card);
                break;
        }
    }
    // The offers not picked are left out: the next offer deals its own.
    if (choice.source == Pile::OFFERS) clear_offers();
    // The choices still waiting on the discard pile are about the same cards, fewer now.
    for (CardChoice& waiting : pending_choices) {
        if (waiting.source == Pile::DISCARD && waiting.pool != WHOLE_PILE) {
            waiting.pool = static_cast<uint8_t>(std::max(0, waiting.pool - taken_from_discard));
        }
    }

    switch (choice.then) {
        case ChoiceThen::DRAW:
            draw_cards(choice.amount);
            break;
        case ChoiceThen::DRAW_PER_CARD:
            draw_cards(static_cast<int>(picks.size()));
            break;
        case ChoiceThen::SHIVS:
            add_shivs(choice.amount, choice.upgraded);
            break;
        case ChoiceThen::NONE:
            break;
    }
}

bool CombatEngine::resolve_choice(std::span<const uint8_t> indices) {
    if (!choice_pending()) return false;
    const CardChoice choice = pending_choices[0];
    const size_t wanted = static_cast<size_t>(choice_size());
    if (indices.size() > wanted || (!choice.optional && indices.size() != wanted)) return false;
    for (size_t i = 0; i < indices.size(); ++i) {
        if (!choice_allows(indices[i])) return false;
        for (size_t j = 0; j < i; ++j) {
            if (indices[j] == indices[i]) return false;
        }
    }

    pending_choices.erase(0);
    apply_choice(choice, indices);
    settle_choices();
    finish_play();
    return true;
}

void CombatEngine::resolve_choice_default() {
    if (!choice_pending()) return;
    FixedVector<uint8_t, 50> picks;
    if (!pending_choices[0].optional) {
        const size_t wanted = static_cast<size_t>(choice_size());
        for (size_t i = 0, pool = choice_pool(pending_choices[0]); i < pool && picks.size() < wanted; ++i) {
            if (choice_allows(i)) picks.push_back(static_cast<uint8_t>(i));
        }
    }
    if (!resolve_choice(std::span<const uint8_t>(picks.begin(), picks.size()))) {
        if (pending_choices[0].source == Pile::OFFERS) clear_offers();
        pending_choices.erase(0);
    }
}

void CombatEngine::boost_copies(CardId id, int amount) {
    auto boost = [id, amount](DeckCard& card) {
        if (card.id == id) card.bonus = bump(card.bonus, amount);
    };
    for (DeckCard& card : hand) boost(card);
    for (DeckCard& card : draw_pile) boost(card);
    for (DeckCard& card : discard_pile) boost(card);
    for (DeckCard& card : exhaust_pile) boost(card);
}

void CombatEngine::change_cost_of_copies(CardId id, int delta) {
    auto change = [id, delta](DeckCard& card) {
        if (card.id == id) card.cost_delta = static_cast<int8_t>(std::clamp(card.cost_delta + delta, -100, 100));
    };
    for (DeckCard& card : hand) change(card);
    for (DeckCard& card : draw_pile) change(card);
    for (DeckCard& card : discard_pile) change(card);
}

void CombatEngine::resolve_effect(DeckCard& played_card, int target_idx, int x_value, AfterPlay& after_play) {
    const CardId card_id = played_card.id;
    const bool upgraded = played_card.upgraded;
    const uint8_t base_value = CardDef::get_value(card_id, upgraded);
    const uint8_t secondary_value = CardDef::get_secondary(card_id, upgraded);
    Enemy& current_target = (target_idx >= 0 && target_idx < static_cast<int>(enemies.size())) ? enemies[target_idx] : enemy;
    (void)x_value;
    (void)after_play;

    // Apply effects
    switch (card_id) {
        case CardId::STRIKE: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Strike+!"sv : "Played Strike!"sv);
            break;
        }
        case CardId::DEFEND: {
            gain_block(static_cast<int16_t>(base_value + power_amount(PowerId::FASTEN)));
            set_log(upgraded ? "Played Defend+!"sv : "Played Defend!"sv);
            break;
        }
        case CardId::BASH: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Bash+!"sv : "Played Bash!"sv);
            break;
        }
        case CardId::IRON_WAVE: {
            gain_block(base_value);
            hit(current_target, secondary_value);
            set_log(upgraded ? "Played Iron Wave+!"sv : "Played Iron Wave!"sv);
            break;
        }
        case CardId::CLEAVE: {
            // AoE: attacks ALL alive enemies!
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                }
            }
            set_log(upgraded ? "Played Cleave+!"sv : "Played Cleave!"sv);
            break;
        }
        case CardId::ANGER: {
            hit(current_target, base_value);
            create_card_in_discard(DeckCard(CardId::ANGER, upgraded)); // Extra copy!
            set_log(upgraded ? "Played Anger+!"sv : "Played Anger!"sv);
            break;
        }
        case CardId::METALLICIZE: {
            player.add_status(StatusType::METALLICIZE, base_value);
            set_log(upgraded ? "Played Metallicize+!"sv : "Played Metallicize!"sv);
            break;
        }
        case CardId::NEUTRALIZE: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::WEAK, secondary_value);
            set_log(upgraded ? "Played Neutralize+!"sv : "Played Neutralize!"sv);
            break;
        }
        case CardId::SURVIVOR: {
            gain_block(base_value);
            ask_choice(CardChoice{ ChoiceKind::DISCARD, 1 });
            set_log(upgraded ? "Played Survivor+!"sv : "Played Survivor!"sv);
            break;
        }
        case CardId::POISON_STAB: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::POISON, secondary_value);
            set_log(upgraded ? "Played Poison Stab+!"sv : "Played Poison Stab!"sv);
            break;
        }
        case CardId::DEADLY_POISON: {
            afflict(current_target, StatusType::POISON, secondary_value);
            set_log(upgraded ? "Played Deadly Poison+!"sv : "Played Deadly Poison!"sv);
            break;
        }
        case CardId::ZAP: {
            channel_orb(OrbType::LIGHTNING);
            set_log(upgraded ? "Channeled Lightning (Zap+)!"sv : "Channeled Lightning (Zap)!"sv);
            break;
        }
        case CardId::DUALCAST: {
            evoke_orb(0, 2);
            set_log(upgraded ? "Triggered Dualcast+!"sv : "Triggered Dualcast!"sv);
            break;
        }
        case CardId::FALLING_STAR: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::WEAK, secondary_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Cast Falling Star+!"sv : "Cast Falling Star!"sv);
            break;
        }
        case CardId::VENERATE: {
            gain_stars(secondary_value);
            set_log(upgraded ? "Cast Venerate+!"sv : "Cast Venerate!"sv);
            break;
        }
        case CardId::UNLEASH: {
            // Osty deals the card's damage plus his current HP.
            osty_attack(current_target, static_cast<int16_t>(base_value + player.minion_hp));
            set_log(upgraded ? "Cast Unleash+!"sv : "Cast Unleash!"sv);
            break;
        }
        case CardId::BODYGUARD: {
            player.summon_minion(secondary_value, 0);
            set_log(upgraded ? "Summoned Osty (Bodyguard+)!"sv : "Summoned Osty (Bodyguard)!"sv);
            break;
        }
        case CardId::TWIN_STRIKE: {
            for (uint8_t blow = 0; blow < secondary_value; ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Twin Strike+!"sv : "Played Twin Strike!"sv);
            break;
        }
        case CardId::POMMEL_STRIKE: {
            hit(current_target, base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Pommel Strike+!"sv : "Played Pommel Strike!"sv);
            break;
        }
        case CardId::SHRUG_IT_OFF: {
            gain_block(base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Shrug It Off+!"sv : "Played Shrug It Off!"sv);
            break;
        }
        case CardId::TAUNT: {
            gain_block(base_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Taunt+!"sv : "Played Taunt!"sv);
            break;
        }
        case CardId::SLICE: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Slice+!"sv : "Played Slice!"sv);
            break;
        }
        case CardId::BACKFLIP: {
            gain_block(base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Backflip+!"sv : "Played Backflip!"sv);
            break;
        }
        case CardId::SUCKER_PUNCH: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::WEAK, secondary_value);
            set_log(upgraded ? "Played Sucker Punch+!"sv : "Played Sucker Punch!"sv);
            break;
        }
        case CardId::DAGGER_SPRAY: {
            for (uint8_t blow = 0; blow < secondary_value; ++blow) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        hit(enemies[i], base_value);
                    }
                }
            }
            set_log(upgraded ? "Played Dagger Spray+!"sv : "Played Dagger Spray!"sv);
            break;
        }
        case CardId::BEAM_CELL: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Beam Cell+!"sv : "Played Beam Cell!"sv);
            break;
        }
        case CardId::BALL_LIGHTNING: {
            hit(current_target, base_value);
            channel_orb(OrbType::LIGHTNING);
            set_log(upgraded ? "Played Ball Lightning+!"sv : "Played Ball Lightning!"sv);
            break;
        }
        case CardId::COLD_SNAP: {
            hit(current_target, base_value);
            channel_orb(OrbType::FROST);
            set_log(upgraded ? "Played Cold Snap+!"sv : "Played Cold Snap!"sv);
            break;
        }
        case CardId::LEAP: {
            gain_block(base_value);
            set_log(upgraded ? "Played Leap+!"sv : "Played Leap!"sv);
            break;
        }
        case CardId::SOLAR_STRIKE: {
            hit(current_target, base_value);
            gain_stars(secondary_value);
            set_log(upgraded ? "Played Solar Strike+!"sv : "Played Solar Strike!"sv);
            break;
        }
        case CardId::GATHER_LIGHT: {
            gain_block(base_value);
            gain_stars(secondary_value);
            set_log(upgraded ? "Played Gather Light+!"sv : "Played Gather Light!"sv);
            break;
        }
        case CardId::CELESTIAL_MIGHT: {
            for (uint8_t blow = 0; blow < secondary_value; ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Celestial Might+!"sv : "Played Celestial Might!"sv);
            break;
        }
        case CardId::PULL_AGGRO: {
            player.summon_minion(secondary_value, 0);
            gain_block(base_value);
            set_log(upgraded ? "Played Pull Aggro+!"sv : "Played Pull Aggro!"sv);
            break;
        }
        case CardId::DRAIN_POWER: {
            hit(current_target, base_value);
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
            hit(current_target, player.block);
            set_log(upgraded ? "Played Body Slam+!"sv : "Played Body Slam!"sv);
            break;
        }
        case CardId::DEFLECT: {
            gain_block(base_value);
            set_log(upgraded ? "Played Deflect+!"sv : "Played Deflect!"sv);
            break;
        }
        case CardId::COOLHEADED: {
            channel_orb(OrbType::FROST);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Coolheaded+!"sv : "Played Coolheaded!"sv);
            break;
        }
        case CardId::SWEEPING_BEAM: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                }
            }
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Sweeping Beam+!"sv : "Played Sweeping Beam!"sv);
            break;
        }
        case CardId::THUNDERCLAP: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                    afflict(enemies[i], StatusType::VULNERABLE, secondary_value);
                }
            }
            set_log(upgraded ? "Played Thunderclap+!"sv : "Played Thunderclap!"sv);
            break;
        }
        case CardId::DISMANTLE: {
            // The second hit is decided before the first one lands.
            int hits = current_target.get_status(StatusType::VULNERABLE) > 0 ? 2 : 1;
            for (int blow = 0; blow < hits; ++blow) {
                hit(current_target, base_value);
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
            hit(current_target, base_value + secondary_value * strikes);
            set_log(upgraded ? "Played Perfected Strike+!"sv : "Played Perfected Strike!"sv);
            break;
        }
        case CardId::BLUDGEON: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Bludgeon+!"sv : "Played Bludgeon!"sv);
            break;
        }
        case CardId::BULLY: {
            hit(current_target, base_value + secondary_value * current_target.get_status(StatusType::VULNERABLE));
            set_log(upgraded ? "Played Bully+!"sv : "Played Bully!"sv);
            break;
        }
        case CardId::SWORD_BOOMERANG: {
            for (uint8_t blow = 0; blow < secondary_value && has_alive_enemies(); ++blow) {
                Enemy& victim = enemies[random_alive_enemy_index()];
                hit(victim, base_value);
            }
            set_log(upgraded ? "Played Sword Boomerang+!"sv : "Played Sword Boomerang!"sv);
            break;
        }
        case CardId::BREAK: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Break+!"sv : "Played Break!"sv);
            break;
        }
        case CardId::CONFLAGRATION: {
            for (uint8_t blow = 0; blow < secondary_value; ++blow) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        hit(enemies[i], base_value);
                    }
                }
            }
            set_log(upgraded ? "Played Conflagration+!"sv : "Played Conflagration!"sv);
            break;
        }
        case CardId::EXPECT_A_FIGHT: {
            int16_t strength = std::max<int16_t>(0, player.get_status(StatusType::STRENGTH));
            gain_block(static_cast<int16_t>(base_value + secondary_value * strength));
            set_log(upgraded ? "Played Expect a Fight+!"sv : "Played Expect a Fight!"sv);
            break;
        }
        case CardId::PILLAGE: {
            hit(current_target, base_value);
            // Stops on the first non-Attack, a full hand or empty piles.
            while (draw_card() && CardDef::get(hand.back().id).type == CardType::ATTACK) {
            }
            set_log(upgraded ? "Played Pillage+!"sv : "Played Pillage!"sv);
            break;
        }
        case CardId::UPPERCUT: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::WEAK, secondary_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Uppercut+!"sv : "Played Uppercut!"sv);
            break;
        }
        case CardId::FIGHT_ME: {
            for (int blow = 0; blow < 2; ++blow) {
                hit(current_target, base_value);
            }
            player.add_status(StatusType::STRENGTH, secondary_value);
            if (current_target.is_alive()) afflict(current_target, StatusType::STRENGTH, 1);
            set_log(upgraded ? "Played Fight Me+!"sv : "Played Fight Me!"sv);
            break;
        }
        case CardId::LEG_SWEEP: {
            afflict(current_target, StatusType::WEAK, secondary_value);
            gain_block(base_value);
            set_log(upgraded ? "Played Leg Sweep+!"sv : "Played Leg Sweep!"sv);
            break;
        }
        case CardId::HAZE: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    afflict(enemies[i], StatusType::POISON, base_value);
                    afflict(enemies[i], StatusType::WEAK, secondary_value);
                }
            }
            set_log(upgraded ? "Played Haze+!"sv : "Played Haze!"sv);
            break;
        }
        case CardId::PRECISE_CUT: {
            // The card already left the hand: what remains are the other cards.
            hit(current_target, static_cast<int16_t>(base_value - secondary_value * static_cast<int>(hand.size())));
            set_log(upgraded ? "Played Precise Cut+!"sv : "Played Precise Cut!"sv);
            break;
        }
        case CardId::BOUNCING_FLASK: {
            for (uint8_t bounce = 0; bounce < secondary_value; ++bounce) {
                afflict(enemies[random_alive_enemy_index()], StatusType::POISON, base_value);
            }
            set_log(upgraded ? "Played Bouncing Flask+!"sv : "Played Bouncing Flask!"sv);
            break;
        }
        case CardId::FLECHETTES: {
            int skills = 0;
            for (const DeckCard& card : hand) skills += CardDef::get(card.id).type == CardType::SKILL;
            for (int blow = 0; blow < skills; ++blow) {
                hit(current_target, base_value);
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
                gain_block(base_value);
            }
            set_log(upgraded ? "Played Escape Plan+!"sv : "Played Escape Plan!"sv);
            break;
        }
        case CardId::DASH: {
            gain_block(secondary_value);
            hit(current_target, base_value);
            set_log(upgraded ? "Played Dash+!"sv : "Played Dash!"sv);
            break;
        }
        case CardId::BUBBLE_BUBBLE: {
            if (current_target.get_status(StatusType::POISON) > 0) {
                afflict(current_target, StatusType::POISON, base_value);
            }
            set_log(upgraded ? "Played Bubble Bubble+!"sv : "Played Bubble Bubble!"sv);
            break;
        }
        case CardId::OUTBREAK: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    afflict(enemies[i], StatusType::POISON, base_value);
                    enemies[i].apply_poison_tick();
                }
            }
            set_log(upgraded ? "Played Outbreak+!"sv : "Played Outbreak!"sv);
            break;
        }
        case CardId::SHADOW_SHIELD: {
            gain_block(base_value);
            channel_orb(OrbType::DARK);
            set_log(upgraded ? "Played Shadow Shield+!"sv : "Played Shadow Shield!"sv);
            break;
        }
        case CardId::METEOR_STRIKE: {
            hit(current_target, base_value);
            for (uint8_t n = 0; n < secondary_value; ++n) channel_orb(OrbType::PLASMA);
            set_log(upgraded ? "Played Meteor Strike+!"sv : "Played Meteor Strike!"sv);
            break;
        }
        case CardId::GLACIER: {
            gain_block(base_value);
            for (uint8_t n = 0; n < secondary_value; ++n) channel_orb(OrbType::FROST);
            set_log(upgraded ? "Played Glacier+!"sv : "Played Glacier!"sv);
            break;
        }
        case CardId::ICE_LANCE: {
            hit(current_target, base_value);
            for (uint8_t n = 0; n < secondary_value; ++n) channel_orb(OrbType::FROST);
            set_log(upgraded ? "Played Ice Lance+!"sv : "Played Ice Lance!"sv);
            break;
        }
        case CardId::SKIM: {
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Skim+!"sv : "Played Skim!"sv);
            break;
        }
        case CardId::ALL_FOR_ONE: {
            hit(current_target, base_value);
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
            hit(current_target, base_value);
            afflict(current_target, StatusType::WEAK, secondary_value);
            channel_orb(OrbType::DARK);
            set_log(upgraded ? "Played Null+!"sv : "Played Null!"sv);
            break;
        }
        case CardId::GO_FOR_THE_EYES: {
            hit(current_target, base_value);
            if (current_target.current_intent.base_damage > 0) {
                afflict(current_target, StatusType::WEAK, secondary_value);
            }
            set_log(upgraded ? "Played Go for the Eyes+!"sv : "Played Go for the Eyes!"sv);
            break;
        }
        case CardId::SCRAPE: {
            hit(current_target, base_value);
            size_t kept = hand.size();
            draw_cards(secondary_value);
            for (size_t i = hand.size(); i-- > kept;) {
                DeckCard drawn = hand[i];
                if (CardDef::get_cost(drawn.id, drawn.upgraded) != 0) {
                    discard_from_hand(i);
                }
            }
            set_log(upgraded ? "Played Scrape+!"sv : "Played Scrape!"sv);
            break;
        }
        case CardId::BURY: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Bury+!"sv : "Played Bury!"sv);
            break;
        }
        case CardId::MISERY: {
            hit(current_target, base_value);
            for (StatusType debuff : { StatusType::VULNERABLE, StatusType::WEAK, StatusType::POISON }) {
                int16_t amount = current_target.get_status(debuff);
                for (size_t i = 0; amount > 0 && i < enemies.size(); ++i) {
                    if (static_cast<int>(i) != target_idx && enemies[i].is_alive()) afflict(enemies[i], debuff, amount);
                }
            }
            set_log(upgraded ? "Played Misery+!"sv : "Played Misery!"sv);
            break;
        }
        case CardId::UNDEATH: {
            gain_block(base_value);
            create_card_in_discard(DeckCard(CardId::UNDEATH, upgraded));
            set_log(upgraded ? "Played Undeath+!"sv : "Played Undeath!"sv);
            break;
        }
        case CardId::ASHEN_STRIKE: {
            hit(current_target, base_value + secondary_value * static_cast<int>(exhaust_pile.size()));
            set_log(upgraded ? "Played Ashen Strike+!"sv : "Played Ashen Strike!"sv);
            break;
        }
        case CardId::TREMBLE: {
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Tremble+!"sv : "Played Tremble!"sv);
            break;
        }
        case CardId::EVIL_EYE: {
            gain_block(base_value);
            if (exhausted_this_turn > 0) gain_block(base_value);
            set_log(upgraded ? "Played Evil Eye+!"sv : "Played Evil Eye!"sv);
            break;
        }
        case CardId::MOLTEN_FIST: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::VULNERABLE, current_target.get_status(StatusType::VULNERABLE));
            set_log(upgraded ? "Played Molten Fist+!"sv : "Played Molten Fist!"sv);
            break;
        }
        case CardId::CINDER: {
            hit(current_target, base_value);
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
                    gain_block(base_value);
                }
            }
            set_log(upgraded ? "Played Second Wind+!"sv : "Played Second Wind!"sv);
            break;
        }
        case CardId::TRUE_GRIT: {
            // The upgraded card lets the player choose the card.
            gain_block(base_value);
            if (upgraded) {
                ask_choice(CardChoice{ ChoiceKind::EXHAUST, 1 });
            } else if (!hand.empty()) {
                exhaust_from_hand(next_random() % hand.size());
            }
            set_log(upgraded ? "Played True Grit+!"sv : "Played True Grit!"sv);
            break;
        }
        case CardId::DOMINATE: {
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            player.add_status(StatusType::STRENGTH, current_target.get_status(StatusType::VULNERABLE));
            set_log(upgraded ? "Played Dominate+!"sv : "Played Dominate!"sv);
            break;
        }
        case CardId::FIEND_FIRE: {
            // Only the cards held now burn: a card an exhaustion draws (Dark Embrace) lands above them and stays.
            size_t burnt = hand.size();
            for (size_t i = burnt; i-- > 0;) exhaust_from_hand(i);
            for (size_t blow = 0; blow < burnt; ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Fiend Fire+!"sv : "Played Fiend Fire!"sv);
            break;
        }
        case CardId::THRASH: {
            for (int blow = 0; blow < 2; ++blow) {
                hit(current_target, base_value + played_card.bonus);
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
            gain_block(base_value);
            set_log(upgraded ? "Played Impervious+!"sv : "Played Impervious!"sv);
            break;
        }
        case CardId::PACTS_END: {
            if (exhaust_pile.size() >= secondary_value) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        hit(enemies[i], base_value);
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
            // Only the cards held now go: a card a discard reaction draws (Gremlin Horn) lands above them and stays.
            int swapped = static_cast<int>(hand.size());
            for (size_t i = hand.size(); i-- > 0;) discard_from_hand(i);
            draw_cards(swapped);
            set_log(upgraded ? "Played Calculated Gamble+!"sv : "Played Calculated Gamble!"sv);
            break;
        }
        case CardId::MIRAGE: {
            int16_t poison = 0;
            for (const Enemy& foe : enemies) {
                if (foe.is_alive()) poison += foe.get_status(StatusType::POISON);
            }
            gain_block(poison);
            set_log(upgraded ? "Played Mirage+!"sv : "Played Mirage!"sv);
            break;
        }
        case CardId::RAINBOW: {
            for (OrbType orb : { OrbType::LIGHTNING, OrbType::FROST, OrbType::DARK }) channel_orb(orb);
            set_log(upgraded ? "Played Rainbow+!"sv : "Played Rainbow!"sv);
            break;
        }
        case CardId::CHILL: {
            for (int foes = count_alive_enemies(); foes > 0; --foes) channel_orb(OrbType::FROST);
            set_log(upgraded ? "Played Chill+!"sv : "Played Chill!"sv);
            break;
        }
        case CardId::FUSION: {
            channel_orb(OrbType::PLASMA);
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
            player.receive_status(StatusType::STRENGTH, static_cast<int16_t>(-base_value));
            afflict(current_target, StatusType::STRENGTH, static_cast<int16_t>(-secondary_value));
            set_log(upgraded ? "Played Shared Fate+!"sv : "Played Shared Fate!"sv);
            break;
        }
        case CardId::WISP: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            set_log(upgraded ? "Played Wisp+!"sv : "Played Wisp!"sv);
            break;
        }
        case CardId::LUMINESCE: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            set_log(upgraded ? "Played Luminesce+!"sv : "Played Luminesce!"sv);
            break;
        }
        case CardId::MAUL: {
            for (uint8_t blow = 0; blow < secondary_value; ++blow) {
                hit(current_target, static_cast<int16_t>(base_value + played_card.bonus));
            }
            const int growth = upgraded ? 3 : 2;
            played_card.bonus = bump(played_card.bonus, growth);
            boost_copies(CardId::MAUL, growth);
            set_log(upgraded ? "Played Maul+!"sv : "Played Maul!"sv);
            break;
        }
        case CardId::APOTHEOSIS: {
            // Every card in every combat pile; the played card is in none of them.
            const auto upgrade_all = [this](auto& pile) {
                for (DeckCard& held : pile) {
                    if (can_be_picked(held, ChoiceKind::UPGRADE)) held.upgraded = true;
                }
            };
            upgrade_all(hand);
            upgrade_all(draw_pile);
            upgrade_all(discard_pile);
            upgrade_all(exhaust_pile);
            set_log(upgraded ? "Played Apotheosis+!"sv : "Played Apotheosis!"sv);
            break;
        }
        case CardId::WHISTLE: {
            hit(current_target, base_value);
            if (current_target.is_alive()) {
                current_target.stun();
            }
            set_log(upgraded ? "Played Whistle+!"sv : "Played Whistle!"sv);
            break;
        }
        case CardId::PROWESS: {
            player.add_status(StatusType::STRENGTH, base_value);
            player.add_status(StatusType::DEXTERITY, base_value);
            set_log(upgraded ? "Played Prowess+!"sv : "Played Prowess!"sv);
            break;
        }
        case CardId::SQUASH: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Squash+!"sv : "Played Squash!"sv);
            break;
        }
        case CardId::EXTERMINATE: {
            for (uint8_t blow = 0; blow < secondary_value; ++blow) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        hit(enemies[i], base_value);
                    }
                }
            }
            set_log(upgraded ? "Played Exterminate+!"sv : "Played Exterminate!"sv);
            break;
        }
        case CardId::BRIGHTEST_FLAME: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            draw_cards(secondary_value);
            player.lose_max_hp(2);
            set_log(upgraded ? "Played Brightest Flame+!"sv : "Played Brightest Flame!"sv);
            break;
        }
        case CardId::FLASH_OF_STEEL: {
            hit(current_target, base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Flash of Steel+!"sv : "Played Flash of Steel!"sv);
            break;
        }
        case CardId::PECK: {
            for (uint8_t blow = 0; blow < secondary_value; ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Peck+!"sv : "Played Peck!"sv);
            break;
        }
        case CardId::PRODUCTION: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            set_log(upgraded ? "Played Production+!"sv : "Played Production!"sv);
            break;
        }
        case CardId::MASTER_OF_STRATEGY: {
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Master of Strategy+!"sv : "Played Master of Strategy!"sv);
            break;
        }
        case CardId::FEEDING_FRENZY: {
            gain_temporary(StatusType::STRENGTH, static_cast<int8_t>(base_value));
            set_log(upgraded ? "Played Feeding Frenzy+!"sv : "Played Feeding Frenzy!"sv);
            break;
        }
        case CardId::OUTMANEUVER: {
            schedule(DelayedType::ENERGY, base_value);
            set_log(upgraded ? "Played Outmaneuver+!"sv : "Played Outmaneuver!"sv);
            break;
        }
        case CardId::TORIC_TOUGHNESS: {
            gain_block(base_value);
            schedule(DelayedType::BLOCK, static_cast<int16_t>(base_value + player.get_status(StatusType::DEXTERITY)), 1, 2);
            set_log(upgraded ? "Played Toric Toughness+!"sv : "Played Toric Toughness!"sv);
            break;
        }
        case CardId::ENTRENCH: {
            add_block(player.block);
            set_log(upgraded ? "Played Entrench+!"sv : "Played Entrench!"sv);
            break;
        }
        case CardId::STACK: {
            gain_block(static_cast<int>(discard_pile.size()) + base_value);
            set_log(upgraded ? "Played Stack+!"sv : "Played Stack!"sv);
            break;
        }
        case CardId::RIP_AND_TEAR: {
            for (uint8_t blow = 0; blow < secondary_value && has_alive_enemies(); ++blow) {
                Enemy& victim = enemies[random_alive_enemy_index()];
                hit(victim, base_value);
            }
            set_log(upgraded ? "Played Rip and Tear+!"sv : "Played Rip and Tear!"sv);
            break;
        }
        case CardId::SCRAWL: {
            while (hand.size() < hand.capacity() && (!draw_pile.empty() || !discard_pile.empty())) {
                const size_t before = hand.size();
                draw_card();
                if (hand.size() == before) break; // Drawing is blocked
            }
            set_log(upgraded ? "Played Scrawl+!"sv : "Played Scrawl!"sv);
            break;
        }
        case CardId::MIND_BLAST: {
            hit(current_target, static_cast<int16_t>(draw_pile.size()));
            set_log(upgraded ? "Played Mind Blast+!"sv : "Played Mind Blast!"sv);
            break;
        }
        case CardId::SHOCKWAVE: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (!enemies[i].is_alive()) continue;
                afflict(enemies[i], StatusType::WEAK, base_value);
                afflict(enemies[i], StatusType::VULNERABLE, base_value);
            }
            set_log(upgraded ? "Played Shockwave+!"sv : "Played Shockwave!"sv);
            break;
        }
        case CardId::BOLAS: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Bolas+!"sv : "Played Bolas!"sv);
            break;
        }
        case CardId::THRUMMING_HATCHET: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Thrumming Hatchet+!"sv : "Played Thrumming Hatchet!"sv);
            break;
        }
        case CardId::CLASH: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Clash+!"sv : "Played Clash!"sv);
            break;
        }
        case CardId::IMPATIENCE: {
            bool has_attack = false;
            for (const DeckCard& held : hand) {
                if (CardDef::get(held.id).type == CardType::ATTACK) has_attack = true;
            }
            if (!has_attack) draw_cards(secondary_value);
            set_log(upgraded ? "Played Impatience+!"sv : "Played Impatience!"sv);
            break;
        }
        case CardId::VOLLEY: {
            for (int blow = 0; blow < x_value && has_alive_enemies(); ++blow) {
                hit(enemies[random_alive_enemy_index()], base_value);
            }
            set_log(upgraded ? "Played Volley+!"sv : "Played Volley!"sv);
            break;
        }
        case CardId::OMNISLICE: {
            hit(current_target, base_value);
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (static_cast<int>(i) != target_idx && enemies[i].is_alive()) hit(enemies[i], base_value);
            }
            set_log(upgraded ? "Played Omnislice+!"sv : "Played Omnislice!"sv);
            break;
        }
        case CardId::RESTLESSNESS: {
            if (hand.empty()) {
                draw_cards(secondary_value);
                player.energy = static_cast<int8_t>(player.energy + secondary_value);
            }
            set_log(upgraded ? "Played Restlessness+!"sv : "Played Restlessness!"sv);
            break;
        }
        case CardId::ETERNAL_ARMOR: {
            player.add_status(StatusType::PLATED_ARMOR, base_value);
            set_log(upgraded ? "Played Eternal Armor+!"sv : "Played Eternal Armor!"sv);
            break;
        }
        case CardId::FASTEN: {
            add_power(PowerId::FASTEN, base_value);
            set_log(upgraded ? "Played Fasten+!"sv : "Played Fasten!"sv);
            break;
        }
        case CardId::REBOUND: {
            hit(current_target, base_value);
            rebound_pending = true;
            set_log(upgraded ? "Played Rebound+!"sv : "Played Rebound!"sv);
            break;
        }
        case CardId::PANIC_BUTTON: {
            gain_block(base_value);
            no_card_block_turns = secondary_value;
            set_log(upgraded ? "Played Panic Button+!"sv : "Played Panic Button!"sv);
            break;
        }
        case CardId::GOLD_AXE: {
            // The tally is updated once the card is resolved: this card counts too.
            hit(current_target, static_cast<int16_t>(counters.combat_cards_played + 1));
            set_log(upgraded ? "Played Gold Axe+!"sv : "Played Gold Axe!"sv);
            break;
        }
        case CardId::ANOINTED: {
            for (size_t i = draw_pile.size(); i-- > 0 && hand.size() < hand.capacity();) {
                if (card_meta(draw_pile[i].id).rarity == CardRarity::RARE) {
                    hand.push_back(draw_pile[i]);
                    draw_pile.erase(i);
                }
            }
            set_log(upgraded ? "Played Anointed+!"sv : "Played Anointed!"sv);
            break;
        }
        case CardId::FISTICUFFS: {
            const int16_t dealt = hit(current_target, base_value);
            // Block from the damage dealt: exact, Dexterity does not apply, but Panic Button still stops it.
            if (no_card_block_turns == 0) add_block(dealt);
            set_log(upgraded ? "Played Fisticuffs+!"sv : "Played Fisticuffs!"sv);
            break;
        }
        case CardId::REND: {
            int debuffs = 0;
            for (const StatusEffect& status : current_target.statuses) {
                if (is_debuff(status.type, status.amount)) ++debuffs;
            }
            hit(current_target, static_cast<int16_t>(base_value + secondary_value * debuffs));
            set_log(upgraded ? "Played Rend+!"sv : "Played Rend!"sv);
            break;
        }
        case CardId::THE_BOMB: {
            schedule(DelayedType::DAMAGE_ALL, base_value, secondary_value);
            set_log(upgraded ? "Played The Bomb+!"sv : "Played The Bomb!"sv);
            break;
        }
        case CardId::ROLLING_BOULDER: {
            add_power(PowerId::ROLLING_BOULDER, base_value);
            set_log(upgraded ? "Played Rolling Boulder+!"sv : "Played Rolling Boulder!"sv);
            break;
        }
        case CardId::PANACHE: {
            add_power(PowerId::PANACHE, base_value);
            set_log(upgraded ? "Played Panache+!"sv : "Played Panache!"sv);
            break;
        }
        case CardId::JACKPOT: {
            hit(current_target, base_value);
            CardFilter zero_cost;
            zero_cost.cost = 0;
            zero_cost.cost_upgraded = upgraded;
            for (uint8_t copy = 0; copy < secondary_value; ++copy) create_random_card(zero_cost, Pile::HAND, upgraded);
            set_log(upgraded ? "Played Jackpot+!"sv : "Played Jackpot!"sv);
            break;
        }
        case CardId::BEAT_DOWN: {
            // The attacks are picked first, so one played now cannot come back into the pick.
            FixedVector<DeckCard, 4> picked;
            for (uint8_t pick = 0; pick < base_value; ++pick) {
                FixedVector<uint8_t, 50> attacks;
                for (size_t i = 0; i < discard_pile.size(); ++i) {
                    if (CardDef::get(discard_pile[i].id).type == CardType::ATTACK) attacks.push_back(static_cast<uint8_t>(i));
                }
                if (attacks.empty()) break;
                const size_t chosen = attacks[next_random() % attacks.size()];
                picked.push_back(discard_pile[chosen]);
                discard_pile.erase(chosen);
            }
            for (size_t i = 0; i < picked.size() && has_alive_enemies(); ++i) autoplay(picked[i]);
            set_log(upgraded ? "Played Beat Down+!"sv : "Played Beat Down!"sv);
            break;
        }
        case CardId::CATASTROPHE: {
            for (uint8_t round = 0; round < base_value && has_alive_enemies(); ++round) {
                if (draw_pile.empty()) shuffle_discard_into_draw();
                if (draw_pile.empty()) break;
                const size_t chosen = next_random() % draw_pile.size();
                const DeckCard picked = draw_pile[chosen];
                draw_pile.erase(chosen);
                autoplay(picked);
            }
            set_log(upgraded ? "Played Catastrophe+!"sv : "Played Catastrophe!"sv);
            break;
        }
        case CardId::NOSTALGIA: {
            add_power(PowerId::NOSTALGIA);
            set_log(upgraded ? "Played Nostalgia+!"sv : "Played Nostalgia!"sv);
            break;
        }
        case CardId::RELAX: {
            gain_block(base_value);
            schedule(DelayedType::DRAW, secondary_value);
            schedule(DelayedType::ENERGY, secondary_value);
            set_log(upgraded ? "Played Relax+!"sv : "Played Relax!"sv);
            break;
        }
        case CardId::NEOWS_FURY: {
            hit(current_target, base_value);
            CardChoice choice;
            choice.kind = ChoiceKind::TO_HAND;
            choice.count = secondary_value;
            choice.optional = true;
            choice.source = Pile::DISCARD;
            ask_choice(choice);
            set_log(upgraded ? "Played Neow's Fury+!"sv : "Played Neow's Fury!"sv);
            break;
        }
        case CardId::METAMORPHOSIS: {
            for (uint8_t added = 0; added < base_value; ++added) {
                const CardId attack = random_card(CardFilter(CardType::ATTACK));
                if (attack == CardId::NONE) break;
                DeckCard copy(attack);
                const uint8_t printed = CardDef::get_cost(attack, false);
                if (printed != COST_X) copy.cost_delta = static_cast<int8_t>(-static_cast<int>(printed));
                create_card(copy, Pile::DRAW);
            }
            set_log(upgraded ? "Played Metamorphosis+!"sv : "Played Metamorphosis!"sv);
            break;
        }
        case CardId::DISTRACTION: {
            const CardId skill = random_card(CardFilter(CardType::SKILL));
            if (skill != CardId::NONE) {
                DeckCard copy(skill);
                copy.free_this_turn = true;
                create_card(copy, Pile::HAND);
            }
            set_log(upgraded ? "Played Distraction+!"sv : "Played Distraction!"sv);
            break;
        }
        case CardId::CALTROPS: {
            player.add_status(StatusType::THORNS, base_value);
            set_log(upgraded ? "Played Caltrops+!"sv : "Played Caltrops!"sv);
            break;
        }
        case CardId::PROLONG: {
            if (player.block > 0) schedule(DelayedType::BLOCK, player.block);
            set_log(upgraded ? "Played Prolong+!"sv : "Played Prolong!"sv);
            break;
        }
        case CardId::SALVO: {
            hit(current_target, base_value);
            retain_hand = true;
            set_log(upgraded ? "Played Salvo+!"sv : "Played Salvo!"sv);
            break;
        }
        case CardId::THINKING_AHEAD: {
            draw_cards(base_value);
            ask_choice(CardChoice{ ChoiceKind::TOP_OF_DRAW, 1 });
            set_log(upgraded ? "Played Thinking Ahead+!"sv : "Played Thinking Ahead!"sv);
            break;
        }
        case CardId::JACK_OF_ALL_TRADES: {
            for (uint8_t added = 0; added < base_value; ++added) {
                create_random_card(CardFilter(CardPool::COLORLESS), Pile::HAND);
            }
            set_log(upgraded ? "Played Jack of All Trades+!"sv : "Played Jack of All Trades!"sv);
            break;
        }
        case CardId::HELLO_WORLD: {
            add_power(PowerId::HELLO_WORLD, 1);
            set_log(upgraded ? "Played Hello World+!"sv : "Played Hello World!"sv);
            break;
        }
        case CardId::DARK_SHACKLES: {
            sap_strength(current_target, base_value);
            set_log(upgraded ? "Played Dark Shackles+!"sv : "Played Dark Shackles!"sv);
            break;
        }
        case CardId::EQUILIBRIUM: {
            gain_block(base_value);
            retain_hand = true;
            set_log(upgraded ? "Played Equilibrium+!"sv : "Played Equilibrium!"sv);
            break;
        }
        case CardId::PURITY: {
            CardChoice choice;
            choice.kind = ChoiceKind::EXHAUST;
            choice.count = base_value;
            choice.optional = true;
            ask_choice(choice);
            set_log(upgraded ? "Played Purity+!"sv : "Played Purity!"sv);
            break;
        }
        case CardId::SEEKER_STRIKE: {
            hit(current_target, base_value);
            offer_from_draw(secondary_value);
            set_log(upgraded ? "Played Seeker Strike+!"sv : "Played Seeker Strike!"sv);
            break;
        }
        case CardId::SPLASH: {
            // Three Attacks of the pool of another hero, drawn at random among the other four.
            uint8_t other = static_cast<uint8_t>(next_random() % (static_cast<uint8_t>(CharacterId::COUNT) - 1));
            if (other >= static_cast<uint8_t>(current_character)) ++other;
            const CardPool pool = static_cast<CardPool>(other + 1);
            offer_random_cards(CardFilter(pool, CardType::ATTACK), secondary_value, upgraded, true);
            set_log(upgraded ? "Played Splash+!"sv : "Played Splash!"sv);
            break;
        }
        case CardId::SECRET_TECHNIQUE:
        case CardId::SECRET_WEAPON: {
            CardChoice choice;
            choice.kind = ChoiceKind::TO_HAND;
            choice.source = Pile::DRAW;
            choice.any_type = false;
            choice.type = played_card.id == CardId::SECRET_TECHNIQUE ? CardType::SKILL : CardType::ATTACK;
            ask_choice(choice);
            set_log(upgraded ? "Played Secret card+!"sv : "Played Secret card!"sv);
            break;
        }
        case CardId::DRAMATIC_ENTRANCE: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) hit(enemies[i], base_value);
            }
            set_log(upgraded ? "Played Dramatic Entrance+!"sv : "Played Dramatic Entrance!"sv);
            break;
        }
        case CardId::MAYHEM: {
            add_power(PowerId::MAYHEM, 1);
            set_log(upgraded ? "Played Mayhem+!"sv : "Played Mayhem!"sv);
            break;
        }
        case CardId::PREP_TIME: {
            add_power(PowerId::PREP_TIME, base_value);
            set_log(upgraded ? "Played Prep Time+!"sv : "Played Prep Time!"sv);
            break;
        }
        case CardId::STRATAGEM: {
            add_power(PowerId::STRATAGEM, 1);
            set_log(upgraded ? "Played Stratagem+!"sv : "Played Stratagem!"sv);
            break;
        }
        case CardId::CALAMITY: {
            add_power(PowerId::CALAMITY, 1);
            set_log(upgraded ? "Played Calamity+!"sv : "Played Calamity!"sv);
            break;
        }
        case CardId::AUTOMATION: {
            add_power(PowerId::AUTOMATION, 1);
            set_log(upgraded ? "Played Automation+!"sv : "Played Automation!"sv);
            break;
        }
        case CardId::ENLIGHTENMENT: {
            for (DeckCard& held : hand) {
                if (!upgraded) {
                    held.cost_one_this_turn = true;
                    continue;
                }
                // For the combat: the copy's own cost change brings it down to 1.
                const int cost = effective_cost(held);
                if (cost != COST_X && cost > 1) held.cost_delta = static_cast<int8_t>(held.cost_delta - (cost - 1));
            }
            set_log(upgraded ? "Played Enlightenment+!"sv : "Played Enlightenment!"sv);
            break;
        }
        case CardId::DUAL_WIELD:
        case CardId::HEIRLOOM_HAMMER:
        case CardId::NIGHTMARE: {
            if (played_card.id == CardId::HEIRLOOM_HAMMER) hit(current_target, base_value);
            CardChoice choice;
            choice.kind = played_card.id == CardId::DUAL_WIELD    ? ChoiceKind::COPY_ATTACK_OR_POWER
                          : played_card.id == CardId::NIGHTMARE ? ChoiceKind::COPY_NEXT_TURN
                                                                : ChoiceKind::COPY_COLORLESS;
            choice.amount = played_card.id == CardId::DUAL_WIELD ? base_value : 1;
            ask_choice(choice);
            set_log(upgraded ? "Played a copying card+!"sv : "Played a copying card!"sv);
            break;
        }
        case CardId::BUNDLE_OF_JOY: {
            for (uint8_t made = 0; made < base_value; ++made) {
                create_random_card(CardFilter(CardPool::COLORLESS), Pile::HAND);
            }
            set_log(upgraded ? "Played Bundle of Joy+!"sv : "Played Bundle of Joy!"sv);
            break;
        }
        case CardId::CALL_OF_THE_VOID:
        case CardId::SPECTRUM_SHIFT:
        case CardId::CREATIVE_AI:
        case CardId::TOOLS_OF_THE_TRADE: {
            const PowerId power = played_card.id == CardId::CALL_OF_THE_VOID ? PowerId::CALL_OF_THE_VOID
                                  : played_card.id == CardId::SPECTRUM_SHIFT ? PowerId::SPECTRUM_SHIFT
                                  : played_card.id == CardId::CREATIVE_AI    ? PowerId::CREATIVE_AI
                                                                             : PowerId::TOOLS_OF_THE_TRADE;
            add_power(power, 1);
            set_log(upgraded ? "Played a Power+!"sv : "Played a Power!"sv);
            break;
        }
        case CardId::DECISIONS_DECISIONS: {
            draw_cards(base_value);
            CardChoice choice;
            choice.kind = ChoiceKind::PLAY_REPEATED;
            choice.any_type = false;
            choice.type = CardType::SKILL;
            choice.amount = secondary_value;
            ask_choice(choice);
            set_log(upgraded ? "Played Decisions, Decisions+!"sv : "Played Decisions, Decisions!"sv);
            break;
        }
        case CardId::WHITE_NOISE:
        case CardId::INFERNAL_BLADE: {
            const CardId made = random_card(CardFilter(played_card.id == CardId::WHITE_NOISE ? CardType::POWER : CardType::ATTACK));
            if (made != CardId::NONE) {
                DeckCard copy(made);
                copy.free_this_turn = true;
                create_card(copy, Pile::HAND);
            }
            set_log(upgraded ? "Played a card maker+!"sv : "Played a card maker!"sv);
            break;
        }
        case CardId::STOKE: {
            // Only the cards held now burn: a card an exhaustion draws (Dark Embrace) lands above them and stays.
            const size_t burnt = hand.size();
            for (size_t i = burnt; i-- > 0;) exhaust_from_hand(i);
            for (size_t made = 0; made < burnt; ++made) create_random_card(CardFilter(), Pile::HAND, upgraded);
            set_log(upgraded ? "Played Stoke+!"sv : "Played Stoke!"sv);
            break;
        }
        case CardId::BYRD_SWOOP: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Byrd Swoop+!"sv : "Played Byrd Swoop!"sv);
            break;
        }
        case CardId::QUASAR: {
            offer_random_cards(CardFilter(CardPool::COLORLESS), static_cast<uint8_t>(base_value), upgraded);
            set_log(upgraded ? "Played Quasar+!"sv : "Played Quasar!"sv);
            break;
        }
        case CardId::DISCOVERY: {
            offer_random_cards(CardFilter(), secondary_value, false, true);
            set_log(upgraded ? "Played Discovery+!"sv : "Played Discovery!"sv);
            break;
        }
        case CardId::ABUNDANCE: {
            offer_random_cards(CardFilter(CardType::POWER), 3, true, true);
            set_log(upgraded ? "Played Abundance+!"sv : "Played Abundance!"sv);
            break;
        }
        case CardId::MAD_SCIENCE: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Mad Science+!"sv : "Played Mad Science!"sv);
            break;
        }
        case CardId::ULTIMATE_STRIKE: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Ultimate Strike+!"sv : "Played Ultimate Strike!"sv);
            break;
        }
        case CardId::ULTIMATE_DEFEND: {
            gain_block(static_cast<int16_t>(base_value + power_amount(PowerId::FASTEN)));
            set_log(upgraded ? "Played Ultimate Defend+!"sv : "Played Ultimate Defend!"sv);
            break;
        }
        case CardId::FINESSE: {
            gain_block(base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Finesse+!"sv : "Played Finesse!"sv);
            break;
        }
        case CardId::PUTREFY: {
            afflict(current_target, StatusType::WEAK, secondary_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
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
        case CardId::BLOOD_WALL: {
            lose_hp(secondary_value);
            gain_block(base_value);
            set_log(upgraded ? "Played Blood Wall+!"sv : "Played Blood Wall!"sv);
            break;
        }
        case CardId::BLOODLETTING: {
            lose_hp(secondary_value);
            player.energy = static_cast<int8_t>(player.energy + base_value);
            set_log(upgraded ? "Played Bloodletting+!"sv : "Played Bloodletting!"sv);
            break;
        }
        case CardId::BREAKTHROUGH: {
            lose_hp(secondary_value);
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                }
            }
            set_log(upgraded ? "Played Breakthrough+!"sv : "Played Breakthrough!"sv);
            break;
        }
        case CardId::FORGOTTEN_RITUAL: {
            if (exhausted_this_turn > 0) {
                player.energy = static_cast<int8_t>(player.energy + base_value);
            }
            set_log(upgraded ? "Played Forgotten Ritual+!"sv : "Played Forgotten Ritual!"sv);
            break;
        }
        case CardId::HEMOKINESIS: {
            lose_hp(secondary_value);
            hit(current_target, base_value);
            set_log(upgraded ? "Played Hemokinesis+!"sv : "Played Hemokinesis!"sv);
            break;
        }
        case CardId::MANGLE: {
            hit(current_target, base_value);
            sap_strength(current_target, secondary_value);
            set_log(upgraded ? "Played Mangle+!"sv : "Played Mangle!"sv);
            break;
        }
        case CardId::OFFERING: {
            lose_hp(6);
            player.energy = static_cast<int8_t>(player.energy + base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Offering+!"sv : "Played Offering!"sv);
            break;
        }
        case CardId::SETUP_STRIKE: {
            hit(current_target, base_value);
            gain_temporary(StatusType::STRENGTH, static_cast<int8_t>(secondary_value));
            set_log(upgraded ? "Played Setup Strike+!"sv : "Played Setup Strike!"sv);
            break;
        }
        case CardId::SPITE: {
            for (int blow = 0; blow < (counters.hp_lost > 0 ? secondary_value : 1); ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Spite+!"sv : "Played Spite!"sv);
            break;
        }
        case CardId::WHIRLWIND: {
            for (int blow = 0; blow < x_value; ++blow) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        hit(enemies[i], base_value);
                    }
                }
            }
            set_log(upgraded ? "Played Whirlwind+!"sv : "Played Whirlwind!"sv);
            break;
        }
        case CardId::ANTICIPATE: {
            gain_temporary(StatusType::DEXTERITY, static_cast<int8_t>(base_value));
            set_log(upgraded ? "Played Anticipate+!"sv : "Played Anticipate!"sv);
            break;
        }
        case CardId::ASSASSINATE: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Assassinate+!"sv : "Played Assassinate!"sv);
            break;
        }
        case CardId::BACKSTAB: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Backstab+!"sv : "Played Backstab!"sv);
            break;
        }
        case CardId::BLUR: {
            gain_block(base_value);
            if (keep_block_turns < UINT8_MAX) ++keep_block_turns;
            set_log(upgraded ? "Played Blur+!"sv : "Played Blur!"sv);
            break;
        }
        case CardId::DODGE_AND_ROLL: {
            gain_block(base_value);
            schedule(DelayedType::BLOCK, static_cast<int16_t>(base_value + player.get_status(StatusType::DEXTERITY)));
            set_log(upgraded ? "Played Dodge and Roll+!"sv : "Played Dodge and Roll!"sv);
            break;
        }
        case CardId::FINISHER: {
            for (int blow = 0; blow < counters.attacks_played; ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Finisher+!"sv : "Played Finisher!"sv);
            break;
        }
        case CardId::MALAISE: {
            int16_t dose = static_cast<int16_t>(x_value + secondary_value);
            afflict(current_target, StatusType::STRENGTH, static_cast<int16_t>(-dose));
            afflict(current_target, StatusType::WEAK, dose);
            set_log(upgraded ? "Played Malaise+!"sv : "Played Malaise!"sv);
            break;
        }
        case CardId::MEMENTO_MORI: {
            hit(current_target, static_cast<int16_t>(base_value + secondary_value * counters.cards_discarded));
            set_log(upgraded ? "Played Memento Mori+!"sv : "Played Memento Mori!"sv);
            break;
        }
        case CardId::MURDER: {
            hit(current_target, static_cast<int16_t>(base_value + secondary_value * counters.combat_cards_drawn));
            set_log(upgraded ? "Played Murder+!"sv : "Played Murder!"sv);
            break;
        }
        case CardId::PIERCING_WAIL: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    sap_strength(enemies[i], secondary_value);
                }
            }
            set_log(upgraded ? "Played Piercing Wail+!"sv : "Played Piercing Wail!"sv);
            break;
        }
        case CardId::PREDATOR: {
            hit(current_target, base_value);
            schedule(DelayedType::DRAW, secondary_value);
            set_log(upgraded ? "Played Predator+!"sv : "Played Predator!"sv);
            break;
        }
        case CardId::SIDESTEP: {
            schedule(DelayedType::ENERGY, base_value);
            set_log(upgraded ? "Played Sidestep+!"sv : "Played Sidestep!"sv);
            break;
        }
        case CardId::SKEWER: {
            for (int blow = 0; blow < x_value; ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Skewer+!"sv : "Played Skewer!"sv);
            break;
        }
        case CardId::SUPPRESS: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::WEAK, secondary_value);
            set_log(upgraded ? "Played Suppress+!"sv : "Played Suppress!"sv);
            break;
        }
        case CardId::EXPERTISE: {
            size_t held = hand.size();
            draw_cards(secondary_value);
            for (size_t i = held; i < hand.size(); ++i) hand[i].added_keywords |= KEYWORD_RETAIN_ONCE;
            set_log(upgraded ? "Played Expertise+!"sv : "Played Expertise!"sv);
            break;
        }
        case CardId::SNAKEBITE: {
            afflict(current_target, StatusType::POISON, secondary_value);
            set_log(upgraded ? "Played Snakebite+!"sv : "Played Snakebite!"sv);
            break;
        }
        case CardId::BOOT_SEQUENCE: {
            gain_block(base_value);
            set_log(upgraded ? "Played Boot Sequence+!"sv : "Played Boot Sequence!"sv);
            break;
        }
        case CardId::CHARGE_BATTERY: {
            gain_block(base_value);
            schedule(DelayedType::ENERGY, secondary_value);
            set_log(upgraded ? "Played Charge Battery+!"sv : "Played Charge Battery!"sv);
            break;
        }
        case CardId::FTL: {
            hit(current_target, base_value);
            if (counters.cards_played() < secondary_value) draw_cards(1);
            set_log(upgraded ? "Played FTL+!"sv : "Played FTL!"sv);
            break;
        }
        case CardId::HELIX_DRILL: {
            for (int blow = 0; blow < counters.energy_spent; ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Helix Drill+!"sv : "Played Helix Drill!"sv);
            break;
        }
        case CardId::LIGHTNING_ROD: {
            gain_block(base_value);
            schedule(DelayedType::CHANNEL_LIGHTNING, 1, 1, secondary_value);
            set_log(upgraded ? "Played Lightning Rod+!"sv : "Played Lightning Rod!"sv);
            break;
        }
        case CardId::TEMPEST: {
            for (int blow = 0; blow < x_value + secondary_value; ++blow) {
                channel_orb(OrbType::LIGHTNING);
            }
            set_log(upgraded ? "Played Tempest+!"sv : "Played Tempest!"sv);
            break;
        }
        case CardId::CRUSH_UNDER: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                    sap_strength(enemies[i], secondary_value);
                }
            }
            set_log(upgraded ? "Played Crush Under+!"sv : "Played Crush Under!"sv);
            break;
        }
        case CardId::GLITTERSTREAM: {
            gain_block(base_value);
            schedule(DelayedType::BLOCK, static_cast<int16_t>(secondary_value + player.get_status(StatusType::DEXTERITY)));
            set_log(upgraded ? "Played Glitterstream+!"sv : "Played Glitterstream!"sv);
            break;
        }
        case CardId::HEAVENLY_DRILL: {
            for (int blow = 0; blow < (x_value >= secondary_value ? x_value * 2 : x_value); ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Heavenly Drill+!"sv : "Played Heavenly Drill!"sv);
            break;
        }
        case CardId::HEGEMONY: {
            hit(current_target, base_value);
            schedule(DelayedType::ENERGY, secondary_value);
            set_log(upgraded ? "Played Hegemony+!"sv : "Played Hegemony!"sv);
            break;
        }
        case CardId::KNOW_THY_PLACE: {
            afflict(current_target, StatusType::WEAK, base_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Know Thy Place+!"sv : "Played Know Thy Place!"sv);
            break;
        }
        case CardId::LUNAR_BLAST: {
            for (int blow = 0; blow < counters.skills_played; ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Lunar Blast+!"sv : "Played Lunar Blast!"sv);
            break;
        }
        case CardId::PROPHESIZE: {
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Prophesize+!"sv : "Played Prophesize!"sv);
            break;
        }
        case CardId::SUPERMASSIVE: {
            hit(current_target, static_cast<int16_t>(base_value + secondary_value * counters.combat_cards_created));
            set_log(upgraded ? "Played Supermassive+!"sv : "Played Supermassive!"sv);
            break;
        }
        case CardId::CONVERGENCE: {
            schedule(DelayedType::ENERGY, base_value);
            schedule(DelayedType::STARS, secondary_value);
            retain_hand = true;
            set_log(upgraded ? "Played Convergence+!"sv : "Played Convergence!"sv);
            break;
        }
        case CardId::DEATH_MARCH: {
            hit(current_target, static_cast<int16_t>(base_value + secondary_value * counters.cards_drawn));
            set_log(upgraded ? "Played Death March+!"sv : "Played Death March!"sv);
            break;
        }
        case CardId::DEFILE: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Defile+!"sv : "Played Defile!"sv);
            break;
        }
        case CardId::DEFY: {
            gain_block(base_value);
            afflict(current_target, StatusType::WEAK, secondary_value);
            set_log(upgraded ? "Played Defy+!"sv : "Played Defy!"sv);
            break;
        }
        case CardId::DELAY: {
            gain_block(base_value);
            schedule(DelayedType::ENERGY, secondary_value);
            set_log(upgraded ? "Played Delay+!"sv : "Played Delay!"sv);
            break;
        }
        case CardId::ENFEEBLING_TOUCH: {
            sap_strength(current_target, secondary_value);
            set_log(upgraded ? "Played Enfeebling Touch+!"sv : "Played Enfeebling Touch!"sv);
            break;
        }
        case CardId::FEAR: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Fear+!"sv : "Played Fear!"sv);
            break;
        }
        case CardId::INVOKE: {
            schedule(DelayedType::SUMMON, secondary_value);
            schedule(DelayedType::ENERGY, base_value);
            set_log(upgraded ? "Played Invoke+!"sv : "Played Invoke!"sv);
            break;
        }
        case CardId::PARSE: {
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Parse+!"sv : "Played Parse!"sv);
            break;
        }
        case CardId::PULL_FROM_BELOW: {
            for (int blow = 0; blow < counters.combat_ethereal_played; ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Pull from Below+!"sv : "Played Pull from Below!"sv);
            break;
        }
        case CardId::REAP: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Reap+!"sv : "Played Reap!"sv);
            break;
        }
        case CardId::SOW: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                }
            }
            set_log(upgraded ? "Played Sow+!"sv : "Played Sow!"sv);
            break;
        }
        case CardId::ERADICATE: {
            for (int blow = 0; blow < x_value; ++blow) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Eradicate+!"sv : "Played Eradicate!"sv);
            break;
        }
        case CardId::ALIGNMENT: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            set_log(upgraded ? "Played Alignment+!"sv : "Played Alignment!"sv);
            break;
        }
        case CardId::ASTRAL_PULSE: {
            for (int wave = 0; wave < secondary_value; ++wave) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        hit(enemies[i], base_value);
                    }
                }
            }
            set_log(upgraded ? "Played Astral Pulse+!"sv : "Played Astral Pulse!"sv);
            break;
        }
        case CardId::CLOAK_OF_STARS: {
            gain_block(base_value);
            set_log(upgraded ? "Played Cloak of Stars+!"sv : "Played Cloak of Stars!"sv);
            break;
        }
        case CardId::COMET: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::WEAK, secondary_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Comet+!"sv : "Played Comet!"sv);
            break;
        }
        case CardId::CRESCENT_SPEAR: {
            // Every card of the deck still in the piles, and this one.
            int star_cards = 1;
            for (const DeckCard& card : hand) star_cards += CardDef::get_star_cost(card.id, card.upgraded) != 0;
            for (const DeckCard& card : draw_pile) star_cards += CardDef::get_star_cost(card.id, card.upgraded) != 0;
            for (const DeckCard& card : discard_pile) star_cards += CardDef::get_star_cost(card.id, card.upgraded) != 0;
            hit(current_target, static_cast<int16_t>(base_value + secondary_value * star_cards));
            set_log(upgraded ? "Played Crescent Spear+!"sv : "Played Crescent Spear!"sv);
            break;
        }
        case CardId::DEVASTATE: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Devastate+!"sv : "Played Devastate!"sv);
            break;
        }
        case CardId::DYING_STAR: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                    sap_strength(enemies[i], secondary_value);
                }
            }
            set_log(upgraded ? "Played Dying Star+!"sv : "Played Dying Star!"sv);
            break;
        }
        case CardId::GAMMA_BLAST: {
            hit(current_target, base_value);
            afflict(current_target, StatusType::WEAK, secondary_value);
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Gamma Blast+!"sv : "Played Gamma Blast!"sv);
            break;
        }
        case CardId::GLOW: {
            gain_stars(base_value);
            draw_cards(secondary_value);
            schedule(DelayedType::DRAW, secondary_value);
            set_log(upgraded ? "Played Glow+!"sv : "Played Glow!"sv);
            break;
        }
        case CardId::GUIDING_STAR: {
            hit(current_target, base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Guiding Star+!"sv : "Played Guiding Star!"sv);
            break;
        }
        case CardId::HIDDEN_CACHE: {
            gain_stars(base_value);
            schedule(DelayedType::STARS, secondary_value);
            set_log(upgraded ? "Played Hidden Cache+!"sv : "Played Hidden Cache!"sv);
            break;
        }
        case CardId::METEOR_SHOWER: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                    afflict(enemies[i], StatusType::WEAK, secondary_value);
                    afflict(enemies[i], StatusType::VULNERABLE, secondary_value);
                }
            }
            set_log(upgraded ? "Played Meteor Shower+!"sv : "Played Meteor Shower!"sv);
            break;
        }
        case CardId::PARTICLE_WALL: {
            gain_block(base_value);
            after_play = AfterPlay::HAND;
            set_log(upgraded ? "Played Particle Wall+!"sv : "Played Particle Wall!"sv);
            break;
        }
        case CardId::RADIATE: {
            for (int wave = 0; wave < counters.stars_gained; ++wave) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        hit(enemies[i], base_value);
                    }
                }
            }
            set_log(upgraded ? "Played Radiate+!"sv : "Played Radiate!"sv);
            break;
        }
        case CardId::RESONANCE: {
            player.add_status(StatusType::STRENGTH, base_value);
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    afflict(enemies[i], StatusType::STRENGTH, static_cast<int16_t>(-secondary_value));
                }
            }
            set_log(upgraded ? "Played Resonance+!"sv : "Played Resonance!"sv);
            break;
        }
        case CardId::ROYAL_GAMBLE: {
            gain_stars(base_value);
            set_log(upgraded ? "Played Royal Gamble+!"sv : "Played Royal Gamble!"sv);
            break;
        }
        case CardId::SEVEN_STARS: {
            for (int wave = 0; wave < secondary_value; ++wave) {
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        hit(enemies[i], base_value);
                    }
                }
            }
            set_log(upgraded ? "Played Seven Stars+!"sv : "Played Seven Stars!"sv);
            break;
        }
        case CardId::SHINING_STRIKE: {
            hit(current_target, base_value);
            gain_stars(secondary_value);
            after_play = AfterPlay::DRAW_TOP;
            set_log(upgraded ? "Played Shining Strike+!"sv : "Played Shining Strike!"sv);
            break;
        }
        case CardId::STARDUST: {
            for (int blow = 0; blow < x_value && has_alive_enemies(); ++blow) {
                Enemy& victim = enemies[random_alive_enemy_index()];
                hit(victim, base_value);
            }
            set_log(upgraded ? "Played Stardust+!"sv : "Played Stardust!"sv);
            break;
        }
        case CardId::FEED: {
            hit(current_target, base_value);
            if (fatal_kills > 0) {
                player.max_hp = static_cast<int16_t>(player.max_hp + secondary_value);
                player.hp = static_cast<int16_t>(player.hp + secondary_value);
            }
            set_log(upgraded ? "Played Feed+!"sv : "Played Feed!"sv);
            break;
        }
        case CardId::BATTLE_TRANCE: {
            draw_cards(secondary_value);
            no_draw_this_turn = true;
            set_log(upgraded ? "Played Battle Trance+!"sv : "Played Battle Trance!"sv);
            break;
        }
        case CardId::ECHOING_SLASH: {
            // Every enemy killed by a sweep buys one more sweep.
            for (int sweeps = 1; sweeps > 0 && has_alive_enemies(); --sweeps) {
                uint8_t killed_before = fatal_kills;
                for (size_t i = 0; i < enemies.size(); ++i) {
                    if (enemies[i].is_alive()) {
                        hit(enemies[i], base_value);
                    }
                }
                sweeps += fatal_kills - killed_before;
            }
            set_log(upgraded ? "Played Echoing Slash+!"sv : "Played Echoing Slash!"sv);
            break;
        }
        case CardId::GRAND_FINALE: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                }
            }
            set_log(upgraded ? "Played Grand Finale+!"sv : "Played Grand Finale!"sv);
            break;
        }
        case CardId::SUNDER: {
            hit(current_target, base_value);
            if (fatal_kills > 0) {
                player.energy = static_cast<int8_t>(player.energy + secondary_value);
            }
            set_log(upgraded ? "Played Sunder+!"sv : "Played Sunder!"sv);
            break;
        }
        case CardId::KNOCKOUT_BLOW: {
            hit(current_target, base_value);
            if (fatal_kills > 0) {
                gain_stars(secondary_value);
            }
            set_log(upgraded ? "Played Knockout Blow+!"sv : "Played Knockout Blow!"sv);
            break;
        }
        case CardId::FEEL_NO_PAIN: {
            add_power(PowerId::FEEL_NO_PAIN, base_value);
            set_log(upgraded ? "Played Feel No Pain+!"sv : "Played Feel No Pain!"sv);
            break;
        }
        case CardId::DARK_EMBRACE: {
            add_power(PowerId::DARK_EMBRACE, base_value);
            set_log(upgraded ? "Played Dark Embrace+!"sv : "Played Dark Embrace!"sv);
            break;
        }
        case CardId::DRUM_OF_BATTLE: {
            draw_cards(base_value);
            set_log(upgraded ? "Played Drum of Battle+!"sv : "Played Drum of Battle!"sv);
            break;
        }
        case CardId::BARRICADE: {
            add_power(PowerId::BARRICADE);
            set_log(upgraded ? "Played Barricade+!"sv : "Played Barricade!"sv);
            break;
        }
        case CardId::INFERNO: {
            add_power(PowerId::INFERNO, base_value);
            set_log(upgraded ? "Played Inferno+!"sv : "Played Inferno!"sv);
            break;
        }
        case CardId::CRIMSON_MANTLE: {
            add_power(PowerId::CRIMSON_MANTLE, base_value);
            set_log(upgraded ? "Played Crimson Mantle+!"sv : "Played Crimson Mantle!"sv);
            break;
        }
        case CardId::RUPTURE: {
            add_power(PowerId::RUPTURE, base_value);
            set_log(upgraded ? "Played Rupture+!"sv : "Played Rupture!"sv);
            break;
        }
        case CardId::AGGRESSION: {
            add_power(PowerId::AGGRESSION);
            set_log(upgraded ? "Played Aggression+!"sv : "Played Aggression!"sv);
            break;
        }
        case CardId::PYRE: {
            add_power(PowerId::TURN_ENERGY, base_value);
            set_log(upgraded ? "Played Pyre+!"sv : "Played Pyre!"sv);
            break;
        }
        case CardId::DEMON_FORM: {
            add_power(PowerId::TURN_STRENGTH, base_value);
            set_log(upgraded ? "Played Demon Form+!"sv : "Played Demon Form!"sv);
            break;
        }
        case CardId::JUGGERNAUT: {
            add_power(PowerId::JUGGERNAUT, base_value);
            set_log(upgraded ? "Played Juggernaut+!"sv : "Played Juggernaut!"sv);
            break;
        }
        case CardId::RAGE: {
            add_power(PowerId::RAGE, base_value, true);
            set_log(upgraded ? "Played Rage+!"sv : "Played Rage!"sv);
            break;
        }
        case CardId::VICIOUS: {
            add_power(PowerId::VICIOUS, base_value);
            set_log(upgraded ? "Played Vicious+!"sv : "Played Vicious!"sv);
            break;
        }
        case CardId::ACCELERANT: {
            add_power(PowerId::ACCELERANT, base_value);
            set_log(upgraded ? "Played Accelerant+!"sv : "Played Accelerant!"sv);
            break;
        }
        case CardId::SPEEDSTER: {
            add_power(PowerId::SPEEDSTER, base_value);
            set_log(upgraded ? "Played Speedster+!"sv : "Played Speedster!"sv);
            break;
        }
        case CardId::ENVENOM: {
            add_power(PowerId::ENVENOM, base_value);
            set_log(upgraded ? "Played Envenom+!"sv : "Played Envenom!"sv);
            break;
        }
        case CardId::SERPENT_FORM: {
            add_power(PowerId::SERPENT_FORM, base_value);
            set_log(upgraded ? "Played Serpent Form+!"sv : "Played Serpent Form!"sv);
            break;
        }
        case CardId::AFTERIMAGE: {
            add_power(PowerId::AFTERIMAGE, base_value);
            set_log(upgraded ? "Played Afterimage+!"sv : "Played Afterimage!"sv);
            break;
        }
        case CardId::WELL_LAID_PLANS: {
            add_power(PowerId::KEEP_HAND);
            set_log(upgraded ? "Played Well-Laid Plans+!"sv : "Played Well-Laid Plans!"sv);
            break;
        }
        case CardId::STRANGLE: {
            hit(current_target, base_value);
            add_power(PowerId::STRANGLE, secondary_value, true, static_cast<int8_t>(target_idx));
            set_log(upgraded ? "Played Strangle+!"sv : "Played Strangle!"sv);
            break;
        }
        case CardId::CORROSIVE_WAVE: {
            add_power(PowerId::CORROSIVE_WAVE, base_value, true);
            set_log(upgraded ? "Played Corrosive Wave+!"sv : "Played Corrosive Wave!"sv);
            break;
        }
        case CardId::NOXIOUS_FUMES: {
            add_power(PowerId::TURN_POISON_ALL, base_value);
            set_log(upgraded ? "Played Noxious Fumes+!"sv : "Played Noxious Fumes!"sv);
            break;
        }
        case CardId::MACHINE_LEARNING: {
            add_power(PowerId::TURN_DRAW, base_value);
            set_log(upgraded ? "Played Machine Learning+!"sv : "Played Machine Learning!"sv);
            break;
        }
        case CardId::STORM: {
            add_power(PowerId::STORM, base_value);
            set_log(upgraded ? "Played Storm+!"sv : "Played Storm!"sv);
            break;
        }
        case CardId::FERAL: {
            add_power(PowerId::FERAL, base_value);
            set_log(upgraded ? "Played Feral+!"sv : "Played Feral!"sv);
            break;
        }
        case CardId::SUBROUTINE: {
            add_power(PowerId::SUBROUTINE, base_value);
            set_log(upgraded ? "Played Subroutine+!"sv : "Played Subroutine!"sv);
            break;
        }
        case CardId::CHILD_OF_THE_STARS: {
            add_power(PowerId::CHILD_OF_THE_STARS, base_value);
            set_log(upgraded ? "Played Child of the Stars+!"sv : "Played Child of the Stars!"sv);
            break;
        }
        case CardId::GENESIS: {
            add_power(PowerId::TURN_STARS, base_value);
            set_log(upgraded ? "Played Genesis+!"sv : "Played Genesis!"sv);
            break;
        }
        case CardId::THE_SEALED_THRONE: {
            add_power(PowerId::SEALED_THRONE, base_value);
            set_log(upgraded ? "Played The Sealed Throne+!"sv : "Played The Sealed Throne!"sv);
            break;
        }
        case CardId::BLACK_HOLE: {
            add_power(PowerId::BLACK_HOLE, base_value);
            set_log(upgraded ? "Played Black Hole+!"sv : "Played Black Hole!"sv);
            break;
        }
        case CardId::MAKE_IT_SO: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Make It So+!"sv : "Played Make It So!"sv);
            break;
        }
        case CardId::PALE_BLUE_DOT: {
            add_power(PowerId::PALE_BLUE_DOT, base_value);
            set_log(upgraded ? "Played Pale Blue Dot+!"sv : "Played Pale Blue Dot!"sv);
            break;
        }
        case CardId::ARSENAL: {
            add_power(PowerId::ARSENAL, base_value);
            set_log(upgraded ? "Played Arsenal+!"sv : "Played Arsenal!"sv);
            break;
        }
        case CardId::MONOLOGUE: {
            add_power(PowerId::MONOLOGUE, base_value, true);
            set_log(upgraded ? "Played Monologue+!"sv : "Played Monologue!"sv);
            break;
        }
        case CardId::ORBIT: {
            add_power(PowerId::ORBIT, base_value);
            set_log(upgraded ? "Played Orbit+!"sv : "Played Orbit!"sv);
            break;
        }
        case CardId::PILLAR_OF_CREATION: {
            add_power(PowerId::PILLAR_OF_CREATION, base_value);
            set_log(upgraded ? "Played Pillar of Creation+!"sv : "Played Pillar of Creation!"sv);
            break;
        }
        case CardId::MONARCHS_GAZE: {
            add_power(PowerId::MONARCHS_GAZE, base_value);
            set_log(upgraded ? "Played Monarch's Gaze+!"sv : "Played Monarch's Gaze!"sv);
            break;
        }
        case CardId::FRIENDSHIP: {
            player.receive_status(StatusType::STRENGTH, static_cast<int16_t>(-secondary_value));
            add_power(PowerId::TURN_ENERGY, base_value);
            set_log(upgraded ? "Played Friendship+!"sv : "Played Friendship!"sv);
            break;
        }
        case CardId::DANSE_MACABRE: {
            add_power(PowerId::DANSE_MACABRE, base_value);
            set_log(upgraded ? "Played Danse Macabre+!"sv : "Played Danse Macabre!"sv);
            break;
        }
        case CardId::DEMESNE: {
            add_power(PowerId::TURN_ENERGY, base_value);
            add_power(PowerId::TURN_DRAW, secondary_value);
            set_log(upgraded ? "Played Demesne+!"sv : "Played Demesne!"sv);
            break;
        }
        case CardId::SPIRIT_OF_ASH: {
            add_power(PowerId::SPIRIT_OF_ASH, base_value);
            set_log(upgraded ? "Played Spirit of Ash+!"sv : "Played Spirit of Ash!"sv);
            break;
        }
        case CardId::PAGESTORM: {
            add_power(PowerId::PAGESTORM, base_value);
            set_log(upgraded ? "Played Pagestorm+!"sv : "Played Pagestorm!"sv);
            break;
        }
        case CardId::SLEIGHT_OF_FLESH: {
            add_power(PowerId::SLEIGHT_OF_FLESH, base_value);
            set_log(upgraded ? "Played Sleight of Flesh+!"sv : "Played Sleight of Flesh!"sv);
            break;
        }
        case CardId::SHATTER: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                }
            }
            while (!player.orbs.empty()) evoke_orb(0, secondary_value);
            set_log(upgraded ? "Played Shatter+!"sv : "Played Shatter!"sv);
            break;
        }
        case CardId::VOLTAIC: {
            for (int bolts = counters.combat_lightning_channeled; bolts > 0; --bolts) channel_orb(OrbType::LIGHTNING);
            set_log(upgraded ? "Played Voltaic+!"sv : "Played Voltaic!"sv);
            break;
        }
        case CardId::QUADCAST: {
            evoke_orb(0, secondary_value);
            set_log(upgraded ? "Played Quadcast+!"sv : "Played Quadcast!"sv);
            break;
        }
        case CardId::BARRAGE: {
            for (int round = 0; round < static_cast<int>(player.orbs.size()); ++round) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Barrage+!"sv : "Played Barrage!"sv);
            break;
        }
        case CardId::TESLA_COIL: {
            hit(current_target, base_value);
            for (int round = 0; round < secondary_value; ++round) {
                for (size_t slot = 0; slot < player.orbs.size(); ++slot) {
                    if (player.orbs[slot] == OrbType::LIGHTNING) trigger_orb_passive(slot, &current_target);
                }
            }
            set_log(upgraded ? "Played Tesla Coil+!"sv : "Played Tesla Coil!"sv);
            break;
        }
        case CardId::CHAOS: {
            constexpr OrbType KINDS[] = { OrbType::LIGHTNING, OrbType::FROST, OrbType::DARK, OrbType::PLASMA };
            for (int round = 0; round < secondary_value; ++round) {
                channel_orb(KINDS[next_random() % 4]);
            }
            set_log(upgraded ? "Played Chaos+!"sv : "Played Chaos!"sv);
            break;
        }
        case CardId::COMPILE_DRIVER: {
            hit(current_target, base_value);
            draw_cards(unique_orbs());
            set_log(upgraded ? "Played Compile Driver+!"sv : "Played Compile Driver!"sv);
            break;
        }
        case CardId::DARKNESS: {
            channel_orb(OrbType::DARK);
            for (int round = 0; round < secondary_value; ++round) {
                for (size_t slot = 0; slot < player.orbs.size(); ++slot) {
                    if (player.orbs[slot] == OrbType::DARK) trigger_orb_passive(slot);
                }
            }
            set_log(upgraded ? "Played Darkness+!"sv : "Played Darkness!"sv);
            break;
        }
        case CardId::MULTI_CAST: {
            if (x_value + secondary_value > 0) evoke_orb(0, x_value + secondary_value);
            set_log(upgraded ? "Played Multi-Cast+!"sv : "Played Multi-Cast!"sv);
            break;
        }
        case CardId::LOOP: {
            add_power(PowerId::LOOP, base_value);
            set_log(upgraded ? "Played Loop+!"sv : "Played Loop!"sv);
            break;
        }
        case CardId::CONSUMING_SHADOW: {
            for (int round = 0; round < secondary_value; ++round) {
                channel_orb(OrbType::DARK);
            }
            add_power(PowerId::CONSUMING_SHADOW);
            set_log(upgraded ? "Played Consuming Shadow+!"sv : "Played Consuming Shadow!"sv);
            break;
        }
        case CardId::HAILSTORM: {
            add_power(PowerId::HAILSTORM, base_value);
            set_log(upgraded ? "Played Hailstorm+!"sv : "Played Hailstorm!"sv);
            break;
        }
        case CardId::THUNDER: {
            add_power(PowerId::THUNDER, base_value);
            set_log(upgraded ? "Played Thunder+!"sv : "Played Thunder!"sv);
            break;
        }
        case CardId::COOLANT: {
            add_power(PowerId::COOLANT, base_value);
            set_log(upgraded ? "Played Coolant+!"sv : "Played Coolant!"sv);
            break;
        }
        case CardId::BIASED_COGNITION: {
            player.add_status(StatusType::FOCUS, base_value);
            add_power(PowerId::BIASED_COGNITION);
            set_log(upgraded ? "Played Biased Cognition+!"sv : "Played Biased Cognition!"sv);
            break;
        }
        case CardId::HOTFIX: {
            gain_temporary(StatusType::FOCUS, static_cast<int8_t>(base_value));
            set_log(upgraded ? "Played Hotfix+!"sv : "Played Hotfix!"sv);
            break;
        }
        case CardId::DEFRAGMENT: {
            player.add_status(StatusType::FOCUS, base_value);
            set_log(upgraded ? "Played Defragment+!"sv : "Played Defragment!"sv);
            break;
        }
        case CardId::FOCUSED_STRIKE: {
            hit(current_target, base_value);
            gain_temporary(StatusType::FOCUS, static_cast<int8_t>(secondary_value));
            set_log(upgraded ? "Played Focused Strike+!"sv : "Played Focused Strike!"sv);
            break;
        }
        case CardId::HYPERBEAM: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                }
            }
            player.receive_status(StatusType::FOCUS, static_cast<int16_t>(-secondary_value));
            set_log(upgraded ? "Played Hyperbeam+!"sv : "Played Hyperbeam!"sv);
            break;
        }
        case CardId::NEGATIVE_PULSE: {
            gain_block(base_value);
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    afflict(enemies[i], StatusType::DOOM, secondary_value);
                }
            }
            set_log(upgraded ? "Played Negative Pulse+!"sv : "Played Negative Pulse!"sv);
            break;
        }
        case CardId::END_OF_DAYS: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    afflict(enemies[i], StatusType::DOOM, secondary_value);
                }
            }
            for (Enemy& foe : enemies) reap(foe);
            set_log(upgraded ? "Played End of Days+!"sv : "Played End of Days!"sv);
            break;
        }
        case CardId::BLIGHT_STRIKE: {
            int16_t dealt = hit(current_target, base_value);
            if (current_target.is_alive()) afflict(current_target, StatusType::DOOM, dealt);
            set_log(upgraded ? "Played Blight Strike+!"sv : "Played Blight Strike!"sv);
            break;
        }
        case CardId::NO_ESCAPE: {
            afflict(current_target, StatusType::DOOM, static_cast<int16_t>(base_value + secondary_value * (current_target.get_status(StatusType::DOOM) / 10)));
            set_log(upgraded ? "Played No Escape+!"sv : "Played No Escape!"sv);
            break;
        }
        case CardId::SCOURGE: {
            afflict(current_target, StatusType::DOOM, base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Scourge+!"sv : "Played Scourge!"sv);
            break;
        }
        case CardId::DEATHBRINGER: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    afflict(enemies[i], StatusType::DOOM, base_value);
                    afflict(enemies[i], StatusType::WEAK, secondary_value);
                }
            }
            set_log(upgraded ? "Played Deathbringer+!"sv : "Played Deathbringer!"sv);
            break;
        }
        case CardId::DEATHS_DOOR: {
            gain_block(base_value);
            if (counters.doom_applied > 0) {
                for (int round = 0; round < secondary_value; ++round) {
                    gain_block(base_value);
                }
            }
            set_log(upgraded ? "Played Death's Door+!"sv : "Played Death's Door!"sv);
            break;
        }
        case CardId::COUNTDOWN: {
            add_power(PowerId::COUNTDOWN, base_value);
            set_log(upgraded ? "Played Countdown+!"sv : "Played Countdown!"sv);
            break;
        }
        case CardId::REAPER_FORM: {
            add_power(PowerId::REAPER_FORM);
            set_log(upgraded ? "Played Reaper Form+!"sv : "Played Reaper Form!"sv);
            break;
        }
        case CardId::NEUROSURGE: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            draw_cards(secondary_value);
            add_power(PowerId::NEUROSURGE, 3);
            set_log(upgraded ? "Played Neurosurge+!"sv : "Played Neurosurge!"sv);
            break;
        }
        case CardId::TIMES_UP: {
            hit(current_target, current_target.get_status(StatusType::DOOM));
            set_log(upgraded ? "Played Time's Up+!"sv : "Played Time's Up!"sv);
            break;
        }
        case CardId::OBLIVION: {
            add_power(PowerId::OBLIVION, base_value, true, static_cast<int8_t>(target_idx));
            set_log(upgraded ? "Played Oblivion+!"sv : "Played Oblivion!"sv);
            break;
        }
        case CardId::SHROUD: {
            add_power(PowerId::SHROUD, base_value);
            set_log(upgraded ? "Played Shroud+!"sv : "Played Shroud!"sv);
            break;
        }
        case CardId::CAPACITOR: {
            change_orb_slots(base_value);
            set_log(upgraded ? "Played Capacitor+!"sv : "Played Capacitor!"sv);
            break;
        }
        case CardId::BULK_UP: {
            change_orb_slots(-secondary_value);
            player.add_status(StatusType::STRENGTH, base_value);
            player.add_status(StatusType::DEXTERITY, base_value);
            set_log(upgraded ? "Played Bulk Up+!"sv : "Played Bulk Up!"sv);
            break;
        }
        case CardId::SYNCHRONIZE: {
            gain_temporary(StatusType::FOCUS, static_cast<int8_t>(base_value * unique_orbs()));
            set_log(upgraded ? "Played Synchronize+!"sv : "Played Synchronize!"sv);
            break;
        }
        case CardId::HOWL_FROM_BEYOND: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                }
            }
            set_log(upgraded ? "Played Howl from Beyond+!"sv : "Played Howl from Beyond!"sv);
            break;
        }
        case CardId::HELLRAISER: {
            add_power(PowerId::HELLRAISER);
            set_log(upgraded ? "Played Hellraiser+!"sv : "Played Hellraiser!"sv);
            break;
        }
        case CardId::STAMPEDE: {
            add_power(PowerId::STAMPEDE);
            set_log(upgraded ? "Played Stampede+!"sv : "Played Stampede!"sv);
            break;
        }
        case CardId::CASCADE: {
            for (int round = x_value + secondary_value; round > 0; --round) {
                if (draw_pile.empty()) shuffle_discard_into_draw();
                if (draw_pile.empty()) break;
                DeckCard top = draw_pile.back();
                draw_pile.pop_back();
                autoplay(top, -1, AfterPlay::DISCARD);
            }
            set_log(upgraded ? "Played Cascade+!"sv : "Played Cascade!"sv);
            break;
        }
        case CardId::ONE_TWO_PUNCH: {
            add_power(PowerId::REPLAY_ATTACK, secondary_value, true);
            set_log(upgraded ? "Played One-Two Punch+!"sv : "Played One-Two Punch!"sv);
            break;
        }
        case CardId::TEAR_ASUNDER: {
            for (int round = 1 + counters.combat_hp_losses; round > 0; --round) {
                hit(current_target, base_value);
            }
            set_log(upgraded ? "Played Tear Asunder+!"sv : "Played Tear Asunder!"sv);
            break;
        }
        case CardId::HAVOC: {
            for (int round = 1; round > 0; --round) {
                if (draw_pile.empty()) shuffle_discard_into_draw();
                if (draw_pile.empty()) break;
                DeckCard top = draw_pile.back();
                draw_pile.pop_back();
                autoplay(top, -1, AfterPlay::EXHAUST);
            }
            set_log(upgraded ? "Played Havoc+!"sv : "Played Havoc!"sv);
            break;
        }
        case CardId::CORRUPTION: {
            add_power(PowerId::CORRUPTION);
            set_log(upgraded ? "Played Corruption+!"sv : "Played Corruption!"sv);
            break;
        }
        case CardId::UNRELENTING: {
            hit(current_target, base_value);
            add_power(PowerId::FREE_ATTACK);
            set_log(upgraded ? "Played Unrelenting+!"sv : "Played Unrelenting!"sv);
            break;
        }
        case CardId::MIDNIGHT: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Midnight+!"sv : "Played Midnight!"sv);
            break;
        }
        case CardId::STOMP: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                }
            }
            set_log(upgraded ? "Played Stomp+!"sv : "Played Stomp!"sv);
            break;
        }
        case CardId::RAMPAGE: {
            hit(current_target, static_cast<int16_t>(base_value + played_card.bonus));
            played_card.bonus = bump(played_card.bonus, secondary_value);
            set_log(upgraded ? "Played Rampage+!"sv : "Played Rampage!"sv);
            break;
        }
        case CardId::BURST: {
            add_power(PowerId::REPLAY_SKILL, secondary_value, true);
            set_log(upgraded ? "Played Burst+!"sv : "Played Burst!"sv);
            break;
        }
        case CardId::BULLET_TIME: {
            no_draw_this_turn = true;
            for (DeckCard& held : hand) held.free_this_turn = true;
            set_log(upgraded ? "Played Bullet Time+!"sv : "Played Bullet Time!"sv);
            break;
        }
        case CardId::PINPOINT: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Pinpoint+!"sv : "Played Pinpoint!"sv);
            break;
        }
        case CardId::POUNCE: {
            hit(current_target, base_value);
            add_power(PowerId::FREE_SKILL);
            set_log(upgraded ? "Played Pounce+!"sv : "Played Pounce!"sv);
            break;
        }
        case CardId::ECHO_FORM: {
            add_power(PowerId::ECHO_FORM, base_value);
            // This turn already had its first card: the echo starts on the next one.
            if (ActivePower* echo = find_power(PowerId::ECHO_FORM)) echo->counter = static_cast<uint8_t>(echo->amount);
            set_log(upgraded ? "Played Echo Form+!"sv : "Played Echo Form!"sv);
            break;
        }
        case CardId::SIGNAL_BOOST: {
            add_power(PowerId::REPLAY_POWER);
            set_log(upgraded ? "Played Signal Boost+!"sv : "Played Signal Boost!"sv);
            break;
        }
        case CardId::UPROAR: {
            for (int round = secondary_value; round > 0; --round) {
                hit(current_target, base_value);
            }
            FixedVector<uint8_t, 50> attacks;
            for (size_t i = 0; i < draw_pile.size(); ++i) {
                if (CardDef::get(draw_pile[i].id).type == CardType::ATTACK) attacks.push_back(static_cast<uint8_t>(i));
            }
            if (!attacks.empty()) {
                size_t pick = attacks[next_random() % attacks.size()];
                DeckCard drawn = draw_pile[pick];
                draw_pile.erase(pick);
                autoplay(drawn);
            }
            set_log(upgraded ? "Played Uproar+!"sv : "Played Uproar!"sv);
            break;
        }
        case CardId::ADAPTIVE_STRIKE: {
            hit(current_target, base_value);
            DeckCard copy(card_id, upgraded);
            copy.cost_delta = static_cast<int8_t>(-CardDef::get_cost(card_id, upgraded));
            create_card_in_discard(copy);
            set_log(upgraded ? "Played Adaptive Strike+!"sv : "Played Adaptive Strike!"sv);
            break;
        }
        case CardId::SYNTHESIS: {
            hit(current_target, base_value);
            add_power(PowerId::FREE_POWER);
            set_log(upgraded ? "Played Synthesis+!"sv : "Played Synthesis!"sv);
            break;
        }
        case CardId::GENETIC_ALGORITHM: {
            gain_block(static_cast<int16_t>(base_value + played_card.bonus));
            played_card.bonus = bump(played_card.bonus, secondary_value);
            if (played_card.deck_index != NOT_IN_DECK) {
                permanent_gains.push_back(PermanentGain{ played_card.deck_index, secondary_value });
            }
            set_log(upgraded ? "Played Genetic Algorithm+!"sv : "Played Genetic Algorithm!"sv);
            break;
        }
        case CardId::MOMENTUM_STRIKE: {
            hit(current_target, base_value);
            played_card.cost_delta = static_cast<int8_t>(-CardDef::get_cost(card_id, upgraded));
            set_log(upgraded ? "Played Momentum Strike+!"sv : "Played Momentum Strike!"sv);
            break;
        }
        case CardId::CLAW: {
            hit(current_target, static_cast<int16_t>(base_value + played_card.bonus));
            played_card.bonus = bump(played_card.bonus, secondary_value);
            boost_copies(CardId::CLAW, secondary_value);
            set_log(upgraded ? "Played Claw+!"sv : "Played Claw!"sv);
            break;
        }
        case CardId::MODDED: {
            change_orb_slots(base_value);
            draw_cards(secondary_value);
            played_card.cost_delta = static_cast<int8_t>(played_card.cost_delta + 1);
            set_log(upgraded ? "Played Modded+!"sv : "Played Modded!"sv);
            break;
        }
        case CardId::BOMBARDMENT: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Bombardment+!"sv : "Played Bombardment!"sv);
            break;
        }
        case CardId::I_AM_INVINCIBLE: {
            gain_block(base_value);
            set_log(upgraded ? "Played I Am Invincible+!"sv : "Played I Am Invincible!"sv);
            break;
        }
        case CardId::VOID_FORM: {
            add_power(PowerId::VOID_FORM, base_value);
            end_turn_requested = true;
            set_log(upgraded ? "Played Void Form+!"sv : "Played Void Form!"sv);
            break;
        }
        case CardId::KINGLY_KICK: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Kingly Kick+!"sv : "Played Kingly Kick!"sv);
            break;
        }
        case CardId::KINGLY_PUNCH: {
            hit(current_target, static_cast<int16_t>(base_value + played_card.bonus));
            set_log(upgraded ? "Played Kingly Punch+!"sv : "Played Kingly Punch!"sv);
            break;
        }
        case CardId::EIDOLON: {
            for (size_t i = 0, count = exhaust_pile.size(); i < count && has_alive_enemies(); ++i) {
                if (CardDef::has_keyword(exhaust_pile[i], KEYWORD_ETHEREAL)) autoplay(exhaust_pile[i], -1, AfterPlay::NOWHERE);
            }
            set_log(upgraded ? "Played Eidolon+!"sv : "Played Eidolon!"sv);
            break;
        }
        case CardId::BANSHEES_CRY: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                }
            }
            set_log(upgraded ? "Played Banshee's Cry+!"sv : "Played Banshee's Cry!"sv);
            break;
        }
        case CardId::VEILPIERCER: {
            hit(current_target, base_value);
            add_power(PowerId::FREE_ETHEREAL);
            set_log(upgraded ? "Played Veilpiercer+!"sv : "Played Veilpiercer!"sv);
            break;
        }
        case CardId::BORROWED_TIME: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            add_power(PowerId::SURCHARGE, secondary_value, true);
            set_log(upgraded ? "Played Borrowed Time+!"sv : "Played Borrowed Time!"sv);
            break;
        }
        case CardId::THE_SCYTHE: {
            hit(current_target, static_cast<int16_t>(base_value + played_card.bonus));
            played_card.bonus = bump(played_card.bonus, secondary_value);
            if (played_card.deck_index != NOT_IN_DECK) {
                permanent_gains.push_back(PermanentGain{ played_card.deck_index, secondary_value });
            }
            set_log(upgraded ? "Played The Scythe+!"sv : "Played The Scythe!"sv);
            break;
        }
        case CardId::MELANCHOLY: {
            gain_block(base_value);
            set_log(upgraded ? "Played Melancholy+!"sv : "Played Melancholy!"sv);
            break;
        }
        case CardId::HANG: {
            hit(current_target, static_cast<int16_t>(base_value << std::min<int>(current_target.hang_marks, 5)));
            current_target.hang_marks = bump(current_target.hang_marks);
            set_log(upgraded ? "Played Hang+!"sv : "Played Hang!"sv);
            break;
        }
        case CardId::TACTICIAN: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            set_log(upgraded ? "Played Tactician+!"sv : "Played Tactician!"sv);
            break;
        }
        case CardId::ABRASIVE: {
            player.add_status(StatusType::DEXTERITY, 1);
            player.add_status(StatusType::THORNS, base_value);
            set_log(upgraded ? "Played Abrasive+!"sv : "Played Abrasive!"sv);
            break;
        }
        case CardId::MASTER_PLANNER: {
            add_power(PowerId::MASTER_PLANNER);
            set_log(upgraded ? "Played Master Planner+!"sv : "Played Master Planner!"sv);
            break;
        }
        case CardId::HAND_TRICK: {
            gain_block(base_value);
            CardChoice choice{ ChoiceKind::MAKE_SLY, 1 };
            choice.any_type = false;
            choice.type = CardType::SKILL;
            ask_choice(choice);
            set_log(upgraded ? "Played Hand Trick+!"sv : "Played Hand Trick!"sv);
            break;
        }
        case CardId::UNTOUCHABLE: {
            gain_block(base_value);
            set_log(upgraded ? "Played Untouchable+!"sv : "Played Untouchable!"sv);
            break;
        }
        case CardId::RICOCHET: {
            for (uint8_t blow = 0; blow < secondary_value && has_alive_enemies(); ++blow) {
                hit(enemies[random_alive_enemy_index()], base_value);
            }
            set_log(upgraded ? "Played Ricochet+!"sv : "Played Ricochet!"sv);
            break;
        }
        case CardId::REFLEX: {
            draw_cards(base_value);
            set_log(upgraded ? "Played Reflex+!"sv : "Played Reflex!"sv);
            break;
        }
        case CardId::FLICK_FLACK: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) hit(enemies[i], base_value);
            }
            set_log(upgraded ? "Played Flick-Flack+!"sv : "Played Flick-Flack!"sv);
            break;
        }
        case CardId::BURN:
        case CardId::DAZED:
        case CardId::SOOT:
        case CardId::VOID_CARD:
        case CardId::WOUND:
        case CardId::FOLLY:
        case CardId::ENTHRALLED:
        case CardId::SPORE_MIND:
        case CardId::INJURY:
        case CardId::GREED:
        case CardId::CLUMSY:
        case CardId::ASCENDERS_BANE:
        case CardId::CURSE_OF_THE_BELL:
        case CardId::POOR_SLEEP:
        case CardId::WRITHE:
        case CardId::NORMALITY:
        case CardId::DOUBT:
        case CardId::SHAME:
        case CardId::DECAY:
        case CardId::REGRET:
        case CardId::BAD_LUCK:
        case CardId::DEBT:
        case CardId::GUILTY:
            // Unplayable: only another card can play them, and nothing happens.
            break;
        case CardId::DEBRIS: {
            set_log("Played Debris!");
            break;
        }
        case CardId::SLIMED: {
            draw_cards(base_value);
            set_log("Played Slimed!");
            break;
        }
        case CardId::FUEL: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            set_log(upgraded ? "Played Fuel+!"sv : "Played Fuel!"sv);
            break;
        }
        case CardId::BOOST_AWAY: {
            gain_block(base_value);
            create_card(CardId::DAZED, Pile::DISCARD);
            set_log(upgraded ? "Played Boost Away+!"sv : "Played Boost Away!"sv);
            break;
        }
        case CardId::FLAK_CANNON: {
            int shots = 0;
            auto purge = [this, &shots](auto& pile) {
                for (size_t i = pile.size(); i-- > 0;) {
                    if (i < pile.size() && CardDef::get(pile[i].id).type == CardType::STATUS) {
                        DeckCard status = pile[i];
                        pile.erase(i);
                        exhaust_card(status);
                        ++shots;
                    }
                }
            };
            purge(hand);
            purge(draw_pile);
            purge(discard_pile);
            for (; shots > 0 && has_alive_enemies(); --shots) hit(enemies[random_alive_enemy_index()], base_value);
            set_log(upgraded ? "Played Flak Cannon+!"sv : "Played Flak Cannon!"sv);
            break;
        }
        case CardId::SMOKESTACK: {
            add_power(PowerId::SMOKESTACK, base_value);
            set_log(upgraded ? "Played Smokestack+!"sv : "Played Smokestack!"sv);
            break;
        }
        case CardId::COMPACT: {
            gain_block(base_value);
            for (DeckCard& held : hand) {
                if (CardDef::get(held.id).type == CardType::STATUS) held = DeckCard(CardId::FUEL, upgraded);
            }
            set_log(upgraded ? "Played Compact+!"sv : "Played Compact!"sv);
            break;
        }
        case CardId::GUNK_UP: {
            for (int blow = 0; blow < secondary_value && current_target.is_alive(); ++blow) hit(current_target, base_value);
            create_card(CardId::SLIMED, Pile::DISCARD);
            set_log(upgraded ? "Played Gunk Up+!"sv : "Played Gunk Up!"sv);
            break;
        }
        case CardId::ITERATION: {
            add_power(PowerId::ITERATION, base_value);
            set_log(upgraded ? "Played Iteration+!"sv : "Played Iteration!"sv);
            break;
        }
        case CardId::FIGHT_THROUGH: {
            gain_block(base_value);
            for (int copy = 0; copy < secondary_value; ++copy) create_card(CardId::WOUND, Pile::DISCARD);
            set_log(upgraded ? "Played Fight Through+!"sv : "Played Fight Through!"sv);
            break;
        }
        case CardId::OVERCLOCK: {
            draw_cards(base_value);
            create_card(CardId::BURN, Pile::DISCARD);
            set_log(upgraded ? "Played Overclock+!"sv : "Played Overclock!"sv);
            break;
        }
        case CardId::ROCKET_PUNCH: {
            hit(current_target, base_value);
            draw_cards(secondary_value);
            played_card.cost_delta = 0;
            set_log(upgraded ? "Played Rocket Punch+!"sv : "Played Rocket Punch!"sv);
            break;
        }
        case CardId::TRASH_TO_TREASURE: {
            add_power(PowerId::TRASH_TO_TREASURE);
            set_log(upgraded ? "Played Trash to Treasure+!"sv : "Played Trash to Treasure!"sv);
            break;
        }
        case CardId::TURBO: {
            player.energy = static_cast<int8_t>(player.energy + base_value);
            create_card(CardId::VOID_CARD, Pile::DISCARD);
            set_log(upgraded ? "Played TURBO+!"sv : "Played TURBO!"sv);
            break;
        }
        case CardId::COLLISION_COURSE: {
            hit(current_target, base_value);
            create_card(CardId::DEBRIS, Pile::HAND);
            set_log(upgraded ? "Played Collision Course+!"sv : "Played Collision Course!"sv);
            break;
        }
        case CardId::CRASH_LANDING: {
            for (size_t i = 0; i < enemies.size(); ++i) {
                if (enemies[i].is_alive()) {
                    hit(enemies[i], base_value);
                }
            }
            while (hand.size() < hand.capacity()) create_card(CardId::DEBRIS, Pile::HAND);
            set_log(upgraded ? "Played Crash Landing+!"sv : "Played Crash Landing!"sv);
            break;
        }
        case CardId::SHIV: {
            int16_t damage = static_cast<int16_t>(base_value + played_card.bonus + power_amount(PowerId::ACCURACY));
            // Phantom Blades: the first Shiv of the turn hits harder.
            if (counters.shivs_played == 0) damage = static_cast<int16_t>(damage + power_amount(PowerId::PHANTOM_BLADES));
            const bool inky = played_card.enchantment == Enchantment::INKY;
            // Fan of Knives: every enemy instead of the one aimed at.
            const bool fan = has_power(PowerId::FAN_OF_KNIVES);
            for (size_t i = 0; i < enemies.size(); ++i) {
                Enemy& foe = enemies[i];
                if (!foe.is_alive() || (!fan && &foe != &current_target)) continue;
                hit(foe, damage);
                if (inky && foe.is_alive()) afflict(foe, StatusType::WEAK, INKY_WEAK);
            }
            set_log(upgraded ? "Played Shiv+!"sv : "Played Shiv!"sv);
            break;
        }
        case CardId::BLADE_DANCE: {
            add_shivs(base_value);
            set_log(upgraded ? "Played Blade Dance+!"sv : "Played Blade Dance!"sv);
            break;
        }
        case CardId::CLOAK_AND_DAGGER: {
            gain_block(base_value);
            add_shivs(secondary_value);
            set_log(upgraded ? "Played Cloak and Dagger+!"sv : "Played Cloak and Dagger!"sv);
            break;
        }
        case CardId::LEADING_STRIKE: {
            hit(current_target, base_value);
            add_shivs(secondary_value);
            set_log(upgraded ? "Played Leading Strike+!"sv : "Played Leading Strike!"sv);
            break;
        }
        case CardId::BLADE_OF_INK: {
            add_shivs(base_value, false, Enchantment::INKY);
            set_log(upgraded ? "Played Blade of Ink+!"sv : "Played Blade of Ink!"sv);
            break;
        }
        case CardId::INFINITE_BLADES: {
            add_power(PowerId::INFINITE_BLADES, base_value);
            set_log(upgraded ? "Played Infinite Blades+!"sv : "Played Infinite Blades!"sv);
            break;
        }
        case CardId::STORM_OF_STEEL: {
            // Only the cards held now go: a card a discard reaction draws (Gremlin Horn) lands above them and stays.
            int thrown = static_cast<int>(hand.size());
            for (size_t i = hand.size(); i-- > 0;) discard_from_hand(i);
            add_shivs(thrown, upgraded);
            set_log(upgraded ? "Played Storm of Steel+!"sv : "Played Storm of Steel!"sv);
            break;
        }
        case CardId::FAN_OF_KNIVES: {
            add_power(PowerId::FAN_OF_KNIVES);
            add_shivs(base_value);
            set_log(upgraded ? "Played Fan of Knives+!"sv : "Played Fan of Knives!"sv);
            break;
        }
        case CardId::PHANTOM_BLADES: {
            add_power(PowerId::PHANTOM_BLADES, base_value);
            set_log(upgraded ? "Played Phantom Blades+!"sv : "Played Phantom Blades!"sv);
            break;
        }
        case CardId::MINION_STRIKE: {
            hit(current_target, base_value);
            draw_cards(1);
            set_log(upgraded ? "Played Minion Strike+!"sv : "Played Minion Strike!"sv);
            break;
        }
        case CardId::MINION_DIVE_BOMB: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Minion Dive Bomb+!"sv : "Played Minion Dive Bomb!"sv);
            break;
        }
        case CardId::MINION_SACRIFICE: {
            gain_block(base_value);
            set_log(upgraded ? "Played Minion Sacrifice+!"sv : "Played Minion Sacrifice!"sv);
            break;
        }
        case CardId::BEGONE: {
            CardChoice choice;
            choice.kind = ChoiceKind::TRANSFORM;
            choice.into = CardId::MINION_STRIKE;
            choice.upgraded = upgraded;
            ask_choice(choice);
            set_log(upgraded ? "Played Begone+!"sv : "Played Begone!"sv);
            break;
        }
        case CardId::CHARGE: {
            CardChoice choice;
            choice.kind = ChoiceKind::TRANSFORM;
            choice.source = Pile::DRAW;
            choice.count = 2;
            choice.into = CardId::MINION_DIVE_BOMB;
            choice.upgraded = upgraded;
            ask_choice(choice);
            set_log(upgraded ? "Played Charge+!"sv : "Played Charge!"sv);
            break;
        }
        case CardId::GUARDS: {
            CardChoice choice;
            choice.kind = ChoiceKind::TRANSFORM;
            choice.count = static_cast<uint8_t>(hand.size());
            choice.optional = true;
            choice.into = CardId::MINION_SACRIFICE;
            choice.upgraded = upgraded;
            ask_choice(choice);
            set_log(upgraded ? "Played Guards+!"sv : "Played Guards!"sv);
            break;
        }
        case CardId::HIDDEN_GEM: {
            // A random card of the draw pile without Echo gains Echo N.
            uint8_t candidates[50];
            size_t count = 0;
            for (size_t i = 0; i < draw_pile.size(); ++i) {
                if (draw_pile[i].echo == 0) candidates[count++] = static_cast<uint8_t>(i);
            }
            if (count > 0) draw_pile[candidates[next_random() % count]].echo = static_cast<uint8_t>(base_value);
            set_log(upgraded ? "Played Hidden Gem+!"sv : "Played Hidden Gem!"sv);
            break;
        }
        case CardId::TRANSFIGURE: {
            CardChoice choice;
            choice.kind = ChoiceKind::ADD_ECHO;
            ask_choice(choice);
            set_log(upgraded ? "Played Transfigure+!"sv : "Played Transfigure!"sv);
            break;
        }
        case CardId::PRIMAL_FORCE: {
            for (size_t i = 0; i < hand.size(); ++i) {
                if (CardDef::get(hand[i].id).type == CardType::ATTACK) transform_card(hand[i], CardId::GIANT_ROCK, upgraded);
            }
            set_log(upgraded ? "Played Primal Force+!"sv : "Played Primal Force!"sv);
            break;
        }
        case CardId::SEANCE: {
            CardChoice choice;
            choice.kind = ChoiceKind::TRANSFORM;
            choice.source = Pile::DRAW;
            choice.into = SOUL_CARD;
            ask_choice(choice);
            set_log(upgraded ? "Played Seance+!"sv : "Played Seance!"sv);
            break;
        }
        case CardId::ENTROPY: {
            add_power(PowerId::ENTROPY, 1);
            set_log(upgraded ? "Played Entropy+!"sv : "Played Entropy!"sv);
            break;
        }
        case CardId::HAND_OF_GREED: {
            hit(current_target, base_value);
            if (fatal_kills > 0) gold_gained = static_cast<int16_t>(gold_gained + base_value);
            set_log(upgraded ? "Played Hand of Greed+!"sv : "Played Hand of Greed!"sv);
            break;
        }
        case CardId::THE_HUNT: {
            hit(current_target, base_value);
            if (fatal_kills > 0) extra_card_rewards = bump(extra_card_rewards);
            set_log(upgraded ? "Played The Hunt+!"sv : "Played The Hunt!"sv);
            break;
        }
        case CardId::ROYALTIES: {
            add_power(PowerId::ROYALTIES, base_value);
            set_log(upgraded ? "Played Royalties+!"sv : "Played Royalties!"sv);
            break;
        }
        case CardId::FORBIDDEN_GRIMOIRE: {
            add_power(PowerId::FORBIDDEN_GRIMOIRE, 1);
            set_log(upgraded ? "Played Forbidden Grimoire+!"sv : "Played Forbidden Grimoire!"sv);
            break;
        }
        case CardId::ALCHEMIZE: {
            potions_gained = bump(potions_gained);
            set_log(upgraded ? "Played Alchemize+!"sv : "Played Alchemize!"sv);
            break;
        }
        case CardId::THE_GAMBIT: {
            gain_block(base_value);
            gambit_active = true;
            set_log(upgraded ? "Played The Gambit+!"sv : "Played The Gambit!"sv);
            break;
        }
        case CardId::SHADOWMELD: {
            double_block_turn = true;
            set_log(upgraded ? "Played Shadowmeld+!"sv : "Played Shadowmeld!"sv);
            break;
        }
        case CardId::UNMOVABLE: {
            add_power(PowerId::UNMOVABLE, 1);
            set_log(upgraded ? "Played Unmovable+!"sv : "Played Unmovable!"sv);
            break;
        }
        case CardId::COLOSSUS: {
            gain_block(base_value);
            player.halve_vulnerable_damage = true;
            set_log(upgraded ? "Played Colossus+!"sv : "Played Colossus!"sv);
            break;
        }
        case CardId::CRUELTY: {
            add_power(PowerId::CRUELTY, base_value);
            set_log(upgraded ? "Played Cruelty+!"sv : "Played Cruelty!"sv);
            break;
        }
        case CardId::TRACKING: {
            add_power(PowerId::TRACKING, 50);
            set_log(upgraded ? "Played Tracking+!"sv : "Played Tracking!"sv);
            break;
        }
        case CardId::LETHALITY: {
            add_power(PowerId::LETHALITY, base_value);
            set_log(upgraded ? "Played Lethality+!"sv : "Played Lethality!"sv);
            break;
        }
        case CardId::SHADOW_STEP: {
            for (size_t i = hand.size(); i-- > 0;) discard_from_hand(i);
            attack_double_next = true;
            set_log(upgraded ? "Played Shadow Step+!"sv : "Played Shadow Step!"sv);
            break;
        }
        case CardId::DEBILITATE: {
            hit(current_target, base_value);
            current_target.debilitate_turns = static_cast<uint8_t>(secondary_value);
            set_log(upgraded ? "Played Debilitate+!"sv : "Played Debilitate!"sv);
            break;
        }
        case CardId::ACCURACY: {
            add_power(PowerId::ACCURACY, base_value);
            set_log(upgraded ? "Played Accuracy+!"sv : "Played Accuracy!"sv);
            break;
        }
        case CardId::UP_MY_SLEEVE: {
            add_shivs(base_value);
            played_card.cost_delta = static_cast<int8_t>(played_card.cost_delta - 1);
            set_log(upgraded ? "Played Up My Sleeve+!"sv : "Played Up My Sleeve!"sv);
            break;
        }
        case CardId::KNIFE_TRAP: {
            for (size_t i = 0, count = exhaust_pile.size(); i < count && has_alive_enemies(); ++i) {
                if (exhaust_pile[i].id != CardId::SHIV) continue;
                if (upgraded) exhaust_pile[i].upgraded = true;
                autoplay(exhaust_pile[i], target_idx, AfterPlay::NOWHERE);
            }
            set_log(upgraded ? "Played Knife Trap+!"sv : "Played Knife Trap!"sv);
            break;
        }
        case CardId::ARMAMENTS: {
            gain_block(base_value);
            if (upgraded) {
                for (DeckCard& held : hand) {
                    if (can_be_picked(held, ChoiceKind::UPGRADE)) held.upgraded = true;
                }
            } else {
                ask_choice(CardChoice{ ChoiceKind::UPGRADE, 1 });
            }
            set_log(upgraded ? "Played Armaments+!"sv : "Played Armaments!"sv);
            break;
        }
        case CardId::BRAND: {
            lose_hp(base_value);
            ask_choice(CardChoice{ ChoiceKind::EXHAUST, 1 });
            player.add_status(StatusType::STRENGTH, secondary_value);
            set_log(upgraded ? "Played Brand+!"sv : "Played Brand!"sv);
            break;
        }
        case CardId::BURNING_PACT: {
            ask_choice(CardChoice{ ChoiceKind::EXHAUST, 1, false, ChoiceThen::DRAW, base_value });
            set_log(upgraded ? "Played Burning Pact+!"sv : "Played Burning Pact!"sv);
            break;
        }
        case CardId::ACROBATICS: {
            draw_cards(base_value);
            ask_choice(CardChoice{ ChoiceKind::DISCARD, 1 });
            set_log(upgraded ? "Played Acrobatics+!"sv : "Played Acrobatics!"sv);
            break;
        }
        case CardId::DAGGER_THROW: {
            hit(current_target, base_value);
            draw_cards(secondary_value);
            ask_choice(CardChoice{ ChoiceKind::DISCARD, 1 });
            set_log(upgraded ? "Played Dagger Throw+!"sv : "Played Dagger Throw!"sv);
            break;
        }
        case CardId::PREPARED: {
            draw_cards(base_value);
            ask_choice(CardChoice{ ChoiceKind::DISCARD, base_value });
            set_log(upgraded ? "Played Prepared+!"sv : "Played Prepared!"sv);
            break;
        }
        case CardId::HIDDEN_DAGGERS: {
            ask_choice(CardChoice{ ChoiceKind::DISCARD, base_value, false, ChoiceThen::SHIVS, secondary_value, upgraded });
            set_log(upgraded ? "Played Hidden Daggers+!"sv : "Played Hidden Daggers!"sv);
            break;
        }
        case CardId::SCAVENGE: {
            ask_choice(CardChoice{ ChoiceKind::EXHAUST, 1 });
            schedule(DelayedType::ENERGY, base_value);
            set_log(upgraded ? "Played Scavenge+!"sv : "Played Scavenge!"sv);
            break;
        }
        case CardId::PHOTON_CUT: {
            hit(current_target, base_value);
            draw_cards(secondary_value);
            ask_choice(CardChoice{ ChoiceKind::TOP_OF_DRAW, 1 });
            set_log(upgraded ? "Played Photon Cut+!"sv : "Played Photon Cut!"sv);
            break;
        }
        case CardId::GLIMMER: {
            draw_cards(base_value);
            ask_choice(CardChoice{ ChoiceKind::TOP_OF_DRAW, 1 });
            set_log(upgraded ? "Played Glimmer+!"sv : "Played Glimmer!"sv);
            break;
        }
        case CardId::TYRANNY: {
            add_power(PowerId::TYRANNY, base_value);
            set_log(upgraded ? "Played Tyranny+!"sv : "Played Tyranny!"sv);
            break;
        }
        case CardId::SCULPTING_STRIKE: {
            hit(current_target, base_value);
            ask_choice(CardChoice{ ChoiceKind::MAKE_ETHEREAL, 1 });
            set_log(upgraded ? "Played Sculpting Strike+!"sv : "Played Sculpting Strike!"sv);
            break;
        }
        case CardId::JUGGLING: {
            add_power(PowerId::JUGGLING);
            set_log(upgraded ? "Played Juggling+!"sv : "Played Juggling!"sv);
            break;
        }
        case CardId::EXPOSE: {
            current_target.clear_block();
            current_target.add_status(StatusType::ARTIFACT, static_cast<int16_t>(-current_target.get_status(StatusType::ARTIFACT)));
            afflict(current_target, StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Expose+!"sv : "Played Expose!"sv);
            break;
        }
        case CardId::HEADBUTT: {
            hit(current_target, base_value);
            ask_pile_choice(Pile::DISCARD, ChoiceKind::TOP_OF_DRAW, 1);
            set_log(upgraded ? "Played Headbutt+!"sv : "Played Headbutt!"sv);
            break;
        }
        case CardId::WISH: {
            ask_pile_choice(Pile::DRAW, ChoiceKind::TO_HAND, 1);
            set_log(upgraded ? "Played Wish+!"sv : "Played Wish!"sv);
            break;
        }
        case CardId::HOLOGRAM: {
            gain_block(base_value);
            ask_pile_choice(Pile::DISCARD, ChoiceKind::TO_HAND, 1);
            set_log(upgraded ? "Played Hologram+!"sv : "Played Hologram!"sv);
            break;
        }
        case CardId::COSMIC_INDIFFERENCE: {
            gain_block(base_value);
            ask_pile_choice(Pile::DISCARD, ChoiceKind::TOP_OF_DRAW, 1);
            set_log(upgraded ? "Played Cosmic Indifference+!"sv : "Played Cosmic Indifference!"sv);
            break;
        }
        case CardId::FOREGONE_CONCLUSION: {
            schedule(DelayedType::PICK_FROM_DRAW, base_value);
            set_log(upgraded ? "Played Foregone Conclusion+!"sv : "Played Foregone Conclusion!"sv);
            break;
        }
        case CardId::GRAVEBLAST: {
            hit(current_target, base_value);
            ask_pile_choice(Pile::DISCARD, ChoiceKind::TO_HAND, 1);
            set_log(upgraded ? "Played Graveblast+!"sv : "Played Graveblast!"sv);
            break;
        }
        case CardId::DREDGE: {
            ask_pile_choice(Pile::DISCARD, ChoiceKind::TO_HAND, base_value);
            set_log(upgraded ? "Played Dredge+!"sv : "Played Dredge!"sv);
            break;
        }
        case CardId::CLEANSE: {
            player.summon_minion(base_value, 0);
            ask_pile_choice(Pile::DRAW, ChoiceKind::EXHAUST, 1);
            set_log(upgraded ? "Played Cleanse+!"sv : "Played Cleanse!"sv);
            break;
        }
        case CardId::STONE_ARMOR: {
            player.add_status(StatusType::PLATED_ARMOR, base_value);
            set_log(upgraded ? "Played Stone Armor+!"sv : "Played Stone Armor!"sv);
            break;
        }
        case CardId::NEUTRON_AEGIS: {
            player.add_status(StatusType::PLATED_ARMOR, base_value);
            set_log(upgraded ? "Played Neutron Aegis+!"sv : "Played Neutron Aegis!"sv);
            break;
        }
        case CardId::FLAME_BARRIER: {
            gain_block(base_value);
            add_power(PowerId::FLAME_BARRIER, secondary_value);
            set_log(upgraded ? "Played Flame Barrier+!"sv : "Played Flame Barrier!"sv);
            break;
        }
        case CardId::REFLECT: {
            gain_block(base_value);
            add_power(PowerId::REFLECT);
            set_log(upgraded ? "Played Reflect+!"sv : "Played Reflect!"sv);
            break;
        }
        case CardId::BUFFER: {
            player.add_status(StatusType::BUFFER, base_value);
            set_log(upgraded ? "Played Buffer+!"sv : "Played Buffer!"sv);
            break;
        }
        case CardId::APPARITION: {
            player.add_status(StatusType::INTANGIBLE, base_value);
            set_log(upgraded ? "Played Apparition+!"sv : "Played Apparition!"sv);
            break;
        }
        case CardId::WRAITH_FORM: {
            player.add_status(StatusType::INTANGIBLE, base_value);
            add_power(PowerId::WRAITH_FORM, secondary_value);
            set_log(upgraded ? "Played Wraith Form+!"sv : "Played Wraith Form!"sv);
            break;
        }
        case CardId::PATTER: {
            gain_block(base_value);
            player.add_status(StatusType::VIGOR, secondary_value);
            set_log(upgraded ? "Played Patter+!"sv : "Played Patter!"sv);
            break;
        }
        case CardId::TERRAFORMING: {
            player.add_status(StatusType::VIGOR, base_value);
            set_log(upgraded ? "Played Terraforming+!"sv : "Played Terraforming!"sv);
            break;
        }
        case CardId::SOVEREIGN_BLADE: {
            const int16_t damage = static_cast<int16_t>(base_value + played_card.bonus);
            // Seeking Edge: every enemy instead of the one aimed at.
            const bool all = has_power(PowerId::SEEKING_EDGE);
            for (size_t i = 0; i < enemies.size(); ++i) {
                Enemy& foe = enemies[i];
                if (!foe.is_alive() || (!all && &foe != &current_target)) continue;
                bool doubled = false;
                for (const ActivePower& power : powers) {
                    if (power.id == PowerId::CONQUEROR && power.target == static_cast<int8_t>(i)) doubled = true;
                }
                // Conqueror doubles the damage once Strength and Vigor are added: hit() adds them once, so
                // they are added here once more.
                const int16_t dealt = doubled
                    ? static_cast<int16_t>(2 * damage + player.get_status(StatusType::STRENGTH) + player.get_status(StatusType::VIGOR))
                    : damage;
                hit(foe, dealt);
            }
            set_log(upgraded ? "Played Sovereign Blade+!"sv : "Played Sovereign Blade!"sv);
            break;
        }
        case CardId::REFINE_BLADE: {
            forge(base_value);
            schedule(DelayedType::ENERGY, secondary_value);
            set_log(upgraded ? "Played Refine Blade+!"sv : "Played Refine Blade!"sv);
            break;
        }
        case CardId::SPOILS_OF_BATTLE: {
            forge(base_value);
            draw_cards(secondary_value);
            set_log(upgraded ? "Played Spoils of Battle+!"sv : "Played Spoils of Battle!"sv);
            break;
        }
        case CardId::WROUGHT_IN_WAR: {
            hit(current_target, base_value);
            forge(secondary_value);
            set_log(upgraded ? "Played Wrought in War+!"sv : "Played Wrought in War!"sv);
            break;
        }
        case CardId::BULWARK: {
            gain_block(base_value);
            forge(secondary_value);
            set_log(upgraded ? "Played Bulwark+!"sv : "Played Bulwark!"sv);
            break;
        }
        case CardId::CONQUEROR: {
            forge(base_value);
            // Bound to the enemy and gone at the end of the turn; a second one on the same enemy adds nothing.
            if (target_idx >= 0 && target_idx < static_cast<int>(enemies.size())) {
                add_power(PowerId::CONQUEROR, 1, true, static_cast<int8_t>(target_idx));
            }
            set_log(upgraded ? "Played Conqueror+!"sv : "Played Conqueror!"sv);
            break;
        }
        case CardId::FURNACE: {
            add_power(PowerId::FURNACE, base_value);
            set_log(upgraded ? "Played Furnace+!"sv : "Played Furnace!"sv);
            break;
        }
        case CardId::PARRY: {
            add_power(PowerId::PARRY, base_value);
            set_log(upgraded ? "Played Parry+!"sv : "Played Parry!"sv);
            break;
        }
        case CardId::SUMMON_FORTH: {
            // From anywhere: the draw and discard piles, and the exhaust pile.
            recall_to_hand(CardId::SOVEREIGN_BLADE);
            for (size_t i = exhaust_pile.size(); i-- > 0 && hand.size() < hand.capacity();) {
                if (exhaust_pile[i].id == CardId::SOVEREIGN_BLADE) {
                    hand.push_back(exhaust_pile[i]);
                    exhaust_pile.erase(i);
                }
            }
            forge(base_value);
            set_log(upgraded ? "Played Summon Forth+!"sv : "Played Summon Forth!"sv);
            break;
        }
        case CardId::BEAT_INTO_SHAPE: {
            // The hits the enemy took this turn before this one, whatever card dealt them.
            const bool aimed = target_idx >= 0 && target_idx < 3;
            const int earlier_hits = aimed ? counters.hits_on_enemy[target_idx] : 0;
            hit(current_target, base_value);
            forge(secondary_value * (1 + earlier_hits));
            set_log(upgraded ? "Played Beat Into Shape+!"sv : "Played Beat Into Shape!"sv);
            break;
        }
        case CardId::BIG_BANG: {
            draw_cards(secondary_value);
            player.energy = static_cast<int8_t>(player.energy + secondary_value);
            gain_stars(secondary_value);
            forge(base_value);
            set_log(upgraded ? "Played Big Bang+!"sv : "Played Big Bang!"sv);
            break;
        }
        case CardId::SEEKING_EDGE: {
            forge(base_value);
            add_power(PowerId::SEEKING_EDGE);
            set_log(upgraded ? "Played Seeking Edge+!"sv : "Played Seeking Edge!"sv);
            break;
        }
        case CardId::SWORD_SAGE: {
            add_power(PowerId::SWORD_SAGE, base_value);
            set_log(upgraded ? "Played Sword Sage+!"sv : "Played Sword Sage!"sv);
            break;
        }
        case CardId::THE_SMITH: {
            forge(base_value);
            set_log(upgraded ? "Played The Smith+!"sv : "Played The Smith!"sv);
            break;
        }
        case CardId::POKE: {
            osty_attack(current_target, base_value);
            set_log(upgraded ? "Played Poke+!"sv : "Played Poke!"sv);
            break;
        }
        case CardId::SNAP: {
            osty_attack(current_target, base_value);
            ask_choice(CardChoice{ ChoiceKind::MAKE_RETAIN, 1 });
            set_log(upgraded ? "Played Snap+!"sv : "Played Snap!"sv);
            break;
        }
        case CardId::RATTLE: {
            // One attack that hits once more for each attack Osty made earlier this turn.
            osty_attack(current_target, base_value, 1 + counters.osty_attacks);
            set_log(upgraded ? "Played Rattle+!"sv : "Played Rattle!"sv);
            break;
        }
        case CardId::BONE_SHARDS: {
            // Nothing happens when Osty is dead.
            if (player.minion_alive()) {
                osty_attack_all(base_value);
                gain_block(secondary_value);
                kill_osty();
            }
            set_log(upgraded ? "Played Bone Shards+!"sv : "Played Bone Shards!"sv);
            break;
        }
        case CardId::SQUEEZE: {
            // The other Osty Attacks of the hand and of the draw and discard piles: this one already left the hand.
            const int others = count_osty_attacks(hand) + count_osty_attacks(draw_pile) + count_osty_attacks(discard_pile);
            osty_attack(current_target, static_cast<int16_t>(base_value + secondary_value * others));
            set_log(upgraded ? "Played Squeeze+!"sv : "Played Squeeze!"sv);
            break;
        }
        case CardId::FLATTEN: {
            osty_attack(current_target, base_value);
            set_log(upgraded ? "Played Flatten+!"sv : "Played Flatten!"sv);
            break;
        }
        case CardId::HIGH_FIVE: {
            if (player.minion_alive()) {
                osty_attack_all(base_value);
                for (Enemy& foe : enemies) {
                    if (foe.is_alive()) afflict(foe, StatusType::VULNERABLE, secondary_value);
                }
            }
            set_log(upgraded ? "Played High Five+!"sv : "Played High Five!"sv);
            break;
        }
        case CardId::FETCH: {
            osty_attack(current_target, base_value);
            // The first Fetch played this turn draws.
            if (counters.fetches_played == 0) draw_cards(secondary_value);
            counters.fetches_played = bump(counters.fetches_played);
            set_log(upgraded ? "Played Fetch+!"sv : "Played Fetch!"sv);
            break;
        }
        case CardId::CALCIFY: {
            add_power(PowerId::CALCIFY, base_value);
            set_log(upgraded ? "Played Calcify+!"sv : "Played Calcify!"sv);
            break;
        }
        case CardId::RIGHT_HAND_HAND: {
            // Its return from the discard pile is in run_card.
            osty_attack(current_target, base_value);
            set_log(upgraded ? "Played Right Hand Hand+!"sv : "Played Right Hand Hand!"sv);
            break;
        }
        case CardId::NECRO_MASTERY: {
            player.summon_minion(base_value, 0);
            // What the enemies lose is what Osty lost: a second Necro Mastery adds nothing.
            add_power(PowerId::NECRO_MASTERY);
            set_log(upgraded ? "Played Necro Mastery+!"sv : "Played Necro Mastery!"sv);
            break;
        }
        case CardId::PROTECTOR: {
            osty_attack(current_target, static_cast<int16_t>(base_value + player.minion_max_hp));
            set_log(upgraded ? "Played Protector+!"sv : "Played Protector!"sv);
            break;
        }
        case CardId::SACRIFICE: {
            if (player.minion_alive()) {
                const int block = std::min(3 * player.minion_max_hp, 999);
                kill_osty();
                gain_block(static_cast<int16_t>(block));
            }
            set_log(upgraded ? "Played Sacrifice+!"sv : "Played Sacrifice!"sv);
            break;
        }
        case CardId::SPUR: {
            player.summon_minion(base_value, 0);
            heal_osty(secondary_value);
            set_log(upgraded ? "Played Spur+!"sv : "Played Spur!"sv);
            break;
        }
        case CardId::SIC_EM: {
            osty_attack(current_target, base_value);
            // Bound to the enemy and gone at the end of the turn: the hit of this card came before it.
            if (player.minion_alive() && current_target.is_alive() && target_idx >= 0 &&
                target_idx < static_cast<int>(enemies.size())) {
                add_power(PowerId::SIC_EM, secondary_value, true, static_cast<int8_t>(target_idx));
            }
            set_log(upgraded ? "Played Sic 'Em+!"sv : "Played Sic 'Em!"sv);
            break;
        }
        case CardId::SENTRY_MODE: {
            add_power(PowerId::SENTRY_MODE, base_value);
            set_log(upgraded ? "Played Sentry Mode+!"sv : "Played Sentry Mode!"sv);
            break;
        }
        case CardId::SWEEPING_GAZE: {
            if (has_alive_enemies()) osty_attack(enemies[random_alive_enemy_index()], base_value);
            set_log(upgraded ? "Played Sweeping Gaze+!"sv : "Played Sweeping Gaze!"sv);
            break;
        }
        case CardId::GIANT_ROCK: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Giant Rock+!"sv : "Played Giant Rock!"sv);
            break;
        }
        case CardId::SOUL: {
            draw_cards(base_value);
            set_log(upgraded ? "Played Soul+!"sv : "Played Soul!"sv);
            break;
        }
        case CardId::REAVE: {
            hit(current_target, base_value);
            // Reave+ adds a Soul+.
            add_souls(secondary_value, Pile::DRAW, upgraded);
            set_log(upgraded ? "Played Reave+!"sv : "Played Reave!"sv);
            break;
        }
        case CardId::GRAVE_WARDEN: {
            gain_block(base_value);
            add_souls(secondary_value);
            set_log(upgraded ? "Played Grave Warden+!"sv : "Played Grave Warden!"sv);
            break;
        }
        case CardId::CAPTURE_SPIRIT: {
            // HP loss: the enemy's Block does not absorb it.
            current_target.lose_hp(base_value);
            add_souls(secondary_value);
            set_log(upgraded ? "Played Capture Spirit+!"sv : "Played Capture Spirit!"sv);
            break;
        }
        case CardId::SEVERANCE: {
            hit(current_target, base_value);
            add_souls(secondary_value, Pile::DRAW);
            add_souls(secondary_value, Pile::HAND);
            add_souls(secondary_value, Pile::DISCARD);
            set_log(upgraded ? "Played Severance+!"sv : "Played Severance!"sv);
            break;
        }
        case CardId::DIRGE: {
            // X separate Summons, then X Souls (Souls+ once upgraded).
            for (int round = 0; round < x_value; ++round) player.summon_minion(base_value, 0);
            add_souls(x_value, Pile::DRAW, upgraded);
            set_log(upgraded ? "Played Dirge+!"sv : "Played Dirge!"sv);
            break;
        }
        case CardId::HAUNT: {
            add_power(PowerId::HAUNT, base_value);
            set_log(upgraded ? "Played Haunt+!"sv : "Played Haunt!"sv);
            break;
        }
        case CardId::DEVOUR_LIFE: {
            add_power(PowerId::DEVOUR_LIFE, base_value);
            set_log(upgraded ? "Played Devour Life+!"sv : "Played Devour Life!"sv);
            break;
        }
        case CardId::SOUL_STORM: {
            hit(current_target, static_cast<int16_t>(base_value + secondary_value * exhausted_souls()));
            set_log(upgraded ? "Played Soul Storm+!"sv : "Played Soul Storm!"sv);
            break;
        }
        case CardId::GLIMPSE_BEYOND: {
            // "ALL players": there is one player.
            add_souls(base_value);
            set_log(upgraded ? "Played Glimpse Beyond+!"sv : "Played Glimpse Beyond!"sv);
            break;
        }
        case CardId::MANIFEST_AUTHORITY: {
            gain_block(base_value);
            for (uint8_t i = 0; i < secondary_value; ++i) {
                create_random_card(CardPool::COLORLESS, Pile::HAND, upgraded);
            }
            set_log(upgraded ? "Played Manifest Authority+!"sv : "Played Manifest Authority!"sv);
            break;
        }
        default:
            set_log("Played card!");
            break;
    }
}

void CombatEngine::trigger_end_of_turn_in_hand(const DeckCard& card) {
    switch (card.id) {
        case CardId::BURN: damage_player(CardDef::get_value(card.id, card.upgraded)); break;
        case CardId::DOUBT: player.add_status(StatusType::WEAK, 1); break;
        case CardId::SHAME: player.add_status(StatusType::FRAIL, 1); break;
        case CardId::DECAY: damage_player(2); break;
        case CardId::REGRET: lose_hp(static_cast<int16_t>(hand.size())); break;
        case CardId::BAD_LUCK: lose_hp(13); break;
        case CardId::DEBT: lose_gold(10); break;
        default: break;
    }
}

void CombatEngine::end_player_turn() {
    vakuu_turn_active = false;
    if (state != CombatState::PLAYER_TURN) return;
    double_block_turn = false;
    attack_double_now = false;
    // A choice left unanswered is answered for the player.
    while (choice_pending()) resolve_choice_default();
    if (state != CombatState::PLAYER_TURN) return;
    end_turn_requested = false;

    emit(CombatEvent::TURN_END);

    // Cards that play themselves at the end of the turn: Howl from Beyond from the exhaust pile (it stays
    // there), I Am Invincible from the top of the draw pile.
    for (size_t i = 0; i < exhaust_pile.size() && has_alive_enemies(); ++i) {
        if (exhaust_pile[i].id == CardId::HOWL_FROM_BEYOND) autoplay(exhaust_pile[i], -1, AfterPlay::NOWHERE);
    }
    if (!draw_pile.empty() && draw_pile.back().id == CardId::I_AM_INVINCIBLE) {
        DeckCard top = draw_pile.back();
        draw_pile.pop_back();
        autoplay(top);
    }
    // So are the choices of the cards that just played themselves.
    while (choice_pending()) resolve_choice_default();
    if (state != CombatState::PLAYER_TURN) return;
    for (DeckCard& card : hand) {
        card.free_this_turn = false;
        card.cost_one_this_turn = false;
    }
    // "Until the end of the turn" Sly goes in every pile (a card discarded this turn keeps its mark otherwise).
    for (DeckCard& card : hand) card.sly_this_turn = false;
    for (DeckCard& card : draw_pile) card.sly_this_turn = false;
    for (DeckCard& card : discard_pile) card.sly_this_turn = false;

    // Pael's Eye: the first turn of the combat ended without a card played exhausts the hand and gives an
    // extra turn, the enemies do not act in between.
    bool extra_turn = extra_turn_pending;
    extra_turn_pending = false;
    if (counters.cards_played() == 0 && power_amount(PowerId::PAELS_EYE) > 0) {
        for (size_t i = powers.size(); i-- > 0;) {
            if (powers[i].id == PowerId::PAELS_EYE) powers.erase(i);
        }
        extra_turn = true;
        while (!hand.empty()) exhaust_from_hand(hand.size() - 1);
    }

    // Cards that hurt while they are still in hand (Burn, curses).
    for (size_t i = 0; i < hand.size() && player.is_alive(); ++i) trigger_end_of_turn_in_hand(hand[i]);
    if (!player.is_alive()) {
        state = CombatState::DEFEAT;
        set_log("You were defeated!");
        return;
    }

    // Ethereal cards left in hand are exhausted first: what their exhaustion draws is discarded with the rest.
    // Hex held by a living enemy makes every card Ethereal.
    bool hexed = false;
    for (const ActivePower& held : powers) {
        if (held.id == PowerId::HEX && held.holder >= 0 && static_cast<size_t>(held.holder) < enemies.size() &&
            enemies[held.holder].is_alive()) hexed = true;
    }
    for (size_t i = hand.size(); i-- > 0;) {
        if (i < hand.size() && (hexed || CardDef::has_keyword(hand[i], KEYWORD_ETHEREAL))) exhaust_from_hand(i);
    }
    // The rest is discarded, except the cards with Retain.
    const bool keep_hand = retain_hand || has_power(PowerId::KEEP_HAND) || has_power(PowerId::STABLE_SERUM);
    for (size_t i = hand.size(); i-- > 0;) {
        // Phantom Blades keeps the Shivs.
        const bool kept_shiv = hand[i].id == CardId::SHIV && has_power(PowerId::PHANTOM_BLADES);
        if (keep_hand || kept_shiv || CardDef::has_keyword(hand[i], KEYWORD_RETAIN) ||
            CardDef::has_keyword(hand[i], KEYWORD_RETAIN_ONCE)) {
            hand[i].added_keywords &= static_cast<uint8_t>(~KEYWORD_RETAIN_ONCE);
            continue;
        }
        discard_pile.push_back(hand[i]);
        hand.erase(i);
    }
    // Stable Serum: one of its turns is spent.
    for (size_t i = powers.size(); i-- > 0;) {
        if (powers[i].id != PowerId::STABLE_SERUM) continue;
        if (--powers[i].amount <= 0) powers.erase(i);
    }

    // Bookmark: a random card kept in hand costs 1 less until it is played.
    if (has_power(PowerId::BOOKMARK) && !hand.empty()) {
        DeckCard& kept = hand[next_random() % hand.size()];
        const uint8_t printed = CardDef::get_cost(kept.id, kept.upgraded);
        if (printed != COST_X && effective_cost(kept) > 0) ++kept.bookmark_discount;
    }

    // Effects that only lasted this turn are over.
    for (size_t i = powers.size(); i-- > 0;) {
        if (powers[i].turn_only) powers.erase(i);
    }

    // Strength and Dexterity gained "until the end of the turn" go away.
    player.add_status(StatusType::STRENGTH, static_cast<int16_t>(-temp_strength));
    player.add_status(StatusType::DEXTERITY, static_cast<int16_t>(-temp_dexterity));
    temp_strength = 0;
    temp_dexterity = 0;

    // End of turn effects: countdowns, Metallicize and Plating block, Regen, then orb passives on the first
    // living enemy.
    run_delayed_effects(true);
    int16_t block_before = player.block;
    player.apply_metallicize_tick();
    player.apply_plating_tick();
    if (player.block > block_before) {
        EventInfo gained;
        gained.amount = static_cast<int16_t>(player.block - block_before);
        emit(CombatEvent::BLOCK_GAINED, gained);
    }
    player.apply_regen_tick();
    trigger_turn_passives(false);
    // Focus gained "this turn" still counts for the passives above.
    player.add_status(StatusType::FOCUS, static_cast<int16_t>(-temp_focus));
    temp_focus = 0;
    settle_deaths();

    // Doom takes the player at the end of the turn, like any enemy.
    int16_t doom = player.get_status(StatusType::DOOM);
    if (doom > 0 && doom >= player.hp) {
        player.hp = 0;
        state = CombatState::DEFEAT;
        set_log("You were defeated!");
        return;
    }
    if (!enemies.empty()) {
        enemy = enemies[0];
    }
    if (!has_alive_enemies()) {
        state = CombatState::VICTORY;
        set_log("Victory!");
        return;
    }

    if (extra_turn) {
        start_player_turn();
        set_log("Extra turn!");
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
        for (int16_t tick = power_amount(PowerId::ACCELERANT); tick >= 0 && enemies[i].is_alive(); --tick) {
            enemies[i].apply_poison_tick();
        }
        settle_deaths();
        if (enemies[i].is_alive()) {
            EnemyActionRecord record;
            record.enemy_index = static_cast<int8_t>(i);
            record.intent_type = enemies[i].current_intent.type;
            record.debuff_attempted = enemies[i].current_intent.debuffs_player();
            const int16_t hp_before = player.hp;
            const int16_t shield_before = player.block + player.minion_hp + player.minion_block;
            const int16_t osty_hp_before = player.minion_hp;

            EventInfo acting;
            acting.enemy = static_cast<int8_t>(i);
            emit(CombatEvent::ENEMY_TURN_START, acting);
            const EnemyIntent played = enemies[i].current_intent;
            // Undying Sigil: an enemy with at least as much Doom as HP deals 50% less damage.
            const bool sigil_halved = has_power(PowerId::UNDYING_SIGIL) && enemies[i].get_status(StatusType::DOOM) >= enemies[i].hp;
            enemies[i].execute_turn(player, power_amount(PowerId::FLAME_BARRIER), has_power(PowerId::REFLECT), sigil_halved);
            // The power of the move: kept by the enemy, or put on a player the attack left alive.
            if (played.power != PowerId::NONE && player.is_alive()) {
                if (!played.power_on_player) {
                    add_enemy_power(static_cast<int>(i), played.power, played.power_amount);
                } else if (afflict_player(played.power, played.power_amount, played.power_turn_only)) {
                    record.power = played.power;
                }
            }
            // The cards of the move, for a player still standing. A full pile takes no more (a full hand sends
            // the card to the discard pile).
            if (played.gives_cards() && player.is_alive()) {
                const size_t before = draw_pile.size() + hand.size() + discard_pile.size();
                for (uint8_t n = 0; n < played.status_cards; ++n) {
                    create_card(played.status_card, played.status_pile, false);
                }
                record.cards_added = played.status_card;
                record.cards_added_count =
                    static_cast<uint8_t>(draw_pile.size() + hand.size() + discard_pile.size() - before);
            }
            // The Rock Bowlbug is thrown off balance by an attack the player's Block took in full.
            if (enemies[i].type == EnemyType::BOWLBUG_ROCK && played.type == EnemyIntentType::ATTACK &&
                enemies[i].is_alive() && hp_before == player.hp) {
                enemies[i].phase = 1;
            }
            record.hits = played.hits;
            record.debuff_attempted = record.debuff_attempted || (played.power_on_player && played.power != PowerId::NONE);
            note_osty_hp_lost(static_cast<int16_t>(osty_hp_before - player.minion_hp));
            settle_deaths(); // Thorns and damage sent back can kill the attacker

            // Doom: an enemy with at least as much Doom as HP does not survive its turn.
            reap(enemies[i]);

            // Demise: enemy loses HP at the end of each of its turns.
            for (const ActivePower& p : powers) {
                if (p.id == PowerId::DEMISE && p.target == static_cast<int8_t>(i) && enemies[i].is_alive()) {
                    enemies[i].lose_hp(p.amount);
                    settle_deaths();
                }
            }

            // The Strength it lost "until the end of its turn" comes back.
            afflict(enemies[i], StatusType::STRENGTH, enemies[i].temp_strength_loss);
            enemies[i].temp_strength_loss = 0;

            record.damage_to_player = hp_before - player.hp;
            // The Gambit: one unblocked point of attack damage and the hero is dead.
            if (gambit_active && record.damage_to_player > 0 && enemies[i].current_intent.base_damage > 0) player.hp = 0;
            note_player_hp_lost(record.damage_to_player);
            record.damage_blocked = shield_before - (player.block + player.minion_hp + player.minion_block);
            record.block_gained = enemies[i].block; // execute_turn() clears block before gaining it
            record.player_hp_after = player.hp;
            record.healed = enemies[i].last_heal;
            record.escaped = enemies[i].escaped;
            for (size_t slot = 0; slot < MAX_INTENT_DEBUFFS; ++slot) record.debuffs[slot] = enemies[i].last_debuffs[slot];
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
    plan_enemies();

    // Start player turn again
    start_player_turn();
    set_log("Your Turn!");
}

bool CombatEngine::use_potion(std::string_view potion_id, int target_index) {
    if (state != CombatState::PLAYER_TURN) return false;

    // The enemy a potion is thrown at: the first living one when the target given is not a living enemy.
    auto aimed_enemy = [this, &target_index]() -> Enemy& {
        if (target_index < 0 || target_index >= static_cast<int>(enemies.size()) || !enemies[target_index].is_alive()) {
            target_index = get_first_alive_enemy_index();
        }
        return enemies[target_index];
    };

    if (potion_id == "FIRE_POTION") {
        if (target_index < 0 || target_index >= static_cast<int>(enemies.size()) || !enemies[target_index].is_alive()) {
            target_index = get_first_alive_enemy_index();
        }
        damage_enemy(enemies[target_index], 20);
        set_log("Fire Potion!");
    } else if (potion_id == "FAIRY_IN_A_BOTTLE") {
        // Automatic: drunk by hand, it is simply made ready to trigger when HP would reach 0.
        fairy_bottles = static_cast<uint8_t>(fairy_bottles + 1);
        set_log("Fairy in a Bottle!");
    } else if (potion_id == "POTION_SHAPED_ROCK") {
        damage_enemy(aimed_enemy(), 15);
        set_log("Potion-Shaped Rock!");
    } else if (potion_id == "GIGANTIFICATION_POTION") {
        triple_next_attack = true;
        set_log("Gigantification Potion!");
    } else if (potion_id == "BLOCK_POTION") {
        add_block(12); // Potion Block is not modified by Dexterity
        set_log("Block Potion!");
    } else if (potion_id == "ENERGY_POTION") {
        player.energy = static_cast<int8_t>(player.energy + 2);
        set_log("Energy Potion!");
    } else if (potion_id == "BLOOD_POTION") {
        player.heal(player.max_hp / 5);
        set_log("Blood Potion!");
    } else if (potion_id == "FRUIT_JUICE") {
        // Max HP and current HP both rise by 5, as Feed does.
        player.max_hp = static_cast<int16_t>(player.max_hp + 5);
        player.hp = static_cast<int16_t>(player.hp + 5);
        set_log("Fruit Juice!");
    } else if (potion_id == "CUNNING_POTION") {
        add_shivs(3, true);
        set_log("Cunning Potion!");
    } else if (potion_id == "EXPLOSIVE_AMPOULE") {
        damage_all_enemies(10);
        set_log("Explosive Ampoule!");
    } else if (potion_id == "WEAK_POTION") {
        afflict(aimed_enemy(), StatusType::WEAK, 3);
        set_log("Weak Potion!");
    } else if (potion_id == "SWIFT_POTION") {
        draw_cards(3);
        set_log("Swift Potion!");
    } else if (potion_id == "DEXTERITY_POTION") {
        player.add_status(StatusType::DEXTERITY, 2);
        set_log("Dexterity Potion!");
    } else if (potion_id == "STRENGTH_POTION") {
        player.add_status(StatusType::STRENGTH, 2);
        set_log("Strength Potion!");
    } else if (potion_id == "POISON_POTION") {
        afflict(aimed_enemy(), StatusType::POISON, 6);
        set_log("Poison Potion!");
    } else if (potion_id == "VULNERABLE_POTION") {
        afflict(aimed_enemy(), StatusType::VULNERABLE, 3);
        set_log("Vulnerable Potion!");
    } else if (potion_id == "STAR_POTION") {
        gain_stars(3);
        set_log("Star Potion!");
    } else if (potion_id == "FOCUS_POTION") {
        player.add_status(StatusType::FOCUS, 2);
        set_log("Focus Potion!");
    } else if (potion_id == "FLEX_POTION") {
        gain_temporary(StatusType::STRENGTH, 5); // lost at the end of the turn
        set_log("Flex Potion!");
    } else if (potion_id == "SPEED_POTION") {
        gain_temporary(StatusType::DEXTERITY, 5); // lost at the end of the turn
        set_log("Speed Potion!");
    } else if (potion_id == "POTION_OF_DOOM") {
        afflict(aimed_enemy(), StatusType::DOOM, 33);
        set_log("Potion of Doom!");
    } else if (potion_id == "BLESSING_OF_THE_FORGE") {
        for (DeckCard& held : hand) {
            if (can_be_picked(held, ChoiceKind::UPGRADE)) held.upgraded = true;
        }
        set_log("Blessing of the Forge!");
    } else if (potion_id == "FORTIFIER") {
        add_block(static_cast<int16_t>(player.block * 2)); // tripled: twice what is there is added
        set_log("Fortifier!");
    } else if (potion_id == "FYSH_OIL") {
        player.add_status(StatusType::STRENGTH, 1);
        player.add_status(StatusType::DEXTERITY, 1);
        set_log("Fysh Oil!");
    } else if (potion_id == "GAMBLERS_BREW") {
        ask_choice(CardChoice{ ChoiceKind::DISCARD, 10, true, ChoiceThen::DRAW_PER_CARD });
        set_log("Gambler's Brew!");
    } else if (potion_id == "BONE_BREW") {
        player.summon_minion(15, 0);
        set_log("Bone Brew!");
    } else if (potion_id == "POTION_OF_BINDING") {
        for (Enemy& foe : enemies) {
            if (!foe.is_alive()) continue;
            afflict(foe, StatusType::WEAK, 1);
            afflict(foe, StatusType::VULNERABLE, 1);
        }
        set_log("Potion of Binding!");
    } else if (potion_id == "CURE_ALL") {
        player.energy = static_cast<int8_t>(player.energy + 1);
        draw_cards(2);
        set_log("Cure All!");
    } else if (potion_id == "CLARITY") {
        draw_cards(1);
        schedule(DelayedType::DRAW, 1, 1, 3);
        set_log("Clarity!");
    } else if (potion_id == "RADIANT_TINCTURE") {
        player.energy = static_cast<int8_t>(player.energy + 1);
        schedule(DelayedType::ENERGY, 1, 1, 3);
        set_log("Radiant Tincture!");
    } else if (potion_id == "POTION_OF_CAPACITY") {
        change_orb_slots(2);
        set_log("Potion of Capacity!");
    } else if (potion_id == "MAZALETHS_GIFT") {
        // Ritual 1: the Strength it gives at the end of a turn is first used on the next one, which is when
        // TURN_STRENGTH gives it.
        add_power(PowerId::TURN_STRENGTH, 1);
        set_log("Mazaleth's Gift!");
    } else if (potion_id == "ESSENCE_OF_DARKNESS") {
        for (uint8_t slot = 0; slot < player.orb_slots; ++slot) channel_orb(OrbType::DARK);
        set_log("Essence of Darkness!");
    } else if (potion_id == "BOTTLED_POTENTIAL") {
        // The hand joins the draw pile, then the discard pile is shuffled into it. Exhausted cards stay out.
        while (!hand.empty() && draw_pile.push_back(hand.back())) hand.pop_back();
        shuffle_discard_into_draw();
        draw_cards(5);
        set_log("Bottled Potential!");
    } else if (potion_id == "DISTILLED_CHAOS") {
        for (int round = 3; round > 0; --round) {
            if (draw_pile.empty()) shuffle_discard_into_draw();
            if (draw_pile.empty()) break;
            DeckCard top = draw_pile.back();
            draw_pile.pop_back();
            autoplay(top, -1, AfterPlay::DISCARD);
        }
        settle_deaths();
        set_log("Distilled Chaos!");
    } else if (potion_id == "SHIP_IN_A_BOTTLE") {
        add_block(10);
        schedule(DelayedType::BLOCK, 10);
        set_log("Ship in a Bottle!");
    } else if (potion_id == "SHACKLING_POTION") {
        for (Enemy& foe : enemies) {
            if (foe.is_alive()) sap_strength(foe, 7);
        }
        set_log("Shackling Potion!");
    } else if (potion_id == "GLOWWATER_POTION") {
        // Only the cards held when the potion is drunk: what their exhaustion draws comes after them and stays.
        for (size_t held = hand.size(); held > 0 && !hand.empty(); --held) exhaust_from_hand(0);
        draw_cards(10);
        set_log("Glowwater Potion!");
    } else if (potion_id == "HEART_OF_IRON") {
        player.add_status(StatusType::PLATED_ARMOR, 7);
        set_log("Heart of Iron!");
    } else if (potion_id == "LIQUID_BRONZE") {
        player.add_status(StatusType::THORNS, 3);
        set_log("Liquid Bronze!");
    } else if (potion_id == "GHOST_IN_A_JAR") {
        player.add_status(StatusType::INTANGIBLE, 1);
        set_log("Ghost in a Jar!");
    } else if (potion_id == "LUCKY_TONIC") {
        player.add_status(StatusType::BUFFER, 1);
        set_log("Lucky Tonic!");
    } else if (potion_id == "KINGS_COURAGE") {
        forge(15);
        set_log("King's Courage!");
    } else if (potion_id == "POT_OF_GHOULS") {
        add_souls(2, Pile::HAND);
        set_log("Pot of Ghouls!");
    } else if (potion_id == "REGEN_POTION") {
        player.add_status(StatusType::REGEN, 5);
        set_log("Regen Potion!");
    } else if (potion_id == "POWDERED_DEMISE") {
        aimed_enemy();
        if (target_index >= 0 && static_cast<size_t>(target_index) < enemies.size()) {
            add_power(PowerId::DEMISE, 9, false, static_cast<int8_t>(target_index));
        }
        set_log("Powdered Demise!");
    } else if (potion_id == "DUPLICATOR") {
        add_power(PowerId::DUPLICATION, 1, true);
        set_log("Duplicator!");
    } else if (potion_id == "ASHWATER") {
        ask_choice(CardChoice{ ChoiceKind::EXHAUST, 10, true });
        set_log("Ashwater!");
    } else if (potion_id == "BEETLE_JUICE") {
        aimed_enemy().damage_down_turns = 4;
        set_log("Beetle Juice!");
    } else if (potion_id == "DROPLET_OF_PRECOGNITION") {
        ask_pile_choice(Pile::DRAW, ChoiceKind::TO_HAND, 1);
        set_log("Droplet of Precognition!");
    } else if (potion_id == "STABLE_SERUM") {
        add_power(PowerId::STABLE_SERUM, 2);
        set_log("Stable Serum!");
    } else if (potion_id == "TOUCH_OF_INSANITY") {
        ask_choice(CardChoice{ ChoiceKind::MAKE_FREE, 1 });
        set_log("Touch of Insanity!");
    } else if (potion_id == "LIQUID_MEMORIES") {
        CardChoice choice;
        choice.kind = ChoiceKind::TO_HAND;
        choice.source = Pile::DISCARD;
        choice.free_this_turn = true;
        ask_choice(choice);
        set_log("Liquid Memories!");
    } else if (potion_id == "ATTACK_POTION") {
        offer_random_cards(CardFilter(CardType::ATTACK), 3, false, true);
        set_log("Attack Potion!");
    } else if (potion_id == "SKILL_POTION") {
        offer_random_cards(CardFilter(CardType::SKILL), 3, false, true);
        set_log("Skill Potion!");
    } else if (potion_id == "POWER_POTION") {
        offer_random_cards(CardFilter(CardType::POWER), 3, false, true);
        set_log("Power Potion!");
    } else if (potion_id == "COLORLESS_POTION") {
        offer_random_cards(CardFilter(CardPool::COLORLESS), 3, false, true);
        set_log("Colorless Potion!");
    } else if (potion_id == "OROBIC_ACID") {
        for (CardType type : { CardType::ATTACK, CardType::SKILL, CardType::POWER }) {
            const CardId card = random_card(CardFilter(type));
            if (card == CardId::NONE) continue;
            DeckCard made(card);
            made.free_this_turn = true;
            create_card(made, Pile::HAND);
        }
        set_log("Orobic Acid!");
    } else if (potion_id == "AMBERGRIS") {
        player.heal(player.max_hp / 2);
        extra_turn_pending = true;
        set_log("Ambergris!");
    } else if (potion_id == "COSMIC_CONCOCTION") {
        for (int copy = 0; copy < 3; ++copy) create_random_card(CardFilter(CardPool::COLORLESS), Pile::HAND, true);
        set_log("Cosmic Concoction!");
    } else if (potion_id == "ENTROPIC_BREW") {
        fill_potion_belt = true; // the run owns the belt: RunManager::use_potion fills it once this potion is gone
        set_log("Entropic Brew!");
    } else if (potion_id == "FOUL_POTION") {
        // Damage to every player and enemy: Block absorbs it, Strength and Vulnerable do not change it.
        damage_all_enemies(12);
        damage_player(12);
        set_log("Foul Potion!");
    } else {
        return false;
    }

    // Reptile Trinket: a potion drunk gives Strength until the end of the turn.
    if (has_power(PowerId::REPTILE_TRINKET)) gain_temporary(StatusType::STRENGTH, power_amount(PowerId::REPTILE_TRINKET));

    // Keep primary enemy in sync
    if (!enemies.empty()) {
        enemy = enemies[0];
    }

    if (!player.is_alive()) {
        // A card played by Distilled Chaos can cost the player's last HP.
        state = CombatState::DEFEAT;
        set_log("You were defeated!");
    } else if (!has_alive_enemies()) {
        state = CombatState::VICTORY;
        set_log("Victory!");
    } else if (selected_target_index < 0 || selected_target_index >= static_cast<int>(enemies.size()) ||
               !enemies[selected_target_index].is_alive()) {
        selected_target_index = get_first_alive_enemy_index();
    }

    return true;
}

} // namespace engine
