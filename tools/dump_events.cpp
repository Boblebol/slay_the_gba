// Dumps the event tables of the engine (src/engine/events/*.h, read through EventManager::slots()) as JSON on the
// standard output: the pages, the options with their label, cost, result text and condition, and each effect
// with all its fields. scripts/build_event_docs.py builds and runs it to write bible/data/event_game_*.json and
// the choices of bible/18_evenements.md, so the documentation of the events follows the code.
//
//   clang++ -std=c++20 -Iinclude <ENGINE_SOURCES of the Makefile> tools/dump_events.cpp -o bin/dump_events
#include <cstdio>
#include <string>
#include <string_view>

#include "engine/card.h"
#include "engine/event_manager.h"

using namespace engine;

namespace {

std::string quoted(std::string_view text) {
    std::string out = "\"";
    for (char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    out += '"';
    return out;
}

const char* effect_name(EventEffectType type) {
    switch (type) {
        case EventEffectType::GAIN_GOLD: return "GAIN_GOLD";
        case EventEffectType::LOSE_GOLD: return "LOSE_GOLD";
        case EventEffectType::HEAL: return "HEAL";
        case EventEffectType::LOSE_HP: return "LOSE_HP";
        case EventEffectType::GAIN_MAX_HP: return "GAIN_MAX_HP";
        case EventEffectType::LOSE_MAX_HP: return "LOSE_MAX_HP";
        case EventEffectType::ADD_CARD: return "ADD_CARD";
        case EventEffectType::ADD_CURSE: return "ADD_CURSE";
        case EventEffectType::ADD_RANDOM_CARD: return "ADD_RANDOM_CARD";
        case EventEffectType::REMOVE_CARD: return "REMOVE_CARD";
        case EventEffectType::UPGRADE_CARD: return "UPGRADE_CARD";
        case EventEffectType::TRANSFORM_CARD: return "TRANSFORM_CARD";
        case EventEffectType::ENCHANT_CARD: return "ENCHANT_CARD";
        case EventEffectType::CARD_REWARD: return "CARD_REWARD";
        case EventEffectType::GAIN_RELIC: return "GAIN_RELIC";
        case EventEffectType::RELIC_REWARD: return "RELIC_REWARD";
        case EventEffectType::LOSE_RELIC: return "LOSE_RELIC";
        case EventEffectType::LOSE_POTION: return "LOSE_POTION";
        case EventEffectType::GAIN_POTION: return "GAIN_POTION";
        case EventEffectType::FIGHT: return "FIGHT";
        case EventEffectType::TRADE_POTION: return "TRADE_POTION";
        case EventEffectType::GOTO_PAGE: return "GOTO_PAGE";
        case EventEffectType::LOSE_CARD: return "LOSE_CARD";
        case EventEffectType::DOWNGRADE_CARD: return "DOWNGRADE_CARD";
        case EventEffectType::DUPLICATE_DECK: return "DUPLICATE_DECK";
        case EventEffectType::LEAVE: return "LEAVE";
        case EventEffectType::CARD_CHOICE: return "CARD_CHOICE";
        case EventEffectType::TRADE_RELIC: return "TRADE_RELIC";
    }
    return "UNKNOWN";
}

const char* check_name(EventCheck check) {
    switch (check) {
        case EventCheck::NONE: return "NONE";
        case EventCheck::MIN_GOLD: return "MIN_GOLD";
        case EventCheck::MIN_HP: return "MIN_HP";
        case EventCheck::HAS_RELIC: return "HAS_RELIC";
        case EventCheck::NO_RELIC: return "NO_RELIC";
        case EventCheck::MIN_DECK: return "MIN_DECK";
        case EventCheck::HAS_ATTACK: return "HAS_ATTACK";
        case EventCheck::HAS_POTION: return "HAS_POTION";
        case EventCheck::CAN_ENCHANT: return "CAN_ENCHANT";
        case EventCheck::CAN_SOULS_POWER: return "CAN_SOULS_POWER";
        case EventCheck::MIN_RELICS: return "MIN_RELICS";
        case EventCheck::HAS_CARD: return "HAS_CARD";
    }
    return "UNKNOWN";
}

const char* enchantment_name(Enchantment e) {
    switch (e) {
        case Enchantment::NONE: return "";
        case Enchantment::INKY: return "INKY";
        case Enchantment::ADROIT: return "ADROIT";
        case Enchantment::NIMBLE: return "NIMBLE";
        case Enchantment::TEZCATARAS_EMBER: return "TEZCATARAS_EMBER";
        case Enchantment::SWIFT: return "SWIFT";
        case Enchantment::CLONE: return "CLONE";
        case Enchantment::MOMENTUM: return "MOMENTUM";
        case Enchantment::GLAM: return "GLAM";
        case Enchantment::IMBUED: return "IMBUED";
        case Enchantment::INSTINCT: return "INSTINCT";
        case Enchantment::ROYALLY_APPROVED: return "ROYALLY_APPROVED";
        case Enchantment::SHARP: return "SHARP";
        case Enchantment::GOOPY: return "GOOPY";
        case Enchantment::CORRUPTED: return "CORRUPTED";
        case Enchantment::VIGOROUS: return "VIGOROUS";
        case Enchantment::PERFECT_FIT: return "PERFECT_FIT";
        case Enchantment::SOULS_POWER: return "SOULS_POWER";
        case Enchantment::STEADY: return "STEADY";
        case Enchantment::COUNT: break;
    }
    return "UNKNOWN";
}

std::string card_name(CardId id) {
    return id == CardId::NONE ? std::string() : std::string(CardDef::get(id).name);
}

void put_condition(const EventCondition& c) {
    std::printf("{\"check\":%s,\"amount\":%d,\"ref\":%s,\"card\":%s}", quoted(check_name(c.check)).c_str(), c.amount,
                quoted(c.ref).c_str(), quoted(card_name(c.card)).c_str());
}

void put_effect(const EventEffect& e) {
    std::printf("{\"type\":%s,\"amount\":%d,\"ref\":%s,\"card\":%s,\"pick\":%s,\"upgraded\":%s,\"enchantment\":%s,"
                "\"enchant_amount\":%d,\"only_type\":%d,\"relic_pool\":[",
                quoted(effect_name(e.type)).c_str(), e.amount, quoted(e.ref).c_str(), quoted(card_name(e.card)).c_str(),
                quoted(e.pick == EventPick::RANDOM ? "RANDOM" : "CHOOSE").c_str(), e.upgraded ? "true" : "false",
                quoted(enchantment_name(e.enchantment)).c_str(), e.enchant_amount, e.only_type);
    for (size_t i = 0; i < e.relic_pool.size(); ++i) std::printf("%s%s", i ? "," : "", quoted(e.relic_pool[i]).c_str());
    std::printf("],\"card_pool\":[");
    for (size_t i = 0; i < e.card_pool.size(); ++i) std::printf("%s%s", i ? "," : "", quoted(card_name(e.card_pool[i])).c_str());
    std::printf("]}");
}

void put_option(const EventOption& o) {
    std::printf("{\"label\":%s,\"cost_benefit\":%s,\"result\":%s,\"condition\":", quoted(o.label).c_str(),
                quoted(o.cost_benefit).c_str(), quoted(o.result).c_str());
    put_condition(o.condition);
    std::printf(",\"effects\":[");
    for (size_t i = 0; i < o.effects.size(); ++i) {
        if (i) std::printf(",");
        put_effect(o.effects[i]);
    }
    std::printf("]}");
}

void put_event(const EventSlot& slot, bool first) {
    std::printf("%s\n{\"id\":%s,\"implemented\":%s", first ? "" : ",", quoted(slot.id).c_str(), slot.def ? "true" : "false");
    if (slot.def) {
        const EventDef& def = *slot.def;
        std::printf(",\"title\":%s,\"acts\":%d,\"condition\":", quoted(def.title).c_str(), def.acts);
        put_condition(def.condition);
        std::printf(",\"pages\":[");
        for (size_t p = 0; p < def.pages.size(); ++p) {
            std::printf("%s{\"text\":%s,\"options\":[", p ? "," : "", quoted(def.pages[p].text).c_str());
            for (size_t o = 0; o < def.pages[p].options.size(); ++o) {
                if (o) std::printf(",");
                put_option(def.pages[p].options[o]);
            }
            std::printf("]}");
        }
        std::printf("]");
    }
    std::printf("}");
}

} // namespace

int main() {
    std::printf("{\"events\":[");
    bool first = true;
    for (const EventSlot& slot : EventManager::slots()) {
        put_event(slot, first);
        first = false;
    }
    std::printf("\n]}\n");
    return 0;
}
