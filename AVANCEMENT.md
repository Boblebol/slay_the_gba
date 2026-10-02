# Avancement de Slay the GBA

> Fichier généré par `make progress` : ne pas le modifier à la main. Les cartes, reliques, potions,
> monstres et évènements sont détectés dans le code du moteur ; les mécaniques, systèmes et écrans
> se règlent dans [`avancement.toml`](avancement.toml).

**Avancement global : 28,8 %** — 344 faits, 26 partiels, 868 à faire (36 hors périmètre).

Un élément partiel compte pour moitié. Le référentiel est le contenu de Slay the Spire 2 (patch v0.111.0 Beta, `bible/data`).

Déjà jouables mais absents du référentiel Slay the Spire 2, donc non comptés : cartes Cleave, Metallicize ; monstres Cultist, Gremlin Nob, Jaw Worm, Louse ; évènements Mystic Altar.

## Synthèse

| Section | Fait | Partiel | À faire | Avancement |
|---|---:|---:|---:|---:|
| [Soldat de Fer](#ironclad) | 68 | 2 | 32 | 67,6 % |
| [La Silencieuse](#silent) | 56 | 2 | 43 | 56,4 % |
| [Le Défectueux](#defect) | 43 | 2 | 60 | 41,9 % |
| [Le Régent](#regent) | 50 | 1 | 49 | 50,5 % |
| [La Nécrolieuse](#necrobinder) | 33 | 3 | 65 | 34,2 % |
| [Cartes communes aux cinq éditions](#cartes) | 0 | 0 | 120 | 0,0 % |
| [Reliques et potions](#reliques) | 38 | 4 | 279 | 12,5 % |
| [Bestiaire](#bestiaire) | 0 | 1 | 103 | 0,5 % |
| [Évènements et Anciens](#rencontres) | 0 | 3 | 62 | 2,3 % |
| [Règles de combat](#regles) | 22 | 0 | 30 | 42,3 % |
| [Systèmes de jeu](#systemes) | 19 | 2 | 15 | 55,6 % |
| [Interface, audio et cartouches](#interface) | 15 | 6 | 10 | 58,1 % |
| **Total** | **344** | **26** | **868** | **28,8 %** |

<a id="ironclad"></a>

## Soldat de Fer — 67,6 %

### Cartes — 70,3 %

60 faits, 1 partiel, 25 à faire (4 hors périmètre).

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Défense | Base · Compétence | Oui |  |
| Frappe | Base · Attaque | Oui |  |
| Heurt | Base · Attaque | Oui |  |
| Armement | Commune · Compétence | Non |  |
| Colère | Commune · Attaque | Oui |  |
| Coup de boule | Commune · Attaque | Non |  |
| Coup de tonnerre | Commune · Attaque | Oui |  |
| Frappe du pommeau | Commune · Attaque | Oui |  |
| Frappe jumelée | Commune · Attaque | Oui |  |
| Frappe perfectionnée | Commune · Attaque | Oui |  |
| Frappe préparatoire | Commune · Attaque | Oui |  |
| Intimidation | Commune · Compétence | Oui |  |
| Mur de sang | Commune · Compétence | Oui |  |
| Même pas mal | Commune · Compétence | Oui |  |
| Percée | Commune · Attaque | Oui |  |
| Plaquage | Commune · Attaque | Oui |  |
| Poing en fusion | Commune · Attaque | Oui |  |
| Provocation | Commune · Compétence | Oui |  |
| Ravage | Commune · Compétence | Non |  |
| Scorie | Commune · Attaque | Oui |  |
| Vague de fer | Commune · Attaque | Oui |  |
| Virilité | Commune · Compétence | Partiel | Améliorée, la carte Épuisée devrait être choisie : elle reste tirée au hasard. [#31](https://github.com/Boblebol/slay_the_gba/issues/31) |
| Épée boomerang | Commune · Attaque | Oui |  |
| Acharnement | Peu commune · Attaque | Non |  |
| Armure de pierre | Peu commune · Pouvoir | Non |  |
| Barrière de flammes | Peu commune · Compétence | Non |  |
| Bouclier démoniaque | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Brasier | Peu commune · Pouvoir | Oui |  |
| Colosse | Peu commune · Compétence | Non |  |
| Cruauté | Peu commune · Pouvoir | Non |  |
| Démantèlement | Peu commune · Attaque | Oui |  |
| Embrasement | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Enflammer | Peu commune · Pouvoir | Oui |  |
| Frappe cendrée | Peu commune · Attaque | Oui |  |
| Gourdin | Peu commune · Attaque | Oui |  |
| Harcèlement | Peu commune · Attaque | Oui |  |
| Hémokinésie | Peu commune · Attaque | Oui |  |
| Ignorer la douleur | Peu commune · Pouvoir | Oui |  |
| Ivresse du combat | Peu commune · Compétence | Oui |  |
| Jonglage | Peu commune · Pouvoir | Non |  |
| Lame infernale | Peu commune · Compétence | Non |  |
| Mauvais œil | Peu commune · Compétence | Oui |  |
| Pacte brûlant | Peu commune · Compétence | Non |  |
| Paré au combat | Peu commune · Compétence | Oui |  |
| Pillage | Peu commune · Attaque | Oui |  |
| Piétinement | Peu commune · Attaque | Non |  |
| Rage | Peu commune · Compétence | Oui |  |
| Rancune | Peu commune · Attaque | Oui |  |
| Rituel oublié | Peu commune · Compétence | Oui |  |
| Rugissement de l'au-delà | Peu commune · Attaque | Non |  |
| Rupture | Peu commune · Pouvoir | Oui |  |
| Ruée | Peu commune · Pouvoir | Non |  |
| Saccage | Peu commune · Attaque | Non |  |
| Saignée | Peu commune · Compétence | Oui |  |
| Scandale | Peu commune · Attaque | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Second souffle | Peu commune · Compétence | Oui |  |
| Tambour de guerre | Peu commune · Compétence | Oui |  |
| Tourbillon | Peu commune · Attaque | Oui |  |
| Uppercut | Peu commune · Attaque | Oui |  |
| Vicieux | Peu commune · Pouvoir | Oui |  |
| Viens te battre  ! | Peu commune · Attaque | Oui |  |
| Agression | Rare · Pouvoir | Oui |  |
| Appétit | Rare · Attaque | Oui |  |
| Attiser les flammes | Rare · Compétence | Non |  |
| Barricade | Rare · Pouvoir | Oui |  |
| Bûcher | Rare · Pouvoir | Oui |  |
| Cascade | Rare · Compétence | Non |  |
| Domination | Rare · Compétence | Oui |  |
| Droite-gauche | Rare · Compétence | Non |  |
| En miettes | Rare · Attaque | Non |  |
| Estropier | Rare · Attaque | Oui |  |
| Feu démoniaque | Rare · Attaque | Oui |  |
| Force primitive | Rare · Compétence | Non |  |
| Forme démoniaque | Rare · Pouvoir | Oui |  |
| Furie | Rare · Attaque | Oui |  |
| Incendie | Rare · Attaque | Oui |  |
| Inflexible | Rare · Pouvoir | Non |  |
| Insensible | Rare · Compétence | Oui |  |
| Juggernaut | Rare · Pouvoir | Oui |  |
| Manteau pourpre | Rare · Pouvoir | Oui |  |
| Marqué au fer rouge | Rare · Compétence | Non |  |
| Minuit | Rare · Attaque | Non |  |
| Offrande | Rare · Compétence | Oui |  |
| Pacte conclu | Rare · Attaque | Oui |  |
| Pas encore | Rare · Compétence | Oui |  |
| Sombre étreinte | Rare · Pouvoir | Oui |  |
| Soulèvement infernal | Rare · Pouvoir | Non |  |
| Tank | Rare · Pouvoir | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Briser | Ancienne · Attaque | Oui |  |
| Corruption | Ancienne · Pouvoir | Non |  |

### Reliques — 12,5 %

1 fait, 0 partiel, 7 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Sang ardent | Départ | Oui |  |
| Crâne rouge | Commune | Non |  |
| Argile autoformante | Peu commune | Non |  |
| Krapo en papier | Peu commune | Non |  |
| Casque délabré | Rare | Non |  |
| Cendres de Charon | Rare | Non |  |
| Langue du Démon | Rare | Non |  |
| Soufre | Boutique | Non |  |

### Mécaniques — 93,8 %

7 faits, 1 partiel, 0 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Force appliquée à chaque attaque |  | Oui |  |
| Vulnérable infligé par ses cartes (Heurt, Provocation) |  | Oui |  |
| Copie de carte dans la Défausse (Colère) |  | Oui |  |
| Dégâts égaux à l'Armure (Plaquage) |  | Oui |  |
| Épuisement de ses cartes et synergies d'Épuisement |  | Oui |  |
| Sacrifice de PV contre de la puissance (cartes « Perdez X PV ») |  | Oui |  |
| Gain de Force tour après tour (Enflammer, Forme démoniaque) |  | Oui |  |
| Animations du héros (repos, attaque, dégâts, garde) |  | Partiel | Flottement au repos, secousse et fondu de mort ; pas de poses dessinées. [#16](https://github.com/Boblebol/slay_the_gba/issues/16) |

<a id="silent"></a>

## La Silencieuse — 56,4 %

### Cartes — 58,7 %

50 faits, 1 partiel, 35 à faire (5 hors périmètre).

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Défense | Base · Compétence | Oui |  |
| Frappe | Base · Attaque | Oui |  |
| Neutralisation | Base · Attaque | Oui |  |
| Survivant | Base · Compétence | Partiel | Donne l'Armure mais ne fait pas défausser de carte. [#31](https://github.com/Boblebol/slay_the_gba/issues/31) |
| Anticipation | Commune · Compétence | Oui |  |
| Cape et épée | Commune · Compétence | Non |  |
| Coup bas | Commune · Attaque | Oui |  |
| Danse des lames | Commune · Compétence | Non |  |
| Dérobade | Commune · Compétence | Oui |  |
| Déviation | Commune · Compétence | Oui |  |
| Frappe initiale | Commune · Attaque | Non |  |
| Hurlement perçant | Commune · Compétence | Oui |  |
| Intouchable | Commune · Compétence | Non |  |
| Lancer de couteau | Commune · Attaque | Non |  |
| Morsure de serpent | Commune · Compétence | Oui |  |
| Pluie de couteaux | Commune · Attaque | Oui |  |
| Poignard empoisonné | Commune · Attaque | Oui |  |
| Poison mortel | Commune · Compétence | Oui |  |
| Prédateur | Commune · Attaque | Oui |  |
| Préparatifs | Commune · Compétence | Non |  |
| Ricochet | Commune · Attaque | Non |  |
| Salto arrière | Commune · Compétence | Oui |  |
| Tic tac | Commune · Attaque | Non |  |
| Trancher | Commune · Attaque | Oui |  |
| Accélérant | Peu commune · Pouvoir | Oui |  |
| Acrobaties | Peu commune · Compétence | Non |  |
| Balayage | Peu commune · Compétence | Oui |  |
| Brouillard | Peu commune · Compétence | Oui |  |
| Coup de poker | Peu commune · Compétence | Oui |  |
| Coup décisif | Peu commune · Attaque | Oui |  |
| Coup résonnant | Peu commune · Attaque | Oui |  |
| Coup sournois | Peu commune · Attaque | Oui |  |
| Décoction | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Embrocher | Peu commune · Attaque | Oui |  |
| Entaille | Peu commune · Attaque | Oui |  |
| Excès de vitesse | Peu commune · Pouvoir | Oui |  |
| Expertise | Peu commune · Compétence | Oui |  |
| Fiole rebondissante | Peu commune · Compétence | Oui |  |
| Fléchettes | Peu commune · Attaque | Oui |  |
| Jeu de jambes | Peu commune · Pouvoir | Oui |  |
| Lames fantomatiques | Peu commune · Pouvoir | Non |  |
| Lames infinies | Peu commune · Pouvoir | Non |  |
| Localisation | Peu commune · Attaque | Non |  |
| Memento mori | Peu commune · Attaque | Oui |  |
| Mirage | Peu commune · Compétence | Oui |  |
| Oubli | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Pas de côté | Peu commune · Compétence | Oui |  |
| Passe-passe | Peu commune · Compétence | Non |  |
| Plan d'évasion | Peu commune · Compétence | Oui |  |
| Poignards dissimulés | Peu commune · Compétence | Non |  |
| Point faible | Peu commune · Compétence | Non |  |
| Précipitation | Peu commune · Attaque | Oui |  |
| Précision | Peu commune · Pouvoir | Non |  |
| Réflexes | Peu commune · Compétence | Non |  |
| Strangulation | Peu commune · Attaque | Oui |  |
| Surgissement | Peu commune · Attaque | Non |  |
| Symphonie de lames | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Tactique | Peu commune · Compétence | Non |  |
| Trucage | Peu commune · Compétence | Non |  |
| Vapeurs toxiques | Peu commune · Pouvoir | Oui |  |
| Venin mousseux | Peu commune · Compétence | Oui |  |
| Vision trouble | Peu commune · Compétence | Oui |  |
| Abrasif | Rare · Pouvoir | Non |  |
| Adrénaline | Rare · Compétence | Oui |  |
| Assassiner | Rare · Attaque | Oui |  |
| Bullet Time | Rare · Compétence | Non |  |
| Camouflage | Rare · Compétence | Non |  |
| Cauchemar | Rare · Compétence | Non |  |
| Clou du spectacle | Rare · Attaque | Oui |  |
| Déflagration | Rare · Compétence | Non |  |
| Encerclement | Rare · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Envenimer | Rare · Pouvoir | Oui |  |
| Forme du serpent | Rare · Pouvoir | Oui |  |
| Furtif | Rare · Pouvoir | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Grands desseins | Rare · Pouvoir | Non |  |
| Image rémanente | Rare · Pouvoir | Oui |  |
| La traque | Rare · Attaque | Non |  |
| Lame d'encre | Rare · Compétence | Non |  |
| Malaise | Rare · Compétence | Oui |  |
| Meurtre | Rare · Attaque | Oui |  |
| Pas de l'ombre | Rare · Compétence | Non |  |
| Pistage | Rare · Pouvoir | Non |  |
| Piège à poignards | Rare · Compétence | Non |  |
| Préméditation | Rare · Pouvoir | Oui |  |
| Savoir-faire | Rare · Pouvoir | Non |  |
| Tempête d'acier | Rare · Compétence | Non |  |
| Vague corrosive | Rare · Compétence | Oui |  |
| Épidémie | Rare · Compétence | Oui |  |
| Éventail de couteaux | Rare · Pouvoir | Non |  |
| Forme spectrale | Ancienne · Pouvoir | Non |  |
| Répression | Ancienne · Attaque | Oui |  |

### Reliques — 37,5 %

3 faits, 0 partiel, 5 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Anneau du serpent | Départ | Oui |  |
| Crâne de geckobra | Commune | Non |  |
| Entonnoir torsadé | Peu commune | Non |  |
| Tingsha | Peu commune | Oui |  |
| Bandages résistants | Rare | Oui |  |
| Fléchette hélicoïdale | Rare | Non |  |
| Grû en papier | Rare | Non |  |
| Parchemin de ninja | Boutique | Non |  |

### Mécaniques — 50,0 %

3 faits, 1 partiel, 3 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Poison : dégâts au début du tour ennemi, traverse l'Armure, diminue de 1 |  | Oui |  |
| Affaibli infligé par ses cartes (Neutralisation, Coup bas) |  | Oui |  |
| Pioche supplémentaire (Salto arrière) |  | Oui |  |
| Défausse au choix (Survivant, Acrobaties) |  | Non | [#31](https://github.com/Boblebol/slay_the_gba/issues/31) |
| Surins : attaques à 0 énergie créées en combat |  | Non | [#96](https://github.com/Boblebol/slay_the_gba/issues/96) |
| Ruse : carte jouée gratuitement quand elle est défaussée |  | Non | [#73](https://github.com/Boblebol/slay_the_gba/issues/73) |
| Animations du héros (repos, attaque, dégâts, garde) |  | Partiel | Flottement au repos, secousse et fondu de mort ; pas de poses dessinées. [#16](https://github.com/Boblebol/slay_the_gba/issues/16) |

<a id="defect"></a>

## Le Défectueux — 41,9 %

### Cartes — 41,9 %

36 faits, 0 partiel, 50 à faire (5 hors périmètre).

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Activation double | Base · Compétence | Oui |  |
| Défense | Base · Compétence | Oui |  |
| Frappe | Base · Attaque | Oui |  |
| Éclair | Base · Compétence | Oui |  |
| Barrage | Commune · Attaque | Non |  |
| Bond | Commune · Compétence | Oui |  |
| Boost | Commune · Compétence | Non |  |
| Compilateur de pilote | Commune · Attaque | Non |  |
| Correctif | Commune · Compétence | Non |  |
| Encrassage | Commune · Attaque | Non |  |
| Faisceau circulaire | Commune · Attaque | Oui |  |
| Foudre en boule | Commune · Attaque | Oui |  |
| Frappe dynamique | Commune · Attaque | Non |  |
| Frappe focalisée | Commune · Attaque | Non |  |
| Griffure | Commune · Attaque | Non |  |
| Hologramme | Commune · Compétence | Non |  |
| Paratonnerre | Commune · Compétence | Oui |  |
| Piles de recharge | Commune · Compétence | Oui |  |
| Rayon cellulaire | Commune · Attaque | Oui |  |
| Sang-froid | Commune · Compétence | Oui |  |
| TURBO | Commune · Compétence | Non |  |
| Tumulte | Commune · Attaque | Non |  |
| Vague de froid | Commune · Attaque | Oui |  |
| Viser les yeux | Commune · Attaque | Oui |  |
| Allumage | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Bobine Tesla | Peu commune · Attaque | Non |  |
| Boucle | Peu commune · Pouvoir | Non |  |
| Bouclier ténébreux | Peu commune · Compétence | Oui |  |
| Bruit blanc | Peu commune · Compétence | Non |  |
| Chaos | Peu commune · Compétence | Non |  |
| Cheminée | Peu commune · Pouvoir | Non |  |
| Compactage | Peu commune · Compétence | Non |  |
| Condensateur | Peu commune · Pouvoir | Non |  |
| FTL | Peu commune · Attaque | Oui |  |
| Flux d'énergie | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Frisson | Peu commune · Compétence | Oui |  |
| Fusion | Peu commune · Compétence | Oui |  |
| Glacier | Peu commune · Compétence | Oui |  |
| Hibernation | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Itération | Peu commune · Pouvoir | Non |  |
| Lutte | Peu commune · Compétence | Non |  |
| Orage | Peu commune · Pouvoir | Oui |  |
| Overclocking | Peu commune · Compétence | Non |  |
| Parcourir | Peu commune · Compétence | Oui |  |
| Pluie de grêlons | Peu commune · Pouvoir | Non |  |
| Poing roquette | Peu commune · Attaque | Non |  |
| Prise de masse | Peu commune · Pouvoir | Non |  |
| Programme de démarrage | Peu commune · Compétence | Oui |  |
| Récupération | Peu commune · Compétence | Non |  |
| Réfraction | Peu commune · Attaque | Non |  |
| Sauvagerie | Peu commune · Pouvoir | Oui |  |
| Scission | Peu commune · Attaque | Oui |  |
| Sous-programme | Peu commune · Pouvoir | Oui |  |
| Synchronisation | Peu commune · Compétence | Non |  |
| Synthèse | Peu commune · Attaque | Non |  |
| Tempête | Peu commune · Compétence | Oui |  |
| Tonnerre | Peu commune · Pouvoir | Non |  |
| Ténèbres | Peu commune · Compétence | Non |  |
| Valeur nulle | Peu commune · Attaque | Oui |  |
| Verrerie | Peu commune · Compétence | Non |  |
| Énergie double | Peu commune · Compétence | Oui |  |
| Éraflure | Peu commune · Attaque | Oui |  |
| Activation multiple | Rare · Compétence | Non |  |
| Algorithme génétique | Rare · Compétence | Non |  |
| Amplification du signal | Rare · Compétence | Non |  |
| Apprentissage guidé | Rare · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Arc-en-ciel | Rare · Compétence | Oui |  |
| Canon anti-aérien | Rare · Attaque | Non |  |
| Défragmentation | Rare · Pouvoir | Non |  |
| Détonation | Rare · Attaque | Non |  |
| Forme résonnante | Rare · Pouvoir | Non |  |
| Frappe adaptative | Rare · Attaque | Non |  |
| Frappe météorique | Rare · Attaque | Oui |  |
| IA créative | Rare · Pouvoir | Non |  |
| Javelot de glace | Rare · Attaque | Oui |  |
| Machine Learning | Rare · Pouvoir | Oui |  |
| Modificateurs | Rare · Compétence | Non |  |
| Ombre dévorante | Rare · Pouvoir | Non |  |
| Perceuse hélicoïdale | Rare · Attaque | Oui |  |
| Redémarrage | Rare · Compétence | Oui |  |
| Revalorisation | Rare · Pouvoir | Non |  |
| Supercritique | Rare · Compétence | Oui |  |
| Tampon | Rare · Pouvoir | Non |  |
| Toupie | Rare · Pouvoir | Non |  |
| Tous pour un | Rare · Attaque | Oui |  |
| Ultralaser | Rare · Attaque | Non |  |
| Un pour tous | Rare · Pouvoir | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Ventilation | Rare · Pouvoir | Non |  |
| Voltaïque | Rare · Compétence | Non |  |
| Activation quadruple | Ancienne · Compétence | Non |  |
| Biais cognitif | Ancienne · Pouvoir | Non |  |

### Reliques — 12,5 %

1 fait, 0 partiel, 7 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Noyau fissuré | Départ | Oui |  |
| Disque dur | Commune | Non |  |
| Câble plaqué or | Peu commune | Non |  |
| Virus symbiotique | Peu commune | Non |  |
| Métronome | Rare | Non |  |
| Pile d'énergie | Rare | Non |  |
| Puce émotionnelle | Rare | Non |  |
| Condensateur runique | Boutique | Non |  |

### Mécaniques — 63,6 %

6 faits, 2 partiels, 3 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Canaliser un Orbe (3 emplacements, le plus ancien est Activé quand ils sont pleins) |  | Oui |  |
| Activer un Orbe (Activation double) |  | Oui |  |
| Orbe kéraunique : 3 dégâts en fin de tour, 8 à l'Activation |  | Oui |  |
| Orbe glaciaire : 2 d'Armure en fin de tour, 5 à l'Activation |  | Oui |  |
| Orbe ténébreux : se charge de 6 par tour, libère sa charge |  | Oui |  |
| Orbe plasmique : 1 énergie par tour, 2 à l'Activation |  | Partiel | Seule Frappe météorique en canalise, et elle coûte 5 d'énergie. |
| Orbe vitreux |  | Non | [#99](https://github.com/Boblebol/slay_the_gba/issues/99) |
| Focalisation : renforce tous les Orbes |  | Non | [#97](https://github.com/Boblebol/slay_the_gba/issues/97) |
| Emplacements d'Orbe variables (Condensateur) |  | Non | [#100](https://github.com/Boblebol/slay_the_gba/issues/100) |
| Compteur d'Orbes affiché en combat |  | Oui |  |
| Animations du héros (repos, attaque, dégâts, garde) |  | Partiel | Flottement au repos, secousse et fondu de mort ; pas de poses dessinées. [#16](https://github.com/Boblebol/slay_the_gba/issues/16) |

<a id="regent"></a>

## Le Régent — 50,5 %

### Cartes — 54,7 %

47 faits, 0 partiel, 39 à faire (5 hors périmètre).

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Chute d'étoile | Base · Attaque | Oui |  |
| Défense | Base · Compétence | Oui |  |
| Frappe | Base · Attaque | Oui |  |
| Vénération | Base · Compétence | Oui |  |
| Broyer | Commune · Attaque | Oui |  |
| Butin de guerre | Commune · Compétence | Non |  |
| Cache secrète | Commune · Compétence | Oui |  |
| Courant pailleté | Commune · Compétence | Oui |  |
| Croissant de lance | Commune · Attaque | Oui |  |
| Entaille photonique | Commune · Attaque | Non |  |
| Forgé par la guerre | Commune · Attaque | Non |  |
| Frappe solaire | Commune · Attaque | Oui |  |
| HORS DE MA VUE ! | Commune · Compétence | Non |  |
| Hiérarchie | Commune · Compétence | Oui |  |
| Indifférence cosmique | Commune · Compétence | Non |  |
| Lame lustrée | Commune · Compétence | Non |  |
| Luminosité | Commune · Compétence | Oui |  |
| Lumière rassemblée | Commune · Compétence | Oui |  |
| Manteau d'étoiles | Commune · Compétence | Oui |  |
| Petits pas | Commune · Compétence | Non |  |
| Puissance céleste | Commune · Attaque | Oui |  |
| Pulsation astrale | Commune · Attaque | Oui |  |
| Trajectoire de collision | Commune · Attaque | Non |  |
| Étoile directrice | Commune · Attaque | Oui |  |
| Alignement | Peu commune · Compétence | Oui |  |
| Autorité manifeste | Peu commune · Compétence | Non |  |
| CHARGEZ !! | Peu commune · Compétence | Non |  |
| Conquérant | Peu commune · Compétence | Non |  |
| Constellation | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Convergence | Peu commune · Compétence | Oui |  |
| Coup de pied royal | Peu commune · Attaque | Non |  |
| Coup de poing royal | Peu commune · Attaque | Non |  |
| Déflagration gamma | Peu commune · Attaque | Oui |  |
| Déflagration lunaire | Peu commune · Attaque | Oui |  |
| Dévastation | Peu commune · Attaque | Oui |  |
| Enfant des étoiles | Peu commune · Pouvoir | Oui |  |
| Esquisse | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Fournaise | Peu commune · Pouvoir | Non |  |
| Frappe rayonnante | Peu commune · Attaque | Oui |  |
| Hors d'état | Peu commune · Attaque | Oui |  |
| Hégémonie | Peu commune · Attaque | Oui |  |
| L'union fait la forge | Peu commune · Compétence | Non |  |
| Largesse | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Lueur | Peu commune · Compétence | Non |  |
| Monologue | Peu commune · Compétence | Oui |  |
| Mur de particules | Peu commune · Compétence | Oui |  |
| Orbite | Peu commune · Pouvoir | Oui |  |
| Parade | Peu commune · Pouvoir | Non |  |
| Pari royal | Peu commune · Compétence | Oui |  |
| Pilier de la création | Peu commune · Pouvoir | Oui |  |
| Point bleu pâle | Peu commune · Pouvoir | Oui |  |
| Poussière d'étoiles | Peu commune · Attaque | Oui |  |
| Prophétie | Peu commune · Compétence | Oui |  |
| Quasar | Peu commune · Compétence | Non |  |
| Rayonnement | Peu commune · Attaque | Oui |  |
| Reflet miroir | Peu commune · Compétence | Non |  |
| Rempart | Peu commune · Compétence | Non |  |
| Résonance | Peu commune · Compétence | Oui |  |
| Spectre visible | Peu commune · Pouvoir | Non |  |
| Supermassif | Peu commune · Attaque | Oui |  |
| Terraformation | Peu commune · Compétence | Non |  |
| Trou noir | Peu commune · Pouvoir | Oui |  |
| Ainsi soit-il | Rare · Attaque | Oui |  |
| Arsenal | Rare · Pouvoir | Oui |  |
| Atterrissage forcé | Rare · Attaque | Non |  |
| Big Bang | Rare · Compétence | Non |  |
| Bombardement | Rare · Attaque | Non |  |
| Cadeau joyeux | Rare · Compétence | Non |  |
| Châtiment | Rare · Attaque | Non |  |
| Comète | Rare · Attaque | Oui |  |
| Couru d'avance | Rare · Compétence | Non |  |
| Foreuse céleste | Rare · Attaque | Oui |  |
| Forme du néant | Rare · Pouvoir | Non |  |
| GARDES !!! | Rare · Compétence | Non |  |
| Genèse | Rare · Pouvoir | Oui |  |
| Grandes décisions | Rare · Compétence | Non |  |
| Je suis invincible | Rare · Compétence | Non |  |
| L'heure du marteau | Rare · Pouvoir | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Lame chercheuse | Rare · Pouvoir | Non |  |
| Le forgeron | Rare · Compétence | Non |  |
| Leçon | Rare · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Marteau d'héritage | Rare · Attaque | Non |  |
| Pléiades | Rare · Attaque | Oui |  |
| Regard monarchique | Rare · Pouvoir | Oui |  |
| Royalties | Rare · Pouvoir | Non |  |
| Sage à l'épée | Rare · Pouvoir | Non |  |
| Tyran | Rare · Pouvoir | Non |  |
| Égide à neutrons | Rare · Pouvoir | Non |  |
| Étoile mourante | Rare · Attaque | Oui |  |
| Le trône scellé | Ancienne · Pouvoir | Oui |  |
| Pluie de météores | Ancienne · Attaque | Oui |  |

### Reliques — 0,0 %

0 fait, 0 partiel, 8 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Droit divin | Départ | Non |  |
| Manuel d'escrime | Commune | Non |  |
| Poussière galactique | Peu commune | Non |  |
| Royalithe | Peu commune | Non |  |
| Mini Régent | Rare | Non |  |
| Pâte orange | Rare | Non |  |
| Pâtisserie lunaire | Rare | Non |  |
| Sbire de Vitruve | Boutique | Non |  |

### Mécaniques — 58,3 %

3 faits, 1 partiel, 2 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Étoiles : seconde ressource gagnée par les cartes |  | Oui |  |
| Cartes à coût en Étoiles |  | Oui |  |
| Forger la Lame souveraine |  | Non | [#102](https://github.com/Boblebol/slay_the_gba/issues/102) |
| Sbires (Frappe de sbire, Sbire en piqué) |  | Non | [#103](https://github.com/Boblebol/slay_the_gba/issues/103) |
| Compteur d'Étoiles affiché en combat |  | Oui |  |
| Animations du héros (repos, attaque, dégâts, garde) |  | Partiel | Flottement au repos, secousse et fondu de mort ; pas de poses dessinées. [#16](https://github.com/Boblebol/slay_the_gba/issues/16) |

<a id="necrobinder"></a>

## La Nécrolieuse — 34,2 %

### Cartes — 36,0 %

30 faits, 2 partiels, 54 à faire (5 hors périmètre).

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Déchaînement | Base · Attaque | Partiel | Dégâts fixes : ne tient pas compte des PV de Titos. [#34](https://github.com/Boblebol/slay_the_gba/issues/34) |
| Défense | Base · Compétence | Oui |  |
| Frappe | Base · Attaque | Oui |  |
| Garde du corps | Base · Compétence | Partiel | Invoque pour 1 et donne de l'Armure, au lieu d'invoquer pour 5. [#34](https://github.com/Boblebol/slay_the_gba/issues/34) |
| Adjuration | Commune · Compétence | Oui |  |
| Anti-pulsation | Commune · Compétence | Non |  |
| Chiquenaude | Commune · Attaque | Non |  |
| Claquement de doigts | Commune · Attaque | Non |  |
| Drain de puissance | Commune · Attaque | Oui |  |
| Défi | Commune · Compétence | Oui |  |
| Déflagration tombale | Commune · Attaque | Non |  |
| Exaction | Commune · Attaque | Non |  |
| Feu follet | Commune · Compétence | Oui |  |
| Fossoyeur | Commune · Compétence | Non |  |
| Frappe du fléau | Commune · Attaque | Non |  |
| Frappe du sculpteur | Commune · Attaque | Non |  |
| Moisson | Commune · Attaque | Oui |  |
| Pestilence | Commune · Compétence | Non |  |
| Profanation | Commune · Attaque | Oui |  |
| Raser | Commune · Attaque | Non |  |
| Râblure | Commune · Compétence | Oui |  |
| Semence | Commune · Attaque | Oui |  |
| Terreur | Commune · Attaque | Oui |  |
| Vie après la mort | Commune · Compétence | Oui |  |
| Amitié | Peu commune · Pouvoir | Oui |  |
| Amoindrissement | Peu commune · Attaque | Non |  |
| Calcification | Peu commune · Pouvoir | Non |  |
| Capture spirituelle | Peu commune · Compétence | Non |  |
| Cliquetis | Peu commune · Attaque | Non |  |
| Césure | Peu commune · Attaque | Non |  |
| Danse Macabre | Peu commune · Pouvoir | Oui |  |
| Droitier | Peu commune · Attaque | Non |  |
| Décomposition | Peu commune · Compétence | Oui |  |
| Décompte | Peu commune · Pouvoir | Non |  |
| Délai | Peu commune · Compétence | Oui |  |
| Déterrer | Peu commune · Compétence | Non |  |
| Détonation osseuse | Peu commune · Attaque | Non |  |
| Enfers | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Enterrer | Peu commune · Attaque | Oui |  |
| Hanter | Peu commune · Pouvoir | Non |  |
| Inéluctable | Peu commune · Compétence | Non |  |
| Légion d'ossements | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Létalité | Peu commune · Pouvoir | Non |  |
| Marche funeste | Peu commune · Attaque | Oui |  |
| Mélancolie | Peu commune · Compétence | Non |  |
| Oraison | Peu commune · Compétence | Non |  |
| Perce-voile | Peu commune · Attaque | Non |  |
| Pluie de papier | Peu commune · Pouvoir | Oui |  |
| Porte-mort | Peu commune · Compétence | Non |  |
| Purge | Peu commune · Compétence | Non |  |
| Putréfaction | Peu commune · Compétence | Oui |  |
| Six pieds sous terre | Peu commune · Attaque | Oui |  |
| Stimulation | Peu commune · Compétence | Non |  |
| Sursis | Peu commune · Compétence | Non |  |
| Tope là | Peu commune · Attaque | Non |  |
| Toucher atrophiant | Peu commune · Compétence | Oui |  |
| Tour de chair | Peu commune · Pouvoir | Oui |  |
| Va chercher | Peu commune · Attaque | Non |  |
| Voile | Peu commune · Pouvoir | Non |  |
| À l'agonie | Peu commune · Compétence | Non |  |
| À l'attaque ! | Peu commune · Attaque | Non |  |
| Âme sœur | Peu commune · Pouvoir | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Appel du néant | Rare · Pouvoir | Non |  |
| Cacophonie | Rare · Pouvoir | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Cri de la banshee | Rare · Attaque | Non |  |
| Destin partagé | Rare · Compétence | Oui |  |
| Domaine | Rare · Pouvoir | Oui |  |
| Dévoration de vie | Rare · Pouvoir | Non |  |
| Eidolon | Rare · Compétence | Non |  |
| Esprit des cendres | Rare · Pouvoir | Oui |  |
| Fin des temps | Rare · Compétence | Non |  |
| Forme de la Faucheuse | Rare · Pouvoir | Non |  |
| Influx neuronal | Rare · Pouvoir | Non |  |
| Joug | Rare · Attaque | Non |  |
| L'heure est venue | Rare · Attaque | Non |  |
| La faux | Rare · Attaque | Non |  |
| Maîtrise nécrologique | Rare · Pouvoir | Non |  |
| Misère | Rare · Attaque | Oui |  |
| Mort-vivant | Rare · Compétence | Oui |  |
| Oblitération | Rare · Compétence | Non |  |
| Pendaison | Rare · Attaque | Non |  |
| Réanimation | Rare · Compétence | Oui |  |
| Sacrifice | Rare · Compétence | Non |  |
| Sentinelle | Rare · Pouvoir | Non |  |
| Séance | Rare · Compétence | Non |  |
| Tempête d'âmes | Rare · Attaque | Non |  |
| Transfiguration | Rare · Compétence | Non |  |
| Vision de l'au-delà | Rare · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Éradication | Rare · Attaque | Oui |  |
| Grimoire interdit | Ancienne · Pouvoir | Non |  |
| Protecteur | Ancienne · Attaque | Non |  |

### Reliques — 0,0 %

0 fait, 0 partiel, 8 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Phylactère enchaîné | Départ | Non |  |
| Flûte d'os | Commune | Non |  |
| Couteau à reliure | Peu commune | Non |  |
| Masque funéraire | Peu commune | Non |  |
| Grand chapeau | Rare | Non |  |
| Marque-page | Rare | Non |  |
| Tuile en ivoire | Rare | Non |  |
| Sceau immortel | Boutique | Non |  |

### Mécaniques — 50,0 %

3 faits, 1 partiel, 3 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Invoquer Titos : ses PV s'additionnent et il encaisse les coups à la place du héros |  | Oui |  |
| Attaques de Titos (Déchaînement proportionnel à ses PV) |  | Non | [#104](https://github.com/Boblebol/slay_the_gba/issues/104) |
| Fatalité : l'ennemi périt quand elle atteint ses PV |  | Non | [#105](https://github.com/Boblebol/slay_the_gba/issues/105) |
| Âmes ajoutées à la Pioche |  | Non | Le moteur ne tient qu'un compteur sans effet. [#106](https://github.com/Boblebol/slay_the_gba/issues/106) |
| Cartes Éthérées |  | Oui |  |
| PV de Titos affichés en combat |  | Oui |  |
| Animations du héros (repos, attaque, dégâts, garde) |  | Partiel | Flottement au repos, secousse et fondu de mort ; pas de poses dessinées. [#16](https://github.com/Boblebol/slay_the_gba/issues/16) |

<a id="cartes"></a>

## Cartes communes aux cinq éditions — 0,0 %

### Cartes incolores — 0,0 %

0 fait, 0 partiel, 53 à faire (12 hors périmètre).

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Artillerie | Peu commune · Attaque | Non |  |
| Assistance | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Automatisation | Peu commune · Pouvoir | Non |  |
| Bagarre | Peu commune · Attaque | Non |  |
| Bien attaché | Peu commune · Pouvoir | Non |  |
| Bouton de secours | Peu commune · Compétence | Non |  |
| Catastrophe | Peu commune · Compétence | Non |  |
| Confiance | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Coordination | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Coup d'avance | Peu commune · Compétence | Non |  |
| Découverte | Peu commune · Compétence | Non |  |
| Défense ultime | Peu commune · Compétence | Non |  |
| Déflagration mentale | Peu commune · Attaque | Non |  |
| Entraves des ténèbres | Peu commune · Compétence | Non |  |
| Entrée fracassante | Peu commune · Attaque | Non |  |
| Finesse | Peu commune · Compétence | Non |  |
| Frappe chercheuse | Peu commune · Attaque | Non |  |
| Frappe ultime | Peu commune · Attaque | Non |  |
| Grégarité | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Hache sifflante | Peu commune · Attaque | Non |  |
| Impatience | Peu commune · Compétence | Non |  |
| Interposition | Peu commune · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| La Boule | Peu commune · Attaque | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| La bombe | Peu commune · Compétence | Non |  |
| Meute | Peu commune · Attaque | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Omnilame | Peu commune · Attaque | Non |  |
| Onde de choc | Peu commune · Compétence | Non |  |
| Panache | Peu commune · Pouvoir | Non |  |
| Polyvalence | Peu commune · Compétence | Non |  |
| Production | Peu commune · Compétence | Non |  |
| Prolongations | Peu commune · Compétence | Non |  |
| Prouesse | Peu commune · Pouvoir | Non |  |
| Relais | Peu commune · Attaque | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Salve | Peu commune · Attaque | Non |  |
| Sans repos | Peu commune · Compétence | Non |  |
| Stratagème | Peu commune · Pouvoir | Non |  |
| Échauffement | Peu commune · Pouvoir | Non |  |
| Éclair d'acier | Peu commune · Attaque | Non |  |
| Épuration | Peu commune · Compétence | Non |  |
| Équilibre | Peu commune · Compétence | Non |  |
| Alchimie | Rare · Compétence | Non |  |
| Arme secrète | Rare · Compétence | Non |  |
| Armure éternelle | Rare · Pouvoir | Non |  |
| Balise d'espoir | Rare · Pouvoir | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Bolas | Rare · Attaque | Non |  |
| Calamité | Rare · Pouvoir | Non |  |
| Clapotis | Rare · Compétence | Non |  |
| Entropie | Rare · Pouvoir | Non |  |
| Grabuge | Rare · Pouvoir | Non |  |
| Griffonage | Rare · Compétence | Non |  |
| Hache dorée | Rare · Attaque | Non |  |
| Imitation | Rare · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Jackpot | Rare · Attaque | Non |  |
| Joyau caché | Rare · Compétence | Non |  |
| La grande manœuvre | Rare · Compétence | Non |  |
| Main de la cupidité | Rare · Attaque | Non |  |
| Maître stratège | Rare · Compétence | Non |  |
| Nostalgie | Rare · Pouvoir | Non |  |
| Passage à tabac | Rare · Compétence | Non |  |
| Pierre qui roule | Rare · Pouvoir | Non |  |
| Pourfendre | Rare · Attaque | Non |  |
| Ralliement | Rare · Compétence | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Renverser | Rare · Attaque | Hors périmètre | Effet multijoueur, sans objet sur GBA. |
| Sacrement | Rare · Compétence | Non |  |
| Technique secrète | Rare · Compétence | Non |  |

### Cartes d'évènement — 0,0 %

0 fait, 0 partiel, 28 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Abondance | Ancienne · Compétence | Non |  |
| Apothéose | Ancienne · Compétence | Non |  |
| Apparition | Ancienne · Compétence | Non |  |
| Flamme éclatante | Ancienne · Compétence | Non |  |
| Fureur de Neow | Ancienne · Attaque | Non |  |
| Mutilation | Ancienne · Attaque | Non |  |
| Relaxation | Ancienne · Compétence | Non |  |
| Sifflet | Ancienne · Attaque | Non |  |
| Vœu | Ancienne · Compétence | Non |  |
| Ambidextrie | Évènement · Compétence | Non |  |
| Aplatissement | Évènement · Attaque | Non |  |
| Astucieux | Évènement · Compétence | Non |  |
| Chausse-trapes | Évènement · Pouvoir | Non |  |
| Conflit | Évènement · Attaque | Non |  |
| Distraction | Évènement · Compétence | Non |  |
| Extermination | Évènement · Attaque | Non |  |
| Fortification | Évènement · Compétence | Non |  |
| Hello World | Évènement · Pouvoir | Non |  |
| Illumination | Évènement · Compétence | Non |  |
| Lacération | Évènement · Attaque | Non |  |
| Métamorphose | Évènement · Compétence | Non |  |
| Picpic | Évènement · Attaque | Non |  |
| Pile | Évènement · Compétence | Non |  |
| Piqué d'oizo | Évènement · Attaque | Non |  |
| Rebond | Évènement · Attaque | Non |  |
| Repas frénétique | Évènement · Compétence | Non |  |
| Science sans conscience | Évènement · Attaque | Non |  |
| Ténacité torique | Évènement · Compétence | Non |  |

### Malédictions — 0,0 %

0 fait, 0 partiel, 18 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Assuétude | Malédiction · Malédiction | Non |  |
| Blessure | Malédiction · Malédiction | Non |  |
| Conscience sporique | Malédiction · Malédiction | Non |  |
| Culpabilité | Malédiction · Malédiction | Non |  |
| Cupidité | Malédiction · Malédiction | Non |  |
| Dette | Malédiction · Malédiction | Non |  |
| Doute | Malédiction · Malédiction | Non |  |
| Déclin | Malédiction · Malédiction | Non |  |
| Fléau de l'ascendant | Malédiction · Malédiction | Non |  |
| Honte | Malédiction · Malédiction | Non |  |
| Hubris | Malédiction · Malédiction | Non |  |
| Insomnie | Malédiction · Malédiction | Non |  |
| Maladresse | Malédiction · Malédiction | Non |  |
| Malchance | Malédiction · Malédiction | Non |  |
| Malédiction de la cloche | Malédiction · Malédiction | Non |  |
| Normalité | Malédiction · Malédiction | Non |  |
| Regrets | Malédiction · Malédiction | Non |  |
| Souffrance | Malédiction · Malédiction | Non |  |

### Quêtes — 0,0 %

0 fait, 0 partiel, 4 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Carte au trésor | Quête · Quête | Non |  |
| Clé de la lanterne | Quête · Quête | Non |  |
| Rhabdomancie | Quête · Quête | Non |  |
| Œuf d'oizodon | Quête · Quête | Non |  |

### Cartes créées en combat et statuts — 0,0 %

0 fait, 0 partiel, 17 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Balayage optique | Créée en combat · Attaque | Non |  |
| Brûlure | Statut · Statut | Non |  |
| Combustible | Créée en combat · Compétence | Non |  |
| Débris | Statut · Statut | Non |  |
| Englué | Statut · Statut | Non |  |
| Frappe de sbire | Créée en combat · Attaque | Non |  |
| Hébété | Statut · Statut | Non |  |
| Lame souveraine | Créée en combat · Attaque | Non |  |
| Néant | Statut · Statut | Non |  |
| Plaie | Statut · Statut | Non |  |
| Rocher gigantesque | Créée en combat · Attaque | Non |  |
| Sbire en piqué | Créée en combat · Attaque | Non |  |
| Sbire sacrificiel | Créée en combat · Compétence | Non |  |
| Suie | Statut · Statut | Non |  |
| Surin | Créée en combat · Attaque | Non |  |
| Âme | Créée en combat · Compétence | Non |  |
| Éclat | Créée en combat · Compétence | Non |  |

<a id="reliques"></a>

## Reliques et potions — 12,5 %

### Reliques partagées — 32,5 %

38 faits, 0 partiel, 79 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Ancre | Commune | Oui |  |
| Aubergine d'améthyste | Commune | Non |  |
| Bracelet Juzu | Commune | Non |  |
| Ceinture à potions | Commune | Non |  |
| Fiole de sang | Commune | Oui |  |
| Fleur joyeuse | Commune | Oui |  |
| Fraise | Commune | Non |  |
| Gorgerette | Commune | Non |  |
| Lanterne | Commune | Oui |  |
| Livre des cinq anneaux | Commune | Non |  |
| Mannequin d'entraînement | Commune | Non |  |
| Masque rouge | Commune | Oui |  |
| Oreiller royal | Commune | Non |  |
| Peinture de guerre | Commune | Non |  |
| Pendule | Commune | Oui |  |
| Pierre à aiguiser | Commune | Non |  |
| Pierre étrangement lisse | Commune | Oui |  |
| Puzzle centenaire | Commune | Oui |  |
| Pétard de fête | Commune | Oui |  |
| Sac de billes | Commune | Oui |  |
| Sacoche de préparatifs | Commune | Non |  |
| Service à thé vénérable | Commune | Non |  |
| Ticket restaurant | Commune | Non |  |
| Vajra | Commune | Oui |  |
| Écailles de bronze | Commune | Non |  |
| Akabeko | Peu commune | Non |  |
| Bassinet à remous | Peu commune | Non |  |
| Billet funéraire | Peu commune | Non |  |
| Bonbon durable | Peu commune | Non |  |
| Bouclier de parade | Peu commune | Oui |  |
| Brassard | Peu commune | Non |  |
| Breloque reptile | Peu commune | Non |  |
| Candélabre | Peu commune | Oui |  |
| Canon miniature | Peu commune | Non |  |
| Chapeau melon | Peu commune | Non |  |
| Corne de diablotin | Peu commune | Oui |  |
| Coupe-papier | Peu commune | Non |  |
| Crapaud pétrifié | Peu commune | Non |  |
| Diapason | Peu commune | Non |  |
| Fard pétillant | Peu commune | Oui |  |
| Kusarigama | Peu commune | Non |  |
| Marteau-piqueur | Peu commune | Oui |  |
| Minuscule boîte aux lettres | Peu commune | Non |  |
| Nunchaku | Peu commune | Non |  |
| Orichalque | Peu commune | Oui |  |
| Pantographe | Peu commune | Oui |  |
| Pergélisol | Peu commune | Oui |  |
| Planisphère | Peu commune | Non |  |
| Plume éternelle | Peu commune | Non |  |
| Pointe de stylo | Peu commune | Non |  |
| Poire | Peu commune | Non |  |
| Poisson chanceux | Peu commune | Non |  |
| Sablier de mercure | Peu commune | Oui |  |
| Taquet d'amarrage | Peu commune | Oui |  |
| Éventail décoratif | Peu commune | Non |  |
| Anneau arc-en-ciel | Rare | Non |  |
| Barre de capitaine | Rare | Oui |  |
| Boucle de cape | Rare | Oui |  |
| Calendrier de pierre | Rare | Oui |  |
| Casque intimidant | Rare | Non |  |
| Casse-tête vexant | Rare | Non |  |
| Chandelier | Rare | Oui |  |
| Crème glacée | Rare | Oui |  |
| Dent de rasoir | Rare | Oui |  |
| Girya | Rare | Non |  |
| Jeton de casino | Rare | Non |  |
| Kunai | Rare | Non |  |
| L'Art de la guerre | Rare | Oui |  |
| Lampe inquiétante | Rare | Non |  |
| Le Coursier | Rare | Non |  |
| Main momifiée | Rare | Non |  |
| Mangue | Rare | Non |  |
| Montre à gousset | Rare | Oui |  |
| Moulin à prières | Rare | Non |  |
| Os à viande | Rare | Non |  |
| Pelle | Rare | Non |  |
| Pince robuste | Rare | Oui |  |
| Pion | Rare | Oui |  |
| Pièce antique | Rare | Non |  |
| Queue de lézard | Rare | Oui |  |
| Shuriken | Rare | Non |  |
| Soufflet | Rare | Oui |  |
| Statue de bête blanche | Rare | Non |  |
| Tige de tungstène | Rare | Non |  |
| Toupie perpétuelle | Rare | Oui |  |
| Vestige battant | Rare | Non |  |
| Étoile blanche | Rare | Non |  |
| Œuf en fusion | Rare | Non |  |
| Œuf gelé | Rare | Non |  |
| Œuf toxique | Rare | Non |  |
| Aile fétiche | Boutique | Non |  |
| Boucle de ceinture | Boutique | Non |  |
| Boîte à outils | Boutique | Non |  |
| Briquet mystique | Boutique | Non |  |
| Bâtonnets enflammés | Boutique | Non |  |
| Carte de fidélité | Boutique | Non |  |
| Chaudron | Boutique | Non |  |
| Composé X | Boutique | Non |  |
| Fronde du courage | Boutique | Oui |  |
| Fruit du dragon | Boutique | Non |  |
| Gaufre de Lee | Boutique | Non |  |
| Graine fantôme | Boutique | Non |  |
| Kifuda | Boutique | Non |  |
| L'Abaque | Boutique | Oui |  |
| Lampe à lave | Boutique | Non |  |
| Marteau tordu | Boutique | Non |  |
| Miroir de Dolly | Boutique | Non |  |
| Pain | Boutique | Oui |  |
| Pichet hurlant | Boutique | Oui |  |
| Planétaire | Boutique | Non |  |
| Poignard | Boutique | Non |  |
| Sceau royal | Boutique | Non |  |
| Tapis miteux | Boutique | Non |  |
| Tente miniature | Boutique | Non |  |
| Triangle résonnant | Boutique | Non |  |
| Lentille de Fresnel | Évènement | Non |  |
| Diadème | Secours | Non |  |

### Reliques d'évènement — 0,0 %

0 fait, 0 partiel, 39 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Anneau du dragon | Départ | Non |  |
| Destin divin | Départ | Non |  |
| Noyau infusé | Départ | Non |  |
| Phylactère déchaîné | Départ | Non |  |
| Sang noir | Départ | Non |  |
| Amulette sombrepierre | Évènement | Non |  |
| Ancre ??? | Évènement | Non |  |
| Attrape-rêves | Évènement | Non |  |
| Bing bong | Évènement | Non |  |
| Champi aromatique | Évènement | Non |  |
| Feu follet perdu | Évènement | Non |  |
| Fille des vents | Évènement | Non |  |
| Fiole de sang ??? | Évènement | Non |  |
| Fleur joyeuse ??? | Évènement | Non |  |
| Fromage d'élection | Évènement | Non |  |
| Gaufre de Lee ??? | Évènement | Non |  |
| Gros champi | Évènement | Non |  |
| La botte | Évènement | Non |  |
| Le tapis du Marchand ??? | Évènement | Non |  |
| Mangue ??? | Évènement | Non |  |
| Mannequin d'entraînement ??? | Évènement | Non |  |
| Manuel d'Histoire | Évènement | Non |  |
| Monsieur Carnage | Évènement | Non |  |
| Noyau pollinique | Évènement | Non |  |
| Oizillon | Évènement | Non |  |
| Orichalque ??? | Évènement | Non |  |
| Perceuse | Évènement | Non |  |
| Pin’s Exclusif de Fidélité Wongo | Évènement | Non |  |
| Poison royal | Évènement | Non |  |
| Service à thé vénérable ??? | Évènement | Non |  |
| Thé d'os | Évènement | Non |  |
| Thé de braises | Évènement | Non |  |
| Thé de l'incivilité | Évènement | Non |  |
| Ticket mystère de Chez Wongo's | Évènement | Non |  |
| Tirelire-gueule | Évènement | Non |  |
| Âme oubliée | Évènement | Non |  |
| Épée de jade | Évènement | Non |  |
| Épée de pierre | Évènement | Non |  |
| Œil de geckobra ??? | Évènement | Non |  |

### Reliques des Anciens — 0,0 %

0 fait, 0 partiel, 102 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Aile de Paël | Ancienne | Non |  |
| Anneau à signet | Ancienne | Non |  |
| Arbalète | Ancienne | Non |  |
| Astrolabe | Ancienne | Non |  |
| Baguette de rhabdomancie | Ancienne | Non |  |
| Bois bénis | Ancienne | Non |  |
| Bois flotté | Ancienne | Non |  |
| Bottes ailées | Ancienne | Non |  |
| Boucle d'oreille susurrante | Ancienne | Non |  |
| Boussole dorée | Ancienne | Non |  |
| Boîte de Pandore | Ancienne | Non |  |
| Boîte à musique | Ancienne | Non |  |
| Bracelet brillant | Ancienne | Non |  |
| Brouillard en conserve | Ancienne | Non |  |
| Cage vide | Ancienne | Non |  |
| Caisse d'alchimie | Ancienne | Non |  |
| Caisse de parchemins | Ancienne | Non |  |
| Caisse à jouets | Ancienne | Non |  |
| Canne à pêche | Ancienne | Non |  |
| Cape distinguée | Ancienne | Non |  |
| Cataplasme | Ancienne | Non |  |
| Chair de Paël | Ancienne | Non |  |
| Chocolat très chaud | Ancienne | Non |  |
| Château de sable | Ancienne | Non |  |
| Ciseaux précis | Ancienne | Non |  |
| Cloche d'appel | Ancienne | Non |  |
| Coffre à bijoux | Ancienne | Non |  |
| Coffret perdu | Ancienne | Non |  |
| Conque retentissante | Ancienne | Non |  |
| Corne de Paël | Ancienne | Non |  |
| Couperet à viande | Ancienne | Non |  |
| Creuset en argent | Ancienne | Non |  |
| Crevaite électrique | Ancienne | Non |  |
| Dent archaïque | Ancienne | Non |  |
| Dent de Paël | Ancienne | Non |  |
| Diadème de diamant | Ancienne | Non |  |
| Délicieux biscuit | Ancienne | Non |  |
| Ectoplasme | Ancienne | Non |  |
| Embarras du choix | Ancienne | Non |  |
| Excroissance de Paël | Ancienne | Non |  |
| Forces précaires | Ancienne | Non |  |
| Fougère délicate | Ancienne | Non |  |
| Fruits exotiques | Ancienne | Non |  |
| Gantelets à pointes | Ancienne | Non |  |
| Gourdin de fer | Ancienne | Non |  |
| Grande capsule | Ancienne | Non |  |
| Griffe de Paël | Ancienne | Non |  |
| Griffes | Ancienne | Non |  |
| Grimoire poussiéreux | Ancienne | Non |  |
| Grooos câlin | Ancienne | Non |  |
| Hache de lancer | Ancienne | Non |  |
| Humidificateur en pierre | Ancienne | Non |  |
| Huître nourrissante | Ancienne | Non |  |
| Joyau prismatique | Ancienne | Non |  |
| Kaléidoscope | Ancienne | Non |  |
| Lanterne-citrouille | Ancienne | Non |  |
| Larmes de Paël | Ancienne | Non |  |
| Livre d'histoires | Ancienne | Non |  |
| Légion de Paël | Ancienne | Non |  |
| Manteau de fourrure | Ancienne | Non |  |
| Marteau de guerre | Ancienne | Non |  |
| Masque serti | Ancienne | Non |  |
| Mitaines réchauffantes | Ancienne | Non |  |
| Nouvelle page | Ancienne | Non |  |
| Osselets de Neow | Ancienne | Non |  |
| Paillettes | Ancienne | Non |  |
| Parasol seigneurial | Ancienne | Non |  |
| Parchemin arcanique | Ancienne | Non |  |
| Parchemin énorme | Ancienne | Non |  |
| Perle dorée | Ancienne | Non |  |
| Perle maudite | Ancienne | Non |  |
| Perle radieuse | Ancienne | Non |  |
| Petite capsule | Ancienne | Non |  |
| Pierre philosophale | Ancienne | Non |  |
| Pierre volcanique | Ancienne | Non |  |
| Pommandre | Ancienne | Non |  |
| Presse-papier en plomb | Ancienne | Non |  |
| Pyramide runique | Ancienne | Non |  |
| Ras du cou en velours | Ancienne | Non |  |
| Rose sanguinolente | Ancienne | Non |  |
| Sacrifice de Neow | Ancienne | Non |  |
| Sang de Paël | Ancienne | Non |  |
| Saï | Ancienne | Non |  |
| Sceau doré | Ancienne | Non |  |
| Serre desséchée | Ancienne | Non |  |
| Sifflet de Tanx | Ancienne | Non |  |
| Soupe nourrissante | Ancienne | Non |  |
| Sozu | Ancienne | Non |  |
| Tablette lourde | Ancienne | Non |  |
| Talisman de Neow | Ancienne | Non |  |
| Toucher d'Orobas | Ancienne | Non |  |
| Tourment de Neow | Ancienne | Non |  |
| Tresse soyeuse | Ancienne | Non |  |
| Tri-boomerang | Ancienne | Non |  |
| Verre marin | Ancienne | Non |  |
| Violon | Ancienne | Non |  |
| Écharpe étincelante | Ancienne | Non |  |
| Étoile noire | Ancienne | Non |  |
| Étui à fioles | Ancienne | Non |  |
| Œil de Paël | Ancienne | Non |  |
| Œil de geckobra | Ancienne | Non |  |
| Œil de verre | Ancienne | Non |  |

### Potions — 3,2 %

0 fait, 4 partiels, 59 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Ampoule explosive | Commune | Non |  |
| Potion d'affaiblissement | Commune | Non |  |
| Potion d'armure | Commune | Partiel | Effet géré par le moteur, pas encore utilisable sur GBA. [#30](https://github.com/Boblebol/slay_the_gba/issues/30) |
| Potion d'attaque | Commune | Non |  |
| Potion d'énergie | Commune | Partiel | Effet géré par le moteur, pas encore utilisable sur GBA. [#30](https://github.com/Boblebol/slay_the_gba/issues/30) |
| Potion d'étoiles | Commune | Non |  |
| Potion de compétence | Commune | Non |  |
| Potion de célérité | Commune | Non |  |
| Potion de dextérité | Commune | Non |  |
| Potion de focalisation | Commune | Non |  |
| Potion de force | Commune | Non |  |
| Potion de poison | Commune | Non |  |
| Potion de pouvoir | Commune | Non |  |
| Potion de vantardise | Commune | Non |  |
| Potion de vitesse | Commune | Non |  |
| Potion de vulnérabilité | Commune | Non |  |
| Potion incendiaire | Commune | Partiel | Effet géré par le moteur, pas encore utilisable sur GBA. [#30](https://github.com/Boblebol/slay_the_gba/issues/30) |
| Potion incolore | Commune | Non |  |
| Potion macabre | Commune | Non |  |
| Potion sanguine | Commune | Partiel | Effet géré par le moteur, pas encore utilisable sur GBA. [#30](https://github.com/Boblebol/slay_the_gba/issues/30) |
| Bronze liquide | Peu commune | Non |  |
| Bénédiction de la forge | Peu commune | Non |  |
| Courage du roi | Peu commune | Non |  |
| Cœur de fer | Peu commune | Non |  |
| Duplicateur | Peu commune | Non |  |
| Décoction du parieur | Peu commune | Non |  |
| Eau cendrée | Peu commune | Non |  |
| Extrait de lucidité | Peu commune | Non |  |
| Fortifiant | Peu commune | Non |  |
| Goutte de folie | Peu commune | Non |  |
| Huile de poissâme | Peu commune | Non |  |
| Infusion osseuse | Peu commune | Non |  |
| Lampée de radiance | Peu commune | Non |  |
| Potion de ruse | Peu commune | Non |  |
| Potion de régénération | Peu commune | Non |  |
| Potion de stockage | Peu commune | Non |  |
| Potion paralysante | Peu commune | Non |  |
| Poudre funeste | Peu commune | Non |  |
| Remède universel | Peu commune | Non |  |
| Sérum de stabilité | Peu commune | Non |  |
| Acide orobique | Rare | Non |  |
| Chaos distillé | Rare | Non |  |
| Concoction cosmique | Rare | Non |  |
| Don de Mazaleth | Rare | Non |  |
| Décoction entropique | Rare | Non |  |
| Essence ténébreuse | Rare | Non |  |
| Fantôme dans un bocal | Rare | Non |  |
| Fée en bouteille | Rare | Non |  |
| Goules en bouteille | Rare | Non |  |
| Goutte de prescience | Rare | Non |  |
| Huile de geckobra | Rare | Non |  |
| Jus de coléoptère | Rare | Non |  |
| Jus de fruits | Rare | Non |  |
| Navire en bouteille | Rare | Non |  |
| Philtre chanceux | Rare | Non |  |
| Potentiel en bouteille | Rare | Non |  |
| Potion d'entrave | Rare | Non |  |
| Potion de géantification | Rare | Non |  |
| Ragoût militaire | Rare | Non |  |
| Souvenirs liquides | Rare | Non |  |
| Ambre gris | Évènement | Non |  |
| Potion infecte | Évènement | Non |  |
| Potion luminescente | Évènement | Non |  |

<a id="bestiaire"></a>

## Bestiaire — 0,5 %

### Monstres — 0,0 %

0 fait, 0 partiel, 72 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Assemblage Cubex | Monstre · Acte 1 (A) · Acte 3 | Non |  |
| Bourbelianes | Monstre · Acte 1 (A) | Non |  |
| Encrelin | Monstre · Acte 1 (A) | Non |  |
| Fongibrume | Monstre · Acte 1 (A) | Non |  |
| Grignotin | Monstre · Acte 1 (A) | Non |  |
| Jaxier à clapet | Monstre · Acte 1 (A) | Non |  |
| Limon brindille (M) | Monstre · Acte 1 (A) | Non |  |
| Limon brindille (S) | Monstre · Acte 1 (A) | Non |  |
| Limon feuillu (M) | Monstre · Acte 1 (A) | Non |  |
| Limon feuillu (S) | Monstre · Acte 1 (A) | Non |  |
| Mycoptère | Monstre · Acte 1 (A) | Non |  |
| Oizillon | Invocation · Acte 1 (A) | Non |  |
| Rampeluche | Monstre · Acte 1 (A) | Non |  |
| Rôdeur Arbalétrier | Monstre · Acte 1 (A) | Non |  |
| Rôdeur Brute | Monstre · Acte 1 (A) | Non |  |
| Rôdeur Traqueur | Monstre · Acte 1 (A) | Non |  |
| Rôdeur assassin | Monstre · Acte 1 (A) | Non |  |
| Rôdeur à la hache | Monstre · Acte 1 (A) | Non |  |
| Scarabrideur | Monstre · Acte 1 (A) | Non |  |
| Écharneur | Monstre · Acte 1 (A) | Non |  |
| Étrangleur | Monstre · Acte 1 (A) | Non |  |
| Œil denté | Monstre · Acte 1 (A) | Non |  |
| Adepte calcifié | Monstre · Acte 1 (B) | Non |  |
| Adepte moisi | Monstre · Acte 1 (B) | Non |  |
| Assemblage de pugilat | Monstre · Acte 1 (B) · Acte 3 | Non |  |
| Bombe à gaz | Monstre · Acte 1 (B) | Non |  |
| Brouillard vivant | Monstre · Acte 1 (B) | Non |  |
| Crapautard | Faible · Acte 1 (B) | Non |  |
| Diablotin mercenaire | Monstre · Acte 1 (B) | Non |  |
| Diablotin sournois | Monstre · Acte 1 (B) | Non |  |
| Gros diablotin | Monstre · Acte 1 (B) | Non |  |
| Navire hanté | Monstre · Acte 1 (B) | Non |  |
| Palourde d'égout | Monstre · Acte 1 (B) | Non |  |
| Putrilimace | Monstre · Acte 1 (B) | Non |  |
| Rat bicaudé | Monstre · Acte 1 (B) | Non |  |
| Tisse-boue | Faible · Acte 1 (B) | Non |  |
| Traqueur fossilisé | Monstre · Acte 1 (B) | Non |  |
| Voyou marin | Monstre · Acte 1 (B) | Non |  |
| Chasseur-tueur | Monstre · Acte 2 | Non |  |
| Crapineux | Monstre · Acte 2 | Non |  |
| Croquebot | Monstre · Acte 2 | Non |  |
| Exosquelette | Monstre · Acte 2 | Non |  |
| L'Obscura | Monstre · Acte 2 | Non |  |
| Mante chapardeuse | Faible · Acte 2 | Non |  |
| Ovicoptère | Monstre · Acte 2 | Non |  |
| Pareffroi | Monstre · Acte 2 | Non |  |
| Progéniteur de poux | Monstre · Acte 2 | Non |  |
| Sarcopte | Monstre · Acte 2 | Non |  |
| Scarabée dormeur | Monstre · Acte 2 | Non |  |
| Sphéropode (Nectar) | Monstre · Acte 2 | Non |  |
| Sphéropode (Pierre) | Monstre · Acte 2 | Non |  |
| Sphéropode (Soie) | Monstre · Acte 2 | Non |  |
| Sphéropode (Œuf) | Monstre · Acte 2 | Non |  |
| Titos | Invocation · Acte 2 | Non |  |
| Tunnelier | Monstre · Acte 2 | Non |  |
| Œuf robuste | Monstre · Acte 2 | Non |  |
| Berserker de vase | Monstre · Acte 3 | Non |  |
| Bouclier vivant | Faible · Acte 3 | Non |  |
| Chevalier rainette | Monstre · Acte 3 | Non |  |
| Déchargebot | Invocation · Acte 3 | Non |  |
| Gardebot | Invocation · Acte 3 | Non |  |
| Globocéphale | Monstre · Acte 3 | Non |  |
| Hachebots | Monstre · Acte 3 | Non |  |
| L'Oublié | Monstre · Acte 3 | Non |  |
| L'Égaré | Monstre · Acte 3 | Non |  |
| Machiniste | Monstre · Acte 3 | Non |  |
| Magistrix | Monstre · Acte 3 | Non |  |
| Opérateur de tourelle | Faible · Acte 3 | Non |  |
| Parchemord | Monstre · Acte 3 | Non |  |
| Poignard-bot | Invocation · Acte 3 | Non |  |
| Sculpteur dévoué | Faible · Acte 3 | Non |  |
| Sonobot | Invocation · Acte 3 | Non |  |

### Élites — 0,0 %

0 fait, 0 partiel, 17 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Idole Oubliée | Élite · Acte 1 (A) | Non |  |
| Larve | Élite · Acte 1 (A) | Non |  |
| Oizodon | Élite · Acte 1 (A) | Non |  |
| Parasite Krapo | Élite · Acte 1 (A) | Non |  |
| Colonie de rôdeurs | Élite · Acte 1 (B) | Non |  |
| Jardinier spectral | Élite · Acte 1 (B) | Non |  |
| Murène de l'effroi | Élite · Acte 1 (B) | Non |  |
| Dix-Mille-Pattes | Élite · Acte 2 | Non |  |
| Dix-Mille-Pattes | Élite · Acte 2 | Non |  |
| Dix-Mille-Pattes | Élite · Acte 2 | Non |  |
| Entomancien | Élite · Acte 2 | Non |  |
| Prisme infesté | Élite · Acte 2 | Non |  |
| Amalg'âmes | Élite · Acte 3 | Non |  |
| Chevalier au fléau | Élite · Acte 3 | Non |  |
| Chevalier mage | Élite · Acte 3 | Non |  |
| Chevalier spectral | Élite · Acte 3 | Non |  |
| Méca-Chevalier | Élite · Acte 3 | Non |  |

### Boss — 3,3 %

0 fait, 1 partiel, 14 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Cerf Auguste | Boss · Acte 1 (A) | Partiel | 150 PV et trois attaques simplifiées, contre 252 PV et six attaques dans le jeu d'origine. [#36](https://github.com/Boblebol/slay_the_gba/issues/36) |
| Disciple de la Tribu | Boss · Acte 1 (A) | Non |  |
| Prêtre de la Tribu | Boss · Acte 1 (A) | Non |  |
| Vantôme | Boss · Acte 1 (A) | Non |  |
| Géant de la Cascade | Boss · Acte 1 (B) | Non |  |
| Matriarche Lagavulin | Boss · Acte 1 (B) | Non |  |
| Poissâme | Boss · Acte 1 (B) | Non |  |
| Anéantisseur | Boss · Acte 2 | Non |  |
| Démon de la Connaissance | Boss · Acte 2 | Non |  |
| L'Insatiable | Boss · Acte 2 | Non |  |
| Roquette | Boss · Acte 2 | Non |  |
| Amalgame Torchetête | Boss · Acte 3 | Non |  |
| Clepséon | Boss · Acte 3 | Non |  |
| Cobaye | Boss · Acte 3 | Non |  |
| Reine | Boss · Acte 3 | Non |  |

<a id="rencontres"></a>

## Évènements et Anciens — 2,3 %

### Évènements — 2,6 %

0 fait, 3 partiels, 54 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Affamé de champis | Acte 3 | Non |  |
| Amalgamateur | Acte 2 | Non |  |
| Arôme du chaos | Acte 1 (A) | Non |  |
| Automate infesté | Acte 2 | Non |  |
| Aventure du labyrinthe de la jungle | Acte 1 (A) | Non |  |
| Bains abyssaux | Acte 1 (B) | Partiel | Récit raccourci et choix réinventés pour la GBA. |
| Bienvenue chez Wongo’s | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Bosquet des Morphiques | Acte 1 (A) | Non |  |
| Bricolage | Acte 3 | Non |  |
| Ceci ou Cela ? | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Champ de trous à taille corporelle | Acte 2 | Non |  |
| Chœur lumineux | Acte 1 (A) | Non |  |
| Convoyeur sans fin | Acte 1 (B) | Non |  |
| Coursier de potions | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Développement personnel | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Esprit greffeur | Acte 2 | Non |  |
| Feu follet égaré | Acte 2 | Non |  |
| Fleur colossale | Acte 2 | Partiel | Récit raccourci et choix réinventés pour la GBA. |
| Graine de saphir | Acte 1 (A) | Non |  |
| Gravures sur bois | Acte 1 (A) | Non |  |
| Jungle dense | Acte 1 (A) | Non |  |
| La clé de la lanterne | Acte 2 | Non |  |
| La légende disait vrai | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| La statue engloutie | Acte 1 (A), Acte 1 (B) | Non |  |
| Le Marchand ??? | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Le procès | Acte 3 | Non |  |
| Le thé de la table ronde | Acte 3 | Non |  |
| L’avenir des potions ? | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Mannequin de combat usé | Acte 3 | Partiel | Récit raccourci et choix réinventés pour la GBA. |
| Maître de thé | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Nid d’oizodon | Acte 1 (A) | Non |  |
| Phare englouti | Acte 1 (B) | Non |  |
| Philosophes multicolores | Acte 2 | Non |  |
| Pierre de tous les temps | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Pont glissant | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Portes de l’ombre et de la lumière | Acte 1 (B) | Non |  |
| Ranwid l’Aîné | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Reflets stelfeR | Acte 3 | Non |  |
| Répi, historien de la guerre | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Salle aux poupées | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Salle du Fromage | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Sangsue cérébrale | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Scriptorium englouti | Acte 1 (B) | Non |  |
| Site sans repos | Acte 1 (A) | Non |  |
| Source mystique | Acte 1 (A) | Non |  |
| Sphère de cristal | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Stèle de la Vérité | Acte 1 (A) | Non |  |
| Symbiote | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Tas de déchets | Acte 1 (B) | Non |  |
| Tisseur zen | Acte 2 | Non |  |
| Tombe du guerrier oublié | Acte 3 | Non |  |
| Tourbillon aquatique | Acte 1 (B) | Non |  |
| Trafiquant de reliques | Acte 1 (A), Acte 1 (B), Acte 2, Acte 3 | Non |  |
| Trésor englouti | Acte 1 (B) | Non |  |
| Tueur d’insectes | Acte 2 | Non |  |
| Vallon des Murmures | Acte 1 (A) | Non |  |
| À coups de poing | Acte 1 (B) | Non |  |

### Anciens — 0,0 %

0 fait, 0 partiel, 8 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Neow | Acte 1 | Non |  |
| Darv | Tous actes | Non |  |
| Orobas | Acte 2 | Non |  |
| Paël | Acte 2 | Non |  |
| Tezcatara | Acte 2 | Non |  |
| Nonupeipée | Acte 3 | Non |  |
| Tanx | Acte 3 | Non |  |
| Vakuu | Acte 3 | Non |  |

<a id="regles"></a>

## Règles de combat — 42,3 %

### Déroulement du combat — 75,0 %

12 faits, 0 partiel, 4 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Énergie : 3 par tour, dépensée pour jouer les cartes |  | Oui |  |
| Pioche de 5 cartes, main limitée à 10, Défausse remélangée dans la Pioche |  | Oui |  |
| Armure remise à zéro au début du tour |  | Oui |  |
| Jusqu'à 3 ennemis par combat, avec ciblage |  | Oui |  |
| Intentions ennemies affichées |  | Oui |  |
| Calcul des dégâts (Force, Affaibli, Vulnérable) |  | Oui |  |
| Attaques de zone et attaques en plusieurs coups |  | Oui |  |
| Cartes qui font piocher |  | Oui |  |
| Amélioration de cartes pendant le combat |  | Oui |  |
| Cartes Pouvoir : restent en jeu jusqu'à la fin du combat |  | Oui |  |
| Défausse d'une carte au choix |  | Non | [#31](https://github.com/Boblebol/slay_the_gba/issues/31) |
| Choix d'une carte parmi plusieurs (Découverte, etc.) |  | Non | [#72](https://github.com/Boblebol/slay_the_gba/issues/72) |
| Cartes à coût X |  | Oui |  |
| Écho : rejouer une carte |  | Non | [#86](https://github.com/Boblebol/slay_the_gba/issues/86) |
| Coup de grâce : effet quand la carte tue un ennemi |  | Oui |  |
| Transformer une carte |  | Non | [#79](https://github.com/Boblebol/slay_the_gba/issues/79) |

### Pouvoirs et statuts — 37,5 %

6 faits, 0 partiel, 10 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Force |  | Oui |  |
| Dextérité |  | Oui |  |
| Vulnérable |  | Oui |  |
| Affaibli |  | Oui |  |
| Poison |  | Oui |  |
| Rituel (ennemis) |  | Oui |  |
| Artéfact |  | Non | [#92](https://github.com/Boblebol/slay_the_gba/issues/92) |
| Fragilisé |  | Non | [#93](https://github.com/Boblebol/slay_the_gba/issues/93) |
| Focalisation |  | Non | [#97](https://github.com/Boblebol/slay_the_gba/issues/97) |
| Intangible |  | Non | [#92](https://github.com/Boblebol/slay_the_gba/issues/92) |
| Blindage |  | Non | [#91](https://github.com/Boblebol/slay_the_gba/issues/91) |
| Épines |  | Non | [#91](https://github.com/Boblebol/slay_the_gba/issues/91) |
| Vigueur |  | Non | [#93](https://github.com/Boblebol/slay_the_gba/issues/93) |
| Fatalité |  | Non | [#105](https://github.com/Boblebol/slay_the_gba/issues/105) |
| Désorienté |  | Non | [#93](https://github.com/Boblebol/slay_the_gba/issues/93) |
| Sables mouvants (L'Insatiable) |  | Non | [#245](https://github.com/Boblebol/slay_the_gba/issues/245) |

### Mots-clés de cartes — 57,1 %

4 faits, 0 partiel, 3 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Épuisement |  | Oui |  |
| Éthéré |  | Oui |  |
| Inné |  | Oui |  |
| Retenue |  | Oui |  |
| Ruse |  | Non | [#73](https://github.com/Boblebol/slay_the_gba/issues/73) |
| Injouable |  | Non | [#95](https://github.com/Boblebol/slay_the_gba/issues/95) |
| Éternel |  | Non | [#95](https://github.com/Boblebol/slay_the_gba/issues/95) |

### Enchantements — 0,0 %

0 fait, 0 partiel, 13 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Adroit |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Agilité |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Braise de Tezcatara |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Célérité |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Clonage |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Élan |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Encrée |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Glamour |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Imprégnation |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Instinct |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Sceau royal |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Tranchant |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |
| Viscosité |  | Non | [#107](https://github.com/Boblebol/slay_the_gba/issues/107) |

<a id="systemes"></a>

## Systèmes de jeu — 55,6 %

### La Tour — 42,9 %

3 faits, 0 partiel, 4 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Carte procédurale à embranchements (16 étages) |  | Oui | [#24](https://github.com/Boblebol/slay_the_gba/issues/24) |
| Trois actes enchaînés, soin entre les actes |  | Oui |  |
| Salles : monstre, élite, boss, repos, trésor, boutique, évènement |  | Oui |  |
| Rencontres propres à chaque acte (81 rencontres du jeu d'origine) |  | Non | Trois groupes fixes de monstres, identiques dans les trois actes. [#225](https://github.com/Boblebol/slay_the_gba/issues/225) |
| Un boss différent par acte |  | Non | [#36](https://github.com/Boblebol/slay_the_gba/issues/36) |
| Neow en début de partie |  | Non | [#6](https://github.com/Boblebol/slay_the_gba/issues/6) |
| Bénédiction d'un Ancien entre les actes |  | Non | La phase existe dans le moteur mais elle est sautée. [#261](https://github.com/Boblebol/slay_the_gba/issues/261) |

### Récompenses — 42,9 %

3 faits, 0 partiel, 4 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Une carte parmi trois, tirées au hasard dans les cartes du héros |  | Oui | [#35](https://github.com/Boblebol/slay_the_gba/issues/35) |
| Or après chaque combat |  | Oui |  |
| Coffre : or et relique jamais en double |  | Oui |  |
| Potion en récompense de combat |  | Non | [#185](https://github.com/Boblebol/slay_the_gba/issues/185) |
| Relique après un élite |  | Non | [#184](https://github.com/Boblebol/slay_the_gba/issues/184) |
| Relique de boss |  | Non | [#184](https://github.com/Boblebol/slay_the_gba/issues/184) |
| Cartes rares et peu communes selon la salle |  | Non | [#67](https://github.com/Boblebol/slay_the_gba/issues/67) |

### Feu de camp — 66,7 %

2 faits, 0 partiel, 1 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Repos : récupère 30 % des PV |  | Oui |  |
| Forge : améliorer une carte, avec aperçu |  | Oui |  |
| Autres actions (Cuisiner, Clonage, éclosion de l'Œuf d'oizodon) |  | Non | [#271](https://github.com/Boblebol/slay_the_gba/issues/271) |

### Marchand — 50,0 %

2 faits, 2 partiels, 2 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Achat de cartes |  | Oui |  |
| Retrait d'une carte, à coût croissant |  | Oui |  |
| Achat de reliques |  | Partiel | Géré par le moteur, absent de la boutique sur GBA. [#184](https://github.com/Boblebol/slay_the_gba/issues/184) |
| Achat de potions |  | Partiel | Géré par le moteur, absent de la boutique sur GBA. [#185](https://github.com/Boblebol/slay_the_gba/issues/185) |
| Inventaire tiré au hasard à chaque visite |  | Non | Trois cartes fixes par héros. [#270](https://github.com/Boblebol/slay_the_gba/issues/270) |
| Prix selon la rareté, carte en solde |  | Non | [#270](https://github.com/Boblebol/slay_the_gba/issues/270) |

### Potions — 50,0 %

1 fait, 0 partiel, 1 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Ceinture de 3 potions |  | Oui |  |
| Boire une potion en combat sur GBA |  | Non | Le moteur sait appliquer quatre potions, sans interface sur GBA. [#30](https://github.com/Boblebol/slay_the_gba/issues/30) |

### Ascension — 71,4 %

5 faits, 0 partiel, 2 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| A1 : élites plus fréquents |  | Oui | [#29](https://github.com/Boblebol/slay_the_gba/issues/29) |
| A2 à A4 : dégâts accrus des monstres, élites et boss |  | Oui | [#29](https://github.com/Boblebol/slay_the_gba/issues/29) |
| A5 : soin réduit après un boss |  | Oui | [#29](https://github.com/Boblebol/slay_the_gba/issues/29) |
| A6 : départ avec 10 % de PV en moins |  | Oui | [#29](https://github.com/Boblebol/slay_the_gba/issues/29) |
| A7 à A9 : PV accrus des monstres, élites et boss |  | Oui | [#29](https://github.com/Boblebol/slay_the_gba/issues/29) |
| A10 : malédiction Fléau de l'ascendant dans le deck de départ |  | Non | [#40](https://github.com/Boblebol/slay_the_gba/issues/40) |
| Choix du niveau d'Ascension sur GBA |  | Non | Toutes les parties démarrent en Ascension 0. [#272](https://github.com/Boblebol/slay_the_gba/issues/272) |

### Sauvegarde — 75,0 %

3 faits, 0 partiel, 1 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Sauvegarde automatique de la partie sur la cartouche (SRAM) |  | Oui | [#13](https://github.com/Boblebol/slay_the_gba/issues/13) |
| Reprise avec « Continuer » à l'écran titre |  | Oui | [#13](https://github.com/Boblebol/slay_the_gba/issues/13) |
| Volumes de la musique et des effets conservés |  | Oui | [#13](https://github.com/Boblebol/slay_the_gba/issues/13) |
| Statistiques et Compendium (cartes, reliques, monstres rencontrés) |  | Non | Écrans prévus dans le moteur, sans interface sur GBA. [#273](https://github.com/Boblebol/slay_the_gba/issues/273) |

<a id="interface"></a>

## Interface, audio et cartouches — 58,1 %

### Écrans — 54,8 %

9 faits, 5 partiels, 7 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Écran titre |  | Oui |  |
| Choix de la teinte du héros (4 palettes) |  | Oui |  |
| Carte de la Tour avec pion animé |  | Oui |  |
| Combat façon Pokémon |  | Oui |  |
| Récompenses |  | Oui |  |
| Évènement avec texte machine à écrire |  | Oui |  |
| Boutique |  | Oui |  |
| Feu de camp |  | Oui |  |
| Registre du deck |  | Oui |  |
| Défaite (Game Over) |  | Partiel | [#11](https://github.com/Boblebol/slay_the_gba/issues/11) |
| Victoire |  | Partiel | [#12](https://github.com/Boblebol/slay_the_gba/issues/12) |
| Indicateurs de vie, d'armure et de progression |  | Partiel | [#20](https://github.com/Boblebol/slay_the_gba/issues/20) |
| Transitions entre les scènes |  | Partiel | [#18](https://github.com/Boblebol/slay_the_gba/issues/18) |
| Système de dialogue et d'évènements textuels |  | Partiel | [#19](https://github.com/Boblebol/slay_the_gba/issues/19) |
| Options (volume, commandes) |  | Non | [#21](https://github.com/Boblebol/slay_the_gba/issues/21) |
| Menu pause en combat et sur la carte |  | Non | [#10](https://github.com/Boblebol/slay_the_gba/issues/10) |
| Menu principal interactif |  | Non | [#9](https://github.com/Boblebol/slay_the_gba/issues/9) |
| Écran titre animé (nuages, tour, brume) |  | Non | [#7](https://github.com/Boblebol/slay_the_gba/issues/7) |
| Écran titre propre à chaque édition |  | Non | [#3](https://github.com/Boblebol/slay_the_gba/issues/3) |
| Crédits et cinématique d'introduction |  | Non | [#1](https://github.com/Boblebol/slay_the_gba/issues/1) |
| Crédits de fin |  | Non | [#22](https://github.com/Boblebol/slay_the_gba/issues/22) |

### Graphismes et audio — 68,8 %

5 faits, 1 partiel, 2 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Sprites des 5 héros et arène par édition |  | Oui |  |
| Illustrations des cartes jouables |  | Oui |  |
| Police avec accents français |  | Oui |  |
| Effets sonores |  | Oui | [#14](https://github.com/Boblebol/slay_the_gba/issues/14) |
| Sept musiques en boucle |  | Oui | [#15](https://github.com/Boblebol/slay_the_gba/issues/15) |
| Logo et identité graphique |  | Non | [#8](https://github.com/Boblebol/slay_the_gba/issues/8) |
| Poses des héros tirées des artworks sources (attaque, blessure) |  | Non | [#2](https://github.com/Boblebol/slay_the_gba/issues/2) |
| Animations des ennemis et des boss |  | Partiel | Flottement au repos et fondu de mort ; pas d'animation d'attaque. [#17](https://github.com/Boblebol/slay_the_gba/issues/17) |

### Cartouches — 50,0 %

1 fait, 0 partiel, 1 à faire.

| Élément | Détail | Fait | Note |
|---|---|---|---|
| Cinq ROMs, une par héros |  | Oui |  |
| ROM unique réunissant tout le contenu |  | Non | [#4](https://github.com/Boblebol/slay_the_gba/issues/4) |
