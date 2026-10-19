#include "engine/event_manager.h"
#include "engine/card_meta.h"
#include "engine/encounter.h"
#include "engine/rewards.h"
#include "engine/run.h"
#include <algorithm>

namespace engine {

// The table of slots lives in event_registry.cpp.
std::span<const EventSlot> event_slots();

namespace {

void safe_copy(char* dest, size_t dest_len, std::string_view src) {
    if (!dest || dest_len == 0) return;
    size_t count = std::min(dest_len - 1, src.size());
    for (size_t i = 0; i < count; ++i) {
        dest[i] = src[i];
    }
    dest[count] = '\0';
}

uint8_t zone_bit(ActZone zone) {
    switch (zone) {
        case ActZone::ACT1A: return EVENT_ACT_1A;
        case ActZone::ACT1B: return EVENT_ACT_1B;
        case ActZone::ACT2: return EVENT_ACT_2;
        default: return EVENT_ACT_3;
    }
}

int count_of(const EventEffect& effect) {
    return effect.amount > 0 ? effect.amount : 1;
}

DeckChoiceKind choice_kind(EventEffectType type) {
    switch (type) {
        case EventEffectType::REMOVE_CARD: return DeckChoiceKind::REMOVE;
        case EventEffectType::UPGRADE_CARD: return DeckChoiceKind::UPGRADE;
        case EventEffectType::TRANSFORM_CARD: return DeckChoiceKind::TRANSFORM;
        default: return DeckChoiceKind::ENCHANT;
    }
}

} // namespace

std::span<const EventSlot> EventManager::slots() {
    return event_slots();
}

int EventManager::slot_index(std::string_view id) {
    std::span<const EventSlot> all = slots();
    for (size_t i = 0; i < all.size(); ++i) {
        if (all[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

const EventDef* EventManager::find(std::string_view id) {
    int index = slot_index(id);
    return index < 0 ? nullptr : slots()[index].def;
}

bool EventManager::meets(const RunManager& run, const EventCondition& condition) {
    switch (condition.check) {
        case EventCheck::MIN_GOLD: return run.gold >= condition.amount;
        case EventCheck::MIN_HP: return run.hp > condition.amount;
        case EventCheck::HAS_RELIC: return run.has_relic(condition.ref);
        case EventCheck::NO_RELIC: return !run.has_relic(condition.ref);
        case EventCheck::MIN_DECK: return run.master_deck.size() >= static_cast<size_t>(std::max<int16_t>(0, condition.amount));
        case EventCheck::HAS_POTION: return !run.potions.empty();
        case EventCheck::CAN_SOULS_POWER:
            for (const DeckCard& card : run.master_deck) {
                if (can_enchant(card, Enchantment::SOULS_POWER)) return true;
            }
            return false;
        case EventCheck::NONE:
        default: return true;
    }
}

int EventManager::draw(const RunManager& run, uint64_t seen, uint32_t seed) {
    const uint8_t zone = zone_bit(act_zone(run.current_act, run.map_seed));
    std::span<const EventSlot> all = slots();
    // Twice at most: first among the events not met yet, then, once the act has none left, among all of them.
    for (int pass = 0; pass < 2; ++pass) {
        int candidates[64];
        int count = 0;
        for (size_t i = 0; i < all.size() && i < 64; ++i) {
            const EventDef* def = all[i].def;
            if (!def || (def->acts & zone) == 0 || def->pages.empty() || !meets(run, def->condition)) continue;
            if (pass == 0 && (seen >> i) & 1u) continue;
            candidates[count++] = static_cast<int>(i);
        }
        if (count > 0) return candidates[(seed >> 16) % static_cast<uint32_t>(count)];
    }
    return -1;
}

void EventManager::open_page(RunManager& run, const EventDef& def, uint8_t page) {
    EventEncounter& enc = run.current_event;
    enc.def = &def;
    enc.id = def.id;
    enc.title = def.title;
    enc.page = page < def.pages.size() ? page : 0;
    enc.choices.clear();
    if (def.pages.empty()) {
        enc.dialogue = {};
        return;
    }
    const EventPage& shown = def.pages[enc.page];
    enc.dialogue = shown.text;
    for (const EventOption& option : shown.options) {
        EventChoice choice;
        choice.label = option.label;
        choice.cost_benefit = option.cost_benefit;
        choice.result_message = option.result;
        choice.enabled = meets(run, option.condition);
        if (!enc.choices.push_back(choice)) break;
    }
}

bool EventManager::choose(RunManager& run, int index, char* out_msg, size_t max_len) {
    EventEncounter& enc = run.current_event;
    if (run.phase != RunPhase::EVENT || run.hp <= 0 || !enc.def || index < 0 || static_cast<size_t>(index) >= enc.choices.size() || !enc.choices[index].enabled) {
        return false;
    }
    const EventOption& option = enc.def->pages[enc.page].options[index];
    safe_copy(out_msg, max_len, option.result);
    // Back on the map unless an effect leads elsewhere: a fight, a reward screen, another page.
    run.phase = RunPhase::MAP_NAVIGATION;
    int next_page = -1;
    for (const EventEffect& effect : option.effects) {
        if (effect.type == EventEffectType::GOTO_PAGE) {
            next_page = effect.amount;
            continue;
        }
        apply_effect(run, effect);
    }
    if (next_page >= 0 && run.hp > 0 && run.phase == RunPhase::MAP_NAVIGATION) {
        open_page(run, *enc.def, static_cast<uint8_t>(next_page));
        run.phase = RunPhase::EVENT;
    }
    // Killed by the event: the run ends on it (see run_end_cause).
    if (run.hp <= 0) run.phase = RunPhase::EVENT;
    return true;
}

void EventManager::apply_effect(RunManager& run, const EventEffect& effect) {
    switch (effect.type) {
        case EventEffectType::GAIN_GOLD:
            run.gain_gold(effect.amount);
            break;
        case EventEffectType::LOSE_GOLD:
            run.gold = static_cast<int16_t>(std::max(0, run.gold - effect.amount));
            break;
        case EventEffectType::HEAL:
            run.heal(effect.amount);
            break;
        case EventEffectType::LOSE_HP:
            run.hp = static_cast<int16_t>(std::max(0, run.hp - effect.amount));
            break;
        case EventEffectType::GAIN_MAX_HP:
            run.gain_max_hp(effect.amount);
            break;
        case EventEffectType::LOSE_MAX_HP:
            run.lose_max_hp(effect.amount);
            break;
        case EventEffectType::ADD_CARD:
            for (int n = 0; n < count_of(effect); ++n) run.add_card_to_deck(DeckCard{ effect.card, effect.upgraded });
            break;
        case EventEffectType::ADD_CURSE:
            for (int n = 0; n < count_of(effect); ++n) {
                CardId curse = effect.card;
                if (curse == CardId::NONE) {
                    FixedVector<CardId, 64> curses;
                    for (uint16_t id = 0; id < static_cast<uint16_t>(CardId::COUNT); ++id) {
                        if (card_meta(static_cast<CardId>(id)).pool == CardPool::CURSE) curses.push_back(static_cast<CardId>(id));
                    }
                    if (curses.empty()) break;
                    curse = curses[run.next_random() % curses.size()];
                }
                run.add_card_to_deck(DeckCard{ curse, false });
            }
            break;
        case EventEffectType::REMOVE_CARD:
        case EventEffectType::UPGRADE_CARD:
        case EventEffectType::TRANSFORM_CARD:
        case EventEffectType::ENCHANT_CARD: {
            const DeckChoiceKind kind = choice_kind(effect.type);
            if (effect.pick == EventPick::CHOOSE) {
                run.request_deck_choice(kind, count_of(effect), false, effect.upgraded, CardId::NONE, effect.enchantment,
                                        effect.enchant_amount);
                break;
            }
            if (kind == DeckChoiceKind::UPGRADE) {
                run.upgrade_random_cards(count_of(effect));
                break;
            }
            // Random: the cards a choice would allow, drawn one by one.
            const DeckChoice before = run.deck_choice;
            run.deck_choice = {};
            run.deck_choice.kind = kind;
            run.deck_choice.enchantment = effect.enchantment;
            for (int n = 0; n < count_of(effect); ++n) {
                FixedVector<int, 50> candidates;
                for (size_t i = 0; i < run.master_deck.size(); ++i) {
                    if (run.deck_choice_allows(static_cast<int>(i))) candidates.push_back(static_cast<int>(i));
                }
                if (candidates.empty()) break;
                const int pick = candidates[run.next_random() % candidates.size()];
                if (kind == DeckChoiceKind::REMOVE) run.remove_card_from_deck(pick);
                else if (kind == DeckChoiceKind::TRANSFORM) run.transform_card_in_deck(pick, effect.upgraded);
                else run.enchant_card_in_deck(pick, effect.enchantment, effect.enchant_amount);
            }
            run.deck_choice = before;
            break;
        }
        case EventEffectType::CARD_REWARD: {
            run.card_rewards_upgraded = false;
            run.gold_reward = 0;
            run.potion_reward = {};
            run.relic_reward = {};
            RewardRng rng(run.next_random());
            run.draw_card_rewards(false, rng);
            run.phase = RunPhase::REWARD_SCREEN;
            break;
        }
        case EventEffectType::GAIN_RELIC:
            if (effect.ref.empty()) {
                run.gain_random_relics(1);
            } else if (!run.has_relic(effect.ref)) {
                run.add_relic(effect.ref);
            }
            break;
        case EventEffectType::RELIC_REWARD: {
            if (run.phase != RunPhase::REWARD_SCREEN) {
                run.current_card_rewards.clear();
                run.gold_reward = 0;
                run.potion_reward = {};
            }
            RewardRng rng(run.next_random());
            run.relic_reward = draw_relic(rng, run.character, RelicSource::ELITE,
                                          std::span<const std::string_view>(run.relics.begin(), run.relics.size()));
            if (!run.relic_reward.empty()) run.phase = RunPhase::REWARD_SCREEN;
            break;
        }
        case EventEffectType::GAIN_POTION:
            if (effect.ref.empty()) {
                run.gain_random_potions(count_of(effect));
            } else if (!run.has_relic("SOZU") && !run.potion_belt_full()) {
                run.potions.push_back(effect.ref);
            }
            break;
        case EventEffectType::FIGHT:
            run.begin_fight(effect.ref == "ELITE" ? RoomType::ELITE : RoomType::MONSTER);
            break;
        case EventEffectType::GOTO_PAGE:
        case EventEffectType::LEAVE:
        default:
            break;
    }
}

} // namespace engine
