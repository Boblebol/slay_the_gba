"""English texts of the website (site/en/), for build_site.py.

The game and the project bible are in French. The English pages read the English description templates that
bible/data keeps next to the French texts (the game's own localisation strings, with {Var:formatter()} fields),
fill them with each entry's values, and name entries after their class name. What has no English source in the
data (event stories, monster move names, Ancient dialogues) stays in French on the English pages.
"""

from __future__ import annotations

import re

# Words the class names glue together that the game spells with an apostrophe or a hyphen.
NAME_FIXES = {
    "Paels": "Pael's", "Vakuus": "Vakuu's", "Tanxs": "Tanx's", "Orobass": "Orobas's", "Darvs": "Darv's",
    "Nonupeipes": "Nonupeipe's", "Tezcataras": "Tezcatara's", "Neows": "Neow's",
}
HERO_SUFFIXES = ("Ironclad", "Silent", "Defect", "Regent", "Necrobinder")
# Keywords the English templates leave out (the French texts print them): above or below the effect, as in game.
KEYWORDS_ABOVE = ("Innate", "Ethereal", "Retain", "Sly", "Unplayable")
KEYWORD_LABELS = {"Sly": "Sly", "Innate": "Innate", "Ethereal": "Ethereal", "Retain": "Retain",
                  "Unplayable": "Unplayable", "Exhaust": "Exhaust", "Eternal": "Eternal"}


# Short words titles keep in lower case ("Art of War"), except as the first word.
SMALL_WORDS = {"of", "the", "a", "an", "in", "on", "to", "and", "for", "with", "at", "from"}


def english_name(class_name: str, strip_hero: bool = False) -> str:
    """'BeetleJuice' -> 'Beetle Juice'; with `strip_hero`, 'StrikeIronclad' -> 'Strike'."""
    if strip_hero:
        for suffix in HERO_SUFFIXES:
            if class_name.endswith(suffix) and class_name != suffix:
                class_name = class_name[: -len(suffix)]
                break
    words = [NAME_FIXES.get(w, w) for w in re.findall(r"[A-Z]+(?![a-z])|[A-Z]?[a-z']+|\d+", class_name)]
    return " ".join(w.lower() if i and w.lower() in SMALL_WORDS else w for i, w in enumerate(words)) or class_name


def id_name(identifier: str) -> str:
    """'BATTLEWORN_DUMMY' -> 'Battleworn Dummy'."""
    words = identifier.lower().split("_")
    return " ".join(w if i and w in SMALL_WORDS else w.capitalize() for i, w in enumerate(words))


class Unresolved(Exception):
    """A template field whose value the data does not give."""


def _split_top(text: str, sep: str = "|") -> list[str]:
    """Split on `sep` outside braces."""
    parts, depth, cur = [], 0, ""
    for ch in text:
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
        if ch == sep and depth == 0:
            parts.append(cur)
            cur = ""
        else:
            cur += ch
    parts.append(cur)
    return parts


def _fields(text: str):
    """Yield (start, end, inner) for every top-level {...} field."""
    depth, start = 0, -1
    for i, ch in enumerate(text):
        if ch == "{":
            if depth == 0:
                start = i
            depth += 1
        elif ch == "}" and depth:
            depth -= 1
            if depth == 0:
                yield start, i + 1, text[start + 1:i]


def _num(value: float) -> str:
    return str(int(value)) if float(value).is_integer() else f"{value:g}"


