#include "gba/card_text.h"

#include "bn_algorithm.h"
#include "bn_sstream.h"

namespace gba
{

namespace
{

bn::string_view base_name(engine::CardId id)
{
    switch(id)
    {
    case engine::CardId::STRIKE:
        return "Frappe";
    case engine::CardId::DEFEND:
        return "Défense";
    case engine::CardId::BASH:
        return "Heurt";
    case engine::CardId::IRON_WAVE:
        return "Vague de fer";
    case engine::CardId::CLEAVE:
        return "Enchaînement";
    case engine::CardId::ANGER:
        return "Colère";
    case engine::CardId::METALLICIZE:
        return "Armure de pierre";
    case engine::CardId::NEUTRALIZE:
        return "Neutralisation";
    case engine::CardId::SURVIVOR:
        return "Survivant";
    case engine::CardId::POISON_STAB:
        return "Poignard empoisonné";
    case engine::CardId::DEADLY_POISON:
        return "Poison mortel";
    case engine::CardId::ZAP:
        return "Éclair";
    case engine::CardId::DUALCAST:
        return "Activation double";
    case engine::CardId::FALLING_STAR:
        return "Chute d'étoile";
    case engine::CardId::VENERATE:
        return "Vénération";
    case engine::CardId::UNLEASH:
        return "Déchaînement";
    case engine::CardId::BODYGUARD:
        return "Garde du corps";
    case engine::CardId::TWIN_STRIKE:
        return "Frappe jumelée";
    case engine::CardId::POMMEL_STRIKE:
        return "Frappe du pommeau";
    case engine::CardId::SHRUG_IT_OFF:
        return "Même pas mal";
    case engine::CardId::TAUNT:
        return "Provocation";
    case engine::CardId::SLICE:
        return "Trancher";
    case engine::CardId::BACKFLIP:
        return "Salto arrière";
    case engine::CardId::SUCKER_PUNCH:
        return "Coup bas";
    case engine::CardId::DAGGER_SPRAY:
        return "Pluie de couteaux";
    case engine::CardId::BEAM_CELL:
        return "Rayon cellulaire";
    case engine::CardId::BALL_LIGHTNING:
        return "Foudre en boule";
    case engine::CardId::COLD_SNAP:
        return "Vague de froid";
    case engine::CardId::LEAP:
        return "Bond";
    case engine::CardId::SOLAR_STRIKE:
        return "Frappe solaire";
    case engine::CardId::GATHER_LIGHT:
        return "Lumière rassemblée";
    case engine::CardId::CELESTIAL_MIGHT:
        return "Puissance céleste";
    case engine::CardId::PULL_AGGRO:
        return "Râblure";
    case engine::CardId::DRAIN_POWER:
        return "Drain de puissance";
    case engine::CardId::BODY_SLAM:
        return "Plaquage";
    case engine::CardId::DEFLECT:
        return "Déviation";
    case engine::CardId::COOLHEADED:
        return "Sang-froid";
    case engine::CardId::SWEEPING_BEAM:
        return "Faisceau circulaire";
    case engine::CardId::THUNDERCLAP:
        return "Coup de tonnerre";
    case engine::CardId::DISMANTLE:
        return "Démantèlement";
    case engine::CardId::INFLAME:
        return "Enflammer";
    case engine::CardId::PERFECTED_STRIKE:
        return "Frappe perfectionnée";
    case engine::CardId::BLUDGEON:
        return "Gourdin";
    case engine::CardId::BULLY:
        return "Harcèlement";
    case engine::CardId::SWORD_BOOMERANG:
        return "Épée boomerang";
    case engine::CardId::BREAK:
        return "Briser";
    case engine::CardId::CONFLAGRATION:
        return "Incendie";
    case engine::CardId::EXPECT_A_FIGHT:
        return "Paré au combat";
    case engine::CardId::PILLAGE:
        return "Pillage";
    case engine::CardId::UPPERCUT:
        return "Uppercut";
    case engine::CardId::FIGHT_ME:
        return "Viens te battre !";
    case engine::CardId::LEG_SWEEP:
        return "Balayage";
    case engine::CardId::HAZE:
        return "Brouillard";
    case engine::CardId::PRECISE_CUT:
        return "Entaille";
    case engine::CardId::BOUNCING_FLASK:
        return "Fiole rebondissante";
    case engine::CardId::FLECHETTES:
        return "Fléchettes";
    case engine::CardId::FOOTWORK:
        return "Jeu de jambes";
    case engine::CardId::ESCAPE_PLAN:
        return "Plan d'évasion";
    case engine::CardId::DASH:
        return "Précipitation";
    case engine::CardId::BUBBLE_BUBBLE:
        return "Venin mousseux";
    case engine::CardId::OUTBREAK:
        return "Épidémie";
    case engine::CardId::SHADOW_SHIELD:
        return "Bouclier ténébreux";
    case engine::CardId::METEOR_STRIKE:
        return "Frappe météorique";
    case engine::CardId::GLACIER:
        return "Glacier";
    case engine::CardId::ICE_LANCE:
        return "Javelot de glace";
    case engine::CardId::SKIM:
        return "Parcourir";
    case engine::CardId::ALL_FOR_ONE:
        return "Tous pour un";
    case engine::CardId::NULL_CARD:
        return "Valeur nulle";
    case engine::CardId::GO_FOR_THE_EYES:
        return "Viser les yeux";
    case engine::CardId::SCRAPE:
        return "Éraflure";
    case engine::CardId::BURY:
        return "Enterrer";
    case engine::CardId::MISERY:
        return "Misère";
    case engine::CardId::UNDEATH:
        return "Mort-vivant";
    case engine::CardId::ASHEN_STRIKE:
        return "Frappe cendrée";
    case engine::CardId::TREMBLE:
        return "Intimidation";
    case engine::CardId::EVIL_EYE:
        return "Mauvais œil";
    case engine::CardId::MOLTEN_FIST:
        return "Poing en fusion";
    case engine::CardId::CINDER:
        return "Scorie";
    case engine::CardId::SECOND_WIND:
        return "Second souffle";
    case engine::CardId::TRUE_GRIT:
        return "Virilité";
    case engine::CardId::DOMINATE:
        return "Domination";
    case engine::CardId::FIEND_FIRE:
        return "Feu démoniaque";
    case engine::CardId::THRASH:
        return "Furie";
    case engine::CardId::IMPERVIOUS:
        return "Insensible";
    case engine::CardId::PACTS_END:
        return "Pacte conclu";
    case engine::CardId::NOT_YET:
        return "Pas encore";
    case engine::CardId::ADRENALINE:
        return "Adrénaline";
    case engine::CardId::CALCULATED_GAMBLE:
        return "Coup de poker";
    case engine::CardId::MIRAGE:
        return "Mirage";
    case engine::CardId::RAINBOW:
        return "Arc-en-ciel";
    case engine::CardId::CHILL:
        return "Frisson";
    case engine::CardId::FUSION:
        return "Fusion";
    case engine::CardId::REBOOT:
        return "Redémarrage";
    case engine::CardId::SUPERCRITICAL:
        return "Supercritique";
    case engine::CardId::DOUBLE_ENERGY:
        return "Énergie double";
    case engine::CardId::SHARED_FATE:
        return "Destin partagé";
    case engine::CardId::WISP:
        return "Feu follet";
    case engine::CardId::PUTREFY:
        return "Putréfaction";
    case engine::CardId::REANIMATE:
        return "Réanimation";
    case engine::CardId::AFTERLIFE:
        return "Vie après la mort";
    case engine::CardId::BLOOD_WALL:
        return "Mur de sang";
    case engine::CardId::BLOODLETTING:
        return "Saignée";
    case engine::CardId::BREAKTHROUGH:
        return "Percée";
    case engine::CardId::FORGOTTEN_RITUAL:
        return "Rituel oublié";
    case engine::CardId::HEMOKINESIS:
        return "Hémokinésie";
    case engine::CardId::MANGLE:
        return "Estropier";
    case engine::CardId::OFFERING:
        return "Offrande";
    case engine::CardId::SETUP_STRIKE:
        return "Frappe préparatoire";
    case engine::CardId::SPITE:
        return "Rancune";
    case engine::CardId::WHIRLWIND:
        return "Tourbillon";
    case engine::CardId::ANTICIPATE:
        return "Anticipation";
    case engine::CardId::ASSASSINATE:
        return "Assassiner";
    case engine::CardId::BACKSTAB:
        return "Coup sournois";
    case engine::CardId::BLUR:
        return "Vision trouble";
    case engine::CardId::DODGE_AND_ROLL:
        return "Dérobade";
    case engine::CardId::FINISHER:
        return "Coup décisif";
    case engine::CardId::MALAISE:
        return "Malaise";
    case engine::CardId::MEMENTO_MORI:
        return "Memento mori";
    case engine::CardId::MURDER:
        return "Meurtre";
    case engine::CardId::PIERCING_WAIL:
        return "Hurlement perçant";
    case engine::CardId::PREDATOR:
        return "Prédateur";
    case engine::CardId::SIDESTEP:
        return "Pas de côté";
    case engine::CardId::SKEWER:
        return "Embrocher";
    case engine::CardId::SUPPRESS:
        return "Répression";
    case engine::CardId::EXPERTISE:
        return "Expertise";
    case engine::CardId::SNAKEBITE:
        return "Morsure de serpent";
    case engine::CardId::BOOT_SEQUENCE:
        return "Programme de démarrage";
    case engine::CardId::CHARGE_BATTERY:
        return "Piles de recharge";
    case engine::CardId::FTL:
        return "FTL";
    case engine::CardId::HELIX_DRILL:
        return "Perceuse hélicoïdale";
    case engine::CardId::LIGHTNING_ROD:
        return "Paratonnerre";
    case engine::CardId::TEMPEST:
        return "Tempête";
    case engine::CardId::CRUSH_UNDER:
        return "Broyer";
    case engine::CardId::GLITTERSTREAM:
        return "Courant pailleté";
    case engine::CardId::HEAVENLY_DRILL:
        return "Foreuse céleste";
    case engine::CardId::HEGEMONY:
        return "Hégémonie";
    case engine::CardId::KNOW_THY_PLACE:
        return "Hiérarchie";
    case engine::CardId::LUNAR_BLAST:
        return "Déflagration lunaire";
    case engine::CardId::PROPHESIZE:
        return "Prophétie";
    case engine::CardId::SUPERMASSIVE:
        return "Supermassif";
    case engine::CardId::CONVERGENCE:
        return "Convergence";
    case engine::CardId::DEATH_MARCH:
        return "Marche funeste";
    case engine::CardId::DEFILE:
        return "Profanation";
    case engine::CardId::DEFY:
        return "Défi";
    case engine::CardId::DELAY:
        return "Délai";
    case engine::CardId::ENFEEBLING_TOUCH:
        return "Toucher atrophiant";
    case engine::CardId::FEAR:
        return "Terreur";
    case engine::CardId::INVOKE:
        return "Adjuration";
    case engine::CardId::PARSE:
        return "Décomposition";
    case engine::CardId::PULL_FROM_BELOW:
        return "Six pieds sous terre";
    case engine::CardId::REAP:
        return "Moisson";
    case engine::CardId::SOW:
        return "Semence";
    case engine::CardId::ERADICATE:
        return "Éradication";
    default:
        return "???";
    }
}

}

void card_name(const engine::DeckCard& card, bn::istring& out)
{
    out.clear();
    out.append(base_name(card.id));

    if(card.upgraded)
    {
        out.push_back('+');
    }
}

void card_description(const engine::DeckCard& card, bn::istring& out)
{
    out.clear();
    bn::ostringstream stream(out);
    int value = engine::CardDef::get_value(card.id, card.upgraded) + card.bonus;
    int secondary = engine::CardDef::get_secondary(card.id, card.upgraded);

    switch(card.id)
    {
    case engine::CardId::STRIKE:
    case engine::CardId::UNLEASH:
        stream << "Inflige " << value << " dégâts.";
        break;
    case engine::CardId::DEFEND:
        stream << "Gagne " << value << " d'Armure.";
        break;
    case engine::CardId::BASH:
        stream << "Inflige " << value << " dégâts. Vulnérable " << secondary << ".";
        break;
    case engine::CardId::IRON_WAVE:
        stream << "Gagne " << secondary << " d'Armure. Inflige " << value << " dégâts.";
        break;
    case engine::CardId::CLEAVE:
        stream << "Inflige " << value << " dégâts à TOUS les ennemis.";
        break;
    case engine::CardId::ANGER:
        stream << "Inflige " << value << " dégâts. Copie dans la défausse.";
        break;
    case engine::CardId::METALLICIZE:
        stream << "Fin de tour : gagne " << value << " d'Armure.";
        break;
    case engine::CardId::NEUTRALIZE:
        stream << "Inflige " << value << " dégâts. Affaibli " << secondary << ".";
        break;
    case engine::CardId::SURVIVOR:
        stream << "Gagne " << value << " d'Armure. Défausse 1 carte.";
        break;
    case engine::CardId::POISON_STAB:
        stream << "Inflige " << value << " dégâts. " << secondary << " de Poison.";
        break;
    case engine::CardId::DEADLY_POISON:
        stream << "Applique " << secondary << " de Poison.";
        break;
    case engine::CardId::ZAP:
        stream << "Canalise " << secondary << " Orbe de foudre.";
        break;
    case engine::CardId::DUALCAST:
        stream << "Évoque votre Orbe la plus ancienne 2 fois.";
        break;
    case engine::CardId::FALLING_STAR:
        stream << "Inflige " << value << " dégâts. Gagne " << secondary << " Étoile(s).";
        break;
    case engine::CardId::VENERATE:
        stream << "Gagne " << value << " d'Armure et " << secondary << " Étoiles.";
        break;
    case engine::CardId::BODYGUARD:
        stream << "Gagne " << value << " d'Armure. Invoque Titos.";
        break;
    case engine::CardId::TWIN_STRIKE:
        stream << "Inflige " << value << " dégâts " << secondary << " fois.";
        break;
    case engine::CardId::POMMEL_STRIKE:
        stream << "Inflige " << value << " dégâts. Pioche " << secondary << (secondary > 1 ? " cartes." : " carte.");
        break;
    case engine::CardId::SHRUG_IT_OFF:
        stream << "Gagne " << value << " d'Armure. Pioche " << secondary << (secondary > 1 ? " cartes." : " carte.");
        break;
    case engine::CardId::TAUNT:
        stream << "Gagne " << value << " d'Armure. Vulnérable " << secondary << ".";
        break;
    case engine::CardId::SLICE:
        stream << "Inflige " << value << " dégâts.";
        break;
    case engine::CardId::BACKFLIP:
        stream << "Gagne " << value << " d'Armure. Pioche " << secondary << (secondary > 1 ? " cartes." : " carte.");
        break;
    case engine::CardId::SUCKER_PUNCH:
        stream << "Inflige " << value << " dégâts. Affaibli " << secondary << ".";
        break;
    case engine::CardId::DAGGER_SPRAY:
        stream << "Inflige " << value << " dégâts à TOUS, " << secondary << " fois.";
        break;
    case engine::CardId::BEAM_CELL:
        stream << "Inflige " << value << " dégâts. Vulnérable " << secondary << ".";
        break;
    case engine::CardId::BALL_LIGHTNING:
        stream << "Inflige " << value << " dégâts. Canalise 1 Foudre.";
        break;
    case engine::CardId::COLD_SNAP:
        stream << "Inflige " << value << " dégâts. Canalise 1 Givre.";
        break;
    case engine::CardId::LEAP:
        stream << "Gagne " << value << " d'Armure.";
        break;
    case engine::CardId::SOLAR_STRIKE:
        stream << "Inflige " << value << " dégâts. Gagne " << secondary << " Étoile(s).";
        break;
    case engine::CardId::GATHER_LIGHT:
        stream << "Gagne " << value << " d'Armure et " << secondary << " Étoile(s).";
        break;
    case engine::CardId::CELESTIAL_MIGHT:
        stream << "Inflige " << value << " dégâts " << secondary << " fois.";
        break;
    case engine::CardId::PULL_AGGRO:
        stream << "Invoque Titos (" << secondary << " PV). Gagne " << value << " d'Armure.";
        break;
    case engine::CardId::DRAIN_POWER:
        stream << "Inflige " << value << " dégâts. Améliore " << secondary << " cartes.";
        break;
    case engine::CardId::BODY_SLAM:
        stream << "Inflige autant de dégâts que d'Armure.";
        break;
    case engine::CardId::DEFLECT:
        stream << "Gagne " << value << " d'Armure.";
        break;
    case engine::CardId::COOLHEADED:
        stream << "Canalise 1 Givre. Pioche " << secondary << (secondary > 1 ? " cartes." : " carte.");
        break;
    case engine::CardId::SWEEPING_BEAM:
        stream << "Inflige " << value << " dégâts à TOUS. Pioche " << secondary << (secondary > 1 ? " cartes." : " carte.");
        break;
    case engine::CardId::THUNDERCLAP:
        stream << "Inflige " << value << " dégâts à TOUS. Vulnérable " << secondary << ".";
        break;
    case engine::CardId::DISMANTLE:
        stream << "Inflige " << value << " dégâts, 2 fois si Vulnérable.";
        break;
    case engine::CardId::INFLAME:
        stream << "Gagne " << value << " de Force.";
        break;
    case engine::CardId::PERFECTED_STRIKE:
        stream << "Inflige " << value << " dégâts, +" << secondary << " par carte Frappe.";
        break;
    case engine::CardId::BLUDGEON:
        stream << "Inflige " << value << " dégâts.";
        break;
    case engine::CardId::BULLY:
        stream << "Inflige " << value << " dégâts, +" << secondary << " par Vulnérable.";
        break;
    case engine::CardId::SWORD_BOOMERANG:
        stream << "Inflige " << value << " dégâts au hasard, " << secondary << " fois.";
        break;
    case engine::CardId::BREAK:
        stream << "Inflige " << value << " dégâts. Vulnérable " << secondary << ".";
        break;
    case engine::CardId::CONFLAGRATION:
        stream << "Inflige " << value << " dégâts à TOUS, " << secondary << " fois.";
        break;
    case engine::CardId::EXPECT_A_FIGHT:
        stream << "Gagne " << value << " d'Armure, +" << secondary << " par Force.";
        break;
    case engine::CardId::PILLAGE:
        stream << "Inflige " << value << ". Pioche jusqu'à 1 non-Attaque.";
        break;
    case engine::CardId::UPPERCUT:
        stream << "Inflige " << value << ". Affaibli et Vulnérable " << secondary << ".";
        break;
    case engine::CardId::FIGHT_ME:
        stream << value << " dégâts 2 fois. Force +" << secondary << ", ennemi +1.";
        break;
    case engine::CardId::LEG_SWEEP:
        stream << "Affaibli " << secondary << ". Gagne " << value << " d'Armure.";
        break;
    case engine::CardId::HAZE:
        stream << value << " de Poison et Affaibli " << secondary << " à TOUS.";
        break;
    case engine::CardId::PRECISE_CUT:
        stream << "Inflige " << value << " dégâts, -" << secondary << " par carte en main.";
        break;
    case engine::CardId::BOUNCING_FLASK:
        stream << value << " de Poison au hasard, " << secondary << " fois.";
        break;
    case engine::CardId::FLECHETTES:
        stream << "Inflige " << value << " dégâts par Compétence en main.";
        break;
    case engine::CardId::FOOTWORK:
        stream << "Gagne " << value << " de Dextérité.";
        break;
    case engine::CardId::ESCAPE_PLAN:
        stream << "Pioche 1. Si Compétence : " << value << " d'Armure.";
        break;
    case engine::CardId::DASH:
        stream << "Gagne " << secondary << " d'Armure. Inflige " << value << " dégâts.";
        break;
    case engine::CardId::BUBBLE_BUBBLE:
        stream << "Cible empoisonnée : " << value << " de Poison.";
        break;
    case engine::CardId::OUTBREAK:
        stream << value << " de Poison à TOUS, déclenché aussitôt.";
        break;
    case engine::CardId::SHADOW_SHIELD:
        stream << "Gagne " << value << " d'Armure. Canalise 1 Ténèbres.";
        break;
    case engine::CardId::METEOR_STRIKE:
        stream << "Inflige " << value << " dégâts. Canalise " << secondary << " Plasma.";
        break;
    case engine::CardId::GLACIER:
        stream << "Gagne " << value << " d'Armure. Canalise " << secondary << " Givre.";
        break;
    case engine::CardId::ICE_LANCE:
        stream << "Inflige " << value << " dégâts. Canalise " << secondary << " Givre.";
        break;
    case engine::CardId::SKIM:
        stream << "Pioche " << secondary << " cartes.";
        break;
    case engine::CardId::ALL_FOR_ONE:
        stream << "Inflige " << value << ". Reprend les cartes à 0.";
        break;
    case engine::CardId::NULL_CARD:
        stream << "Inflige " << value << ". Affaibli " << secondary << ". 1 Ténèbres.";
        break;
    case engine::CardId::GO_FOR_THE_EYES:
        stream << "Inflige " << value << " dégâts. Affaibli " << secondary << " s'il attaque.";
        break;
    case engine::CardId::SCRAPE:
        stream << "Inflige " << value << ". Pioche " << secondary << ", défausse les non-0.";
        break;
    case engine::CardId::BURY:
        stream << "Inflige " << value << " dégâts.";
        break;
    case engine::CardId::MISERY:
        stream << (card.upgraded ? "Retenue. " : "") << "Inflige " << value << ". Propage ses malus.";
        break;
    case engine::CardId::UNDEATH:
        stream << "Gagne " << value << " d'Armure. Copie dans la défausse.";
        break;
    case engine::CardId::ASHEN_STRIKE:
        stream << "Inflige " << value << " dégâts, +" << secondary << " par carte épuisée.";
        break;
    case engine::CardId::TREMBLE:
        stream << "Vulnérable " << secondary << ". Épuisement.";
        break;
    case engine::CardId::EVIL_EYE:
        stream << "Gagne " << value << " d'Armure, x2 si carte épuisée.";
        break;
    case engine::CardId::MOLTEN_FIST:
        stream << "Inflige " << value << ". Double Vulnérable. Épuis.";
        break;
    case engine::CardId::CINDER:
        stream << "Inflige " << value << ". Épuise 1 carte au hasard.";
        break;
    case engine::CardId::SECOND_WIND:
        stream << "Épuise les non-Attaques : +" << value << " d'Armure.";
        break;
    case engine::CardId::TRUE_GRIT:
        stream << "Gagne " << value << " d'Armure. Épuise 1 carte.";
        break;
    case engine::CardId::DOMINATE:
        stream << "Vulnérable " << secondary << ". Force +1 par Vuln. Épuis.";
        break;
    case engine::CardId::FIEND_FIRE:
        stream << "Épuise la main : " << value << " par carte. Épuis.";
        break;
    case engine::CardId::THRASH:
        stream << value << " dégâts 2 fois. Absorbe 1 Attaque.";
        break;
    case engine::CardId::IMPERVIOUS:
        stream << "Gagne " << value << " d'Armure. Épuisement.";
        break;
    case engine::CardId::PACTS_END:
        stream << "Si " << secondary << " épuisées : " << value << " dégâts à TOUS.";
        break;
    case engine::CardId::NOT_YET:
        stream << "Récupère " << value << " PV. Épuisement.";
        break;
    case engine::CardId::ADRENALINE:
        stream << "Énergie +" << value << ". Pioche " << secondary << " cartes. Épuisement.";
        break;
    case engine::CardId::CALCULATED_GAMBLE:
        stream << (card.upgraded ? "Retenue. " : "") << "Change toute la main. Épuisement.";
        break;
    case engine::CardId::MIRAGE:
        stream << "Armure = Poison des ennemis." << (card.upgraded ? "" : " Épuisement.");
        break;
    case engine::CardId::RAINBOW:
        stream << "Canalise Foudre, Givre, Ténèbres." << (card.upgraded ? "" : " Épuis.");
        break;
    case engine::CardId::CHILL:
        stream << "Canalise 1 Givre par ennemi." << (card.upgraded ? "" : " Épuisement.");
        break;
    case engine::CardId::FUSION:
        stream << "Canalise 1 Plasma." << (card.upgraded ? "" : " Épuisement.");
        break;
    case engine::CardId::REBOOT:
        stream << "Remélange tout. Pioche " << secondary << ". Épuisement.";
        break;
    case engine::CardId::SUPERCRITICAL:
        stream << "Énergie +" << value << ". Épuisement.";
        break;
    case engine::CardId::DOUBLE_ENERGY:
        stream << "Double votre énergie. Épuisement.";
        break;
    case engine::CardId::SHARED_FATE:
        stream << "Force -" << value << ". Ennemi : Force -" << secondary << ". Épuisement.";
        break;
    case engine::CardId::WISP:
        stream << (card.upgraded ? "Retenue. " : "") << "Énergie +" << value << ". Épuisement.";
        break;
    case engine::CardId::PUTREFY:
        stream << "Affaibli et Vulnérable " << secondary << ". Épuisement.";
        break;
    case engine::CardId::REANIMATE:
        stream << "Invoque Titos (" << secondary << " PV). Épuisement.";
        break;
    case engine::CardId::AFTERLIFE:
        stream << "Invoque Titos (" << secondary << " PV). Épuisement.";
        break;
    case engine::CardId::BLOOD_WALL:
        stream << "Perd " << secondary << " PV. Gagne " << value << " d'Armure.";
        break;
    case engine::CardId::BLOODLETTING:
        stream << "Perd " << secondary << " PV. Énergie +" << value << ".";
        break;
    case engine::CardId::BREAKTHROUGH:
        stream << "Perd " << secondary << " PV. Inflige " << value << " dégâts à TOUS.";
        break;
    case engine::CardId::FORGOTTEN_RITUAL:
        stream << "Si carte épuisée : Énergie +" << value << ". Épuis.";
        break;
    case engine::CardId::HEMOKINESIS:
        stream << "Perd " << secondary << " PV. Inflige " << value << " dégâts.";
        break;
    case engine::CardId::MANGLE:
        stream << "Inflige " << value << ". Ennemi : Force -" << secondary << " ce tour.";
        break;
    case engine::CardId::OFFERING:
        stream << "Perd 6 PV. Énergie +" << value << ". Pioche " << secondary << ". Épuis.";
        break;
    case engine::CardId::SETUP_STRIKE:
        stream << "Inflige " << value << " dégâts. Force +" << secondary << " ce tour.";
        break;
    case engine::CardId::SPITE:
        stream << "Inflige " << value << ", " << secondary << " fois si PV perdus ce tour.";
        break;
    case engine::CardId::WHIRLWIND:
        stream << "Inflige " << value << " dégâts à TOUS, X fois.";
        break;
    case engine::CardId::ANTICIPATE:
        stream << "Dextérité +" << value << " jusqu'à la fin du tour.";
        break;
    case engine::CardId::ASSASSINATE:
        stream << "Inné. Inflige " << value << ". Vulnérable " << secondary << ". Épuis.";
        break;
    case engine::CardId::BACKSTAB:
        stream << "Inné. Inflige " << value << " dégâts. Épuisement.";
        break;
    case engine::CardId::BLUR:
        stream << "Gagne " << value << " d'Armure, gardée au tour suivant.";
        break;
    case engine::CardId::DODGE_AND_ROLL:
        stream << "Gagne " << value << " d'Armure, et au tour suivant.";
        break;
    case engine::CardId::FINISHER:
        stream << "Inflige " << value << " par Attaque jouée ce tour.";
        break;
    case engine::CardId::MALAISE:
        stream << (card.upgraded ? "Force -(X+1), Affaibli X+1. Épuisement." : "Force -X, Affaibli X. Épuisement.");
        break;
    case engine::CardId::MEMENTO_MORI:
        stream << "Inflige " << value << ", +" << secondary << " par défausse ce tour.";
        break;
    case engine::CardId::MURDER:
        stream << "Inflige " << value << ", +" << secondary << " par carte piochée ce combat.";
        break;
    case engine::CardId::PIERCING_WAIL:
        stream << "TOUS : Force -" << secondary << " ce tour. Épuisement.";
        break;
    case engine::CardId::PREDATOR:
        stream << "Inflige " << value << ". Tour suivant : pioche " << secondary << ".";
        break;
    case engine::CardId::SIDESTEP:
        stream << "Tour suivant : Énergie +" << value << ".";
        break;
    case engine::CardId::SKEWER:
        stream << "Inflige " << value << " dégâts X fois.";
        break;
    case engine::CardId::SUPPRESS:
        stream << "Inné. Inflige " << value << " dégâts. Affaibli " << secondary << ".";
        break;
    case engine::CardId::EXPERTISE:
        stream << "Pioche " << secondary << " cartes, Retenues ce tour.";
        break;
    case engine::CardId::SNAKEBITE:
        stream << "Retenue. Applique " << secondary << " de Poison.";
        break;
    case engine::CardId::BOOT_SEQUENCE:
        stream << "Inné. Gagne " << value << " d'Armure. Épuisement.";
        break;
    case engine::CardId::CHARGE_BATTERY:
        stream << "Armure +" << value << ". Tour suivant : Énergie +" << secondary << ".";
        break;
    case engine::CardId::FTL:
        stream << "Inflige " << value << ". Pioche 1 si < " << secondary << " cartes jouées.";
        break;
    case engine::CardId::HELIX_DRILL:
        stream << "Inflige " << value << " par énergie dépensée ce tour.";
        break;
    case engine::CardId::LIGHTNING_ROD:
        stream << "Armure +" << value << ". " << secondary << " tours : canalise 1 Foudre.";
        break;
    case engine::CardId::TEMPEST:
        stream << (card.upgraded ? "Canalise X+1 Orbes de foudre." : "Canalise X Orbes de foudre.");
        break;
    case engine::CardId::CRUSH_UNDER:
        stream << "Inflige " << value << " à TOUS. Force -" << secondary << " ce tour.";
        break;
    case engine::CardId::GLITTERSTREAM:
        stream << "Armure +" << value << ". Tour suivant : Armure +" << secondary << ".";
        break;
    case engine::CardId::HEAVENLY_DRILL:
        stream << "Inflige " << value << " X fois. X doublé dès " << secondary << ".";
        break;
    case engine::CardId::HEGEMONY:
        stream << "Inflige " << value << ". Tour suivant : Énergie +" << secondary << ".";
        break;
    case engine::CardId::KNOW_THY_PLACE:
        stream << "Affaibli " << value << ". Vulnérable " << secondary << "." << (card.upgraded ? "" : " Épuisement.");
        break;
    case engine::CardId::LUNAR_BLAST:
        stream << "Inflige " << value << " par Compétence jouée ce tour.";
        break;
    case engine::CardId::PROPHESIZE:
        stream << "Pioche " << secondary << " cartes.";
        break;
    case engine::CardId::SUPERMASSIVE:
        stream << "Inflige " << value << ", +" << secondary << " par carte créée ce combat.";
        break;
    case engine::CardId::CONVERGENCE:
        stream << "Garde la main. Ensuite : +" << value << " ⚡, +" << secondary << " ★.";
        break;
    case engine::CardId::DEATH_MARCH:
        stream << "Inflige " << value << ", +" << secondary << " par carte piochée ce tour.";
        break;
    case engine::CardId::DEFILE:
        stream << "Éthéré. Inflige " << value << " dégâts.";
        break;
    case engine::CardId::DEFY:
        stream << "Éthéré. Gagne " << value << " d'Armure. Affaibli " << secondary << ".";
        break;
    case engine::CardId::DELAY:
        stream << "Armure +" << value << ". Tour suivant : Énergie +" << secondary << ".";
        break;
    case engine::CardId::ENFEEBLING_TOUCH:
        stream << "Éthéré. Ennemi : Force -" << secondary << " ce tour.";
        break;
    case engine::CardId::FEAR:
        stream << "Éthéré. Inflige " << value << " dégâts. Vulnérable " << secondary << ".";
        break;
    case engine::CardId::INVOKE:
        stream << "Tour suivant : invoque " << secondary << ", Énergie +" << value << ".";
        break;
    case engine::CardId::PARSE:
        stream << "Éthéré. Pioche " << secondary << " cartes.";
        break;
    case engine::CardId::PULL_FROM_BELOW:
        stream << "Inflige " << value << " par carte Éthérée jouée.";
        break;
    case engine::CardId::REAP:
        stream << "Retenue. Inflige " << value << " dégâts.";
        break;
    case engine::CardId::SOW:
        stream << "Retenue. Inflige " << value << " dégâts à TOUS.";
        break;
    case engine::CardId::ERADICATE:
        stream << "Retenue. Inflige " << value << " dégâts X fois.";
        break;
    default:
        break;
    }
}

bn::string_view card_cost_label(const engine::DeckCard& card)
{
    static constexpr bn::string_view DIGITS[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };
    uint8_t cost = engine::CardDef::get_cost(card.id, card.upgraded);

    if(cost == engine::COST_X)
    {
        return "X";
    }

    return DIGITS[bn::min(int(cost), 9)];
}

bn::string_view card_type_name(engine::CardType type)
{
    switch(type)
    {
    case engine::CardType::ATTACK:
        return "Attaque";
    case engine::CardType::SKILL:
        return "Compétence";
    default:
        return "Pouvoir";
    }
}

void status_line(const engine::CombatEntity& entity, bn::istring& out)
{
    out.clear();
    bn::ostringstream stream(out);

    for(const engine::StatusEffect& status : entity.statuses)
    {
        if(status.amount == 0)
        {
            continue;
        }

        if(! out.empty())
        {
            stream << ' ';
        }

        switch(status.type)
        {
        case engine::StatusType::STRENGTH:
            stream << "Force" << (status.amount > 0 ? "+" : "") << status.amount;
            break;
        case engine::StatusType::DEXTERITY:
            stream << "Dex" << (status.amount > 0 ? "+" : "") << status.amount;
            break;
        case engine::StatusType::VULNERABLE:
            stream << "Vuln" << status.amount;
            break;
        case engine::StatusType::WEAK:
            stream << "Faible" << status.amount;
            break;
        case engine::StatusType::POISON:
            stream << "Poison" << status.amount;
            break;
        case engine::StatusType::METALLICIZE:
            stream << "Métal" << status.amount;
            break;
        case engine::StatusType::PLATED_ARMOR:
            stream << "Plaque" << status.amount;
            break;
        case engine::StatusType::ARTIFACT:
            stream << "Artef" << status.amount;
            break;
        case engine::StatusType::RITUAL:
            stream << "Rituel" << status.amount;
            break;
        default:
            break;
        }
    }
}

}
