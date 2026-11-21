#!/usr/bin/env python3
"""Checks the engine's layering rules (see CLAUDE.md and docs/architecture/README.md):
only engine/src/platform/ and engine/src/xr/ may include SDL or Emscripten headers, and only
engine/src/gfx/ may call OpenGL functions."""
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "engine", "src")


def main():
    problems = []
    for dirpath, _, files in os.walk(ROOT):
        rel_dir = os.path.relpath(dirpath, ROOT).replace("\\", "/")
        top = rel_dir.split("/")[0]
        for name in files:
            if not name.endswith((".c", ".h")):
                continue
            path = os.path.join(dirpath, name)
            text = open(path, encoding="utf-8").read()
            rel = "engine/src/%s/%s" % (rel_dir, name)
            if top not in ("platform", "xr") and re.search(r'#include\s*[<"](SDL3/|emscripten)', text):
                problems.append("%s includes SDL/Emscripten headers (only platform/ and xr/ may)" % rel)
            if top != "gfx" and re.search(r"\bgl[A-Z][A-Za-z]+\s*\(", text):
                problems.append("%s calls OpenGL (only gfx/ may)" % rel)
    for p in problems:
        print(p)
    print("layering: %s" % ("ok" if not problems else "%d problem(s)" % len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
