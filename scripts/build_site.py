# /// script
# requires-python = ">=3.11"
# dependencies = ["markdown>=3.6", "pillow>=10"]
# ///
"""Generate the static Slay the GBA website (site/) from the design bible and the game art.

Run from the repository root:  uv run scripts/build_site.py
Open site/index.html in a browser. Images are referenced from ../bible/images, so keep the
site/ folder next to bible/ (GitHub Pages serving the repository root works as-is).
"""

from __future__ import annotations

import html
import json
import os
import re
from dataclasses import dataclass
from pathlib import Path

import markdown
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
BIBLE = ROOT / "bible"
DATA = BIBLE / "data"
SITE = ROOT / "site"
ASSETS = SITE / "assets"
GRAPHICS = ROOT / "graphics"
# Public URL used for canonical links, Open Graph and the sitemap (override with SITE_URL=...).
SITE_URL = os.environ.get("SITE_URL", "https://slay-the-gba.alexandre-enouf.fr").rstrip("/")
ARTICLE_URL = "https://alexandre-enouf.fr/blog/slay-the-gba-vibe-coding-game-boy-advance/"
PAGES: list[str] = []  # indexable pages, collected for sitemap.xml

# ---------------------------------------------------------------------------------------------
# Data
# ---------------------------------------------------------------------------------------------


@dataclass(frozen=True)
class Hero:
    slug: str
    pool: str
    name: str
    title: str
    hp: int
    relic: str
    relic_image: str
    relic_text: str
    deck: str
    mechanic: str
    zone: str
    color: str
    accent: str
    rom: str


HEROES = [
    Hero("ironclad", "Ironclad", "Soldat de Fer", "Guerrier maudit de l'Enfer", 80, "Sang ardent", "burning_blood",
         "À la fin du combat, récupérez 6 PV.", "5× Frappe, 4× Défense, 1× Heurt",
         "Force brute, attaques de zone, armure et pactes de sang.", "Forges du Carnage", "#c82030", "#f05030",
         "slay_ironclad.gba"),
    Hero("silent", "Silent", "La Silencieuse", "Chasseresse mortelle des brumes", 70, "Anneau du serpent",
         "ring_of_the_snake", "Au début de chaque combat, piochez 2 cartes supplémentaires.",
         "5× Frappe, 5× Défense, 1× Neutralisation, 1× Survivant",
         "Poisons exponentiels, dagues, défausse et esquive.", "Marais du Venin", "#40a048", "#78d060",
         "slay_silent.gba"),
    Hero("defect", "Defect", "Le Défectueux", "Automate de combat éveillé", 75, "Noyau fissuré", "cracked_core",
         "Au début de chaque combat, canalisez 1 Orbe de foudre.", "4× Frappe, 4× Défense, 1× Éclair, 1× Activation double",
         "Orbes de foudre, glace, ténèbres et plasma ; focus et évocations.", "Complexe Astral", "#3870d8",
         "#58b0f0", "slay_defect.gba"),
    Hero("regent", "Regent", "Le Régent", "Souverain céleste déchu", 75, "Droit divin", "divine_right",
         "Relique de départ du Régent : ses Étoiles alimentent ses sentences.",
         "4× Frappe, 4× Défense, 1× Chute d'étoile, 1× Vénération",
         "Accumulation d'Étoiles souveraines pour des frappes cosmiques.", "Palais Souverain", "#e89828",
         "#f0c060", "slay_regent.gba"),
    Hero("necrobinder", "Necrobinder", "La Nécrolieuse", "Gardienne des morts et des os", 65,
         "Phylactère enchaîné", "bound_phylactery", "Relique de départ : invoque et lie Titos, son familier squelette.",
         "4× Frappe, 4× Défense, 1× Déchaînement, 1× Garde du corps",
         "Invocation de Titos, âmes, Fatalité et manipulation de la mort.", "Crypte d'Outre-Tombe", "#c860b8",
         "#e8b0ff", "slay_necrobinder.gba"),
]

TYPE_FR = {"Attack": "Attaque", "Skill": "Compétence", "Power": "Pouvoir", "Curse": "Malédiction", "Quest": "Quête"}
RARITY_FR = {"Basic": "Base", "Common": "Commune", "Uncommon": "Peu commune", "Rare": "Rare", "Ancient": "Ancienne",
             "Event": "Évènement", "Curse": "Malédiction", "Quest": "Quête", "Shared": "Partagée",
             "Fallback": "Secours", "Starter": "Départ", "Boss": "Boss", "Shop": "Boutique"}
POOL_FR = {"Colorless": "Incolores", "Event": "Évènement", "Curse": "Malédictions", "Quest": "Quêtes"}
ENCOUNTER_FR = {"normal": "Monstre", "elite": "Élite", "boss": "Boss", "weak": "Faible", "summon": "Invocation"}
INTENT_FR = {"attack": "Attaque", "defend": "Défense", "buff": "Renforcement", "debuff": "Affaiblissement",
             "status": "Statut", "heal": "Soin", "summon": "Invocation", "escape": "Fuite", "sleep": "Sommeil",
             "stun": "Étourdi", "unknown": "Inconnu", "hidden": "Caché", "card_debuff": "Malus de carte"}


def load(name: str, key: str) -> list[dict]:
    return json.loads((DATA / f"{name}_v0.111.0.json").read_text())[key]


def esc(text: object) -> str:
    return html.escape(str(text), quote=True)


def rich(text: str) -> str:
    """Turn the dump's inline markup (<g>keyword</g>, [E], newlines) into safe HTML."""
    text = esc(text or "")
    text = re.sub(r"&lt;g&gt;(.*?)&lt;/g&gt;", r'<b class="kw">\1</b>', text)
    text = re.sub(r"&lt;s&gt;(.*?)&lt;/s&gt;", r'\1 <span class="star">★</span>', text)
    text = re.sub(r"&lt;e&gt;(.*?)&lt;/e&gt;", r'\1 <span class="nrg">●</span>', text)
    text = text.replace("&lt;e/&gt;", '<span class="nrg">●</span>').replace("[E]", '<span class="nrg">●</span>')
    text = re.sub(r"&lt;/?\w+/?&gt;", "", text)
    return text.replace("\n", "<br>")


