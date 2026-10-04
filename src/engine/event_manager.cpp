#include "engine/event_manager.h"
#include "engine/potion.h"
#include "engine/relic.h"
#include "engine/rewards.h"
#include "engine/run.h"
#include <algorithm>

namespace engine {

namespace {

using namespace std::string_view_literals;

using Outcome = EventOutcomeType;

// Every event of the game. scripts/progress.py reads the `.id = "..."` lines of this table to know which
// events exist, and scripts/event_text_test.py measures every text against the event screen of the GBA.
//
// The reference (bible/data/event_v0.111.0.json) gives each event its acts, its story and a number of
// options, not what the options do (#252): an event enters the table once its choices are documented.
// The four below are the ones the game had before the table, with the choices it had invented for them.
constexpr EventEncounter EVENT_TABLE[] = {
    {
        .id = "BATTLEWORN_DUMMY",
        .title = "Mannequin d'Entraînement",
        .dialogue = "« BZZZT ! SÉQUENCE D'ENTRAÎNEMENT INITIÉE ! Réglez vos coups ou subissez une humiliation létale ! »",
        .zones = EVENT_ZONE_ACT3,
        .choices = {
            { .label = "Attaque acharnée",
              .cost_benefit = "Subissez 6 dégâts, gagnez 60 Or.",
              .outcomes = { { .type = Outcome::LOSE_HP, .value = 6 }, { .type = Outcome::GAIN_GOLD, .value = 60 } },
              .result_message = "Vous frappez sans retenue ! Le mannequin explose en pièces d'or." },
            { .label = "Réparer les rouages",
              .cost_benefit = "Améliorez une carte de votre deck.",
              .outcomes = { { .type = Outcome::UPGRADE_CARD, .value = 1, .pick = EventPick::FIRST } },
              .result_message = "Vous étudiez la mécanique interne. Une de vos cartes est améliorée (+) !" },
            { .label = "Prendre la fuite",
              .cost_benefit = "Passez votre chemin sans risque.",
              .result_message = "Vous quittez discrètement la salle." },
        },
    },
    {
        .id = "COLOSSAL_FLOWER",
        .title = "Fleur Colossale",
        .dialogue = "Une fleur cyclopéenne exsude un parfum suave. Du nectar doré perle sur ses pétales tranchants comme des rasoirs.",
        .zones = EVENT_ZONE_ACT2,
        .choices = {
            { .label = "Boire le nectar doré",
              .cost_benefit = "Récupérez 20 PV.",
              .outcomes = { { .type = Outcome::HEAL, .value = 20 } },
              .result_message = "Le nectar restaure vos forces (+20 PV) !" },
            { .label = "Arracher l'Épine Ancestrale",
              .cost_benefit = "Subissez 10 dégâts, obtenez la Relique Vajra (+1 Force).",
              .outcomes = { { .type = Outcome::LOSE_HP, .value = 10 }, { .type = Outcome::GAIN_RELIC, .item = "VAJRA" } },
              .result_message = "Vous coupez l'épine au prix d'une vive blessure. Vous obtenez Vajra !" },
            { .label = "Contourner la plante",
              .cost_benefit = "Ne prenez aucun risque.",
              .result_message = "Vous contournez la fleur avec précaution." },
        },
    },
    {
        .id = "ABYSSAL_BATHS",
        .title = "Bains Abyssaux",
        .dialogue = "Des bassins fumants émettent des vapeurs minérales revigorantes. Au fond d'une vasque, des pièces d'or scintillent.",
        .zones = EVENT_ZONE_ACT1B,
        .choices = {
            // The 35 gold announced were never taken before the table; the port keeps what the choice did.
            { .label = "Se prélasser dans le bain",
              .cost_benefit = "Récupérez 25 PV, coûte 35 Or.",
              .outcomes = { { .type = Outcome::HEAL, .value = 25 } },
              .result_message = "Les vapeurs chaudes dissolvent vos courbatures (+25 PV)." },
            { .label = "Plonger dans les profondeurs",
              .cost_benefit = "Gagnez 80 Or, subissez 8 dégâts de brûlure.",
              .outcomes = { { .type = Outcome::LOSE_HP, .value = 8 }, { .type = Outcome::GAIN_GOLD, .value = 80 } },
              .result_message = "L'eau bouillante vous brûle mais vous repêchez 80 Pièces d'or !" },
            { .label = "Repartir",
              .cost_benefit = "Ignorer les bains.",
              .result_message = "Vous reprenez votre ascension sans vous attarder." },
        },
    },
    {
        // Not an event of the reference: kept from before the table, met in every act.
        .id = "MYSTIC_ALTAR",
        .title = "Autel de la Tour",
        .dialogue = "Un monolithe d'obsidienne pulse au rythme d'une énergie ancienne. Une cavité d'offrande attend votre tribut.",
        .zones = EVENT_ZONE_ALL,
        .choices = {
            { .label = "Offrande de sang",
              .cost_benefit = "Sacrifiez 7 PV, gagnez 90 Or.",
              .outcomes = { { .type = Outcome::LOSE_HP, .value = 7 }, { .type = Outcome::GAIN_GOLD, .value = 90 } },
              .result_message = "L'autel absorbe votre sang et fait jaillir 90 Pièces d'or !" },
            { .label = "Prière méditative",
              .cost_benefit = "Améliorez une carte de votre deck.",
              .outcomes = { { .type = Outcome::UPGRADE_CARD, .value = 1, .pick = EventPick::FIRST } },
              .result_message = "Une bénédiction mystique forge l'une de vos cartes en version (+) !" },
            { .label = "S'incliner et partir",
              .cost_benefit = "Ne rien sacrifier.",
              .result_message = "Vous saluez l'autel et poursuivez votre chemin." },
        },
    },
};

constexpr size_t EVENT_COUNT = sizeof(EVENT_TABLE) / sizeof(EVENT_TABLE[0]);
constexpr size_t MAX_EVENTS = 64;
static_assert(EVENT_COUNT <= MAX_EVENTS, "EventManager::pick orders the events in a 64-entry buffer");

void safe_copy(char* dest, size_t dest_len, std::string_view src) {
    if (!dest || dest_len == 0) return;
    size_t count = std::min(dest_len - 1, src.size());
    for (size_t i = 0; i < count; ++i) {
        dest[i] = src[i];
    }
    dest[count] = '\0';
}

// A stream of its own, so that an event never moves the run's main random sequence.
struct EventRng {
    uint32_t state;

