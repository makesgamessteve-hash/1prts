#!/usr/bin/env python3
"""Generates 1stPRTS placeholder person sprites (8 directions, idle + 4-frame walk) into
data/base/sprites/. The script is MIT-licensed code; the images are original placeholder art
under CC BY 4.0 (credited in data/base/CREDITS.md). Standard library only, deterministic.

Sheet layout: 4 columns (frames) x 16 rows of 32x64 px. Rows 0-7: idle, directions 0-7;
rows 8-15: walk, directions 0-7. Direction 0 faces the viewer, then clockwise seen from
above in 45-degree steps (2 = the left side is towards the viewer, 6 = the right side)."""
import math
import os
import struct
import sys
import zlib

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "base", "sprites")
FW, FH, COLS, ROWS = 32, 64, 4, 16


def write_png(path, w, h, rgba):
    raw = b"".join(b"\x00" + bytes(rgba[y * w * 4:(y + 1) * w * 4]) for y in range(h))

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


class Frame:
    def __init__(self):
        self.px = [[None] * FW for _ in range(FH)]

    def put(self, x, y, c):
        x, y = int(round(x)), int(round(y))
        if 0 <= x < FW and 0 <= y < FH:
            self.px[y][x] = c

    def rect(self, x0, y0, x1, y1, c):
        for y in range(int(round(y0)), int(round(y1)) + 1):
            for x in range(int(round(x0)), int(round(x1)) + 1):
                self.put(x, y, c)

    def ellipse(self, cx, cy, rx, ry, c):
        for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
                if ((x - cx) / max(rx, 0.5)) ** 2 + ((y - cy) / max(ry, 0.5)) ** 2 <= 1.0:
                    self.put(x, y, c)

    def outline(self, color):
        out = [row[:] for row in self.px]
        for y in range(FH):
            for x in range(FW):
                if self.px[y][x] is not None:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < FW and 0 <= yy < FH and self.px[yy][xx] is not None:
                        out[y][x] = color
                        break
        self.px = out


def shade(c, k):
    return tuple(max(0, min(255, int(v * k))) for v in c)


