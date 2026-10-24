# /// script
# requires-python = ">=3.12"
# dependencies = ["markdown>=3.6", "pillow>=10", "numpy>=1.26"]
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
from pathlib import Path

import markdown
from PIL import Image

import progress
from build_identity import icon_image
from editions import (CONTROLS, DIFFICULTY, ENCOUNTER_FR, ENERGY, GOLD, HEROES, POTION_SLOTS, RARITY_FR, TYPE_FR,
                      Hero)

ROOT = Path(__file__).resolve().parent.parent
BIBLE = ROOT / "bible"
DATA = BIBLE / "data"
SITE = ROOT / "site"
ASSETS = SITE / "assets"
GRAPHICS = ROOT / "graphics"
# Public URL used for canonical links, Open Graph and the sitemap (override with SITE_URL=...).
SITE_URL = os.environ.get("SITE_URL", "https://slay-the-gba.alexandre-enouf.fr").rstrip("/")
ARTICLE_URL = "https://alexandre-enouf.fr/blog/slay-the-gba-vibe-coding-game-boy-advance/"
ARTICLE_URL_EN = ARTICLE_URL + "?lang=en"
REPO_URL = "https://github.com/Boblebol/slay_the_gba"
FEEDBACK_URL = f"{REPO_URL}/issues/new"  # where playtesters report bugs and feedback
ARTICLE_URL_EN = "https://alexandre-enouf.fr/en/blog/slay-the-gba-vibe-coding-game-boy-advance/"
REPO_URL = "https://github.com/Boblebol/slay_the_gba"
FEEDBACK_URL = f"{REPO_URL}/issues/new"  # where testers report bugs and feedback
PAGES: list[str] = []  # indexable pages, collected for sitemap.xml

# ---------------------------------------------------------------------------------------------
# Data
# ---------------------------------------------------------------------------------------------


POOL_FR = {"Colorless": "Incolores", "Event": "Évènement", "Curse": "Malédictions", "Quest": "Quêtes"}
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
# What a cartridge is built from: a ROM older than the newest of these files is a stale local build.
ROM_INPUTS = ("src", "include", "graphics", "audio")


def copy_roms() -> None:
    """Publish the built cartridges next to the pages, so download links work locally and once deployed.
    A ROM built before the last change to the game is not published: the previous download stays in place."""
    ROMS.mkdir(exist_ok=True)
    newest_input = max(f.stat().st_mtime for folder in ROM_INPUTS for f in (ROOT / folder).rglob("*") if f.is_file())
    for hero in HEROES:
        src = ROOT / hero.rom
        if not src.exists():
            continue
        if src.stat().st_mtime < newest_input:
            print(f"WARNING: {hero.rom} is older than the game sources and was not published; run `make roms` first")
            continue
        (ROMS / hero.rom).write_bytes(src.read_bytes())


def rom_size(hero: "Hero") -> str:
    """Size of the published cartridge, the file the download button serves."""
    path = ROMS / hero.rom
    return f"{path.stat().st_size / 1024:.0f} Ko" if path.exists() else "à compiler"


def download_button(hero: "Hero", depth: int, cls: str = "btn", en: bool = False) -> str:
    size = rom_size(hero)
    if en:
        size = size.replace(" Ko", " KB").replace("à compiler", "not built yet")
    return (f'<a class="{cls}" href="{"../" * depth}roms/{hero.rom}" download>'
            f'▼ {"Download" if en else "Télécharger"} {esc(hero.rom)} <small>({size})</small></a>')


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
    ("avancement.html", "Avancement"),
    ("guide.html", "Guide"),
    ("bible/index.html", "Bible"),
]
# The English pages only translate the landing: the encyclopedia stays in French, the language of the game.
NAV_EN = [
    ("en/index.html", "Home"),
    ("heros.html", "Heroes (FR)"),
    ("cartes.html", "Cards (FR)"),
    ("avancement.html", "Progress (FR)"),
]


