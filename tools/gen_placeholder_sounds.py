#!/usr/bin/env python3
"""Generates 1stPRTS placeholder sounds and music into data/base/sounds/ and data/base/music/.

The script is MIT-licensed code; the audio it produces is original placeholder work released
under CC BY 4.0 (credited in data/base/CREDITS.md). Only the standard library is used and the
output is deterministic (fixed seeds). Music is encoded to OGG Vorbis with `oggenc` from
vorbis-tools (`apt-get install -y vorbis-tools`), with a fixed stream serial number.

usage: gen_placeholder_sounds.py            write the game's sounds and music
       gen_placeholder_sounds.py --fixtures DIR   write the engine's audio test fixtures"""
import math
import os
import random
import shutil
import struct
import subprocess
import sys
import tempfile
import wave

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SOUNDS = os.path.join(ROOT, "data", "base", "sounds")
MUSIC = os.path.join(ROOT, "data", "base", "music")
RATE = 22050
TAU = 2.0 * math.pi


# --- output -------------------------------------------------------------------------------

def write_wav(path, channels, rate, frames, bits=16):
    """frames: list of samples (mono) or of (left, right) pairs, floats in -1..1."""
    data = bytearray()
    for f in frames:
        for s in (f if channels == 2 else (f,)):
            s = max(-1.0, min(1.0, s))
            if bits == 16:
                data += struct.pack("<h", int(round(s * 32767)))
            else:
                data += struct.pack("<B", int(round(s * 127)) + 128)
    with wave.open(path, "wb") as w:
        w.setnchannels(channels)
        w.setsampwidth(bits // 8)
        w.setframerate(rate)
        w.writeframes(bytes(data))


def encode_ogg(path, channels, rate, frames, quality):
    oggenc = shutil.which("oggenc")
    if not oggenc:
        sys.exit("oggenc not found: install vorbis-tools (apt-get install -y vorbis-tools)")
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "in.wav")
        write_wav(src, channels, rate, frames)
        subprocess.run([oggenc, "--quiet", "-q", str(quality), "-s", "1407", "-o", path, src], check=True)


def fade_tail(x, seconds=0.015):
    """Ramps the last samples of a one-shot sound to silence, so it never ends with a click."""
    n = min(len(x), int(seconds * RATE))
    for i in range(n):
        x[len(x) - n + i] *= (n - 1 - i) / n
    return x


def normalize(buf, peak=0.8):
    top = max(1e-9, max(abs(s) for s in buf))
    return [s * peak / top for s in buf]


# --- building blocks ------------------------------------------------------------------------

def lowpass(x, cutoff):
    a = 1.0 - math.exp(-TAU * cutoff / RATE)
    y, out = 0.0, []
    for s in x:
        y += a * (s - y)
        out.append(y)
    return out


def highpass(x, cutoff):
    low = lowpass(x, cutoff)
    return [s - l for s, l in zip(x, low)]


def bandpass(x, freq, q):
    """RBJ band-pass biquad with 0 dB peak gain."""
    w0 = TAU * freq / RATE
    alpha = math.sin(w0) / (2.0 * q)
    a0 = 1.0 + alpha
    b0, b2 = alpha / a0, -alpha / a0
    a1, a2 = -2.0 * math.cos(w0) / a0, (1.0 - alpha) / a0
    x1 = x2 = y1 = y2 = 0.0
    out = []
    for s in x:
        y = b0 * s + b2 * x2 - a1 * y1 - a2 * y2
        x2, x1 = x1, s
        y2, y1 = y1, y
        out.append(y)
    return out


def noise(rnd, n):
    return [rnd.uniform(-1.0, 1.0) for _ in range(n)]


def mix_into(buf, start, x, gain=1.0):
    for i, s in enumerate(x):
        j = start + i
        if 0 <= j < len(buf):
            buf[j] += s * gain


def partials(n, freqs, decays, amps, phase=0.0):
    out = [0.0] * n
    for f, d, a in zip(freqs, decays, amps):
        k = TAU * f / RATE
        for i in range(n):
            out[i] += a * math.sin(k * i + phase) * math.exp(-i / (d * RATE))
    return out


