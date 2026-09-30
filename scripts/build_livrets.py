# /// script
# requires-python = ">=3.11"
# ///
"""Generate the five cartridge instruction booklets (bible/livrets_gba/) from editions.py and bible/data.

Run from the repository root:  uv run scripts/build_livrets.py
The booklets share one structure so the five editions stay consistent with each other and with the website.
"""

from __future__ import annotations

import json
import re
import unicodedata
from pathlib import Path

from editions import CONTROLS, DIFFICULTY, ENERGY, GOLD, HEROES, POTION_SLOTS, Hero

ROOT = Path(__file__).resolve().parent.parent
DATA = ROOT / "bible" / "data"
OUT = ROOT / "bible" / "livrets_gba"

TYPE_FR = {"Attack": "Attaque", "Skill": "Compétence", "Power": "Pouvoir"}
RARITY_FR = {"Basic": "Base", "Common": "Commune", "Uncommon": "Peu commune", "Rare": "Rare", "Ancient": "Ancienne"}


def load(name: str, key: str) -> dict[str, dict]:
    return {item["id"]: item for item in json.loads((DATA / f"{name}_v0.111.0.json").read_text())[key]}


def md_text(text: str) -> str:
    """Turn the dump's inline markup into Markdown (keywords in bold, ★ stars, ⚡ energy)."""
    text = text or ""
    text = re.sub(r"<g>(.*?)</g>", r"**\1**", text)
    text = re.sub(r"<s>(.*?)</s>", r"\1 ★", text)
    text = re.sub(r"<e>(.*?)</e>", r"\1 ⚡", text)
    text = text.replace("<e/>", "⚡").replace("[E]", "⚡")
    text = re.sub(r"</?\w+/?>", "", text)
    return text.replace("\n", " ").replace("|", "/").strip()


def sort_key(text: str) -> str:
    return unicodedata.normalize("NFD", text).encode("ascii", "ignore").decode().lower()


def cost(card: dict) -> str:
    return "X" if card.get("energy") in (-1, None) else str(card["energy"])


def card_row(card: dict) -> str:
    base = md_text(card.get("descBase") or card.get("descriptionClean", ""))
    up = md_text(card.get("descUpgraded") or "") or base
    return (f"| ![{card['name']}](..{card['imagePath']}) **{card['name']}** | {cost(card)} ⚡ | "
            f"{TYPE_FR.get(card['type'], card['type'])} | {base} | {up} |")


def booklet(hero: Hero, cards: dict[str, dict], relics: dict[str, dict]) -> str:
    relic = relics[hero.relic_id]
    pool = [c for c in cards.values() if c["pool"] == hero.pool]
    commons = sorted((c for c in pool if c["rarity"] == "Common"), key=lambda c: sort_key(c["name"]))
    counts = {r: sum(1 for c in pool if c["rarity"] == r) for r in RARITY_FR}
    starter = "\n".join(f"* **{n}×** {cards[cid]['name']} — {md_text(cards[cid].get('descBase', ''))}"
                        for cid, n in hero.starter)
    deck_size = sum(n for _, n in hero.starter)
    controls = "\n".join(f"| {screen} | **{button}** | {action} |" for screen, button, action in CONTROLS)
    palettes = "\n".join(f"{i}. **{p.name}** (`{p.hex}`) — {p.description}" for i, p in enumerate(hero.palettes, 1))
    key_cards = "\n".join(card_row(cards[cid]) for cid in hero.key_cards)
    tips = "\n".join(f"{i}. {tip}" for i, tip in enumerate(hero.tips, 1))
    common_rows = "\n".join(card_row(c) for c in commons)
    others = [h.name for h in HEROES if h is not hero]
    return f"""# Livret d'instructions — Slay the GBA : {hero.edition}

```
====================================================================================
   SLAY THE GBA — LIVRET D'INSTRUCTIONS
   {hero.edition.upper()} · CARTOUCHE {hero.rom} · RÉF. {hero.ref}
====================================================================================
```

> **{hero.tagline}**
> {hero.pitch}

| Cartouche | Couleur | Zone | Difficulté |
|---|---|---|---|
| `{hero.rom}` (réf. `{hero.ref}`) | {hero.color_name} (`{hero.color}`) | {hero.zone} | {'★' * hero.difficulty}{'☆' * (3 - hero.difficulty)} {DIFFICULTY[hero.difficulty]} |

---

## 1. Histoire : {hero.name}, {hero.title[0].lower() + hero.title[1:]}

{hero.lore}

*Idéal pour {hero.ideal}*

---

## 2. Fiche du héros

* **Nom :** {hero.name} (*{hero.english}*)
* **Points de vie :** {hero.hp} PV
* **Or de départ :** {GOLD} pièces d'or
* **Énergie :** {ENERGY} ⚡ par tour
* **Emplacements de potion :** {POTION_SLOTS}

### Relique de départ : ![{relic['name']}](..{relic['imagePath']}) {relic['name']}

> {md_text(relic.get('descI18n') or relic.get('descriptionClean', ''))}

### Deck de départ ({deck_size} cartes)

{starter}

---

## 3. Mécanique signature : {hero.mechanic_name}

{hero.mechanic_text}

---

## 4. Commandes

Les commandes sont identiques sur les cinq cartouches.

| Écran | Bouton | Action |
|---|---|---|
{controls}

---

## 5. La Tour et l'arène de combat

* **La carte de la Tour :** chaque acte compte 16 étages. Choisissez votre chemin parmi les embranchements : combats, élites, évènements mystères `?`, feux de camp, boutiques, trésors, puis le boss de l'acte. Entre les actes, un **Ancien** vous offre ses bénédictions.
* **L'arène façon Pokémon :** {hero.name} apparaît de dos à gauche, les monstres (jusqu'à trois) à droite sur leurs socles, avec leur **intention** au-dessus de la tête. Barres de PV, dégâts flottants et secousses rythment chaque coup.
* **La main de cartes** s'étale en bas de l'écran : la carte sélectionnée grossit et affiche son effet ; si plusieurs ennemis sont en vie, un curseur de ciblage apparaît.
* **Récompenses :** après chaque combat, choisissez une carte parmi trois pour enrichir votre deck, et récupérez or et potions.
* **Feu de camp :** reposez-vous pour soigner vos PV ou **forgez** une carte (amélioration `+`), avec aperçu de la version améliorée.

---

## 6. Les teintes du héros

Au lancement d'une partie, choisissez la teinte de {hero.name} :

{palettes}

---

## 7. Cartes phares

| Carte | Coût | Type | Effet | Amélioré (`+`) |
|---|:---:|:---:|---|---|
{key_cards}

---

## 8. Catalogue des {len(commons)} cartes communes

Les cartes communes forment le socle de votre deck : ce sont celles que vous verrez le plus souvent en récompense.

| Carte | Coût | Type | Effet | Amélioré (`+`) |
|---|:---:|:---:|---|---|
{common_rows}

L'édition compte **{len(pool)} cartes** au total : {counts['Basic']} de base, {counts['Common']} communes, {counts['Uncommon']} peu communes, {counts['Rare']} rares et {counts['Ancient']} anciennes. Retrouvez-les toutes dans le [tome des cartes]({hero.tome}.md).

---

## 9. Conseils de survie

{tips}

---

## 10. Sauvegarde, Ascension et encyclopédie

* **Sauvegarde sur la cartouche :** la partie est enregistrée automatiquement en SRAM à chaque salle ; choisissez **Continuer** à l'écran titre pour reprendre là où vous vous étiez arrêté. Les réglages (volume de la musique et des effets) sont conservés.
* **Ascension 0 à 10 :** chaque victoire débloque un niveau de difficulté supplémentaire (élites plus fréquents, monstres plus robustes, soins réduits…).
* **Encyclopédie :** cartes, reliques, monstres, potions et Anciens rencontrés sont consignés au fil de vos parties.

---

*Slay the GBA existe en cinq éditions. Les quatre autres : {', '.join(others[:-1])} et {others[-1]}. [Voir la collection des livrets](README.md).*
"""