def page(title: str, body: str, *, depth: int = 0, active: str = "", theme: str = "", description: str = "",
         path: str = "", noindex: bool = False, absolute: bool = False, lang: str = "fr") -> str:
    """Wrap a page body in the site shell. `path` is the page's URL path, used for SEO tags and the sitemap.
    `lang="en"` gives the English shell (navigation, footer, locale) and links back to the French landing."""
    up = "/" if absolute else "../" * depth
    en = lang == "en"
    desc = description or ("Slay the GBA: Slay the Spire 2 reimagined on Game Boy Advance, one cartridge per hero." if en
                           else "Slay the GBA : Slay the Spire 2 réinventé sur Game Boy Advance, une cartouche par héros.")
    canonical = f"{SITE_URL}/{path}" if path else SITE_URL + "/"
    if path and not noindex:
        PAGES.append(path)
    seo = f"""<link rel="canonical" href="{esc(canonical)}">
<meta name="theme-color" content="#0d0b16">
<meta name="robots" content="{'noindex, follow' if noindex else 'index, follow'}">
<meta property="og:type" content="website">
<meta property="og:site_name" content="Slay the GBA">
<meta property="og:locale" content="{'en_US' if en else 'fr_FR'}">
<meta property="og:title" content="{esc(title)} — Slay the GBA">
<meta property="og:description" content="{esc(desc)}">
<meta property="og:url" content="{esc(canonical)}">
<meta property="og:image" content="{SITE_URL}/assets/og.png">
<meta property="og:image:width" content="1200">
<meta property="og:image:height" content="630">
<meta property="og:image:alt" content="{'Slay the GBA — title screen on Game Boy Advance' if en else 'Slay the GBA — écran titre sur Game Boy Advance'}">
<meta name="twitter:card" content="summary_large_image">
<meta name="twitter:title" content="{esc(title)} — Slay the GBA">
<meta name="twitter:description" content="{esc(desc)}">
<meta name="twitter:image" content="{SITE_URL}/assets/og.png">
<link rel="apple-touch-icon" href="{up}assets/sprites/icon.png">"""
    if path in ("", "en/index.html"):  # the two landings point at each other for search engines
        seo += (f'\n<link rel="alternate" hreflang="fr" href="{SITE_URL}/">'
                f'\n<link rel="alternate" hreflang="en" href="{SITE_URL}/en/index.html">'
                f'\n<link rel="alternate" hreflang="x-default" href="{SITE_URL}/">')
    links = "".join(
        f'<a href="{up}{href}"{" class=\"on\"" if href == active else ""}>{esc(label)}</a>' for href, label in (NAV_EN if en else NAV)
    )
    switch = (f'<a class="lang" href="{up}index.html" hreflang="fr" lang="fr" title="Version française">FR</a>' if en else
              f'<a class="lang" href="{up}en/index.html" hreflang="en" lang="en" title="English version">EN</a>')
    if en:
        foot = f"""<p>Slay the GBA — unofficial fan game. <i>Slay the Spire</i> © Mega Crit. Butano engine © Gustavo Valiente.</p>
  <p>Data: project bible (patch v0.111.0 Beta). <a href="{up}mentions.html">Legal notice &amp; privacy (FR)</a> · <a href="{ARTICLE_URL_EN}" target="_blank" rel="noopener">How this game was vibe coded</a></p>"""
        tag, author_label = "Product developer &amp; AI builder · Paris, France", "Author links"
    else:
        foot = f"""<p>Slay the GBA — fan-game non officiel. <i>Slay the Spire</i> © Mega Crit. Moteur Butano © Gustavo Valiente.</p>
  <p>Données : bible du projet (patch v0.111.0 Beta). <a href="{up}mentions.html">Mentions légales &amp; confidentialité</a> · <a href="{ARTICLE_URL}" target="_blank" rel="noopener">Comment ce jeu a été vibe codé</a></p>"""
        tag, author_label = "Développeur produit &amp; Builder IA · Paris, France", "Liens de l'auteur"
    return f"""<!doctype html>
<html lang="{lang}">
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
  <nav>{links}{switch}</nav>
</header>
<main>
{body}
</main>
<footer class="foot">
  {foot}
  <div class="ae-footer">
    <a class="ae-logo" href="https://alexandre-enouf.fr/" target="_blank" rel="noopener">Alexandre <em>Enouf</em></a>
    <span class="ae-tag">{tag}</span>
    <nav class="ae-links" aria-label="{author_label}">
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
        export_sprite(f"hero_{hero.slug}", f"hero_{hero.slug}", 64)  # standing pose, the first frame
        export_sprite(f"ui_energy_{hero.slug}", f"energy_{hero.slug}")
        export_sprite(f"card_strike_{hero.slug}", f"card_strike_{hero.slug}")
        export_bg(f"bg_battle_{hero.slug}", f"battle_{hero.slug}")
    for mon in ["cultist", "jaw_worm", "louse", "gremlin_nob"]:
        export_sprite(f"mon_{mon}", f"mon_{mon}")
    export_sprite("boss_ceremonial_beast", "boss_ceremonial_beast", 64, (0, 1, 2, 3), (2, 2))
    icon_image().save(ASSETS / "sprites" / "icon.png")  # favicon of the visual identity (#8)
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

FAQ = [
    ("Quelle édition choisir pour commencer ?",
     "Le <b>Soldat de Fer</b> : 80 PV, une relique qui soigne après chaque combat et un deck direct. La Silencieuse et "
     "le Défectueux demandent un peu plus de planification ; le Régent et la Nécrolieuse, nouveaux dans Slay the Spire 2, "
     "sont pensés pour les joueurs confirmés."),
    ("Qu'est-ce qui change d'une cartouche à l'autre ?",
     "Le héros, ses cartes, sa relique de départ, sa mécanique signature, sa couleur, sa zone et ses teintes. La Tour, "
     "les monstres, les reliques partagées, les potions, les évènements et les Anciens sont communs aux cinq éditions."),
    ("C'est gratuit ?",
     "Oui. Les cinq ROMs sont des créations homebrew téléchargeables gratuitement. Aucune ROM commerciale n'est distribuée."),
    ("Ma partie est-elle sauvegardée ?",
     "Oui, automatiquement sur la cartouche (SRAM) à chaque salle, avec vos réglages de volume. Choisissez "
     "<i>Continuer</i> à l'écran titre pour reprendre."),
    ("Puis-je jouer sur une vraie Game Boy Advance ?",
     "Oui : copiez la ROM sur une cartouche flash (EZ-Flash, EverDrive) et jouez sur GBA, GBA SP, Game Boy Micro ou DS."),
    ("Est-ce un jeu officiel ?",
     "Non. Slay the GBA est un fan-game non officiel et non commercial, sans lien avec Mega Crit ni Nintendo."),
]


def stars(level: int) -> str:
    return f'<span class="diff" title="{DIFFICULTY[level]}">{"★" * level}{"☆" * (3 - level)}</span>'


def cards_by_id(cards: list[dict]) -> dict[str, dict]:
    return {c["id"]: c for c in cards}


def starter_html(hero: Hero, by_id: dict[str, dict]) -> str:
    return ", ".join(f"{n}× {esc(by_id[cid]['name'])}" for cid, n in hero.starter)


def relic_html(relic: dict, depth: int, cls: str = "relic") -> str:
    return (f'<div class="{cls}"><img loading="lazy" src="{img_path(relic["imagePath"], depth)}" alt="">'
            f'<div><b>{esc(relic["name"])}</b><p>{rich(relic.get("descI18n") or relic.get("descriptionClean", ""))}</p></div></div>')


def edition_card(h: Hero, up: str = "") -> str:
    return f"""<a class="cart" href="{up}heros/{h.slug}.html" style="--hc:{h.color};--ha:{h.accent}">
  <div class="cart-label" style="background-image:url({up}assets/bg/battle_{h.slug}.png)">
    <span class="cart-ref">{esc(h.ref)}</span>
    <img class="pix" src="{up}assets/sprites/hero_{h.slug}.png" alt="{esc(h.name)}">
  </div>
  <div class="cart-info"><b>{esc(h.name)}</b><span>{esc(h.edition)}</span>
  <em>« {esc(h.tagline)} »</em>
  <small>{h.hp} PV · {esc(h.mechanic_name)} · {stars(h.difficulty)}</small></div>