def thump(n, f0, f1, decay):
    """A pitch-dropping sine: body of knocks, thuds and drums."""
    out, ph = [], 0.0
    for i in range(n):
        t = i / RATE
        f = f1 + (f0 - f1) * math.exp(-t / 0.03)
        ph += TAU * f / RATE
        out.append(math.sin(ph) * math.exp(-t / decay))
    return out


def seamless(x, overlap):
    """Folds the tail past the loop length back onto the start (x has `overlap` extra samples)."""
    n = len(x) - overlap
    out = x[:n]
    for i in range(overlap):
        f = i / overlap
        out[i] = out[i] * math.sin(f * math.pi / 2) + x[n + i] * math.cos(f * math.pi / 2)
    return out


def creak(rnd, seconds, rate0, rate1, body=(620, 1350, 2400)):
    """Wood creak: a stick-slip impulse train (rate gliding rate0 -> rate1) through body resonances."""
    n = int(seconds * RATE)
    pulses = [0.0] * n
    t = 0.0
    while True:
        u = t / seconds
        rate = rate0 + (rate1 - rate0) * u + 25 * math.sin(u * 17.0)
        t += (1.0 / rate) * rnd.uniform(0.8, 1.2)
        i = int(t * RATE)
        if i >= n:
            break
        pulses[i] = rnd.uniform(0.5, 1.0)
    out = [0.0] * n
    for f, g in zip(body, (1.0, 0.6, 0.3)):
        mix_into(out, 0, bandpass(pulses, f * rnd.uniform(0.97, 1.03), 9.0), g * 3.0)
    for i in range(n):
        u = i / n
        out[i] *= min(1.0, u * 12) * min(1.0, (1 - u) * 5)
    return out


# --- sound effects --------------------------------------------------------------------------

def door_open(rnd):
    out = creak(rnd, 1.0, 70, 150)
    out += [0.0] * int(0.15 * RATE)
    mix_into(out, int(0.02 * RATE), bandpass(noise(rnd, 300), 2500, 3), 0.5)  # latch
    return normalize(out, 0.75)


def door_close(rnd):
    n = int(0.8 * RATE)
    out = [0.0] * n
    mix_into(out, 0, creak(rnd, 0.35, 140, 90), 0.6)
    hit = int(0.36 * RATE)
    mix_into(out, hit, thump(int(0.4 * RATE), 160, 65, 0.09), 1.0)
    mix_into(out, hit, [s * math.exp(-i / (0.03 * RATE)) for i, s in enumerate(lowpass(noise(rnd, 3000), 900))], 1.2)
    mix_into(out, hit + 60, bandpass(noise(rnd, 250), 2800, 4), 0.6)  # latch
    return normalize(out, 0.8)


def lift_clank(rnd):
    n = int(1.3 * RATE)
    out = [0.0] * n
    metal = ([217, 563, 1011, 1597, 2340], [0.5, 0.35, 0.25, 0.15, 0.1], [0.5, 0.8, 0.6, 0.4, 0.3])
    mix_into(out, 0, partials(n, *metal), 1.0)
    mix_into(out, int(0.19 * RATE), partials(n - int(0.19 * RATE), [f * 1.04 for f in metal[0]], *metal[1:]), 0.5)
    mix_into(out, 0, thump(int(0.3 * RATE), 120, 55, 0.08), 0.8)
    rattle = [0.0] * n
    t = 0.05
    while t < 1.1:  # chain links
        mix_into(rattle, int(t * RATE), [math.exp(-i / 40.0) * rnd.uniform(-1, 1) for i in range(200)], rnd.uniform(0.2, 0.6) * (1.2 - t))
        t += rnd.uniform(0.02, 0.07)
    mix_into(out, 0, bandpass(rattle, 3200, 2), 1.5)
    return normalize(out, 0.8)


def lever_click(rnd):
    n = int(0.25 * RATE)
    out = [0.0] * n
    for at, g in ((0.0, 1.0), (0.07, 0.6)):
        burst = [math.exp(-i / 25.0) * rnd.uniform(-1, 1) for i in range(400)]
        mix_into(out, int(at * RATE), bandpass(burst, 2600, 3), g * 2.0)
        mix_into(out, int(at * RATE), partials(1500, [1240, 3100], [0.02, 0.01], [0.4, 0.2]), g)
    return normalize(out, 0.7)


