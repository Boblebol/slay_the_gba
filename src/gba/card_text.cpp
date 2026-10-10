#include "gba/card_text.h"
#include "gba/text.h"

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
    case engine::CardId::HOWL_FROM_BEYOND:
        return "Rugissement de l'au-delà";
    case engine::CardId::HELLRAISER:
        return "Soulèvement infernal";
    case engine::CardId::STAMPEDE:
        return "Ruée";
    case engine::CardId::CASCADE:
        return "Cascade";
    case engine::CardId::ONE_TWO_PUNCH:
        return "Droite-gauche";
    case engine::CardId::TEAR_ASUNDER:
        return "En miettes";
    case engine::CardId::HAVOC:
        return "Ravage";
    case engine::CardId::CORRUPTION:
        return "Corruption";
    case engine::CardId::UNRELENTING:
        return "Acharnement";
    case engine::CardId::MIDNIGHT:
        return "Minuit";
    case engine::CardId::STOMP:
        return "Piétinement";
    case engine::CardId::RAMPAGE:
        return "Saccage";
    case engine::CardId::BURST:
        return "Déflagration";
    case engine::CardId::BULLET_TIME:
        return "Bullet Time";
    case engine::CardId::PINPOINT:
        return "Localisation";
    case engine::CardId::POUNCE:
        return "Surgissement";
    case engine::CardId::ECHO_FORM:
        return "Forme résonnante";
    case engine::CardId::SIGNAL_BOOST:
        return "Amplification du signal";
    case engine::CardId::UPROAR:
        return "Tumulte";
    case engine::CardId::ADAPTIVE_STRIKE:
        return "Frappe adaptative";
    case engine::CardId::SYNTHESIS:
        return "Synthèse";
    case engine::CardId::GENETIC_ALGORITHM:
        return "Algorithme génétique";
    case engine::CardId::MOMENTUM_STRIKE:
        return "Frappe dynamique";
    case engine::CardId::CLAW:
        return "Griffure";
    case engine::CardId::MODDED:
        return "Modificateurs";
    case engine::CardId::BOMBARDMENT:
        return "Bombardement";
    case engine::CardId::I_AM_INVINCIBLE:
        return "Je suis invincible";
    case engine::CardId::VOID_FORM:
        return "Forme du néant";
    case engine::CardId::KINGLY_KICK:
        return "Coup de pied royal";
    case engine::CardId::KINGLY_PUNCH:
        return "Coup de poing royal";
    case engine::CardId::EIDOLON:
        return "Eidolon";
    case engine::CardId::BANSHEES_CRY:
        return "Cri de la banshee";
    case engine::CardId::VEILPIERCER:
        return "Perce-voile";
    case engine::CardId::BORROWED_TIME:
        return "Sursis";
    case engine::CardId::THE_SCYTHE:
        return "La faux";
    case engine::CardId::MELANCHOLY:
        return "Mélancolie";
    case engine::CardId::HANG:
        return "Pendaison";
    case engine::CardId::BURN:
        return "Brûlure";
    case engine::CardId::DAZED:
        return "Hébété";
    case engine::CardId::DEBRIS:
        return "Débris";
    case engine::CardId::SLIMED:
        return "Englué";
    case engine::CardId::SOOT:
        return "Suie";
    case engine::CardId::VOID_CARD:
        return "Néant";
    case engine::CardId::WOUND:
        return "Plaie";
    case engine::CardId::FUEL:
        return "Combustible";
    case engine::CardId::BOOST_AWAY:
        return "Boost";
    case engine::CardId::FLAK_CANNON:
        return "Canon anti-aérien";
    case engine::CardId::SMOKESTACK:
        return "Cheminée";
    case engine::CardId::COMPACT:
        return "Compactage";
    case engine::CardId::GUNK_UP:
        return "Encrassage";
    case engine::CardId::ITERATION:
        return "Itération";
    case engine::CardId::FIGHT_THROUGH:
        return "Lutte";
    case engine::CardId::OVERCLOCK:
        return "Overclocking";
    case engine::CardId::ROCKET_PUNCH:
        return "Poing roquette";
    case engine::CardId::TRASH_TO_TREASURE:
        return "Revalorisation";
    case engine::CardId::TURBO:
        return "TURBO";
    case engine::CardId::COLLISION_COURSE:
        return "Trajectoire de collision";
    case engine::CardId::CRASH_LANDING:
        return "Atterrissage forcé";
    case engine::CardId::SHIV:
        return "Surin";
    case engine::CardId::GIANT_ROCK:
        return "Rocher gigantesque";
    case engine::CardId::LUMINESCE:
        return "Éclat";
    case engine::CardId::APPARITION:
        return "Apparition";
    case engine::CardId::MAUL:
        return "Mutilation";
    case engine::CardId::APOTHEOSIS:
        return "Apothéose";
    case engine::CardId::WISH:
        return "Vœu";
    case engine::CardId::WHISTLE:
        return "Sifflet";
    case engine::CardId::BRIGHTEST_FLAME:
        return "Flamme éclatante";
    case engine::CardId::PROWESS:
        return "Prouesse";
    case engine::CardId::SQUASH:
        return "Aplatissement";
    case engine::CardId::EXTERMINATE:
        return "Extermination";
    case engine::CardId::FOLLY:
        return "Hubris";
    case engine::CardId::ENTHRALLED:
        return "Assuétude";
    case engine::CardId::FLASH_OF_STEEL:
        return "Éclair d'acier";
    case engine::CardId::PECK:
        return "Picpic";
    case engine::CardId::PRODUCTION:
        return "Production";
    case engine::CardId::MASTER_OF_STRATEGY:
        return "Maître stratège";
    case engine::CardId::FEEDING_FRENZY:
        return "Repas frénétique";
    case engine::CardId::OUTMANEUVER:
        return "Astucieux";
    case engine::CardId::TORIC_TOUGHNESS:
        return "Ténacité torique";
    case engine::CardId::ENTRENCH:
        return "Fortification";
    case engine::CardId::STACK:
        return "Pile";
    case engine::CardId::RIP_AND_TEAR:
        return "Lacération";
    case engine::CardId::SCRAWL:
        return "Griffonage";
    case engine::CardId::MIND_BLAST:
        return "Déflagration mentale";
    case engine::CardId::SHOCKWAVE:
        return "Onde de choc";
    case engine::CardId::BOLAS:
        return "Bolas";
    case engine::CardId::THRUMMING_HATCHET:
        return "Hache sifflante";
    case engine::CardId::IMPATIENCE:
        return "Impatience";
    case engine::CardId::CLASH:
        return "Conflit";
    case engine::CardId::VOLLEY:
        return "Artillerie";
    case engine::CardId::OMNISLICE:
        return "Omnilame";
    case engine::CardId::RESTLESSNESS:
        return "Sans repos";
    case engine::CardId::ETERNAL_ARMOR:
        return "Armure éternelle";
    case engine::CardId::FASTEN:
        return "Bien attaché";
    case engine::CardId::REBOUND:
        return "Rebond";
    case engine::CardId::PANIC_BUTTON:
        return "Bouton de secours";
    case engine::CardId::GOLD_AXE:
        return "Hache dorée";
    case engine::CardId::ANOINTED:
        return "Sacrement";
    case engine::CardId::FISTICUFFS:
        return "Bagarre";
    case engine::CardId::REND:
        return "Pourfendre";
    case engine::CardId::THE_BOMB:
        return "La bombe";
    case engine::CardId::ROLLING_BOULDER:
        return "Pierre qui roule";
    case engine::CardId::PANACHE:
        return "Panache";
    case engine::CardId::JACKPOT:
        return "Jackpot";
    case engine::CardId::BEAT_DOWN:
        return "Passage à tabac";
    case engine::CardId::CATASTROPHE:
        return "Catastrophe";
    case engine::CardId::NOSTALGIA:
        return "Nostalgie";
    case engine::CardId::RELAX:
        return "Relaxation";
    case engine::CardId::NEOWS_FURY:
        return "Fureur de Neow";
    case engine::CardId::METAMORPHOSIS:
        return "Métamorphose";
    case engine::CardId::DISTRACTION:
        return "Distraction";
    case engine::CardId::CALTROPS:
        return "Chausse-trapes";
    case engine::CardId::PROLONG:
        return "Prolongations";
    case engine::CardId::SALVO:
        return "Salve";
    case engine::CardId::THINKING_AHEAD:
        return "Coup d'avance";
    case engine::CardId::JACK_OF_ALL_TRADES:
        return "Polyvalence";
    case engine::CardId::HELLO_WORLD:
        return "Hello World";
    case engine::CardId::DARK_SHACKLES:
        return "Entraves des ténèbres";
    case engine::CardId::EQUILIBRIUM:
        return "Équilibre";
    case engine::CardId::PURITY:
        return "Épuration";
    case engine::CardId::SECRET_TECHNIQUE:
        return "Technique secrète";
    case engine::CardId::DRAMATIC_ENTRANCE:
        return "Entrée fracassante";
    case engine::CardId::SECRET_WEAPON:
        return "Arme secrète";
    case engine::CardId::MAYHEM:
        return "Grabuge";
    case engine::CardId::PREP_TIME:
        return "Échauffement";
    case engine::CardId::STRATAGEM:
        return "Stratagème";
    case engine::CardId::CALAMITY:
        return "Calamité";
    case engine::CardId::AUTOMATION:
        return "Automatisation";
    case engine::CardId::ENLIGHTENMENT:
        return "Illumination";
    case engine::CardId::DUAL_WIELD:
        return "Ambidextrie";
    case engine::CardId::CALL_OF_THE_VOID:
        return "Appel du néant";
    case engine::CardId::HEIRLOOM_HAMMER:
        return "Marteau d'héritage";
    case engine::CardId::BUNDLE_OF_JOY:
        return "Cadeau joyeux";
    case engine::CardId::SPECTRUM_SHIFT:
        return "Spectre visible";
    case engine::CardId::DECISIONS_DECISIONS:
        return "Grandes décisions";
    case engine::CardId::WHITE_NOISE:
        return "Bruit blanc";
    case engine::CardId::CREATIVE_AI:
        return "IA créative";
    case engine::CardId::NIGHTMARE:
        return "Cauchemar";
    case engine::CardId::TOOLS_OF_THE_TRADE:
        return "Savoir-faire";
    case engine::CardId::INFERNAL_BLADE:
        return "Lame infernale";
    case engine::CardId::STOKE:
        return "Attiser les flammes";
    case engine::CardId::INJURY:
        return "Blessure";
    case engine::CardId::GREED:
        return "Cupidité";
    case engine::CardId::CLUMSY:
        return "Maladresse";
    case engine::CardId::ASCENDERS_BANE:
        return "Fléau de l'ascendant";
    case engine::CardId::CURSE_OF_THE_BELL:
        return "Malédiction de la cloche";
    case engine::CardId::POOR_SLEEP:
        return "Insomnie";
    case engine::CardId::WRITHE:
        return "Souffrance";
    case engine::CardId::NORMALITY:
        return "Normalité";
    case engine::CardId::DOUBT:
        return "Doute";
    case engine::CardId::SHAME:
        return "Honte";
    case engine::CardId::DECAY:
        return "Déclin";
    case engine::CardId::REGRET:
        return "Regrets";
    case engine::CardId::BAD_LUCK:
        return "Malchance";
    case engine::CardId::DEBT:
        return "Dette";
    case engine::CardId::SPORE_MIND:
        return "Conscience sporique";
    case engine::CardId::TACTICIAN:
        return "Tactique";
    case engine::CardId::ABRASIVE:
        return "Abrasif";
    case engine::CardId::MASTER_PLANNER:
        return "Grands desseins";
    case engine::CardId::HAND_TRICK:
        return "Trucage";
    case engine::CardId::UNTOUCHABLE:
        return "Intouchable";
    case engine::CardId::RICOCHET:
        return "Ricochet";
    case engine::CardId::REFLEX:
        return "Réflexes";
    case engine::CardId::FLICK_FLACK:
        return "Tic tac";
    case engine::CardId::BYRD_SWOOP:
        return "Piqué d'oizo";
    case engine::CardId::ULTIMATE_STRIKE:
        return "Frappe ultime";
    case engine::CardId::ULTIMATE_DEFEND:
        return "Défense ultime";
    case engine::CardId::FINESSE:
        return "Finesse";
    case engine::CardId::BLADE_DANCE:
        return "Danse des lames";
    case engine::CardId::CLOAK_AND_DAGGER:
        return "Cape et épée";
    case engine::CardId::LEADING_STRIKE:
        return "Frappe initiale";
    case engine::CardId::BLADE_OF_INK:
        return "Lame d'encre";
    case engine::CardId::INFINITE_BLADES:
        return "Lames infinies";
    case engine::CardId::STORM_OF_STEEL:
        return "Tempête d'acier";
    case engine::CardId::FAN_OF_KNIVES:
        return "Éventail de couteaux";
    case engine::CardId::PHANTOM_BLADES:
        return "Lames fantomatiques";
    case engine::CardId::ACCURACY:
        return "Précision";
    case engine::CardId::UP_MY_SLEEVE:
        return "Passe-passe";
    case engine::CardId::KNIFE_TRAP:
        return "Piège à poignards";
    case engine::CardId::ARMAMENTS:
        return "Armement";
    case engine::CardId::BRAND:
        return "Marqué au fer rouge";
    case engine::CardId::BURNING_PACT:
        return "Pacte brûlant";
    case engine::CardId::ACROBATICS:
        return "Acrobaties";
    case engine::CardId::DAGGER_THROW:
        return "Lancer de couteau";
    case engine::CardId::PREPARED:
        return "Préparatifs";
    case engine::CardId::HIDDEN_DAGGERS:
        return "Poignards dissimulés";
    case engine::CardId::SCAVENGE:
        return "Récupération";
    case engine::CardId::PHOTON_CUT:
        return "Entaille photonique";
    case engine::CardId::GLIMMER:
        return "Lueur";
    case engine::CardId::TYRANNY:
        return "Tyran";
    case engine::CardId::SCULPTING_STRIKE:
        return "Frappe du sculpteur";
    case engine::CardId::JUGGLING:
        return "Jonglage";
    case engine::CardId::EXPOSE:
        return "Point faible";
    case engine::CardId::HEADBUTT:
        return "Coup de boule";
    case engine::CardId::HOLOGRAM:
        return "Hologramme";
    case engine::CardId::COSMIC_INDIFFERENCE:
        return "Indifférence cosmique";
    case engine::CardId::FOREGONE_CONCLUSION:
        return "Couru d'avance";
    case engine::CardId::GRAVEBLAST:
        return "Déflagration tombale";
    case engine::CardId::DREDGE:
        return "Déterrer";
    case engine::CardId::CLEANSE:
        return "Purge";
    case engine::CardId::STONE_ARMOR:
        return "Armure de pierre";
    case engine::CardId::FLAME_BARRIER:
        return "Barrière de flammes";
    case engine::CardId::WRAITH_FORM:
        return "Forme spectrale";
    case engine::CardId::BUFFER:
        return "Tampon";
    case engine::CardId::NEUTRON_AEGIS:
        return "Égide à neutrons";
    case engine::CardId::REFLECT:
        return "Reflet miroir";
    case engine::CardId::PATTER:
        return "Petits pas";
    case engine::CardId::TERRAFORMING:
        return "Terraformation";
    case engine::CardId::SOVEREIGN_BLADE:
        return "Lame souveraine";
    case engine::CardId::REFINE_BLADE:
        return "Lame lustrée";
    case engine::CardId::SPOILS_OF_BATTLE:
        return "Butin de guerre";
    case engine::CardId::WROUGHT_IN_WAR:
        return "Forgé par la guerre";
    case engine::CardId::BULWARK:
        return "Rempart";
    case engine::CardId::CONQUEROR:
        return "Conquérant";
    case engine::CardId::FURNACE:
        return "Fournaise";
    case engine::CardId::PARRY:
        return "Parade";
    case engine::CardId::SUMMON_FORTH:
        return "L'union fait la forge";
    case engine::CardId::BEAT_INTO_SHAPE:
        return "Châtiment";
    case engine::CardId::BIG_BANG:
        return "Big Bang";
    case engine::CardId::SEEKING_EDGE:
        return "Lame chercheuse";
    case engine::CardId::SWORD_SAGE:
        return "Sage à l'épée";
    case engine::CardId::THE_SMITH:
        return "Le forgeron";
    case engine::CardId::POKE:
        return "Chiquenaude";
    case engine::CardId::SNAP:
        return "Claquement de doigts";
    case engine::CardId::RATTLE:
        return "Cliquetis";
    case engine::CardId::BONE_SHARDS:
        return "Détonation osseuse";
    case engine::CardId::SQUEEZE:
        return "Joug";
    case engine::CardId::FLATTEN:
        return "Raser";
    case engine::CardId::HIGH_FIVE:
        return "Tope là";
    case engine::CardId::FETCH:
        return "Va chercher";
    case engine::CardId::CALCIFY:
        return "Calcification";
    case engine::CardId::RIGHT_HAND_HAND:
        return "Droitier";
    case engine::CardId::NECRO_MASTERY:
        return "Maîtrise nécro";
    case engine::CardId::PROTECTOR:
        return "Protecteur";
    case engine::CardId::SACRIFICE:
        return "Sacrifice";
    case engine::CardId::SPUR:
        return "Stimulation";
    case engine::CardId::SIC_EM:
        return "À l'attaque !";
    case engine::CardId::SENTRY_MODE:
        return "Sentinelle";
    case engine::CardId::SWEEPING_GAZE:
        return "Balayage optique";
    case engine::CardId::SOUL:
        return "Âme";
    case engine::CardId::REAVE:
        return "Exaction";
    case engine::CardId::GRAVE_WARDEN:
        return "Fossoyeur";
    case engine::CardId::CAPTURE_SPIRIT:
        return "Capture spirituelle";
    case engine::CardId::SEVERANCE:
        return "Césure";
    case engine::CardId::DIRGE:
        return "Oraison";
    case engine::CardId::HAUNT:
        return "Hanter";
    case engine::CardId::DEVOUR_LIFE:
        return "Dévoration de vie";
    case engine::CardId::SOUL_STORM:
        return "Tempête d'âmes";
    case engine::CardId::GLIMPSE_BEYOND:
        return "Vision de l'au-delà";
    case engine::CardId::MANIFEST_AUTHORITY:
        return "Autorité manifeste";
    default:
        return "???";
    }
}

}