</a>"""


def comparison_table(by_id: dict[str, dict], relics: dict[str, dict], up: str = "") -> str:
    head = "".join(f'<th style="--hc:{h.color}"><a href="{up}heros/{h.slug}.html"><img class="pix" src="{up}assets/sprites/hero_{h.slug}.png" alt=""><br>{esc(h.name)}</a></th>' for h in HEROES)
    rows = [
        ("Cartouche", lambda h: f"<code>{esc(h.rom)}</code>"),
        ("Couleur", lambda h: f'<span class="swatch" style="background:{h.color}"></span> {esc(h.color_name)}'),
        ("Zone", lambda h: esc(h.zone)),
        ("Points de vie", lambda h: f"<b>{h.hp}</b> PV"),
        ("Relique de départ", lambda h: esc(relics[h.relic_id]["name"])),
        ("Deck de départ", lambda h: starter_html(h, by_id)),
        ("Mécanique", lambda h: esc(h.mechanic_name)),
        ("Difficulté", lambda h: f"{stars(h.difficulty)} {DIFFICULTY[h.difficulty]}"),
    ]
    body = "".join(f"<tr><th>{label}</th>{''.join(f'<td>{fn(h)}</td>' for h in HEROES)}</tr>" for label, fn in rows)
    return f'<div class="table-scroll"><table class="compare"><tr><th></th>{head}</tr>{body}</table></div>'


def controls_table() -> str:
    rows = "".join(f"<tr><td>{esc(s)}</td><td><b>{esc(b)}</b></td><td>{esc(a)}</td></tr>" for s, b, a in CONTROLS)
    return f'<table class="keys"><tr><th>Écran</th><th>Bouton</th><th>Action</th></tr>{rows}</table>'


LANDING_COLUMNS = (
    (progress.DONE, "Fait", "#68e868", "✓"),
    (progress.PARTIAL, "En cours", "#f8d048", "◐"),
    (progress.TODO, "À venir", "#9a98b0", "○"),
)
JOURNAL_SECTIONS = ("systemes", "interface", "regles")  # hand-written checklists of avancement.toml, in priority order
JOURNAL_LIMIT = 10
BIBLE_MONSTERS = (("mon_cultist", "Cultist"), ("mon_jaw_worm", "Jaw Worm"), ("mon_louse", "Morpion"),
                  ("mon_gremlin_nob", "Gremlin Nob"))
DEFAULT_EDITION = HEROES[0].slug


def progress_color(percent: float) -> str:
    """Bar colour of a section: the closer to done, the greener."""
    return next((color for floor, color in ((85, "#68e868"), (55, "#f8d048"), (25, "#f0a060")) if percent >= floor),
                "#f85848")


def hero_mechanics(section: progress.Section, limit: int = 3) -> list[str]:
    """Names of the signature mechanics already in the game, from the hero's "Mécaniques" checklist."""
    return [item.name for category in section.categories if category.title == "Mécaniques"
            for item in category.items if item.status == progress.DONE][:limit]


def journal_items(prog: progress.Progress, status: str) -> list[progress.Item]:
    """Checklist items of one status for the journal: game systems and interface before the combat rules."""
    items = [item for slug in JOURNAL_SECTIONS for category in prog.section(slug).categories
             for item in category.items if item.status == status]
    return items[:JOURNAL_LIMIT]


def counters(tally: progress.Tally) -> str:
    return (f'<span class="cnt cnt-done">{tally.done} faits</span><span class="cnt cnt-partial">{tally.partial} partiels</span>'
            f'<span class="cnt cnt-todo">{tally.todo} à faire</span>')


def launch_sprites() -> str:
    """Hero picker of the banner: one sprite button per edition."""
    return "".join(
        f'<button type="button" class="pick" data-edition="{h.slug}" style="--hc:{h.color};--ha:{h.accent}" '
        f'title="{esc(h.name)}" aria-pressed="{"true" if h.slug == DEFAULT_EDITION else "false"}">'
        f'<img class="pix" src="assets/sprites/hero_{h.slug}.png" alt="{esc(h.name)}"></button>'
        for h in HEROES
    )


def edition_tab(h: Hero) -> str:
    return (f'<button type="button" class="cart tab" data-edition="{h.slug}" style="--hc:{h.color};--ha:{h.accent}" '
            f'aria-pressed="{"true" if h.slug == DEFAULT_EDITION else "false"}">'
            f'<span class="cart-ref">{esc(h.ref)}</span>'
            f'<img class="pix" src="assets/sprites/hero_{h.slug}.png" alt=""><b>{esc(h.name)}</b></button>')


def edition_showcase(h: Hero, prog: progress.Progress, relic_by_id: dict[str, dict], by_id: dict[str, dict]) -> str:
    """The big panel of one edition. All five are rendered; the script only toggles `hidden`."""
    section = prog.section(h.slug)
    mechanics = "".join(f"<li>{esc(name)}</li>" for name in hero_mechanics(section))
    facts = [("Points de vie", f"{h.hp} PV"), ("Relique de départ", esc(relic_by_id[h.relic_id]["name"])),
             ("Mécanique", esc(h.mechanic_name)), ("Difficulté", f"{stars(h.difficulty)} {DIFFICULTY[h.difficulty]}"),
             ("Zone", esc(h.zone)), ("Deck de départ", starter_html(h, by_id))]
    tiles = "".join(f"<div><span>{k}</span><b>{v}</b></div>" for k, v in facts)
    hidden = "" if h.slug == DEFAULT_EDITION else " hidden"
    return f"""<div class="edition-panel" data-panel="{h.slug}" style="--hc:{h.color};--ha:{h.accent}"{hidden}>
  <div class="ed-shot">
    <div class="shot-frame"><img class="pix" loading="lazy" src="assets/screens/{h.screenshot}.png" alt="Combat de {esc(h.name)}"><span class="rom-tag">{esc(h.rom)}</span></div>
    <p class="meta">Capture réelle · {esc(h.zone)}</p>
  </div>
  <div class="ed-body">
    <div class="ed-head"><img class="pix" src="assets/sprites/hero_{h.slug}.png" alt="">
      <div><span class="ed-kicker">Édition {esc(h.name)} · {esc(h.color_name)}</span><h3>{esc(h.name)}</h3></div></div>
    <p class="ed-quote">« {esc(h.tagline)} »</p>
    <ul class="ed-sig">{mechanics}</ul>
    <div class="ed-facts">{tiles}</div>
    <div class="ed-prog"><span class="meta">Portage de l'édition</span><b>{progress.percent_fr(section.tally.percent)}</b>
      {meter(section.tally, f"Avancement de l'édition {h.name}", h.color)}
      <span class="meta">{esc(progress.counts_fr(section.tally))}</span></div>
    <div class="cta">{download_button(h, 0)}<a class="btn ghost" href="heros/{h.slug}.html">Toutes ses cartes ►</a></div>
  </div>
</div>"""


def landing_progress(prog: progress.Progress) -> str:
    version = progress.current_version()
    release = f"Version {esc(version)} · " if version else ""
    tiles = "".join(
        f'<a class="sect" href="avancement.html#{s.slug}"><span>{esc(s.title)}<b>{progress.percent_fr(s.tally.percent)}</b></span>'
        f'{meter(s.tally, s.title, progress_color(s.tally.percent))}</a>'
        for s in prog.sections
    )
    return panel(f"""<h2 id="avancement">Où en est le portage ?</h2>
<p class="intro">Le jeu se construit carte après carte. Voici la part du contenu de Slay the Spire 2 déjà
implémentée dans le code du jeu, recalculée à chaque mise à jour du site.</p>
<div class="prog-total"><b>{progress.percent_fr(prog.percent)}</b><div>{meter(prog.tally, "Avancement global", percent=prog.percent)}
<p class="meta">{release}global</p><p class="counters">{counters(prog.tally)}</p><p class="meta">{progress.GLOBAL_RULE}</p></div></div>
<div class="sect-grid">{tiles}</div>
<p><a class="btn small" href="avancement.html">Voir tout le détail ►</a></p>""")