    explicit EventRng(uint32_t seed) : state(seed * 2654435761u + 0x9E3779B9u) {
        if (state == 0) state = 12345;
        next();
    }

    uint32_t next() {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    // 0 to count - 1; the high bits, which xorshift mixes best.
    uint32_t below(uint32_t count) { return count ? (next() >> 8) % count : 0; }
};

void shuffle(uint8_t* begin, size_t count, EventRng& rng) {
    for (size_t i = 0; i + 1 < count; ++i) {
        size_t pick = i + rng.below(static_cast<uint32_t>(count - i));
        std::swap(begin[i], begin[pick]);
    }
}

std::string_view random_relic(const RunManager& run, EventRng& rng) {
    // The rule of the chests: a relic with an implemented effect, shared or of this hero, not owned yet.
    std::string_view hero_pool = get_character(run.character).english_name;
    auto is_candidate = [&](std::string_view id) {
        const RelicData* relic = find_relic(id);
        return relic && (relic->pool == "Shared" || relic->pool == hero_pool) && !run.has_relic(id);
    };
    uint32_t count = 0;
    for (std::string_view id : RunManager::active_relics()) {
        if (is_candidate(id)) ++count;
    }
    if (count == 0) return {};
    uint32_t pick = rng.below(count);
    for (std::string_view id : RunManager::active_relics()) {
        if (is_candidate(id) && pick-- == 0) return id;
    }
    return {};
}

// Index of the deck card a card outcome acts on, -1 when no card qualifies.
template<typename Qualifies>
int pick_card(const RunManager& run, EventPick pick, EventRng& rng, Qualifies qualifies) {
    uint32_t count = 0;
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (qualifies(run.master_deck[i])) ++count;
    }
    if (count == 0) return -1;
    uint32_t wanted = pick == EventPick::FIRST ? 0 : rng.below(count);
    for (size_t i = 0; i < run.master_deck.size(); ++i) {
        if (qualifies(run.master_deck[i]) && wanted-- == 0) return static_cast<int>(i);
    }
    return -1;
}

void report(EventResult* result, EventOutcomeType type, int16_t amount, std::string_view name = {}) {
    if (result) result->effects.push_back(EventEffect{ type, amount, name });
}

int16_t clamp16(int value) {
    return static_cast<int16_t>(std::clamp(value, -32768, 32767));
}

} // namespace

std::span<const EventEncounter> EventManager::all() {
    return EVENT_TABLE;
}

const EventEncounter* EventManager::find(std::string_view id) {
    for (const EventEncounter& event : EVENT_TABLE) {
        if (event.id == id) return &event;
    }
    return nullptr;
}

const EventEncounter& EventManager::pick(const MapGraph& map, uint8_t act, uint32_t map_seed, uint8_t floor) {
    // The events of the zone, then the others, each part in an order drawn from the seed of the act's map.
    const uint8_t zone = zone_bit(act_zone(act, map_seed));
    uint8_t order[MAX_EVENTS];
    size_t own = 0;
    for (size_t i = 0; i < EVENT_COUNT; ++i) {
        if (EVENT_TABLE[i].zones & zone) order[own++] = static_cast<uint8_t>(i);
    }
    size_t total = own;
    for (size_t i = 0; i < EVENT_COUNT; ++i) {
        if (!(EVENT_TABLE[i].zones & zone)) order[total++] = static_cast<uint8_t>(i);
    }
    EventRng rng(map_seed ^ (static_cast<uint32_t>(act) << 24));
    shuffle(order, own, rng);
    shuffle(order + own, total - own, rng);

    // A hero enters one room per floor: counting the floors below this one that hold an event room gives
    // every event floor of the act its own place in the order. `floor` counts from 1, MapNode::floor from 0.
    uint32_t event_floors = 0;
    for (const MapNode& node : map.nodes) {
        if (node.type == RoomType::EVENT && node.floor < 32 && node.floor + 1 < floor) event_floors |= 1u << node.floor;
    }
    size_t rank = 0;
    for (uint32_t bits = event_floors; bits; bits &= bits - 1) ++rank;
    return EVENT_TABLE[order[rank % total]];
}

bool EventManager::choice_available(const RunManager& run, const EventChoice& choice) {
    switch (choice.condition) {
        case EventCondition::GOLD_AT_LEAST:
            return run.gold >= choice.condition_value;
        case EventCondition::HP_ABOVE:
            return run.hp > choice.condition_value;
        case EventCondition::MAX_HP_ABOVE:
            return run.max_hp > choice.condition_value;
        case EventCondition::DECK_AT_LEAST:
            return static_cast<int>(run.master_deck.size()) >= choice.condition_value;
        case EventCondition::UPGRADABLE_CARD:
            return run.can_smith();
        case EventCondition::FREE_POTION_SLOT:
            return run.potions.size() < run.potions.capacity();
        case EventCondition::NONE:
        default:
            return true;
    }
}

bool EventManager::apply_choice(RunManager& run, const EventChoice& choice, char* out_msg, size_t max_len,
                                EventResult* result, uint32_t seed) {
    if (result) {
        result->effects.clear();
        result->fight = false;
    }
    if (!choice_available(run, choice)) return false;

    EventRng rng(seed);
    std::string_view message = choice.result_message;
    bool fight = false;
    RoomType fight_room = RoomType::MONSTER;

    for (const EventOutcome& outcome : choice.outcomes) {
        const int amount = outcome.value + (outcome.spread > 0 ? static_cast<int>(rng.below(outcome.spread + 1)) : 0);

        switch (outcome.type) {
            case EventOutcomeType::GAIN_GOLD: {
                int16_t before = run.gold;
                run.gold = clamp16(run.gold + amount);
                report(result, outcome.type, run.gold - before);
                break;
            }
            case EventOutcomeType::LOSE_GOLD: {
                int16_t before = run.gold;
                run.gold = clamp16(std::max(0, run.gold - amount));
                report(result, outcome.type, before - run.gold);
                break;
            }
            case EventOutcomeType::HEAL: {
                int16_t before = run.hp;
                run.hp = clamp16(std::min<int>(run.max_hp, run.hp + amount));
                report(result, outcome.type, run.hp - before);
                break;
            }
            case EventOutcomeType::LOSE_HP: {
                int16_t before = run.hp;
                run.hp = clamp16(std::max(1, run.hp - amount));
                report(result, outcome.type, before - run.hp);
                break;
            }
            case EventOutcomeType::GAIN_MAX_HP: {
                int16_t before = run.max_hp;
                run.max_hp = clamp16(run.max_hp + amount);
                run.hp = clamp16(run.hp + (run.max_hp - before));
                report(result, outcome.type, run.max_hp - before);
                break;
            }
            case EventOutcomeType::LOSE_MAX_HP: {
                int16_t before = run.max_hp;
                run.max_hp = clamp16(std::max(1, run.max_hp - amount));
                run.hp = std::min(run.hp, run.max_hp);
                report(result, outcome.type, before - run.max_hp);
                break;
            }
            case EventOutcomeType::GAIN_RELIC: {
                std::string_view id = outcome.item.empty() ? random_relic(run, rng) : outcome.item;
                if (id.empty() || run.relics.size() >= run.relics.capacity()) break;
                run.add_relic(id);
                const RelicData* relic = find_relic(id);
                report(result, outcome.type, 1, relic ? relic->name : id);
                break;
            }
            case EventOutcomeType::GAIN_POTION: {
                if (run.potions.size() >= run.potions.capacity()) break;
                std::string_view id = outcome.item;
                if (id.empty()) {
                    // The draw of the fight rewards: by rarity, among the potions this hero can use.
                    RewardRng potion_rng(rng.next());
                    id = draw_potion(potion_rng, run.character);
                }
                if (id.empty()) break;
                run.potions.push_back(id);
                const PotionData* potion = find_potion(id);
                report(result, outcome.type, 1, potion ? potion->name : id);
                break;
            }
            case EventOutcomeType::GAIN_CARD: {
                CardId id = outcome.card;
                if (id == CardId::NONE) {
                    std::span<const CardId> pool = RunManager::card_reward_pool(run.character);
                    id = pool[rng.below(static_cast<uint32_t>(pool.size()))];
                }
                if (!run.master_deck.push_back(DeckCard(id))) break;
                report(result, outcome.type, 1, CardDef::get(id).name);
                break;
            }
            case EventOutcomeType::REMOVE_CARD: {
                for (int n = 0; n < amount; ++n) {
                    int index = pick_card(run, outcome.pick, rng, [](const DeckCard&) { return true; });
                    if (index < 0) break;
                    const DeckCard card = run.master_deck[index];
                    run.remove_card_from_deck(index);
                    report(result, outcome.type, 1, CardDef::get_display_name(card.id, card.upgraded));
                }
                break;
            }
            case EventOutcomeType::UPGRADE_CARD: {
                for (int n = 0; n < amount; ++n) {
                    int index = pick_card(run, outcome.pick, rng, [](const DeckCard& card) { return !card.upgraded; });
                    if (index < 0) {
                        if (n == 0) message = "Toutes vos cartes sont déjà améliorées !"sv;
                        break;
                    }
                    run.upgrade_card_in_deck(index);
                    report(result, outcome.type, 1, CardDef::get_display_name(run.master_deck[index].id, true));
                }
                break;
            }
            case EventOutcomeType::TRANSFORM_CARD: {
                std::span<const CardId> pool = RunManager::card_reward_pool(run.character);
                for (int n = 0; n < amount; ++n) {
                    int index = pick_card(run, outcome.pick, rng, [](const DeckCard&) { return true; });
                    if (index < 0) break;
                    // A different card: the pool holds each card once, so the next one is different.
                    size_t drawn = rng.below(static_cast<uint32_t>(pool.size()));
                    if (pool[drawn] == run.master_deck[index].id) drawn = (drawn + 1) % pool.size();
                    run.master_deck[index] = DeckCard(pool[drawn]);
                    report(result, outcome.type, 1, CardDef::get(pool[drawn]).name);
                }
                break;
            }
            case EventOutcomeType::FIGHT:
                fight = true;
                fight_room = outcome.value == EVENT_FIGHT_ELITE ? RoomType::ELITE : RoomType::MONSTER;
                break;
            case EventOutcomeType::NONE:
            default:
                break;
        }
    }

    // Last, whatever its place in the list: the fight starts with the HP the other outcomes left.
    if (fight && run.start_event_fight(fight_room)) {
        report(result, EventOutcomeType::FIGHT, 1);
        if (result) result->fight = true;
    }

    safe_copy(out_msg, max_len, message);
    return true;
}

} // namespace engine
