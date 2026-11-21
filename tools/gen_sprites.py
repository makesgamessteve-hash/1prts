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
    def __init__(self, w=FW, h=FH):
        self.w, self.h = w, h
        self.px = [[None] * w for _ in range(h)]

    def put(self, x, y, c):
        x, y = int(round(x)), int(round(y))
        if 0 <= x < self.w and 0 <= y < self.h:
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
        for y in range(self.h):
            for x in range(self.w):
                if self.px[y][x] is not None:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < self.w and 0 <= yy < self.h and self.px[yy][xx] is not None:
                        out[y][x] = color
                        break
        self.px = out


def shade(c, k):
    return tuple(max(0, min(255, int(v * k))) for v in c)


# Side colours for the skirmish: the Crown's blue and gold, the rival lord's red and black.
SIDES = {"crown": ((44, 66, 150), (214, 172, 62)), "red": ((150, 40, 36), (40, 34, 30))}


def person(d, step, kind, side="crown"):
    """d: direction 0..7, step: walk phase (None for idle, else 0..3). kind: guard, clerk, serf,
    crossbow or outrider; side picks the colours of soldiers and serfs."""
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
        if side != "crown":
            body, trim = SIDES[side]
    elif kind == "clerk":
        body, trim, legs, head_c = (112, 74, 48), (190, 160, 110), (80, 60, 44), (70, 48, 30)
    elif kind == "serf":  # a plain tunic, a hood in the side's colour
        body, trim, legs, head_c = (120, 104, 76), SIDES[side][0], (86, 72, 54), SIDES[side][0]
    elif kind == "crossbow":  # a padded coat in the side's colour, a kettle hat
        body, trim, legs, head_c = SIDES[side][0], SIDES[side][1], (78, 70, 60), (120, 124, 134)
    elif kind in ("kobb", "firebrand"):  # Kobb: small, big-eared, ochre and moss paint
        body, trim, legs, head_c = (112, 96, 60), (176, 112, 48), (88, 76, 50), (120, 140, 96)
    elif kind in ("ghorr", "spear", "giant"):  # Ghorr and giants: grey-green hide, fur and bone
        body, trim, legs, head_c = (104, 86, 62), (150, 70, 40), (70, 60, 48), (112, 128, 104)
        if kind == "giant":
            body, trim, legs, head_c = (120, 112, 100), (90, 80, 70), (100, 92, 84), (150, 140, 128)
    else:  # outrider: a long riding coat, a pennant
        body, trim, legs, head_c = shade(SIDES[side][0], 1.2), SIDES[side][1], (60, 50, 40), (96, 70, 44)
    skin, boots, wood, steel = (214, 164, 124), (58, 40, 26), (128, 92, 52), (196, 200, 210)
    if kind in ("kobb", "firebrand", "ghorr", "spear", "giant"):
        skin = head_c

    # legs and boots (walking moves them forward/back; seen from the side that spreads them)
    spread = 3.0 * abs(facing) * swing
    for side in (-1, 1):
        lx = cx + side * 2.6 * abs(front) + side * spread * (1 if side > 0 else -1) * 0.5 + facing * swing * side * 1.5
        ly = 56 - (1 if (step is not None and side * swing > 0 and abs(front) > 0.5) else 0)
        f.rect(lx - 1.5, 39 + bob, lx + 1.5, ly, shade(legs, 0.9 if side < 0 else 1.0))
        f.rect(lx - 2, ly, lx + 2 + facing * 1.5, ly + 3, boots)
    # torso / tabard or robe
    bottom = {"guard": 41, "clerk": 54, "serf": 42, "crossbow": 44, "outrider": 48, "kobb": 40, "firebrand": 40,
              "ghorr": 42, "spear": 42, "giant": 44}[kind]
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
    if kind in ("guard", "crossbow"):
        f.ellipse(cx + facing * 0.8, hy, 5.2, 6.2, head_c)                  # helmet
        f.rect(cx - 6, hy + 0.5, cx + 6, hy + 1.2, shade(head_c, 0.75))     # brim
    else:
        f.ellipse(cx + facing * 0.8, hy, 4.8, 6.0, head_c)                  # hair
    if kind in ("kobb", "firebrand") and front > -0.6:  # the big ears
        f.rect(cx - 8, hy - 1, cx - 5, hy + 1, head_c)
        f.rect(cx + 5, hy - 1, cx + 8, hy + 1, head_c)
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
    elif kind == "clerk":
        sx = cx - math.cos(a) * (width + 1.0)
        f.rect(sx - 2, 28 + bob, sx + 2, 31 + bob, (230, 220, 190))         # scroll
    elif kind == "serf":  # a pick over the shoulder
        sx = cx - math.cos(a) * (width + 1.0)
        for y in range(14, 34):
            f.put(sx + (y - 14) * 0.15, y + bob, wood)
        f.rect(sx - 3, 13 + bob, sx + 3, 15 + bob, steel)
    elif kind == "crossbow":  # the crossbow held across the body
        sx = cx - math.cos(a) * 2.0
        f.rect(sx - 6 * max(abs(front), 0.3), 26 + bob, sx + 6 * max(abs(front), 0.3), 27 + bob, wood)
        f.rect(sx - 1, 22 + bob, sx + 1, 32 + bob, shade(wood, 0.8))
    elif kind in ("ghorr", "giant"):  # tusks, and a club over the shoulder
        if front > -0.3:
            f.put(cx + facing * 2.2 - 2, hy + 5, (236, 230, 210))
            f.put(cx + facing * 2.2 + 2, hy + 5, (236, 230, 210))
        sx = cx - math.cos(a) * (width + 1.2)
        f.rect(sx - 1, 10 + bob, sx + 1, 34 + bob, wood)
        f.rect(sx - 2.5, 8 + bob, sx + 2.5, 14 + bob, shade(wood, 0.8))
    elif kind == "spear":  # javelins held high
        sx = cx - math.cos(a) * (width + 1.2) * 0.9
        for y in range(2, 46):
            f.put(sx, y + bob, wood)
        f.rect(sx - 1, 0 + bob, sx + 1, 3 + bob, (236, 230, 210))
    elif kind == "firebrand":  # a burning brand
        sx = cx - math.cos(a) * (width + 1.0)
        f.rect(sx - 0.8, 18 + bob, sx + 0.8, 32 + bob, wood)
        f.ellipse(sx, 15 + bob, 2.4, 3.4, (250, 150, 40))
        f.ellipse(sx, 15 + bob, 1.2, 1.8, (255, 236, 140))
    elif kind == "kobb":  # a small pick
        sx = cx - math.cos(a) * (width + 1.0)
        for y in range(18, 34):
            f.put(sx + (y - 18) * 0.15, y + bob, wood)
        f.rect(sx - 2, 17 + bob, sx + 2, 18 + bob, steel)
    else:  # outrider: a lance with a pennant in the side's colours
        sx = cx - math.cos(a) * (width + 1.2) * 0.9
        for y in range(1, 50):
            f.put(sx, y, wood)
        f.rect(sx + 1, 2, sx + 6, 6, trim)
    f.outline((22, 18, 16))
    return f


