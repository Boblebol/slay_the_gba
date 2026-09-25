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

| Button | Action |
|---|---|
| **D-Pad Left / Right** | Select card in hand |
| **A** | Play selected card |
| **START** or **R** | End player turn |
| **SELECT** | Restart / New combat |

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
