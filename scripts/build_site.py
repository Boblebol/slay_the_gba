# /// script
# requires-python = ">=3.12"
# dependencies = ["markdown>=3.6", "pillow>=10", "numpy>=1.26"]
# ///
"""Generate the static Slay the GBA website (site/) from the design bible and the game art.

Run from the repository root:  uv run scripts/build_site.py
Open site/index.html in a browser. Images are referenced from ../bible/images, so keep the
site/ folder next to bible/ (GitHub Pages serving the repository root works as-is).

The site is built twice: in French at the root of site/, in English under site/en/ (same file names). The game and
the design bible are in French; the English pages take card, relic and potion texts from the English templates of
bible/data (scripts/site_en.py) and the other English texts from site_en.py and site_en_strings.py.
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
import site_en
from build_identity import icon_image
from editions import (CONTROLS, DIFFICULTY, ENCOUNTER_FR, ENERGY, GOLD, HEROES, POTION_SLOTS, RARITY_FR, TYPE_FR,
                      Hero)
from site_en import HERO_TEXT, describe, english_name, id_name, markup
from site_en_strings import DETAIL_PARTS, TRACKER

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
PAGES: list[str] = []  # indexable pages, collected for sitemap.xml

# ---------------------------------------------------------------------------------------------
# Languages
# ---------------------------------------------------------------------------------------------

LANGS = ("fr", "en")
LANG = "fr"  # language of the pages being built; English pages go to site/en/
EN_NAMES: dict[str, str] = {}  # data id -> English name, for the items of the progress tracker


def set_lang(lang: str) -> None:
    global LANG
    LANG = lang


def en() -> bool:
    return LANG == "en"


def tr(fr: str, en_text: str) -> str:
    return en_text if LANG == "en" else fr


def root(depth: int = 0) -> str:
    """Path from a page `depth` folders deep in its language folder up to site/ (assets, ROMs, bible pages)."""
    return "../" * (depth + en())


def write_page(rel: str, text: str) -> None:
    """Write a page of the current language: site/<rel> in French, site/en/<rel> in English."""
    path = SITE / ("en" if en() else "") / rel
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)


DIFFICULTY_EN = {1: "Beginner friendly", 2: "Intermediate", 3: "Expert"}
RARITY_EN = {"Token": "Created in combat"}
POOL_FR = {"Colorless": "Incolores", "Event": "Évènement", "Curse": "Malédictions", "Quest": "Quêtes"}
POOL_EN = {"Colorless": "Colorless", "Event": "Event", "Curse": "Curses", "Quest": "Quests"}
INTENT_FR = {"attack": "Attaque", "defend": "Défense", "buff": "Renforcement", "debuff": "Affaiblissement",
             "status": "Statut", "heal": "Soin", "summon": "Invocation", "escape": "Fuite", "sleep": "Sommeil",
             "stun": "Étourdi", "unknown": "Inconnu", "hidden": "Caché", "card_debuff": "Malus de carte"}
ENCOUNTER_EN = {"normal": "Monster", "elite": "Elite", "boss": "Boss", "weak": "Weak", "summon": "Summon"}
ENCOUNTER_GROUPS = {"boss": ("Boss", "Bosses"), "elite": ("Élites", "Elites"), "normal": ("Monstres", "Monsters"),
                    "weak": ("Faibles", "Weak monsters"), "summon": ("Invocations", "Summons")}
ANCIENT_GROUPS = {"blessing": ("Bénédictions", "Blessings"), "curse": ("Fardeaux", "Burdens")}
GLOBAL_RULE_EN = "The overall percentage is the average of the section percentages, each section weighing the same."


def type_label(kind: str) -> str:
    return tr(TYPE_FR.get(kind, kind), kind)


def rarity_label(rarity: str) -> str:
    return tr(RARITY_FR.get(rarity, rarity), RARITY_EN.get(rarity, rarity))


def difficulty(level: int) -> str:
    return tr(DIFFICULTY[level], DIFFICULTY_EN[level])


def act_label(act: str) -> str:
    fr = progress.ACT_FR.get(act, act)
    return tr(fr, DETAIL_PARTS.get(fr, fr))


def hero_name(h: Hero) -> str:
    return tr(h.name, h.english)


def hero_text(h: Hero, field: str) -> str:
    """A text of the edition: editions.py in French, site_en.HERO_TEXT in English."""
    return HERO_TEXT[h.slug][field] if en() else getattr(h, field)


def hp(amount: int | str) -> str:
    return tr(f"{amount} PV", f"{amount} HP")


def quote(text: str) -> str:
    return tr(f"« {esc(text)} »", f"“{esc(text)}”")


def percent(value: float) -> str:
    return tr(progress.percent_fr(value), f"{value:.1f}%")


def counts(tally: progress.Tally) -> str:
    if not en():
        return progress.counts_fr(tally)
    text = f"{tally.done} done, {tally.partial} partial, {tally.todo} to do"
    return text + (f" ({tally.excluded} out of scope)" if tally.excluded else "")


def global_rule() -> str:
    return tr(progress.GLOBAL_RULE, GLOBAL_RULE_EN)


# ---------------------------------------------------------------------------------------------
# Data
# ---------------------------------------------------------------------------------------------


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


def entry_name(entry: dict, strip_hero: bool = False) -> str:
    """Name of a card, relic, potion or monster: French from the data, English from its class name."""
    if en() and entry.get("className"):
        return english_name(entry["className"], strip_hero)
    return entry["name"]


def entry_text(entry: dict, french: str, upgraded: bool = False) -> str:
    """Rules text as HTML: the French text, or the English template filled with the entry's values. An English
    template the data cannot fill (a few curses and relics) falls back to the French text."""
    if en() and entry.get("description"):
        text = describe(entry, upgraded=upgraded, french=french)
        if text is not None:
            return markup(esc(text))
    return rich(french)


def card_name(card: dict) -> str:
    return entry_name(card, strip_hero=True)


def card_text(card: dict, upgraded: bool = False) -> str:
    if upgraded:
        return entry_text(card, card["descUpgraded"], upgraded=True) if card.get("descUpgraded") else ""
    return entry_text(card, card.get("descBase") or card.get("descriptionClean", ""))


def relic_text(relic: dict) -> str:
    return entry_text(relic, relic.get("descI18n") or relic.get("descriptionClean", ""))


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
    if path.exists():
        return f"{path.stat().st_size / 1024:.0f} {tr('Ko', 'KB')}"
    return tr("à compiler", "not built yet")


def download_button(hero: "Hero", depth: int, cls: str = "btn") -> str:
    return (f'<a class="{cls}" href="{root(depth)}roms/{hero.rom}" download>'
            f'▼ {tr("Télécharger", "Download")} {esc(hero.rom)} <small>({rom_size(hero)})</small></a>')


def img_path(image_path: str, depth: int) -> str:
    return root(depth) + "../bible" + image_path


# ---------------------------------------------------------------------------------------------
# Layout
# ---------------------------------------------------------------------------------------------

# (page, French label, English label, group). The bible is only in French: the English menu links the French tomes.
# The compendium pages share one drop-down on wide screens; the START menu (phones) lists every group.
NAV = [
    ("index.html", "Accueil", "Home", "jeu"),
    ("heros.html", "Héros", "Heroes", "jeu"),
    ("guide.html", "Guide", "Guide", "jeu"),
    ("cartes.html", "Cartes", "Cards", "compendium"),
    ("bestiaire.html", "Bestiaire", "Bestiary", "compendium"),
    ("reliques.html", "Reliques", "Relics", "compendium"),
    ("potions.html", "Potions", "Potions", "compendium"),
    ("evenements.html", "Évènements", "Events", "compendium"),
    ("anciens.html", "Anciens", "Ancients", "compendium"),
    ("avancement.html", "Avancement", "Progress", "projet"),
    ("bible/index.html", "Bible", "Bible (FR)", "projet"),
]
NAV_GROUPS = {"jeu": ("Le jeu", "The game"), "compendium": ("Compendium", "Compendium"), "projet": ("Le projet", "The project")}

# Pixel icons of the phone dock, 10x10, drawn for the site (MIT).
ICONS = {
    "home": ["....##....", "...####...", "..######..", ".########.", "##########", ".########.", ".##....##.",
             ".##.##.##.", ".##.##.##.", ".########."],
    "hero": ["........##", ".......###", "......###.", ".....###..", "#...###...", "##.###....", ".####.....",
             "..##......", ".####.....", "##..#....."],
    "cards": ["...#######", "...#.....#", "#######..#", "#.....#..#", "#.###.#..#", "#.###.#..#", "#.....####",
              "#.....#...", "#.....#...", "#######..."],
    "play": ["....##....", "....##....", "....##....", ".########.", "..######..", "...####...", "....##....",
             "..........", "##########", "##########"],
    "menu": ["..........", "##########", "##########", "..........", "##########", "##########", "..........",
             "##########", "##########", ".........."],
}


def icon(name: str) -> str:
    """Inline SVG of a pixel icon, in the text colour."""
    rects = "".join(f'<rect x="{x}" y="{y}" width="1" height="1"/>' for y, row in enumerate(ICONS[name])
                    for x, ch in enumerate(row) if ch == "#")
    return (f'<svg class="ico" viewBox="0 0 10 10" width="20" height="20" aria-hidden="true" '
            f'shape-rendering="crispEdges" fill="currentColor">{rects}</svg>')


def page(title: str, body: str, *, depth: int = 0, active: str = "", theme: str = "", description: str = "",
         path: str = "", noindex: bool = False, absolute: bool = False, translated: bool = True) -> str:
    """Wrap a page body in the site shell, in the current language. `path` is the page's URL path inside its
    language folder ("" for the landing), used for SEO tags and the sitemap. A `translated` page exists in both
    languages: it links its counterpart (hreflang and the FR/EN switch); otherwise the switch leads to the other
    landing."""
    up = "/" if absolute else root(depth)  # site/: assets and the bible
    home = ("/en/" if en() else "/") if absolute else "../" * depth  # the language folder: the other pages
    desc = description or tr("Slay the GBA : Slay the Spire 2 réinventé sur Game Boy Advance, une cartouche par héros.",
                             "Slay the GBA: Slay the Spire 2 reimagined on Game Boy Advance, one cartridge per hero.")
    public = ("en/" + (path or "index.html")) if en() else path
    canonical = f"{SITE_URL}/{public}"
    if not noindex:
        PAGES.append(public)
    seo = f"""<link rel="canonical" href="{esc(canonical)}">