def sheet(kind, side="crown"):
    rgba = [0] * (COLS * FW * ROWS * FH * 4)
    W = COLS * FW
    for row in range(ROWS):
        d = row % 8
        for col in range(COLS):
            step = None if row < 8 else col
            if row < 8 and col > 0:
                step = None
            fr = person(d, step, kind, side)
            for y in range(FH):
                for x in range(FW):
                    c = fr.px[y][x]
                    if c is None:
                        continue
                    i = ((row * FH + y) * W + col * FW + x) * 4
                    rgba[i:i + 4] = [c[0], c[1], c[2], 255]
    return W, ROWS * FH, rgba


# --- horses and riders: 64 x 64 frames ---------------------------------------------------------

MW, MH = 64, 64
COATS = {"crown": (122, 84, 52), "red": (52, 40, 34), "wolf": (120, 120, 116), "boar": (70, 50, 36)}


def mounted(d, step, rider, side="crown", animal="horse"):
    """A horse seen from direction d (0 faces the viewer), walking (step 0..3) or standing,
    with an optional rider ("knight" or "outrider") in the side's colours."""
    f = Frame(MW, MH)
    a = d * math.pi / 4.0
    facing = -math.sin(a)   # -1 head to screen-left ... +1 screen-right
    front = math.cos(a)     # 1: head towards the viewer
    cx = 32
    coat = COATS[side] if animal == "horse" else COATS[animal]
    dark = shade(coat, 0.7)
    mane = (40, 30, 22)
    hoof = (34, 28, 24)
    swing = 0 if step is None else (-1, 0, 1, 0)[step]
    bob = 0 if step is None else (0, 1, 0, 1)[step]
    half = 7 + 12 * abs(facing)          # half the body's length (or the chest's width) as seen
    by = 34 + bob                        # the body's middle
    if animal == "wolf":
        half, by = 5 + 9 * abs(facing), 40 + bob
    elif animal == "boar":
        half, by = 8 + 10 * abs(facing), 40 + bob
    # legs: four, at the body's ends, swinging in pairs
    for i, (lx, phase) in enumerate(((-1, 1), (-0.6, -1), (0.6, 1), (1, -1))):
        x = cx + lx * (half - 3) * (1 if abs(facing) > 0.3 else 0.4) + (0 if abs(facing) > 0.3 else (i - 1.5) * 2)
        sw = swing * phase * 2 * abs(facing)
        col = dark if i % 2 else coat
        f.rect(x - 1.2 + sw * 0.5, by + 4, x + 1.2 + sw, 58, col)
        f.rect(x - 1.5 + sw, 58, x + 1.5 + sw, 60, hoof)
    # body
    f.ellipse(cx, by, half + 2, 7.5, coat)
    # tail at the back end
    tx = cx - facing * (half + 2)
    if abs(facing) > 0.3:
        f.rect(tx - 1, by - 3, tx + 1, by + 12, mane)
    elif front < 0:
        f.rect(cx - 1.5, by - 2, cx + 1.5, by + 12, mane)
    # neck and head at the front end
    hx = cx + facing * (half + 1)
    if abs(facing) > 0.3:
        f.rect(hx - 3, by - 16, hx + 3, by - 2, coat)                  # neck
        f.rect(hx - 3 - (1 if facing < 0 else 0), by - 18, hx + 3, by - 15, mane)
        f.ellipse(hx + facing * 4, by - 15, 5, 3.2, coat)              # head
        f.put(hx + facing * 3, by - 16, (20, 16, 12))                  # eye
        f.rect(hx - 1, by - 21, hx + 1, by - 18, dark)                  # ears
    if animal == "boar" and abs(facing) > 0.3:  # tusks and bristles
        tx = cx + facing * (half + 6)
        f.rect(tx - 1, by - 8, tx + 1, by - 6, (236, 230, 210))
        f.rect(cx - half, by - 9, cx + half, by - 7, mane)
    if rider in ("ghorr", "kobb"):
        body, trim = (104, 86, 62), (150, 70, 40)
        rx, ry = cx - facing * 2, by - 9
        w = (4 if rider == "kobb" else 5) + 2 * abs(front)
        top = 10 if rider == "kobb" else 14
        f.rect(rx - w, ry - top, rx + w, ry, body)
        f.rect(rx - 1, ry - top, rx + 1, ry, trim)
        f.ellipse(rx, ry - top - 4, 4, 4.5, (112, 128, 104))
        for t in range(0, 22):  # a spear
            f.put(rx + facing * 2 + facing * t * 0.9, ry - 6 - t * (0.35 if abs(facing) > 0.3 else 0.9), (128, 92, 52))
        rider = None
    if rider:
        body, trim = SIDES[side]
        if rider == "knight":
            body, trim = (196, 200, 210), SIDES[side][0]                 # plate and a surcoat stripe
        rx = cx - facing * 2
        ry = by - 9
        w = 4 + 2 * abs(front)
        f.rect(rx - w, ry - 13, rx + w, ry, body)                         # torso
        f.rect(rx - 1, ry - 13, rx + 1, ry, trim)
        f.rect(rx - 2, ry - 1, rx + 2, ry + 5, shade(body, 0.8))          # leg over the flank
        helm = (150, 156, 168) if rider == "knight" else (96, 70, 44)
        f.ellipse(rx, ry - 18, 4.2, 5, helm)
        if rider == "knight":
            f.rect(rx - 4, ry - 18, rx + 4, ry - 17, (40, 40, 46))         # visor slit
            f.rect(rx - 1, ry - 25, rx + 1, ry - 22, trim)                 # plume
        else:
            f.ellipse(rx + facing * 1.5, ry - 16, 2.8, 2.6, (214, 164, 124))
        # the lance, held couched forward
        lx = rx + facing * 2
        wood = (128, 92, 52)
        for t in range(0, 30):
            x = lx + facing * t * 0.9
            y = ry - 8 - t * (0.35 if abs(facing) > 0.3 else 0.9)
            f.put(x, y, wood)
        tipx = lx + facing * 29 * 0.9
        tipy = ry - 8 - 29 * (0.35 if abs(facing) > 0.3 else 0.9)
        f.rect(tipx - 1, tipy - 1, tipx + 1, tipy + 1, (196, 200, 210))
        f.rect(lx + facing * 6, ry - 13, lx + facing * 10, ry - 10, trim)  # pennant
    if abs(facing) <= 0.3 and front > 0:  # the head towards the viewer, in front of the rider
        f.ellipse(cx, by - 7, 3.8, 8, coat)
        f.rect(cx - 1, by - 12, cx + 1, by - 1, (230, 224, 210))       # blaze
        f.rect(cx - 5, by - 16, cx - 3, by - 13, dark)                 # ears
        f.rect(cx + 3, by - 16, cx + 5, by - 13, dark)
        f.put(cx - 2.5, by - 9, (20, 16, 12))
        f.put(cx + 2.5, by - 9, (20, 16, 12))
    f.outline((22, 18, 16))
    return f