ROMS = SITE / "roms"


def copy_roms() -> None:
    """Copy the built cartridges next to the pages so download links work locally and once deployed."""
    ROMS.mkdir(exist_ok=True)
    for hero in HEROES:
        src = ROOT / hero.rom
        if src.exists():
            (ROMS / hero.rom).write_bytes(src.read_bytes())


def rom_size(hero: "Hero") -> str:
    path = ROOT / hero.rom
    return f"{path.stat().st_size / 1024:.0f} Ko" if path.exists() else "à compiler"


def download_button(hero: "Hero", depth: int, cls: str = "btn") -> str:
    return (f'<a class="{cls}" href="{"../" * depth}roms/{hero.rom}" download>'
            f'▼ Télécharger {esc(hero.rom)} <small>({rom_size(hero)})</small></a>')


def img_path(image_path: str, depth: int) -> str:
    return "../" * (depth + 1) + "bible" + image_path


# ---------------------------------------------------------------------------------------------
# Layout
# ---------------------------------------------------------------------------------------------

NAV = [
    ("index.html", "Accueil"),
    ("heros.html", "Héros"),
    ("cartes.html", "Cartes"),
    ("bestiaire.html", "Bestiaire"),
    ("reliques.html", "Reliques"),
    ("potions.html", "Potions"),
    ("evenements.html", "Évènements"),
    ("anciens.html", "Anciens"),
    ("guide.html", "Guide"),
    ("bible/index.html", "Bible"),
]


def page(title: str, body: str, *, depth: int = 0, active: str = "", theme: str = "", description: str = "",
         path: str = "", noindex: bool = False, absolute: bool = False) -> str:
    """Wrap a page body in the site shell. `path` is the page's URL path, used for SEO tags and the sitemap."""
    up = "/" if absolute else "../" * depth
    desc = description or "Slay the GBA : Slay the Spire 2 réinventé sur Game Boy Advance, une cartouche par héros."
    canonical = f"{SITE_URL}/{path}" if path else SITE_URL + "/"
    if path and not noindex:
        PAGES.append(path)
    seo = f"""<link rel="canonical" href="{esc(canonical)}">
<meta name="theme-color" content="#0d0b16">
<meta name="robots" content="{'noindex, follow' if noindex else 'index, follow'}">
<meta property="og:type" content="website">
<meta property="og:site_name" content="Slay the GBA">
<meta property="og:locale" content="fr_FR">
<meta property="og:title" content="{esc(title)} — Slay the GBA">
<meta property="og:description" content="{esc(desc)}">
<meta property="og:url" content="{esc(canonical)}">
<meta property="og:image" content="{SITE_URL}/assets/og.png">
<meta property="og:image:width" content="1200">
<meta property="og:image:height" content="630">
<meta property="og:image:alt" content="Slay the GBA — écran titre sur Game Boy Advance">
<meta name="twitter:card" content="summary_large_image">
<meta name="twitter:title" content="{esc(title)} — Slay the GBA">
<meta name="twitter:description" content="{esc(desc)}">
<meta name="twitter:image" content="{SITE_URL}/assets/og.png">
<link rel="apple-touch-icon" href="{up}assets/sprites/icon.png">"""
    links = "".join(
        f'<a href="{up}{href}"{" class=\"on\"" if href == active else ""}>{esc(label)}</a>' for href, label in NAV
    )
    return f"""<!doctype html>
<html lang="fr">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(title)} — Slay the GBA</title>
<meta name="description" content="{esc(desc)}">
{seo}
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Pixelify+Sans:wght@400;700&family=Silkscreen&display=swap" rel="stylesheet">
<link rel="stylesheet" href="{up}assets/style.css">
<link rel="icon" href="{up}assets/sprites/icon.png">
</head>
<body class="{theme}">
<header class="top">
  <a class="brand" href="{up}index.html"><img src="{up}assets/sprites/icon.png" alt="">SLAY THE GBA</a>
  <button class="burger" aria-label="Menu" onclick="document.body.classList.toggle('menu')">☰</button>
  <nav>{links}</nav>
</header>
<main>
{body}
</main>
<footer class="foot">
  <p>Slay the GBA — fan-game non officiel. <i>Slay the Spire</i> © Mega Crit. Moteur Butano © Gustavo Valiente.</p>
  <p>Données : bible du projet (patch v0.111.0 Beta). <a href="{up}mentions.html">Mentions légales &amp; confidentialité</a> · <a href="{ARTICLE_URL}" target="_blank" rel="noopener">Comment ce jeu a été vibe codé</a></p>
  <div class="ae-footer">
    <a class="ae-logo" href="https://alexandre-enouf.fr/" target="_blank" rel="noopener">Alexandre <em>Enouf</em></a>
    <span class="ae-tag">Développeur produit &amp; Builder IA · Paris, France</span>
    <nav class="ae-links" aria-label="Liens de l'auteur">
      <a href="https://alexandre-enouf.fr/#lab" target="_blank" rel="noopener">⚡ Lab &amp; Projets</a>
      <a href="https://alexandre-enouf.fr/blog/" target="_blank" rel="noopener">📖 Blog</a>
      <a href="https://github.com/Boblebol" target="_blank" rel="noopener">GitHub</a>
      <a href="https://www.linkedin.com/in/alexandreenouf-47834990" target="_blank" rel="noopener">LinkedIn</a>
      <a href="mailto:alexandre.enouf@gmail.com">Contact</a>
    </nav>
  </div>
</footer>
<script src="{up}assets/site.js"></script>
</body>
</html>
"""


