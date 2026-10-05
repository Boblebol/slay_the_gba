#include "engine/event_manager.h"
#include "engine/run.h"
#include <cstdio>
#include <algorithm>

namespace engine {

namespace {

void safe_copy(char* dest, size_t dest_len, std::string_view src) {
    if (!dest || dest_len == 0) return;
    size_t count = std::min(dest_len - 1, src.size());
    for (size_t i = 0; i < count; ++i) {
        dest[i] = src[i];
    }
    dest[count] = '\0';
}

} // namespace

EventEncounter EventManager::get_encounter_for_floor(uint8_t floor, uint32_t seed) {
    uint32_t idx = (seed + floor) % 4;
    EventEncounter enc;

    if (idx == 0) {
        enc.id = "BATTLEWORN_DUMMY";
        enc.title = "Mannequin d'Entraînement";
        enc.dialogue = "« BZZZT ! SÉQUENCE D'ENTRAÎNEMENT INITIÉE ! Réglez vos coups ou subissez une humiliation létale ! »";
        
        EventChoice c1;
        c1.label = "Attaque acharnée";
        c1.cost_benefit = "Subissez 6 dégâts, gagnez 60 Or.";
        c1.outcome_type = EventOutcomeType::GOLD;
        c1.value = 60;
        c1.result_message = "Vous frappez sans retenue ! Le mannequin explose en pièces d'or.";
        enc.choices.push_back(c1);

        EventChoice c2;
        c2.label = "Réparer les rouages";
        c2.cost_benefit = "Améliorez une carte de votre deck.";
        c2.outcome_type = EventOutcomeType::UPGRADE_CARD;
        c2.value = 0;
        c2.result_message = "Vous étudiez la mécanique interne. Une de vos cartes est améliorée (+) !";
        enc.choices.push_back(c2);

        EventChoice c3;
        c3.label = "Prendre la fuite";
        c3.cost_benefit = "Passez votre chemin sans risque.";
        c3.outcome_type = EventOutcomeType::NONE;
        c3.value = 0;
        c3.result_message = "Vous quittez discrètement la salle.";
        enc.choices.push_back(c3);
    } else if (idx == 1) {
        enc.id = "COLOSSAL_FLOWER";
        enc.title = "Fleur Colossale";
        enc.dialogue = "Une fleur cyclopéenne exsude un parfum suave. Du nectar doré perle sur ses pétales tranchants comme des rasoirs.";

        EventChoice c1;
        c1.label = "Boire le nectar doré";
        c1.cost_benefit = "Récupérez 20 PV.";
        c1.outcome_type = EventOutcomeType::HEAL;
        c1.value = 20;
        c1.result_message = "Le nectar restaure vos forces (+20 PV) !";
        enc.choices.push_back(c1);

        EventChoice c2;
        c2.label = "Arracher l'Épine Ancestrale";
        c2.cost_benefit = "Subissez 10 dégâts, obtenez la Relique Vajra (+1 Force).";
        c2.outcome_type = EventOutcomeType::RELIC;
        c2.value = 10;
        c2.extra_id = "VAJRA";
        c2.result_message = "Vous coupez l'épine au prix d'une vive blessure. Vous obtenez Vajra !";
        enc.choices.push_back(c2);

        EventChoice c3;
        c3.label = "Contourner la plante";
        c3.cost_benefit = "Ne prenez aucun risque.";
        c3.outcome_type = EventOutcomeType::NONE;
        c3.value = 0;
        c3.result_message = "Vous contournez la fleur avec précaution.";
        enc.choices.push_back(c3);
    } else if (idx == 2) {
        enc.id = "ABYSSAL_BATHS";
        enc.title = "Bains Abyssaux";
        enc.dialogue = "Des bassins fumants émettent des vapeurs minérales revigorantes. Au fond d'une vasque, des pièces d'or scintillent.";

        EventChoice c1;
        c1.label = "Se prélasser dans le bain";
        c1.cost_benefit = "Récupérez 25 PV, coûte 35 Or.";
        c1.outcome_type = EventOutcomeType::HEAL;
        c1.value = 25;
        c1.result_message = "Les vapeurs chaudes dissolvent vos courbatures (+25 PV).";
        enc.choices.push_back(c1);

        EventChoice c2;
        c2.label = "Plonger dans les profondeurs";
        c2.cost_benefit = "Gagnez 80 Or, subissez 8 dégâts de brûlure.";
        c2.outcome_type = EventOutcomeType::GOLD;
        c2.value = 80;
        c2.result_message = "L'eau bouillante vous brûle mais vous repêchez 80 Pièces d'or !";
        enc.choices.push_back(c2);

        EventChoice c3;
        c3.label = "Repartir";
        c3.cost_benefit = "Ignorer les bains.";
        c3.outcome_type = EventOutcomeType::NONE;
        c3.value = 0;
        c3.result_message = "Vous reprenez votre ascension sans vous attarder.";
        enc.choices.push_back(c3);
    } else {
        enc.id = "MYSTIC_ALTAR";
        enc.title = "Autel de la Tour";
        enc.dialogue = "Un monolithe d'obsidienne pulse au rythme d'une énergie ancienne. Une cavité d'offrande attend votre tribut.";

        EventChoice c1;
        c1.label = "Offrande de sang";
        c1.cost_benefit = "Sacrifiez 7 PV, gagnez 90 Or.";
        c1.outcome_type = EventOutcomeType::GOLD;
        c1.value = 90;
        c1.result_message = "L'autel absorbe votre sang et fait jaillir 90 Pièces d'or !";
        enc.choices.push_back(c1);

        EventChoice c2;
        c2.label = "Prière méditative";
        c2.cost_benefit = "Améliorez une carte de votre deck.";
        c2.outcome_type = EventOutcomeType::UPGRADE_CARD;
        c2.value = 0;
        c2.result_message = "Une bénédiction mystique forge l'une de vos cartes en version (+) !";
        enc.choices.push_back(c2);

        EventChoice c3;
        c3.label = "S'incliner et partir";
        c3.cost_benefit = "Ne rien sacrifier.";
        c3.outcome_type = EventOutcomeType::NONE;
        c3.value = 0;
        c3.result_message = "Vous saluez l'autel et poursuivez votre chemin.";
        enc.choices.push_back(c3);
    }

    return enc;
}

bool EventManager::apply_choice(RunManager& run, const EventChoice& choice, char* out_msg, size_t max_len) {
    switch (choice.outcome_type) {
        case EventOutcomeType::HEAL: {
            run.hp = std::min<int16_t>(run.max_hp, run.hp + choice.value);
            safe_copy(out_msg, max_len, choice.result_message);
            break;
        }
        case EventOutcomeType::DAMAGE: {
            run.hp = std::max<int16_t>(1, run.hp - choice.value);
            safe_copy(out_msg, max_len, choice.result_message);
            break;
        }
        case EventOutcomeType::GOLD: {
            if (choice.value == 60) {
                run.hp = std::max<int16_t>(1, run.hp - 6);
            } else if (choice.value == 80) {
                run.hp = std::max<int16_t>(1, run.hp - 8);
            } else if (choice.value == 90) {
                run.hp = std::max<int16_t>(1, run.hp - 7);
            }
            run.gold += choice.value;
            safe_copy(out_msg, max_len, choice.result_message);
            break;
        }
        case EventOutcomeType::UPGRADE_CARD: {
            bool upgraded = false;
            for (size_t i = 0; i < run.master_deck.size(); ++i) {
                if (!run.master_deck[i].upgraded) {
                    run.master_deck[i].upgraded = true;
                    upgraded = true;
                    break;
                }
            }
            if (upgraded) {
                safe_copy(out_msg, max_len, choice.result_message);
            } else {
                safe_copy(out_msg, max_len, "Toutes vos cartes sont déjà améliorées !");
            }
            break;
        }
        case EventOutcomeType::RELIC: {
            run.hp = std::max<int16_t>(1, run.hp - choice.value);
            if (!choice.extra_id.empty()) {
                run.add_relic(choice.extra_id);
            }
            safe_copy(out_msg, max_len, choice.result_message);
            break;
        }
        case EventOutcomeType::POTION: {
            if (!run.potion_belt_full()) {
                run.potions.push_back(choice.extra_id.empty() ? "FIRE_POTION" : choice.extra_id);
            }
            safe_copy(out_msg, max_len, choice.result_message);
            break;
        }
        case EventOutcomeType::NONE:
        default: {
            safe_copy(out_msg, max_len, choice.result_message);
            break;
        }
    }
    return true;
}

} // namespace engine