def landing_journal(prog: progress.Progress) -> str:
    columns = ""
    for status, label, color, icon in LANDING_COLUMNS:
        rows = "".join(
            f'<li><span class="ico">{icon}</span><span>{esc(item.name)}'
            + (f' <a href="{progress.ISSUE_URL}{item.issue}" target="_blank" rel="noopener">#{item.issue}</a>'
               if item.issue else "") + "</span></li>"
            for item in journal_items(prog, status)
        )
        columns += (f'<div class="jcard" style="--jc:{color}"><h3>{label}</h3>'
                    f'<ul>{rows or "<li><span class=meta>Rien pour le moment.</span></li>"}</ul></div>')
    return panel(f"""<h2 id="fait">Journal de bord</h2>
<p class="intro">Ce qui est fait, ce qui avance et ce qui arrive, tiré du suivi d'avancement du projet.</p>
<div class="journal">{columns}</div>""")


def landing_captures() -> str:
    first, caption = SCREENS[0]
    thumbs = "".join(
        f'<a class="thumb" href="assets/screens/{n}.png" data-caption="{esc(c)}"><img class="pix" loading="lazy" '
        f'src="assets/screens/{n}.png" alt="{esc(c)}"></a>' for n, c in SCREENS
    )
    return panel(f"""<h2 id="captures">Captures</h2><p>Images réelles de la ROM, capturées dans l'émulateur.</p>
<div class="gallery"><figure class="gallery-main"><img class="pix" id="gallery-main" src="assets/screens/{first}.png" alt="{esc(caption)}">
<figcaption id="gallery-caption">{esc(caption)}</figcaption></figure><div class="thumbs">{thumbs}</div></div>""")


def bible_chapters() -> list[tuple[str, str]]:
    """(page stem, title) of the numbered bible tomes, in reading order."""
    return [(path.stem, bible_title(path)) for path in sorted(BIBLE.glob("[0-9]*.md"))]


def landing_bible(monsters_html: str) -> str:
    chapters = "".join(
        f'<a href="bible/{stem}.html"><span>{stem.split("_")[0]}</span>{esc(title)}</a>' for stem, title in bible_chapters())
    livrets = len(list((BIBLE / "livrets_gba").glob("livret_*.md")))
    return panel(f"""<h2 id="bible">La bible du projet</h2>
<p class="intro">Chaque carte, monstre, relique et règle du jeu est documentée dans la bible de conception, d'où le site tire ses données.</p>
<div class="mini-mons">{monsters_html}</div>
<div class="chapters">{chapters}</div>
<p><a class="btn small" href="bible/livrets.html">×{livrets} Livrets d'instructions ►</a> <a class="btn small ghost" href="bible/index.html">Toute la bible ►</a></p>""")


def testers_panel(prog: progress.Progress, *, en: bool = False) -> str:
    """Call for playtesters, shown on both landings: where to report a bug or give feedback."""
    done = prog.percent >= 100
    if en:
        title = "100% reached: playtesters wanted!" if done else "Playtesters wanted!"
        text = ("Every piece of Slay the Spire 2 content in the project bible (patch v0.111.0 Beta) is now in the five "
                "cartridges: cards, relics, potions, monsters, bosses, events and Ancients. " if done else "")
        text += ("Now it needs players. Grab an edition, play a run and tell me how it went: bugs, crashes, softlocks, "
                 "balance, text that overflows, anything.")
        links = (f'<a class="btn small" href="{FEEDBACK_URL}" target="_blank" rel="noopener">Report a bug or give feedback ►</a>'
                 '<a class="btn small ghost" href="mailto:alexandre.enouf@gmail.com">By e-mail</a>')
    else:
        title = "100 % atteint : testeurs recherchés !" if done else "Testeurs recherchés !"
        text = ("Tout le contenu de Slay the Spire 2 de la bible du projet (patch v0.111.0 Beta) est maintenant dans les "
                "cinq cartouches : cartes, reliques, potions, monstres, boss, évènements et Anciens. " if done else "")
        text += ("Il faut maintenant des joueurs. Téléchargez une édition, faites une partie et racontez-moi : bug, crash, "
                 "blocage, équilibrage, texte qui déborde, tout m'intéresse.")
        links = (f'<a class="btn small" href="{FEEDBACK_URL}" target="_blank" rel="noopener">Signaler un bug ou donner un avis ►</a>'
                 '<a class="btn small ghost" href="mailto:alexandre.enouf@gmail.com">Par e-mail</a>')
    anchor = "testers" if en else "testeurs"
    return panel(f'<h2 id="{anchor}">🎉 {title}</h2><p>{text}</p><div class="cta">{links}</div>', "callout")