def footstep(rnd):
    n = int(0.3 * RATE)
    out = [0.0] * n
    mix_into(out, 0, thump(n, 140, 70, 0.035), 0.9)
    scuff = lowpass(noise(rnd, n), 1600)
    mix_into(out, 0, [s * math.exp(-i / (0.05 * RATE)) for i, s in enumerate(scuff)], 2.0)
    return normalize(out, 0.6)


def forge_loop(rnd):
    seconds, overlap = 2.4, int(0.3 * RATE)
    n = int(seconds * RATE)
    total = n + overlap
    bed = lowpass(lowpass(noise(rnd, total), 300), 500)  # the hearth's roar
    out = [s * 1.8 * (1.0 + 0.3 * math.sin(TAU * i / total * 3)) for i, s in enumerate(bed)]
    out = seamless(out, overlap)
    ring = ([1180, 2790, 4130, 5260], [0.35, 0.25, 0.15, 0.1], [0.6, 0.5, 0.3, 0.2])
    for at, g in ((0.10, 1.0), (0.70, 0.9), (1.30, 1.0), (1.75, 0.45)):
        hit = partials(int(1.0 * RATE), *ring)
        hit = [s + t * 0.8 for s, t in zip(hit, thump(len(hit), 300, 120, 0.03))]
        start = int(at * RATE)
        for i, s in enumerate(hit):  # strikes ring across the loop point
            out[(start + i) % n] += s * g
    return normalize(out, 0.7)


def torch_loop(rnd):
    seconds, overlap = 2.0, int(0.25 * RATE)
    n = int(seconds * RATE)
    total = n + overlap
    whoosh = lowpass(noise(rnd, total), 400)
    out = [s * 1.5 * (0.8 + 0.2 * math.sin(TAU * i / total * 2)) for i, s in enumerate(whoosh)]
    out = seamless(out, overlap)
    t = 0.0
    while True:
        t += rnd.expovariate(14.0)
        if t >= seconds:
            break
        pop = highpass([math.exp(-i / rnd.uniform(8, 30)) * rnd.uniform(-1, 1) for i in range(160)], 1500)
        start, g = int(t * RATE), rnd.uniform(0.1, 0.7)
        for i, s in enumerate(pop):
            out[(start + i) % n] += s * g
    return normalize(out, 0.6)


def alarm_bell(rnd):
    n = int(2.6 * RATE)
    out = [0.0] * n
    base = 523.0
    ratios = [0.5, 1.0, 1.183, 1.506, 2.0, 2.514, 2.662, 3.011]
    decays = [1.4, 1.1, 0.9, 0.7, 0.8, 0.45, 0.4, 0.3]
    amps = [0.4, 0.7, 0.5, 0.3, 0.5, 0.25, 0.2, 0.15]
    for at in (0.0, 0.65, 1.3):
        start = int(at * RATE)
        mix_into(out, start, partials(n - start, [base * r for r in ratios], decays, amps), 1.0)
        mix_into(out, start, bandpass(noise(rnd, 200), 4000, 2), 0.4)  # clapper
    return normalize(out, 0.75)


def armour_jingle(rnd, n, count, spread):
    """Small rings of mail and buckles: short high bursts scattered over `spread` seconds."""
    out = [0.0] * n
    for _ in range(count):
        at = int(rnd.uniform(0.0, spread) * RATE)
        ring = partials(int(0.08 * RATE), [rnd.uniform(3000, 5200), rnd.uniform(6000, 7500)], [0.02, 0.012], [0.5, 0.3])
        mix_into(out, at, ring, rnd.uniform(0.15, 0.4))
    return out