class Template:
    """Fills one SmartFormat-like template. Unknown values come, in order, from `fallback` numbers."""

    def __init__(self, values: dict[str, float], upgraded: bool, fallback: list[str] | None = None,
                 strings: dict[str, str] | None = None):
        self.values = {k.lower(): v for k, v in values.items() if isinstance(v, (int, float))}
        self.strings = {k.lower(): v for k, v in (strings or {}).items()}
        self.upgraded = upgraded
        # The French numbers stand in only for entries without any value in the data (most potions).
        self.fallback = [] if self.values else list(fallback or [])

    def value(self, name: str) -> float:
        key = name.lower()
        if key in self.values:
            return self.values[key]
        snake = re.sub(r"(?<!^)(?=[A-Z])", "_", name).lower()
        # {CalculatedDamage} and friends show the base value out of combat.
        calculated = "calculationbase" if key.startswith("calculated") else ""
        for k in (snake, key.replace("power", ""), snake.replace("_power", ""), calculated):
            if k in self.values:
                return self.values[k]
        if self.fallback:
            v = float(self.fallback.pop(0))
            self.values[key] = v
            return v
        raise Unresolved(name)

    def render(self, text: str) -> str:
        out, last = "", 0
        for start, end, inner in _fields(text):
            out += text[last:start] + self.field(inner)
            last = end
        return out + text[last:]

    def field(self, inner: str) -> str:
        if inner == "":
            return ""
        if inner == "singleStarIcon":
            return "★"
        name, _, rest = inner.partition(":")
        if not rest:
            if name.lower() in self.strings:
                return self.strings[name.lower()]
            if re.fullmatch(r"[A-Za-z]\w*", name):
                return _num(self.value(name))
            raise Unresolved(inner)
        fmt, _, args = rest.partition(":")
        if fmt in ("diff()", "inverseDiff()"):
            return _num(abs(self.value(name)))
        if fmt in ("percentMore()", "percentLess()"):
            v = self.value(name)
            return _num(round(abs(v - 1) * 100) if 0 < v < 3 and not float(v).is_integer() else v)
        if fmt.startswith("energyIcons("):
            n = fmt[len("energyIcons("):-1]
            # energyIcons(1) is an icon after a number already written; energyIcons() writes the amount.
            return "●" * int(n) if n else f"{_num(self.value(name))} ●"
        if fmt == "starIcons()":
            return f"{_num(self.value(name))} ★"
        if fmt == "plural":
            options = _split_top(args)
            v = self.value(name)
            chosen = options[0] if v == 1 or len(options) == 1 else options[1]
            return self.render(re.sub(r"\{(?::[^{}]*)?\}", _num(v), chosen))
        if fmt.startswith("choose("):
            raise Unresolved(inner)
        if fmt == "show" and name == "IfUpgraded":
            options = _split_top(args)
            return self.render(options[0] if self.upgraded else (options[1] if len(options) > 1 else ""))
        if fmt == "cond" and name.endswith(".StringValue"):
            # The card or relic a run picked ({StarterCard.StringValue:cond:...|a starter card}): out of a run, the
            # generic else branch.
            return self.render(_split_top(args)[-1])
        if fmt == "cond":
            cond, _, branches = args.partition("?")
            options = _split_top(branches)
            m = re.fullmatch(r"([<>=!]=?)(-?\d+)", cond)
            if not m:
                raise Unresolved(inner)
            v, op, ref = self.value(name), m.group(1), float(m.group(2))
            ok = {">": v > ref, "<": v < ref, ">=": v >= ref, "<=": v <= ref, "=": v == ref, "==": v == ref,
                  "!=": v != ref}[op]
            return self.render(options[0] if ok else (options[1] if len(options) > 1 else ""))
        # Combat-only and state flags ({InCombat:...|}, {IsOstyAlive:a|b}, {Violence:...|}): the encyclopedia
        # shows the card out of combat, so the else branch.
        options = _split_top(rest)
        if len(options) >= 2 and re.fullmatch(r"\w+", name):
            return self.render(options[-1])
        raise Unresolved(inner)


def markup(text: str) -> str:
    """Game colour tags to the site's HTML; the text must already be escaped."""
    text = re.sub(r"\[gold\](.*?)\[/gold\]", r'<b class="kw">\1</b>', text, flags=re.S)
    text = re.sub(r"\[/?\w+\]", "", text)
    text = text.replace("★", '<span class="star">★</span>').replace("●", '<span class="nrg">●</span>')
    return text.strip().replace("\n", "<br>")


NUMBER = re.compile(r"\d+(?:[.,]\d+)?")


def numbers(text: str) -> list[str]:
    return [n.replace(",", ".") for n in NUMBER.findall(re.sub(r"<[^>]+>", "", text or ""))]


def _norm(key: str) -> str:
    key = key.lower().replace("_", "")
    return key[:-5] if key.endswith("power") and key != "power" else key


