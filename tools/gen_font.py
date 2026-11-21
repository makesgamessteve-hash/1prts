#!/usr/bin/env python3
"""Generates the 1stPRTS UI bitmap font: data/base/fonts/ui.png (atlas) and
data/base/fonts/ui.lua (metrics, plain Lua data).

The script is MIT-licensed code. The glyph shapes below are an original design made for
1stPRTS (5 x 7 pixels with two descender rows), released with the atlas under CC BY 4.0 and
credited in data/base/CREDITS.md. Only the standard library is used and the output is
deterministic.

Atlas layout (128 x 128, white on transparent):
  - printable ASCII 32..126 in 16 columns of 8 x 12 pixel cells, from (0, 0)
  - UI icons (check, close, collapsed, expanded, a white block) on the row at y = 80
  - touch overlay shapes (disc and ring, 32 x 32) on the row at y = 96"""
import os
import struct
import sys
import zlib

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "base", "fonts")
ATLAS = 128
CELL_W, CELL_H, COLUMNS, FIRST = 8, 12, 16, 32
HEIGHT, LINE_HEIGHT = 9, 11

# Each glyph is up to 5 columns wide and 9 rows tall: rows 0-6 sit on the baseline (row 6),
# rows 7-8 are descenders. '#' is ink, anything else is empty. Lines are listed top-down.
GLYPHS = r"""
! #|#|#|#|#|.|#
" #.#|#.#
# .#.#.|#####|.#.#.|.#.#.|#####|.#.#.
$ ..#..|.####|#.#..|.###.|..#.#|####.|..#..
% ##...|##..#|...#.|..#..|.#...|#..##|...##
& .##..|#..#.|.##..|.#...|#.#.#|#..#.|.##.#
' #|#
( .#|#.|#.|#.|#.|#.|.#
) #.|.#|.#|.#|.#|.#|#.
* .....|#.#.#|.###.|#####|.###.|#.#.#
+ .....|..#..|..#..|#####|..#..|..#..
, ..|..|..|..|..|.#|.#|#.
- ....|....|....|####
. .|.|.|.|.|.|#
/ ....#|...#.|...#.|..#..|.#...|.#...|#....
0 .###.|#...#|#..##|#.#.#|##..#|#...#|.###.
1 .#.|##.|.#.|.#.|.#.|.#.|###
2 .###.|#...#|....#|..##.|.#...|#....|#####
3 ####.|....#|....#|.###.|....#|....#|####.
4 ...#.|..##.|.#.#.|#..#.|#####|...#.|...#.
5 #####|#....|####.|....#|....#|#...#|.###.
6 ..##.|.#...|#....|####.|#...#|#...#|.###.
7 #####|....#|...#.|..#..|.#...|.#...|.#...
8 .###.|#...#|#...#|.###.|#...#|#...#|.###.
9 .###.|#...#|#...#|.####|....#|...#.|.##..
: .|.|#|.|.|#|.
; ..|..|.#|..|..|.#|.#|#.
< ...#|..#.|.#..|#...|.#..|..#.|...#
= ....|....|####|....|####
> #...|.#..|..#.|...#|..#.|.#..|#...
? .###.|#...#|....#|..##.|..#..|.....|..#..
@ .###.|#...#|#.###|#.#.#|#.##.|#....|.####
A .###.|#...#|#...#|#####|#...#|#...#|#...#
B ####.|#...#|#...#|####.|#...#|#...#|####.
C .###.|#...#|#....|#....|#....|#...#|.###.
D ###..|#..#.|#...#|#...#|#...#|#..#.|###..
E #####|#....|#....|####.|#....|#....|#####
F #####|#....|#....|####.|#....|#....|#....
G .###.|#...#|#....|#.###|#...#|#...#|.####
H #...#|#...#|#...#|#####|#...#|#...#|#...#
I ###|.#.|.#.|.#.|.#.|.#.|###
J ..###|...#.|...#.|...#.|...#.|#..#.|.##..
K #...#|#..#.|#.#..|##...|#.#..|#..#.|#...#
L #....|#....|#....|#....|#....|#....|#####
M #...#|##.##|#.#.#|#.#.#|#...#|#...#|#...#
N #...#|##..#|#.#.#|#.#.#|#..##|#...#|#...#
O .###.|#...#|#...#|#...#|#...#|#...#|.###.
P ####.|#...#|#...#|####.|#....|#....|#....
Q .###.|#...#|#...#|#...#|#.#.#|#..#.|.##.#
R ####.|#...#|#...#|####.|#.#..|#..#.|#...#
S .####|#....|#....|.###.|....#|....#|####.
T #####|..#..|..#..|..#..|..#..|..#..|..#..
U #...#|#...#|#...#|#...#|#...#|#...#|.###.
V #...#|#...#|#...#|#...#|.#.#.|.#.#.|..#..
W #...#|#...#|#...#|#.#.#|#.#.#|##.##|#...#
X #...#|#...#|.#.#.|..#..|.#.#.|#...#|#...#
Y #...#|#...#|.#.#.|..#..|..#..|..#..|..#..
Z #####|....#|...#.|..#..|.#...|#....|#####
[ ##|#.|#.|#.|#.|#.|##
\ #....|.#...|.#...|..#..|...#.|...#.|....#
] ##|.#|.#|.#|.#|.#|##
^ ..#..|.#.#.|#...#
_ .....|.....|.....|.....|.....|.....|.....|#####
` #.|.#
a .....|.....|.###.|....#|.####|#...#|.####
b #....|#....|####.|#...#|#...#|#...#|####.
c ....|....|.###|#...|#...|#...|.###
d ....#|....#|.####|#...#|#...#|#...#|.####
e .....|.....|.###.|#...#|#####|#....|.###.
f ..##|.#..|####|.#..|.#..|.#..|.#..
g .....|.....|.####|#...#|#...#|#...#|.####|....#|.###.
h #....|#....|####.|#...#|#...#|#...#|#...#
i #|.|#|#|#|#|#
j ..#|...|.##|..#|..#|..#|..#|..#|##.
k #...|#...|#..#|#.#.|##..|#.#.|#..#
l ##.|.#.|.#.|.#.|.#.|.#.|..#
m .....|.....|##.#.|#.#.#|#.#.#|#.#.#|#...#
n .....|.....|####.|#...#|#...#|#...#|#...#
o .....|.....|.###.|#...#|#...#|#...#|.###.
p .....|.....|####.|#...#|#...#|#...#|####.|#....|#....
q .....|.....|.####|#...#|#...#|#...#|.####|....#|....#
r ....|....|#.##|##..|#...|#...|#...
s ....|....|.###|#...|.##.|...#|###.
t .#..|.#..|####|.#..|.#..|.#..|..##
u .....|.....|#...#|#...#|#...#|#...#|.####
v .....|.....|#...#|#...#|.#.#.|.#.#.|..#..
w .....|.....|#...#|#.#.#|#.#.#|#.#.#|.#.#.
x .....|.....|#...#|.#.#.|..#..|.#.#.|#...#
y .....|.....|#...#|#...#|#...#|#...#|.####|....#|.###.
z .....|.....|#####|...#.|..#..|.#...|#####
{ ..#|.#.|.#.|#..|.#.|.#.|..#
| #|#|#|#|#|#|#|#
} #..|.#.|.#.|..#|.#.|.#.|#..
~ .....|.....|.#...|#.#.#|...#.
"""