def land_thud(rnd):
    """Boots coming down hard: a heavy thump, grit and the commander's armour."""
    n = int(0.45 * RATE)
    out = [0.0] * n
    mix_into(out, 0, thump(n, 150, 48, 0.07), 1.0)
    grit = lowpass(noise(rnd, n), 1200)
    mix_into(out, 0, [s * math.exp(-i / (0.06 * RATE)) for i, s in enumerate(grit)], 2.4)
    mix_into(out, int(0.01 * RATE), armour_jingle(rnd, n, 7, 0.12), 1.0)
    return normalize(out, 0.7)


def jump_rustle(rnd):
    """Pushing off: a scuff of the boots and a rustle of cloth and mail."""
    n = int(0.3 * RATE)
    out = [0.0] * n
    mix_into(out, 0, thump(int(0.12 * RATE), 120, 80, 0.02), 0.5)
    cloth = bandpass(noise(rnd, n), 1800, 1.2)
    env = [math.sin(math.pi * min(1.0, i / (0.25 * RATE))) ** 2 for i in range(n)]
    mix_into(out, 0, [s * e for s, e in zip(cloth, env)], 1.2)
    mix_into(out, int(0.03 * RATE), armour_jingle(rnd, n, 4, 0.15), 0.8)
    return normalize(out, 0.5)


def portcullis_slam(rnd):
    """An iron grille hitting the stone sill: a deep boom, ringing bars and rattling chains."""
    n = int(1.6 * RATE)
    out = [0.0] * n
    mix_into(out, 0, thump(n, 95, 32, 0.28), 1.2)
    bars = ([96, 241, 377, 612, 958], [0.6, 0.45, 0.35, 0.25, 0.15], [0.6, 0.7, 0.5, 0.35, 0.2])
    mix_into(out, 0, partials(n, *bars), 0.8)
    rattle = [0.0] * n
    t = 0.02
    while t < 0.9:
        mix_into(rattle, int(t * RATE), [math.exp(-i / 35.0) * rnd.uniform(-1, 1) for i in range(180)], rnd.uniform(0.2, 0.6) * (1.0 - t))
        t += rnd.uniform(0.02, 0.06)
    mix_into(out, 0, bandpass(rattle, 2800, 2), 1.4)
    return normalize(out, 0.85)


def talk_murmur(rnd):
    """A placeholder voice: a short spoken 'hm-hm' (a buzz through two vowel formants)."""
    n = int(0.42 * RATE)
    buzz, ph = [], 0.0
    for i in range(n):
        t = i / RATE
        f = 118 + 14 * math.sin(TAU * 2.3 * t) - 20 * t
        ph += f / RATE
        buzz.append(2.0 * (ph % 1.0) - 1.0)
    voice = [a + b for a, b in zip(bandpass(buzz, 620, 4), bandpass(buzz, 1150, 5))]
    env = [math.sin(math.pi * min(1.0, i / (0.18 * RATE))) ** 2 if i < 0.18 * RATE else
           (math.sin(math.pi * min(1.0, (i - 0.22 * RATE) / (0.2 * RATE))) ** 2 if i > 0.22 * RATE else 0.0)
           for i in range(n)]
    return normalize([v * e for v, e in zip(voice, env)], 0.55)


def discover_chime(rnd):
    """A small hand bell, struck once: a new place found, an order done."""
    n = int(1.8 * RATE)
    out = [0.0] * n
    base = 784.0
    ratios = [1.0, 2.0, 2.76, 5.4]
    mix_into(out, 0, partials(n, [base * r for r in ratios], [0.9, 0.5, 0.35, 0.15], [0.7, 0.35, 0.25, 0.1]), 1.0)
    mix_into(out, 0, bandpass(noise(rnd, 120), 5000, 2), 0.2)
    return normalize(out, 0.5)


EFFECTS = [
    ("door_open", door_open),
    ("door_close", door_close),
    ("lift_clank", lift_clank),
    ("lever_click", lever_click),
    ("footstep", footstep),
    ("forge_loop", forge_loop),
    ("torch_loop", torch_loop),
    ("alarm_bell", alarm_bell),
    ("land_thud", land_thud),
    ("jump_rustle", jump_rustle),
    ("portcullis_slam", portcullis_slam),
    ("talk_murmur", talk_murmur),
    ("discover_chime", discover_chime),
]


# --- music ------------------------------------------------------------------------------------