def mounted_sheet(rider, side, animal="horse"):
    rgba = [0] * (COLS * MW * ROWS * MH * 4)
    W = COLS * MW
    for row in range(ROWS):
        d = row % 8
        for col in range(COLS):
            step = col if row >= 8 else None
            fr = mounted(d, step, rider, side, animal)
            for y in range(MH):
                for x in range(MW):
                    c = fr.px[y][x]
                    if c is None:
                        continue
                    i = ((row * MH + y) * W + col * MW + x) * 4
                    rgba[i:i + 4] = [c[0], c[1], c[2], 255]
    return W, ROWS * MH, rgba


MOUNTED_META = """-- {title}: 8 directions, standing and a 4-frame walk. Generated by tools/gen_sprites.py.
return {{
  texture = "sprites/{name}.png",
  frame = {{ 64, 64 }}, columns = 4, rows = 16,
  height = {height}, directions = 8,
  anims = {{
    {{ name = "idle", first_row = 0, frames = 1, fps = 1 }},
    {{ name = "walk", first_row = 8, frames = 4, fps = 8 }},
  }},
}}
"""

MOUNTED = (("horse", None, "crown", "Saddled horse", 2.4),
           ("crown_knight", "knight", "crown", "Crown knight", 3.0),
           ("red_knight", "knight", "red", "Rival knight", 3.0),
           ("crown_rider", "outrider", "crown", "Crown outrider on horseback", 2.9),
           ("red_rider", "outrider", "red", "Rival outrider on horseback", 2.9),
           ("wb_wolf", "kobb", "crown", "Wildborn wolf runner", 2.0, "wolf"),
           ("wb_boar", "ghorr", "crown", "Wildborn boar rider", 2.8, "boar"))


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
    for name, kind, side, title, height in (
            ("crown_guard", "guard", "crown", "Crown man-at-arms", 1.85),
            ("crown_clerk", "clerk", "crown", "Crown war-room clerk", 1.75),
            ("crown_serf", "serf", "crown", "Crown serf", 1.7),
            ("crown_crossbow", "crossbow", "crown", "Crown crossbowman", 1.8),
            ("crown_outrider", "outrider", "crown", "Crown outrider", 1.8),
            ("red_guard", "guard", "red", "Rival man-at-arms", 1.85),
            ("red_serf", "serf", "red", "Rival serf", 1.7),
            ("red_crossbow", "crossbow", "red", "Rival crossbowman", 1.8),
            ("red_outrider", "outrider", "red", "Rival outrider", 1.8),
            ("wb_kobb", "kobb", "crown", "Wildborn Kobb", 1.1),
            ("wb_firebrand", "firebrand", "crown", "Wildborn Kobb firebrand", 1.1),
            ("wb_ghorr", "ghorr", "crown", "Wildborn Ghorr brute", 2.3),
            ("wb_spear", "spear", "crown", "Wildborn spearhurler", 2.2),
            ("wb_giant", "giant", "crown", "Wildborn ridge giant", 6.0)):
        w, h, rgba = sheet(kind, side)
        write_png(os.path.join(OUT, name + ".png"), w, h, rgba)
        with open(os.path.join(OUT, name + ".lua"), "w") as f:
            f.write(META.format(title=title, name=name, height=height))
    for entry in MOUNTED:
        name, rider, side, title, height = entry[:5]
        animal = entry[5] if len(entry) > 5 else "horse"
        w, h, rgba = mounted_sheet(rider, side, animal)
        write_png(os.path.join(OUT, name + ".png"), w, h, rgba)
        with open(os.path.join(OUT, name + ".lua"), "w") as f:
            f.write(MOUNTED_META.format(title=title, name=name, height=height))
    print("wrote sprites to", os.path.normpath(OUT))
    return 0


if __name__ == "__main__":
    sys.exit(main())