def panel(inner: str, cls: str = "") -> str:
    return f'<section class="panel {cls}">{inner}</section>'


def card_html(card: dict, depth: int) -> str:
    kind = card["type"]
    cost = card.get("energy")
    cost_txt = "X" if cost in (-1, None) else str(cost)
    desc = rich(card.get("descBase") or card.get("descriptionClean", ""))
    up = rich(card.get("descUpgraded") or "")
    keywords = " ".join(card.get("keywords") or [])
    return f"""<article class="card t-{kind.lower()} p-{card['pool'].lower()}" data-type="{esc(kind)}" data-rarity="{esc(card['rarity'])}" data-name="{esc(card['name'].lower())} {esc(card['id'].lower())}">
  <div class="art"><img loading="lazy" src="{img_path(card['imagePath'], depth)}" alt="{esc(card['name'])}"><span class="gem">{esc(cost_txt)}</span></div>
  <h4>{esc(card['name'])}</h4>
  <p class="meta">{esc(TYPE_FR.get(kind, kind))} · {esc(RARITY_FR.get(card['rarity'], card['rarity']))}{' · ' + esc(keywords) if keywords else ''}</p>
  <p class="desc">{desc}</p>
  {f'<details><summary>Amélioré +</summary><p class="desc up">{up}</p></details>' if up and up != desc else ''}
</article>"""


def card_filters() -> str:
    types = "".join(f'<button data-filter-type="{t}">{TYPE_FR[t]}</button>' for t in ["Attack", "Skill", "Power"])
    rarities = "".join(
        f'<button data-filter-rarity="{r}">{RARITY_FR[r]}</button>' for r in ["Basic", "Common", "Uncommon", "Rare", "Ancient"]
    )
    return f"""<div class="filters" data-target=".cards">
  <input type="search" placeholder="Rechercher une carte…" data-search>
  <div class="chips"><button class="on" data-filter-type="">Tous types</button>{types}</div>
  <div class="chips"><button class="on" data-filter-rarity="">Toutes raretés</button>{rarities}</div>
  <p class="count"></p>
</div>"""


# ---------------------------------------------------------------------------------------------
# Assets
# ---------------------------------------------------------------------------------------------


