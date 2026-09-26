# Tome 21 — Interface Style Pokémon, Navigation de Carte & Inspecteur de Deck

## 1. Vision & Architecture UI/UX

Ce tome spécifie l'expérience joueur immersive combinant deux philosophies emblématiques :
1. **La navigation et progression de *Slay the Spire*** : exploration procédurale de 16 étages, vision des embranchements futurs, gestion et consultation permanente du Deck principal via raccourci manette.
2. **L'ergonomie des combats et événements inspirée des jeux *Pokémon GBA (Génération 3)*** : disposition en diagonale de l'arène (héros vu de dos/trois-quarts en bas à gauche, monstre en haut à droite), boîtes de statut à jauges dynamiques, menu d'action 4 directions, et fenêtres de dialogue pour les événements mystères.

---

## 2. Navigation de Carte Classique (*Slay the Spire*)

### Disposition Écran & Défilement D-Pad
Sur l'écran GBA (240x160 pixels), la carte de l'Acte est visualisée verticalement :
- **Défilement vertical (Haut / Bas)** : Permet d'inspecter les étages 1 à 16 (sommet de la Tour avec le Boss) pour planifier son cheminement (repérer les Feux de camp, Boutiques et Élites).
- **Sélection des embranchements (Gauche / Droite)** : Met en surbrillance l'une des voies connectées accessibles depuis la salle actuelle (`MapGraph::get_next_nodes`).
- **Confirmation (Bouton A)** : Valide le déplacement et fait entrer le joueur dans la salle choisie.
- **Consultation rapide du Deck (Bouton SELECT)** : Bascule instantanément sur l'Inspecteur de Deck.

```
┌────────────────────────────────────────────────────────┐
│  🗺️  CARTE DE LA TOUR — ACTE 1   PV:80/80   OR:99 💰   │
├────────────────────────────────────────────────────────┤
│  Étage 16 :               👑 [BOSS GUARDIAN]           │
│  Étage 15 :     [Feu]            [Feu]           [Feu] │
│  Étage 14 :    [Combat]        [Élite]         [Shop]  │
│  ...                                                   │
│  Étage  2 :   👉[Combat]        [Event ?]       [Shop] │
│  Étage  1 :    *[Combat]                               │
├────────────────────────────────────────────────────────┤
│  > Voie Gauche (Étage 2) : Combat de Monstre [Combat]  │
│  Haut/Bas: Défiler   G/D: Choisir Route   A: Entrer    │
│  SELECT: Registre du Deck                              │
└────────────────────────────────────────────────────────┘
```

### Typologie des Salles et Icônes GBA
| Type de Salle | Icône GBA | Libellé Écran | Comportement à l'entrée |
| :--- | :---: | :--- | :--- |
| `MONSTER` | `[Combat]` | Combat Standard | Lance le combat contre un monstre de l'acte |
| `ELITE` | `[Élite]` | Combat d'Élite | Combat difficile, récompense relique majeure |
| `REST_SITE` | `[Feu]` | Feu de Camp | Choix : Se reposer (+30% PV) ou Forger (+1 carte) |
| `SHOP` | `[Shop]` | Marchand de la Tour | Achats de cartes, reliques, potions et purge de carte |
| `TREASURE` | `[Coffre]` | Salle au Trésor | Coffre scellé (+50 Or et 1 relique gratuite) |
| `EVENT` | `[Event ?]` | Événement Mystère | Rencontre narrative avec choix style Pokémon |
| `BOSS` | `[BOSS]` | Gardien de l'Étage | Combat de boss final de l'acte |

---

## 3. Registre & Inspecteur de Deck (`DeckViewer`)

Accessible à tout moment en navigation (et via le menu en combat) par la touche **SELECT** ou l'option dédiée :
- **En-tête** : Identité du Héros, Teinte/Couleur choisie (*Flamme Azur*, etc.), PV actuels/Max, Or possédé, Étage courant.
- **Barre de Reliques** : Liste de toutes les reliques passives actives (*Sang Ardent*, *Vajra*, *Akabeko*...).
- **Inventaire du Deck** : Défilement carte par carte avec affichage clair du statut d'amélioration (`+`), coût en Énergie, Type (Attaque, Compétence, Pouvoir) et description complète des effets.
- **Touche B ou SELECT** : Retour immédiat à l'écran de navigation sans transition superflue.

---

## 4. Arène de Combat Style Pokémon (GBA Gen 3)

### Disposition Spatiale (240x160)
- **Socles de combat (Piédestaux)** :
  - Joueur : Positionné en bas à gauche `(x = -65, y = 14)`.
  - Ennemi : Positionné en haut à droite `(x = 65, y = -34)`.
- **Boîte de Statut Ennemi (Haut-Gauche)** :
  - Nom du monstre et Étage.
  - Jauge de PV proportionnelle `[████████░░]` avec décompte numérique `34/44`.
  - Bouclier d'Armure dynamique (`+6B`).
  - Bulle d'Intention (*Attaque 11 dégâts*, *Buff Force +3*).
  - Badges d'états et altérations (`[STR+3]`, `[VULN:1]`, `[POISON:7]`).