<meta name="theme-color" content="#0d0b16">
<meta name="robots" content="{'noindex, follow' if noindex else 'index, follow'}">
<meta property="og:type" content="website">
<meta property="og:site_name" content="Slay the GBA">
<meta property="og:locale" content="{tr('fr_FR', 'en_US')}">
<meta property="og:title" content="{esc(title)} — Slay the GBA">
<meta property="og:description" content="{esc(desc)}">
<meta property="og:url" content="{esc(canonical)}">
<meta property="og:image" content="{SITE_URL}/assets/og.png">
<meta property="og:image:width" content="1200">
<meta property="og:image:height" content="630">
<meta property="og:image:alt" content="{tr('Slay the GBA — écran titre sur Game Boy Advance', 'Slay the GBA — title screen on Game Boy Advance')}">
<meta name="twitter:card" content="summary_large_image">
<meta name="twitter:title" content="{esc(title)} — Slay the GBA">
<meta name="twitter:description" content="{esc(desc)}">
<meta name="twitter:image" content="{SITE_URL}/assets/og.png">
<link rel="apple-touch-icon" href="{up}assets/sprites/icon.png">"""
    target = path or "index.html"
    if translated and not noindex:  # each page points at its translation for search engines
        seo += (f'\n<link rel="alternate" hreflang="fr" href="{SITE_URL}/{path}">'
                f'\n<link rel="alternate" hreflang="en" href="{SITE_URL}/en/{target}">'
                + (f'\n<link rel="alternate" hreflang="x-default" href="{SITE_URL}/">' if not path else ""))
    if not translated:
        target = "index.html"
    if absolute:
        other = tr("/en/", "/") + target
    else:
        other = "../" * depth + tr("en/", "../") + target
    switch = tr(f'<span class="lang on" aria-current="true">FR</span><a class="lang" href="{other}" hreflang="en" lang="en" title="English version">EN</a>',
                f'<a class="lang" href="{other}" hreflang="fr" lang="fr" title="Version française">FR</a><span class="lang on" aria-current="true">EN</span>')
    switch = f'<div class="langs" role="group" aria-label="{tr("Langue", "Language")}">{switch}</div>'

    def link(href: str, fr: str, en_label: str) -> str:
        current = ' class="on" aria-current="page"' if href == active else ""
        return f'<a href="{up if href.startswith("bible/") else home}{href}"{current}>{esc(tr(fr, en_label))}</a>'

    # Wide screens: the brand is the way home and the compendium pages share a drop-down after "Héros".
    compendium = "".join(f"<li>{link(href, fr, en_label)}</li>" for href, fr, en_label, group in NAV if group == "compendium")
    in_compendium = any(href == active for href, _, _, group in NAV if group == "compendium")
    dropdown = (f'<li class="dd{" on" if in_compendium else ""}"><details><summary>Compendium</summary>'
                f'<ul class="win">{compendium}</ul></details></li>')
    links = "".join(f"<li>{link(href, fr, en_label)}</li>" + (dropdown if href == "heros.html" else "")
                    for href, fr, en_label, group in NAV if group != "compendium" and href != "index.html")
    # Phones: the START menu lists every page by group, and the dock keeps four shortcuts under the thumb.
    menu = "".join(
        f'<section><h2>{esc(tr(*NAV_GROUPS[group]))}</h2><ul>'
        + "".join(f"<li>{link(href, fr, en_label)}</li>" for href, fr, en_label, g in NAV if g == group)
        + "</ul></section>" for group in NAV_GROUPS)
    shortcuts = (("index.html", "home", tr("Accueil", "Home")), ("heros.html", "hero", tr("Héros", "Heroes")),
                 ("cartes.html", "cards", tr("Cartes", "Cards")), ("index.html#telecharger", "play", tr("Jouer", "Play")))
    dock = "".join(f'<a href="{home}{href}"{" aria-current=\"page\"" if href == active else ""}>{icon(ico)}<span>{label}</span></a>'
                   for href, ico, label in shortcuts)
    readable = (f'<button type="button" class="opt" data-readable aria-pressed="false">'
                f'<span aria-hidden="true">Aa</span> {tr("Texte lisible", "Readable text")}</button>')
    foot = tr(
        f"""<p>Slay the GBA — fan-game non officiel. <i>Slay the Spire</i> © Mega Crit. Moteur Butano © Gustavo Valiente.</p>
  <p>Données : bible du projet (patch v0.111.0 Beta). <a href="{home}mentions.html">Mentions légales &amp; confidentialité</a> · <a href="{ARTICLE_URL}" target="_blank" rel="noopener">Comment ce jeu a été vibe codé</a></p>""",
        f"""<p>Slay the GBA — unofficial fan game. <i>Slay the Spire</i> © Mega Crit. Butano engine © Gustavo Valiente.</p>
  <p>Data: project bible (patch v0.111.0 Beta). The game itself is in French. <a href="{home}mentions.html">Legal notice &amp; privacy</a> · <a href="{ARTICLE_URL_EN}" target="_blank" rel="noopener">How this game was vibe coded</a></p>""")
    tag = tr("Développeur produit &amp; Builder IA · Paris, France", "Product developer &amp; AI builder · Paris, France")
    return f"""<!doctype html>
<html lang="{LANG}">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{esc(title)} — Slay the GBA</title>
<meta name="description" content="{esc(desc)}">
{seo}
<link rel="preload" href="{up}assets/fonts/slay-gba.woff2" as="font" type="font/woff2" crossorigin>
<link rel="stylesheet" href="{up}assets/style.css">
<link rel="icon" href="{up}assets/sprites/icon.png">
<script>try{{if(localStorage.getItem("readable")==="1")document.documentElement.classList.add("readable")}}catch(e){{}}</script>
</head>
<body class="{theme}">
<a class="skip" href="#main">{tr("Aller au contenu", "Skip to content")}</a>
<header class="top">
  <a class="brand" href="{home}index.html"{' aria-current="page"' if active == "index.html" else ""}><img src="{up}assets/sprites/icon.png" alt="" width="32" height="32"><span>SLAY THE GBA</span></a>
  <nav class="nav" aria-label="{tr("Menu principal", "Main menu")}"><ul>{links}</ul></nav>
  {switch}
  <a class="btn small get" href="{home}index.html#telecharger">▼ {tr("Télécharger", "Download")}</a>
</header>
<dialog class="start-menu win" id="start-menu" aria-label="Menu">
  <div class="menu-head"><b>MENU</b><button type="button" class="close" data-close aria-label="{tr("Fermer le menu", "Close the menu")}">×</button></div>
  <nav aria-label="{tr("Toutes les pages", "Every page")}">{menu}</nav>
  <section><h2>Options</h2>{switch}{readable}</section>
</dialog>
<nav class="dock" aria-label="{tr("Raccourcis", "Shortcuts")}">{dock}<button type="button" data-menu aria-controls="start-menu" aria-expanded="false">{icon("menu")}<span>Menu</span></button></nav>
<main id="main">
{body}
</main>
<footer class="foot">
  {foot}
  <p class="foot-tools">{readable}<a class="btn small ghost" href="#main">▲ {tr("Haut de page", "Back to top")}</a></p>
  <div class="ae-footer">
    <a class="ae-logo" href="https://alexandre-enouf.fr/" target="_blank" rel="noopener">Alexandre <em>Enouf</em></a>
    <span class="ae-tag">{tag}</span>
    <nav class="ae-links" aria-label="{tr("Liens de l'auteur", "Author links")}">
      <a href="https://alexandre-enouf.fr/#lab" target="_blank" rel="noopener">⚡ Lab &amp; {tr("Projets", "Projects")}</a>
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
    name = card_name(card)
    desc = card_text(card)
    up = card_text(card, upgraded=True)
    keywords = " ".join(card.get("keywords") or [])
    return f"""<article class="card t-{kind.lower()} p-{card['pool'].lower()}" data-type="{esc(kind)}" data-rarity="{esc(card['rarity'])}" data-name="{esc(name.lower())} {esc(card['id'].lower())}">
  <div class="art"><img loading="lazy" src="{img_path(card['imagePath'], depth)}" alt="{esc(name)}"><span class="gem">{esc(cost_txt)}</span></div>
  <h4>{esc(name)}</h4>
  <p class="meta">{esc(type_label(kind))} · {esc(rarity_label(card['rarity']))}{' · ' + esc(keywords) if keywords and not en() else ''}</p>
  <p class="desc">{desc}</p>
  {f'<details><summary>{tr("Amélioré +", "Upgraded +")}</summary><p class="desc up">{up}</p></details>' if up and up != desc else ''}
</article>"""


