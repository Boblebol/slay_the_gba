# /// script
# requires-python = ">=3.12"
# ///
"""Document the choices of the events from the code (#252).

The source of truth is the code: src/engine/events/<id>.h, read through EventManager::slots() by
tools/dump_events.cpp. This script builds and runs that program, then writes

  * bible/data/event_game_v0.111.0.json: every event of the registry with its pages, and for each option the
    label, the cost, the effects (typed), the condition and the text shown once it is chosen;
  * the "Choix" section of every event in bible/18_evenements.md, between the markers
    `<!-- choix:ID -->` and `<!-- /choix:ID -->` (the rest of the tome is left as it is).

An event whose file does not exist yet is documented from the reference choices
(bible/data/event_choices/{a,b,c}.json) and marked as not yet in the game.

Run from the repository root, after adding or changing an event:

    uv run scripts/build_event_docs.py            # rebuild the dump and rewrite both files
    uv run scripts/build_event_docs.py --check    # exit 1 when a rewrite would change them (does not write)
    uv run scripts/build_event_docs.py --from-json dump.json   # skip the C++ build (tests)
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "bible" / "data"
TOME = ROOT / "bible" / "18_evenements.md"
GAME_JSON = DATA / "event_game_v0.111.0.json"
DUMP_SOURCE = ROOT / "tools" / "dump_events.cpp"
DUMP_BINARY = ROOT / "bin" / "dump_events"
PATCH = "v0.111.0"

ACT_BITS = [(1, "Acte 1a"), (2, "Acte 1b"), (4, "Acte 2"), (8, "Acte 3")]


# ── the dump ────────────────────────────────────────────────────────────────────────────────────────────────

def engine_sources() -> list[str]:
    """ENGINE_SOURCES of the Makefile: the files the unit tests link."""
    text = (ROOT / "Makefile").read_text()
    block = re.search(r"^ENGINE_SOURCES\s*:=(.*?)\n\n", text, re.S | re.M)
    if not block:
        raise SystemExit("ENGINE_SOURCES not found in the Makefile")
    return re.findall(r"src/engine/\w+\.cpp", block.group(1))


def run_dump() -> dict:
    DUMP_BINARY.parent.mkdir(exist_ok=True)
    command = ["clang++", "-std=c++20", "-Iinclude", *engine_sources(), str(DUMP_SOURCE.relative_to(ROOT)),
               "-o", str(DUMP_BINARY.relative_to(ROOT))]
    subprocess.run(command, cwd=ROOT, check=True)
    out = subprocess.run([str(DUMP_BINARY)], cwd=ROOT, check=True, capture_output=True, text=True).stdout
    return json.loads(out)


# ── the French wording of the effects and conditions ────────────────────────────────────────────────────────

def plural(n: int, word: str, many: str | None = None) -> str:
    return f"{n} {word if n <= 1 else (many or word + 's')}"


def cards_of(effect: dict, count: int) -> str:
    n = count or 1
    if effect["card"]:
        return f"{n} × {effect['card']}" + (" (améliorée)" if effect["upgraded"] else "")
    return plural(n, "carte")


def describe_effect(e: dict) -> str:
    t, a, ref = e["type"], e["amount"], e["ref"]
    how = "au hasard" if e["pick"] == "RANDOM" else "au choix"
    if t == "GAIN_GOLD":
        return f"gagne {a} Or"
    if t == "LOSE_GOLD":
        return f"perd {a} Or"
    if t == "HEAL":
        return f"soigne {a} PV"
    if t == "LOSE_HP":
        return f"perd {a} PV"
    if t == "GAIN_MAX_HP":
        return f"+{a} PV max"
    if t == "LOSE_MAX_HP":
        return f"-{a} PV max"
    if t == "ADD_CARD":
        up = " améliorée" if e["upgraded"] else ""
        if e["card_pool"]:
            return f"ajoute au deck 1 carte{up} parmi : {', '.join(e['card_pool'])}"
        if ref == "COMMON":
            return f"ajoute au deck {plural(a or 1, 'carte Commune')} du héros{up}"
        if e["card"]:
            return f"ajoute au deck {a or 1} × {e['card']}{up}"
        return f"ajoute au deck {plural(a or 1, 'carte Incolore')} aléatoire{up}"
    if t == "ADD_CURSE":
        return f"ajoute au deck {a or 1} × {e['card']}" if e["card"] else f"ajoute au deck {plural(a or 1, 'malédiction')} aléatoire"
    if t == "ADD_RANDOM_CARD":
        kind = {"POWER": "Pouvoir", "ZERO_COST": "coûtant 0"}.get(ref, ref or "non basique")
        return f"ajoute au deck 1 carte {kind} aléatoire du héros"
    if t == "REMOVE_CARD":
        return f"retire {cards_of(e, a)} du deck" + ("" if e["card"] else f" ({how})")
    if t == "UPGRADE_CARD":
        return f"améliore {cards_of(e, a)} ({how})"
    if t == "TRANSFORM_CARD":
        return f"transforme {cards_of(e, a)} ({how})" + (", la nouvelle carte est améliorée" if e["upgraded"] else "")
    if t == "ENCHANT_CARD":
        kind = f", type {e['only_type']}" if e["only_type"] >= 0 else ""
        return f"enchante {cards_of(e, a)} ({how}) : {e['enchantment']} {e['enchant_amount'] or ''}".rstrip() + kind
    if t == "CARD_REWARD":
        who = {"COLORLESS": "Incolores", "": "du héros"}.get(ref, ref)
        return f"récompense de carte ×{a or 1} ({who}), à la sortie de l'évènement"
    if t == "GAIN_RELIC":
        if ref == "COMMON":
            return "gagne une relique Commune aléatoire"
        if ref:
            return f"gagne la relique {ref}"
        if e["relic_pool"]:
            return f"gagne 1 relique parmi : {', '.join(e['relic_pool'])}"
        return "gagne une relique aléatoire"
    if t == "RELIC_REWARD":
        return "récompense de relique (comme un Élite), à la sortie de l'évènement"
    if t == "LOSE_RELIC":
        return "perd la dernière relique obtenue"
    if t == "LOSE_POTION":
        return f"perd la potion {ref}" if ref else f"perd {plural(a or 1, 'potion')} au hasard"
    if t == "GAIN_POTION":
        return f"gagne la potion {ref}" if ref else f"gagne {plural(a or 1, 'potion')} aléatoire"
    if t == "FIGHT":
        kind = {"ELITE": "un combat d'Élite", "PUNCH_CONSTRUCTS": "2 Constructions à poing, puis une relique"}.get(ref, "un combat normal")
        return f"déclenche {kind}, à la sortie de l'évènement"
    if t == "TRADE_POTION":
        return "échange la première potion contre une carte améliorée de même rareté"
    if t == "GOTO_PAGE":
        if e["enchant_amount"] > 1:
            return f"mène à l'une des pages {a} à {a + e['enchant_amount'] - 1} (tirée au hasard)"
        return f"mène à la page {a}"
    if t == "LOSE_CARD":
        return f"perd 1 × {e['card']}"
    if t == "DOWNGRADE_CARD":
        return f"{plural(a or 1, 'carte améliorée', 'cartes améliorées')} redevient normale (au hasard)"
    if t == "DUPLICATE_DECK":
        return "chaque carte du deck est copiée une fois"
    if t == "LEAVE":
        return "aucun effet"
    if t == "CARD_CHOICE":
        return f"propose {a} cartes du héros (rares comprises), le joueur en ajoute 1"
    if t == "TRADE_RELIC":
        return "échange une relique au hasard contre une relique qu'on n'avait pas"
    return f"{t} {a} {ref}".strip()


def describe_condition(c: dict) -> str:
    check, a, ref = c["check"], c["amount"], c["ref"]
    if check == "NONE":
        return ""
    return {
        "MIN_GOLD": f"au moins {a} Or",
        "MIN_HP": f"plus de {a} PV",
        "HAS_RELIC": f"posséder la relique {ref}",
        "NO_RELIC": f"ne pas posséder la relique {ref}",
        "MIN_DECK": f"au moins {a} cartes dans le deck",
        "HAS_ATTACK": "une Attaque du deck peut être enchantée",
        "HAS_POTION": f"posséder la potion {ref}" if ref else "au moins une potion",
        "CAN_ENCHANT": "une carte du deck peut être enchantée" if a < 0 else f"une carte de type {a} peut être enchantée",
        "CAN_SOULS_POWER": "une carte du deck a Épuisement",
        "MIN_RELICS": f"au moins {a} reliques",
        "HAS_CARD": f"posséder {a or 1} × {c['card']}",
    }.get(check, f"{check} {a} {ref}".strip())


def acts_label(mask: int) -> str:
    if mask == 0:
        return "aucune (ouvert par une quête uniquement)"
    if mask == 15:
        return "tous les actes"
    return ", ".join(name for bit, name in ACT_BITS if mask & bit)


# ── the reference, for the events the game does not have yet ────────────────────────────────────────────────

def reference_choices() -> dict:
    events: dict = {}
    for part in "abc":
        events.update(json.loads((DATA / "event_choices" / f"{part}.json").read_text())["events"])
    return events


def describe_reference_effect(e: dict) -> str:
    text = e["type"].replace("_", " ")
    if e["amount"]:
        text += f" {e['amount']}"
    if e["ref"]:
        text += f" ({e['ref']})"
    if e["note"]:
        text += f" - {e['note']}"
    return text


# ── the data file ───────────────────────────────────────────────────────────────────────────────────────────

def build_game_json(dump: dict) -> dict:
    reference = json.loads((DATA / "event_v0.111.0.json").read_text())["events"]
    order = {event["id"]: i for i, event in enumerate(reference)}
    events = sorted(dump["events"], key=lambda event: order.get(event["id"], len(order)))
    return {
        "patch": PATCH,
        "generated_by": "scripts/build_event_docs.py from src/engine/events (tools/dump_events.cpp): do not edit by hand",
        "events": events,
    }


# ── the tome ────────────────────────────────────────────────────────────────────────────────────────────────

def option_lines(option: dict) -> list[str]:
    cost = option["cost_benefit"] or "-"
    lines = [f"* **{option['label']}** - {cost}"]
    condition = describe_condition(option["condition"])
    if condition:
        lines.append(f"  * Condition : {condition} (sinon l'option est grisée)")
    effects = [describe_effect(e) for e in option["effects"]]
    lines.append(f"  * Effets : {' ; '.join(effects) if effects else 'aucun'}")
    lines.append(f"  * Résultat : « {option['result']} »" if option["result"] else "  * Résultat : -")
    return lines


def choices_section(event: dict, reference: dict) -> str:
    out = [f"<!-- choix:{event['id']} -->", "**Choix dans le jeu** (généré depuis `src/engine/events`) :", ""]
    if event["implemented"]:
        out.append(f"* **Titre en jeu :** {event['title']} | **Zones :** {acts_label(event['acts'])}")
        condition = describe_condition(event["condition"])
        if condition:
            out.append(f"* **Apparaît seulement si :** {condition}")
        for number, page in enumerate(event["pages"]):
            out += ["", f"Page {number}{' (début)' if number == 0 else ''} : « {page['text']} »", ""]
            for option in page["options"]:
                out += option_lines(option)
    else:
        out.append("*Pas encore dans le jeu* : choix de la référence (`bible/data/event_choices`).")
        for page in reference.get(event["id"], {}).get("pages", []):
            out += ["", f"Page `{page['id']}` :", ""]
            for option in page["options"]:
                out.append(f"* **{option['label_fr']}** ({option['label_en']})")
                effects = [describe_reference_effect(e) for e in option["effects"]]
                out.append(f"  * Effets : {' ; '.join(effects)}")
    out += ["", f"<!-- /choix:{event['id']} -->"]
    return "\n".join(out)


def render_tome(text: str, game: dict) -> str:
    reference = reference_choices()
    by_id = {event["id"]: event for event in game["events"]}
    text = re.sub(r"\n*<!-- choix:(\w+) -->.*?<!-- /choix:\1 -->\n*", "\n\n", text, flags=re.S)
    parts = re.split(r"(?m)^(?=### )", text)
    for i, part in enumerate(parts):
        heading = re.match(r"### .*\(`(\w+)`\)", part)
        if heading and heading.group(1) in by_id:
            parts[i] = part.rstrip("\n") + "\n\n" + choices_section(by_id[heading.group(1)], reference) + "\n\n"
    return "".join(parts).rstrip("\n") + "\n"


def dumps(data: dict) -> str:
    return json.dumps(data, ensure_ascii=False, indent=1) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="fail when the files are not up to date, write nothing")
    parser.add_argument("--from-json", type=Path, help="use this dump instead of building tools/dump_events.cpp")
    args = parser.parse_args()

    dump = json.loads(args.from_json.read_text()) if args.from_json else run_dump()
    game = build_game_json(dump)
    json_text = dumps(game)
    tome_text = render_tome(TOME.read_text(), game)

    stale = [path for path, new in ((GAME_JSON, json_text), (TOME, tome_text))
             if not path.exists() or path.read_text() != new]
    if args.check:
        for path in stale:
            print(f"out of date: {path.relative_to(ROOT)} (run: uv run scripts/build_event_docs.py)")
        return 1 if stale else 0
    GAME_JSON.write_text(json_text)
    TOME.write_text(tome_text)
    done = sum(event["implemented"] for event in game["events"])
    print(f"{len(game['events'])} events, {done} in the game: wrote {GAME_JSON.relative_to(ROOT)} and {TOME.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
