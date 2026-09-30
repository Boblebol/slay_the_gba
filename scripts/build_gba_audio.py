# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy>=1.26"]
# ///
"""Synthesise every GBA audio asset of the game.

Run from the repository root:  uv run scripts/build_gba_audio.py

Sound effects are rendered as 8-bit mono WAV files (Butano plays them through maxmod).
Everything is generated from simple oscillators so the sounds stay faithful to the GBA's
chiptune palette and the repository never ships third-party audio.
Outputs land in audio/ (Butano does not scan sub-folders).
"""

from __future__ import annotations

import wave
from collections.abc import Callable
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parent.parent
AUDIO_DIR = ROOT / "audio"

SFX_RATE = 16384  # Hz: low enough to keep the ROM small, high enough for crisp blips
RNG = np.random.default_rng(7)


# ------------------------------------------------------------------------------------------------
# Oscillators and envelopes
# ------------------------------------------------------------------------------------------------

def timeline(seconds: float) -> np.ndarray:
    return np.arange(int(seconds * SFX_RATE)) / SFX_RATE


def phase_of(freq: np.ndarray | float, length: int) -> np.ndarray:
    """Integrated phase (in cycles) of a possibly time-varying frequency."""
    freqs = np.broadcast_to(np.asarray(freq, dtype=np.float64), (length,))
    return np.cumsum(freqs) / SFX_RATE


def square(freq: np.ndarray | float, length: int, duty: float = 0.5) -> np.ndarray:
    return np.where((phase_of(freq, length) % 1.0) < duty, 1.0, -1.0)


def triangle(freq: np.ndarray | float, length: int) -> np.ndarray:
    return 4.0 * np.abs((phase_of(freq, length) % 1.0) - 0.5) - 1.0


def sine(freq: np.ndarray | float, length: int) -> np.ndarray:
    return np.sin(2 * np.pi * phase_of(freq, length))


def noise(length: int, hold: int = 1) -> np.ndarray:
    """White noise; `hold` > 1 repeats samples, lowering the pitch like the GB noise channel."""
    samples = RNG.uniform(-1.0, 1.0, (length + hold - 1) // hold)
    return np.repeat(samples, hold)[:length]


def lowpass(signal: np.ndarray, strength: float) -> np.ndarray:
    """One-pole low-pass filter; strength in (0, 1], lower is darker."""
    out = np.empty_like(signal)
    acc = 0.0
    for index, value in enumerate(signal):
        acc += strength * (value - acc)
        out[index] = acc
    return out


def decay(length: int, speed: float) -> np.ndarray:
    return np.exp(-speed * np.arange(length) / SFX_RATE)


def attack_release(length: int, attack: float = 0.004, release: float = 0.02) -> np.ndarray:
    """Short linear fades at both ends so no sound starts or stops with a click."""
    env = np.ones(length)
    a = max(1, int(attack * SFX_RATE))
    r = max(1, int(release * SFX_RATE))
    env[:a] = np.linspace(0.0, 1.0, a)
    env[-r:] *= np.linspace(1.0, 0.0, r)
    return env


def sweep(start: float, end: float, length: int, curve: float = 1.0) -> np.ndarray:
    t = np.linspace(0.0, 1.0, length) ** curve
    return start * (end / start) ** t


def notes(sequence: list[tuple[float, float]], voice: Callable[[float, int], np.ndarray],
          speed: float = 18.0) -> np.ndarray:
    """Concatenates (frequency, seconds) notes, each with its own decaying envelope."""
    parts = []
    for freq, seconds in sequence:
        length = int(seconds * SFX_RATE)
        parts.append(voice(freq, length) * decay(length, speed) * attack_release(length, 0.002, 0.01))
    return np.concatenate(parts)


def note(name: str) -> float:
    """Frequency of a note such as 'C5' or 'F#4'."""
    semitones = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8,
                 "A": 9, "A#": 10, "B": 11}
    pitch, octave = name[:-1], int(name[-1])
    midi = 12 * (octave + 1) + semitones[pitch]
    return 440.0 * 2 ** ((midi - 69) / 12)


# ------------------------------------------------------------------------------------------------
# Sound effects
# ------------------------------------------------------------------------------------------------

def sfx_ui_cursor() -> np.ndarray:
    length = len(timeline(0.035))
    return 0.5 * square(note("A5"), length, 0.25) * decay(length, 60)


def sfx_ui_confirm() -> np.ndarray:
    return 0.55 * notes([(note("E5"), 0.05), (note("B5"), 0.09)], lambda f, n: square(f, n, 0.25), 20)


def sfx_ui_cancel() -> np.ndarray:
    return 0.55 * notes([(note("B4"), 0.05), (note("E4"), 0.09)], lambda f, n: square(f, n, 0.5), 20)


def sfx_ui_open() -> np.ndarray:
    seq = [(note(n), 0.045) for n in ("C5", "E5", "G5")] + [(note("C6"), 0.12)]
    return 0.5 * notes(seq, triangle, 14)


def sfx_card_play() -> np.ndarray:
    # Rising filtered noise: a card swept through the air.
    length = len(timeline(0.14))
    brightness = np.linspace(0.08, 0.5, length)
    swish = np.empty(length)
    acc = 0.0
    for index, value in enumerate(noise(length)):
        acc += brightness[index] * (value - acc)
        swish[index] = acc
    env = np.sin(np.linspace(0, np.pi, length)) ** 1.5
    return 0.9 * swish * env


