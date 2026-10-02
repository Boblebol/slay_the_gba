# /// script
# requires-python = ">=3.12"
# ///
"""Implementation tracker: how much of Slay the Spire 2 the GBA game already implements.

Everything to implement comes from bible/data (cards, relics, potions, monsters, events, ancients). What is
done is read from the engine sources, so the tracker follows the code. avancement.toml adds what the code
cannot tell (mechanics, game systems, screens) and corrects the detection where an entry is only partly
faithful to the original game.

Run from the repository root:  uv run scripts/progress.py
It rewrites AVANCEMENT.md and prints a summary. build_site.py renders the same data as site/avancement.html.

The release workflow derives the version number from the same figures (--next-version) and uses the
summary as release notes (--release-notes).
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import tomllib
from dataclasses import dataclass
from pathlib import Path

from editions import ENCOUNTER_FR, HEROES, RARITY_FR, TYPE_FR, Hero

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "bible" / "data"
ENGINE = ROOT / "src" / "engine"
MANUAL = ROOT / "avancement.toml"
REPORT = ROOT / "AVANCEMENT.md"
VERSION_FILE = ROOT / "VERSION"  # version of the last release, written by the release workflow
ISSUE_URL = "https://github.com/Boblebol/slay_the_gba/issues/"

DONE, PARTIAL, TODO, EXCLUDED = "oui", "partiel", "non", "exclu"
STATUS_LABEL = {DONE: "Oui", PARTIAL: "Partiel", TODO: "Non", EXCLUDED: "Hors périmètre"}

# Sections that are not a hero, in display order. avancement.toml can add categories to any of them.
GLOBAL_SECTIONS = {
    "cartes": "Cartes communes aux cinq éditions",
    "reliques": "Reliques et potions",
    "bestiaire": "Bestiaire",
    "rencontres": "Évènements et Anciens",
    "regles": "Règles de combat",
    "systemes": "Systèmes de jeu",
    "interface": "Interface, audio et cartouches",
}
OVERRIDE_KINDS = ("cards", "relics", "potions", "monsters", "events", "ancients")
RARITY_ORDER = ["Basic", "Starter", "Common", "Uncommon", "Rare", "Shop", "Ancient", "Event", "Curse", "Quest"]
ACT_FR = {"act1a": "Acte 1 (A)", "act1b": "Acte 1 (B)", "act2": "Acte 2", "act3": "Acte 3"}
# Cards and relics written for co-op play: they have no meaning on a single-player cartridge.
MULTIPLAYER = re.compile(r"autres? joueurs?|tous les joueurs|alliés?", re.I)
MULTIPLAYER_NOTE = "Effet multijoueur, sans objet sur GBA."


@dataclass(frozen=True)
class Tally:
    done: int = 0
    partial: int = 0
    todo: int = 0
    excluded: int = 0

    @property
    def total(self) -> int:
        """Items that count: everything but the excluded ones."""
        return self.done + self.partial + self.todo

    @property
    def percent(self) -> float:
        """A partial item counts for half."""
        return 100 * (self.done + self.partial / 2) / self.total if self.total else 0.0

    def __add__(self, other: Tally) -> Tally:
        return Tally(self.done + other.done, self.partial + other.partial, self.todo + other.todo,
                     self.excluded + other.excluded)


@dataclass(frozen=True)
class Item:
    key: str
    name: str
    status: str
    detail: str = ""
    note: str = ""
    issue: int | None = None


@dataclass(frozen=True)
class Category:
    title: str
    items: tuple[Item, ...]

    @property
    def tally(self) -> Tally:
        count = {status: sum(1 for item in self.items if item.status == status) for status in STATUS_LABEL}
        return Tally(count[DONE], count[PARTIAL], count[TODO], count[EXCLUDED])


@dataclass(frozen=True)
class Section:
    slug: str
    title: str
    categories: tuple[Category, ...]
    hero: Hero | None = None

    @property
    def tally(self) -> Tally:
        return sum((category.tally for category in self.categories), Tally())


@dataclass(frozen=True)
class Progress:
    sections: tuple[Section, ...]
    # Playable content the reference data does not know, as (label, names): leftovers of the first
    # Slay the Spire and GBA-only additions. Not counted.
    extras: tuple[tuple[str, tuple[str, ...]], ...] = ()

    @property
    def tally(self) -> Tally:
        return sum((section.tally for section in self.sections), Tally())

    def section(self, slug: str) -> Section:
        return next(section for section in self.sections if section.slug == slug)


def percent_fr(value: float) -> str:
    return f"{value:.1f}".replace(".", ",") + " %"


# ---------------------------------------------------------------------------------------------
# What the engine implements
# ---------------------------------------------------------------------------------------------


def detect(pattern: str, path: Path) -> set[str]:
    """Identifiers captured by `pattern` in an engine source. An empty result means the engine was
    refactored: fail loudly rather than report everything as missing."""
    found = set(re.findall(pattern, re.sub(r"//.*", "", path.read_text())))
    if not found:
        raise SystemExit(f"progress: nothing matches {pattern!r} in {path.relative_to(ROOT)}; "
                         "update the detection in scripts/progress.py")
    return found


# Engine card names that differ from the identifier of the same card in bible/data.
ENGINE_CARD_ALIASES = {"POISON_STAB": "POISONED_STAB", "NULL_CARD": "NULL"}


def implemented_cards() -> set[str]:
    """Cards with an effect in CombatEngine::play_card. The engine has one Strike and one Defend for the
    five heroes, the reference data one per hero."""
    cards = {ENGINE_CARD_ALIASES.get(name, name) for name in detect(r"case CardId::(\w+):", ENGINE / "combat.cpp")}
    for shared in ("STRIKE", "DEFEND"):
        if shared in cards:
            cards.remove(shared)
            cards.update(f"{shared}_{hero.pool.upper()}" for hero in HEROES)
    return cards


def implemented_relics() -> set[str]:
    return detect(r'(?:has_relic\(|relics\[\w+\] == )"(\w+)"', ENGINE / "run.cpp")


def implemented_potions() -> set[str]:
    return detect(r'potion_id == "(\w+)"', ENGINE / "combat.cpp")


def implemented_monsters() -> set[str]:
    header = re.sub(r"//.*", "", (ROOT / "include" / "engine" / "entity.h").read_text())
    body = re.search(r"enum class EnemyType[^{]*\{(.*?)\}", header, re.S)
    if not body:
        raise SystemExit("progress: enum EnemyType not found in include/engine/entity.h")
    return set(re.findall(r"\w+", body.group(1)))


def implemented_events() -> set[str]:
    return detect(r'enc\.id = "(\w+)"', ENGINE / "event_manager.cpp")


# ---------------------------------------------------------------------------------------------
# Reference data and manual statuses
# ---------------------------------------------------------------------------------------------


def load(name: str) -> dict:
    return json.loads((DATA / f"{name}_v0.111.0.json").read_text())


def read_entry(entry: dict, where: str) -> tuple[str, str, int | None]:
    """Status, note and issue of a manual entry."""
    unknown = set(entry) - {"name", "status", "note", "issue"}
    if unknown or entry.get("status") not in STATUS_LABEL:
        raise SystemExit(f"avancement.toml: {where}: expected status = oui | partiel | non | exclu "
                         f"and optional note, issue (got {entry})")
    return entry["status"], entry.get("note", ""), entry.get("issue")


class Catalogue:
    """Builds the categories whose items come from bible/data."""

    def __init__(self, manual: dict) -> None:
        self.overrides: dict[str, dict] = manual.get("overrides", {})
        unknown = set(self.overrides) - set(OVERRIDE_KINDS)
        if unknown:
            raise SystemExit(f"avancement.toml: unknown [overrides.{sorted(unknown)[0]}], expected one of {OVERRIDE_KINDS}")
        self.used: set[tuple[str, str]] = set()

    def category(self, title: str, kind: str, entries: list[dict], implemented: set[str]) -> Category:
        """`entries` are reference entries with `id`, `name`, `detail` and optionally `text` (rules text)."""
        items = []
        for entry in entries:
            key = entry["id"]
            status, note, issue = DONE if key in implemented else TODO, "", None
            if key in self.overrides.get(kind, {}):
                status, note, issue = read_entry(self.overrides[kind][key], f"overrides.{kind}.{key}")
                self.used.add((kind, key))
            elif status == TODO and MULTIPLAYER.search(entry.get("text", "")):
                status, note = EXCLUDED, MULTIPLAYER_NOTE
            items.append(Item(key, entry["name"], status, entry["detail"], note, issue))
        return Category(title, tuple(items))

    def check_overrides(self) -> None:
        for kind, entries in self.overrides.items():
            for key in entries:
                if (kind, key) not in self.used:
                    raise SystemExit(f"avancement.toml: overrides.{kind}.{key} is not in bible/data")


def rarity_rank(rarity: str) -> int:
    return RARITY_ORDER.index(rarity) if rarity in RARITY_ORDER else len(RARITY_ORDER)


def card_entries(cards: list[dict], pool: str) -> list[dict]:
    subset = sorted((c for c in cards if c["pool"] == pool), key=lambda c: (rarity_rank(c["rarity"]), c["name"]))
    return [{"id": c["id"], "name": c["name"], "text": c.get("descriptionClean", ""),
             "detail": f"{RARITY_FR.get(c['rarity'], c['rarity'])} · {TYPE_FR.get(c['type'], c['type'])}"}
            for c in subset]


def token_entries(card_data: dict) -> list[dict]:
    """Cards created during a combat (Shiv, Soul, Burn…): they only exist as tooltips in the dump."""
    listed = {c["className"] for c in card_data["cards"]}
    tips = [tip for key, tip in card_data["hoverTips"].items()
            if key.startswith("card:") and key.removeprefix("card:") not in listed]
    return [{"id": tip["key"].removeprefix("card:"), "name": tip["title"],
             "detail": f"{RARITY_FR.get(tip['card']['rarity'], tip['card']['rarity'])} · "
                       f"{TYPE_FR.get(tip['card']['type'], tip['card']['type'])}"}
            for tip in sorted(tips, key=lambda tip: tip["title"])]


def relic_entries(relics: list[dict]) -> list[dict]:
    subset = sorted(relics, key=lambda r: (rarity_rank(r["rarity"]), r["name"]))
    return [{"id": r["id"], "name": r["name"], "text": r.get("descriptionClean", ""),
             "detail": RARITY_FR.get(r["rarity"], "Secours")} for r in subset]


def monster_entries(monsters: list[dict], kinds: tuple[str, ...]) -> list[dict]:
    """Monsters met in an act; the dump also holds test and cut monsters, which have none."""
    subset = sorted((m for m in monsters if m.get("acts") and m["encounterType"] in kinds),
                    key=lambda m: (m["acts"][0], m["name"]))
    return [{"id": m["id"], "name": m["name"],
             "detail": " · ".join([ENCOUNTER_FR[m["encounterType"]]] + [ACT_FR.get(a, a) for a in m["acts"]])}
            for m in subset]


def manual_categories(manual: dict, slugs: set[str]) -> dict[str, list[Category]]:
    by_section: dict[str, list[Category]] = {}
    for block in manual.get("category", []):
        where = f"[[category]] {block.get('section')} / {block.get('title')}"
        if block.get("section") not in slugs or not block.get("title"):
            raise SystemExit(f"avancement.toml: {where}: section must be one of {sorted(slugs)}, with a title")
        items = []
        for entry in block.get("items", []):
            status, note, issue = read_entry(entry, where)
            items.append(Item(entry["name"], entry["name"], status, "", note, issue))
        by_section.setdefault(block["section"], []).append(Category(block["title"], tuple(items)))
    return by_section


def extras(implemented: set[str], reference: set[str]) -> tuple[str, ...]:
    return tuple(sorted(name.replace("_", " ").title() for name in implemented - reference))


def compute() -> Progress:
    manual = tomllib.loads(MANUAL.read_text())
    catalogue = Catalogue(manual)
    card_data = load("card")
    cards = card_data["cards"]
    relics = load("relic")["relics"]
    monsters = load("monster")["monsters"]
    events = sorted(load("event")["events"], key=lambda e: e["name"])
    hero_pools = {hero.pool for hero in HEROES}
    done_cards, done_relics, done_monsters = implemented_cards(), implemented_relics(), implemented_monsters()
    done_events = implemented_events()
    extra = manual_categories(manual, {hero.slug for hero in HEROES} | set(GLOBAL_SECTIONS))

    def relic_group(title: str, keep) -> Category:
        return catalogue.category(title, "relics", relic_entries([r for r in relics if keep(r)]), done_relics)

    def card_group(title: str, pool: str) -> Category:
        return catalogue.category(title, "cards", card_entries(cards, pool), done_cards)

    def monster_group(title: str, kinds: tuple[str, ...]) -> Category:
        return catalogue.category(title, "monsters", monster_entries(monsters, kinds), done_monsters)

    sections = [
        Section(hero.slug, hero.name, (
            card_group("Cartes", hero.pool),
            relic_group("Reliques", lambda r, pool=hero.pool: r["pool"] == pool),
            *extra.get(hero.slug, []),
        ), hero)
        for hero in HEROES
    ]
    auto = {
        "cartes": [
            card_group("Cartes incolores", "Colorless"),
            card_group("Cartes d'évènement", "Event"),
            card_group("Malédictions", "Curse"),
            card_group("Quêtes", "Quest"),
            catalogue.category("Cartes créées en combat et statuts", "cards", token_entries(card_data), done_cards),
        ],
        "reliques": [
            relic_group("Reliques partagées", lambda r: r["pool"] not in hero_pools | {"Event"} and r["rarity"] != "Ancient"),
            relic_group("Reliques d'évènement", lambda r: r["pool"] == "Event" and r["rarity"] != "Ancient"),
            relic_group("Reliques des Anciens", lambda r: r["pool"] not in hero_pools and r["rarity"] == "Ancient"),
            catalogue.category("Potions", "potions", [
                {"id": p["id"], "name": p["name"], "detail": RARITY_FR.get(p["rarity"], "Spéciale")}
                for p in sorted(load("potion")["potions"], key=lambda p: (rarity_rank(p["rarity"]), p["name"]))
            ], implemented_potions()),
        ],
        "bestiaire": [
            monster_group("Monstres", ("normal", "weak", "summon")),
            monster_group("Élites", ("elite",)),
            monster_group("Boss", ("boss",)),
        ],
        "rencontres": [
            catalogue.category("Évènements", "events", [
                {"id": e["id"], "name": e["name"],
                 "detail": ", ".join(ACT_FR.get(a, a) for a in e.get("acts") or []) or "Tous actes"} for e in events
            ], done_events),
            catalogue.category("Anciens", "ancients", [
                {"id": a["id"], "name": a["name"],
                 "detail": "Tous actes" if a.get("anyAct") else ", ".join(f"Acte {x}" for x in a.get("acts", []))}
                for a in load("ancient")["ancients"]
            ], set()),
        ],
    }
    catalogue.check_overrides()

    for slug, title in GLOBAL_SECTIONS.items():
        sections.append(Section(slug, title, tuple(auto.get(slug, []) + extra.get(slug, []))))
    return Progress(tuple(sections), (
        ("Cartes", extras(done_cards, {c["id"] for c in cards})),
        ("Monstres", extras(done_monsters, {m["id"] for m in monsters})),
        ("Évènements", extras(done_events, {e["id"] for e in events})),
    ))


# ---------------------------------------------------------------------------------------------
# Report
# ---------------------------------------------------------------------------------------------


def counts_fr(tally: Tally) -> str:
    def plural(count: int, word: str) -> str:
        return f"{count} {word}{'s' if count > 1 else ''}"

    text = f"{plural(tally.done, 'fait')}, {plural(tally.partial, 'partiel')}, {tally.todo} à faire"
    return text + (f" ({tally.excluded} hors périmètre)" if tally.excluded else "")


def extras_fr(progress: Progress) -> str:
    """Sentence listing the playable content that is not part of the reference data."""
    groups = " ; ".join(f"{label.lower()} {', '.join(names)}" for label, names in progress.extras if names)
    return f"Déjà jouables mais absents du référentiel Slay the Spire 2, donc non comptés : {groups}."


def next_version(tally: Tally, tags: list[str]) -> str:
    """Version of the next release: 0.<whole percentage>.<patch> while something is left to do, 1.0.<patch> once
    every item is done. The patch number counts the releases already tagged for the same major.minor."""
    complete = tally.total > 0 and tally.todo == 0 and tally.partial == 0
    prefix = "1.0" if complete else f"0.{int(tally.percent)}"
    patches = [int(match.group(1)) for tag in tags if (match := re.fullmatch(rf"v{re.escape(prefix)}\.(\d+)", tag))]
    return f"{prefix}.{max(patches) + 1 if patches else 0}"


def current_version() -> str:
    return VERSION_FILE.read_text().strip() if VERSION_FILE.exists() else ""


def summary_table(progress: Progress, links: bool = True) -> list[str]:
    out = ["| Section | Fait | Partiel | À faire | Avancement |", "|---|---:|---:|---:|---:|"]
    for section in progress.sections:
        t = section.tally
        title = f"[{section.title}](#{section.slug})" if links else section.title
        out.append(f"| {title} | {t.done} | {t.partial} | {t.todo} | {percent_fr(t.percent)} |")
    t = progress.tally
    out.append(f"| **Total** | **{t.done}** | **{t.partial}** | **{t.todo}** | **{percent_fr(t.percent)}** |")
    return out


def release_notes(progress: Progress) -> str:
    return "\n".join([f"## Avancement : {percent_fr(progress.tally.percent)}", "",
                      f"{counts_fr(progress.tally).capitalize()}, sur le contenu de Slay the Spire 2.", "",
                      *summary_table(progress, links=False)]) + "\n"


def markdown_report(progress: Progress) -> str:
    def cell(text: str) -> str:
        return text.replace("|", "\\|")

    out = [
        "# Avancement de Slay the GBA",
        "",
        "> Fichier généré par `make progress` : ne pas le modifier à la main. Les cartes, reliques, potions,",
        "> monstres et évènements sont détectés dans le code du moteur ; les mécaniques, systèmes et écrans",
        "> se règlent dans [`avancement.toml`](avancement.toml).",
        "",
        f"**Avancement global : {percent_fr(progress.tally.percent)}** — {counts_fr(progress.tally)}.",
        "",
        "Un élément partiel compte pour moitié. Le référentiel est le contenu de Slay the Spire 2 "
        "(patch v0.111.0 Beta, `bible/data`).",
        "",
        extras_fr(progress),
        "",
        "## Synthèse",
        "",
        *summary_table(progress),
    ]
    for section in progress.sections:
        out += ["", f'<a id="{section.slug}"></a>', "", f"## {section.title} — {percent_fr(section.tally.percent)}"]
        for category in section.categories:
            out += ["", f"### {category.title} — {percent_fr(category.tally.percent)}", "", counts_fr(category.tally) + ".", ""]
            out += ["| Élément | Détail | Fait | Note |", "|---|---|---|---|"]
            for item in category.items:
                note = item.note + (f" [#{item.issue}]({ISSUE_URL}{item.issue})" if item.issue else "")
                out.append(f"| {cell(item.name)} | {cell(item.detail)} | {STATUS_LABEL[item.status]} | {cell(note.strip())} |")
    return "\n".join(out) + "\n"


def write_report(progress: Progress) -> None:
    REPORT.write_text(markdown_report(progress))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--next-version", action="store_true", help="print the version of the next release and exit")
    parser.add_argument("--release-notes", action="store_true", help="print the progress summary as release notes and exit")
    args = parser.parse_args()
    progress = compute()
    if args.next_version:
        tags = subprocess.run(["git", "tag", "--list", "v*"], cwd=ROOT, capture_output=True, text=True, check=True).stdout.split()
        print(next_version(progress.tally, tags))
        return
    if args.release_notes:
        print(release_notes(progress), end="")
        return
    write_report(progress)
    for section in progress.sections:
        print(f"{percent_fr(section.tally.percent):>8}  {section.title} — {counts_fr(section.tally)}")
    print(f"{percent_fr(progress.tally.percent):>8}  TOTAL — {counts_fr(progress.tally)}")
    print("Written", REPORT.relative_to(ROOT))


if __name__ == "__main__":
    main()
