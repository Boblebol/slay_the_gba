# /// script
# requires-python = ">=3.11"
# dependencies = ["numpy>=1.26"]
# ///
"""Synthesise every GBA audio asset of the game.

Run from the repository root:  uv run scripts/build_gba_audio.py

Sound effects are rendered as 8-bit mono WAV files and background music as 4-channel ProTracker
modules (.mod), both played by Butano through maxmod. Everything is generated from simple
oscillators and hand-written note sequences, so the audio stays faithful to the GBA's chiptune
palette and the repository never ships third-party audio.
Outputs land in audio/ (Butano does not scan sub-folders).
"""

from __future__ import annotations

import struct
import wave
from collections.abc import Callable
from dataclasses import dataclass
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


def sfx_text_blip() -> np.ndarray:
    """Soft, very short tick: the voice of a dialogue box typing a letter."""
    length = len(timeline(0.022))
    return 0.3 * triangle(note("E5"), length) * decay(length, 90)


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
    "sfx_text_blip": sfx_text_blip,
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


# ------------------------------------------------------------------------------------------------
# Background music: ProTracker modules
# ------------------------------------------------------------------------------------------------
#
# A row is a sixteenth note, a bar is 16 rows and a pattern holds 4 bars. Four channels:
#   1. lead melody   2. chord arpeggio   3. bass   4. drums
# Tonal samples are single looped waveform cycles: 32 bytes for lead/arpeggio, 64 bytes for the
# bass (one octave lower). With ProTracker periods, C-2 (period 428) plays a 32-byte cycle at
# about 259 Hz, i.e. musical C4, so musical octaves 3..5 map to periods octaves 1..3.

PAL_CLOCK = 7093789.2
DRUM_RATE = int(PAL_CLOCK / (2 * 428))  # drums are triggered at C-2
ROWS_PER_BAR = 16
BARS_PER_PATTERN = 4

PERIODS = [
    856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453,
    428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226,
    214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113,
]

NOTE_INDEX = {"C": 0, "C#": 1, "Db": 1, "D": 2, "D#": 3, "Eb": 3, "E": 4, "F": 5, "F#": 6, "Gb": 6,
              "G": 7, "G#": 8, "Ab": 8, "A": 9, "A#": 10, "Bb": 10, "B": 11}

CHORD_INTERVALS = {"": (0, 4, 7), "m": (0, 3, 7), "7": (0, 4, 7, 10), "m7": (0, 3, 7, 10),
                   "dim": (0, 3, 6), "sus": (0, 5, 7)}

# Sample slots (1-based in the module).
LEAD, ARP, BASS, KICK, SNARE, HAT, PAD = 1, 2, 3, 4, 5, 6, 7


def pitch(name: str) -> int:
    """Semitone number of a note name such as 'F#4' (C0 = 0)."""
    return 12 * int(name[-1]) + NOTE_INDEX[name[:-1]]


def period_for(semitone: int, cycle_octave_shift: int) -> int:
    """Period playing `semitone` on a sample whose C-2 sounds as C(4 - cycle_octave_shift)."""
    index = semitone - 12 * (4 - cycle_octave_shift) + 12  # musical C4 -> index 12 (C-2)
    if not 0 <= index < len(PERIODS):
        raise ValueError(f"note out of ProTracker range: {semitone}")
    return PERIODS[index]


def parse_chord(name: str) -> tuple[int, tuple[int, ...]]:
    root = name[:2] if len(name) > 1 and name[1] in "#b" else name[:1]
    return NOTE_INDEX[root], CHORD_INTERVALS[name[len(root):]]


def parse_melody(text: str) -> list[tuple[int | None, int]]:
    """'E5:4 -:2 G5:2' -> [(semitone, rows), (None, rows), ...]."""
    events = []
    for token in text.split():
        name, rows = token.split(":")
        events.append((None if name == "-" else pitch(name), int(rows)))
    return events


