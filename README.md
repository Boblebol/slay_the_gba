# Slay The GBA 🗡️🃏

A faithful **Slay the Spire** demake for the **Nintendo Game Boy Advance**, built with modern **C++20** and the **Butano** engine.

---

## 🎮 Features & MVP

* **Decoupled Combat Engine**: Hardware-agnostic game logic running on both real GBA hardware and host terminal (Mac/Linux).
* **Ironclad Starter Deck**:
  * `Strike` (1 Energy, 6 Damage)
  * `Defend` (1 Energy, 5 Block)
  * `Bash` (2 Energy, 8 Damage + 2 Vulnerable)
  * Bonus cards: `Iron Wave`, `Cleave`, `Anger`
* **Status Effect System**:
  * Vulnerable (+50% damage taken)
  * Weak (-25% damage dealt)
  * Strength (+flat damage bonus)
  * Ritual (Cultist gains strength at end of turn)
* **Classic Enemies**:
  * **Cultist** (*"Caw Caaw!"* with Incantation & Dark Strike)
  * **Jaw Worm** (Chomp, Thrash, Bellow)
* **Burning Blood Relic**: Heals 6 HP at the end of combat.
* **GBA Interface (240x160)**:
  * Top HUD: Player & Enemy HP, Block shields, Energy meter.
  * Intent display: Enemy projected attack/buff displayed above the enemy.
  * Detailed card inspector & hand selector.

---

## 🕹️ Controls (GBA)

| Screen | Button | Action |
|---|---|---|
| Map | **D-Pad Left / Right** | Choose the next room |
| Map | **D-Pad Up / Down** | Scroll the tower |
| Map | **A** | Walk to the selected room |
| Combat | **D-Pad Left / Right** | Select a card in hand |
| Combat | **A** | Play the card (then pick a target when several enemies are alive) |
| Combat | **START** or **R** | End the turn |
| Map / Combat | **SELECT** | Open the deck viewer (**B** to go back) |
| Menus | **A / B** | Confirm / cancel |

---

## 🎨 Art Pipeline

Sprites, backgrounds and the French-capable font are generated from the `bible/images/` dump
(card art, monster & boss art, event paintings, card frames):

```bash
uv run scripts/build_gba_art.py
```

Everything lands in `graphics/` (plus `include/gba/slay_font.h`). The GBA presentation layer lives in
`src/gba/` (one class per scene, cached text labels, tweened animations and fades); the game rules stay in
the hardware-agnostic `src/engine/`.

---

## 🛠️ Quick Start & Build

The build system supports **automatic Docker fallback** (via OrbStack/Docker with `devkitpro/devkitarm`), so you don't even need devkitARM installed on your host machine!

### 1. Build the GBA ROM
```bash
make
```
This generates `slay_the_gba.gba` (approx 110 KB), ready to be loaded into any GBA emulator (like **mGBA**) or flashed onto an authentic flashcart (EZ-Flash / EverDrive).

### 2. Play in Terminal (Native CLI Prototype)
Want to test the card logic and play immediately without launching an emulator?
```bash
make play
```

### 3. Run Native Unit Tests
```bash
make test
```

---

## 📁 Project Structure

```
slay_the_gba/
├── include/
│   └── engine/           # Pure C++20 core game engine (cards, entities, combat)
├── src/
│   ├── engine/           # Card & Entity & Combat logic
│   └── main.cpp          # GBA presentation layer (Butano loop, keypad, text HUD)
├── tests/
│   ├── cli_test.cpp      # Automated unit test suite
│   └── interactive_cli.cpp # Playable terminal game runner
├── butano/               # Submodule: Butano GBA modern C++ framework
├── Makefile              # Dual-target build system (GBA ROM + native runner)
└── slay_the_gba.gba      # Compiled Game Boy Advance ROM
```

---

## 📖 Inspiration & Credits
* [Tonc: GBA Programming Guide](https://www.coranac.com/tonc/text/) by J Vijn
* [Butano C++ Engine](https://github.com/GValiente/butano) by Gustavo Valiente
* Original game design & mechanics by Mega Crit Games (*Slay the Spire*)

## 🌐 Website

A static fan site (landing page, one page per cartridge with every card, bestiary, relics, potions,
events, ancients and the full design bible) is generated into `site/`:

```bash
make site          # build and open in the browser (make site-build: build only)
```

Deploy to Netlify (the ROMs are built locally, so the site is deployed from your machine):

```bash
make roms                                   # build the 5 cartridges (download links)
npx netlify-cli login && npx netlify-cli init   # first time only: link a Netlify site
make deploy-preview                         # draft URL to check
make deploy                                 # production
```

`make site-dist` assembles `dist/` (pages + bible images + ROMs); `netlify.toml` publishes it and
serves `/roms/*.gba` as downloads.
