# Contributing to Slay the GBA

Thanks for your interest! *Les contributions en français sont les bienvenues : le jeu, les issues et
[`AGENTS.md`](AGENTS.md) sont en français.* English is fine too.

## Report a bug

The game is at 100% of its content and needs playtesters most of all.

1. Check the [open issues](https://github.com/Boblebol/slay_the_gba/issues) in case it is already known.
2. Open the [bug report form](https://github.com/Boblebol/slay_the_gba/issues/new?template=bug_report.yml): game
   version (shown on the title screen and in the release name), hero, emulator or console, what you did and what
   happened.
3. A screenshot, a short video or your save file (`.sav`, or the emulator's save state) helps a lot.

Balance feedback, text that overflows the screen and translation mistakes are welcome as issues too.

## Build and test

Requirements: `make`, a C++20 `clang++` for the native tests, [uv](https://docs.astral.sh/uv/) for the Python
scripts, and devkitARM for the ROMs (the Makefile falls back to the `devkitpro/devkitarm` Docker image when it is
not installed).

```bash
git clone --recursive https://github.com/Boblebol/slay_the_gba
make test test-e2e                                                  # engine tests, no GBA needed
uv run --with pytest --with pillow --with numpy --with markdown pytest scripts   # script tests
make ironclad                                                       # one cartridge (or: make roms)
```

To see the game without a window (screenshots of a pull request):

```bash
uv run scripts/setup_headless_mgba.py
uv run scripts/gba_headless.py slay_ironclad.gba wait:300 A:90 A:90 shot:combat.png
```

Where things live: game rules in `src/engine/` (hardware-agnostic, compiled natively for the tests), the GBA
presentation layer in `src/gba/`, the reference data in `bible/`, the asset and site generators in `scripts/`.

## Pull requests

The full rules are in [`AGENTS.md`](AGENTS.md) (in French). In short:

* One issue, one branch (`feat/<issue number>-<slug>`), one pull request whose description says
  `Closes #<number>`. Do not mix unrelated changes.
* Do not commit `site/`, `AVANCEMENT.md` or `VERSION`: the release workflow regenerates them on `main`.
* Run the tests above before pushing. CI must be green, and the branch up to date with `main`, before a merge.
* A change to what the GBA displays needs screenshots in the pull request.
* Values follow the reference game (Slay the Spire 2, patch noted in `bible/`). When a value is a guess, say so
  in the PR.

`main` is protected: every change goes through a pull request and CI. Releases are automatic: each merge that
changes the game builds the five ROMs and publishes a GitHub release.

## Content and rights

Slay the GBA reproduces Slay the Spire 2 content (names, texts, artwork) owned by Mega Crit, as an unofficial,
free fan project (see the [legal notice](README.md#%EF%B8%8F-legal-notice)). Do not add content from other games
or third-party assets whose license does not allow it, and never anything Nintendo-owned (BIOS, official
SDK code).

## Code of conduct

Everyone taking part follows the [code of conduct](CODE_OF_CONDUCT.md).