void card_name(const engine::DeckCard& card, bn::istring& out)
{
    out.clear();
    out.append(base_name(card.id));

    if(card.enchantment == engine::Enchantment::INKY)
    {
        out.append(" encré");
    }

    // A Status card has no upgraded version, whatever upgraded the hand it was in.
    if(card.upgraded && engine::CardDef::get(card.id).type != engine::CardType::STATUS)
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
        stream << "Inflige " << value << " dégâts.";
        break;
    case engine::CardId::UNLEASH:
        stream << "Titos inflige " << value << " dégâts + ses PV.";
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
        stream << "Invoque Titos (" << secondary << " PV).";
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
    case engine::CardId::WISH:
        stream << (card.upgraded ? "Retenue. 1 carte Pioche vers main. Épuis." : "1 carte Pioche vers main. Épuisement.");
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
    case engine::CardId::LUMINESCE:
        stream << "Retenue. Énergie +" << value << ". Épuisement.";
        break;
    case engine::CardId::APPARITION:
        stream << (card.upgraded ? "" : "Éthéré. ") << "Intangible " << value << ". Épuisement.";
        break;
    case engine::CardId::MAUL:
        stream << "Inflige " << value << " dégâts " << secondary << " fois. Mutilations +" << (card.upgraded ? "3" : "2") << ".";
        break;
    case engine::CardId::APOTHEOSIS:
        stream << "Inné. Améliore toutes vos cartes. Épuis.";
        break;
    case engine::CardId::WHISTLE:
        stream << "Inflige " << value << " dégâts. Étourdit. Épuisement.";
        break;
    case engine::CardId::BRIGHTEST_FLAME:
        stream << "Énergie +" << value << ". Pioche " << secondary << ". -2 PV Max.";
        break;
    case engine::CardId::FLASH_OF_STEEL:
        stream << "Inflige " << value << " dégâts. Pioche " << secondary << ".";
        break;
    case engine::CardId::PECK:
        stream << "Inflige " << value << " dégâts " << secondary << " fois.";
        break;
    case engine::CardId::PRODUCTION:
        stream << "Énergie +" << value << ". Épuisement.";
        break;
    case engine::CardId::MASTER_OF_STRATEGY:
        stream << "Pioche " << secondary << ". Épuisement.";
        break;
    case engine::CardId::FEEDING_FRENZY:
        stream << "Force +" << value << " ce tour.";
        break;
    case engine::CardId::OUTMANEUVER:
        stream << "Tour suivant : Énergie +" << value << ".";
        break;
    case engine::CardId::TORIC_TOUGHNESS:
        stream << "Armure +" << value << ", puis +" << value << " aux 2 tours suivants.";
        break;
    case engine::CardId::ENTRENCH:
        stream << "Double l'Armure.";
        break;
    case engine::CardId::STACK:
        stream << "Armure = cartes en Défausse" << (card.upgraded ? " +3." : ".");
        break;
    case engine::CardId::RIP_AND_TEAR:
        stream << "Inflige " << value << " dégâts au hasard, " << secondary << " fois.";
        break;
    case engine::CardId::SCRAWL:
        stream << (card.upgraded ? "Retenue. " : "") << "Remplit la main (pioche). " << (card.upgraded ? "Épuis." : "Épuisement.");
        break;
    case engine::CardId::MIND_BLAST:
        stream << "Inné. Inflige 1 dégât par carte en Pioche.";
        break;
    case engine::CardId::SHOCKWAVE:
        stream << "Affaibli et Vulnérable " << value << " à TOUS. Épuis.";
        break;
    case engine::CardId::BOLAS:
    case engine::CardId::THRUMMING_HATCHET:
        stream << "Inflige " << value << ". Revient au tour suivant.";
        break;
    case engine::CardId::IMPATIENCE:
        stream << "Sans Attaque en main : pioche " << secondary << ".";
        break;
    case engine::CardId::CLASH:
        stream << "Jouable si Main d'Attaques : " << value << " dégâts.";
        break;
    case engine::CardId::VOLLEY:
        stream << "Inflige " << value << " dégâts au hasard, X fois.";
        break;
    case engine::CardId::OMNISLICE:
        stream << "Inflige " << value << " dégâts, autant aux autres.";
        break;
    case engine::CardId::RESTLESSNESS:
        stream << "Retenue. Main vide : pioche " << secondary << ", énergie +" << secondary << ".";
        break;
    case engine::CardId::ETERNAL_ARMOR:
        stream << "Gagne " << value << " de Blindage.";
        break;
    case engine::CardId::FASTEN:
        stream << "Défense : Armure +" << value << ".";
        break;
    case engine::CardId::REBOUND:
        stream << "Inflige " << value << ". Prochaine carte sur la Pioche.";
        break;
    case engine::CardId::PANIC_BUTTON:
        stream << "Armure +" << value << ". Pas d'Armure 2 tours. Épuis.";
        break;
    case engine::CardId::GOLD_AXE:
        stream << (card.upgraded ? "Retenue. " : "") << "Dégâts = cartes jouées.";
        break;
    case engine::CardId::ANOINTED:
        stream << (card.upgraded ? "Retenue. " : "") << "Rares de Pioche en main. Épuis.";
        break;
    case engine::CardId::FISTICUFFS:
        stream << "Inflige " << value << " dégâts. Armure = dégâts.";
        break;
    case engine::CardId::REND:
        stream << "Inflige " << value << " dégâts, +" << secondary << " par effet néfaste.";
        break;
    case engine::CardId::THE_BOMB:
        stream << "Dans 3 tours : " << value << " dégâts à TOUS.";
        break;
    case engine::CardId::ROLLING_BOULDER:
        stream << "Début de tour : " << value << " dégâts à TOUS, +5.";
        break;
    case engine::CardId::PANACHE:
        stream << "Toutes les 5 cartes : " << value << " dégâts à TOUS.";
        break;
    case engine::CardId::JACKPOT:
        stream << "Inflige " << value << ". +" << secondary << " cartes à 0" << (card.upgraded ? " améliorées." : " en Main.");
        break;
    case engine::CardId::BEAT_DOWN:
        stream << "Joue " << value << " Attaques au hasard de la Défausse.";
        break;
    case engine::CardId::CATASTROPHE:
        stream << "Joue " << value << " cartes au hasard de la Pioche.";
        break;
    case engine::CardId::NOSTALGIA:
        stream << "Par tour, 1re Att./Comp. sur la Pioche.";
        break;
    case engine::CardId::RELAX:
        stream << "Armure +" << value << ". Puis +" << secondary << " cartes/én. Épuis.";
        break;
    case engine::CardId::NEOWS_FURY:
        stream << value << " dégâts. Défausse vers main : " << secondary << ". Épuis.";
        break;
    case engine::CardId::METAMORPHOSIS:
        stream << "+" << value << " Attaques en Pioche, coût 0. Épuis.";
        break;
    case engine::CardId::DISTRACTION:
        stream << "+1 Compétence gratuite ce tour. Épuis.";
        break;
    case engine::CardId::CALTROPS:
        stream << "Attaqué : " << value << " dégâts à l'attaquant.";
        break;
    case engine::CardId::PROLONG:
        stream << "Tour suivant : regagne l'Armure." << (card.upgraded ? "" : " Épuis.");
        break;
    case engine::CardId::SALVO:
        stream << "Inflige " << value << " dégâts. Retient la Main.";
        break;
    case engine::CardId::THINKING_AHEAD:
        stream << "Pioche " << value << ". 1 carte sur la Pioche." << (card.upgraded ? "" : " Épuis.");
        break;
    case engine::CardId::JACK_OF_ALL_TRADES:
        stream << "+" << value << (card.upgraded ? " cartes Incolores" : " carte Incolore") << " en main. Épuisement.";
        break;
    case engine::CardId::HELLO_WORLD:
        stream << (card.upgraded ? "Inné. " : "") << "Début de tour : +1 carte Commune.";
        break;
    case engine::CardId::DARK_SHACKLES:
        stream << "Ennemi : Force -" << value << " ce tour. Épuisement.";
        break;
    case engine::CardId::EQUILIBRIUM:
        stream << "Armure +" << value << ". Retient la Main.";
        break;
    case engine::CardId::PURITY:
        stream << "Retenue. Épuise jusqu'à " << value << " cartes. Épuis.";
        break;
    case engine::CardId::SECRET_TECHNIQUE:
        stream << "Compétence de la Pioche en main." << (card.upgraded ? "" : " Épuis.");
        break;
    case engine::CardId::DRAMATIC_ENTRANCE:
        stream << "Inné. " << value << " dégâts à TOUS. Épuisement.";
        break;
    case engine::CardId::SECRET_WEAPON:
        stream << "Attaque de la Pioche en main." << (card.upgraded ? "" : " Épuisement.");
        break;
    case engine::CardId::MAYHEM:
        stream << "Début de tour : joue le haut de la Pioche.";
        break;
    case engine::CardId::PREP_TIME:
        stream << "Début de tour : Vigueur +" << value << ".";
        break;
    case engine::CardId::STRATAGEM:
        stream << "Pioche mélangée : 1 carte en main.";
        break;
    case engine::CardId::CALAMITY:
        stream << "Attaque jouée : +1 Attaque en main.";
        break;
    case engine::CardId::AUTOMATION:
        stream << "Par 10 cartes piochées : +1 énergie.";
        break;
    case engine::CardId::ENLIGHTENMENT:
        stream << "Main : coût max 1 " << (card.upgraded ? "ce combat" : "ce tour") << ". Épuisement.";
        break;
    case engine::CardId::DUAL_WIELD:
        stream << "+" << value << (card.upgraded ? " copies" : " copie") << " d'une Attaque ou d'un Pouvoir.";
        break;
    case engine::CardId::CALL_OF_THE_VOID:
        stream << (card.upgraded ? "Inné. " : "") << "Début de tour : +1 carte Éthérée.";
        break;
    case engine::CardId::HEIRLOOM_HAMMER:
        stream << "Inflige " << value << ". Copie 1 Incolore de la Main.";
        break;
    case engine::CardId::BUNDLE_OF_JOY:
        stream << "+" << value << " cartes Incolores en main. Épuisement.";
        break;
    case engine::CardId::SPECTRUM_SHIFT:
        stream << "Début de tour : +1 carte Incolore.";
        break;
    case engine::CardId::DECISIONS_DECISIONS:
        stream << "Pioche " << value << ". Joue 1 Compétence x3. Épuis.";
        break;
    case engine::CardId::WHITE_NOISE:
        stream << "+1 Pouvoir gratuit ce tour. Épuisement.";
        break;
    case engine::CardId::CREATIVE_AI:
        stream << "Début de tour : +1 Pouvoir en main.";
        break;
    case engine::CardId::NIGHTMARE:
        stream << "1 carte : 3 copies au tour suivant. Épuis.";
        break;
    case engine::CardId::TOOLS_OF_THE_TRADE:
        stream << "Début de tour : pioche 1, défausse 1.";
        break;
    case engine::CardId::INFERNAL_BLADE:
        stream << "+1 Attaque gratuite ce tour. Épuisement.";
        break;
    case engine::CardId::STOKE:
        stream << "Épuise la Main. +1 carte" << (card.upgraded ? " amél." : "") << " par carte.";
        break;
    case engine::CardId::INJURY:
        stream << "Injouable.";
        break;
    case engine::CardId::GREED:
        stream << "Injouable. Éternel.";
        break;
    case engine::CardId::CLUMSY:
        stream << "Injouable. Éthéré.";
        break;
    case engine::CardId::ASCENDERS_BANE:
        stream << "Injouable. Éthéré. Éternel.";
        break;
    case engine::CardId::CURSE_OF_THE_BELL:
        stream << "Injouable. Éternel.";
        break;
    case engine::CardId::POOR_SLEEP:
        stream << "Injouable. Retenue.";
        break;
    case engine::CardId::WRITHE:
        stream << "Injouable. Inné.";
        break;
    case engine::CardId::NORMALITY:
        stream << "Injouable. 3 cartes max par tour.";
        break;
    case engine::CardId::DOUBT:
        stream << "Injouable. Fin de tour : Affaibli 1.";
        break;
    case engine::CardId::SHAME:
        stream << "Injouable. Fin de tour : Fragilisé 1.";
        break;
    case engine::CardId::DECAY:
        stream << "Injouable. Fin de tour : 2 dégâts.";
        break;
    case engine::CardId::REGRET:
        stream << "Injouable. Fin de tour : -1 PV par carte.";
        break;
    case engine::CardId::BAD_LUCK:
        stream << "Injouable. Fin de tour : -13 PV. Éternel.";
        break;
    case engine::CardId::DEBT:
        stream << "Injouable. Fin de tour : -10 or.";
        break;
    case engine::CardId::SPORE_MIND:
        stream << "Épuisement.";
        break;
    case engine::CardId::TACTICIAN:
        stream << "Ruse. Énergie +" << value << ".";
        break;
    case engine::CardId::ABRASIVE:
        stream << "Ruse. Dextérité +1. Épines " << value << ".";
        break;
    case engine::CardId::MASTER_PLANNER:
        stream << "Vos Compétences jouées ont Ruse.";
        break;
    case engine::CardId::HAND_TRICK:
        stream << "Armure " << value << ". Ruse à 1 Compétence ce tour.";
        break;
    case engine::CardId::UNTOUCHABLE:
        stream << "Ruse. Gagne " << value << " d'Armure.";
        break;
    case engine::CardId::RICOCHET:
        stream << "Ruse. " << value << " dégâts au hasard " << secondary << " fois.";
        break;
    case engine::CardId::REFLEX:
        stream << "Ruse. Pioche " << value << ".";
        break;
    case engine::CardId::FLICK_FLACK:
        stream << "Ruse. Inflige " << value << " dégâts à TOUS.";
        break;
    case engine::CardId::BYRD_SWOOP:
    case engine::CardId::ULTIMATE_STRIKE:
        stream << "Inflige " << value << " dégâts.";
        break;
    case engine::CardId::ULTIMATE_DEFEND:
        stream << "Gagne " << value << " d'Armure.";
        break;
    case engine::CardId::FINESSE:
        stream << "Gagne " << value << " d'Armure. Pioche " << secondary << ".";
        break;
    case engine::CardId::PROWESS:
        stream << "Force +" << value << ". Dextérité +" << value << ".";
        break;
    case engine::CardId::SQUASH:
        stream << "Inflige " << value << " dégâts. Vulnérable " << secondary << ".";
        break;
    case engine::CardId::EXTERMINATE:
        stream << "Inflige " << value << " dégâts à TOUS " << secondary << " fois.";
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
    case engine::CardId::HOWL_FROM_BEYOND:
        stream << "TOUS : " << value << ". Épuisée, rejouée fin de tour.";
        break;
    case engine::CardId::HELLRAISER:
        stream << "Frappe piochée : jouée au hasard aussitôt.";
        break;
    case engine::CardId::STAMPEDE:
        stream << "Fin de tour : joue 1 Attaque de la main.";
        break;
    case engine::CardId::CASCADE:
        stream << (card.upgraded ? "Joue X+1 cartes du dessus de la pioche." : "Joue X cartes du dessus de la pioche.");
        break;
    case engine::CardId::ONE_TWO_PUNCH:
        stream << "Ce tour : " << secondary << " Attaque(s) jouée(s) 2 fois.";
        break;
    case engine::CardId::TEAR_ASUNDER:
        stream << "Inflige " << value << ", +1 fois par perte de PV.";
        break;
    case engine::CardId::HAVOC:
        stream << "Joue le dessus de la pioche et l'épuise.";
        break;
    case engine::CardId::CORRUPTION:
        stream << "Vos Compétences coûtent 0 et s'épuisent.";
        break;
    case engine::CardId::UNRELENTING:
        stream << "Inflige " << value << ". Prochaine Attaque : coût 0.";
        break;
    case engine::CardId::MIDNIGHT:
        stream << "Inflige " << value << ". Coût -1 par carte épuisée.";
        break;
    case engine::CardId::STOMP:
        stream << "TOUS : " << value << ". Coût -1 par Attaque ce tour.";
        break;
    case engine::CardId::RAMPAGE:
        stream << "Inflige " << value << ". +" << secondary << " dégâts pour le combat.";
        break;
    case engine::CardId::BURST:
        stream << "Ce tour : " << secondary << " Compétence(s) jouée(s) 2 fois.";
        break;
    case engine::CardId::BULLET_TIME:
        stream << "Ce tour : plus de pioche, la main coûte 0.";
        break;
    case engine::CardId::PINPOINT:
        stream << "Inflige " << value << ". Coût -1 par Compétence.";
        break;
    case engine::CardId::POUNCE:
        stream << "Inflige " << value << ". Compétence suivante : coût 0.";
        break;
    case engine::CardId::ECHO_FORM:
        stream << (card.upgraded ? "" : "Éthéré. ") << "1re carte du tour : jouée 2 fois.";
        break;
    case engine::CardId::SIGNAL_BOOST:
        stream << "Prochain Pouvoir joué 2 fois. Épuisement.";
        break;
    case engine::CardId::UPROAR:
        stream << value << " dégâts x" << secondary << ". Joue 1 Attaque de la pioche.";
        break;
    case engine::CardId::ADAPTIVE_STRIKE:
        stream << "Inflige " << value << ". Copie à coût 0 en défausse.";
        break;
    case engine::CardId::SYNTHESIS:
        stream << "Inflige " << value << ". Prochain Pouvoir : coût 0.";
        break;
    case engine::CardId::GENETIC_ALGORITHM:
        stream << "Armure " << value << ". +" << secondary << " pour toujours. Épuisement.";
        break;
    case engine::CardId::MOMENTUM_STRIKE:
        stream << "Inflige " << value << ". Cette carte coûte ensuite 0.";
        break;
    case engine::CardId::CLAW:
        stream << "Inflige " << value << ". Vos Griffures : +" << secondary << " dégâts.";
        break;
    case engine::CardId::MODDED:
        stream << "+" << value << " Emplacement. Pioche " << secondary << ". Puis coût +1.";
        break;
    case engine::CardId::BOMBARDMENT:
        stream << "Inflige " << value << ". Épuis. Rejouée à chaque tour.";
        break;
    case engine::CardId::I_AM_INVINCIBLE:
        stream << "Armure " << value << ". Jouée si 1re de la pioche.";
        break;
    case engine::CardId::VOID_FORM:
        stream << (card.upgraded ? "" : "Éthéré. ") << "Fin du tour. " << value << " cartes à 0/tour.";
        break;
    case engine::CardId::KINGLY_KICK:
        stream << "Inflige " << value << ". Coût -1 quand piochée.";
        break;
    case engine::CardId::KINGLY_PUNCH:
        stream << "Inflige " << value << ". +" << secondary << " dégâts quand piochée.";
        break;
    case engine::CardId::EIDOLON:
        stream << "Joue vos Éthérées épuisées. Épuisement.";
        break;
    case engine::CardId::BANSHEES_CRY:
        stream << "TOUS : " << value << ". Coût -2 par Éthérée jouée.";
        break;
    case engine::CardId::VEILPIERCER:
        stream << "Inflige " << value << ". Prochaine Éthérée : coût 0.";
        break;
    case engine::CardId::BORROWED_TIME:
        stream << "Énergie +" << value << ". Ce tour, cartes à coût +" << secondary << ".";
        break;
    case engine::CardId::THE_SCYTHE:
        stream << "Inflige " << value << ". +" << secondary << " pour toujours. Épuisement.";
        break;
    case engine::CardId::MELANCHOLY:
        stream << "Armure " << value << ". Coût -1 à chaque mort.";
        break;
    case engine::CardId::HANG:
        stream << "Inflige " << value << ". Pendaison x2 sur cet ennemi.";
        break;
    case engine::CardId::SOOT:
    case engine::CardId::WOUND:
        stream << "Injouable.";
        break;
    case engine::CardId::FOLLY:
        stream << "Injouable. Inné. Éthéré. Éternel.";
        break;
    case engine::CardId::ENTHRALLED:
        stream << "À jouer avant les autres cartes. Éternel.";
        break;
    case engine::CardId::BURN:
        stream << "Injouable. Fin de tour en main : " << value << " dégâts.";
        break;
    case engine::CardId::DAZED:
        stream << "Injouable. Éthéré.";
        break;
    case engine::CardId::DEBRIS:
        stream << "Épuisement.";
        break;
    case engine::CardId::SLIMED:
        stream << "Pioche " << value << " carte. Épuisement.";
        break;
    case engine::CardId::VOID_CARD:
        stream << "Injouable. Éthéré. Piochée : perd " << value << " ⚡.";
        break;
    case engine::CardId::FUEL:
        stream << "Gagne " << value << " ⚡. Épuisement.";
        break;
    case engine::CardId::BOOST_AWAY:
        stream << "Gagne " << value << " d'Armure. 1 Hébété en défausse.";
        break;
    case engine::CardId::FLAK_CANNON:
        stream << "Statuts épuisés : " << value << " au hasard par carte.";
        break;
    case engine::CardId::SMOKESTACK:
        stream << "Statut créé : " << value << " dégâts à TOUS.";
        break;
    case engine::CardId::COMPACT:
        stream << "Armure " << value << ". Statuts en main : " << (card.upgraded ? "Combustible+." : "Combustible.");
        break;
    case engine::CardId::GUNK_UP:
        stream << "Inflige " << value << " " << secondary << " fois. 1 Englué en défausse.";
        break;
    case engine::CardId::ITERATION:
        stream << "1er Statut pioché du tour : pioche " << value << ".";
        break;
    case engine::CardId::FIGHT_THROUGH:
        stream << "Gagne " << value << " d'Armure. " << secondary << " Plaies en défausse.";
        break;
    case engine::CardId::OVERCLOCK:
        stream << "Pioche " << value << " cartes. 1 Brûlure en défausse.";
        break;
    case engine::CardId::ROCKET_PUNCH:
        stream << "Inflige " << value << ". Pioche " << secondary << ". -1 ⚡ par Statut.";
        break;
    case engine::CardId::TRASH_TO_TREASURE:
        stream << "Statut créé : canalise 1 Orbe au hasard.";
        break;
    case engine::CardId::TURBO:
        stream << "Gagne " << value << " ⚡. 1 Néant en défausse.";
        break;
    case engine::CardId::COLLISION_COURSE:
        stream << "Inflige " << value << " dégâts. 1 Débris en main.";
        break;
    case engine::CardId::CRASH_LANDING:
        stream << "TOUS : " << value << ". Main remplie de Débris.";
        break;
    case engine::CardId::GIANT_ROCK:
        stream << "Inflige " << value << " dégâts.";
        break;
    case engine::CardId::SHIV:
        stream << "Inflige " << value << " dégâts."
               << (card.enchantment == engine::Enchantment::INKY ? " Affaibli 1." : "") << " Épuisement.";
        break;
    case engine::CardId::BLADE_DANCE:
        stream << "Ajoute " << value << " Surins à la main. Épuisement.";
        break;
    case engine::CardId::CLOAK_AND_DAGGER:
        stream << "Gagne " << value << " d'Armure. Surins en main : " << secondary << ".";
        break;
    case engine::CardId::LEADING_STRIKE:
        stream << "Inflige " << value << " dégâts. " << secondary << " Surins en main.";
        break;
    case engine::CardId::BLADE_OF_INK:
        stream << "Ajoute " << value << " Surins encrés à la main.";
        break;
    case engine::CardId::INFINITE_BLADES:
        stream << (card.upgraded ? "Inné. " : "") << "Début de tour : " << value << " Surin en main.";
        break;
    case engine::CardId::STORM_OF_STEEL:
        stream << (card.upgraded ? "Défausse la main : 1 Surin+ par carte." : "Défausse la main : 1 Surin par carte.");
        break;
    case engine::CardId::FAN_OF_KNIVES:
        stream << "Surins : touchent TOUS. " << value << " en main.";
        break;
    case engine::CardId::PHANTOM_BLADES:
        stream << "Surins : Retenue. 1er du tour : +" << value << ".";
        break;
    case engine::CardId::ACCURACY:
        stream << "Les Surins infligent " << value << " dégâts de plus.";
        break;
    case engine::CardId::UP_MY_SLEEVE:
        stream << value << " Surins en main. Puis coût -1.";
        break;
    case engine::CardId::KNIFE_TRAP:
        stream << (card.upgraded ? "Améliore et joue les Surins épuisés." : "Joue les Surins épuisés sur l'ennemi.");
        break;
    case engine::CardId::ARMAMENTS:
        stream << "Armure " << value << ". " << (card.upgraded ? "Améliore la main." : "Améliore 1 carte en main.");
        break;
    case engine::CardId::BRAND:
        stream << "Perd " << value << " PV. Épuise 1 carte. Force +" << secondary << ".";
        break;
    case engine::CardId::BURNING_PACT:
        stream << "Épuise 1 carte. Pioche " << value << " cartes.";
        break;
    case engine::CardId::ACROBATICS:
        stream << "Pioche " << value << " cartes. Défausse 1 carte.";
        break;
    case engine::CardId::DAGGER_THROW:
        stream << "Inflige " << value << ". Pioche " << secondary << ", défausse 1.";
        break;
    case engine::CardId::PREPARED:
        stream << "Pioche " << value << ", puis défausse " << value << ".";
        break;
    case engine::CardId::HIDDEN_DAGGERS:
        stream << "Défausse " << value << " cartes. " << secondary << (card.upgraded ? " Surins+ en main." : " Surins en main.");
        break;
    case engine::CardId::SCAVENGE:
        stream << "Épuise 1 carte. Tour suivant : +" << value << " ⚡.";
        break;
    case engine::CardId::PHOTON_CUT:
        stream << "Inflige " << value << ". Pioche " << secondary << ", remet 1 carte.";
        break;
    case engine::CardId::GLIMMER:
        stream << "Pioche " << value << ". Remet 1 carte sur la pioche.";
        break;
    case engine::CardId::TYRANNY:
        stream << (card.upgraded ? "Inné. " : "") << "Début de tour : pioche " << value << ", épuise " << value << ".";
        break;
    case engine::CardId::SCULPTING_STRIKE:
        stream << "Inflige " << value << ". 1 carte devient Éthérée.";
        break;
    case engine::CardId::JUGGLING:
        stream << (card.upgraded ? "Inné. " : "") << "3e Attaque du tour : copie en Main.";
        break;
    case engine::CardId::EXPOSE:
        stream << "Armure, Artéf. à 0. Vulnérable " << secondary << ". Épuis.";
        break;
    case engine::CardId::HEADBUTT:
        stream << "Inflige " << value << ". 1 défaussée sur la pioche.";
        break;
    case engine::CardId::HOLOGRAM:
        stream << "Armure " << value << ". 1 défaussée en main." << (card.upgraded ? "" : " Épuis.");
        break;
    case engine::CardId::COSMIC_INDIFFERENCE:
        stream << "Armure " << value << ". 1 défaussée sur la pioche.";
        break;
    case engine::CardId::FOREGONE_CONCLUSION:
        stream << "Tour suivant : " << value << " cartes de la pioche.";
        break;
    case engine::CardId::GRAVEBLAST:
        stream << "Inflige " << value << ". 1 défaussée en main." << (card.upgraded ? "" : " Épuis.");
        break;
    case engine::CardId::DREDGE:
        stream << (card.upgraded ? "Retenue. " : "") << value << " défaussées en main. Épuis.";
        break;
    case engine::CardId::CLEANSE:
        stream << "Titos +" << value << " PV. Épuise 1 de la pioche.";
        break;
    case engine::CardId::STONE_ARMOR:
    case engine::CardId::NEUTRON_AEGIS:
        stream << "Gagne " << value << " de Blindage.";
        break;
    case engine::CardId::FLAME_BARRIER:
        stream << "Armure " << value << ". Attaqué : " << secondary << " dégâts en retour.";
        break;
    case engine::CardId::WRAITH_FORM:
        stream << "Intangible " << value << ". Chaque tour : Dextérité -" << secondary << ".";
        break;
    case engine::CardId::BUFFER:
        stream << "Annule " << value << " perte" << (value > 1 ? "s" : "") << " de PV.";
        break;
    case engine::CardId::REFLECT:
        stream << "Armure " << value << ". Dégâts bloqués renvoyés.";
        break;
    case engine::CardId::PATTER:
        stream << "Gagne " << value << " d'Armure. Vigueur " << secondary << ".";
        break;
    case engine::CardId::TERRAFORMING:
        stream << "Gagne " << value << " de Vigueur.";
        break;
    case engine::CardId::SOVEREIGN_BLADE:
        stream << "Retenue. Inflige " << value << " dégâts.";
        break;
    case engine::CardId::REFINE_BLADE:
        stream << "Forge " << value << ". Tour suivant : Énergie +" << secondary << ".";
        break;
    case engine::CardId::SPOILS_OF_BATTLE:
        stream << "Forge " << value << ". Pioche " << secondary << " cartes.";
        break;
    case engine::CardId::WROUGHT_IN_WAR:
        stream << "Inflige " << value << " dégâts. Forge " << secondary << ".";
        break;
    case engine::CardId::BULWARK:
        stream << "Gagne " << value << " d'Armure. Forge " << secondary << ".";
        break;
    case engine::CardId::CONQUEROR:
        stream << "Forge " << value << ". Ce tour : Lame x2 sur l'ennemi.";
        break;
    case engine::CardId::FURNACE:
        stream << "Début de tour : Forge " << value << ".";
        break;
    case engine::CardId::PARRY:
        stream << "Lame souveraine donne " << value << " d'Armure.";
        break;
    case engine::CardId::SUMMON_FORTH:
        stream << "Lame souveraine en main. Forge " << value << ".";
        break;
    case engine::CardId::BEAT_INTO_SHAPE:
        stream << "Inflige " << value << ". Forge " << secondary << ", +" << secondary << " par coup reçu.";
        break;
    case engine::CardId::BIG_BANG:
        stream << (card.upgraded ? "Inné. " : "") << "Pioche " << secondary << ". 1⚡, 1★. Forge " << value << "." << (card.upgraded ? " Épuis." : " Épuisement.");
        break;
    case engine::CardId::SEEKING_EDGE:
        stream << "Forge " << value << ". Lame souveraine touche TOUS.";
        break;
    case engine::CardId::SWORD_SAGE:
        stream << "Lame souveraine : jouée " << value << " fois de plus.";
        break;
    case engine::CardId::THE_SMITH:
        stream << "Forge " << value << ".";
        break;
    case engine::CardId::POKE:
        stream << "Titos inflige " << value << " dégâts.";
        break;
    case engine::CardId::SNAP:
        stream << "Titos inflige " << value << ". Retenue sur 1 carte.";
        break;
    case engine::CardId::RATTLE:
        stream << "Titos : " << value << ", +1 fois par attaque ce tour.";
        break;
    case engine::CardId::BONE_SHARDS:
        stream << "Titos périt : " << value << " à TOUS, " << secondary << " d'Armure.";
        break;
    case engine::CardId::SQUEEZE:
        stream << "Titos : " << value << ", +" << secondary << " par autre Attaque Titos.";
        break;
    case engine::CardId::FLATTEN:
        stream << "Titos inflige " << value << ". Coût 0 s'il a attaqué.";
        break;
    case engine::CardId::HIGH_FIVE:
        stream << "Titos : " << value << " et Vulnérable " << secondary << " à TOUS.";
        break;
    case engine::CardId::FETCH:
        stream << "Titos inflige " << value << ". 1re du tour : pioche " << secondary << ".";
        break;
    case engine::CardId::CALCIFY:
        stream << "Attaques de Titos : +" << value << " dégâts.";
        break;
    case engine::CardId::RIGHT_HAND_HAND:
        stream << "Titos : " << value << ". Carte à " << secondary << " ⚡ ou plus : revient.";
        break;
    case engine::CardId::NECRO_MASTERY:
        stream << "Invoque Titos (" << value << "). Ses PV perdus : TOUS.";
        break;
    case engine::CardId::PROTECTOR:
        stream << "Titos inflige " << value << " dégâts + ses PV Max.";
        break;
    case engine::CardId::SACRIFICE:
        stream << "Retenue. Titos périt : Armure 3x PV Max.";
        break;
    case engine::CardId::SPUR:
        stream << "Retenue. Invoque Titos (" << value << " PV). Soin " << secondary << ".";
        break;
    case engine::CardId::SIC_EM:
        stream << "Titos : " << value << ". Ce tour, touché : Invoque " << secondary << ".";
        break;
    case engine::CardId::SENTRY_MODE:
        stream << "Début de tour : " << value << " Balayage optique.";
        break;
    case engine::CardId::SWEEPING_GAZE:
        stream << "Éthéré. Titos : " << value << " au hasard. Épuisement.";
        break;
    case engine::CardId::SOUL:
        stream << "Pioche " << value << " cartes. Épuisement.";
        break;
    case engine::CardId::REAVE:
        stream << "Inflige " << value << " dégâts. Pioche : " << (card.upgraded ? "1 Âme+." : "1 Âme.");
        break;
    case engine::CardId::GRAVE_WARDEN:
        stream << "Gagne " << value << " d'Armure. Pioche : 1 Âme.";
        break;
    case engine::CardId::CAPTURE_SPIRIT:
        stream << "L'ennemi perd " << value << " PV. Pioche : " << secondary << " Âmes.";
        break;
    case engine::CardId::SEVERANCE:
        stream << "Inflige " << value << ". Âme : Pioche, Main, Défausse.";
        break;
    case engine::CardId::DIRGE:
        stream << "Invoque " << value << " X fois. " << (card.upgraded ? "X Âmes+" : "X Âmes") << ". Épuisement.";
        break;
    case engine::CardId::HAUNT:
        stream << "Âme jouée : ennemi au hasard, -" << value << " PV.";
        break;
    case engine::CardId::DEVOUR_LIFE:
        stream << "Âme jouée : Invoque Titos (" << value << " PV).";
        break;
    case engine::CardId::SOUL_STORM:
        stream << "Inflige " << value << ", +" << secondary << " par Âme Épuisée.";
        break;
    case engine::CardId::GLIMPSE_BEYOND:
        stream << "Ajoute " << value << " Âmes à la Pioche. Épuisement.";
        break;
    case engine::CardId::MANIFEST_AUTHORITY:
        stream << "Gagne " << value << " d'Armure. 1 Incolore" << (card.upgraded ? "+" : "") << " en Main.";
        break;
    default:
        break;
    }
}

