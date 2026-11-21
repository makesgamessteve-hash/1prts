#!/usr/bin/env python3
"""Asset checks for every pack under data/:
  - each pack has mod.lua and CREDITS.md
  - every art/content file is covered by a CREDITS.md row (path or glob) with an allowed license
  - every texture path used by a script exists in the pack
Code (scripts/**, shaders/**) is MIT and needs no credits row.

usage: validate_assets.py [data_dir]"""
import fnmatch
import os
import re
import sys

ALLOWED = {"CC-BY-4.0", "CC0-1.0"}
CODE_DIRS = ("scripts/", "shaders/")
META = {"mod.lua", "CREDITS.md"}


def credits(pack):
    rows = []
    for line in open(os.path.join(pack, "CREDITS.md"), encoding="utf-8"):
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) >= 6 and cells[0].startswith("`"):
            rows.append((cells[0].strip("`"), cells[3]))
    return rows


def check_pack(pack):
    problems = []
    for need in META:
        if not os.path.isfile(os.path.join(pack, need)):
            problems.append("%s: missing %s" % (pack, need))
    if problems:
        return problems
    rows = credits(pack)
    for glob_, lic in rows:
        if lic not in ALLOWED:
            problems.append("%s: CREDITS row %s has license %s (allowed: %s)" % (pack, glob_, lic, ", ".join(sorted(ALLOWED))))
    scripts = ""
    for dirpath, _, files in os.walk(pack):
        for name in files:
            rel = os.path.relpath(os.path.join(dirpath, name), pack).replace("\\", "/")
            if rel in META:
                continue
            if rel.startswith(CODE_DIRS):
                if rel.endswith(".lua"):
                    scripts += open(os.path.join(dirpath, name), encoding="utf-8").read()
                continue
            if not any(fnmatch.fnmatch(rel, g) for g, _ in rows):
                problems.append("%s: %s has no CREDITS.md entry" % (pack, rel))
    for ref in sorted(set(re.findall(r'"(textures/[^"]+)"', scripts))):
        if not os.path.isfile(os.path.join(pack, ref)):
            problems.append("%s: scripts reference missing file %s" % (pack, ref))
    return problems


def main():
    data = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")
    problems = []
    packs = [os.path.join(data, p) for p in sorted(os.listdir(data)) if os.path.isdir(os.path.join(data, p))]
    for pack in packs:
        problems += check_pack(pack)
    for p in problems:
        print(p)
    print("assets: %d pack(s), %s" % (len(packs), "ok" if not problems else "%d problem(s)" % len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
