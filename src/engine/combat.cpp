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

uint8_t bump(uint8_t value, int amount = 1) {
    return static_cast<uint8_t>(std::min(255, value + amount));
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
    counters.cards_drawn = bump(counters.cards_drawn);
    counters.combat_cards_drawn = bump(counters.combat_cards_drawn);
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

void CombatEngine::discard_from_hand(size_t index) {
    discard_pile.push_back(hand[index]);
    hand.erase(index);
    counters.cards_discarded = bump(counters.cards_discarded);
}

void CombatEngine::create_card_in_discard(DeckCard card) {
    discard_pile.push_back(card);
    counters.combat_cards_created = bump(counters.combat_cards_created);
}

int16_t CombatEngine::hit(Enemy& target, int16_t base) {
    counters.hits_dealt = bump(counters.hits_dealt);
    return target.take_damage(attack_damage(base, player, target));
}

void CombatEngine::note_player_hp_lost(int16_t amount) {
    if (amount <= 0) return;
    counters.hp_lost = static_cast<int16_t>(counters.hp_lost + amount);
    counters.combat_hp_losses = bump(counters.combat_hp_losses);
}

void CombatEngine::lose_hp(int16_t amount) {
    if (amount <= 0) return;
    int16_t lost = std::min<int16_t>(amount, player.hp);
    player.hp = static_cast<int16_t>(player.hp - lost);
    note_player_hp_lost(lost);
    if (!player.is_alive()) {
        state = CombatState::DEFEAT;
    }
}

void CombatEngine::gain_temporary(StatusType type, int8_t amount) {
    if (type == StatusType::STRENGTH) {
        temp_strength = static_cast<int8_t>(temp_strength + amount);
    } else if (type == StatusType::DEXTERITY) {
        temp_dexterity = static_cast<int8_t>(temp_dexterity + amount);
    } else {
        return;
    }
    player.add_status(type, amount);
}

void CombatEngine::sap_strength(Enemy& target, int16_t amount) {
    // Measured rather than assumed: the status list can be full.
    int16_t before = target.get_status(StatusType::STRENGTH);
    target.add_status(StatusType::STRENGTH, static_cast<int16_t>(-amount));
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
                player.block = static_cast<int16_t>(player.block + effect.amount);
                break;
            case DelayedType::DRAW:
                extra_draw += effect.amount;
                break;
            case DelayedType::STARS:
                player.add_stars(static_cast<uint8_t>(effect.amount));
                break;
            case DelayedType::SUMMON:
                player.summon_minion(effect.amount, 0);
                break;
            case DelayedType::CHANNEL_LIGHTNING:
                for (int16_t orb = 0; orb < effect.amount; ++orb) {
                    player.channel_orb(OrbType::LIGHTNING, has_alive_enemies() ? &enemies[get_first_alive_enemy_index()] : nullptr);
                }
                break;
            case DelayedType::DAMAGE_ALL:
                for (Enemy& foe : enemies) {
                    if (foe.is_alive()) foe.take_damage(effect.amount);
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

    open_first_turn();
    set_log("Combat started!");
}

void CombatEngine::open_first_turn() {
    counters = CombatCounters();
    delayed_effects.clear();
    returning_cards.clear();
    keep_block_turns = 0;
    retain_hand = false;
    temp_strength = 0;
    temp_dexterity = 0;

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
}

void CombatEngine::start_player_turn() {
    state = CombatState::PLAYER_TURN;
    int16_t hp_before = player.hp;
    player.apply_poison_tick();
    counters.start_turn();
    note_player_hp_lost(static_cast<int16_t>(hp_before - player.hp));
    if (!player.is_alive()) {
        state = CombatState::DEFEAT;
        set_log("You were defeated!");
        return;
    }
    int16_t kept_block = 0;
    if (keep_block_turns > 0) {
        --keep_block_turns;
        kept_block = player.block;
    }
    player.reset_turn();
    player.block = kept_block;
    exhausted_this_turn = 0;
    retain_hand = false;
    player.trigger_start_of_turn_orbs();
    int extra_draw = run_delayed_effects(false);

    // Cards played with Return come back before the draw.
    for (const DeckCard& card : returning_cards) {
        if (!hand.push_back(card)) discard_pile.push_back(card);
    }
    returning_cards.clear();

    draw_cards(5 + extra_draw);
    // Only the cards drawn by effects during the turn count as drawn this turn.
    counters.cards_drawn = 0;
    selected_card_index = 0;
    selected_target_index = get_first_alive_enemy_index();
}

bool CombatEngine::can_play_card(int index) const {
    if (state != CombatState::PLAYER_TURN) return false;
    if (index < 0 || static_cast<size_t>(index) >= hand.size()) return false;

    uint8_t cost = CardDef::get_cost(hand[index].id, hand[index].upgraded);
    return cost == COST_X || player.energy >= cost; // an X card can be played for X = 0
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

    // Pay energy. An X card spends everything left: `x_value` is what it spent.
    int x_value = 0;
    if (cost == COST_X) {
        x_value = std::max<int>(player.energy, 0);
        cost = static_cast<uint8_t>(x_value);
    }
    player.energy -= cost;
    // The counters are updated once the effect is resolved: inside the switch they still describe what
    // happened before this card ("already played this turn", "spent on other cards").

    // Remove from hand
    hand.erase(index);

    // Apply effects
    switch (card_id) {
        case CardId::STRIKE: {
            hit(current_target, base_value + player.sovereign_stars);
            set_log(upgraded ? "Played Strike+!"sv : "Played Strike!"sv);
            break;
        }
        case CardId::DEFEND: {
            player.gain_block(base_value + player.sovereign_stars);
            set_log(upgraded ? "Played Defend+!"sv : "Played Defend!"sv);
            break;
        }
        case CardId::BASH: {
            hit(current_target, base_value);
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Bash+!"sv : "Played Bash!"sv);
            break;
        }
        case CardId::IRON_WAVE: {
            player.gain_block(base_value);
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
            hit(current_target, base_value);
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
            hit(current_target, base_value);
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
            hit(current_target, base_value);
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
            hit(current_target, base_value);
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
            hit(current_target, base_value);
            current_target.add_status(StatusType::WEAK, secondary_value);
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
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Beam Cell+!"sv : "Played Beam Cell!"sv);
            break;
        }
        case CardId::BALL_LIGHTNING: {
            hit(current_target, base_value);
            player.channel_orb(OrbType::LIGHTNING, &current_target);
            set_log(upgraded ? "Played Ball Lightning+!"sv : "Played Ball Lightning!"sv);
            break;
        }
        case CardId::COLD_SNAP: {
            hit(current_target, base_value);
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
            hit(current_target, base_value);
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
            for (uint8_t blow = 0; blow < secondary_value; ++blow) {
                hit(current_target, base_value);
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
                    enemies[i].add_status(StatusType::VULNERABLE, secondary_value);
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
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
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
            player.gain_block(static_cast<int16_t>(base_value + secondary_value * strength));
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
            current_target.add_status(StatusType::WEAK, secondary_value);
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Uppercut+!"sv : "Played Uppercut!"sv);
            break;
        }
        case CardId::FIGHT_ME: {
            for (int blow = 0; blow < 2; ++blow) {
                hit(current_target, base_value);
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
            hit(current_target, static_cast<int16_t>(base_value - secondary_value * static_cast<int>(hand.size())));
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
                player.gain_block(base_value);
            }
            set_log(upgraded ? "Played Escape Plan+!"sv : "Played Escape Plan!"sv);
            break;
        }
        case CardId::DASH: {
            player.gain_block(secondary_value);
            hit(current_target, base_value);
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
            hit(current_target, base_value);
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
            hit(current_target, base_value);
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
            current_target.add_status(StatusType::WEAK, secondary_value);
            player.channel_orb(OrbType::DARK, &current_target);
            set_log(upgraded ? "Played Null+!"sv : "Played Null!"sv);
            break;
        }
        case CardId::GO_FOR_THE_EYES: {
            hit(current_target, base_value);
            if (current_target.current_intent.base_damage > 0) {
                current_target.add_status(StatusType::WEAK, secondary_value);
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
                    if (static_cast<int>(i) != target_idx && enemies[i].is_alive()) enemies[i].add_status(debuff, amount);
                }
            }
            set_log(upgraded ? "Played Misery+!"sv : "Played Misery!"sv);
            break;
        }
        case CardId::UNDEATH: {
            player.gain_block(base_value);
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
            hit(current_target, base_value);
            current_target.add_status(StatusType::VULNERABLE, current_target.get_status(StatusType::VULNERABLE));
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
            player.gain_block(base_value);
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
            int swapped = static_cast<int>(hand.size());
            while (!hand.empty()) discard_from_hand(hand.size() - 1);
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
        case CardId::BLOOD_WALL: {
            lose_hp(secondary_value);
            player.gain_block(base_value);
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
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
            set_log(upgraded ? "Played Assassinate+!"sv : "Played Assassinate!"sv);
            break;
        }
        case CardId::BACKSTAB: {
            hit(current_target, base_value);
            set_log(upgraded ? "Played Backstab+!"sv : "Played Backstab!"sv);
            break;
        }
        case CardId::BLUR: {
            player.gain_block(base_value);
            if (keep_block_turns < UINT8_MAX) ++keep_block_turns;
            set_log(upgraded ? "Played Blur+!"sv : "Played Blur!"sv);
            break;
        }
        case CardId::DODGE_AND_ROLL: {
            player.gain_block(base_value);
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
            current_target.add_status(StatusType::STRENGTH, static_cast<int16_t>(-dose));
            current_target.add_status(StatusType::WEAK, dose);
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
            current_target.add_status(StatusType::WEAK, secondary_value);
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
            current_target.add_status(StatusType::POISON, secondary_value);
            set_log(upgraded ? "Played Snakebite+!"sv : "Played Snakebite!"sv);
            break;
        }
        case CardId::BOOT_SEQUENCE: {
            player.gain_block(base_value);
            set_log(upgraded ? "Played Boot Sequence+!"sv : "Played Boot Sequence!"sv);
            break;
        }
        case CardId::CHARGE_BATTERY: {
            player.gain_block(base_value);
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
            player.gain_block(base_value);
            schedule(DelayedType::CHANNEL_LIGHTNING, 1, 1, secondary_value);
            set_log(upgraded ? "Played Lightning Rod+!"sv : "Played Lightning Rod!"sv);
            break;
        }
        case CardId::TEMPEST: {
            for (int blow = 0; blow < x_value + secondary_value; ++blow) {
                player.channel_orb(OrbType::LIGHTNING, &current_target);
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
            player.gain_block(base_value);
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
            current_target.add_status(StatusType::WEAK, base_value);
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
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
            player.gain_block(base_value);
            current_target.add_status(StatusType::WEAK, secondary_value);
            set_log(upgraded ? "Played Defy+!"sv : "Played Defy!"sv);
            break;
        }
        case CardId::DELAY: {
            player.gain_block(base_value);
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
            current_target.add_status(StatusType::VULNERABLE, secondary_value);
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
        default:
            set_log("Played card!");
            break;
    }

    // Keep primary enemy in sync
    if (!enemies.empty()) {
        enemy = enemies[0];
    }

    // Tallies of the card just played
    CardType played_type = CardDef::get(card_id).type;
    if (played_type == CardType::ATTACK) {
        counters.attacks_played = bump(counters.attacks_played);
    } else if (played_type == CardType::SKILL) {
        counters.skills_played = bump(counters.skills_played);
    } else {
        counters.powers_played = bump(counters.powers_played);
    }
    counters.energy_spent = bump(counters.energy_spent, cost);
    counters.combat_cards_played = bump(counters.combat_cards_played);
    if (CardDef::has_keyword(played_card, KEYWORD_ETHEREAL)) {
        counters.combat_ethereal_played = bump(counters.combat_ethereal_played);
    }

    // Powers stay in play for the rest of the combat: they never return to the piles.
    if (played_type != CardType::POWER) {
        if (CardDef::has_keyword(played_card, KEYWORD_EXHAUST)) {
            exhaust_card(played_card);
        } else if (CardDef::has_keyword(played_card, KEYWORD_RETURN)) {
            if (!returning_cards.push_back(played_card)) discard_pile.push_back(played_card);
        } else {
            discard_pile.push_back(played_card);
        }
    }

    // A card that costs HP can kill its player.
    if (!player.is_alive()) {
        state = CombatState::DEFEAT;
        set_log("You were defeated!");
        return true;
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

    // Ethereal cards left in hand are exhausted; the rest is discarded, except the cards with Retain.
    for (size_t i = hand.size(); i-- > 0;) {
        if (CardDef::has_keyword(hand[i], KEYWORD_ETHEREAL)) {
            exhaust_from_hand(i);
            continue;
        }
        if (retain_hand || CardDef::has_keyword(hand[i], KEYWORD_RETAIN) ||
            CardDef::has_keyword(hand[i], KEYWORD_RETAIN_ONCE)) {
            hand[i].added_keywords &= static_cast<uint8_t>(~KEYWORD_RETAIN_ONCE);
            continue;
        }
        discard_pile.push_back(hand[i]);
        hand.erase(i);
    }

    // Strength and Dexterity gained "until the end of the turn" go away.
    player.add_status(StatusType::STRENGTH, static_cast<int16_t>(-temp_strength));
    player.add_status(StatusType::DEXTERITY, static_cast<int16_t>(-temp_dexterity));
    temp_strength = 0;
    temp_dexterity = 0;

    // End of turn effects: countdowns, Metallicize block, then orb passives on the first living enemy.
    run_delayed_effects(true);
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

            // The Strength it lost "until the end of its turn" comes back.
            enemies[i].add_status(StatusType::STRENGTH, enemies[i].temp_strength_loss);
            enemies[i].temp_strength_loss = 0;

            record.damage_to_player = hp_before - player.hp;
            note_player_hp_lost(record.damage_to_player);
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
