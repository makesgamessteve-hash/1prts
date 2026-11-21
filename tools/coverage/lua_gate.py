#!/usr/bin/env python3
"""Merges Lua lcov reports written by `1stprts --lua-coverage` and enforces the line
coverage gate for the base game's scripts (data/base/scripts, without scripts/tests).

usage: lua_gate.py <dir with .info files> [--min 95]"""
import argparse
import glob
import os
import sys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("--min", type=float, default=95.0)
    args = ap.parse_args()
    lines = {}  # file -> {line: hits}
    for path in glob.glob(os.path.join(args.dir, "*.info")):
        current = None
        with open(path) as f:
            for raw in f:
                raw = raw.strip()
                if raw.startswith("SF:"):
                    current = os.path.normpath(raw[3:])
                    lines.setdefault(current, {})
                elif raw.startswith("DA:") and current:
                    line, hits = raw[3:].split(",")[:2]
                    d = lines[current]
                    d[int(line)] = max(d.get(int(line), 0), int(hits))
    total = hit = 0
    rows = []
    for path in sorted(lines):
        norm = path.replace("\\", "/")
        if "/data/base/scripts/" not in norm or "/scripts/tests/" in norm:
            continue
        d = lines[path]
        f_total, f_hit = len(d), sum(1 for h in d.values() if h > 0)
        total += f_total
        hit += f_hit
        missing = [str(n) for n, h in sorted(d.items()) if h == 0]
        rows.append((norm.split("/data/base/")[1], f_total, f_hit, missing))
    if total == 0:
        print("no Lua coverage data found in", args.dir)
        return 1
    print("%-40s %6s %6s %6s  %s" % ("File", "Lines", "Exec", "Cover", "Missing"))
    for name, t, h, missing in rows:
        print("%-40s %6d %6d %5.0f%%  %s" % (name, t, h, 100.0 * h / t if t else 100, ",".join(missing)))
    pct = 100.0 * hit / total
    print("%-40s %6d %6d %5.1f%%" % ("TOTAL", total, hit, pct))
    if pct < args.min:
        print("Lua line coverage %.1f%% is below the %.0f%% gate" % (pct, args.min))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