def build_index(cards: list[dict], monsters: list[dict], relics: list[dict], potions: list[dict], events: list[dict],
                prog: progress.Progress) -> None:
    by_id = cards_by_id(cards)
    relic_by_id = {r["id"]: r for r in relics}
    downloads = "".join(
        f'<div class="download" style="--hc:{h.color}"><img class="pix" src="assets/sprites/hero_{h.slug}.png" alt=""><div><b>{esc(h.name)}</b><span class="meta">{esc(h.edition)} · {esc(h.color_name)}</span>{download_button(h, 0, "btn small")}</div></div>'
        for h in HEROES
    )
    pool_size = len([c for c in cards if c["pool"] == "Ironclad"])
    stats = [(5, "éditions"), (len(cards), "cartes"), (len([m for m in monsters if m.get('acts')]), "monstres"),
             (len(relics), "reliques"), (len(potions), "potions"), (len(events), "évènements"), (8, "Anciens")]
    stat_html = "".join(f"<div><b>{n}</b><span>{esc(l)}</span></div>" for n, l in stats)
    bestiary = "".join(
        f'<div class="mini-mon"><img class="pix" src="assets/sprites/{s}.png" alt="{esc(n)}"><span>{esc(n)}</span></div>'
        for s, n in BIBLE_MONSTERS
    ) + '<div class="mini-mon boss"><img class="pix" src="assets/sprites/boss_ceremonial_beast.png" alt="Cerf Auguste"><span>Cerf Auguste (Boss)</span></div>'
    faq = "".join(f"<details class='faq'><summary>{esc(q)}</summary><p>{a}</p></details>" for q, a in FAQ)
    version = progress.current_version()
    badge = f'<span class="ver">v{esc(version)}</span>' if version else ""
    tabs = "".join(edition_tab(h) for h in HEROES)
    showcases = "".join(edition_showcase(h, prog, relic_by_id, by_id) for h in HEROES)
    common = f'{GOLD} pièces d’or au départ, {ENERGY} énergie par tour, {POTION_SLOTS} emplacements de potion, 4 teintes, Ascension 0 à 10.'
    body = f"""
<section class="hero-banner landing" style="background-image:url(assets/bg/title.png)">
  <div class="hero-inner">
    <p class="kicker">{badge}<span>Game Boy Advance · Deckbuilder roguelike · 5 éditions</span></p>
    <h1>SLAY<br>THE GBA</h1>
    <p class="lead"><b>Slay the Spire 2 dans votre poche.</b> Cinq héros, cinq cartouches, une seule Tour à gravir — en 240×160, avec des combats façon Pokémon et les vraies illustrations du jeu.</p>
    <div class="cta"><a class="btn big" href="#telecharger">▼ Télécharger gratuitement</a><a class="btn ghost big" href="#editions">Choisir mon édition</a><span class="press-start">▶ PRESS START</span></div>
    <p class="pick-label">Choisissez votre héros</p>
    <div class="picks">{launch_sprites()}</div>
  </div>
</section>

<div class="stats">{stat_html}</div>

{testers_panel(prog)}

{panel(f'<h2 id="editions">Choisissez votre édition</h2><p class="intro">Comme les grandes séries portables vendues en plusieurs versions, Slay the GBA existe en <b>cinq cartouches</b>. Chacune a son héros, son deck de {pool_size}+ cartes, sa relique, sa couleur, sa zone et sa mécanique signature.</p><div class="tabs">' + tabs + '</div>' + showcases + f'<p class="meta">Toutes les éditions : {common}</p><details class="faq"><summary>Comparer les cinq éditions</summary>' + comparison_table(by_id, relic_by_id) + '</details><details class="faq"><summary>Commandes (identiques sur les cinq cartouches)</summary>' + controls_table() + '</details>')}

{landing_progress(prog)}

{landing_journal(prog)}

{landing_captures()}

{landing_bible(bestiary)}

{panel('<h2 id="telecharger">Télécharger les cartouches</h2><p>Une ROM <code>.gba</code> par héros, gratuite, à ouvrir dans un émulateur (mGBA, Delta sur iPhone, RetroArch…) ou à flasher sur une cartouche EZ-Flash / EverDrive.</p><div class="downloads">' + downloads + '</div>')}

{panel('''<h2 id="jouer">Jouer en 3 minutes</h2>
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
</ol>''')}

{panel('<h2 id="faq">Questions fréquentes</h2>' + faq + f'<p><a class="btn small ghost" href="{ARTICLE_URL}" target="_blank" rel="noopener">📖 L’histoire du projet</a></p>')}
"""
    (SITE / "index.html").write_text(page("Accueil", body, active="index.html", path="", theme=f"hero-{DEFAULT_EDITION}",
                                          description="Slay the GBA : Slay the Spire 2 réinventé sur Game Boy Advance. Cinq héros, cinq cartouches gratuites, combats façon Pokémon."))


# English landing: one entry per edition (tagline, signature mechanic, difficulty), keyed by hero slug.
HERO_EN = {
    "ironclad": ("Brute strength, forged in blood.", "Strength & sacrifice", "Beginner friendly"),
    "silent": ("Strike unseen. Let the venom finish the job.", "Poison & shivs", "Intermediate"),
    "defect": ("One automaton, three slots, endless circuits.", "Orbs & Focus", "Intermediate"),
    "regent": ("A crown, the stars, and the Spire on its knees.", "Stars & Sovereign Blade", "Expert"),
    "necrobinder": ("She never fights alone.", "Osty & Doom", "Expert"),
}

FAQ_EN = [
    ("Is the game in English?",
     "Not yet: every text in the game (cards, relics, events, menus) is in <b>French</b>, as is this website's "
     "encyclopedia. Card art, numbers and icons make most of it readable if you know Slay the Spire. An English "
     "version is possible if there is enough interest: tell me!"),
    ("Which edition should I start with?",
     "<b>The Ironclad</b>: 80 HP, a relic that heals after every fight and a straightforward deck. The Silent and the "
     "Defect ask for more planning; the Regent and the Necrobinder, new in Slay the Spire 2, are for seasoned players."),
    ("Is it free?",
     "Yes. The five ROMs are homebrew builds you can download for free. No commercial ROM is distributed."),
    ("Is my run saved?",
     "Yes, automatically on the cartridge (SRAM) at every room. Pick <i>Continuer</i> on the title screen to resume."),
    ("Can I play on a real Game Boy Advance?",
     "Yes: copy the ROM to a flash cart (EZ-Flash, EverDrive) and play on GBA, GBA SP, Game Boy Micro or DS."),
    ("Is this an official game?",
     "No. Slay the GBA is an unofficial, non-commercial fan game with no ties to Mega Crit or Nintendo."),
]

STEPS_EN = """<ol class="steps">
<li><b>Download your hero's cartridge</b> (a <code>.gba</code> file) from the editions above.</li>
<li><b>Install a free Game Boy Advance emulator</b>:
  <ul>
    <li>Computer (Windows, macOS, Linux): <a href="https://mgba.io/downloads.html" target="_blank" rel="noopener">mGBA</a> (recommended).</li>
    <li>iPhone / iPad: <a href="https://apps.apple.com/app/delta-game-emulator/id1048524688" target="_blank" rel="noopener">Delta</a>.</li>
    <li>Android: <a href="https://play.google.com/store/apps/details?id=com.fastemulator.gba" target="_blank" rel="noopener">My Boy!</a> or <a href="https://www.retroarch.com/" target="_blank" rel="noopener">RetroArch</a>.</li>
  </ul></li>
<li><b>Open the <code>.gba</code> file</b> in the emulator and press START.</li>
<li>Got real hardware? Copy the ROM to a flash cart (EZ-Flash, EverDrive) and play on GBA, GBA SP or DS.</li>
</ol>"""

LANGUAGE_NOTICE_EN = (
    '<h2 id="language">🇫🇷 The game is in French</h2><p>All in-game text (cards, relics, events, menus) is in '
    '<b>French</b>, and so is the <a href="../cartes.html">card encyclopedia</a> on this site. If you know Slay the '
    'Spire, the card art, numbers and intent icons carry you a long way. An English version will depend on how much '
    'interest there is.</p>')


