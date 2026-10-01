"""The five Slay the GBA cartridges ("éditions"): the single source of truth shared by the website
(build_site.py) and the instruction booklets (build_livrets.py).

Numbers that also live in the game (HP, gold, palettes, ROM names) mirror src/engine/character.cpp and
the Makefile; card and relic texts are read from bible/data at build time.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class Palette:
    name: str
    description: str
    bgr555: int  # primary colour, as stored in src/engine/character.cpp

    @property
    def hex(self) -> str:
        r, g, b = (self.bgr555 & 31) << 3, ((self.bgr555 >> 5) & 31) << 3, ((self.bgr555 >> 10) & 31) << 3
        return f"#{r:02x}{g:02x}{b:02x}"


@dataclass(frozen=True)
class Hero:
    slug: str
    pool: str  # pool name in bible/data
    name: str  # French name, as shown in game
    english: str
    title: str  # epithet
    edition: str
    ref: str  # ROM header title (Makefile ROMTITLE)
    color_name: str  # cartridge colour
    color: str
    accent: str
    zone: str
    hp: int
    relic_id: str
    starter: tuple[tuple[str, int], ...]  # (card id, copies), card names come from the data
    mechanic: str  # one-line summary
    mechanic_name: str
    mechanic_text: str  # how the signature mechanic works
    tagline: str
    pitch: str
    lore: str
    ideal: str  # "for players who like…"
    difficulty: int  # 1 (accessible) to 3 (expert)
    key_cards: tuple[str, ...]
    tips: tuple[str, ...]
    palettes: tuple[Palette, ...]
    tome: str  # bible card tome

    @property
    def rom(self) -> str:
        return f"slay_{self.slug}.gba"

    @property
    def screenshot(self) -> str:
        return "combat" if self.slug == "ironclad" else f"combat_{self.slug}"


GOLD = 99
ENERGY = 3
POTION_SLOTS = 3
DIFFICULTY = {1: "Accessible", 2: "Intermédiaire", 3: "Expert"}

# French labels for the values found in bible/data.
TYPE_FR = {"Attack": "Attaque", "Skill": "Compétence", "Power": "Pouvoir", "Curse": "Malédiction", "Quest": "Quête",
           "Status": "Statut"}
RARITY_FR = {"Basic": "Base", "Common": "Commune", "Uncommon": "Peu commune", "Rare": "Rare", "Ancient": "Ancienne",
             "Event": "Évènement", "Curse": "Malédiction", "Quest": "Quête", "Shared": "Partagée",
             "Fallback": "Secours", "Starter": "Départ", "Boss": "Boss", "Shop": "Boutique", "Token": "Créée en combat",
             "Status": "Statut"}
ENCOUNTER_FR = {"normal": "Monstre", "elite": "Élite", "boss": "Boss", "weak": "Faible", "summon": "Invocation"}

HEROES = [
    Hero(
        slug="ironclad", pool="Ironclad", name="Soldat de Fer", english="The Ironclad",
        title="Guerrier maudit de l'Enfer", edition="Édition Soldat de Fer", ref="SLAYIRON",
        color_name="Rouge Sang", color="#c82030", accent="#f05030", zone="Forges du Carnage", hp=80,
        relic_id="BURNING_BLOOD", starter=(("STRIKE_IRONCLAD", 5), ("DEFEND_IRONCLAD", 4), ("BASH", 1)),
        mechanic="Force brute, attaques de zone, armure et pactes de sang.",
        mechanic_name="Force & sacrifices",
        mechanic_text="La Force augmente les dégâts de chaque attaque, pour tout le combat. Le Soldat de Fer "
                      "l'accumule (Enflammer, Forme démoniaque), sacrifie ses PV contre de la puissance et "
                      "Épuise ses cartes faibles pour affûter son deck. Le Sang ardent referme ses plaies "
                      "après chaque victoire.",
        tagline="La force brute, forgée dans le sang.",
        pitch="Le héros idéal pour découvrir la Tour : beaucoup de PV, une relique qui soigne après chaque combat "
              "et des attaques qui cognent fort. Empilez la Force, encaissez sous l'Armure, puis écrasez tout.",
        lore="Dernier survivant d'un ordre guerrier décimé, le Soldat de Fer a scellé un pacte avec des puissances "
             "démoniaques pour venger ses frères d'armes. Armé d'une lame colossale forgée dans le sang et le fer "
             "noirci, il gravit la Tour pour atteindre son cœur.",
        ideal="les joueurs qui aiment frapper fort, encaisser et pardonner leurs erreurs grâce au soin.",
        difficulty=1,
        key_cards=("BASH", "INFLAME", "DEMON_FORM", "BARRICADE"),
        tips=(
            "Ouvrez avec **Heurt** : Vulnérable augmente de 50 % les dégâts reçus par l'ennemi, enchaînez ensuite vos Frappes.",
            "Le **Sang ardent** rend 6 PV après chaque combat : un peu de dégâts pour finir un combat un tour plus tôt est souvent rentable.",
            "Au feu de camp, améliorez d'abord **Heurt** et vos cartes de Force : elles profitent à toutes vos attaques.",
            "Contre les groupes, privilégiez les attaques qui touchent TOUS les ennemis (Percée, Coup de tonnerre).",
        ),
        palettes=(
            Palette("Rouge Carmin (Classique)", "L'armure rouge sang traditionnelle du Soldat de Fer.", 0x001F),
            Palette("Flamme Azur (Chromatique)", "Une teinte cyan étincelante née du feu mystique.", 0x7C00),
            Palette("Obsidienne (Ombre)", "Une armure noir charbon forgée dans les abysses.", 0x1084),
            Palette("Soleil Radieux (Or)", "Une parure dorée impériale ornée d'éclats solaires.", 0x03FF),
        ),
        tome="09_cartes_ironclad",
    ),
    Hero(
        slug="silent", pool="Silent", name="La Silencieuse", english="The Silent",
        title="Chasseresse mortelle des brumes", edition="Édition La Silencieuse", ref="SLAYSILENT",
        color_name="Vert Venin", color="#40a048", accent="#78d060", zone="Marais du Venin", hp=70,
        relic_id="RING_OF_THE_SNAKE",
        starter=(("STRIKE_SILENT", 5), ("DEFEND_SILENT", 5), ("NEUTRALIZE", 1), ("SURVIVOR", 1)),
        mechanic="Poisons exponentiels, surins, défausse et esquive.",
        mechanic_name="Poison & surins",
        mechanic_text="Le Poison inflige ses dégâts au début du tour de l'ennemi puis diminue de 1 : il traverse "
                      "l'Armure et s'accumule. Les Surins sont des attaques à 0 énergie créées par ses cartes, "
                      "parfaites avec Précision. Pioche et défausse font tourner son deck plus vite que tout autre.",
        tagline="Frappez sans bruit. Laissez le venin finir le travail.",
        pitch="Moins de PV, mais une main plus grande dès le premier tour grâce à l'Anneau du serpent. "
              "Empoisonnez, esquivez, noyez l'ennemi sous les surins : la Silencieuse gagne à l'usure.",
        lore="Chasseresse redoutée venue des terres de brume, la Silencieuse dissimule son visage sous un crâne de "
             "fauve cornu. Là où les guerriers misent sur la force, elle compte sur l'agilité, les lames cachées "
             "sous sa cape et des toxines qui consument ses proies de l'intérieur.",
        ideal="les joueurs patients qui aiment les combos, la pioche et les dégâts sur la durée.",
        difficulty=2,
        key_cards=("NEUTRALIZE", "NOXIOUS_FUMES", "BLADE_DANCE", "ADRENALINE"),
        tips=(
            "**Neutralisation** au premier tour : Affaibli réduit de 25 % les dégâts de l'ennemi.",
            "Le Poison ignore l'Armure : c'est votre meilleure réponse aux ennemis qui se barricadent.",
            "Avec l'**Anneau du serpent**, vous commencez chaque combat avec 7 cartes : cherchez vos pièces maîtresses dès le tour 1.",
            "Précision + Danse des lames transforme chaque Surin en vraie attaque.",
        ),
        palettes=(
            Palette("Vert Émeraude (Classique)", "La cape d'assassin verte de la tribu du serpent.", 0x03E0),
            Palette("Améthyste (Chromatique)", "Une tenue pourpre venimeuse des ombres sylvestres.", 0x7810),
            Palette("Spectre Nocturne (Ombre)", "Un costume d'assassin invisible dans l'obscurité.", 0x14A5),
            Palette("Venin Doré (Or)", "La parure d'apparat des maîtres de la guilde silencieuse.", 0x03E8),
        ),
        tome="10_cartes_silent",
    ),
    Hero(
        slug="defect", pool="Defect", name="Le Défectueux", english="The Defect",
        title="Automate de combat éveillé", edition="Édition Le Défectueux", ref="SLAYDEFECT",
        color_name="Bleu Saphir", color="#3870d8", accent="#58b0f0", zone="Complexe Astral", hp=75,
        relic_id="CRACKED_CORE", starter=(("STRIKE_DEFECT", 4), ("DEFEND_DEFECT", 4), ("ZAP", 1), ("DUALCAST", 1)),
        mechanic="Orbes de foudre, glace, ténèbres et plasma ; focus et évocations.",
        mechanic_name="Orbes & Focalisation",
        mechanic_text="Le Défectueux Canalise des Orbes dans ses emplacements. Chaque orbe agit seul à la fin du "
                      "tour (kéraunique : dégâts, glaciaire : Armure, ténébreux : charge, plasma : énergie) et "
                      "libère un effet plus fort quand il est Activé ou expulsé par un nouvel orbe. La "
                      "Focalisation renforce tous les orbes à la fois.",
        tagline="Un automate, trois emplacements, une infinité de circuits.",
        pitch="Le héros des ingénieurs : chaque orbe canalisé travaille pour vous, tour après tour. "
              "Montez votre Focalisation, alignez foudre et glace, et regardez la machine s'emballer.",
        lore="Automate conçu pour purger la Tour, le Défectueux a développé une conscience propre après une "
             "défaillance de ses sous-systèmes de ciblage. Alimenté par un noyau fissuré, il fait graviter autour "
             "de son châssis des orbes de foudre, de givre, de ténèbres et de plasma.",
        ideal="les joueurs qui aiment planifier, optimiser et voir un moteur tourner tout seul.",
        difficulty=2,
        key_cards=("ZAP", "BALL_LIGHTNING", "GLACIER", "ECHO_FORM"),
        tips=(
            "Le **Noyau fissuré** canalise un orbe kéraunique dès le début : jouez **Activation double** tôt pour un gros burst.",
            "Les orbes glaciaires génèrent de l'Armure gratuite chaque tour : deux ou trois suffisent à stabiliser un combat.",
            "La Focalisation (Défragmentation, Biais cognitif) est la statistique qui fait passer le deck de bon à injouable pour l'ennemi.",
            "Condensateur ajoute des emplacements : plus d'orbes, plus d'effets passifs.",
        ),
        palettes=(
            Palette("Bleu Électrique (Classique)", "Le châssis automatique en cobalt forgé.", 0x7FE0),
            Palette("Néon Cyberpunk (Chromatique)", "Une finition orange vif surchargée d'énergie.", 0x021F),
            Palette("Néant Sidéral (Ombre)", "Un alliage noir mat dénué de tout reflet lumineux.", 0x1842),
            Palette("Cuivre Antique (Or)", "Une structure en laiton doré restaurée d'un âge perdu.", 0x02DF),
        ),
        tome="11_cartes_defect",
    ),
    Hero(
        slug="regent", pool="Regent", name="Le Régent", english="The Regent",
        title="Souverain céleste déchu", edition="Édition Le Régent", ref="SLAYREGENT",
        color_name="Or Solaire", color="#e89828", accent="#f0c060", zone="Palais Souverain", hp=75,
        relic_id="DIVINE_RIGHT",
        starter=(("STRIKE_REGENT", 4), ("DEFEND_REGENT", 4), ("FALLING_STAR", 1), ("VENERATE", 1)),
        mechanic="Accumulation d'Étoiles souveraines et forge de la Lame souveraine.",
        mechanic_name="Étoiles & Lame souveraine",
        mechanic_text="Le Régent amasse des Étoiles (★), une seconde ressource qui s'ajoute à l'énergie et "
                      "alimente ses cartes les plus puissantes. Il Forge aussi la Lame souveraine : chaque carte "
                      "« Forgez » la rend plus tranchante, jusqu'à la frappe royale qui renverse le combat.",
        tagline="Une couronne, des étoiles, et la Tour à genoux.",
        pitch="Nouveau venu de Slay the Spire 2, le Régent joue avec deux ressources : l'énergie et les Étoiles. "
              "Le Droit divin lui en offre dès le début du combat. Préparez vos décrets, forgez votre lame, frappez en roi.",
        lore="Souverain d'une dynastie stellaire oubliée, le Régent a vu son royaume céleste s'effondrer sous "
             "l'emprise de la Tour. Vêtu de pourpre et d'or, il manie un sceptre capable de canaliser les "
             "constellations et entend bien reprendre son trône.",
        ideal="les joueurs qui aiment gérer une ressource de plus et préparer un tour décisif.",
        difficulty=3,
        key_cards=("FALLING_STAR", "VENERATE", "CONQUEROR", "COMET"),
        tips=(
            "Le **Droit divin** donne 3 ★ au début de chaque combat : dépensez-les tôt, ne les laissez pas dormir.",
            "**Chute d'étoile** coûte 0 et applique Affaibli et Vulnérable : la meilleure ouverture du Régent.",
            "Les cartes « Forgez » s'additionnent : **Conquérant** double ensuite les dégâts de la Lame souveraine.",
            "Gardez un œil sur l'énergie ET les Étoiles : un tour sans l'une ou l'autre est un tour perdu.",
        ),
        palettes=(
            Palette("Pourpre Royal (Classique)", "La pourpre souveraine réservée à la lignée des rois.", 0x510D),
            Palette("Rubis Écarlate (Chromatique)", "Une robe cramoisie éclatante symbole de bravoure.", 0x001B),
            Palette("Éclipse Astrale (Ombre)", "Une cape d'ombre tissée de poussière d'étoiles éteintes.", 0x0C43),
            Palette("Couronne Céleste (Or)", "Une splendeur dorée rayonnant de dignité céleste.", 0x03FF),
        ),
        tome="12_cartes_regent",
    ),
    Hero(
        slug="necrobinder", pool="Necrobinder", name="La Nécrolieuse", english="The Necrobinder",
        title="Gardienne des morts et des os", edition="Édition La Nécrolieuse", ref="SLAYNECRO",
        color_name="Violet Spectral", color="#c860b8", accent="#e8b0ff", zone="Crypte d'Outre-Tombe", hp=65,
        relic_id="BOUND_PHYLACTERY",
        starter=(("STRIKE_NECROBINDER", 4), ("DEFEND_NECROBINDER", 4), ("UNLEASH", 1), ("BODYGUARD", 1)),
        mechanic="Invocation de Titos, Fatalité, Âmes et manipulation de la mort.",
        mechanic_name="Titos & Fatalité",
        mechanic_text="La Nécrolieuse combat avec Titos, son familier squelette : « Invoquez » lui donne des PV, "
                      "il encaisse les coups à sa place et frappe avec Déchaînement. La Fatalité marque les "
                      "ennemis pour une mort certaine, et les Âmes ajoutées à la pioche nourrissent ses sortilèges.",
        tagline="Elle ne combat jamais seule.",
        pitch="Les PV les plus bas de la Tour, mais un garde du corps squelette qui grandit à chaque tour grâce au "
              "Phylactère enchaîné. Protégez Titos, marquez vos proies de Fatalité, et laissez la mort faire son œuvre.",
        lore="Liée à jamais aux âmes errantes de la Tour, la Nécrolieuse chemine escortée de Titos, un familier "
             "squelettique qu'elle relève encore et encore. Fragile de corps, elle compense par une présence "
             "dédoublée sur le champ de bataille.",
        ideal="les joueurs qui aiment les invocations, les effets à retardement et les parties à haut risque.",
        difficulty=3,
        key_cards=("BODYGUARD", "UNLEASH", "BONE_SHARDS", "DEATHBRINGER"),
        tips=(
            "Le **Phylactère enchaîné** invoque au début de chaque tour : plus le combat dure, plus Titos est solide.",
            "**Déchaînement** ajoute les PV actuels de Titos à ses dégâts : faites-le grossir avant de le lâcher.",
            "Avec 65 PV, chaque point compte : laissez Titos encaisser les grosses attaques d'élites.",
            "La Fatalité (Porte-mort, Décompte) achève les ennemis dont les PV passent sous son seuil.",
        ),
        palettes=(
            Palette("Teinte Spectrale (Classique)", "La bure sombre bordée d'un halo spectral vert d'eau.", 0x4104),
            Palette("Âme Pourpre (Chromatique)", "Une aura violacée incandescente de magie interdite.", 0x001E),
            Palette("Abysse Mortuaire (Ombre)", "Le noir absolu des tombeaux millénaires de la Tour.", 0x0821),
            Palette("Pharaonique (Or)", "Les bandelettes tressées de fils d'or pur des défunts rois.", 0x03B9),
        ),
        tome="13_cartes_necrobinder",
    ),
]

# Controls, identical on every cartridge (README.md).
CONTROLS = [
    ("Carte", "◄ ►", "Choisir la salle suivante"),
    ("Carte", "▲ ▼", "Faire défiler la Tour"),
    ("Carte", "A", "Avancer vers la salle"),
    ("Combat", "◄ ►", "Sélectionner une carte"),
    ("Combat", "A", "Jouer la carte (puis choisir la cible si plusieurs ennemis)"),
    ("Combat", "B", "Annuler le ciblage"),
    ("Combat", "START / R", "Finir le tour"),
    ("Carte / Combat", "SELECT", "Registre du deck (B pour revenir)"),
    ("Menus", "A / B", "Valider / annuler"),
]
