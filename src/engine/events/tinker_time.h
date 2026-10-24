#pragma once
// Bricolage (TINKER_TIME), Act 3. Choices: bible/data/event_choices/b.json.
// The reference builds a custom card: a type (Weapon = Attack, Gadget = Power, Protector = Skill), then a "rider"
// effect. The game has no custom cards, so everything is guessed: the type gives a card of that type (Ultimate
// Strike, Panache, Ultimate Defend) and the rider changes how it comes: upgraded (Improvement), a random Colorless
// card instead (Chaos), or one card out of 3 of the hero's pool (Wisdom). Pages: 0 accept, 1 type, 2-4 rider.
#include "engine/event_script.h"

namespace engine::events {

inline constexpr int16_t TINKER_TIME_WISDOM_CARDS = 3;

inline constexpr EventEffect TINKER_TIME_ATTACK_UPGRADED[] = {
    { .type = EventEffectType::ADD_CARD, .card = CardId::ULTIMATE_STRIKE, .upgraded = true },
};
inline constexpr EventEffect TINKER_TIME_POWER_UPGRADED[] = {
    { .type = EventEffectType::ADD_CARD, .card = CardId::PANACHE, .upgraded = true },
};
inline constexpr EventEffect TINKER_TIME_SKILL_UPGRADED[] = {
    { .type = EventEffectType::ADD_CARD, .card = CardId::ULTIMATE_DEFEND, .upgraded = true },
};
inline constexpr EventEffect TINKER_TIME_CHAOS[] = {
    { .type = EventEffectType::ADD_CARD },
};
inline constexpr EventEffect TINKER_TIME_WISDOM[] = {
    { .type = EventEffectType::CARD_CHOICE, .amount = TINKER_TIME_WISDOM_CARDS },
};
inline constexpr EventEffect TINKER_TIME_TO_TYPE[] = {
    { .type = EventEffectType::GOTO_PAGE, .amount = 1 },
};
inline constexpr EventEffect TINKER_TIME_TO_ATTACK[] = { { .type = EventEffectType::GOTO_PAGE, .amount = 2 } };
inline constexpr EventEffect TINKER_TIME_TO_POWER[] = { { .type = EventEffectType::GOTO_PAGE, .amount = 3 } };
inline constexpr EventEffect TINKER_TIME_TO_SKILL[] = { { .type = EventEffectType::GOTO_PAGE, .amount = 4 } };

inline constexpr EventOption TINKER_TIME_ACCEPT[] = {
    { .label = "Accepter",
      .cost_benefit = "Créez une carte sur mesure.",
      .result = "Elle sort ses outils avec un sourire inquiétant.",
      .effects = TINKER_TIME_TO_TYPE },
};

inline constexpr EventOption TINKER_TIME_TYPE[] = {
    { .label = "Arme",
      .cost_benefit = "Créez une Attaque.",
      .result = "Une lame de bric et de broc prend forme.",
      .effects = TINKER_TIME_TO_ATTACK },
    { .label = "Gadget",
      .cost_benefit = "Créez un Pouvoir.",
      .result = "Un mécanisme bourdonnant prend forme.",
      .effects = TINKER_TIME_TO_POWER },
    { .label = "Protecteur",
      .cost_benefit = "Créez une Compétence.",
      .result = "Une carapace bancale prend forme.",
      .effects = TINKER_TIME_TO_SKILL },
};

// The riders are the same for the three types, only the card of "Amélioration" changes.
#define TINKER_TIME_RIDERS(UPGRADED)                                                                                   \
    { .label = "Amélioration",                                                                                         \
      .cost_benefit = "Ajoutez la carte, améliorée.",                                                                  \
      .result = "L'ingénieure peaufine son œuvre : la carte rejoint votre deck, améliorée.",                           \
      .effects = UPGRADED },                                                                                           \
    { .label = "Chaos",                                                                                                \
      .cost_benefit = "Ajoutez une carte incolore aléatoire.",                                                         \
      .result = "L'atrociteur s'emballe et crache une carte inattendue.",                                              \
      .effects = TINKER_TIME_CHAOS },                                                                                  \
    { .label = "Sagesse",                                                                                              \
      .cost_benefit = "Choisissez 1 carte parmi 3.",                                                                   \
      .result = "L'ingénieure vous tend trois plans. Choisissez-en un.",                                               \
      .effects = TINKER_TIME_WISDOM }

inline constexpr EventOption TINKER_TIME_RIDER_ATTACK[] = { TINKER_TIME_RIDERS(TINKER_TIME_ATTACK_UPGRADED) };
inline constexpr EventOption TINKER_TIME_RIDER_POWER[] = { TINKER_TIME_RIDERS(TINKER_TIME_POWER_UPGRADED) };
inline constexpr EventOption TINKER_TIME_RIDER_SKILL[] = { TINKER_TIME_RIDERS(TINKER_TIME_SKILL_UPGRADED) };
#undef TINKER_TIME_RIDERS

inline constexpr EventPage TINKER_TIME_PAGES[] = {
    { .text = "Après avoir traversé un océan de cadavres, vous tombez sur une ingénieure en train de farfouiller. "
              "« Salut ! Tu as l'air compétent... J'ai besoin de quelqu'un pour tester mon prochain atrociteur ! »",
      .options = TINKER_TIME_ACCEPT },
    { .text = "« Parfait ! Quel genre d'engin veux-tu ? » Une arme, un gadget ou un protecteur.",
      .options = TINKER_TIME_TYPE },
    { .text = "Une arme. « Et quel petit supplément y ajouter ? »", .options = TINKER_TIME_RIDER_ATTACK },
    { .text = "Un gadget. « Et quel petit supplément y ajouter ? »", .options = TINKER_TIME_RIDER_POWER },
    { .text = "Un protecteur. « Et quel petit supplément y ajouter ? »", .options = TINKER_TIME_RIDER_SKILL },
};

inline constexpr EventDef TINKER_TIME = {
    .id = "TINKER_TIME",
    .title = "Bricolage",
    .acts = EVENT_ACT_3,
    .pages = TINKER_TIME_PAGES,
};

} // namespace engine::events