def person(d, step, kind):
    """d: direction 0..7, step: walk phase (None for idle, else 0..3)."""
    f = Frame()
    a = d * math.pi / 4.0            # 0 = facing the viewer
    facing = -math.sin(a)            # -1: profile facing screen-left ... +1: screen-right
    front = math.cos(a)              # 1 front, -1 back
    cx = 16
    width = 6.0 + 2.0 * abs(front)   # half-width of the torso
    swing = 0.0 if step is None else (-1, 0, 1, 0)[step]
    bob = 0 if step is None else (0, 1, 0, 1)[step]

    if kind == "guard":
        body, trim, legs, head_c = (44, 66, 150), (214, 172, 62), (92, 82, 70), (150, 156, 168)
    else:
        body, trim, legs, head_c = (112, 74, 48), (190, 160, 110), (80, 60, 44), (70, 48, 30)
    skin, boots, wood, steel = (214, 164, 124), (58, 40, 26), (128, 92, 52), (196, 200, 210)

    # legs and boots (walking moves them forward/back; seen from the side that spreads them)
    spread = 3.0 * abs(facing) * swing
    for side in (-1, 1):
        lx = cx + side * 2.6 * abs(front) + side * spread * (1 if side > 0 else -1) * 0.5 + facing * swing * side * 1.5
        ly = 56 - (1 if (step is not None and side * swing > 0 and abs(front) > 0.5) else 0)
        f.rect(lx - 1.5, 39 + bob, lx + 1.5, ly, shade(legs, 0.9 if side < 0 else 1.0))
        f.rect(lx - 2, ly, lx + 2 + facing * 1.5, ly + 3, boots)
    # torso / tabard or robe
    bottom = 41 if kind == "guard" else 54
    f.rect(cx - width, 18 + bob, cx + width, bottom + bob, body)
    if front > 0.2 or (kind == "guard" and front < -0.2):
        stripe = cx + facing * 2.5
        f.rect(stripe - 1, 18 + bob, stripe + 1, bottom + bob, trim)       # tabard stripe
    f.rect(cx - width, 30 + bob, cx + width, 31 + bob, shade(trim, 0.8))    # belt
    # arms, swinging opposite to the legs
    for side in (-1, 1):
        ax = cx + side * (width + 1.2) * max(abs(front), 0.35)
        sw = -swing * side * 2.0
        f.rect(ax - 1.2, 19 + bob + max(0, sw), ax + 1.2, 33 + bob + sw, shade(body, 0.85))
        f.rect(ax - 1.2, 33 + bob + sw, ax + 1.2, 35 + bob + sw, skin)
    # head
    hy = 11 + bob
    if kind == "guard":
        f.ellipse(cx + facing * 0.8, hy, 5.2, 6.2, head_c)                  # helmet
        f.rect(cx - 6, hy + 0.5, cx + 6, hy + 1.2, shade(head_c, 0.75))     # brim
    else:
        f.ellipse(cx + facing * 0.8, hy, 4.8, 6.0, head_c)                  # hair
    if front > -0.3:
        fx = cx + facing * 2.2
        f.ellipse(fx, hy + 2.5, 3.6 - 1.4 * abs(facing), 3.4, skin)        # face
        if abs(facing) < 0.8:
            f.put(fx - 1.4, hy + 2, (30, 24, 20))
            f.put(fx + 1.4, hy + 2, (30, 24, 20))
        else:
            f.put(fx + facing * 2.6, hy + 3, skin)                         # nose in profile
            f.put(fx + facing * 1.2, hy + 2, (30, 24, 20))
    # the guard's spear, held in his right hand (screen-left when facing us)
    if kind == "guard":
        hand = cx - math.cos(a) * (width + 1.2) * 0.9 + math.sin(a) * 1.0
        behind = math.sin(a) > 0.5  # held on the far side
        sx = hand
        for y in range(1, 58):
            if behind and 18 < y < 40 and abs(sx - cx) < width:
                continue
            f.put(sx, y + bob * 0, wood)
        f.rect(sx - 1, 0, sx + 1, 3, steel)
    else:
        sx = cx - math.cos(a) * (width + 1.0)
        f.rect(sx - 2, 28 + bob, sx + 2, 31 + bob, (230, 220, 190))         # scroll
    f.outline((22, 18, 16))
    return f


def sheet(kind):
    rgba = [0] * (COLS * FW * ROWS * FH * 4)
    W = COLS * FW
    for row in range(ROWS):
        d = row % 8
        for col in range(COLS):
            step = None if row < 8 else col
            if row < 8 and col > 0:
                step = None
            fr = person(d, step, kind)
            for y in range(FH):
                for x in range(FW):
                    c = fr.px[y][x]
                    if c is None:
                        continue
                    i = ((row * FH + y) * W + col * FW + x) * 4
                    rgba[i:i + 4] = [c[0], c[1], c[2], 255]
    return W, ROWS * FH, rgba


META = """-- {title}: 8 directions, idle and a 4-frame walk. Generated by tools/gen_sprites.py.
return {{
  texture = "sprites/{name}.png",
  frame = {{ 32, 64 }}, columns = 4, rows = 16,
  height = {height}, directions = 8,
  anims = {{
    {{ name = "idle", first_row = 0, frames = 1, fps = 1 }},
    {{ name = "walk", first_row = 8, frames = 4, fps = 6 }},
  }},
}}
"""


def main():
    os.makedirs(OUT, exist_ok=True)
    for name, kind, title, height in (("crown_guard", "guard", "Crown man-at-arms", 1.85),
                                      ("crown_clerk", "clerk", "Crown war-room clerk", 1.75)):
        w, h, rgba = sheet(kind)
        write_png(os.path.join(OUT, name + ".png"), w, h, rgba)
        with open(os.path.join(OUT, name + ".lua"), "w") as f:
            f.write(META.format(title=title, name=name, height=height))
    print("wrote sprites to", os.path.normpath(OUT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
