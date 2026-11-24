#!/usr/bin/env python3
"""Generates the 1stPRTS UI bitmap font: data/base/fonts/ui.png (atlas) and
data/base/fonts/ui.lua (metrics, plain Lua data).

The script is MIT-licensed code. The glyph shapes below are an original design made for
1stPRTS (5 x 7 pixels with two descender rows), released with the atlas under CC BY 4.0 and
credited in data/base/CREDITS.md. Only the standard library is used and the output is
deterministic.

Atlas layout (128 x 256, white on transparent):
  - printable ASCII 32..126 in 16 columns of 8 x 12 pixel cells, from (0, 0)
  - UI icons (check, close, collapsed, expanded, a white block) on the row at y = 80
  - touch overlay shapes (disc and ring, 32 x 32) on the row at y = 96
  - Latin-1 Supplement U+00A0..U+00FF in the same cells, from (0, 128)

The metrics list each block as a range ({ first, x, y, advance }); the engine maps UTF-8
code points in a range to its glyphs and draws '?' for anything else."""
import os
import struct
import sys
import zlib

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "base", "fonts")
ATLAS_W, ATLAS_H = 128, 256
CELL_W, CELL_H, COLUMNS = 8, 12, 16
# (first code point, count, x, y) of each block of glyphs in the atlas
RANGES = ((32, 95, 0, 0), (160, 96, 0, 128))
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

# Latin-1 Supplement (U+00A0..U+00FF). Symbols are drawn here, one per line as above (the
# key is the character itself); accented letters are composed below from a base letter and
# an accent mark.
LATIN1 = r"""
¡ .|.|#|.|#|#|#|#|#
¢ .....|..#..|.###.|#.#..|#.#..|#.#..|.###.|..#..
£ ..##.|.#..#|.#...|###..|.#...|.#...|#####
¤ .....|#...#|.###.|.#.#.|.###.|#...#
¥ #...#|.#.#.|..#..|#####|..#..|#####|..#..
¦ #|#|#|.|.|#|#|#
§ .###|#...|.##.|#..#|.##.|...#|###.
¨ #.#
© .#####.|#.....#|#.###.#|#.#...#|#.###.#|#.....#|.#####.
ª ##.|..#|.##|#.#|.##|...|###
« .....|..#.#|.#.#.|#.#..|.#.#.|..#.#
¬ ....|....|....|####|...#
­ ....|....|....|####
® .#####.|#.....#|#.##..#|#.#.#.#|#.##..#|#.#.#.#|.#####.
¯ #####
° .#.|#.#|.#.
± .....|..#..|..#..|#####|..#..|..#..|#####
² ##.|..#|.#.|###
³ ###|..#|.##|..#|###
´ .#|#.
µ .....|.....|#...#|#...#|#...#|##..#|#.##.|#....|#....
¶ .####|###.#|###.#|.##.#|..#.#|..#.#|..#.#
· .|.|.|#
¸ ..|..|..|..|..|..|..|.#|##
¹ .#.|##.|.#.|.#.|###
º .#.|#.#|#.#|.#.|...|###
» .....|#.#..|.#.#.|..#.#|.#.#.|#.#..
¼ #....#.|#...#..|#..#...|..#.#.#|.#..###|#.....#|......#
½ #....#.|#...#..|#..#...|..#.##.|.#....#|#....#.|....###
¾ ##...#.|.#..#..|##.#...|..#.#.#|.#..###|#.....#|......#
¿ ..#..|.....|..#..|.##..|#....|#...#|.###.
Æ .####|#.#..|#.#..|####.|#.#..|#.#..|#.###
Ð ###..|#..#.|#...#|###.#|#...#|#..#.|###..
× .....|.....|#...#|.#.#.|..#..|.#.#.|#...#
Ø .####|#..##|#.#.#|#.#.#|#.#.#|##..#|####.
Þ #....|####.|#...#|#...#|####.|#....|#....
ß .###.|#...#|#..#.|#.#..|#..#.|#...#|#.##.
Å ..#..|.#.#.|..#..|.#.#.|#...#|#####|#...#
æ .....|.....|##.#.|..#.#|.####|#.#..|.#.##
ð .#.#.|..#..|.#.#.|....#|.####|#...#|.###.
å ..#..|.#.#.|.###.|....#|.####|#...#|.####
÷ .....|..#..|.....|#####|.....|..#..
ø .....|.....|.###.|#..##|#.#.#|##..#|.###.
þ #....|#....|####.|#...#|#...#|#...#|####.|#....|#....
"""

# Capitals that carry an accent are shortened to five rows (2-6) under a two-row accent.
SHORT_CAPS = {
    "A": ".###.|#...#|#####|#...#|#...#",
    "C": ".###.|#...#|#....|#...#|.###.",
    "E": "#####|#....|####.|#....|#####",
    "I": "###|.#.|.#.|.#.|###",
    "N": "#...#|##..#|#.#.#|#..##|#...#",
    "O": ".###.|#...#|#...#|#...#|.###.",
    "U": "#...#|#...#|#...#|#...#|.###.",
    "Y": "#...#|.#.#.|..#..|..#..|..#..",
}
# Accent marks on rows 0-1, centered over the letter (one form per letter width).
ACCENTS = {
    "grave": {5: ".#...|..#..", 3: "#..|.#."},
    "acute": {5: "...#.|..#..", 3: "..#|.#."},
    "circumflex": {5: "..#..|.#.#.", 3: ".#.|#.#"},
    "tilde": {5: ".##.#|#..#.", 3: "###|..."},
    "diaeresis": {5: ".#.#.|.....", 3: "#.#|..."},
}
# Composed letters: code point -> (base letter, accent); "cedilla" hangs below the baseline.
COMPOSED = {}
for _base, _first in (("A", 0xC0), ("a", 0xE0)):
    for _i, _acc in enumerate(("grave", "acute", "circumflex", "tilde", "diaeresis")):
        COMPOSED[_first + _i] = (_base, _acc)