def export_sprite(name: str, out_name: str, frame_h: int | None = None, frames: tuple[int, ...] = (0,),
                  grid: tuple[int, int] | None = None, scale: int = 1) -> None:
    """Convert a Butano BMP (index 0 = transparent) into a PNG usable on the web."""
    img = Image.open(GRAPHICS / f"{name}.bmp")
    rgba = img.convert("RGBA")
    px = img.load()
    data = rgba.load()
    for y in range(img.height):
        for x in range(img.width):
            if px[x, y] == 0:
                data[x, y] = (0, 0, 0, 0)
    if frame_h is None:
        out = rgba
    elif grid:
        cols, rows = grid
        w = rgba.width
        out = Image.new("RGBA", (w * cols, frame_h * rows))
        for i, f in enumerate(frames):
            tile = rgba.crop((0, f * frame_h, w, (f + 1) * frame_h))
            out.paste(tile, ((i % cols) * w, (i // cols) * frame_h))
    else:
        f = frames[0]
        out = rgba.crop((0, f * frame_h, rgba.width, (f + 1) * frame_h))
    if scale > 1:
        out = out.resize((out.width * scale, out.height * scale), Image.NEAREST)
    out.save(ASSETS / "sprites" / f"{out_name}.png")


def export_bg(name: str, out_name: str) -> None:
    Image.open(GRAPHICS / f"{name}.bmp").convert("RGB").crop((0, 0, 240, 160)).save(ASSETS / "bg" / f"{out_name}.png")


def build_assets() -> None:
    (ASSETS / "sprites").mkdir(parents=True, exist_ok=True)
    (ASSETS / "bg").mkdir(parents=True, exist_ok=True)
    for hero in HEROES:
        export_sprite(f"hero_{hero.slug}", f"hero_{hero.slug}")
        export_sprite(f"ui_energy_{hero.slug}", f"energy_{hero.slug}")
        export_sprite(f"card_strike_{hero.slug}", f"card_strike_{hero.slug}")
        export_bg(f"bg_battle_{hero.slug}", f"battle_{hero.slug}")
    for mon in ["cultist", "jaw_worm", "louse", "gremlin_nob"]:
        export_sprite(f"mon_{mon}", f"mon_{mon}")
    export_sprite("boss_ceremonial_beast", "boss_ceremonial_beast", 64, (0, 1, 2, 3), (2, 2))
    export_sprite("map_boss_ceremonial_beast", "icon")
    for bg in ["title", "map", "campfire", "shop", "rewards", "victory", "gameover", "event_battleworn_dummy",
               "event_colossal_flower", "event_abyssal_baths", "event_mystic_altar"]:
        export_bg(f"bg_{bg}", bg)


# ---------------------------------------------------------------------------------------------
# Pages
# ---------------------------------------------------------------------------------------------

SCREENS = [
    ("title", "Écran titre sur la key-art de Slay the Spire 2"),
    ("palette", "Choix de la teinte du héros (Classique, Chromatique, Ombre, Or)"),
    ("map", "Carte de la Tour : 16 étages, embranchements et pion animé"),
    ("combat", "Arène de combat façon Pokémon avec main de cartes illustrées"),
    ("card_flight", "La carte jouée grossit au centre avant l'impact"),
    ("impact", "Slash, dégâts flottants et barre de PV qui se vide"),
    ("enemy_turn", "Tour ennemi rejoué attaque par attaque"),
    ("target", "Ciblage quand plusieurs ennemis sont en vie"),
    ("death", "Mort des monstres en fondu-mosaïque"),
    ("boss", "Boss : le Cerf Auguste"),
    ("rewards", "Récompenses : choisissez une carte parmi trois"),
    ("event", "Évènements mystères avec texte machine à écrire"),
    ("event_choice", "Choix d'évènement et conséquences"),
    ("shop", "Boutique du marchand"),
    ("forge", "Feu de camp : forge avec aperçu amélioré"),
    ("deck", "Registre du deck"),
]


def build_index(cards: list[dict], monsters: list[dict], relics: list[dict], potions: list[dict], events: list[dict]) -> None:
    carts = "".join(
        f"""<a class="cart" href="heros/{h.slug}.html" style="--hc:{h.color};--ha:{h.accent}">
  <div class="cart-label" style="background-image:url(assets/bg/battle_{h.slug}.png)">
    <img class="pix" src="assets/sprites/hero_{h.slug}.png" alt="{esc(h.name)}">
  </div>
  <div class="cart-info"><b>{esc(h.name)}</b><span>{esc(h.zone)}</span><small>{h.hp} PV · {esc(h.rom)}</small></div>
</a>"""
        for h in HEROES
    )
    downloads = "".join(
        f'<div class="download" style="--hc:{h.color}"><img class="pix" src="assets/sprites/hero_{h.slug}.png" alt=""><div><b>{esc(h.name)}</b><span class="meta">{esc(h.zone)}</span>{download_button(h, 0, "btn small")}</div></div>'
        for h in HEROES
    )
    shots = "".join(
        f'<figure class="shot"><img class="pix" loading="lazy" src="assets/screens/{n}.png" alt="{esc(c)}"><figcaption>{esc(c)}</figcaption></figure>'
        for n, c in SCREENS
    )
    stats = [(len(cards), "cartes"), (len([m for m in monsters if m.get('acts')]), "monstres"), (len(relics), "reliques"),
             (len(potions), "potions"), (len(events), "évènements"), (8, "Anciens")]
    stat_html = "".join(f"<div><b>{n}</b><span>{esc(l)}</span></div>" for n, l in stats)
    bestiary = "".join(
        f'<div class="mini-mon"><img class="pix" src="assets/sprites/{s}.png" alt="{esc(n)}"><span>{esc(n)}</span></div>'
        for s, n in [("mon_cultist", "Cultist"), ("mon_jaw_worm", "Jaw Worm"), ("mon_louse", "Morpion"),
                     ("mon_gremlin_nob", "Gremlin Nob")]
    )
    body = f"""
<section class="hero-banner" style="background-image:url(assets/bg/title.png)">
  <div class="hero-inner">
    <p class="kicker">Game Boy Advance · 5 cartouches</p>
    <h1>SLAY THE GBA</h1>
    <p class="lead">Slay the Spire 2 réinventé en 240×160 : deckbuilding roguelike, arène de combat façon Pokémon GBA et une cartouche par héros.</p>
    <div class="cta"><a class="btn" href="#jouer">Comment jouer ►</a><a class="btn ghost" href="#cartouches">Choisir un héros</a><a class="btn ghost" href="#telecharger">▼ Télécharger les ROMs</a><a class="btn ghost" href="bible/index.html">Lire la bible</a><a class="btn ghost" href="{ARTICLE_URL}" target="_blank" rel="noopener">📖 L'histoire du projet</a></div>
  </div>
</section>

<div class="stats">{stat_html}</div>

{panel('<h2 id="cartouches">Les 5 cartouches</h2><p>Un héros par cartouche, chacun avec son deck, sa relique, son décor et sa mécanique signature.</p><div class="carts">' + carts + '</div>')}

{panel('''<h2>Le jeu</h2>
<div class="features">
  <div><h3>♥ Roguelike deckbuilder</h3><p>Gravissez une Tour de 16 étages : combats, élites, évènements, feux de camp, boutiques, trésors, puis le boss de l'acte.</p></div>
  <div><h3>⚔ Combats à la Pokémon</h3><p>Héros de dos à gauche, monstres à droite sur leurs socles, intentions visibles, barres de PV qui se vident, dégâts flottants et secousses.</p></div>
  <div><h3>★ Vraies illustrations</h3><p>Cartes, monstres, boss et évènements convertis depuis l'art de Slay the Spire 2 en palettes 16 couleurs.</p></div>
  <div><h3>● Fluide</h3><p>Moteur C++20 Butano, texte mis en cache, animations interpolées, fondus entre scènes : 60 images par seconde.</p></div>
</div>''')}

<section class="panel"><h2>Captures</h2><p>Images réelles de la ROM, capturées dans l'émulateur.</p><div class="shots">{shots}</div></section>

{panel('<h2>Bestiaire de l’acte 1</h2><div class="mini-mons">' + bestiary + '<div class="mini-mon boss"><img class="pix" src="assets/sprites/boss_ceremonial_beast.png" alt="Cerf Auguste"><span>Cerf Auguste (Boss)</span></div></div><p><a class="btn small" href="bestiaire.html">Tout le bestiaire ►</a></p>')}

{panel('''<h2>Commandes</h2>
<table class="keys"><tr><th>Écran</th><th>Bouton</th><th>Action</th></tr>
<tr><td>Carte</td><td>◄ ►</td><td>Choisir la salle suivante</td></tr>
<tr><td>Carte</td><td>▲ ▼</td><td>Faire défiler la Tour</td></tr>
<tr><td>Carte</td><td>A</td><td>Avancer vers la salle</td></tr>
<tr><td>Combat</td><td>◄ ►</td><td>Sélectionner une carte</td></tr>
<tr><td>Combat</td><td>A</td><td>Jouer la carte (puis choisir la cible)</td></tr>
<tr><td>Combat</td><td>START / R</td><td>Finir le tour</td></tr>
<tr><td>Carte / Combat</td><td>SELECT</td><td>Registre du deck (B pour revenir)</td></tr></table>''')}

{panel('<h2 id="telecharger">Télécharger les cartouches</h2><p>Une ROM <code>.gba</code> par héros, à ouvrir dans un émulateur (mGBA, Delta sur iPhone, RetroArch…) ou à flasher sur une cartouche EZ-Flash / EverDrive.</p><div class="downloads">' + downloads + '</div>')}

{panel('''<h2 id="jouer">Jouer</h2>
<ol class="steps">
<li><b>Téléchargez la cartouche</b> de votre héros (fichier <code>.gba</code>) dans la <a href="#telecharger">section Télécharger</a> ou sur la page du héros.</li>
<li><b>Installez un émulateur Game Boy Advance</b> gratuit :
  <ul>
    <li>Ordinateur (Windows, macOS, Linux) : <a href="https://mgba.io/downloads.html" target="_blank" rel="noopener">mGBA</a> (recommandé).</li>
    <li>iPhone / iPad : <a href="https://apps.apple.com/app/delta-game-emulator/id1048524688" target="_blank" rel="noopener">Delta</a>.</li>
    <li>Android : <a href="https://play.google.com/store/apps/details?id=com.fastemulator.gba" target="_blank" rel="noopener">My Boy!</a> ou <a href="https://www.retroarch.com/" target="_blank" rel="noopener">RetroArch</a>.</li>
  </ul></li>
<li><b>Ouvrez le fichier <code>.gba</code></b> dans l'émulateur (Fichier → Charger une ROM, ou importez-le dans l'app), puis appuyez sur START.</li>
<li>Vous avez une vraie console ? Copiez la ROM sur une cartouche flash (EZ-Flash, EverDrive) et jouez sur GBA, GBA SP ou DS.</li>
</ol>
<p class="meta">Les ROMs sont des créations homebrew du projet : aucune ROM commerciale n'est distribuée.</p>''')}
"""
    (SITE / "index.html").write_text(page("Accueil", body, active="index.html", path=""))


def build_heroes_index() -> None:
    rows = "".join(
        f"""<a class="hero-row" href="heros/{h.slug}.html" style="--hc:{h.color};--ha:{h.accent}">
  <img class="pix big" src="assets/sprites/hero_{h.slug}.png" alt="">
  <div><h3>{esc(h.name)}</h3><p class="meta">{esc(h.title)}</p><p>{esc(h.mechanic)}</p>
  <p class="meta">{h.hp} PV · Relique : {esc(h.relic)} · Deck : {esc(h.deck)}</p></div>
</a>"""
        for h in HEROES
    )
    body = panel("<h1>Les héros</h1><p>Cinq archétypes, cinq cartouches.</p>" + rows)
    (SITE / "heros.html").write_text(page("Héros", body, active="heros.html", path="heros.html",
                                          description="Les 5 héros de Slay the GBA et leurs cartouches."))


def md_to_html(text: str, depth: int, image_base: str) -> str:
    """Render a bible tome, rewriting its relative image and tome links."""
    html_text = markdown.markdown(text, extensions=["tables", "fenced_code", "toc", "sane_lists"])
    up = "../" * depth
    html_text = re.sub(r'src="images/', f'loading="lazy" src="{up}../bible/{image_base}images/', html_text)
    html_text = re.sub(r'src="\.\./images/', f'loading="lazy" src="{up}../bible/images/', html_text)
    html_text = re.sub(r'href="(?:livrets_gba/)?([\w.-]+)\.md(#[^"]*)?"', r'href="\1.html\2"', html_text)
    html_text = html_text.replace('href="images/', f'href="{up}../bible/images/')
    # Absolute file:// links and source links written for the repository layout.
    html_text = re.sub(r'href="file:///[^"]*?/bible/(?:livrets_gba/)?([\w.-]+)\.md"', r'href="\1.html"', html_text)
    html_text = re.sub(r'href="file:///[^"]*?/slay_the_gba/', f'href="{up}../', html_text)
    # Source files are not part of the website: show those links as plain code references.
    html_text = re.sub(r'<a href="(?:\.\./)+(?:include|src|tests|scripts)/[^"]*">(.*?)</a>', r'<code>\1</code>', html_text)
    return html_text


def build_hero_pages(cards: list[dict], relics: list[dict]) -> None:
    (SITE / "heros").mkdir(exist_ok=True)
    for h in HEROES:
        pool = [c for c in cards if c["pool"] == h.pool]
        order = {"Basic": 0, "Common": 1, "Uncommon": 2, "Rare": 3, "Ancient": 4}
        pool.sort(key=lambda c: (order.get(c["rarity"], 9), c["name"]))
        hero_relics = [r for r in relics if r["pool"] == h.pool]
        relic_html = "".join(
            f'<div class="relic"><img loading="lazy" src="{img_path(r["imagePath"], 1)}" alt=""><div><b>{esc(r["name"])}</b><p>{rich(r.get("descI18n") or r.get("descriptionClean", ""))}</p></div></div>'
            for r in hero_relics
        )
        livret = (BIBLE / "livrets_gba" / f"livret_{h.slug}.md").read_text()
        livret_html = md_to_html(livret, 1, "livrets_gba/../")
        counts = {t: sum(1 for c in pool if c["type"] == t) for t in ["Attack", "Skill", "Power"]}
        body = f"""
<section class="hero-banner small" style="background-image:url(../assets/bg/battle_{h.slug}.png)">
  <div class="hero-inner split">
    <img class="pix giant" src="../assets/sprites/hero_{h.slug}.png" alt="{esc(h.name)}">
    <div><p class="kicker">Cartouche {esc(h.rom)}</p><h1>{esc(h.name)}</h1><p class="lead">{esc(h.title)}</p>
    <p>{esc(h.mechanic)}</p>{download_button(h, 1)}</div>
  </div>
</section>
<div class="stats"><div><b>{h.hp}</b><span>PV</span></div><div><b>3</b><span>énergie</span></div><div><b>{len(pool)}</b><span>cartes</span></div>
<div><b>{counts['Attack']}</b><span>attaques</span></div><div><b>{counts['Skill']}</b><span>compétences</span></div><div><b>{counts['Power']}</b><span>pouvoirs</span></div></div>
{panel(f'''<h2>Fiche</h2><div class="sheet">
<div class="relic big"><img src="../../bible/images/relics/{h.relic_image}.webp" alt=""><div><b>Relique de départ : {esc(h.relic)}</b><p>{esc(h.relic_text)}</p></div></div>
<div><h3>Deck de départ</h3><p>{esc(h.deck)}</p><h3>Zone de la cartouche</h3><p>{esc(h.zone)}</p></div>
<img class="pix energy" src="../assets/sprites/energy_{h.slug}.png" alt="Orbe d'énergie">
</div>''')}
<section class="panel"><h2>Toutes les cartes ({len(pool)})</h2>{card_filters()}<div class="cards">{''.join(card_html(c, 1) for c in pool)}</div></section>
{panel('<h2>Reliques exclusives</h2><div class="relics">' + relic_html + '</div>')}
{panel('<div class="md">' + livret_html + '</div>', 'livret')}
"""
        (SITE / "heros" / f"{h.slug}.html").write_text(
            page(h.name, body, depth=1, active="heros.html", theme=f"hero-{h.slug}",
                 description=f"{h.name}, {h.title} : {h.mechanic} Toutes ses cartes et la ROM {h.rom}.",
                 path=f"heros/{h.slug}.html"))


def build_cards_page(cards: list[dict]) -> None:
    sections = ""
    for pool, label in [(h.pool, h.name) for h in HEROES] + [(p, POOL_FR[p]) for p in ["Colorless", "Event", "Curse", "Quest"]]:
        subset = sorted([c for c in cards if c["pool"] == pool], key=lambda c: c["name"])
        sections += f'<h2 class="pool-title" id="{pool.lower()}">{esc(label)} <small>({len(subset)})</small></h2><div class="cards">' + "".join(card_html(c, 0) for c in subset) + "</div>"
    jump = " ".join(f'<a href="#{p.lower()}">{esc(l)}</a>' for p, l in [(h.pool, h.name) for h in HEROES] + [(p, POOL_FR[p]) for p in ["Colorless", "Event", "Curse", "Quest"]])
    filters = card_filters().replace('data-target=".cards"', 'data-target=".cards"')
    body = panel(f"<h1>Toutes les cartes</h1><p class='jump'>{jump}</p>{filters}{sections}")
    (SITE / "cartes.html").write_text(page("Cartes", body, active="cartes.html", description=f"Les {len(cards)} cartes de Slay the Spire 2 : filtres par héros, type et rareté.", path="cartes.html"))


def build_bestiary(monsters: dict) -> None:
    effect_names = monsters.get("effectNames", {})
    groups = {"boss": [], "elite": [], "normal": [], "weak": [], "summon": []}
    for m in monsters["monsters"]:
        if not m.get("acts"):
            continue
        groups.setdefault(m["encounterType"], []).append(m)
    out = ""
    for key in ["boss", "elite", "normal", "weak", "summon"]:
        items = sorted(groups.get(key, []), key=lambda m: (m["acts"][0], m.get("minHp", 0)))
        if not items:
            continue
        cards_html = ""
        for m in items:
            moves = ""
            for mv in m.get("moves", []):
                bits = []
                if mv.get("damage"):
                    bits.append(f'{mv["damage"]}{"×" + str(mv["hits"]) if mv.get("hits", 1) > 1 else ""} dégâts')
                if mv.get("block"):
                    bits.append(f'{mv["block"]} armure')
                for eff in mv.get("effects", []) or []:
                    name = effect_names.get(eff.get("type", ""), eff.get("type", ""))
                    amt = eff.get("amount")
                    bits.append(f'{esc(name)}{" " + str(amt) if amt else ""}')
                intents = ", ".join(INTENT_FR.get(i, i) for i in mv.get("intents", []))
                moves += f'<li><b>{esc(mv.get("name", mv.get("id")))}</b> <span class="meta">{esc(intents)}</span>{" — " + ", ".join(bits) if bits else ""}</li>'
            hp = f'{m.get("minHp")}–{m.get("maxHp")}' if m.get("minHp") != m.get("maxHp") else str(m.get("minHp"))
            acts = ", ".join(a.replace("act", "Acte ") for a in m["acts"])
            cards_html += f"""<article class="monster e-{key}"><div class="mon-art"><img loading="lazy" src="../bible{m['image']}" alt="{esc(m['name'])}"></div>
<div><h3>{esc(m['name'])}</h3><p class="meta">{esc(ENCOUNTER_FR.get(key, key))} · {esc(acts)} · ♥ {esc(hp)} PV</p><ul class="moves">{moves}</ul></div></article>"""
        out += f'<h2 id="{key}">{esc(ENCOUNTER_FR[key])}s <small>({len(items)})</small></h2><div class="monsters">{cards_html}</div>'
    jump = " ".join(f'<a href="#{k}">{ENCOUNTER_FR[k]}s</a>' for k in ["boss", "elite", "normal", "weak", "summon"])
    body = panel(f"<h1>Bestiaire</h1><p class='jump'>{jump}</p><input type='search' class='solo-search' placeholder='Rechercher un monstre…' data-solo='.monster'>{out}")
    (SITE / "bestiaire.html").write_text(page("Bestiaire", body, active="bestiaire.html", path="bestiaire.html",
                                                  description="Bestiaire de Slay the Spire 2 : monstres, élites et boss avec PV et attaques."))


def build_list_page(filename: str, title: str, items: list[dict], group_key: str, labels: dict, text_key: str,
                    active: str) -> None:
    groups: dict[str, list[dict]] = {}
    for it in items:
        groups.setdefault(it.get(group_key, "Autre"), []).append(it)
    out = ""
    for key in sorted(groups, key=lambda k: list(labels).index(k) if k in labels else 99):
        rows = "".join(
            f'<div class="relic" data-name="{esc(it["name"].lower())}"><img loading="lazy" src="{img_path(it["imagePath"], 0)}" alt=""><div><b>{esc(it["name"])}</b><p>{rich(it.get(text_key) or it.get("descriptionClean", ""))}</p></div></div>'
            for it in sorted(groups[key], key=lambda x: x["name"])
        )
        out += f'<h2>{esc(labels.get(key, key))} <small>({len(groups[key])})</small></h2><div class="relics">{rows}</div>'
    body = panel(f"<h1>{esc(title)}</h1><input type='search' class='solo-search' placeholder='Rechercher…' data-solo='.relic'>{out}")
    (SITE / filename).write_text(page(title, body, active=active, path=filename,
                                      description=f"{title} de Slay the Spire 2 : {len(items)} entrées avec illustrations et effets."))


def build_events(events: list[dict]) -> None:
    out = "".join(
        f"""<article class="event">{f'<img loading="lazy" src="{img_path(e["imagePath"], 0)}" alt="">' if e.get('imagePath') else '<div class="noimg">?</div>'}<div><h3>{esc(e['name'])}</h3>
<p class="meta">{esc(', '.join('Acte ' + str(a) for a in e.get('acts') or []) or 'Tous actes')} · {e.get('optionCount', '?')} choix</p>
<details><summary>Lire le récit</summary><p>{rich(e.get('descriptionClean', ''))}</p></details></div></article>"""
        for e in sorted(events, key=lambda e: e["name"])
    )
    body = panel(f"<h1>Évènements mystères</h1><input type='search' class='solo-search' placeholder='Rechercher un évènement…' data-solo='.event'><div class='events'>{out}</div>")
    (SITE / "evenements.html").write_text(page("Évènements", body, active="evenements.html", path="evenements.html",
                                                    description="Tous les évènements mystères de Slay the Spire 2, avec leurs récits."))


def build_ancients(ancients: list[dict]) -> None:
    out = ""
    for a in ancients:
        slots = ""
        for s in a.get("slots", []):
            opts = "".join(
                f'<div class="relic"><img loading="lazy" src="{img_path(o["imagePath"], 0)}" alt=""><div><b>{esc(o["name"])}</b> <span class="tier">{esc(o.get("tier", ""))}</span><p>{rich(o.get("description", ""))}</p></div></div>'
                for o in s.get("options", [])
            )
            slots += f'<h4>{esc(s.get("group", "").capitalize())} — {s.get("picks", 1)} choix</h4><div class="relics">{opts}</div>'
        acts = "Tous actes" if a.get("anyAct") else ", ".join(f"Acte {x}" for x in a.get("acts", []))
        out += f"""<article class="ancient" style="--ac:{esc(a.get('color', '#333'))}">
<div class="ancient-art" style="background-image:url(../bible{a['artPath']})"><img src="../bible{a['iconPath']}" alt=""><div><h2>{esc(a['name'])}</h2><p>{esc(a.get('epithet', ''))} · {esc(acts)}</p></div></div>
<details><summary>Bénédictions et reliques</summary>{slots}</details><p><a href="bible/{ancient_tome(a['slug'])}">Tome détaillé ►</a></p></article>"""
    body = panel(f"<h1>Les Anciens</h1><p>Divinités rencontrées entre les actes, qui façonnent votre run par des reliques uniques.</p>{out}")
    (SITE / "anciens.html").write_text(page("Anciens", body, active="anciens.html", path="anciens.html",
                                                 description="Les 8 Anciens de Slay the Spire 2, leurs bénédictions et reliques."))


def ancient_tome(slug: str) -> str:
    for f in sorted(BIBLE.glob("0*_*.md")):
        if slug in f.stem:
            return f.stem + ".html"
    return "index.html"


def build_guide() -> None:
    parts = []
    for name in ["19_boucle_de_gameplay_et_ascension.md", "21_interface_pokemon_et_navigation_carte.md"]:
        parts.append('<div class="md">' + md_to_html((BIBLE / name).read_text(), 0, "").replace("../bible/", "bible_raw/") + "</div>")
    body = "".join(panel(p) for p in parts)
    body = body.replace('src="bible_raw/', 'src="../bible/').replace('href="bible_raw/', 'href="../bible/')
    body = re.sub(r'href="([\w.-]+)\.html', r'href="bible/\1.html', body)
    (SITE / "guide.html").write_text(page("Guide", body, active="guide.html", path="guide.html",
                                               description="Boucle de gameplay, ascension et interface de Slay the GBA."))


def build_bible() -> None:
    out_dir = SITE / "bible"
    out_dir.mkdir(exist_ok=True)
    tomes = sorted(BIBLE.glob("[0-9]*.md")) + [BIBLE / "spec_technique_gba.md"]
    livrets = sorted((BIBLE / "livrets_gba").glob("*.md"))
    toc = []
    for path in tomes + livrets:
        text = path.read_text()
        m = re.search(r"^#\s+(.+)$", text, re.M)
        title = re.sub(r"[^\w\sÀ-ÿ'’:—\-&()!,.]", "", m.group(1)).strip() if m else path.stem
        stem = "livrets" if path.name == "README.md" and path.parent.name == "livrets_gba" else path.stem
        base = "livrets_gba/" if path.parent.name == "livrets_gba" else ""
        html_body = md_to_html(text, 1, base)
        html_body = html_body.replace('href="README.html', 'href="livrets.html')
        (out_dir / f"{stem}.html").write_text(page(title, panel(f'<p class="crumb"><a href="index.html">◄ Bible</a></p><div class="md">{html_body}</div>'),
                                                   depth=1, active="bible/index.html", path=f"bible/{stem}.html",
                                                   description=f"Bible de Slay the GBA : {title}."))
        toc.append((stem, title, base))
    readme = md_to_html((BIBLE / "README.md").read_text(), 1, "")
    readme = readme.replace('href="README.html', 'href="index.html')
    listing = "".join(f'<li><a href="{s}.html">{esc(t)}</a>{" <span class=meta>livret</span>" if b else ""}</li>' for s, t, b in toc)
    body = panel(f"<h1>La bible complète</h1><p>Tous les tomes de conception du projet, rendus depuis <code>bible/</code>.</p><ol class='toc'>{listing}</ol>") + panel(f'<div class="md">{readme}</div>')
    (out_dir / "index.html").write_text(page("Bible", body, depth=1, active="bible/index.html", path="bible/index.html",
                                             description="La bible de conception complète de Slay the GBA."))


def build_static_pages() -> None:
    """404 (served by Netlify for unknown URLs, so it uses absolute paths) and the legal notice."""
    body = f"""<section class="hero-banner small" style="background-image:url(/assets/bg/gameover.png)">
  <div class="hero-inner"><p class="kicker">Erreur 404</p><h1>DÉFAITE…</h1>
  <p class="lead">Cette salle n'existe pas dans la Tour. Votre ascension s'arrête ici… à moins de rebrousser chemin.</p>
  <div class="cta"><a class="btn" href="/index.html">◄ Retour à l'accueil</a><a class="btn ghost" href="/heros.html">Choisir un héros</a></div></div>
</section>"""
    (SITE / "404.html").write_text(page("Page introuvable", body, noindex=True, absolute=True, path="404.html"))

    body = panel("""<h1>Mentions légales &amp; confidentialité</h1>
<h2>Éditeur</h2>
<p>Site personnel et non commercial édité par Alexandre Enouf, Paris, France — <a href="mailto:alexandre.enouf@gmail.com">alexandre.enouf@gmail.com</a>.</p>
<h2>Hébergement</h2>
<p>Netlify, Inc., 512 2nd Street, Suite 200, San Francisco, CA 94107, États-Unis — <a href="https://www.netlify.com" rel="noopener">netlify.com</a>.</p>
<h2>Propriété intellectuelle</h2>
<p>Slay the GBA est un fan-game non officiel, sans lien avec Mega Crit. <i>Slay the Spire</i>, ses personnages, cartes, illustrations et textes sont la propriété de Mega Crit Games. Les données et illustrations proviennent de la bible du projet (patch v0.111.0 Beta). Le moteur Butano est distribué sous licence zlib par Gustavo Valiente. Pour toute demande de retrait, écrivez à l'adresse ci-dessus.</p>
<h2>Données personnelles &amp; cookies</h2>
<p>Ce site ne dépose aucun cookie, n'utilise aucun outil de mesure d'audience et ne collecte aucune donnée personnelle. Les polices sont chargées depuis Google Fonts, qui peut recevoir votre adresse IP lors du chargement. L'hébergeur conserve des journaux techniques de connexion pour la sécurité du service.</p>
<h2>ROMs</h2>
<p>Les fichiers <code>.gba</code> proposés sont des créations homebrew compilées depuis le code du projet ; aucune ROM commerciale n'est distribuée.</p>""")
    (SITE / "mentions.html").write_text(page("Mentions légales", body, path="mentions.html",
                                             description="Mentions légales et politique de confidentialité de Slay the GBA."))


def build_seo_files() -> None:
    """robots.txt, sitemap.xml and the 1200x630 social preview image."""
    (SITE / "robots.txt").write_text(f"User-agent: *\nAllow: /\nDisallow: /roms/\n\nSitemap: {SITE_URL}/sitemap.xml\n")
    urls = "".join(
        f"  <url><loc>{esc(SITE_URL + '/' + p)}</loc><priority>{'1.0' if p == '' else '0.8' if p.startswith('heros') else '0.6'}</priority></url>\n"
        for p in dict.fromkeys(PAGES)
    )
    (SITE / "sitemap.xml").write_text(
        f'<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n{urls}</urlset>\n')
    # Real title screen capture (logo included), pixel-doubled to the social card size.
    title = Image.open(ASSETS / "screens" / "title.png").convert("RGB")
    og = title.resize((1200, 800), Image.NEAREST).crop((0, 0, 1200, 630))
    og.save(ASSETS / "og.png", optimize=True)


def main() -> None:
    SITE.mkdir(exist_ok=True)
    cards = load("card", "cards")
    relics = load("relic", "relics")
    potions = load("potion", "potions")
    events = load("event", "events")
    ancients = load("ancient", "ancients")
    monsters = json.loads((DATA / "monster_v0.111.0.json").read_text())
    build_assets()
    copy_roms()
    build_index(cards, monsters["monsters"], relics, potions, events)
    build_heroes_index()
    build_hero_pages(cards, relics)
    build_cards_page(cards)
    build_bestiary(monsters)
    relic_labels = {"Shared": "Partagées", **{h.pool: h.name for h in HEROES}, "Event": "Évènement", "Fallback": "Secours"}
    build_list_page("reliques.html", "Reliques", relics, "pool", relic_labels, "descI18n", "reliques.html")
    potion_labels = {"Common": "Communes", "Uncommon": "Peu communes", "Rare": "Rares", "Event": "Évènement", "Token": "Spéciales"}
    build_list_page("potions.html", "Potions", potions, "rarity", potion_labels, "descI18n", "potions.html")
    build_events(events)
    build_ancients(ancients)
    build_guide()
    build_bible()
    build_static_pages()
    build_seo_files()
    print("Site generated in", SITE, "for", SITE_URL)


if __name__ == "__main__":
    main()