def sfx_hit_slash() -> np.ndarray:
    length = len(timeline(0.16))
    blade = square(sweep(1400, 300, length, 0.6), length, 0.125) * decay(length, 30)
    hiss = noise(length) * decay(length, 22)
    return 0.35 * blade + 0.55 * hiss


def sfx_hit_heavy() -> np.ndarray:
    length = len(timeline(0.26))
    thud = sine(sweep(160, 40, length, 0.5), length) * decay(length, 12)
    crunch = lowpass(noise(length, 3), 0.35) * decay(length, 25)
    return np.clip(0.9 * thud + 0.6 * crunch, -1, 1)


def sfx_hit_blunt() -> np.ndarray:
    length = len(timeline(0.18))
    body = square(sweep(320, 90, length, 0.5), length, 0.5) * decay(length, 22)
    knock = lowpass(noise(length, 2), 0.5) * decay(length, 40)
    return 0.45 * body + 0.5 * knock


def sfx_block() -> np.ndarray:
    # Metallic ping: two detuned square waves ringing out.
    length = len(timeline(0.25))
    ring = square(note("E6"), length, 0.25) + 0.6 * square(note("E6") * 1.41, length, 0.5)
    clank = noise(length) * decay(length, 90)
    return 0.3 * ring * decay(length, 16) + 0.4 * clank


def sfx_gold() -> np.ndarray:
    return 0.5 * notes([(note("B5"), 0.06), (note("E6"), 0.22)], lambda f, n: square(f, n, 0.5), 12)


def sfx_relic() -> np.ndarray:
    seq = [(note(n), 0.05) for n in ("G5", "B5", "D6", "G6", "B6")] + [(note("D7"), 0.2)]
    return 0.4 * notes(seq, lambda f, n: square(f, n, 0.125), 10)


def sfx_heal() -> np.ndarray:
    seq = [(note(n), 0.08) for n in ("C5", "E5", "G5", "C6")] + [(note("E6"), 0.25)]
    return 0.6 * notes(seq, triangle, 7)


def sfx_discard() -> np.ndarray:
    length = len(timeline(0.09))
    return 0.5 * lowpass(noise(length), 0.3) * np.linspace(1, 0, length) ** 2


def sfx_enemy_death() -> np.ndarray:
    length = len(timeline(0.55))
    fall = square(sweep(700, 60, length, 0.7), length, 0.5) * decay(length, 5)
    crumble = lowpass(noise(length, 4), 0.4) * decay(length, 6)
    return 0.35 * fall + 0.5 * crumble


def sfx_buff() -> np.ndarray:
    length = len(timeline(0.22))
    rise = square(sweep(300, 1200, length, 1.0), length, 0.25)
    return 0.4 * rise * np.linspace(1, 0.2, length) * attack_release(length)


def sfx_debuff() -> np.ndarray:
    length = len(timeline(0.24))
    wobble = sweep(700, 200, length, 1.0) * (1 + 0.08 * np.sin(2 * np.pi * 30 * timeline(0.24)[:length]))
    return 0.4 * square(wobble, length, 0.5) * np.linspace(1, 0.2, length) * attack_release(length)


def sfx_turn_start() -> np.ndarray:
    return 0.45 * notes([(note("D5"), 0.06), (note("A5"), 0.12)], triangle, 14)


def sfx_error() -> np.ndarray:
    length = len(timeline(0.16))
    return 0.45 * square(note("C3"), length, 0.5) * attack_release(length)


SOUND_EFFECTS: dict[str, Callable[[], np.ndarray]] = {
    "sfx_ui_cursor": sfx_ui_cursor,
    "sfx_ui_confirm": sfx_ui_confirm,
    "sfx_ui_cancel": sfx_ui_cancel,
    "sfx_ui_open": sfx_ui_open,
    "sfx_card_play": sfx_card_play,
    "sfx_hit_slash": sfx_hit_slash,
    "sfx_hit_heavy": sfx_hit_heavy,
    "sfx_hit_blunt": sfx_hit_blunt,
    "sfx_block": sfx_block,
    "sfx_gold": sfx_gold,
    "sfx_relic": sfx_relic,
    "sfx_heal": sfx_heal,
    "sfx_discard": sfx_discard,
    "sfx_enemy_death": sfx_enemy_death,
    "sfx_buff": sfx_buff,
    "sfx_debuff": sfx_debuff,
    "sfx_turn_start": sfx_turn_start,
    "sfx_error": sfx_error,
}


def write_wav_8bit(path: Path, signal: np.ndarray, rate: int) -> None:
    """Writes unsigned 8-bit mono PCM, the most compact format maxmod accepts."""
    tail = np.zeros(int(0.01 * rate))
    samples = np.clip(np.concatenate([signal, tail]), -1.0, 1.0)
    pcm = np.round(samples * 127 + 128).astype(np.uint8)
    with wave.open(str(path), "wb") as out:
        out.setnchannels(1)
        out.setsampwidth(1)
        out.setframerate(rate)
        out.writeframes(pcm.tobytes())


def build_sound_effects() -> None:
    for name, render in SOUND_EFFECTS.items():
        write_wav_8bit(AUDIO_DIR / f"{name}.wav", render(), SFX_RATE)
    print(f"{len(SOUND_EFFECTS)} sound effects written to {AUDIO_DIR.relative_to(ROOT)}/")


def main() -> None:
    AUDIO_DIR.mkdir(exist_ok=True)
    build_sound_effects()


if __name__ == "__main__":
    main()
