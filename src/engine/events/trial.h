#pragma once
// Le procès (TRIAL), Act 3. Choices: bible/data/event_choices/c.json.
// Page 0: accept (or "reject", which is not allowed) -> page 1: accept the verdict, or double down (lethal).
// Accepting draws one of the three trials at random (pages 2 to 4): the merchant, the noble, the nondescript.
// Guessed values (marked unknown in the reference): the consequence of "Double Down" is death, and the texts of the
// pages after the first one, which the reference does not give.
#include "engine/event_script.h"

namespace engine::events {

// Guessed: "Double Down" has lethal consequences, the value is not documented.
inline constexpr int16_t TRIAL_DOUBLE_DOWN_HP = 999;

inline constexpr EventEffect TRIAL_TO_WARNING[] = {
    { .type = EventEffectType::GOTO_PAGE, .amount = 1 },
};

// The trial is one of the pages 2, 3 and 4.
inline constexpr EventEffect TRIAL_TO_RANDOM_TRIAL[] = {
    { .type = EventEffectType::GOTO_PAGE, .amount = 2, .enchant_amount = 3 },
};

inline constexpr EventEffect TRIAL_DOUBLE_DOWN[] = {
    { .type = EventEffectType::LOSE_HP, .amount = TRIAL_DOUBLE_DOWN_HP },
};

inline constexpr EventEffect TRIAL_MERCHANT_GUILTY[] = {
    { .type = EventEffectType::ADD_CURSE, .card = CardId::REGRET },
    { .type = EventEffectType::GAIN_RELIC },
    { .type = EventEffectType::GAIN_RELIC },
};

inline constexpr EventEffect TRIAL_MERCHANT_INNOCENT[] = {
    { .type = EventEffectType::ADD_CURSE, .card = CardId::SHAME },
    { .type = EventEffectType::UPGRADE_CARD, .amount = 2 },
};

inline constexpr EventEffect TRIAL_NOBLE_GUILTY[] = {
    { .type = EventEffectType::HEAL, .amount = 10 },
};

inline constexpr EventEffect TRIAL_NOBLE_INNOCENT[] = {
    { .type = EventEffectType::ADD_CURSE, .card = CardId::REGRET },
    { .type = EventEffectType::GAIN_GOLD, .amount = 300 },
};

inline constexpr EventEffect TRIAL_NONDESCRIPT_GUILTY[] = {
    { .type = EventEffectType::ADD_CURSE, .card = CardId::DOUBT },
    { .type = EventEffectType::CARD_REWARD, .amount = 2 },
};

inline constexpr EventEffect TRIAL_NONDESCRIPT_INNOCENT[] = {
    { .type = EventEffectType::ADD_CURSE, .card = CardId::DOUBT },
    { .type = EventEffectType::TRANSFORM_CARD, .amount = 2 },
};

inline constexpr EventOption TRIAL_START[] = {
    { .label = "Accepter",
      .cost_benefit = "Devenez le Décideur.",
      .result = "Vous prenez place sur l'estrade, sous les acclamations.",
      .effects = TRIAL_TO_WARNING },
    { .label = "Refuser",
      .cost_benefit = "Vous n'en avez pas le droit.",
      .result = "« Le Décideur n'a pas le droit de refuser. » Des gardes vous poussent vers l'estrade.",
      .effects = TRIAL_TO_WARNING },
};

inline constexpr EventOption TRIAL_WARNING[] = {
    { .label = "Accepter",
      .cost_benefit = "Un procès est tiré au sort.",
      .result = "On amène l'accusé devant vous.",
      .effects = TRIAL_TO_RANDOM_TRIAL },
    { .label = "Insister",
      .cost_benefit = "Conséquences mortelles.",
      .result = "Vous défiez la cour. Elle ne le prend pas bien.",
      .effects = TRIAL_DOUBLE_DOWN },
};

inline constexpr EventOption TRIAL_MERCHANT[] = {
    { .label = "Coupable",
      .cost_benefit = "2 reliques, ajoute Regrets.",
      .result = "Le marchand est condamné ; ses biens vous reviennent. Vous obtenez 2 reliques, et des Regrets.",
      .effects = TRIAL_MERCHANT_GUILTY },
    { .label = "Innocent",
      .cost_benefit = "Améliore 2 cartes, ajoute Honte.",
      .result = "Le marchand vous remercie. Choisissez 2 cartes à améliorer ; vous obtenez la Honte.",
      .effects = TRIAL_MERCHANT_INNOCENT },
};

inline constexpr EventOption TRIAL_NOBLE[] = {
    { .label = "Coupable",
      .cost_benefit = "Récupérez 10 PV.",
      .result = "Le noble est condamné sous les vivats. Vous vous sentez mieux (+10 PV).",
      .effects = TRIAL_NOBLE_GUILTY },
    { .label = "Innocent",
      .cost_benefit = "Gagnez 300 Or, ajoute Regrets.",
      .result = "Le noble vous glisse une bourse bien remplie (+300 Or). Vous obtenez des Regrets.",
      .effects = TRIAL_NOBLE_INNOCENT },
};

inline constexpr EventOption TRIAL_NONDESCRIPT[] = {
    { .label = "Coupable",
      .cost_benefit = "2 choix de carte, ajoute Doute.",
      .result = "La cour vous récompense. Vous obtenez le Doute ; choisissez une carte, puis une autre.",
      .effects = TRIAL_NONDESCRIPT_GUILTY },
    { .label = "Innocent",
      .cost_benefit = "Transforme 2 cartes, ajoute Doute.",
      .result = "L'accusé s'éclipse. Vous obtenez le Doute ; choisissez 2 cartes à transformer.",
      .effects = TRIAL_NONDESCRIPT_INNOCENT },
};

inline constexpr EventPage TRIAL_PAGES[] = {
    { .text = "Vous rejoignez une file entrant dans un énorme bâtiment. Sous une arche dorée, des cors beuglent, "
              "des confettis jaillissent : « Citoyen -1, vous êtes LE DÉCIDEUR pour le Procès d'aujourd'hui. »",
      .options = TRIAL_START },
    { .text = "Le juge vous fixe : « Le Décideur ne peut se dérober. Rendez votre verdict... ou insistez, et assumez "
              "les conséquences. »",
      .options = TRIAL_WARNING },
    { .text = "Un marchand est à la barre, accusé d'avoir vendu des reliques truquées. Coupable ou innocent ?",
      .options = TRIAL_MERCHANT },
    { .text = "Un noble drapé de soie est à la barre, accusé de crimes que personne ne détaille. Coupable ou "
              "innocent ?",
      .options = TRIAL_NOBLE },
    { .text = "Un individu quelconque, que personne ne semble reconnaître, est à la barre. Coupable ou innocent ?",
      .options = TRIAL_NONDESCRIPT },
};

inline constexpr EventDef TRIAL = {
    .id = "TRIAL",
    .title = "Le procès",
    .acts = EVENT_ACT_3,
    .pages = TRIAL_PAGES,
};

} // namespace engine::events