def edition_card_en(h: Hero) -> str:
    tagline, mechanic, difficulty = HERO_EN[h.slug]
    return f"""<div class="cart" style="--hc:{h.color};--ha:{h.accent}">
  <div class="cart-label" style="background-image:url(../assets/bg/battle_{h.slug}.png)">
    <span class="cart-ref">{esc(h.ref)}</span>
    <img class="pix" src="../assets/sprites/hero_{h.slug}.png" alt="{esc(h.english)}">
  </div>
  <div class="cart-info"><b>{esc(h.english)}</b><span>In game: {esc(h.name)}</span>
  <em>“{esc(tagline)}”</em>
  <small>{h.hp} HP · {esc(mechanic)} · {stars(h.difficulty)} {difficulty}</small>
  {download_button(h, 1, "btn small", en=True)}</div>
</div>"""


def build_index_en(prog: progress.Progress) -> None:
    """English landing (site/en/index.html): pitch, French-only notice, editions, downloads, how to play, FAQ."""
    (SITE / "en").mkdir(exist_ok=True)
    version = progress.current_version()
    badge = f'<span class="ver">v{esc(version)}</span>' if version else ""
    carts = "".join(edition_card_en(h) for h in HEROES)
    faq = "".join(f"<details class='faq'><summary>{esc(q)}</summary><p>{a}</p></details>" for q, a in FAQ_EN)
    thumbs = "".join(f'<a class="thumb" href="../assets/screens/{n}.png"><img class="pix" loading="lazy" src="../assets/screens/{n}.png" alt=""></a>'
                     for n in ("title", "map", "combat", "impact", "event", "shop"))
    body = f"""
<section class="hero-banner landing" style="background-image:url(../assets/bg/title.png)">
  <div class="hero-inner">
    <p class="kicker">{badge}<span>Game Boy Advance · Roguelike deckbuilder · 5 editions</span></p>
    <h1>SLAY<br>THE GBA</h1>
    <p class="lead"><b>Slay the Spire 2 in your pocket.</b> Five heroes, five cartridges, one Spire to climb, in 240×160 with Pokémon-style battles and the game's real card art.</p>
    <div class="cta"><a class="btn big" href="#download">▼ Download for free</a><a class="btn ghost big" href="#editions">Pick my edition</a></div>
  </div>
</section>

{panel(LANGUAGE_NOTICE_EN, "callout")}

{testers_panel(prog, en=True)}

{panel(f'<h2 id="editions">Five editions, one per hero</h2><p class="intro">Like the classic handheld series sold in several versions, Slay the GBA comes as <b>five cartridges</b>. Each one has its hero, card pool, starting relic, colour and signature mechanic. The Spire, monsters, shared relics, potions, events and Ancients are the same in all five.</p><div class="carts">{carts}</div>')}

{panel(f'<h2 id="screens">Screenshots</h2><p>Real captures of the ROM, taken in an emulator.</p><div class="thumbs">{thumbs}</div><p><a href="../index.html#captures">More screenshots (French page) ►</a></p>')}

{panel('<h2 id="download">Play in 3 minutes</h2>' + STEPS_EN + f'<p class="meta">Source code, release notes and every ROM: <a href="{REPO_URL}" target="_blank" rel="noopener">GitHub</a>.</p>')}

{panel('<h2 id="faq">FAQ</h2>' + faq + f'<p><a class="btn small ghost" href="{ARTICLE_URL_EN}" target="_blank" rel="noopener">📖 The story of the project</a></p>')}
"""
    (SITE / "en" / "index.html").write_text(
        page("Slay the Spire 2 on Game Boy Advance", body, depth=1, active="en/index.html", path="en/index.html",
             theme=f"hero-{DEFAULT_EDITION}", lang="en",
             description="Slay the GBA: a free Slay the Spire 2 demake for Game Boy Advance. Five heroes, five "
                         "cartridges, Pokémon-style battles. In French, playtesters wanted."))


def build_heroes_index(cards: list[dict], relics: list[dict]) -> None:
    by_id = cards_by_id(cards)
    relic_by_id = {r["id"]: r for r in relics}
    rows = "".join(
        f"""<a class="hero-row" href="heros/{h.slug}.html" style="--hc:{h.color};--ha:{h.accent}">
  <img class="pix big" src="assets/sprites/hero_{h.slug}.png" alt="">
  <div><p class="meta">{esc(h.edition)} · {esc(h.color_name)} · {esc(h.zone)}</p><h3>{esc(h.name)} <small>— {esc(h.title)}</small></h3>
  <p class="tagline">« {esc(h.tagline)} »</p><p>{esc(h.pitch)}</p>
  <p class="meta">{h.hp} PV · Relique : {esc(relic_by_id[h.relic_id]['name'])} · {esc(h.mechanic_name)} · {stars(h.difficulty)} {DIFFICULTY[h.difficulty]}</p></div>
</a>"""
        for h in HEROES
    )
    body = (panel("<h1>Les 5 éditions</h1><p class='intro'>Un héros par cartouche. Même Tour, cinq façons de la gravir.</p>" + rows)
            + panel("<h2>Comparer les éditions</h2>" + comparison_table(by_id, relic_by_id)))
    (SITE / "heros.html").write_text(page("Héros", body, active="heros.html", path="heros.html",
                                          description="Les 5 éditions de Slay the GBA : Soldat de Fer, La Silencieuse, Le Défectueux, Le Régent et La Nécrolieuse."))


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