def card_filters() -> str:
    types = "".join(f'<button data-filter-type="{t}">{type_label(t)}</button>' for t in ["Attack", "Skill", "Power"])
    rarities = "".join(
        f'<button data-filter-rarity="{r}">{rarity_label(r)}</button>' for r in ["Basic", "Common", "Uncommon", "Rare", "Ancient"]
    )
    return f"""<div class="filters" data-target=".cards">
  <input type="search" placeholder="{tr('Rechercher une carte…', 'Search a card…')}" data-search>
  <div class="chips"><button class="on" data-filter-type="">{tr('Tous types', 'All types')}</button>{types}</div>
  <div class="chips"><button class="on" data-filter-rarity="">{tr('Toutes raretés', 'All rarities')}</button>{rarities}</div>
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
    ("title", "Écran titre sur la key-art de Slay the Spire 2", "Title screen over the Slay the Spire 2 key art"),
    ("palette", "Choix de la teinte du héros (Classique, Chromatique, Ombre, Or)",
     "Picking the hero's palette (Classic, Chromatic, Shadow, Gold)"),
    ("map", "Carte de la Tour : 16 étages, embranchements et pion animé",
     "The Spire map: 16 floors, branching paths and an animated pawn"),
    ("combat", "Arène de combat façon Pokémon avec main de cartes illustrées",
     "Pokémon-style battle arena with an illustrated hand of cards"),
    ("card_flight", "La carte jouée grossit au centre avant l'impact", "The played card grows in the centre before impact"),
    ("impact", "Slash, dégâts flottants et barre de PV qui se vide", "Slash, floating damage and a draining HP bar"),
    ("enemy_turn", "Tour ennemi rejoué attaque par attaque", "The enemy turn, replayed attack by attack"),
    ("target", "Ciblage quand plusieurs ennemis sont en vie", "Targeting when several enemies are alive"),
    ("death", "Mort des monstres en fondu-mosaïque", "Monsters dying in a mosaic fade"),
    ("boss", "Boss : le Cerf Auguste", "Boss: the Ceremonial Beast"),
    ("rewards", "Récompenses : choisissez une carte parmi trois", "Rewards: pick one card out of three"),
    ("event", "Évènements mystères avec texte machine à écrire", "Mystery events with typewriter text"),
    ("event_choice", "Choix d'évènement et conséquences", "Event choices and their outcome"),
    ("shop", "Boutique du marchand", "The merchant's shop"),
    ("forge", "Feu de camp : forge avec aperçu amélioré", "Campfire: smithing with an upgrade preview"),
    ("deck", "Registre du deck", "The deck viewer"),
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

FAQ_EN = [
    ("Is the game in English?",
     "Not yet: every text in the game (cards, relics, events, menus) is in <b>French</b>. This English site gives the "
     "English names and texts of the cards, relics and potions, so you can look them up while you play, and the card "
     "art, numbers and icons make most of the game readable if you know Slay the Spire. An English version is possible "
     "if there is enough interest: tell me!"),
    ("Which edition should I start with?",
     "<b>The Ironclad</b>: 80 HP, a relic that heals after every fight and a straightforward deck. The Silent and the "
     "Defect ask for more planning; the Regent and the Necrobinder, new in Slay the Spire 2, are for seasoned players."),
    ("What changes from one cartridge to the next?",
     "The hero, their cards, starting relic, signature mechanic, colour, zone and palettes. The Spire, monsters, shared "
     "relics, potions, events and Ancients are the same in all five editions."),
    ("Is it free?",
     "Yes. The five ROMs are homebrew builds you can download for free. No commercial ROM is distributed."),
    ("Is my run saved?",
     "Yes, automatically on the cartridge (SRAM) at every room, with your volume settings. Pick <i>Continuer</i> on the "
     "title screen to resume."),
    ("Can I play on a real Game Boy Advance?",
     "Yes: copy the ROM to a flash cart (EZ-Flash, EverDrive) and play on GBA, GBA SP, Game Boy Micro or DS."),
    ("Is this an official game?",
     "No. Slay the GBA is an unofficial, non-commercial fan game with no ties to Mega Crit or Nintendo."),
]


def stars(level: int) -> str:
    return f'<span class="diff" title="{difficulty(level)}">{"★" * level}{"☆" * (3 - level)}</span>'


def cards_by_id(cards: list[dict]) -> dict[str, dict]:
    return {c["id"]: c for c in cards}


def starter_html(hero: Hero, by_id: dict[str, dict]) -> str:
    return ", ".join(f"{n}× {esc(card_name(by_id[cid]))}" for cid, n in hero.starter)


def relic_html(relic: dict, depth: int, cls: str = "relic") -> str:
    return (f'<div class="{cls}"><img loading="lazy" src="{img_path(relic["imagePath"], depth)}" alt="">'
            f'<div><b>{esc(entry_name(relic))}</b><p>{relic_text(relic)}</p></div></div>')


def edition_card(h: Hero, depth: int = 0) -> str:
    """Link card to an edition's page, from a page `depth` folders deep."""
    nav, up = "../" * depth, root(depth)
    return f"""<a class="cart" href="{nav}heros/{h.slug}.html" style="--hc:{h.color};--ha:{h.accent}">
  <div class="cart-label" style="background-image:url({up}assets/bg/battle_{h.slug}.png)">
    <span class="cart-ref">{esc(h.ref)}</span>
    <img class="pix" src="{up}assets/sprites/hero_{h.slug}.png" alt="{esc(hero_name(h))}">
  </div>
  <div class="cart-info"><b>{esc(hero_name(h))}</b><span>{esc(hero_text(h, 'edition'))}</span>
  <em>{quote(hero_text(h, 'tagline'))}</em>
  <small>{hp(h.hp)} · {esc(hero_text(h, 'mechanic_name'))} · {stars(h.difficulty)}</small></div>
</a>"""


def comparison_table(by_id: dict[str, dict], relics: dict[str, dict], depth: int = 0) -> str:
    nav, up = "../" * depth, root(depth)
    head = "".join(f'<th style="--hc:{h.color}"><a href="{nav}heros/{h.slug}.html"><img class="pix" src="{up}assets/sprites/hero_{h.slug}.png" alt=""><br>{esc(hero_name(h))}</a></th>' for h in HEROES)
    rows = [
        (tr("Cartouche", "Cartridge"), lambda h: f"<code>{esc(h.rom)}</code>"),
        (tr("Couleur", "Colour"), lambda h: f'<span class="swatch" style="background:{h.color}"></span> {esc(hero_text(h, "color_name"))}'),
        ("Zone", lambda h: esc(hero_text(h, "zone"))),
        (tr("Points de vie", "Hit points"), lambda h: f"<b>{h.hp}</b> {tr('PV', 'HP')}"),
        (tr("Relique de départ", "Starting relic"), lambda h: esc(entry_name(relics[h.relic_id]))),
        (tr("Deck de départ", "Starting deck"), lambda h: starter_html(h, by_id)),
        (tr("Mécanique", "Mechanic"), lambda h: esc(hero_text(h, "mechanic_name"))),
        (tr("Difficulté", "Difficulty"), lambda h: f"{stars(h.difficulty)} {difficulty(h.difficulty)}"),
    ]
    body = "".join(f"<tr><th>{label}</th>{''.join(f'<td>{fn(h)}</td>' for h in HEROES)}</tr>" for label, fn in rows)
    return f'<div class="table-scroll"><table class="compare"><tr><th></th>{head}</tr>{body}</table></div>'


def controls_table() -> str:
    rows = "".join(f"<tr><td>{esc(s)}</td><td><b>{esc(b)}</b></td><td>{esc(a)}</td></tr>"
                   for s, b, a in (site_en.CONTROLS if en() else CONTROLS))
    return (f'<table class="keys"><tr><th>{tr("Écran", "Screen")}</th><th>{tr("Bouton", "Button")}</th>'
            f'<th>Action</th></tr>{rows}</table>')


LANDING_COLUMNS = (
    (progress.DONE, "Fait", "Done", "#68e868", "✓"),
    (progress.PARTIAL, "En cours", "In progress", "#f8d048", "◐"),
    (progress.TODO, "À venir", "Coming up", "#9a98b0", "○"),
)
JOURNAL_SECTIONS = ("systemes", "interface", "regles")  # hand-written checklists of avancement.toml, in priority order
JOURNAL_LIMIT = 10
BIBLE_MONSTERS = (("mon_cultist", "Cultist", "Cultist"), ("mon_jaw_worm", "Jaw Worm", "Jaw Worm"),
                  ("mon_louse", "Morpion", "Louse"), ("mon_gremlin_nob", "Gremlin Nob", "Gremlin Nob"))
DEFAULT_EDITION = HEROES[0].slug


def progress_color(percent: float) -> str:
    """Bar colour of a section: the closer to done, the greener."""
    return next((color for floor, color in ((85, "#68e868"), (55, "#f8d048"), (25, "#f0a060")) if percent >= floor),
                "#f85848")


def tracker(text: str) -> str:
    """A text of the progress tracker (avancement.toml, progress.py) in the current language."""
    return TRACKER.get(text, text) if en() else text


def item_name(item: progress.Item) -> str:
    if not en():
        return item.name
    if item.key != item.name:  # an entry of bible/data, named after its id
        return EN_NAMES.get(item.key) or id_name(item.key)
    return TRACKER.get(item.name, item.name)


def item_detail(item: progress.Item) -> str:
    if not en() or not item.detail:
        return item.detail
    return " · ".join(DETAIL_PARTS.get(part, part) for part in item.detail.split(" · "))


def extras_text(prog: progress.Progress) -> str:
    if not en():
        return progress.extras_fr(prog)
    labels = {"Cartes": "cards", "Monstres": "monsters", "Évènements": "events"}
    groups = "; ".join(f"{labels.get(label, label.lower())} {', '.join(names)}" for label, names in prog.extras if names)
    return f"Already playable but missing from the Slay the Spire 2 reference data, so not counted: {groups}."


def hero_mechanics(section: progress.Section, limit: int = 3) -> list[str]:
    """Names of the signature mechanics already in the game, from the hero's "Mécaniques" checklist."""
    return [item_name(item) for category in section.categories if category.title == "Mécaniques"
            for item in category.items if item.status == progress.DONE][:limit]


def journal_items(prog: progress.Progress, status: str) -> list[progress.Item]:
    """Checklist items of one status for the journal: game systems and interface before the combat rules."""
    items = [item for slug in JOURNAL_SECTIONS for category in prog.section(slug).categories
             for item in category.items if item.status == status]
    return items[:JOURNAL_LIMIT]


def counters(tally: progress.Tally) -> str:
    return (f'<span class="cnt cnt-done">{tally.done} {tr("faits", "done")}</span>'
            f'<span class="cnt cnt-partial">{tally.partial} {tr("partiels", "partial")}</span>'
            f'<span class="cnt cnt-todo">{tally.todo} {tr("à faire", "to do")}</span>')


def launch_sprites() -> str:
    """Hero picker of the banner: one sprite button per edition, which also shows its battle on the console."""
    return "".join(
        f'<button type="button" class="pick" data-edition="{h.slug}" data-shot="{root()}assets/screens/{h.screenshot}.png" '
        f'style="--hc:{h.color};--ha:{h.accent}" '
        f'title="{esc(hero_name(h))}" aria-pressed="{"true" if h.slug == DEFAULT_EDITION else "false"}">'
        f'<img class="pix" src="{root()}assets/sprites/hero_{h.slug}.png" alt="{esc(hero_name(h))}"></button>'
        for h in HEROES
    )


ATTRACT_SCREENS = ("title", "map", "combat", "card_flight", "impact", "shop", "rewards")  # the console's demo loop


def console() -> str:
    """A handheld drawn in CSS, in the colour of the picked edition, playing real captures of the ROM. It wears the
    game's name, not a console maker's logo."""
    up = root()
    shots = ",".join(f"{up}assets/screens/{name}.png" for name in ATTRACT_SCREENS)
    alt = tr("Capture réelle de la ROM", "Real capture of the ROM")
    return f"""<div class="console">
    <span class="shoulder l" aria-hidden="true"></span><span class="shoulder r" aria-hidden="true"></span>
    <div class="pad" aria-hidden="true"><span class="dpad"></span><span class="ss">SELECT</span></div>
    <div class="bezel">
      <p class="bezel-name" aria-hidden="true"><span class="led"></span>SLAY THE GBA</p>
      <div class="lcd"><img class="pix" id="console-shot" src="{up}assets/screens/title.png" width="240" height="160" alt="{alt}" data-cycle="{shots}"></div>
    </div>
    <div class="pad" aria-hidden="true"><span class="ab"><i>B</i><i>A</i></span><span class="ss">START</span></div>
  </div>"""


def edition_tab(h: Hero) -> str:
    return (f'<button type="button" class="cart tab" data-edition="{h.slug}" style="--hc:{h.color};--ha:{h.accent}" '
            f'aria-pressed="{"true" if h.slug == DEFAULT_EDITION else "false"}">'
            f'<span class="cart-ref">{esc(h.ref)}</span>'
            f'<img class="pix" src="{root()}assets/sprites/hero_{h.slug}.png" alt=""><b>{esc(hero_name(h))}</b></button>')


