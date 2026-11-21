#!/usr/bin/env python3
"""Generates 1stPRTS placeholder textures (retro, 64x64) into data/base/textures/.

The script is MIT-licensed code; the images it produces are original placeholder art
released under CC BY 4.0 (credited in data/base/CREDITS.md). Only the standard library is
used, and the output is deterministic (fixed seeds)."""
import os
import random
import struct
import sys
import zlib

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "base", "textures")
SIZE = 64


def write_png(path, w, h, pixels):
    raw = b"".join(b"\x00" + bytes(pixels[y * w * 3:(y + 1) * w * 3]) for y in range(h))

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def clamp(v):
    return max(0, min(255, int(v)))


def make(name, fn, seed):
    rnd = random.Random(seed)
    px = []
    for y in range(SIZE):
        for x in range(SIZE):
            r, g, b = fn(x, y, rnd)
            px += [clamp(r), clamp(g), clamp(b)]
    write_png(os.path.join(OUT, name + ".png"), SIZE, SIZE, px)


def noise(rnd, amp):
    return rnd.uniform(-amp, amp)


def grass(x, y, rnd):
    n = noise(rnd, 18)
    blade = 14 if (x * 7 + y * 3) % 11 == 0 else 0
    return 74 + n, 112 + n + blade, 52 + n * 0.5


def dirt(x, y, rnd):
    n = noise(rnd, 16)
    return 118 + n, 92 + n, 64 + n


def stone_wall(x, y, rnd):
    row = y // 16
    off = 16 if row % 2 else 0
    mortar = (y % 16 == 0) or ((x + off) % 32 == 0)
    block = ((x + off) // 32 + row * 3) % 5
    base = 128 + block * 7
    n = noise(rnd, 10)
    if mortar:
        return 82, 78, 72
    return base + n, base - 4 + n, base - 10 + n


def stone_floor(x, y, rnd):
    edge = (x % 32 == 0) or (y % 32 == 0) or (x % 32 == 31) or (y % 32 == 31)
    tile = ((x // 32) + 2 * (y // 32)) % 3
    n = noise(rnd, 9)
    if edge:
        return 70, 66, 62
    return 112 + tile * 9 + n, 108 + tile * 8 + n, 100 + tile * 6 + n


def planks(x, y, rnd, base=(140, 98, 58)):
    row = y // 8
    seam = y % 8 == 0 or (x + row * 23) % 64 == 0
    grain = 10 if (x + y * 5 + row * 7) % 13 == 0 else 0
    n = noise(rnd, 6)
    if seam:
        return base[0] * 0.55, base[1] * 0.55, base[2] * 0.55
    k = 1.0 - (row % 3) * 0.06
    return base[0] * k + n - grain, base[1] * k + n - grain, base[2] * k + n - grain


def wood_dark(x, y, rnd):
    stud = (x % 21 in (9, 10)) and (y % 21 in (9, 10))
    if stud:
        return 52, 52, 56
    col = x // 8
    seam = x % 8 == 0
    n = noise(rnd, 5)
    if seam:
        return 38, 24, 14
    k = 1.0 - (col % 3) * 0.07
    return 92 * k + n, 58 * k + n, 32 * k + n


def plaster(x, y, rnd):
    n = noise(rnd, 7)
    crack = 18 if (x == (y * 3) % 64 and 20 < y < 34) else 0
    return 196 + n - crack, 186 + n - crack, 160 + n - crack


def roof_slate(x, y, rnd):
    row = y // 8
    off = 4 if row % 2 else 0
    edge = y % 8 == 7 or (x + off) % 8 == 0
    n = noise(rnd, 8)
    if edge:
        return 34, 38, 48
    return 70 + n, 76 + n, 92 + n


def iron(x, y, rnd):
    n = noise(rnd, 10)
    rivet = (x % 16 == 3 and y % 16 == 3)
    return (120 if rivet else 66) + n, (118 if rivet else 66) + n, (112 if rivet else 70) + n


def banner(x, y, rnd):
    n = noise(rnd, 5)
    cx = abs(x - 32)
    gold = (abs(y - (20 + cx // 2)) < 3) or (x < 3 or x > 60)
    if gold:
        return 214 + n, 170 + n, 60
    return 34 + n, 58 + n, 140 + n


def table_oak(x, y, rnd):
    return planks(x, y, rnd, base=(120, 80, 44))


def rug(x, y, rnd):
    n = noise(rnd, 6)
    border = x < 6 or y < 6 or x > 57 or y > 57
    diamond = abs(x - 32) + abs(y - 32) in range(14, 18)
    if border or diamond:
        return 200 + n, 160 + n, 70 + n
    return 130 + n, 30 + n, 32 + n


def bark(x, y, rnd):
    stripe = 18 if x % 6 < 2 else 0
    n = noise(rnd, 8)
    return 86 - stripe + n, 60 - stripe + n, 40 - stripe * 0.5 + n


def pine(x, y, rnd):
    n = noise(rnd, 14)
    needle = 16 if (x + y) % 5 == 0 else 0
    return 34 + n, 74 + n + needle, 46 + n


def flame(x, y, rnd):
    t = y / 63.0
    n = noise(rnd, 20)
    return 255, 150 + 90 * t + n, 40 + 60 * t


def rock(x, y, rnd):
    n = noise(rnd, 18)
    vein = 14 if (x * 2 + y) % 17 == 0 else 0
    return 112 + n - vein, 110 + n - vein, 104 + n - vein


def gate_iron(x, y, rnd):
    bar = x % 8 < 3 or y % 16 < 3
    n = noise(rnd, 6)
    if bar:
        return 58 + n, 58 + n, 64 + n
    return 20, 18, 16


TEXTURES = [
    ("grass", grass), ("dirt", dirt), ("stone_wall", stone_wall), ("stone_floor", stone_floor),
    ("wood_planks", planks), ("wood_dark", wood_dark), ("plaster", plaster), ("roof_slate", roof_slate),
    ("iron", iron), ("banner_crown", banner), ("table_oak", table_oak), ("rug", rug), ("bark", bark),
    ("pine", pine), ("flame", flame), ("rock", rock), ("gate_iron", gate_iron),
]


def main():
    os.makedirs(OUT, exist_ok=True)
    for i, (name, fn) in enumerate(TEXTURES):
        make(name, fn, 1000 + i)
    print("wrote %d textures to %s" % (len(TEXTURES), os.path.normpath(OUT)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