@dataclass
class Song:
    name: str
    speed: int          # ticks per row (6 = standard)
    tempo: int          # BPM for 4 rows per beat at speed 6 (Fxx >= 32)
    chords: list[str]   # one chord per bar
    melody: str
    arp: str            # "eighths", "sixteenths" or "pad"
    bass: str           # "whole", "quarters", "eighths" or "octaves"
    drums: str | None   # 16 characters per bar: k kick, s snare, h hat, x kick+hat, . rest
    arp_octave: int = 4
    bass_octave: int = 2


@dataclass
class Cell:
    sample: int = 0
    period: int = 0
    effect: int = 0
    param: int = 0

    def pack(self) -> bytes:
        return bytes([(self.sample & 0xF0) | (self.period >> 8), self.period & 0xFF,
                      ((self.sample & 0x0F) << 4) | self.effect, self.param])


def render_song(song: Song) -> list[list[list[Cell]]]:
    rows = len(song.chords) * ROWS_PER_BAR
    grid = [[Cell() for _ in range(4)] for _ in range(rows)]

    # Lead melody
    row = 0
    for semitone, length in parse_melody(song.melody):
        if semitone is not None and row < rows:
            grid[row][0] = Cell(LEAD, period_for(semitone, 0))
            if length >= 4:  # long notes fade a little so phrases breathe
                for tail in range(row + 2, min(row + min(length, 6), rows)):
                    grid[tail][0].effect, grid[tail][0].param = 0xA, 0x01
        elif semitone is None and row < rows:
            grid[row][0] = Cell(0, 0, 0xC, 0)  # rest: silence the previous note
        row += length
    if row != rows:
        raise ValueError(f"{song.name}: melody lasts {row} rows, expected {rows}")

    for bar, chord in enumerate(song.chords):
        root, intervals = parse_chord(chord)
        base = bar * ROWS_PER_BAR
        tones = [12 * song.arp_octave + root + i for i in intervals]
        tones.append(tones[0] + 12)

        # Arpeggio
        if song.arp == "pad":
            for step, offset in enumerate((0, 8)):
                grid[base + offset][1] = Cell(PAD, period_for(tones[step % len(tones)], 0))
        else:
            step_rows = 2 if song.arp == "eighths" else 1
            for index, offset in enumerate(range(0, ROWS_PER_BAR, step_rows)):
                tone = tones[index % len(tones)]
                grid[base + offset][1] = Cell(ARP, period_for(tone, 0), 0xE, 0xC0 | (song.speed // 2 + 1))

        # Bass
        bass_root = 12 * song.bass_octave + root
        fifth = bass_root + intervals[2]
        pattern = {
            "whole": [(0, bass_root)],
            "quarters": [(0, bass_root), (4, fifth), (8, bass_root), (12, fifth)],
            "eighths": [(o, bass_root) for o in range(0, 16, 2)],
            "octaves": [(o, bass_root + (12 if (o // 2) % 2 else 0)) for o in range(0, 16, 2)],
        }[song.bass]
        for offset, tone in pattern:
            grid[base + offset][2] = Cell(BASS, period_for(tone, 1))

        # Drums
        if song.drums:
            for offset, hit in enumerate(song.drums):
                sample = {"k": KICK, "x": KICK, "s": SNARE, "h": HAT}.get(hit)
                if sample:
                    grid[base + offset][3] = Cell(sample, 428)

    # Speed and tempo at the start (drum and bass effect columns are otherwise unused on row 0).
    grid[0][3].effect, grid[0][3].param = 0xF, song.speed
    grid[0][2].effect, grid[0][2].param = 0xF, song.tempo

    patterns = []
    rows_per_pattern = ROWS_PER_BAR * BARS_PER_PATTERN
    for start in range(0, rows, rows_per_pattern):
        chunk = grid[start:start + rows_per_pattern]
        if len(chunk) < rows_per_pattern:
            chunk[-1][3].effect, chunk[-1][3].param = 0xD, 0  # pattern break: loop straight away
            chunk += [[Cell() for _ in range(4)] for _ in range(rows_per_pattern - len(chunk))]
        patterns.append(chunk)
    return patterns


def cycle(kind: str, length: int) -> np.ndarray:
    t = np.arange(length) / length
    if kind == "square25":
        wave_ = np.where(t < 0.25, 1.0, -1.0)
    elif kind == "square50":
        wave_ = np.where(t < 0.5, 1.0, -1.0)
    elif kind == "triangle":
        wave_ = 4.0 * np.abs(t - 0.5) - 1.0
    else:  # "bass": triangle with a touch of square bite
        wave_ = 0.75 * (4.0 * np.abs(t - 0.5) - 1.0) + 0.25 * np.where(t < 0.5, 1.0, -1.0)
    return wave_


def drum(kind: str) -> np.ndarray:
    n = int(0.18 * DRUM_RATE)
    idx = np.arange(n)
    if kind == "kick":
        freq = 150 * (45 / 150) ** (idx / n)
        body = np.sin(2 * np.pi * np.cumsum(freq) / DRUM_RATE)
        return body * np.exp(-idx / (0.06 * DRUM_RATE))
    if kind == "snare":
        tone = np.sin(2 * np.pi * 190 * idx / DRUM_RATE) * np.exp(-idx / (0.03 * DRUM_RATE))
        return 0.5 * tone + 0.8 * RNG.uniform(-1, 1, n) * np.exp(-idx / (0.05 * DRUM_RATE))
    n = int(0.05 * DRUM_RATE)
    return RNG.uniform(-1, 1, n) * np.exp(-np.arange(n) / (0.012 * DRUM_RATE))


def to_signed_bytes(signal: np.ndarray) -> bytes:
    data = np.round(np.clip(signal, -1, 1) * 127).astype(np.int8).tobytes()
    return data + (b"\0" if len(data) % 2 else b"")  # sample lengths are counted in words


def module_samples() -> list[tuple[str, bytes, int, bool]]:
    """(name, signed 8-bit data, default volume 0-64, looped) for each module sample slot."""
    return [
        ("lead", to_signed_bytes(cycle("square25", 32)), 34, True),
        ("arp", to_signed_bytes(cycle("square50", 32)), 18, True),
        ("bass", to_signed_bytes(cycle("bass", 64)), 48, True),
        ("kick", to_signed_bytes(drum("kick")), 56, False),
        ("snare", to_signed_bytes(drum("snare")), 36, False),
        ("hat", to_signed_bytes(drum("hat")), 18, False),
        ("pad", to_signed_bytes(cycle("triangle", 32)), 30, True),
    ]


def write_mod(path: Path, song: Song) -> None:
    patterns = render_song(song)
    samples = module_samples()
    header = song.name.encode("ascii")[:20].ljust(20, b"\0")
    for index in range(31):
        if index < len(samples):
            name, data, volume, looped = samples[index]
            words = len(data) // 2
            header += name.encode("ascii").ljust(22, b"\0")
            header += struct.pack(">HBBHH", words, 0, volume, 0, words if looped else 1)
        else:
            header += b"\0" * 22 + struct.pack(">HBBHH", 0, 0, 0, 0, 1)
    header += bytes([len(patterns), 127])
    header += bytes(range(len(patterns))).ljust(128, b"\0")
    header += b"M.K."
    body = b"".join(cell.pack() for pattern in patterns for row in pattern for cell in row)
    path.write_bytes(header + body + b"".join(data for _, data, _, _ in samples))


SONGS = [
    Song(
        "bgm_title", speed=8, tempo=125,
        chords=["Am", "Am", "F", "F", "Dm", "Dm", "E", "E"],
        melody="E5:8 D5:4 C5:4 B4:8 A4:8 C5:8 D5:4 E5:4 F5:12 E5:4 "
               "D5:8 C5:4 A4:4 F5:8 E5:4 D5:4 E5:8 G#4:8 B4:16",
        arp="eighths", bass="whole", drums=None),
    Song(
        "bgm_map", speed=6, tempo=118,
        chords=["Dm", "C", "Bb", "C", "Dm", "C", "Bb", "A"],
        melody="D5:4 F5:4 A5:6 G5:2 E5:4 C5:4 G5:8 F5:4 D5:4 Bb4:4 D5:4 E5:8 C5:8 "
               "D5:4 F5:4 A5:4 G5:4 E5:4 G5:4 C5:8 D5:4 F5:4 Bb4:4 D5:4 C#5:8 A4:8",
        arp="eighths", bass="quarters", drums="k.......s......."),
    Song(
        "bgm_combat", speed=5, tempo=140,
        chords=["Em", "Em", "C", "D", "Em", "Em", "C", "B"],
        melody="E5:2 -:2 E5:2 G5:2 B5:4 A5:2 G5:2 F#5:4 E5:4 D5:4 B4:4 "
               "C5:2 -:2 C5:2 E5:2 G5:4 F#5:2 E5:2 D5:4 F#5:4 A5:8 "
               "B5:4 A5:2 G5:2 F#5:4 E5:4 G5:4 F#5:2 E5:2 D5:4 B4:4 "
               "C5:4 E5:4 G5:4 E5:4 D#5:8 F#5:8",
        arp="sixteenths", bass="octaves", drums="k.h.s.h.k.k.s.h."),
    Song(
        "bgm_elite", speed=5, tempo=150,
        chords=["Em", "G", "Am", "B", "Em", "G", "Am", "B7"],
        melody="B4:2 E5:2 G5:2 B5:2 A5:4 G5:4 F#5:2 G5:2 A5:4 D5:8 "
               "C5:2 E5:2 A5:4 G5:2 F#5:2 E5:4 D#5:4 F#5:4 B5:8 "
               "B5:2 A5:2 G5:4 E5:4 G5:4 A5:4 G5:4 D5:8 E5:4 C5:4 A4:4 C5:4 B4:16",
        arp="sixteenths", bass="eighths", drums="k.hks.h.k.hks.hs"),
    Song(
        "bgm_boss", speed=4, tempo=140,
        chords=["Cm", "Cm", "Ab", "Bb", "Cm", "Cm", "Ab", "G"],
        melody="C5:2 D#5:2 G5:2 C5:2 D#5:2 G5:2 A#5:2 G5:2 C5:4 -:2 C5:2 D5:2 D#5:2 F5:2 G5:2 "
               "G#5:4 G5:4 D#5:4 C5:4 A#4:2 D5:2 F5:2 A#5:2 G#5:4 G5:4 "
               "C5:2 D#5:2 G5:2 C5:2 D#5:2 G5:2 A#5:2 G5:2 G5:4 A#5:4 G5:4 D#5:4 "
               "G#5:2 G5:2 F5:2 D#5:2 D5:4 C5:4 B4:8 D5:4 G5:4",
        arp="sixteenths", bass="octaves", drums="k.k.s.k.k.k.s.ks"),
    Song(
        "bgm_victory", speed=6, tempo=125,
        chords=["C", "F", "G", "C", "Am", "F", "G", "C"],
        melody="G4:2 C5:2 E5:2 G5:6 E5:2 G5:2 A5:8 F5:8 G5:4 F5:2 E5:2 D5:8 E5:4 C5:4 G4:8 "
               "A4:2 C5:2 E5:4 A5:8 F5:4 A5:4 C5:8 D5:4 G5:4 B4:4 D5:4 C5:16",
        arp="eighths", bass="quarters", drums="k...s...k...s..."),
    Song(
        "bgm_game_over", speed=9, tempo=110,
        chords=["Am", "Em", "F", "E", "Am", "Dm", "E", "Am"],
        melody="A4:8 C5:8 B4:8 G4:8 A4:4 C5:4 F5:8 E5:16 E5:8 C5:8 D5:4 F5:4 A4:8 G#4:8 B4:8 A4:16",
        arp="pad", bass="whole", drums=None),
]


def build_music() -> None:
    for song in SONGS:
        write_mod(AUDIO_DIR / f"{song.name}.mod", song)
    print(f"{len(SONGS)} music modules written to {AUDIO_DIR.relative_to(ROOT)}/")


def main() -> None:
    AUDIO_DIR.mkdir(exist_ok=True)
    build_sound_effects()
    build_music()


if __name__ == "__main__":
    main()