def edition_showcase(h: Hero, prog: progress.Progress, relic_by_id: dict[str, dict], by_id: dict[str, dict]) -> str:
    """The big panel of one edition. All five are rendered; the script only toggles `hidden`."""
    section = prog.section(h.slug)
    name = hero_name(h)
    mechanics = "".join(f"<li>{esc(mechanic)}</li>" for mechanic in hero_mechanics(section))
    facts = [(tr("Points de vie", "Hit points"), hp(h.hp)),
             (tr("Relique de départ", "Starting relic"), esc(entry_name(relic_by_id[h.relic_id]))),
             (tr("Mécanique", "Mechanic"), esc(hero_text(h, "mechanic_name"))),
             (tr("Difficulté", "Difficulty"), f"{stars(h.difficulty)} {difficulty(h.difficulty)}"),
             ("Zone", esc(hero_text(h, "zone"))), (tr("Deck de départ", "Starting deck"), starter_html(h, by_id))]
    tiles = "".join(f"<div><span>{k}</span><b>{v}</b></div>" for k, v in facts)
    hidden = "" if h.slug == DEFAULT_EDITION else " hidden"
    up = root()
    kicker = tr(f"Édition {esc(name)} · {esc(h.color_name)}",
                f"{esc(hero_text(h, 'edition'))} · {esc(hero_text(h, 'color_name'))}")
    return f"""<div class="edition-panel" data-panel="{h.slug}" style="--hc:{h.color};--ha:{h.accent}"{hidden}>
  <div class="ed-shot">
    <div class="shot-frame"><img class="pix" loading="lazy" src="{up}assets/screens/{h.screenshot}.png" alt="{tr('Combat de', 'Battle of')} {esc(name)}"><span class="rom-tag">{esc(h.rom)}</span></div>
    <p class="meta">{tr('Capture réelle', 'Real capture')} · {esc(hero_text(h, 'zone'))}</p>
  </div>
  <div class="ed-body">
    <div class="ed-head"><img class="pix" src="{up}assets/sprites/hero_{h.slug}.png" alt="">
      <div><span class="ed-kicker">{kicker}</span><h3>{esc(name)}</h3></div></div>
    <p class="ed-quote">{quote(hero_text(h, 'tagline'))}</p>
    <ul class="ed-sig">{mechanics}</ul>
    <div class="ed-facts">{tiles}</div>
    <div class="ed-prog"><span class="meta">{tr("Portage de l'édition", "Port of the edition")}</span><b>{percent(section.tally.percent)}</b>
      {meter(section.tally, tr(f"Avancement de l'édition {name}", f"Progress of the {name} edition"), h.color)}
      <span class="meta">{esc(counts(section.tally))}</span></div>
    <div class="cta">{download_button(h, 0)}<a class="btn ghost" href="heros/{h.slug}.html">{tr('Toutes ses cartes ►', 'Every card ►')}</a></div>
  </div>
</div>"""


def landing_progress(prog: progress.Progress) -> str:
    version = progress.current_version()
    release = f"Version {esc(version)} · " if version else ""
    tiles = "".join(
        f'<a class="sect" href="avancement.html#{s.slug}"><span>{esc(tracker(s.title))}<b>{percent(s.tally.percent)}</b></span>'
        f'{meter(s.tally, tracker(s.title), progress_color(s.tally.percent))}</a>'
        for s in prog.sections
    )
    intro = tr("""Le jeu se construit carte après carte. Voici la part du contenu de Slay the Spire 2 déjà
implémentée dans le code du jeu, recalculée à chaque mise à jour du site.""",
               """The game was built card after card. Here is the share of Slay the Spire 2's content already
implemented in the game code, recomputed on every update of the site.""")
    return panel(f"""<h2 id="avancement">{tr("Où en est le portage ?", "How far along is the port?")}</h2>
<p class="intro">{intro}</p>
<div class="prog-total"><b>{percent(prog.percent)}</b><div>{meter(prog.tally, tr("Avancement global", "Overall progress"), percent=prog.percent)}
<p class="meta">{release}{tr("global", "overall")}</p><p class="counters">{counters(prog.tally)}</p><p class="meta">{global_rule()}</p></div></div>
<div class="sect-grid">{tiles}</div>
<p><a class="btn small" href="avancement.html">{tr("Voir tout le détail ►", "See every detail ►")}</a></p>""")


def landing_journal(prog: progress.Progress) -> str:
    columns = ""
    for status, label_fr, label_en, color, icon in LANDING_COLUMNS:
        rows = "".join(
            f'<li><span class="ico">{icon}</span><span>{esc(item_name(item))}'
            + (f' <a href="{progress.ISSUE_URL}{item.issue}" target="_blank" rel="noopener">#{item.issue}</a>'
               if item.issue else "") + "</span></li>"
            for item in journal_items(prog, status)
        )
        empty = f'<li><span class=meta>{tr("Rien pour le moment.", "Nothing for now.")}</span></li>'
        columns += f'<div class="jcard" style="--jc:{color}"><h3>{tr(label_fr, label_en)}</h3><ul>{rows or empty}</ul></div>'
    return panel(f"""<h2 id="fait">{tr("Journal de bord", "Logbook")}</h2>
<p class="intro">{tr("Ce qui est fait, ce qui avance et ce qui arrive, tiré du suivi d'avancement du projet.",
                     "What is done, what is moving and what is coming, from the project's progress tracker.")}</p>
<div class="journal">{columns}</div>""")


def landing_captures() -> str:
    up = root()
    first, caption_fr, caption_en = SCREENS[0]
    thumbs = "".join(
        f'<a class="thumb" href="{up}assets/screens/{n}.png" data-caption="{esc(tr(fr, en_text))}"><img class="pix" loading="lazy" '
        f'src="{up}assets/screens/{n}.png" alt="{esc(tr(fr, en_text))}"></a>' for n, fr, en_text in SCREENS
    )
    caption = tr(caption_fr, caption_en)
    return panel(f"""<h2 id="captures">{tr("Captures", "Screenshots")}</h2><p>{tr("Images réelles de la ROM, capturées dans l'émulateur.", "Real images of the ROM, captured in an emulator.")}</p>
<div class="gallery"><figure class="gallery-main"><img class="pix" id="gallery-main" src="{up}assets/screens/{first}.png" alt="{esc(caption)}">
<figcaption id="gallery-caption">{esc(caption)}</figcaption></figure><div class="thumbs">{thumbs}</div></div>""")


def bible_chapters() -> list[tuple[str, str]]:
    """(page stem, title) of the numbered bible tomes, in reading order."""
    return [(path.stem, bible_title(path)) for path in sorted(BIBLE.glob("[0-9]*.md"))]


def landing_bible(monsters_html: str) -> str:
    up = root()
    chapters = "".join(
        f'<a href="{up}bible/{stem}.html"><span>{stem.split("_")[0]}</span>{esc(title)}</a>' for stem, title in bible_chapters())
    livrets = len(list((BIBLE / "livrets_gba").glob("livret_*.md")))
    intro = tr("Chaque carte, monstre, relique et règle du jeu est documentée dans la bible de conception, d'où le site tire ses données.",
               "Every card, monster, relic and rule of the game is documented in the design bible, where this site gets "
               "its data. The bible is written in French.")
    return panel(f"""<h2 id="bible">{tr("La bible du projet", "The project bible (in French)")}</h2>
<p class="intro">{intro}</p>
<div class="mini-mons">{monsters_html}</div>
<div class="chapters">{chapters}</div>
<p><a class="btn small" href="{up}bible/livrets.html">×{livrets} {tr("Livrets d'instructions", "Instruction booklets")} ►</a> <a class="btn small ghost" href="{up}bible/index.html">{tr("Toute la bible", "The whole bible")} ►</a></p>""")


def testers_panel(prog: progress.Progress) -> str:
    """Call for playtesters, shown on both landings: where to report a bug or give feedback."""
    done = prog.percent >= 100
    if en():
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
    anchor = tr("testeurs", "testers")
    return panel(f'<h2 id="{anchor}">🎉 {title}</h2><p>{text}</p><div class="cta">{links}</div>', "callout")


LANGUAGE_NOTICE_EN = (
    '<h2 id="language">🇫🇷 The game is in French</h2><p>All in-game text (cards, relics, events, menus) is in '
    '<b>French</b>. This English site gives the English names and texts of Slay the Spire 2 for every '
    '<a href="cartes.html">card</a>, <a href="reliques.html">relic</a> and <a href="potions.html">potion</a>, so you '
    'can look them up while you play; event stories and the design bible stay in French. If you know Slay the Spire, '
    'the card art, numbers and intent icons carry you a long way.</p>')

STEPS = ("""<ol class="steps">
<li><b>Téléchargez la cartouche</b> de votre héros (fichier <code>.gba</code>) dans la <a href="#telecharger">section Télécharger</a> ou sur la page du héros.</li>
<li><b>Installez un émulateur Game Boy Advance</b> gratuit :
  <ul>
    <li>Ordinateur (Windows, macOS, Linux) : <a href="https://mgba.io/downloads.html" target="_blank" rel="noopener">mGBA</a> (recommandé).</li>
    <li>iPhone / iPad : <a href="https://apps.apple.com/app/delta-game-emulator/id1048524688" target="_blank" rel="noopener">Delta</a>.</li>
    <li>Android : <a href="https://play.google.com/store/apps/details?id=com.fastemulator.gba" target="_blank" rel="noopener">My Boy!</a> ou <a href="https://www.retroarch.com/" target="_blank" rel="noopener">RetroArch</a>.</li>
  </ul></li>
<li><b>Ouvrez le fichier <code>.gba</code></b> dans l'émulateur (Fichier → Charger une ROM, ou importez-le dans l'app), puis appuyez sur START.</li>
<li>Vous avez une vraie console ? Copiez la ROM sur une cartouche flash (EZ-Flash, EverDrive) et jouez sur GBA, GBA SP ou DS.</li>
</ol>""", """<ol class="steps">
<li><b>Download your hero's cartridge</b> (a <code>.gba</code> file) from the <a href="#telecharger">Download section</a> or the hero's page.</li>
<li><b>Install a free Game Boy Advance emulator</b>:
  <ul>
    <li>Computer (Windows, macOS, Linux): <a href="https://mgba.io/downloads.html" target="_blank" rel="noopener">mGBA</a> (recommended).</li>
    <li>iPhone / iPad: <a href="https://apps.apple.com/app/delta-game-emulator/id1048524688" target="_blank" rel="noopener">Delta</a>.</li>
    <li>Android: <a href="https://play.google.com/store/apps/details?id=com.fastemulator.gba" target="_blank" rel="noopener">My Boy!</a> or <a href="https://www.retroarch.com/" target="_blank" rel="noopener">RetroArch</a>.</li>
  </ul></li>
<li><b>Open the <code>.gba</code> file</b> in the emulator (File → Load ROM, or import it in the app), then press START.</li>
<li>Got real hardware? Copy the ROM to a flash cart (EZ-Flash, EverDrive) and play on GBA, GBA SP or DS.</li>
</ol>""")