def describe(entry: dict, *, upgraded: bool = False, french: str = "") -> str | None:
    """English plain text (with game tags) of a card, relic or potion, or None when it cannot be filled.
    `french` is the matching French text: its numbers fill the fields the data has no value for."""
    template = entry.get("description") or ""
    keywords = list(entry.get("keywords") or [])
    if not template and not (keywords and len((french or "x").strip().split("\n")) == len(keywords)):
        return None  # an empty template is only right for cards that are nothing but keywords (most curses)
    values = dict(entry.get("vars") or {})
    if upgraded:
        for key, delta in (entry.get("upgrade") or {}).items():
            # "energy" is the card's cost; every other key names a value, under any of its aliases.
            if key.lower() == "energy" or not (isinstance(delta, str) and re.fullmatch(r"[+-]\d+", delta)):
                continue
            targets = {_norm(key)} | ({"energy"} if _norm(key) == "energygain" else set())
            for k in list(values):
                if _norm(k) in targets and isinstance(values[k], (int, float)):
                    values[k] = values[k] + int(delta)
    try:
        # Enchantments are named by id ({"EnchantmentName": "GOOPY"}).
        strings = {k: id_name(v) for k, v in (entry.get("enchantmentVars") or {}).items() if isinstance(v, str)}
        text = Template(values, upgraded, numbers(french), strings).render(template)
    except (Unresolved, ValueError, KeyError):
        return None
    if "{" in text or "}" in text:
        return None
    if upgraded:
        keywords += [k for k in (entry.get("upgrade") or {}).get("keywords", []) if k not in keywords]
        keywords = [k for k in keywords if k not in (entry.get("upgrade") or {}).get("removeKeywords", [])]
    above = [k for k in keywords if k in KEYWORDS_ABOVE]
    below = [k for k in keywords if k not in KEYWORDS_ABOVE]
    lines = [f"[gold]{KEYWORD_LABELS.get(k, k)}[/gold]." for k in above] + [text.strip()]
    lines += [f"[gold]{KEYWORD_LABELS.get(k, k)}[/gold]." for k in below]
    return "\n".join(line for line in lines if line)