namespace
{

bn::string_view cost_digit(uint8_t cost)
{
    static constexpr bn::string_view DIGITS[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12" };
    constexpr int HIGHEST = sizeof(DIGITS) / sizeof(DIGITS[0]) - 1;

    if(cost == engine::COST_X)
    {
        return "X";
    }

    return DIGITS[bn::min(int(cost), HIGHEST)];
}

}

bn::string_view card_cost_label(const engine::DeckCard& card)
{
    if(engine::CardDef::has_keyword(card, engine::KEYWORD_UNPLAYABLE))
    {
        return "-";
    }

    return cost_digit(engine::CardDef::get_cost(card.id, card.upgraded));
}

bn::string_view cost_label(int cost)
{
    return cost_digit(uint8_t(bn::clamp(cost, 0, 255)));
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
    case engine::CardType::STATUS:
        return "Statut";
    case engine::CardType::CURSE:
        return "Malédiction";
    case engine::CardType::QUEST:
        return "Quête";
    default:
        return "Pouvoir";
    }
}

void status_line(const engine::CombatEntity& entity, bn::istring& out, int max_width)
{
    // Room one status takes at most, in bytes: a space, an accented label, a sign and the amount.
    constexpr int STATUS_BYTES = 16;
    constexpr bn::string_view MORE = " +";

    out.clear();
    bn::ostringstream stream(out);

    for(const engine::StatusEffect& status : entity.statuses)
    {
        if(status.amount == 0)
        {
            continue;
        }

        // The statuses that do not fit, in the string or on screen, are replaced by a "+" mark.
        const int size_before = out.size();

        if(size_before + STATUS_BYTES + MORE.size() > out.max_size())
        {
            stream << MORE;
            break;
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
            stream << "Blind" << status.amount;
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
        case engine::StatusType::THORNS:
            stream << "Épine" << status.amount;
            break;
        case engine::StatusType::INTANGIBLE:
            stream << "Intang" << status.amount;
            break;
        case engine::StatusType::BUFFER:
            stream << "Tampon" << status.amount;
            break;
        case engine::StatusType::FRAIL:
            stream << "Frag" << status.amount;
            break;
        case engine::StatusType::VIGOR:
            stream << "Vig" << status.amount;
            break;
        case engine::StatusType::REGEN:
            stream << "Régén" << status.amount;
            break;
        case engine::StatusType::CONFUSED:
            stream << "Désor";
            break;
        default:
            break;
        }

        if(size_before > 0 && text_width(out) + text_width(MORE) > max_width)
        {
            out.shrink(size_before);
            stream << MORE;
            break;
        }
    }
}