def build_index(cards: list[dict], monsters: list[dict], relics: list[dict], potions: list[dict], events: list[dict],
                prog: progress.Progress) -> None:
    by_id = cards_by_id(cards)
    relic_by_id = {r["id"]: r for r in relics}
    up = root()
    downloads = "".join(
        f'<div class="download" style="--hc:{h.color}"><img class="pix" src="{up}assets/sprites/hero_{h.slug}.png" alt=""><div><b>{esc(hero_name(h))}</b><span class="meta">{esc(hero_text(h, "edition"))} · {esc(hero_text(h, "color_name"))}</span>{download_button(h, 0, "btn small")}</div></div>'
        for h in HEROES
    )
    pool_size = len([c for c in cards if c["pool"] == "Ironclad"])
    stats = [(5, tr("éditions", "editions")), (len(cards), tr("cartes", "cards")),
             (len([m for m in monsters if m.get('acts')]), tr("monstres", "monsters")),
             (len(relics), tr("reliques", "relics")), (len(potions), "potions"),
             (len(events), tr("évènements", "events")), (8, tr("Anciens", "Ancients"))]
    stat_html = "".join(f"<div><b>{n}</b><span>{esc(l)}</span></div>" for n, l in stats)
    boss = tr("Cerf Auguste", "Ceremonial Beast")
    bestiary = "".join(
        f'<div class="mini-mon"><img class="pix" src="{up}assets/sprites/{s}.png" alt="{esc(tr(fr, en_name))}"><span>{esc(tr(fr, en_name))}</span></div>'
        for s, fr, en_name in BIBLE_MONSTERS
    ) + f'<div class="mini-mon boss"><img class="pix" src="{up}assets/sprites/boss_ceremonial_beast.png" alt="{boss}"><span>{boss} (Boss)</span></div>'
    faq = "".join(f"<details class='faq'><summary>{esc(q)}</summary><p>{a}</p></details>" for q, a in (FAQ_EN if en() else FAQ))
    version = progress.current_version()
    badge = f'<span class="ver">v{esc(version)}</span>' if version else ""
    tabs = "".join(edition_tab(h) for h in HEROES)
    showcases = "".join(edition_showcase(h, prog, relic_by_id, by_id) for h in HEROES)
    common = tr(f'{GOLD} pièces d’or au départ, {ENERGY} énergie par tour, {POTION_SLOTS} emplacements de potion, 4 teintes, Ascension 0 à 10.',
                f'{GOLD} gold to start, {ENERGY} energy per turn, {POTION_SLOTS} potion slots, 4 palettes, Ascension 0 to 10.')
    editions_intro = tr(f'Comme les grandes séries portables vendues en plusieurs versions, Slay the GBA existe en <b>cinq cartouches</b>. Chacune a son héros, son deck de {pool_size}+ cartes, sa relique, sa couleur, sa zone et sa mécanique signature.',
                        f'Like the classic handheld series sold in several versions, Slay the GBA comes as <b>five cartridges</b>. Each one has its hero, a deck of {pool_size}+ cards, a starting relic, a colour, a zone and a signature mechanic.')
    editions = (f'<h2 id="editions">{tr("Choisissez votre édition", "Pick your edition")}</h2><p class="intro">{editions_intro}</p>'
                f'<div class="tabs">{tabs}</div>{showcases}<p class="meta">{tr("Toutes les éditions : ", "Every edition: ")}{common}</p>'
                + f'<details class="faq"><summary>{tr("Comparer les cinq éditions", "Compare the five editions")}</summary>{comparison_table(by_id, relic_by_id)}</details>'
                + f'<details class="faq"><summary>{tr("Commandes (identiques sur les cinq cartouches)", "Controls (the same on all five cartridges)")}</summary>{controls_table()}</details>')
    download = tr('<h2 id="telecharger">Télécharger les cartouches</h2><p>Une ROM <code>.gba</code> par héros, gratuite, à ouvrir dans un émulateur (mGBA, Delta sur iPhone, RetroArch…) ou à flasher sur une cartouche EZ-Flash / EverDrive.</p>',
                  '<h2 id="telecharger">Download the cartridges</h2><p>One free <code>.gba</code> ROM per hero, to open in an emulator (mGBA, Delta on iPhone, RetroArch…) or to flash on an EZ-Flash / EverDrive cart. The game is in French.</p>')
    banner = tr(f"""<p class="kicker">{badge}<span>Game Boy Advance · Deckbuilder roguelike · 5 éditions</span></p>
    <h1>SLAY<br>THE GBA</h1>
    <p class="lead"><b>Slay the Spire 2 dans votre poche.</b> Cinq héros, cinq cartouches, une seule Tour à gravir — en 240×160, avec des combats façon Pokémon et les vraies illustrations du jeu.</p>
    <div class="cta"><a class="btn big" href="#telecharger">▼ Télécharger gratuitement</a><a class="btn ghost big" href="#editions">Choisir mon édition</a><span class="press-start">▶ PRESS START</span></div>
    <p class="pick-label">Choisissez votre héros</p>""", f"""<p class="kicker">{badge}<span>Game Boy Advance · Roguelike deckbuilder · 5 editions</span></p>
    <h1>SLAY<br>THE GBA</h1>
    <p class="lead"><b>Slay the Spire 2 in your pocket.</b> Five heroes, five cartridges, one Spire to climb, in 240×160 with Pokémon-style battles and the game's real card art.</p>
    <div class="cta"><a class="btn big" href="#telecharger">▼ Download for free</a><a class="btn ghost big" href="#editions">Pick my edition</a><span class="press-start">▶ PRESS START</span></div>
    <p class="pick-label">Choose your hero</p>""")
    body = f"""
<section class="title-screen">
  <div class="ts-text">
    {banner}
    <div class="picks">{launch_sprites()}</div>
  </div>
  {console()}
</section>

<div class="stats">{stat_html}</div>

{panel(LANGUAGE_NOTICE_EN, "callout") if en() else ""}

{testers_panel(prog)}

{panel(editions)}

{landing_progress(prog)}

{landing_journal(prog)}

{landing_captures()}

{landing_bible(bestiary)}

{panel(download + '<div class="downloads">' + downloads + '</div>')}

{panel(f'<h2 id="jouer">{tr("Jouer en 3 minutes", "Play in 3 minutes")}</h2>' + tr(*STEPS))}

{panel(f'<h2 id="faq">{tr("Questions fréquentes", "FAQ")}</h2>' + faq + f'<p><a class="btn small ghost" href="{tr(ARTICLE_URL, ARTICLE_URL_EN)}" target="_blank" rel="noopener">📖 {tr("L’histoire du projet", "The story of the project")}</a></p>')}
"""
    write_page("index.html", page(
        tr("Accueil", "Slay the Spire 2 on Game Boy Advance"), body, active="index.html", path="",
        theme=f"hero-{DEFAULT_EDITION}",
        description=tr("Slay the GBA : Slay the Spire 2 réinventé sur Game Boy Advance. Cinq héros, cinq cartouches gratuites, combats façon Pokémon.",
                       "Slay the GBA: a free Slay the Spire 2 demake for Game Boy Advance. Five heroes, five cartridges, "
                       "Pokémon-style battles. In French, playtesters wanted.")))