def midi_hz(n):
    return 440.0 * 2.0 ** ((n - 69) / 12.0)


NOTE = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def parse_note(name):
    """'F#5' -> MIDI number."""
    pitch = NOTE[name[0]]
    rest = name[1:]
    if rest.startswith("#"):
        pitch, rest = pitch + 1, rest[1:]
    elif rest.startswith("b"):
        pitch, rest = pitch - 1, rest[1:]
    return 12 * (int(rest) + 1) + pitch


def parse_line(text):
    """'A4:2 D5:1 | r:3' -> [(beat, beats, midi or None)]."""
    notes, beat = [], 0.0
    for tok in text.replace("|", " ").split():
        name, dur = tok.split(":")
        dur = float(dur)
        notes.append((beat, dur, None if name == "r" else parse_note(name)))
        beat += dur
    return notes


def pluck(rnd, freq, seconds, bright=0.5):
    """Karplus-Strong string (lute/harp)."""
    period = max(2, int(RATE / freq))
    line = [rnd.uniform(-1, 1) for _ in range(period)]
    line = lowpass(line, 2000 + 6000 * bright)
    n = int(seconds * RATE)
    out = [0.0] * n
    damp = 0.996
    j = 0
    for i in range(n):
        nxt = (j + 1) % period
        v = line[j]
        line[j] = damp * 0.5 * (v + line[nxt])
        out[i] = v
        j = nxt
    return out


def fife(rnd, freq, seconds):
    """A breathy wooden flute: sine plus a little second harmonic, vibrato and breath."""
    n = int(seconds * RATE)
    out, ph = [], 0.0
    breath = lowpass(noise(rnd, n), 3000)
    att, rel = int(0.04 * RATE), int(0.08 * RATE)
    for i in range(n):
        t = i / RATE
        vib = 1.0 + 0.006 * math.sin(TAU * 5.2 * t) * min(1.0, t / 0.3)
        ph += TAU * freq * vib / RATE
        env = min(1.0, i / att) * min(1.0, (n - i) / rel)
        out.append(env * (math.sin(ph) + 0.18 * math.sin(2 * ph) + 0.05 * breath[i]))
    return out


def drone(n, freqs, gain):
    """Hurdy-gurdy style drone: buzzy partials that swell slowly. Every frequency is nudged to
    a whole number of periods in n samples, so the drone loops without a seam."""
    out = [0.0] * n
    swell = TAU * 4 / n  # four swells per loop
    for f in freqs:
        for h, a in ((1, 1.0), (2, 0.45), (3, 0.25), (4, 0.12)):
            k = TAU * round(f * h * n / RATE) / n
            for i in range(n):
                out[i] += gain * a * math.sin(k * i) * (0.85 + 0.15 * math.sin(swell * i + f))
    return out


def tabor(rnd, accent):
    """Frame drum: low skin thump plus a short rattle."""
    n = int(0.35 * RATE)
    body = thump(n, 180, 90, 0.08 if accent else 0.04)
    snare = [s * math.exp(-i / (0.025 * RATE)) for i, s in enumerate(bandpass(noise(rnd, n), 2200, 1.5))]
    return [(b * (1.0 if accent else 0.35) + s * (0.5 if accent else 0.7)) for b, s in zip(body, snare)]