# The five editions, in English (scripts/editions.py holds the French texts and every number).
HERO_TEXT = {
    "ironclad": dict(
        title="Cursed warrior from Hell", edition="Ironclad Edition", color_name="Blood Red", zone="Carnage Forges",
        mechanic="Brute strength, area attacks, Block and blood pacts.",
        mechanic_name="Strength & sacrifice",
        mechanic_text="Strength raises the damage of every attack for the whole fight. The Ironclad stacks it (Inflame, "
                      "Demon Form), trades HP for power and Exhausts his weak cards to sharpen his deck. Burning Blood "
                      "closes his wounds after every victory.",
        tagline="Brute strength, forged in blood.",
        pitch="The ideal hero to discover the Spire: lots of HP, a relic that heals after every fight and attacks that "
              "hit hard. Stack Strength, tank behind your Block, then crush everything.",
        lore="Last survivor of a decimated warrior order, the Ironclad sealed a pact with demonic powers to avenge his "
             "brothers in arms. Wielding a colossal blade forged in blood and blackened iron, he climbs the Spire to "
             "reach its heart.",
        ideal="players who like to hit hard, take hits and have their mistakes forgiven by healing.",
        tips=(
            "Open with **Bash**: Vulnerable makes the enemy take 50% more damage, then follow up with your Strikes.",
            "**Burning Blood** heals 6 HP after every fight: taking a little damage to end a fight a turn earlier often pays off.",
            "At the campfire, upgrade **Bash** and your Strength cards first: they boost all your attacks.",
            "Against groups, favour attacks that hit ALL enemies (Cleave, Thunderclap).",
        ),
        palettes=(("Crimson Red (Classic)", "The Ironclad's traditional blood-red armour."),
                  ("Azure Flame (Chromatic)", "A sparkling cyan hue born of mystic fire."),
                  ("Obsidian (Shadow)", "Charcoal-black armour forged in the abyss."),
                  ("Radiant Sun (Gold)", "Imperial golden attire studded with sunbursts.")),
    ),
    "silent": dict(
        title="Deadly huntress of the mists", edition="Silent Edition", color_name="Venom Green", zone="Venom Marsh",
        mechanic="Snowballing poison, shivs, discard and evasion.",
        mechanic_name="Poison & shivs",
        mechanic_text="Poison deals its damage at the start of the enemy's turn, then goes down by 1: it ignores Block "
                      "and stacks up. Shivs are 0-energy attacks her cards create, perfect with Accuracy. Draw and "
                      "discard cycle her deck faster than anyone else's.",
        tagline="Strike unseen. Let the venom finish the job.",
        pitch="Fewer HP, but a bigger hand from the first turn thanks to Ring of the Snake. Poison, dodge and drown the "
              "enemy in shivs: the Silent wins by attrition.",
        lore="A feared huntress from the misty lands, the Silent hides her face under the skull of a horned beast. "
             "Where warriors rely on strength, she relies on agility, blades hidden under her cloak and toxins that "
             "consume her prey from within.",
        ideal="patient players who like combos, card draw and damage over time.",
        tips=(
            "**Neutralize** on turn one: Weak reduces the enemy's damage by 25%.",
            "Poison ignores Block: it is your best answer to enemies who turtle up.",
            "With **Ring of the Snake** you start every fight with 7 cards: look for your key pieces from turn 1.",
            "Accuracy + Blade Dance turns every Shiv into a real attack.",
        ),
        palettes=(("Emerald Green (Classic)", "The green assassin's cloak of the snake tribe."),
                  ("Amethyst (Chromatic)", "A venomous purple outfit from the forest shadows."),
                  ("Night Wraith (Shadow)", "An assassin's garb invisible in the dark."),
                  ("Golden Venom (Gold)", "The ceremonial attire of the silent guild's masters.")),
    ),
    "defect": dict(
        title="Awakened combat automaton", edition="Defect Edition", color_name="Sapphire Blue", zone="Astral Complex",
        mechanic="Lightning, frost, dark and plasma orbs; Focus and Evokes.",
        mechanic_name="Orbs & Focus",
        mechanic_text="The Defect Channels Orbs into its slots. Each orb acts on its own at the end of the turn "
                      "(Lightning: damage, Frost: Block, Dark: charge, Plasma: energy) and releases a stronger effect "
                      "when it is Evoked or pushed out by a new orb. Focus strengthens every orb at once.",
        tagline="One automaton, three slots, endless circuits.",
        pitch="The engineer's hero: every channeled orb works for you, turn after turn. Raise your Focus, line up "
              "lightning and frost, and watch the machine run away with the fight.",
        lore="An automaton built to purge the Spire, the Defect grew a mind of its own after its targeting subsystems "
             "failed. Powered by a cracked core, it keeps orbs of lightning, frost, darkness and plasma orbiting its "
             "chassis.",
        ideal="players who like to plan, optimise and watch an engine run by itself.",
        tips=(
            "**Cracked Core** channels a Lightning orb from the start: play **Dualcast** early for a big burst.",
            "Frost orbs make free Block every turn: two or three are enough to stabilise a fight.",
            "Focus (Defragment, Biased Cognition) is the stat that takes the deck from good to unplayable for the enemy.",
            "Capacitor adds slots: more orbs, more passive effects.",
        ),
        palettes=(("Electric Blue (Classic)", "The automaton's forged cobalt chassis."),
                  ("Cyberpunk Neon (Chromatic)", "A bright orange finish overloaded with energy."),
                  ("Starless Void (Shadow)", "A matte black alloy without a single reflection."),
                  ("Antique Copper (Gold)", "A golden brass frame restored from a lost age.")),
    ),
    "regent": dict(
        title="Fallen celestial sovereign", edition="Regent Edition", color_name="Solar Gold", zone="Sovereign Palace",
        mechanic="Gathering Sovereign Stars and forging the Sovereign Blade.",
        mechanic_name="Stars & Sovereign Blade",
        mechanic_text="The Regent gathers Stars (★), a second resource on top of energy that fuels his strongest cards. "
                      "He also Forges the Sovereign Blade: every \"Forge\" card makes it sharper, up to the royal strike "
                      "that turns the fight around.",
        tagline="A crown, the stars, and the Spire on its knees.",
        pitch="New in Slay the Spire 2, the Regent plays with two resources: energy and Stars. Divine Right gives him "
              "some from the start of the fight. Prepare your decrees, forge your blade, strike like a king.",
        lore="Sovereign of a forgotten stellar dynasty, the Regent watched his celestial kingdom collapse under the "
             "Spire's grip. Clad in purple and gold, he wields a sceptre that channels the constellations and means to "
             "take his throne back.",
        ideal="players who like managing one more resource and setting up a decisive turn.",
        tips=(
            "**Divine Right** gives 3 ★ at the start of every fight: spend them early, don't let them sleep.",
            "**Falling Star** costs 0 and applies Weak and Vulnerable: the Regent's best opener.",
            "\"Forge\" cards add up: **Conqueror** then doubles the Sovereign Blade's damage.",
            "Keep an eye on energy AND Stars: a turn without one or the other is a wasted turn.",
        ),
        palettes=(("Royal Purple (Classic)", "The sovereign purple reserved for the line of kings."),
                  ("Scarlet Ruby (Chromatic)", "A brilliant crimson robe, a symbol of bravery."),
                  ("Astral Eclipse (Shadow)", "A cloak of shadow woven from the dust of dead stars."),
                  ("Celestial Crown (Gold)", "A golden splendour radiating celestial dignity.")),
    ),
    "necrobinder": dict(
        title="Keeper of the dead and the bones", edition="Necrobinder Edition", color_name="Spectral Violet",
        zone="Beyond-the-Grave Crypt",
        mechanic="Summoning Osty, Doom, Souls and bending death.",
        mechanic_name="Osty & Doom",
        mechanic_text="The Necrobinder fights alongside Osty, her skeletal familiar: \"Summon\" gives him HP, he takes "
                      "hits in her place and strikes with Unleash. Doom marks enemies for certain death, and the Souls "
                      "added to her Draw Pile feed her spells.",
        tagline="She never fights alone.",
        pitch="The lowest HP in the Spire, but a skeleton bodyguard who grows every turn thanks to Bound Phylactery. "
              "Protect Osty, mark your prey with Doom, and let death do its work.",
        lore="Bound forever to the Spire's wandering souls, the Necrobinder walks escorted by Osty, a skeletal "
             "familiar she raises again and again. Frail of body, she makes up for it with a twofold presence on the "
             "battlefield.",
        ideal="players who like summons, delayed effects and high-risk runs.",
        tips=(
            "**Bound Phylactery** summons at the start of every turn: the longer the fight, the sturdier Osty gets.",
            "**Unleash** adds Osty's current HP to its damage: let him grow before you let him loose.",
            "With 65 HP every point counts: let Osty take the big elite attacks.",
            "Doom (Deathbringer, Countdown) finishes off enemies whose HP drops below it.",
        ),
        palettes=(("Spectral Hue (Classic)", "The dark robe edged with a sea-green spectral halo."),
                  ("Crimson Soul (Chromatic)", "A glowing violet aura of forbidden magic."),
                  ("Mortuary Abyss (Shadow)", "The utter black of the Spire's ancient tombs."),
                  ("Pharaonic (Gold)", "Wrappings braided with pure gold thread of dead kings.")),
    ),
}

ANCIENT_EPITHETS = {
    "NEOW": "Mother of Resurrection (exiled)", "DARV": "The Hoarder", "OROBAS": "Living Rainbow",
    "PAEL": "Wax Dragon", "TEZCATARA": "Keeper of the Flame", "NONUPEIPE": "Incarnation of Fortune",
    "TANX": "Chimera King", "VAKUU": "The First Demon",
}

CONTROLS = [
    ("Map", "◄ ►", "Choose the next room"),
    ("Map", "▲ ▼", "Scroll the Spire"),
    ("Map", "A", "Walk to the room"),
    ("Combat", "◄ ►", "Select a card"),
    ("Combat", "A", "Play the card (then pick a target when several enemies are alive)"),
    ("Combat", "B", "Cancel targeting"),
    ("Combat", "R", "End the turn (L with the Quick control preset)"),
    ("Combat", "L", "Potion belt (R with the Quick control preset)"),
    ("Map / Combat", "START", "Pause menu"),
    ("Map / Combat", "SELECT", "Deck viewer (B to go back)"),
    ("Menus", "A / B", "Confirm / cancel"),
]