def build_heroes_index(cards: list[dict], relics: list[dict]) -> None:
    by_id = cards_by_id(cards)
    relic_by_id = {r["id"]: r for r in relics}
    rows = "".join(
        f"""<a class="hero-row" href="heros/{h.slug}.html" style="--hc:{h.color};--ha:{h.accent}">
  <img class="pix big" src="{root()}assets/sprites/hero_{h.slug}.png" alt="">
  <div><p class="meta">{esc(hero_text(h, 'edition'))} · {esc(hero_text(h, 'color_name'))} · {esc(hero_text(h, 'zone'))}</p><h3>{esc(hero_name(h))} <small>— {esc(hero_text(h, 'title'))}</small></h3>
  <p class="tagline">{quote(hero_text(h, 'tagline'))}</p><p>{esc(hero_text(h, 'pitch'))}</p>
  <p class="meta">{hp(h.hp)} · {tr('Relique : ', 'Relic: ')}{esc(entry_name(relic_by_id[h.relic_id]))} · {esc(hero_text(h, 'mechanic_name'))} · {stars(h.difficulty)} {difficulty(h.difficulty)}</p></div>
</a>"""
        for h in HEROES
    )
    body = (panel(tr("<h1>Les 5 éditions</h1><p class='intro'>Un héros par cartouche. Même Tour, cinq façons de la gravir.</p>",
                     "<h1>The 5 editions</h1><p class='intro'>One hero per cartridge. Same Spire, five ways to climb it.</p>") + rows)
            + panel(f"<h2>{tr('Comparer les éditions', 'Compare the editions')}</h2>" + comparison_table(by_id, relic_by_id)))
    write_page("heros.html", page(
        tr("Héros", "Heroes"), body, active="heros.html", path="heros.html",
        description=tr("Les 5 éditions de Slay the GBA : Soldat de Fer, La Silencieuse, Le Défectueux, Le Régent et La Nécrolieuse.",
                       "The 5 editions of Slay the GBA: the Ironclad, the Silent, the Defect, the Regent and the Necrobinder.")))


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
    by_id = cards_by_id(cards)
    relic_by_id = {r["id"]: r for r in relics}
    order = {"Basic": 0, "Common": 1, "Uncommon": 2, "Rare": 3, "Ancient": 4}
    up = root(1)
    for i, h in enumerate(HEROES):
        name = hero_name(h)
        pool = sorted((c for c in cards if c["pool"] == h.pool), key=lambda c: (order.get(c["rarity"], 9), card_name(c)))
        hero_relics = sorted((r for r in relics if r["pool"] == h.pool), key=entry_name)
        tally = {t: sum(1 for c in pool if c["type"] == t) for t in ["Attack", "Skill", "Power"]}
        key_cards = "".join(card_html(by_id[cid], 1) for cid in h.key_cards)
        starter = "".join(
            f'<li><b>{n}×</b> {esc(card_name(by_id[cid]))} <span class="meta">— {card_text(by_id[cid])}</span></li>'
            for cid, n in h.starter)
        palette_texts = HERO_TEXT[h.slug]["palettes"] if en() else [(p.name, p.description) for p in h.palettes]
        palettes = "".join(
            f'<div class="palette"><span class="swatch big" style="background:{p.hex}"></span><div><b>{esc(p_name)}</b><p>{esc(p_desc)}</p></div></div>'
            for p, (p_name, p_desc) in zip(h.palettes, palette_texts))
        tips = "".join(f"<li>{re.sub(r'[*][*](.*?)[*][*]', r'<b>\1</b>', esc(t))}</li>" for t in hero_text(h, "tips"))
        prev_h, next_h = HEROES[i - 1], HEROES[(i + 1) % len(HEROES)]
        others = "".join(edition_card(o, 1) for o in HEROES if o is not h)
        zone, color_name = esc(hero_text(h, "zone")), esc(hero_text(h, "color_name"))
        kicker = tr(f"{esc(h.edition)} · Cartouche {esc(h.rom)}",
                    f"{esc(hero_text(h, 'edition'))} · Cartridge {esc(h.rom)} · In game: {esc(h.name)}")
        stats = [(h.hp, tr("PV", "HP")), (ENERGY, tr("énergie", "energy")), (GOLD, tr("or", "gold")),
                 (len(pool), tr("cartes", "cards")), (tally["Attack"], tr("attaques", "attacks")),
                 (tally["Skill"], tr("compétences", "skills")), (tally["Power"], tr("pouvoirs", "powers")),
                 ("★" * h.difficulty, difficulty(h.difficulty).lower())]
        stat_html = "".join(f"<div><b>{n}</b><span>{esc(label)}</span></div>" for n, label in stats)
        booklet = f'{up}bible/livret_{h.slug}.html'
        story = tr(f"""<p><b>Idéal pour</b> {esc(h.ideal)}</p>
<p class="meta">Zone : {zone} · Couleur de cartouche : <span class="swatch" style="background:{h.color}"></span> {color_name}</p>""",
                   f"""<p><b>Ideal for</b> {esc(hero_text(h, 'ideal'))}</p>
<p class="meta">Zone: {zone} · Cartridge colour: <span class="swatch" style="background:{h.color}"></span> {color_name}</p>""")
        body = f"""
<section class="hero-banner small" style="background-image:url({up}assets/bg/battle_{h.slug}.png)">
  <div class="hero-inner split">
    <img class="pix giant" src="{up}assets/sprites/hero_{h.slug}.png" alt="{esc(name)}">
    <div><p class="kicker">{kicker}</p><h1>{esc(name)}</h1><p class="lead">{quote(hero_text(h, 'tagline'))}</p>
    <p>{esc(hero_text(h, 'pitch'))}</p><div class="cta">{download_button(h, 1)}<a class="btn ghost" href="{booklet}">{tr("Lire le livret", "Read the booklet (FR)")}</a></div></div>
  </div>
</section>
<div class="stats">{stat_html}</div>
{panel(f'''<div class="story"><div><h2>{esc(hero_text(h, 'title'))}</h2><p>{esc(hero_text(h, 'lore'))}</p>{story}</div>
<figure class="shot"><img class="pix" src="{up}assets/screens/{h.screenshot}.png" alt="{tr('Combat de', 'Battle of')} {esc(name)}"><figcaption>{tr(f"{esc(name)} dans l'arène — capture réelle de la ROM", f"{esc(name)} in the arena, a real capture of the ROM")}</figcaption></figure></div>''')}
{panel(f'<h2>{tr("Mécanique signature", "Signature mechanic")}{tr(" : ", ": ")}{esc(hero_text(h, "mechanic_name"))}</h2><p>{esc(hero_text(h, "mechanic_text"))}</p><h3>{tr("Cartes phares", "Key cards")}</h3><div class="cards">{key_cards}</div>')}
{panel(f'''<h2>{tr("Fiche de départ", "Starting kit")}</h2><div class="sheet">
{relic_html(relic_by_id[h.relic_id], 1, "relic big")}
<div><h3>{tr("Deck de départ", "Starting deck")} ({sum(n for _, n in h.starter)} {tr("cartes", "cards")})</h3><ul class="starter">{starter}</ul>
<p class="meta">{tr(f"{GOLD} pièces d'or · {ENERGY} énergie par tour · {POTION_SLOTS} emplacements de potion", f"{GOLD} gold · {ENERGY} energy per turn · {POTION_SLOTS} potion slots")}</p></div>
<img class="pix energy" src="{up}assets/sprites/energy_{h.slug}.png" alt="{tr("Orbe d'énergie", "Energy orb")}">
</div>''')}
{panel(f'<h2>{tr("Les 4 teintes", "The 4 palettes")}</h2><p>{tr(f"Choisissez l’apparence de {esc(name)} au lancement de chaque partie.", f"Choose how {esc(name)} looks at the start of every run.")}</p><div class="palettes">{palettes}</div>')}
{panel(f'<h2>{tr("Conseils de survie", "Survival tips")}</h2><ol class="tips">{tips}</ol><p><a class="btn small" href="{booklet}">{tr("Livret complet de l’édition ►", "The edition booklet (FR) ►")}</a> <a class="btn small ghost" href="{up}bible/{h.tome}.html">{tr("Tome des cartes ►", "Card tome (FR) ►")}</a></p>')}
{hero_progress_panel(prog.section(h.slug))}
<section class="panel"><h2>{tr("Toutes les cartes", "Every card")} ({len(pool)})</h2>{card_filters()}<div class="cards">{''.join(card_html(c, 1) for c in pool)}</div></section>
{panel(f'<h2>{tr("Reliques exclusives", "Exclusive relics")}</h2><div class="relics">' + ''.join(relic_html(r, 1) for r in hero_relics) + '</div>')}
{panel(f'<h2>{tr("Les autres éditions", "The other editions")}</h2><p class="pager"><a href="{prev_h.slug}.html">◄ {esc(hero_name(prev_h))}</a><a href="{next_h.slug}.html">{esc(hero_name(next_h))} ►</a></p><div class="carts">{others}</div>')}
"""
        description = tr(f"{h.edition} de Slay the GBA — {h.tagline} {h.mechanic} Toutes ses cartes et la ROM {h.rom}.",
                         f"{hero_text(h, 'edition')} of Slay the GBA: {hero_text(h, 'tagline')} "
                         f"{hero_text(h, 'mechanic')} Every card and the {h.rom} ROM.")
        write_page(f"heros/{h.slug}.html", page(name, body, depth=1, active="heros.html", theme=f"hero-{h.slug}",
                                                 description=description, path=f"heros/{h.slug}.html"))


def card_pools() -> list[tuple[str, str]]:
    return ([(h.pool, hero_name(h)) for h in HEROES]
            + [(p, tr(POOL_FR[p], POOL_EN[p])) for p in ["Colorless", "Event", "Curse", "Quest"]])


def build_cards_page(cards: list[dict]) -> None:
    sections = ""
    for pool, label in card_pools():
        subset = sorted([c for c in cards if c["pool"] == pool], key=card_name)
        sections += f'<h2 class="pool-title" id="{pool.lower()}">{esc(label)} <small>({len(subset)})</small></h2><div class="cards">' + "".join(card_html(c, 0) for c in subset) + "</div>"
    jump = " ".join(f'<a href="#{p.lower()}">{esc(l)}</a>' for p, l in card_pools())
    note = tr("", "<p class='meta'>English names and texts of Slay the Spire 2; the game shows them in French.</p>")
    body = panel(f"<h1>{tr('Toutes les cartes', 'Every card')}</h1>{note}<p class='jump'>{jump}</p>{card_filters()}{sections}")
    write_page("cartes.html", page(
        tr("Cartes", "Cards"), body, active="cartes.html", path="cartes.html",
        description=tr(f"Les {len(cards)} cartes de Slay the Spire 2 : filtres par héros, type et rareté.",
                       f"The {len(cards)} cards of Slay the Spire 2, with filters by hero, type and rarity.")))


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
                    bits.append(f'{mv["damage"]}{"×" + str(mv["hits"]) if mv.get("hits", 1) > 1 else ""} {tr("dégâts", "damage")}')
                if mv.get("block"):
                    bits.append(f'{mv["block"]} {tr("armure", "Block")}')
                for eff in mv.get("effects", []) or []:
                    kind = eff.get("type", "")
                    effect = tr(effect_names.get(kind, kind), kind)
                    amt = eff.get("amount")
                    bits.append(f'{esc(effect)}{" " + str(amt) if amt else ""}')
                intents = ", ".join(tr(INTENT_FR.get(i, i), i.replace("_", " ").capitalize()) for i in mv.get("intents", []))
                move = tr(mv.get("name", mv.get("id")), id_name(mv.get("id", "")))
                moves += f'<li><b>{esc(move)}</b> <span class="meta">{esc(intents)}</span>{" — " + ", ".join(bits) if bits else ""}</li>'
            health = f'{m.get("minHp")}–{m.get("maxHp")}' if m.get("minHp") != m.get("maxHp") else str(m.get("minHp"))
            acts = ", ".join(act_label(a) for a in m["acts"])
            name = entry_name(m)
            encounter = tr(ENCOUNTER_FR.get(key, key), ENCOUNTER_EN.get(key, key))
            cards_html += f"""<article class="monster e-{key}"><div class="mon-art"><img loading="lazy" src="{img_path(m['image'], 0)}" alt="{esc(name)}"></div>
<div><h3>{esc(name)}</h3><p class="meta">{esc(encounter)} · {esc(acts)} · ♥ {esc(hp(health))}</p><ul class="moves">{moves}</ul></div></article>"""
        out += f'<h2 id="{key}">{esc(tr(*ENCOUNTER_GROUPS[key]))} <small>({len(items)})</small></h2><div class="monsters">{cards_html}</div>'
    jump = " ".join(f'<a href="#{k}">{tr(*ENCOUNTER_GROUPS[k])}</a>' for k in ["boss", "elite", "normal", "weak", "summon"])
    note = tr("", "<p class='meta'>Monster and move names are translated from the game's identifiers.</p>")
    body = panel(f"<h1>{tr('Bestiaire', 'Bestiary')}</h1>{note}<p class='jump'>{jump}</p><input type='search' class='solo-search' "
                 f"placeholder='{tr('Rechercher un monstre…', 'Search a monster…')}' data-solo='.monster'>{out}")
    write_page("bestiaire.html", page(
        tr("Bestiaire", "Bestiary"), body, active="bestiaire.html", path="bestiaire.html",
        description=tr("Bestiaire de Slay the Spire 2 : monstres, élites et boss avec PV et attaques.",
                       "Slay the Spire 2 bestiary: monsters, elites and bosses with their HP and moves.")))