ICONS = {
    # name: (x, y, rows)
    "check": (8, 80, ["......#", ".....##", "#...##.", "##.##..", ".###...", "..#...."]),
    "close": (16, 80, ["#.....#", ".#...#.", "..#.#..", "...#...", "..#.#..", ".#...#.", "#.....#"]),
    "collapsed": (24, 80, ["#....", "##...", "###..", "####.", "###..", "##...", "#...."]),
    "expanded": (32, 80, ["#######", ".#####.", "..###..", "...#..."]),
    "white": (40, 80, ["####", "####", "####", "####"]),
}


def parse_glyphs():
    glyphs = {}
    for line in GLYPHS.strip("\n").split("\n"):
        ch, rows = line[0], line[2:].split("|")
        glyphs[ord(ch)] = rows
    return glyphs


def write_png(path, w, h, rgba):
    raw = b"".join(b"\x00" + bytes(rgba[y * w * 4:(y + 1) * w * 4]) for y in range(h))

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def main():
    glyphs = parse_glyphs()
    px = [0] * (ATLAS * ATLAS * 4)

    def plot(x, y):
        i = (y * ATLAS + x) * 4
        px[i:i + 4] = [255, 255, 255, 255]

    def blit(x0, y0, rows):
        for y, row in enumerate(rows):
            for x, c in enumerate(row):
                if c == "#":
                    plot(x0 + x, y0 + y)

    advances = []
    for code in range(FIRST, 127):
        col, row = (code - FIRST) % COLUMNS, (code - FIRST) // COLUMNS
        rows = glyphs.get(code, [])
        assert len(rows) <= HEIGHT and all(len(r) <= CELL_W - 1 for r in rows), chr(code)
        blit(col * CELL_W, row * CELL_H, rows)
        width = max((len(r.rstrip(".")) for r in rows), default=0)
        advances.append(width + 1 if code != 32 else 4)
    missing = [chr(c) for c in range(FIRST + 1, 127) if c not in glyphs]
    assert not missing, "glyphs missing: %s" % "".join(missing)

    icon_rects = {}
    for name, (x, y, rows) in ICONS.items():
        blit(x, y, rows)
        icon_rects[name] = (x, y, max(len(r) for r in rows), len(rows))
    # touch overlay: a filled disc and a two-pixel ring, 32 x 32
    for name, x0 in (("disc", 0), ("ring", 32)):
        for y in range(32):
            for x in range(32):
                d = ((x - 15.5) ** 2 + (y - 15.5) ** 2) ** 0.5
                if d <= 15.5 and (name == "disc" or d >= 13.5):
                    plot(x0 + x, 96 + y)
        icon_rects[name] = (x0, 96, 32, 32)

    os.makedirs(ROOT, exist_ok=True)
    write_png(os.path.join(ROOT, "ui.png"), ATLAS, ATLAS, px)
    lines = [
        "-- 1stPRTS UI bitmap font metrics. Generated by tools/gen_font.py; do not edit.",
        "-- CC BY 4.0, see CREDITS.md.",
        "return {",
        '  texture = "fonts/ui.png",',
        "  atlas = { %d, %d }," % (ATLAS, ATLAS),
        "  height = %d," % HEIGHT,
        "  line_height = %d," % LINE_HEIGHT,
        "  cell = { %d, %d }," % (CELL_W, CELL_H),
        "  columns = %d," % COLUMNS,
        "  first = %d," % FIRST,
        "  advance = { %s }," % ", ".join(str(a) for a in advances),
        "  icons = {",
    ]
    for name in sorted(icon_rects):
        lines.append("    %s = { %d, %d, %d, %d }," % ((name,) + icon_rects[name]))
    lines += ["  },", "}", ""]
    with open(os.path.join(ROOT, "ui.lua"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
    print("wrote %d glyphs and %d icons to %s" % (len(advances), len(icon_rects), os.path.normpath(ROOT)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