- **Boîte de Statut Héros (Bas-Droite / Centre-Droit)** :
  - Nom du Héros et Teinte active (`[Shiny]`).
  - Jauge de PV proportionnelle `[██████████]` avec décompte `80/80`.
  - Bouclier d'Armure (`+5B`).
  - Compteur d'Énergie `[●][●][●] (3/3)`.
  - Jauge de ressource spécifique selon l'édition de cartouche :
    - *Soldat de Fer* : `FORCE: +2 | MÉTALLISATION: 3`
    - *La Silencieuse* : `POISON ENNEMI: 7`
    - *Le Défectueux* : `ORBES: [⚡ Foudre] [❄️ Glace] [🌑 Dark]` (3 emplacements)
    - *Le Régent* : `ÉTOILES SOUVERAINES: ⭐⭐⭐ (3/5)`
    - *La Nécrolieuse* : `ÂMES: 4 | OSTY LE MINION: 12 PV [Intercepte]`

```
┌────────────────────────────────────────────────────────┐
│ [ENNEMI] Cultist (Étage 1)                             │
│ PV: [==========] 44/44                                 │
│ Intent: Attaque 6 dégâts                               │
│                                                        │
│                           [HÉROS] Soldat de Fer [Shiny]│
│                           PV: [==========] 80/80  [+5B]│
│                           ÉNERGIE: [●][●][●] (3/3)     │
│                           RESSOURCE: STR: +0 | METAL: 0│
├────────────────────────────────────────────────────────┤
│ Que doit faire Soldat de Fer ?                         │
│  > [1] CARTES (Attaques)         [2] SAC (Potions : 1) │
│    [3] MON DECK (Select)         [4] FIN DE TOUR       │
└────────────────────────────────────────────────────────┘
```

### Menu d'Action 4 Directions (Type Pokémon)
Le pavé de commande inférieur adopte la structure 2x2 des jeux Pokémon :
1. **`[1] CARTES`** : Ouvre le sous-menu des cartes en main.
   - Présentation façon "Capacités Pokémon" : Nom de la carte, coût en Énergie (remplaçant les PP), type et description complète de l'effet.
   - Touche A pour déclencher la carte.
   - Touche B pour annuler et revenir au menu principal.
2. **`[2] SAC`** : Affiche les potions portées à la ceinture (jusqu'à 3 fioles) pour utilisation instantanée.
3. **`[3] MON DECK`** : Ouvre l'overlay de consultation des cartes et reliques sans passer son tour.
4. **`[4] FIN DE TOUR`** : Conclut le tour du joueur, applique les statuts de fin de tour et déclenche l'animation d'offensive de l'ennemi.

### Sensations & Animations de Combat
- **Respiration au repos (Idle Bobbing)** : Flottement subtil de 1 pixel alterné toutes les 24 frames.
- **Fente offensive (Player Lunge)** : Déplacement rapide vers l'ennemi lors du jeu d'une carte d'attaque.
- **Secousse de dégâts (Enemy Shake & Flicker)** : Trépidation latérale de l'ennemi et clignotement lors de la réception d'un coup.
- **Indicateur de dégâts flottant** : Affichage temporaire du montant de dégâts infligés au-dessus de la cible (`-9 HP !`).

---

## 5. Événements Mystères Style Pokémon

Les salles `[Event ?]` abandonnent les pavés textuels rigides pour adopter la scénarisation des rencontres de PNJ / Pokémon légendaires :
- **Cadre de Dialogue immersif** : Texte narratif avec défilement fluide et typographie rétro.
- **Boîte de Choix interactive** : 2 à 3 alternatives explicites avec indication des bénéfices et sacrifices.
- **Résolution avec feedback sonore et visuel** : Message d'issue confirmant l'octroi d'Or, la guérison, l'amélioration d'une carte ou l'obtention d'une relique.

```
┌────────────────────────────────────────────────────────┐
│  ❓ MANNEQUIN D'ENTRAÎNEMENT USÉ                       │
├────────────────────────────────────────────────────────┤
│  « BZZZT ! SÉQUENCE D'ENTRAÎNEMENT INITIÉE !           │
│    Réglez vos coups ou subissez l'humiliation ! »      │
├────────────────────────────────────────────────────────┤
│  > [1] Attaque acharnée (Subissez 6 dégâts, +60 Or)    │
│    [2] Réparer les rouages (Améliorez 1 carte du deck) │
│    [3] Prendre la fuite (Continuer la route)           │
└────────────────────────────────────────────────────────┘
```

---

## 6. Automatisation des Builds & Nettoyage (`make clean`)

Afin d'éviter tout conflit de cache et d'éliminer les versions obsolètes de l'application :

| Commande Makefile | Action effectuée |
| :--- | :--- |
| `make clean` | Supprime les ROMs obsolètes (ex: `slay_the_gba.gba`), les `.elf`, `.map`, fichiers temporaires, répertoires `bin/` et `build_*`. **Préserve les 5 ROMs officielles**. |
| `make clean-all` | Nettoyage complet incluant la suppression de **toutes** les cartouches `.gba` pour repartir d'un dépôt totalement vierge. |
| `make all-roms` | Compile proprement et simultanément les 5 éditions de cartouches (`slay_ironclad.gba`, `slay_silent.gba`, `slay_defect.gba`, `slay_regent.gba`, `slay_necrobinder.gba`). |
| `make test` | Exécute la suite de tests unitaires C++20 déterministes (100% PASS). |
| `make test-e2e` | Exécute la simulation complète de bout en bout de la boucle de jeu (100% PASS). |
| `make play` | Lance le simulateur interactif complet dans le terminal. |