def build_list_page(filename: str, title: str, items: list[dict], group_key: str, labels: dict, text_key: str,
                    active: str) -> None:
    groups: dict[str, list[dict]] = {}
    for it in items:
        groups.setdefault(it.get(group_key, "Autre"), []).append(it)
    out = ""
    for key in sorted(groups, key=lambda k: list(labels).index(k) if k in labels else 99):
        rows = "".join(
            f'<div class="relic" data-name="{esc(entry_name(it).lower())}"><img loading="lazy" src="{img_path(it["imagePath"], 0)}" alt=""><div><b>{esc(entry_name(it))}</b><p>{entry_text(it, it.get(text_key) or it.get("descriptionClean", ""))}</p></div></div>'
            for it in sorted(groups[key], key=entry_name)
        )
        out += f'<h2>{esc(labels.get(key, key))} <small>({len(groups[key])})</small></h2><div class="relics">{rows}</div>'
    body = panel(f"<h1>{esc(title)}</h1><input type='search' class='solo-search' placeholder='{tr('Rechercher…', 'Search…')}' data-solo='.relic'>{out}")
    write_page(filename, page(title, body, active=active, path=filename,
                              description=tr(f"{title} de Slay the Spire 2 : {len(items)} entrées avec illustrations et effets.",
                                             f"{title} of Slay the Spire 2: {len(items)} entries with art and effects.")))


def build_events(events: list[dict]) -> None:
    def name(e: dict) -> str:
        return tr(e["name"], id_name(e["id"]))

    out = "".join(
        f"""<article class="event">{f'<img loading="lazy" src="{img_path(e["imagePath"], 0)}" alt="">' if e.get('imagePath') else '<div class="noimg">?</div>'}<div><h3>{esc(name(e))}</h3>
<p class="meta">{esc(', '.join(act_label(a) for a in e.get('acts') or []) or tr('Tous actes', 'Any act'))} · {e.get('optionCount', '?')} {tr('choix', 'choices')}</p>
<details><summary>{tr('Lire le récit', 'Read the story (French)')}</summary><p lang="fr">{rich(e.get('descriptionClean', ''))}</p></details></div></article>"""
        for e in sorted(events, key=name)
    )
    note = tr("", "<p class='meta'>Event names are translated from the game's identifiers; the stories are only "
                  "available in French.</p>")
    body = panel(f"<h1>{tr('Évènements mystères', 'Mystery events')}</h1>{note}<input type='search' class='solo-search' "
                 f"placeholder='{tr('Rechercher un évènement…', 'Search an event…')}' data-solo='.event'><div class='events'>{out}</div>")
    write_page("evenements.html", page(
        tr("Évènements", "Events"), body, active="evenements.html", path="evenements.html",
        description=tr("Tous les évènements mystères de Slay the Spire 2, avec leurs récits.",
                       "Every mystery event of Slay the Spire 2, with its story.")))


def build_ancients(ancients: list[dict], relics: list[dict]) -> None:
    relic_by_id = {r["id"]: r for r in relics}
    out = ""
    for a in ancients:
        slots = ""
        for s in a.get("slots", []):
            opts = ""
            for o in s.get("options", []):
                relic = relic_by_id.get(o.get("relicId", ""))
                name = entry_name(relic) if relic and en() else o["name"]
                text = relic_text(relic) if relic and en() else rich(o.get("description", ""))
                opts += (f'<div class="relic"><img loading="lazy" src="{img_path(o["imagePath"], 0)}" alt=""><div><b>{esc(name)}</b> '
                         f'<span class="tier">{esc(o.get("tier", ""))}</span><p>{text}</p></div></div>')
            picks = s.get("picks", 1)
            group = tr(*ANCIENT_GROUPS[s["group"]]) + " · " if s.get("group") in ANCIENT_GROUPS else ""
            slots += f'<h4>{group}{tr(f"{picks} choix", f"{picks} offered")}</h4><div class="relics">{opts}</div>'
        acts = tr("Tous actes", "Any act") if a.get("anyAct") else ", ".join(tr(f"Acte {x}", f"Act {x}") for x in a.get("acts", []))
        epithet = site_en.ANCIENT_EPITHETS.get(a["id"], a.get("epithet", "")) if en() else a.get("epithet", "")
        name = entry_name(a)
        out += f"""<article class="ancient" style="--ac:{esc(a.get('color', '#333'))}">
<div class="ancient-art" style="background-image:url({img_path(a['artPath'], 0)})"><img src="{img_path(a['iconPath'], 0)}" alt=""><div><h2>{esc(name)}</h2><p>{esc(epithet)} · {esc(acts)}</p></div></div>
<details><summary>{tr('Bénédictions et reliques', 'Blessings and relics')}</summary>{slots}</details><p><a href="{root()}bible/{ancient_tome(a['slug'])}">{tr('Tome détaillé ►', 'Detailed tome (FR) ►')}</a></p></article>"""
    body = panel(f"<h1>{tr('Les Anciens', 'The Ancients')}</h1><p>{tr('Divinités rencontrées entre les actes, qui façonnent votre run par des reliques uniques.', 'Deities met between acts, who shape your run with unique relics.')}</p>{out}")
    write_page("anciens.html", page(
        tr("Anciens", "Ancients"), body, active="anciens.html", path="anciens.html",
        description=tr("Les 8 Anciens de Slay the Spire 2, leurs bénédictions et reliques.",
                       "The 8 Ancients of Slay the Spire 2, their blessings and relics.")))


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
STATUS_BADGE_EN = {progress.DONE: "<i>●</i> Yes", progress.PARTIAL: "<i>◐</i> Partial", progress.TODO: "<i>○</i> No",
                   progress.EXCLUDED: "<i>–</i> Excluded"}


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
    return (f'<div class="prog-total"><b>{percent(prog.percent)}</b>'
            f'<div>{meter(prog.tally, tr("Avancement global", "Overall progress"), percent=prog.percent)}'
            f'<p class="meta">{release}{esc(counts(prog.tally))}</p>'
            f'<p class="meta">{esc(global_rule())}</p></div></div>')


def progress_tile(section: progress.Section) -> str:
    hero = section.hero
    title = tracker(section.title)
    sprite = f'<img class="pix" src="{root()}assets/sprites/hero_{hero.slug}.png" alt="">' if hero else ""
    return (f'<a class="prog-tile" href="avancement.html#{section.slug}">{sprite}<div><h3>{esc(title)}</h3>'
            f'<b>{percent(section.tally.percent)}</b>'
            f'{meter(section.tally, title, hero.color if hero else "")}'
            f'<p class="meta">{esc(counts(section.tally))}</p></div></a>')


def progress_item(item: progress.Item) -> str:
    issue = (f' <a href="{progress.ISSUE_URL}{item.issue}" target="_blank" rel="noopener">#{item.issue}</a>'
             if item.issue else "")
    note = f"<small>{esc(tracker(item.note))}{issue}</small>" if item.note or issue else ""
    detail = f' <span class="meta">{esc(item_detail(item))}</span>' if item.detail else ""
    name = item_name(item)
    search = name if item.key == name else f"{name} {item.key}"
    badge = (STATUS_BADGE_EN if en() else STATUS_BADGE)[item.status]
    return (f'<li data-status="{item.status}" data-name="{esc(search.lower())}">'
            f'<span class="st st-{item.status}">{badge}</span>'
            f'<div><b>{esc(name)}</b>{detail}{note}</div></li>')


def progress_category(category: progress.Category, color: str = "", items: bool = True) -> str:
    """One category row: name, counts, track and percentage, unfolding to its checklist when `items` is set."""
    tally = category.tally
    title = tracker(category.title)
    head = (f'<span><b>{esc(title)}</b><span class="meta">{esc(counts(tally))}</span></span>'
            f'{meter(tally, title, color)}<span class="prog-num">{percent(tally.percent)}</span>')
    if not items:
        return f'<div class="prog-cat"><div class="prog-row">{head}</div></div>'
    rows = "".join(progress_item(item) for item in category.items)
    return f'<details class="prog-cat"><summary class="prog-row">{head}</summary><ul class="prog-list">{rows}</ul></details>'


def hero_progress_panel(section: progress.Section) -> str:
    rows = "".join(progress_category(category, section.hero.color, items=False) for category in section.categories)
    title = esc(tracker(section.title))
    return panel(f"""<h2>{tr("Avancement de l'édition : ", "Edition progress: ")}{percent(section.tally.percent)}</h2>
<p>{tr(f"Ce qui est déjà implémenté dans le code du jeu, sur l'ensemble des cartes, reliques et mécaniques de {title}.",
       f"What is already implemented in the game code, across all of {title}'s cards, relics and mechanics.")}</p>
{rows}<p><a class="btn small" href="../avancement.html#{section.slug}">{tr("Voir le détail ►", "See the detail ►")}</a></p>""")


def build_progress_page(prog: progress.Progress) -> None:
    heroes = "".join(progress_tile(section) for section in prog.sections if section.hero)
    shared = "".join(progress_tile(section) for section in prog.sections if not section.hero)
    chips = "".join(f'<button data-filter-status="{status}">{tr(fr, en_label)}</button>' for status, fr, en_label in
                    [(progress.DONE, "Fait", "Done"), (progress.PARTIAL, "Partiel", "Partial"),
                     (progress.TODO, "À faire", "To do"), (progress.EXCLUDED, "Hors périmètre", "Out of scope")])
    details = ""
    for section in prog.sections:
        color = section.hero.color if section.hero else ""
        categories = "".join(progress_category(category, color) for category in section.categories)
        details += (f'<section class="panel prog-section" id="{section.slug}"><h2>{esc(tracker(section.title))} '
                    f'<small>{percent(section.tally.percent)}</small></h2>{categories}</section>')
    rules = tr("""Le numéro de version suit l'avancement : 0.9 à 9 %, 1.0 quand tout est fait.
Un élément partiel compte pour moitié ; les cartes et reliques multijoueur sont hors périmètre.
Les cartes, reliques, potions, monstres et évènements sont détectés dans le code du jeu.""",
               """The version number follows progress: 0.9 at 9%, 1.0 when everything is done.
A partial item counts as half; multiplayer cards and relics are out of scope.
Cards, relics, potions, monsters and events are detected in the game code.""")
    intro = tr("""Tout ce que Slay the GBA doit implémenter pour couvrir Slay the Spire 2, héros par héros puis pour
le jeu entier, et ce qui est déjà fait.""", """Everything Slay the GBA has to implement to cover Slay the Spire 2, hero
by hero then for the whole game, and what is already done.""")
    body = panel(f"""<h1>{tr("Avancement du portage", "Port progress")}</h1>
<p class="intro">{intro}</p>
{progress_total(prog)}
<p class="meta">{rules}
{esc(extras_text(prog))}</p>""")
    body += panel(f'<h2>{tr("Par édition", "By edition")}</h2><div class="prog-grid">{heroes}</div>')
    body += panel(f'<h2>{tr("Le jeu commun", "The shared game")}</h2><div class="prog-grid">{shared}</div>')
    body += panel(f"""<h2>{tr("Le détail", "The detail")}</h2><div class="filters" data-progress>
  <input type="search" placeholder="{tr("Rechercher une carte, une relique, un monstre, une mécanique…", "Search a card, a relic, a monster, a mechanic…")}" data-search>
  <div class="chips"><button class="on" data-filter-status="">{tr("Tout", "All")}</button>{chips}</div>
  <p class="count"></p>
</div>""")
    write_page("avancement.html", page(
        tr("Avancement", "Progress"), body + details, active="avancement.html", path="avancement.html",
        description=tr(f"Avancement de Slay the GBA : {percent(prog.percent)} du contenu de Slay the Spire 2 est "
                       "implémenté. Le détail par héros, carte, relique, monstre et système.",
                       f"Slay the GBA progress: {percent(prog.percent)} of Slay the Spire 2's content is implemented. "
                       "The detail by hero, card, relic, monster and system.")))


