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
    case engine::CardId::ALIGNMENT:
        return "Alignement";
    case engine::CardId::ASTRAL_PULSE:
        return "Pulsation astrale";
    case engine::CardId::CLOAK_OF_STARS:
        return "Manteau d'étoiles";
    case engine::CardId::COMET:
        return "Comète";
    case engine::CardId::CRESCENT_SPEAR:
        return "Croissant de lance";
    case engine::CardId::DEVASTATE:
        return "Dévastation";
    case engine::CardId::DYING_STAR:
        return "Étoile mourante";
    case engine::CardId::GAMMA_BLAST:
        return "Déflagration gamma";
    case engine::CardId::GLOW:
        return "Luminosité";
    case engine::CardId::GUIDING_STAR:
        return "Étoile directrice";
    case engine::CardId::HIDDEN_CACHE:
        return "Cache secrète";
    case engine::CardId::METEOR_SHOWER:
        return "Pluie de météores";
    case engine::CardId::PARTICLE_WALL:
        return "Mur de particules";
    case engine::CardId::RADIATE:
        return "Rayonnement";
    case engine::CardId::RESONANCE:
        return "Résonance";
    case engine::CardId::ROYAL_GAMBLE:
        return "Pari royal";
    case engine::CardId::SEVEN_STARS:
        return "Pléiades";
    case engine::CardId::SHINING_STRIKE:
        return "Frappe rayonnante";
    case engine::CardId::STARDUST:
        return "Poussière d'étoiles";
    case engine::CardId::FEED:
        return "Appétit";
    case engine::CardId::BATTLE_TRANCE:
        return "Ivresse du combat";
    case engine::CardId::ECHOING_SLASH:
        return "Coup résonnant";
    case engine::CardId::GRAND_FINALE:
        return "Clou du spectacle";
    case engine::CardId::SUNDER:
        return "Scission";
    case engine::CardId::KNOCKOUT_BLOW:
        return "Hors d'état";
    case engine::CardId::FEEL_NO_PAIN:
        return "Ignorer la douleur";
    case engine::CardId::DARK_EMBRACE:
        return "Sombre étreinte";
    case engine::CardId::DRUM_OF_BATTLE:
        return "Tambour de guerre";
    case engine::CardId::BARRICADE:
        return "Barricade";
    case engine::CardId::INFERNO:
        return "Brasier";
    case engine::CardId::CRIMSON_MANTLE:
        return "Manteau pourpre";
    case engine::CardId::RUPTURE:
        return "Rupture";
    case engine::CardId::AGGRESSION:
        return "Agression";
    case engine::CardId::PYRE:
        return "Bûcher";
    case engine::CardId::DEMON_FORM:
        return "Forme démoniaque";
    case engine::CardId::JUGGERNAUT:
        return "Juggernaut";
    case engine::CardId::RAGE:
        return "Rage";
    case engine::CardId::VICIOUS:
        return "Vicieux";
    case engine::CardId::ACCELERANT:
        return "Accélérant";
    case engine::CardId::SPEEDSTER:
        return "Excès de vitesse";
    case engine::CardId::ENVENOM:
        return "Envenimer";
    case engine::CardId::SERPENT_FORM:
        return "Forme du serpent";
    case engine::CardId::AFTERIMAGE:
        return "Image rémanente";
    case engine::CardId::WELL_LAID_PLANS:
        return "Préméditation";
    case engine::CardId::STRANGLE:
        return "Strangulation";
    case engine::CardId::CORROSIVE_WAVE:
        return "Vague corrosive";
    case engine::CardId::NOXIOUS_FUMES:
        return "Vapeurs toxiques";
    case engine::CardId::MACHINE_LEARNING:
        return "Machine Learning";
    case engine::CardId::STORM:
        return "Orage";
    case engine::CardId::FERAL:
        return "Sauvagerie";
    case engine::CardId::SUBROUTINE:
        return "Sous-programme";
    case engine::CardId::CHILD_OF_THE_STARS:
        return "Enfant des étoiles";
    case engine::CardId::GENESIS:
        return "Genèse";
    case engine::CardId::THE_SEALED_THRONE:
        return "Le trône scellé";
    case engine::CardId::BLACK_HOLE:
        return "Trou noir";
    case engine::CardId::MAKE_IT_SO:
        return "Ainsi soit-il";
    case engine::CardId::PALE_BLUE_DOT:
        return "Point bleu pâle";
    case engine::CardId::ARSENAL:
        return "Arsenal";
    case engine::CardId::MONOLOGUE:
        return "Monologue";
    case engine::CardId::ORBIT:
        return "Orbite";
    case engine::CardId::PILLAR_OF_CREATION:
        return "Pilier de la création";
    case engine::CardId::MONARCHS_GAZE:
        return "Regard monarchique";
    case engine::CardId::FRIENDSHIP:
        return "Amitié";
    case engine::CardId::DANSE_MACABRE:
        return "Danse Macabre";
    case engine::CardId::DEMESNE:
        return "Domaine";
    case engine::CardId::SPIRIT_OF_ASH:
        return "Esprit des cendres";
    case engine::CardId::PAGESTORM:
        return "Pluie de papier";
    case engine::CardId::SLEIGHT_OF_FLESH:
        return "Tour de chair";
    case engine::CardId::SHATTER:
        return "Détonation";
    case engine::CardId::VOLTAIC:
        return "Voltaïque";
    case engine::CardId::QUADCAST:
        return "Activation quadruple";
    case engine::CardId::BARRAGE:
        return "Barrage";
    case engine::CardId::TESLA_COIL:
        return "Bobine Tesla";
    case engine::CardId::CHAOS:
        return "Chaos";
    case engine::CardId::COMPILE_DRIVER:
        return "Compilateur de pilote";
    case engine::CardId::DARKNESS:
        return "Ténèbres";
    case engine::CardId::MULTI_CAST:
        return "Activation multiple";
    case engine::CardId::LOOP:
        return "Boucle";
    case engine::CardId::CONSUMING_SHADOW:
        return "Ombre dévorante";
    case engine::CardId::HAILSTORM:
        return "Pluie de grêlons";
    case engine::CardId::THUNDER:
        return "Tonnerre";
    case engine::CardId::COOLANT:
        return "Ventilation";
    case engine::CardId::BIASED_COGNITION:
        return "Biais cognitif";
    case engine::CardId::HOTFIX:
        return "Correctif";
    case engine::CardId::DEFRAGMENT:
        return "Défragmentation";
    case engine::CardId::FOCUSED_STRIKE:
        return "Frappe focalisée";
    case engine::CardId::HYPERBEAM:
        return "Ultralaser";
    case engine::CardId::NEGATIVE_PULSE:
        return "Anti-pulsation";
    case engine::CardId::END_OF_DAYS:
        return "Fin des temps";
    case engine::CardId::BLIGHT_STRIKE:
        return "Frappe du fléau";
    case engine::CardId::NO_ESCAPE:
        return "Inéluctable";
    case engine::CardId::SCOURGE:
        return "Pestilence";
    case engine::CardId::DEATHBRINGER:
        return "Porte-mort";
    case engine::CardId::DEATHS_DOOR:
        return "À l'agonie";
    case engine::CardId::COUNTDOWN:
        return "Décompte";
    case engine::CardId::REAPER_FORM:
        return "Forme de la Faucheuse";
    case engine::CardId::NEUROSURGE:
        return "Influx neuronal";
    case engine::CardId::TIMES_UP:
        return "L'heure est venue";
    case engine::CardId::OBLIVION:
        return "Oblitération";
    case engine::CardId::SHROUD:
        return "Voile";
    case engine::CardId::CAPACITOR:
        return "Condensateur";
    case engine::CardId::BULK_UP:
        return "Prise de masse";
    case engine::CardId::SYNCHRONIZE:
        return "Synchronisation";
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
        stream << "Inflige " << value << ". Affaibli " << secondary << ". Vulnérable " << secondary << ".";
        break;
    case engine::CardId::VENERATE:
        stream << "Gagne " << secondary << " Étoiles.";
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
    case engine::CardId::ALIGNMENT:
        stream << "Énergie +" << value << ".";
        break;
    case engine::CardId::ASTRAL_PULSE:
        stream << "Inflige " << value << " dégâts à TOUS, " << secondary << " fois.";
        break;
    case engine::CardId::CLOAK_OF_STARS:
        stream << "Gagne " << value << " d'Armure.";
        break;
    case engine::CardId::COMET:
        stream << "Inflige " << value << ". Affaibli " << secondary << ". Vulnérable " << secondary << ".";
        break;
    case engine::CardId::CRESCENT_SPEAR:
        stream << "Inflige " << value << ", +" << secondary << " par carte coûtant des ★.";
        break;
    case engine::CardId::DEVASTATE:
        stream << "Inflige " << value << " dégâts.";
        break;
    case engine::CardId::DYING_STAR:
        stream << "Éthéré. TOUS : " << value << ", Force -" << secondary << " ce tour.";
        break;
    case engine::CardId::GAMMA_BLAST:
        stream << "Inflige " << value << ". Affaibli " << secondary << ". Vulnérable " << secondary << ".";
        break;
    case engine::CardId::GLOW:
        stream << "Gagne " << value << " ★. Pioche " << secondary << ", et au tour suivant.";
        break;
    case engine::CardId::GUIDING_STAR:
        stream << "Inflige " << value << " dégâts. Pioche " << secondary << " cartes.";
        break;
    case engine::CardId::HIDDEN_CACHE:
        stream << "Gagne " << value << " ★. Tour suivant : +" << secondary << " ★.";
        break;
    case engine::CardId::METEOR_SHOWER:
        stream << "TOUS : " << value << ", Affaibli et Vulnérable " << secondary << ".";
        break;
    case engine::CardId::PARTICLE_WALL:
        stream << "Gagne " << value << " d'Armure. Revient en main.";
        break;
    case engine::CardId::RADIATE:
        stream << "Inflige " << value << " à TOUS par ★ gagnée ce tour.";
        break;
    case engine::CardId::RESONANCE:
        stream << "Force +" << value << ". TOUS les ennemis : Force -" << secondary << ".";
        break;
    case engine::CardId::ROYAL_GAMBLE:
        stream << (card.upgraded ? "Retenue. " : "") << "Gagne " << value << " ★. Épuisement.";
        break;
    case engine::CardId::SEVEN_STARS:
        stream << "Inflige " << value << " dégâts à TOUS, " << secondary << " fois.";
        break;
    case engine::CardId::SHINING_STRIKE:
        stream << "Inflige " << value << ". +" << secondary << " ★. Retourne sur la pioche.";
        break;
    case engine::CardId::STARDUST:
        stream << "Inflige " << value << " à un ennemi au hasard, X fois.";
        break;
    case engine::CardId::FEED:
        stream << "Inflige " << value << ". Si tue : PV Max +" << secondary << ". Épuis.";
        break;
    case engine::CardId::BATTLE_TRANCE:
        stream << "Pioche " << secondary << " cartes. Plus de pioche ce tour.";
        break;
    case engine::CardId::ECHOING_SLASH:
        stream << "TOUS : " << value << " dégâts. Rejoue par ennemi tué.";
        break;
    case engine::CardId::GRAND_FINALE:
        stream << "Pioche vide requise. Inflige " << value << " à TOUS.";
        break;
    case engine::CardId::SUNDER:
        stream << "Inflige " << value << ". Si tue : Énergie +" << secondary << ".";
        break;
    case engine::CardId::KNOCKOUT_BLOW:
        stream << "Inflige " << value << ". Si tue : +" << secondary << " ★.";
        break;
    case engine::CardId::FEEL_NO_PAIN:
        stream << "Carte épuisée : gagne " << value << " d'Armure.";
        break;
    case engine::CardId::DARK_EMBRACE:
        stream << "Carte épuisée : pioche " << value << " carte.";
        break;
    case engine::CardId::DRUM_OF_BATTLE:
        stream << "Pioche " << value << " cartes. Si épuisée : Énergie +" << secondary << ".";
        break;
    case engine::CardId::BARRICADE:
        stream << "Armure conservée d'un tour à l'autre.";
        break;
    case engine::CardId::INFERNO:
        stream << "Tour : -1 PV. PV perdus : " << value << " à TOUS.";
        break;
    case engine::CardId::CRIMSON_MANTLE:
        stream << "Début de tour : -1 PV, +" << value << " d'Armure.";
        break;
    case engine::CardId::RUPTURE:
        stream << "PV perdus pendant votre tour : Force +" << value << ".";
        break;
    case engine::CardId::AGGRESSION:
        stream << (card.upgraded ? "Inné. " : "") << "Tour : reprend 1 Attaque, améliorée.";
        break;
    case engine::CardId::PYRE:
        stream << "Début de tour : Énergie +" << value << ".";
        break;
    case engine::CardId::DEMON_FORM:
        stream << "Début de tour : Force +" << value << ".";
        break;
    case engine::CardId::JUGGERNAUT:
        stream << "Armure gagnée : " << value << " à 1 ennemi au hasard.";
        break;
    case engine::CardId::RAGE:
        stream << "Ce tour, Attaque jouée : +" << value << " d'Armure.";
        break;
    case engine::CardId::VICIOUS:
        stream << "Vulnérable appliqué : pioche " << value << ".";
        break;
    case engine::CardId::ACCELERANT:
        stream << "Le Poison se déclenche " << value << " fois de plus.";
        break;
    case engine::CardId::SPEEDSTER:
        stream << (card.upgraded ? "Inné. " : "") << "Pioche pendant le tour : " << value << " à TOUS.";
        break;
    case engine::CardId::ENVENOM:
        stream << "Attaque non bloquée : " << value << " de Poison.";
        break;
    case engine::CardId::SERPENT_FORM:
        stream << "Carte jouée : " << value << " à 1 ennemi au hasard.";
        break;
    case engine::CardId::AFTERIMAGE:
        stream << (card.upgraded ? "Inné. " : "") << "Carte jouée : gagne " << value << " d'Armure.";
        break;
    case engine::CardId::WELL_LAID_PLANS:
        stream << "Main conservée en fin de tour.";
        break;
    case engine::CardId::STRANGLE:
        stream << "Inflige " << value << ". Ce tour, par carte : -" << secondary << " PV.";
        break;
    case engine::CardId::CORROSIVE_WAVE:
        stream << "Ce tour, carte piochée : " << value << " Poison à TOUS.";
        break;
    case engine::CardId::NOXIOUS_FUMES:
        stream << "Début de tour : " << value << " de Poison à TOUS.";
        break;
    case engine::CardId::MACHINE_LEARNING:
        stream << (card.upgraded ? "Inné. " : "") << "Début de tour : pioche " << value << " de plus.";
        break;
    case engine::CardId::STORM:
        stream << "Pouvoir joué : canalise " << value << " Orbe de foudre.";
        break;
    case engine::CardId::FERAL:
        stream << "Tour : 1re Attaque à 0 revient en main.";
        break;
    case engine::CardId::SUBROUTINE:
        stream << "Pouvoir joué : Énergie +" << value << ".";
        break;
    case engine::CardId::CHILD_OF_THE_STARS:
        stream << "★ dépensée : " << value << " d'Armure chacune.";
        break;
    case engine::CardId::GENESIS:
        stream << "Début de tour : +" << value << " ★.";
        break;
    case engine::CardId::THE_SEALED_THRONE:
        stream << "Carte jouée : +" << value << " ★.";
        break;
    case engine::CardId::BLACK_HOLE:
        stream << "★ gagnées ou dépensées : " << value << " à TOUS.";
        break;
    case engine::CardId::MAKE_IT_SO:
        stream << "Inflige " << value << ". Revient après " << secondary << " Compétences.";
        break;
    case engine::CardId::PALE_BLUE_DOT:
        stream << "5 cartes jouées : pioche +" << value << " ensuite.";
        break;
    case engine::CardId::ARSENAL:
        stream << (card.upgraded ? "Inné. " : "") << "Carte créée : Force +" << value << ".";
        break;
    case engine::CardId::MONOLOGUE:
        stream << (card.upgraded ? "Retenue. " : "") << "Ce tour, carte jouée : Force +" << value << ".";
        break;
    case engine::CardId::ORBIT:
        stream << "4 énergies dépensées : Énergie +" << value << ".";
        break;
    case engine::CardId::PILLAR_OF_CREATION:
        stream << "Carte créée : gagne " << value << " d'Armure.";
        break;
    case engine::CardId::MONARCHS_GAZE:
        stream << "Ennemi attaqué : Force -" << value << " ce tour.";
        break;
    case engine::CardId::FRIENDSHIP:
        stream << "Force -" << secondary << ". Début de tour : Énergie +" << value << ".";
        break;
    case engine::CardId::DANSE_MACABRE:
        stream << "Carte à 2 ⚡ ou plus jouée : +" << value << " d'Armure.";
        break;
    case engine::CardId::DEMESNE:
        stream << "Éthéré. Tour : Énergie +" << value << ", pioche +" << secondary << ".";
        break;
    case engine::CardId::SPIRIT_OF_ASH:
        stream << "Carte Éthérée jouée : gagne " << value << " d'Armure.";
        break;
    case engine::CardId::PAGESTORM:
        stream << "Carte Éthérée piochée : pioche " << value << ".";
        break;
    case engine::CardId::SLEIGHT_OF_FLESH:
        stream << "Malus appliqué à un ennemi : " << value << " dégâts.";
        break;
    case engine::CardId::SHATTER:
        stream << "TOUS : " << value << ". Évoque tout " << secondary << " fois. Épuis.";
        break;
    case engine::CardId::VOLTAIC:
        stream << "Recanalise chaque Foudre du combat." << (card.upgraded ? "" : " Épuis.");
        break;
    case engine::CardId::QUADCAST:
        stream << "Évoque votre Orbe la plus ancienne " << secondary << " fois.";
        break;
    case engine::CardId::BARRAGE:
        stream << "Inflige " << value << " dégâts par Orbe en place.";
        break;
    case engine::CardId::TESLA_COIL:
        stream << "Inflige " << value << ". Passif des Foudres, " << secondary << " fois.";
        break;
    case engine::CardId::CHAOS:
        stream << "Canalise " << secondary << " Orbe(s) au hasard.";
        break;
    case engine::CardId::COMPILE_DRIVER:
        stream << "Inflige " << value << ". Pioche 1 par type d'Orbe.";
        break;
    case engine::CardId::DARKNESS:
        stream << "Canalise 1 Ténèbres. Leur passif, " << secondary << " fois.";
        break;
    case engine::CardId::MULTI_CAST:
        stream << (card.upgraded ? "Évoque l'Orbe la plus ancienne X+1 fois." : "Évoque l'Orbe la plus ancienne X fois.");
        break;
    case engine::CardId::LOOP:
        stream << "Tour : passif du plus vieil Orbe, " << value << " fois.";
        break;
    case engine::CardId::CONSUMING_SHADOW:
        stream << secondary << " Ténèbres. Fin du tour : évoque 1 Orbe.";
        break;
    case engine::CardId::HAILSTORM:
        stream << "Fin de tour avec Givre : " << value << " à TOUS.";
        break;
    case engine::CardId::THUNDER:
        stream << "Foudre évoquée : +" << value << " dégâts à sa cible.";
        break;
    case engine::CardId::COOLANT:
        stream << "Tour : " << value << " d'Armure par type d'Orbe.";
        break;
    case engine::CardId::BIASED_COGNITION:
        stream << "Focalisation +" << value << ". Chaque tour : -" << secondary << ".";
        break;
    case engine::CardId::HOTFIX:
        stream << "Focalisation +" << value << " ce tour." << (card.upgraded ? "" : " Épuisement.");
        break;
    case engine::CardId::DEFRAGMENT:
        stream << "Focalisation +" << value << ".";
        break;
    case engine::CardId::FOCUSED_STRIKE:
        stream << "Inflige " << value << ". Focalisation +" << secondary << " ce tour.";
        break;
    case engine::CardId::HYPERBEAM:
        stream << "Inflige " << value << " à TOUS. Focalisation -" << secondary << ".";
        break;
    case engine::CardId::NEGATIVE_PULSE:
        stream << "Gagne " << value << " d'Armure. Fatalité " << secondary << " à TOUS.";
        break;
    case engine::CardId::END_OF_DAYS:
        stream << "Fatalité " << secondary << " à TOUS, mortelle aussitôt.";
        break;
    case engine::CardId::BLIGHT_STRIKE:
        stream << "Inflige " << value << ". Fatalité = dégâts infligés.";
        break;
    case engine::CardId::NO_ESCAPE:
        stream << "Fatalité " << value << ", +" << secondary << " par 10 déjà présents.";
        break;
    case engine::CardId::SCOURGE:
        stream << "Fatalité " << value << ". Pioche " << secondary << ".";
        break;
    case engine::CardId::DEATHBRINGER:
        stream << "Fatalité " << value << " et Affaibli " << secondary << " à TOUS.";
        break;
    case engine::CardId::DEATHS_DOOR:
        stream << "Armure " << value << ", 3 fois si Fatalité appliquée.";
        break;
    case engine::CardId::COUNTDOWN:
        stream << "Début de tour : Fatalité " << value << " à 1 ennemi.";
        break;
    case engine::CardId::REAPER_FORM:
        stream << (card.upgraded ? "Retenue. Dégâts d'Attaque = Fatalité." : "Dégâts d'Attaque : autant de Fatalité.");
        break;
    case engine::CardId::NEUROSURGE:
        stream << "+" << value << " ⚡, pioche " << secondary << ". Vous : Fatalité 3/tour.";
        break;
    case engine::CardId::TIMES_UP:
        stream << (card.upgraded ? "Retenue. " : "") << "Dégâts = Fatalité de l'ennemi.";
        break;
    case engine::CardId::OBLIVION:
        stream << "Ce tour, carte jouée : Fatalité " << value << " sur lui.";
        break;
    case engine::CardId::SHROUD:
        stream << "Fatalité appliquée : +" << value << " d'Armure.";
        break;
    case engine::CardId::CAPACITOR:
        stream << "Gagne " << value << " Emplacements d'Orbe.";
        break;
    case engine::CardId::BULK_UP:
        stream << "-" << secondary << " Emplacement. Force et Dextérité +" << value << ".";
        break;
    case engine::CardId::SYNCHRONIZE:
        stream << "Ce tour : Focalisation +" << value << " par type d'Orbe.";
        break;
    default:
        break;
    }
}

namespace
{

bn::string_view cost_digit(uint8_t cost)
{
    static constexpr bn::string_view DIGITS[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9" };

    if(cost == engine::COST_X)
    {
        return "X";
    }

    return DIGITS[bn::min(int(cost), 9)];
}

}

bn::string_view card_cost_label(const engine::DeckCard& card)
{
    return cost_digit(engine::CardDef::get_cost(card.id, card.upgraded));
}

bn::string_view card_star_cost_label(const engine::DeckCard& card)
{
    uint8_t cost = engine::CardDef::get_star_cost(card.id, card.upgraded);
    return cost == 0 ? bn::string_view() : cost_digit(cost);
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
        case engine::StatusType::FOCUS:
            stream << "Foc" << (status.amount > 0 ? "+" : "") << status.amount;
            break;
        case engine::StatusType::DOOM:
            stream << "Fatal" << status.amount;
            break;
        default:
            break;
        }
    }
}

}