def build_hero_pages(cards: list[dict], relics: list[dict], prog: progress.Progress) -> None:
    (SITE / "heros").mkdir(exist_ok=True)
    by_id = cards_by_id(cards)
    relic_by_id = {r["id"]: r for r in relics}
    order = {"Basic": 0, "Common": 1, "Uncommon": 2, "Rare": 3, "Ancient": 4}
    for i, h in enumerate(HEROES):
        pool = sorted((c for c in cards if c["pool"] == h.pool), key=lambda c: (order.get(c["rarity"], 9), c["name"]))
        hero_relics = [r for r in relics if r["pool"] == h.pool]
        counts = {t: sum(1 for c in pool if c["type"] == t) for t in ["Attack", "Skill", "Power"]}
        key_cards = "".join(card_html(by_id[cid], 1) for cid in h.key_cards)
        starter = "".join(
            f'<li><b>{n}×</b> {esc(by_id[cid]["name"])} <span class="meta">— {rich(by_id[cid].get("descBase", ""))}</span></li>'
            for cid, n in h.starter)
        palettes = "".join(
            f'<div class="palette"><span class="swatch big" style="background:{p.hex}"></span><div><b>{esc(p.name)}</b><p>{esc(p.description)}</p></div></div>'
            for p in h.palettes)
        tips = "".join(f"<li>{re.sub(r'[*][*](.*?)[*][*]', r'<b>\1</b>', esc(t))}</li>" for t in h.tips)
        prev_h, next_h = HEROES[i - 1], HEROES[(i + 1) % len(HEROES)]
        others = "".join(edition_card(o, "../") for o in HEROES if o is not h)
        body = f"""
<section class="hero-banner small" style="background-image:url(../assets/bg/battle_{h.slug}.png)">
  <div class="hero-inner split">
    <img class="pix giant" src="../assets/sprites/hero_{h.slug}.png" alt="{esc(h.name)}">
    <div><p class="kicker">{esc(h.edition)} · Cartouche {esc(h.rom)}</p><h1>{esc(h.name)}</h1><p class="lead">« {esc(h.tagline)} »</p>
    <p>{esc(h.pitch)}</p><div class="cta">{download_button(h, 1)}<a class="btn ghost" href="../bible/livret_{h.slug}.html">Lire le livret</a></div></div>
  </div>
</section>
<div class="stats"><div><b>{h.hp}</b><span>PV</span></div><div><b>{ENERGY}</b><span>énergie</span></div><div><b>{GOLD}</b><span>or</span></div><div><b>{len(pool)}</b><span>cartes</span></div>
<div><b>{counts['Attack']}</b><span>attaques</span></div><div><b>{counts['Skill']}</b><span>compétences</span></div><div><b>{counts['Power']}</b><span>pouvoirs</span></div><div><b>{"★" * h.difficulty}</b><span>{DIFFICULTY[h.difficulty].lower()}</span></div></div>
{panel(f'''<div class="story"><div><h2>{esc(h.title)}</h2><p>{esc(h.lore)}</p><p><b>Idéal pour</b> {esc(h.ideal)}</p>
<p class="meta">Zone : {esc(h.zone)} · Couleur de cartouche : <span class="swatch" style="background:{h.color}"></span> {esc(h.color_name)}</p></div>
<figure class="shot"><img class="pix" src="../assets/screens/{h.screenshot}.png" alt="Combat de {esc(h.name)}"><figcaption>{esc(h.name)} dans l'arène — capture réelle de la ROM</figcaption></figure></div>''')}
{panel(f'<h2>Mécanique signature : {esc(h.mechanic_name)}</h2><p>{esc(h.mechanic_text)}</p><h3>Cartes phares</h3><div class="cards">{key_cards}</div>')}
{panel(f'''<h2>Fiche de départ</h2><div class="sheet">
{relic_html(relic_by_id[h.relic_id], 1, "relic big")}
<div><h3>Deck de départ ({sum(n for _, n in h.starter)} cartes)</h3><ul class="starter">{starter}</ul>
<p class="meta">{GOLD} pièces d'or · {ENERGY} énergie par tour · {POTION_SLOTS} emplacements de potion</p></div>
<img class="pix energy" src="../assets/sprites/energy_{h.slug}.png" alt="Orbe d'énergie">
</div>''')}
{panel(f'<h2>Les 4 teintes</h2><p>Choisissez l’apparence de {esc(h.name)} au lancement de chaque partie.</p><div class="palettes">{palettes}</div>')}
{panel(f'<h2>Conseils de survie</h2><ol class="tips">{tips}</ol><p><a class="btn small" href="../bible/livret_{h.slug}.html">Livret complet de l’édition ►</a> <a class="btn small ghost" href="../bible/{h.tome}.html">Tome des cartes ►</a></p>')}
{hero_progress_panel(prog.section(h.slug))}
<section class="panel"><h2>Toutes les cartes ({len(pool)})</h2>{card_filters()}<div class="cards">{''.join(card_html(c, 1) for c in pool)}</div></section>
{panel('<h2>Reliques exclusives</h2><div class="relics">' + ''.join(relic_html(r, 1) for r in hero_relics) + '</div>')}
{panel(f'<h2>Les autres éditions</h2><p class="pager"><a href="{prev_h.slug}.html">◄ {esc(prev_h.name)}</a><a href="{next_h.slug}.html">{esc(next_h.name)} ►</a></p><div class="carts">{others}</div>')}
"""
        (SITE / "heros" / f"{h.slug}.html").write_text(
            page(h.name, body, depth=1, active="heros.html", theme=f"hero-{h.slug}",
                 description=f"{h.edition} de Slay the GBA — {h.tagline} {h.mechanic} Toutes ses cartes et la ROM {h.rom}.",
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


# ---------------------------------------------------------------------------------------------
# Progress tracker (scripts/progress.py)
# ---------------------------------------------------------------------------------------------

# Status is told by the glyph and the label; the colour only repeats it.
STATUS_BADGE = {progress.DONE: "<i>●</i> Oui", progress.PARTIAL: "<i>◐</i> Partiel", progress.TODO: "<i>○</i> Non",
                progress.EXCLUDED: "<i>–</i> Exclu"}


def meter(tally: progress.Tally, label: str, color: str = "", percent: float | None = None) -> str:
    """Progress track of a tally. `label` names what is measured, for screen readers."""
    value = tally.percent if percent is None else percent
    width = f"max(3px, {value:.1f}%)" if value else "0"
    fill = f' style="--fill:{color}"' if color else ""
    return (f'<div class="meter" role="progressbar" aria-valuemin="0" aria-valuemax="100" '
            f'aria-valuenow="{value:.0f}" aria-label="{esc(label)}"{fill}><span style="width:{width}"></span></div>')


def progress_total(prog: progress.Progress) -> str:
    version = progress.current_version()
    release = f"Version {esc(version)} · " if version else ""
    return (f'<div class="prog-total"><b>{progress.percent_fr(prog.percent)}</b>'
            f'<div>{meter(prog.tally, "Avancement global", percent=prog.percent)}'
            f'<p class="meta">{release}{esc(progress.counts_fr(prog.tally))}</p>'
            f'<p class="meta">{esc(progress.GLOBAL_RULE)}</p></div></div>')


def progress_tile(section: progress.Section, up: str = "") -> str:
    hero = section.hero
    sprite = f'<img class="pix" src="{up}assets/sprites/hero_{hero.slug}.png" alt="">' if hero else ""
    return (f'<a class="prog-tile" href="{up}avancement.html#{section.slug}">{sprite}<div><h3>{esc(section.title)}</h3>'
            f'<b>{progress.percent_fr(section.tally.percent)}</b>'
            f'{meter(section.tally, section.title, hero.color if hero else "")}'
            f'<p class="meta">{esc(progress.counts_fr(section.tally))}</p></div></a>')


def progress_item(item: progress.Item) -> str:
    issue = (f' <a href="{progress.ISSUE_URL}{item.issue}" target="_blank" rel="noopener">#{item.issue}</a>'
             if item.issue else "")
    note = f"<small>{esc(item.note)}{issue}</small>" if item.note or issue else ""
    detail = f' <span class="meta">{esc(item.detail)}</span>' if item.detail else ""
    search = item.name if item.key == item.name else f"{item.name} {item.key}"
    return (f'<li data-status="{item.status}" data-name="{esc(search.lower())}">'
            f'<span class="st st-{item.status}">{STATUS_BADGE[item.status]}</span>'
            f'<div><b>{esc(item.name)}</b>{detail}{note}</div></li>')


def progress_category(category: progress.Category, color: str = "", items: bool = True) -> str:
    """One category row: name, counts, track and percentage, unfolding to its checklist when `items` is set."""
    tally = category.tally
    head = (f'<span><b>{esc(category.title)}</b><span class="meta">{esc(progress.counts_fr(tally))}</span></span>'
            f'{meter(tally, category.title, color)}<span class="prog-num">{progress.percent_fr(tally.percent)}</span>')
    if not items:
        return f'<div class="prog-cat"><div class="prog-row">{head}</div></div>'
    rows = "".join(progress_item(item) for item in category.items)
    return f'<details class="prog-cat"><summary class="prog-row">{head}</summary><ul class="prog-list">{rows}</ul></details>'


def progress_panel(prog: progress.Progress) -> str:
    """Home page summary."""
    tiles = "".join(progress_tile(section) for section in prog.sections if section.hero)
    return panel(f"""<h2 id="avancement">Où en est le portage ?</h2>
<p class="intro">Le jeu se construit carte après carte. Voici la part du contenu de Slay the Spire 2 déjà
implémentée dans le code du jeu, recalculée à chaque mise à jour du site.</p>
{progress_total(prog)}<div class="prog-grid">{tiles}</div>
<p><a class="btn small" href="avancement.html">Voir tout le détail ►</a></p>""")


def hero_progress_panel(section: progress.Section) -> str:
    rows = "".join(progress_category(category, section.hero.color, items=False) for category in section.categories)
    return panel(f"""<h2>Avancement de l'édition : {progress.percent_fr(section.tally.percent)}</h2>
<p>Ce qui est déjà implémenté dans le code du jeu, sur l'ensemble des cartes, reliques et mécaniques de {esc(section.title)}.</p>
{rows}<p><a class="btn small" href="../avancement.html#{section.slug}">Voir le détail ►</a></p>""")


def build_progress_page(prog: progress.Progress) -> None:
    heroes = "".join(progress_tile(section) for section in prog.sections if section.hero)
    shared = "".join(progress_tile(section) for section in prog.sections if not section.hero)
    chips = "".join(f'<button data-filter-status="{status}">{label}</button>' for status, label in
                    [(progress.DONE, "Fait"), (progress.PARTIAL, "Partiel"), (progress.TODO, "À faire"),
                     (progress.EXCLUDED, "Hors périmètre")])
    details = ""
    for section in prog.sections:
        color = section.hero.color if section.hero else ""
        categories = "".join(progress_category(category, color) for category in section.categories)
        details += (f'<section class="panel prog-section" id="{section.slug}"><h2>{esc(section.title)} '
                    f'<small>{progress.percent_fr(section.tally.percent)}</small></h2>{categories}</section>')
    body = panel(f"""<h1>Avancement du portage</h1>
<p class="intro">Tout ce que Slay the GBA doit implémenter pour couvrir Slay the Spire 2, héros par héros puis pour
le jeu entier, et ce qui est déjà fait.</p>
{progress_total(prog)}
<p class="meta">Le numéro de version suit l'avancement : 0.9 à 9 %, 1.0 quand tout est fait.
Un élément partiel compte pour moitié ; les cartes et reliques multijoueur sont hors périmètre.
Les cartes, reliques, potions, monstres et évènements sont détectés dans le code du jeu.
{esc(progress.extras_fr(prog))}</p>""")
    body += panel(f'<h2>Par édition</h2><div class="prog-grid">{heroes}</div>')
    body += panel(f'<h2>Le jeu commun</h2><div class="prog-grid">{shared}</div>')
    body += panel(f"""<h2>Le détail</h2><div class="filters" data-progress>
  <input type="search" placeholder="Rechercher une carte, une relique, un monstre, une mécanique…" data-search>
  <div class="chips"><button class="on" data-filter-status="">Tout</button>{chips}</div>
  <p class="count"></p>
</div>""")
    (SITE / "avancement.html").write_text(page(
        "Avancement", body + details, active="avancement.html", path="avancement.html",
        description=f"Avancement de Slay the GBA : {progress.percent_fr(prog.percent)} du contenu de "
                    "Slay the Spire 2 est implémenté. Le détail par héros, carte, relique, monstre et système."))


def build_guide() -> None:
    parts = []
    for name in ["19_boucle_de_gameplay_et_ascension.md", "21_interface_pokemon_et_navigation_carte.md"]:
        parts.append('<div class="md">' + md_to_html((BIBLE / name).read_text(), 0, "").replace("../bible/", "bible_raw/") + "</div>")
    body = "".join(panel(p) for p in parts)
    body = body.replace('src="bible_raw/', 'src="../bible/').replace('href="bible_raw/', 'href="../bible/')
    body = re.sub(r'href="([\w.-]+)\.html', r'href="bible/\1.html', body)
    (SITE / "guide.html").write_text(page("Guide", body, active="guide.html", path="guide.html",
                                               description="Boucle de gameplay, ascension et interface de Slay the GBA."))


def bible_title(path: Path) -> str:
    """First heading of a bible markdown file, stripped of emoji."""
    m = re.search(r"^#\s+(.+)$", path.read_text(), re.M)
    return re.sub(r"[^\w\sÀ-ÿ'’:—\-&()!,.]", "", m.group(1)).strip() if m else path.stem


def build_bible() -> None:
    out_dir = SITE / "bible"
    out_dir.mkdir(exist_ok=True)
    tomes = sorted(BIBLE.glob("[0-9]*.md")) + [BIBLE / "spec_technique_gba.md"]
    livrets = sorted((BIBLE / "livrets_gba").glob("*.md"))
    toc = []
    for path in tomes + livrets:
        text = path.read_text()
        title = bible_title(path)
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
    prog = progress.compute()
    progress.write_report(prog)
    build_assets()
    copy_roms()
    build_index(cards, monsters["monsters"], relics, potions, events, prog)
    build_index_en(prog)
    build_heroes_index(cards, relics)
    build_hero_pages(cards, relics, prog)
    build_progress_page(prog)
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