def build_guide() -> None:
    if en():  # an English player guide; the French page renders two tomes of the bible
        guide = markdown.markdown((Path(__file__).parent / "site_en_guide.md").read_text(), extensions=["tables"])
        body = panel(f'<div class="md">{guide}</div>')
    else:
        parts = []
        for name in ["19_boucle_de_gameplay_et_ascension.md", "21_interface_pokemon_et_navigation_carte.md"]:
            parts.append('<div class="md">' + md_to_html((BIBLE / name).read_text(), 0, "").replace("../bible/", "bible_raw/") + "</div>")
        body = "".join(panel(p) for p in parts)
        body = body.replace('src="bible_raw/', 'src="../bible/').replace('href="bible_raw/', 'href="../bible/')
        body = re.sub(r'href="([\w.-]+)\.html', r'href="bible/\1.html', body)
    write_page("guide.html", page(
        "Guide", body, active="guide.html", path="guide.html",
        description=tr("Boucle de gameplay, ascension et interface de Slay the GBA.",
                       "How to play Slay the GBA: a run, the rooms, combat, controls, Ascension and saving.")))


def bible_title(path: Path) -> str:
    """First heading of a bible markdown file, stripped of emoji."""
    m = re.search(r"^#\s+(.+)$", path.read_text(), re.M)
    return re.sub(r"[^\w\sÀ-ÿ'’:—\-&()!,.]", "", m.group(1)).strip() if m else path.stem


def build_bible() -> None:
    """The design bible, in French only (its pages have no English counterpart)."""
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
                                                   description=f"Bible de Slay the GBA : {title}.", translated=False))
        toc.append((stem, title, base))
    readme = md_to_html((BIBLE / "README.md").read_text(), 1, "")
    readme = readme.replace('href="README.html', 'href="index.html')
    listing = "".join(f'<li><a href="{s}.html">{esc(t)}</a>{" <span class=meta>livret</span>" if b else ""}</li>' for s, t, b in toc)
    body = panel(f"<h1>La bible complète</h1><p>Tous les tomes de conception du projet, rendus depuis <code>bible/</code>.</p><ol class='toc'>{listing}</ol>") + panel(f'<div class="md">{readme}</div>')
    (out_dir / "index.html").write_text(page("Bible", body, depth=1, active="bible/index.html", path="bible/index.html",
                                             description="La bible de conception complète de Slay the GBA.",
                                             translated=False))


def build_404() -> None:
    """Served by Netlify for unknown URLs in both languages, so it uses absolute paths and says it twice."""
    body = """<section class="hero-banner small" style="background-image:url(/assets/bg/gameover.png)">
  <div class="hero-inner"><p class="kicker">Erreur 404</p><h1>DÉFAITE…</h1>
  <p class="lead">Cette salle n'existe pas dans la Tour. Votre ascension s'arrête ici… à moins de rebrousser chemin.</p>
  <p lang="en">This room does not exist in the Spire. <a href="/en/index.html">Back to the English home page</a>.</p>
  <div class="cta"><a class="btn" href="/index.html">◄ Retour à l'accueil</a><a class="btn ghost" href="/heros.html">Choisir un héros</a></div></div>
</section>"""
    (SITE / "404.html").write_text(page("Page introuvable", body, noindex=True, absolute=True, path="404.html",
                                        translated=False))


def build_legal_page() -> None:
    body = panel(tr("""<h1>Mentions légales &amp; confidentialité</h1>
<h2>Éditeur</h2>
<p>Site personnel et non commercial édité par Alexandre Enouf, Paris, France — <a href="mailto:alexandre.enouf@gmail.com">alexandre.enouf@gmail.com</a>.</p>
<h2>Hébergement</h2>
<p>Netlify, Inc., 512 2nd Street, Suite 200, San Francisco, CA 94107, États-Unis — <a href="https://www.netlify.com" rel="noopener">netlify.com</a>.</p>
<h2>Propriété intellectuelle</h2>
<p>Slay the GBA est un fan-game non officiel, sans lien avec Mega Crit. <i>Slay the Spire</i>, ses personnages, cartes, illustrations et textes sont la propriété de Mega Crit Games. Les données et illustrations proviennent de la bible du projet (patch v0.111.0 Beta). Le code du projet est sous licence MIT (code uniquement : le contenu de Mega Crit en est exclu, voir <a href="https://github.com/Boblebol/slay_the_gba/blob/main/NOTICE.md">NOTICE.md</a>). Le moteur Butano est distribué sous licence zlib par Gustavo Valiente. Le jeu est gratuit et ne doit pas être vendu. Pour toute demande de retrait, écrivez à l'adresse ci-dessus.</p>
<h2>Données personnelles &amp; cookies</h2>
<p>Ce site ne dépose aucun cookie, n'utilise aucun outil de mesure d'audience et ne collecte aucune donnée personnelle. Les polices sont servies par le site lui-même : aucune ressource n'est chargée depuis un service tiers. L'hébergeur conserve des journaux techniques de connexion pour la sécurité du service.</p>
<h2>ROMs</h2>
<p>Les fichiers <code>.gba</code> proposés sont des créations homebrew compilées depuis le code du projet ; aucune ROM commerciale n'est distribuée.</p>""",
                    """<h1>Legal notice &amp; privacy</h1>
<h2>Publisher</h2>
<p>Personal, non-commercial website published by Alexandre Enouf, Paris, France — <a href="mailto:alexandre.enouf@gmail.com">alexandre.enouf@gmail.com</a>.</p>
<h2>Hosting</h2>
<p>Netlify, Inc., 512 2nd Street, Suite 200, San Francisco, CA 94107, United States — <a href="https://www.netlify.com" rel="noopener">netlify.com</a>.</p>
<h2>Intellectual property</h2>
<p>Slay the GBA is an unofficial fan game with no ties to Mega Crit. <i>Slay the Spire</i>, its characters, cards, art and texts are the property of Mega Crit Games. The data and art come from the project bible (patch v0.111.0 Beta). The project's code is under the MIT licence (code only: Mega Crit content is excluded, see <a href="https://github.com/Boblebol/slay_the_gba/blob/main/NOTICE.md">NOTICE.md</a>). The Butano engine is distributed under the zlib licence by Gustavo Valiente. The game is free and must not be sold. For any takedown request, write to the address above.</p>
<h2>Personal data &amp; cookies</h2>
<p>This site sets no cookie, uses no audience measurement tool and collects no personal data. Fonts are served by the site itself: nothing is loaded from a third-party service. The host keeps technical connection logs for the security of the service.</p>
<h2>ROMs</h2>
<p>The <code>.gba</code> files offered here are homebrew builds compiled from the project's code; no commercial ROM is distributed.</p>"""))
    write_page("mentions.html", page(
        tr("Mentions légales", "Legal notice"), body, path="mentions.html",
        description=tr("Mentions légales et politique de confidentialité de Slay the GBA.",
                       "Legal notice and privacy policy of Slay the GBA.")))


def build_seo_files() -> None:
    """robots.txt, sitemap.xml and the 1200x630 social preview image."""
    (SITE / "robots.txt").write_text(f"User-agent: *\nAllow: /\nDisallow: /roms/\n\nSitemap: {SITE_URL}/sitemap.xml\n")

    def priority(path: str) -> str:
        path = path.removeprefix("en/")
        return "1.0" if path in ("", "index.html") else "0.8" if path.startswith("heros") else "0.6"

    urls = "".join(f"  <url><loc>{esc(SITE_URL + '/' + p)}</loc><priority>{priority(p)}</priority></url>\n"
                   for p in dict.fromkeys(PAGES))
    (SITE / "sitemap.xml").write_text(
        f'<?xml version="1.0" encoding="UTF-8"?>\n<urlset xmlns="http://www.sitemaps.org/schemas/sitemap/0.9">\n{urls}</urlset>\n')
    # Real title screen capture (logo included), pixel-doubled to the social card size.
    title = Image.open(ASSETS / "screens" / "title.png").convert("RGB")
    og = title.resize((1200, 800), Image.NEAREST).crop((0, 0, 1200, 630))
    og.save(ASSETS / "og.png", optimize=True)


def english_names(cards: list[dict], relics: list[dict], potions: list[dict], monsters: list[dict],
                  events: list[dict]) -> dict[str, str]:
    """English name of every data entry, by id: what the English progress page shows."""
    names = {e["id"]: id_name(e["id"]) for e in events}
    for entries, strip_hero in ((cards, True), (relics, False), (potions, False), (monsters, False)):
        names.update({e["id"]: english_name(e["className"], strip_hero) for e in entries if e.get("className")})
    return names


def main() -> None:
    SITE.mkdir(exist_ok=True)
    cards = load("card", "cards")
    relics = load("relic", "relics")
    potions = load("potion", "potions")
    events = load("event", "events")
    ancients = load("ancient", "ancients")
    monsters = json.loads((DATA / "monster_v0.111.0.json").read_text())
    EN_NAMES.update(english_names(cards, relics, potions, monsters["monsters"], events))
    prog = progress.compute()
    progress.write_report(prog)
    build_assets()
    copy_roms()
    for lang in LANGS:
        set_lang(lang)
        build_index(cards, monsters["monsters"], relics, potions, events, prog)
        build_heroes_index(cards, relics)
        build_hero_pages(cards, relics, prog)
        build_progress_page(prog)
        build_cards_page(cards)
        build_bestiary(monsters)
        relic_labels = {"Shared": tr("Partagées", "Shared"), **{h.pool: hero_name(h) for h in HEROES},
                        "Event": tr("Évènement", "Event"), "Fallback": tr("Secours", "Fallback")}
        build_list_page("reliques.html", tr("Reliques", "Relics"), relics, "pool", relic_labels, "descI18n", "reliques.html")
        potion_labels = {"Common": tr("Communes", "Common"), "Uncommon": tr("Peu communes", "Uncommon"),
                         "Rare": tr("Rares", "Rare"), "Event": tr("Évènement", "Event"), "Token": tr("Spéciales", "Special")}
        build_list_page("potions.html", "Potions", potions, "rarity", potion_labels, "descI18n", "potions.html")
        build_events(events)
        build_ancients(ancients, relics)
        build_guide()
        build_legal_page()
    set_lang("fr")
    build_bible()
    build_404()
    build_seo_files()
    print("Site generated in", SITE, "for", SITE_URL)


if __name__ == "__main__":
    main()