def reverb(left, right, wet):
    """A small Schroeder hall (four combs and two all-passes per channel)."""
    def comb(x, d, fb):
        buf, out, j = [0.0] * d, [], 0
        for s in x:
            y = buf[j]
            buf[j] = s + y * fb
            out.append(y)
            j = (j + 1) % d
        return out

    def allpass(x, d, g):
        buf, out, j = [0.0] * d, [], 0
        for s in x:
            b = buf[j]
            y = -g * s + b
            buf[j] = s + g * y
            out.append(y)
            j = (j + 1) % d
        return out

    mono = [(l + r) * 0.5 for l, r in zip(left, right)]
    outs = []
    for combs in ((1116, 1188, 1277, 1356), (1139, 1211, 1300, 1379)):
        acc = [0.0] * len(mono)
        for d in combs:
            for i, s in enumerate(comb(mono, d // 2, 0.80)):
                acc[i] += s * 0.25
        outs.append(allpass(allpass(acc, 278, 0.5), 113, 0.5))
    return ([l + wet * w for l, w in zip(left, outs[0])], [r + wet * w for r, w in zip(right, outs[1])])


def render_song(seed, bpm, beats_per_bar, chords, melody, drone_notes, drums, lute_pattern, lute_gain):
    rnd = random.Random(seed)
    beat_s = 60.0 / bpm
    bars = len(chords)
    loop = int(bars * beats_per_bar * beat_s * RATE)
    tail = int(3.0 * RATE)
    n = loop + tail
    left, right = [0.0] * n, [0.0] * n

    def place(x, beat, gain, pan):
        start = int(beat * beat_s * RATE)
        gl, gr = gain * math.cos((pan + 1) * math.pi / 4), gain * math.sin((pan + 1) * math.pi / 4)
        for i, s in enumerate(x):
            j = start + i
            if j < n:
                left[j] += s * gl
                right[j] += s * gr

    for bar, chord in enumerate(chords):
        root = parse_note(chord[0])
        tones = [root + iv for iv in chord[1]]
        for step, (offset, which) in enumerate(lute_pattern):
            note = tones[which % len(tones)] + 12 * (which // len(tones))
            place(pluck(rnd, midi_hz(note), 1.8, 0.4), bar * beats_per_bar + offset, lute_gain, -0.3 + 0.1 * (step % 3))
        place(pluck(rnd, midi_hz(root - 12), 2.2, 0.2), bar * beats_per_bar, lute_gain * 1.1, 0.1)
    for beat, beats, note in parse_line(melody):
        if note is not None:
            place(fife(rnd, midi_hz(note), beats * beat_s * 0.95), beat, 0.28, 0.25)
    if drums:
        for bar in range(bars):
            for offset, accent in drums:
                place(tabor(rnd, accent), bar * beats_per_bar + offset, 0.5, -0.1)
    # fold notes still ringing past the loop end back onto the start, so the loop is seamless
    for i in range(tail):
        left[i] += left[loop + i]
        right[i] += right[loop + i]
    left, right = left[:loop], right[:loop]
    d = drone(loop, [midi_hz(parse_note(x)) for x in drone_notes], 0.05)
    left = [s + v for s, v in zip(left, d)]
    right = [s + v for s, v in zip(right, d)]
    # run the reverb over two passes of the loop and keep the second: it starts with the
    # reverb state the loop ends with
    left, right = reverb(left + left, right + right, 0.35)
    left, right = left[loop:], right[loop:]
    peak = max(1e-9, max(max(abs(s) for s in left), max(abs(s) for s in right)))
    return [(l * 0.8 / peak, r * 0.8 / peak) for l, r in zip(left, right)]


MINOR, MAJOR = (0, 3, 7), (0, 4, 7)

MENU = dict(
    seed=71, bpm=72, beats_per_bar=3, lute_gain=0.32, drums=None,
    # D dorian: a slow lute and fife air over a drone
    chords=[("D3", MINOR), ("C3", MAJOR), ("D3", MINOR), ("A2", MINOR), ("F2", MAJOR), ("C3", MAJOR),
            ("G2", MAJOR), ("A2", MINOR), ("D3", MINOR), ("C3", MAJOR), ("G2", MAJOR), ("F2", MAJOR),
            ("C3", MAJOR), ("D3", MINOR), ("A2", MINOR), ("D3", MINOR)],
    lute_pattern=[(0.0, 0), (0.5, 1), (1.0, 2), (1.5, 4), (2.0, 3), (2.5, 1)],
    melody="r:3 | r:3 | A4:2 D5:1 | E5:2 C5:1 | F5:1.5 E5:0.5 D5:1 | E5:3 | D5:1 B4:1 G4:1 | A4:3 |"
           " A4:1 D5:1 F5:1 | G5:2 E5:1 | B4:1 D5:1 G5:1 | A5:2 F5:1 | G5:1 E5:1 C5:1 | D5:2 F5:1 |"
           " E5:2 C5:1 | D5:3",
    drone_notes=["D2", "A2"],
)

PROVING_GROUND = dict(
    seed=1407, bpm=104, beats_per_bar=4, lute_gain=0.26,
    # A dorian march: fife over tabor, lute and drone
    drums=[(0, True), (1, False), (1.5, False), (2, True), (3, False), (3.5, False)],
    chords=[("A2", MINOR), ("G2", MAJOR), ("A2", MINOR), ("E2", MINOR), ("A2", MINOR), ("G2", MAJOR),
            ("D3", MAJOR), ("E2", MINOR), ("C3", MAJOR), ("G2", MAJOR), ("A2", MINOR), ("E2", MINOR),
            ("C3", MAJOR), ("D3", MAJOR), ("E2", MINOR), ("A2", MINOR)],
    lute_pattern=[(0.0, 3), (1.0, 4), (2.0, 3), (3.0, 5)],
    melody="E5:1 A5:1 G5:0.5 E5:0.5 D5:1 | D5:1 G5:1 F#5:0.5 E5:0.5 D5:1 | E5:1.5 D5:0.5 C5:1 B4:1 |"
           " B4:2 E5:2 | E5:1 A5:1 B5:0.5 A5:0.5 G5:1 | G5:1 D5:1 B4:1 G4:1 | F#4:1 A4:1 D5:1 F#5:1 |"
           " E5:4 | E5:1 G5:1 C6:1 G5:1 | D5:1 G5:1 B5:1 G5:1 | A5:1.5 G5:0.5 E5:1 C5:1 | B4:2 G4:2 |"
           " C5:1 E5:1 G5:1 E5:1 | D5:1 F#5:1 A5:1 F#5:1 | G5:1 F#5:1 E5:1 D5:1 | A4:3 r:1",
    drone_notes=["A2", "E3"],
)

SONGS = [("menu", MENU), ("proving_ground", PROVING_GROUND)]


# --- engine test fixtures -----------------------------------------------------------------------

def sine(freq, seconds, rate, amp=0.5):
    return [amp * math.sin(TAU * freq * i / rate) for i in range(int(seconds * rate))]


def fixtures(out_dir):
    os.makedirs(out_dir, exist_ok=True)
    # whole numbers of periods, so the tones loop without a seam
    encode_ogg(os.path.join(out_dir, "tone_440.ogg"), 1, RATE, sine(440, 2.5, RATE), 0)
    a, b = sine(660, 2.5, RATE), sine(660, 2.5, RATE, 0.25)
    encode_ogg(os.path.join(out_dir, "tone_660_stereo.ogg"), 2, RATE, list(zip(a, b)), 0)
    encode_ogg(os.path.join(out_dir, "empty.ogg"), 1, RATE, [], 0)  # valid headers, no audio
    write_wav(os.path.join(out_dir, "tone_16bit.wav"), 1, RATE, sine(1000, 0.25, RATE))
    left = sine(500, 0.1, 11025)
    write_wav(os.path.join(out_dir, "left_8bit_stereo.wav"), 2, 11025, [(s, 0.0) for s in left], bits=8)
    print("wrote audio fixtures to %s" % os.path.normpath(out_dir))


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "--fixtures":
        fixtures(sys.argv[2])
        return 0
    if len(sys.argv) != 1:
        print(__doc__)
        return 1
    os.makedirs(SOUNDS, exist_ok=True)
    os.makedirs(MUSIC, exist_ok=True)
    for i, (name, fn) in enumerate(EFFECTS):
        x = fn(random.Random(2000 + i))
        write_wav(os.path.join(SOUNDS, name + ".wav"), 1, RATE, x if name.endswith("_loop") else fade_tail(x))
    for name, song in SONGS:
        frames = render_song(**song)
        encode_ogg(os.path.join(MUSIC, name + ".ogg"), 2, RATE, frames, 1)
        print("%s: %.1f s" % (name, len(frames) / RATE))
    print("wrote %d sounds to %s and %d tracks to %s" % (len(EFFECTS), os.path.normpath(SOUNDS), len(SONGS),
                                                         os.path.normpath(MUSIC)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