bn::string_view enemy_power_label(engine::PowerId id)
{
    switch(id)
    {
    case engine::PowerId::TANGLED:
        return "Paraly";
    case engine::PowerId::TENDER:
        return "Décomp";
    case engine::PowerId::PERSONAL_HIVE:
        return "Ruche";
    default:
        return "";
    }
}

bn::string_view enemy_power_description(engine::PowerId id)
{
    switch(id)
    {
    case engine::PowerId::TANGLED:
        return "Paralysie : Attaques +1 ● ce tour.";
    case engine::PowerId::TENDER:
        return "Décomp. : chaque carte, -1 Force/Dex.";
    case engine::PowerId::PERSONAL_HIVE:
        return "Ruche : frappée, Hébété en Pioche.";
    default:
        return "";
    }
}

void append_enemy_powers(const engine::CombatEngine& combat, int holder, bn::istring& out, int max_width)
{
    bn::ostringstream stream(out);

    for(const engine::ActivePower& power : combat.powers)
    {
        bn::string_view label = enemy_power_label(power.id);

        if(label.empty() || power.holder != holder)
        {
            continue;
        }

        const int size_before = out.size();

        // Room for a space, the label and its amount.
        if(size_before + label.size() + 8 > out.max_size())
        {
            break;
        }

        if(! out.empty())
        {
            stream << ' ';
        }

        stream << label << power.amount;

        if(size_before > 0 && text_width(out) > max_width)
        {
            out.shrink(size_before);
            break;
        }
    }
}

}