def index(relics: dict[str, dict]) -> str:
    rows = "\n".join(
        f"| `{h.ref}` | **{h.edition}** (*{h.english}*) | {h.color_name} (`{h.color}`) | {h.zone} | {h.hp} PV | "
        f"{relics[h.relic_id]['name']} | {h.mechanic_name} | {'★' * h.difficulty}{'☆' * (3 - h.difficulty)} | "
        f"[Livret](livret_{h.slug}.md) |"
        for h in HEROES
    )
    taglines = "\n".join(f"* **{h.name}** — *{h.tagline}*" for h in HEROES)
    return f"""# Collection des livrets — Slay the GBA, 5 éditions

```
====================================================================================
   SLAY THE GBA — COLLECTION DES LIVRETS D'INSTRUCTIONS · 5 ÉDITIONS
====================================================================================
```

Slay the GBA réinvente **Slay the Spire 2** sur Game Boy Advance, à la manière des grandes séries portables
vendues en plusieurs versions : **une cartouche par héros**. Chaque édition a son héros, son deck, sa relique
de départ, sa couleur, sa zone et sa mécanique signature. La Tour, les monstres, les reliques partagées, les
potions, les évènements et les Anciens sont communs aux cinq.

{taglines}

## Charte des 5 éditions

| Réf. | Édition | Couleur | Zone | PV | Relique de départ | Mécanique | Difficulté | Livret |
|---|---|---|---|:---:|---|---|:---:|:---:|
{rows}

Toutes les éditions partagent : {GOLD} pièces d'or au départ, {ENERGY} ⚡ par tour, {POTION_SLOTS} emplacements de
potion, 4 teintes de héros, l'Ascension 0 à 10 et la sauvegarde sur cartouche.

## Commandes communes

| Écran | Bouton | Action |
|---|---|---|
{chr(10).join(f"| {s} | **{b}** | {a} |" for s, b, a in CONTROLS)}

---

*Ces livrets sont générés par `scripts/build_livrets.py` à partir de `scripts/editions.py` et des données de la
bible : modifiez ces sources plutôt que les fichiers Markdown.*
"""


def main() -> None:
    cards = load("card", "cards")
    relics = load("relic", "relics")
    for hero in HEROES:
        (OUT / f"livret_{hero.slug}.md").write_text(booklet(hero, cards, relics))
    (OUT / "README.md").write_text(index(relics))
    print("Booklets generated in", OUT)


if __name__ == "__main__":
    main()