for _base, _first in (("E", 0xC8), ("e", 0xE8), ("I", 0xCC), ("i", 0xEC), ("U", 0xD9), ("u", 0xF9)):
    for _i, _acc in enumerate(("grave", "acute", "circumflex", "diaeresis")):
        COMPOSED[_first + _i] = (_base, _acc)
for _base, _first in (("O", 0xD2), ("o", 0xF2)):
    for _i, _acc in enumerate(("grave", "acute", "circumflex", "tilde", "diaeresis")):
        COMPOSED[_first + _i] = (_base, _acc)
COMPOSED.update({0xC7: ("C", "cedilla"), 0xE7: ("c", "cedilla"), 0xD1: ("N", "tilde"), 0xF1: ("n", "tilde"),
                 0xDD: ("Y", "acute"), 0xFD: ("y", "acute"), 0xFF: ("y", "diaeresis")})
CEDILLA = {5: "..#..|.##..", 4: "..#.|.##."}

ICONS = {
    # name: (x, y, rows)
    "check": (8, 80, ["......#", ".....##", "#...##.", "##.##..", ".###...", "..#...."]),
    "close": (16, 80, ["#.....#", ".#...#.", "..#.#..", "...#...", "..#.#..", ".#...#.", "#.....#"]),
    "collapsed": (24, 80, ["#....", "##...", "###..", "####.", "###..", "##...", "#...."]),
    "expanded": (32, 80, ["#######", ".#####.", "..###..", "...#..."]),
    "white": (40, 80, ["####", "####", "####", "####"]),
}


def parse_table(text):
    glyphs = {}
    for line in text.strip("\n").split("\n"):
        ch, rows = line[0], line[2:].split("|")
        glyphs[ord(ch)] = rows
    return glyphs


def compose(glyphs, base, accent):
    """An accented letter: the base letter (shortened if it is a capital) under an accent."""
    full = glyphs[ord(base)]
    if accent == "cedilla":
        width = len(full[0])
        return full[:7] + CEDILLA[width].split("|")
    if base == "i":
        body = ["...", ".#.", ".#.", ".#.", ".#."]  # a dotless stem, one row below the accent
    elif base.isupper():
        body = SHORT_CAPS[base].split("|")
    else:
        body = full[2:]  # lowercase letters sit on rows 2-6 below an empty x-height cap
    width = len(body[0])
    return ACCENTS[accent][width].split("|") + body


def parse_glyphs():
    glyphs = parse_table(GLYPHS)
    glyphs[0xA0] = []  # no-break space
    glyphs.update(parse_table(LATIN1))
    for code, (base, accent) in sorted(COMPOSED.items()):
        assert code not in glyphs, hex(code)
        glyphs[code] = compose(glyphs, base, accent)
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
    px = [0] * (ATLAS_W * ATLAS_H * 4)

    def plot(x, y):
        i = (y * ATLAS_W + x) * 4
        px[i:i + 4] = [255, 255, 255, 255]

    def blit(x0, y0, rows):
        for y, row in enumerate(rows):
            for x, c in enumerate(row):
                if c == "#":
                    plot(x0 + x, y0 + y)

    advances = []  # one list per range
    for first, count, x0, y0 in RANGES:
        advances.append([])
        for code in range(first, first + count):
            col, row = (code - first) % COLUMNS, (code - first) // COLUMNS
            assert code == 32 or code in glyphs, "glyph missing: U+%04X" % code
            rows = glyphs.get(code, [])
            assert len(rows) <= HEIGHT and all(len(r) <= CELL_W - 1 for r in rows), "U+%04X" % code
            blit(x0 + col * CELL_W, y0 + row * CELL_H, rows)
            width = max((len(r.rstrip(".")) for r in rows), default=0)
            advances[-1].append(width + 1 if code not in (32, 0xA0) else 4)
    assert set(glyphs) <= {c for f, n, _, _ in RANGES for c in range(f, f + n)}

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
    write_png(os.path.join(ROOT, "ui.png"), ATLAS_W, ATLAS_H, px)
    lines = [
        "-- 1stPRTS UI bitmap font metrics. Generated by tools/gen_font.py; do not edit.",
        "-- CC BY 4.0, see CREDITS.md.",
        "return {",
        '  texture = "fonts/ui.png",',
        "  atlas = { %d, %d }," % (ATLAS_W, ATLAS_H),
        "  height = %d," % HEIGHT,
        "  line_height = %d," % LINE_HEIGHT,
        "  cell = { %d, %d }," % (CELL_W, CELL_H),
        "  columns = %d," % COLUMNS,
        "  -- blocks of glyphs: the first code point, the top-left cell in the atlas and the",
        "  -- advance of each character (its count is the number of characters)",
        "  ranges = {",
    ]
    for (first, _, x0, y0), adv in zip(RANGES, advances):
        lines.append("    { first = %d, x = %d, y = %d," % (first, x0, y0))
        lines.append("      advance = { %s } }," % ", ".join(str(a) for a in adv))
    lines += ["  },", "  icons = {"]
    for name in sorted(icon_rects):
        lines.append("    %s = { %d, %d, %d, %d }," % ((name,) + icon_rects[name]))
    lines += ["  },", "}", ""]
    with open(os.path.join(ROOT, "ui.lua"), "w", encoding="utf-8", newline="\n") as f:
        f.write("\n".join(lines))
    total = sum(len(a) for a in advances)
    print("wrote %d glyphs and %d icons to %s" % (total, len(icon_rects), os.path.normpath(ROOT)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
