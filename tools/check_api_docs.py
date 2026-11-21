#!/usr/bin/env python3
"""Checks that every Lua function the engine registers is documented in docs/modding/api-m1.md.

It scans engine/src/**/*.c for
  - `prts_script_fn NAME[] = {{"fn", c_fn}, ...}` tables and the `prts_script_register(s, "ns", NAME)`
    call that puts them into prts.<ns> (NULL: prts itself),
  - `luaL_Reg NAME[]` tables other than Lua's own libraries; their Lua prefix is given by an
    `/* api-docs: <prefix> */` comment on the line above (for example the scenario object `t`),
  - functions defined by Lua code embedded in C strings (`function T.case(...)`, `t.input = { hold = ...}`),
and requires each fully qualified name (`prts.scene.box`, `t.place`, ...) to appear in backticks in
the document. Names starting with `_` are private and skipped. Whatever exists when the check runs
is checked, so a new API must come with its documentation.

usage: check_api_docs.py [--list] [--doc path]"""
import argparse
import os
import re
import sys

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SRC = os.path.join(ROOT, "engine", "src")
DOC = os.path.join(ROOT, "docs", "modding", "api-m1.md")

ARRAY = re.compile(r"(?:static\s+)?const\s+(prts_script_fn|luaL_Reg)\s+(\w+)\s*\[\s*\]\s*=\s*\{(.*?)\};", re.S)
ENTRY = re.compile(r'\{\s*"(\w+)"\s*,\s*(\w+)\s*\}')
REGISTER = r'prts_script_register\s*\([^;]*?,\s*(NULL|"([\w.]+)")\s*,\s*%s\s*\)'
MARKER = re.compile(r"/\*\s*api-docs:\s*([\w.]+)\s*\*/")
LUA_FUNCTION = re.compile(r"function\s+([A-Za-z_]\w*(?:\.[A-Za-z_]\w*)+)\s*\(")
LUA_TABLE_OPEN = re.compile(r"^\s*([A-Za-z_]\w*(?:\.[A-Za-z_]\w*)+)\s*=\s*\{\s*$")
LUA_TABLE_FIELD = re.compile(r"^\s*([A-Za-z_]\w*)\s*=\s*function\s*\(")


def c_strings(text):
    """Yields the contents of each C string literal line (prelude code), unescaped enough."""
    for line in text.splitlines():
        m = re.match(r'^\s*"(.*)"\s*;?\s*$', line)
        if m:
            yield m.group(1).replace("\\n", "").replace('\\"', '"')


def scan(src_dir):
    """Returns {qualified name: "file:line"} and a list of problems."""
    names, problems = {}, []
    for dirpath, _, files in os.walk(src_dir):
        for fname in sorted(files):
            if not fname.endswith(".c"):
                continue
            path = os.path.join(dirpath, fname)
            rel = os.path.relpath(path, ROOT).replace("\\", "/")
            with open(path, encoding="utf-8") as f:
                text = f.read()
            for m in ARRAY.finditer(text):
                kind, var, body = m.groups()
                line = text.count("\n", 0, m.start()) + 1
                entries = ENTRY.findall(body)
                if kind == "luaL_Reg":
                    if entries and all(fn.startswith("luaopen_") for _, fn in entries) or not entries:
                        continue  # Lua's standard libraries
                    before = text[: m.start()].rstrip().splitlines()
                    marker = MARKER.search(before[-1]) if before else None
                    if not marker:
                        problems.append("%s:%d: luaL_Reg table %s needs an /* api-docs: <prefix> */ comment above it"
                                        % (rel, line, var))
                        continue
                    prefixes = [marker.group(1)]
                else:
                    regs = re.findall(REGISTER % re.escape(var), text)
                    if not regs:
                        problems.append("%s:%d: prts_script_fn table %s is never passed to prts_script_register"
                                        % (rel, line, var))
                        continue
                    prefixes = ["prts" if ns == "NULL" else "prts." + name for ns, name in regs]
                for prefix in prefixes:
                    for fn, _ in entries:
                        if not fn.startswith("_"):
                            names.setdefault(prefix + "." + fn, "%s:%d" % (rel, line))
            # Lua code embedded in C strings (test preludes)
            table = None
            for code in c_strings(text):
                for full in LUA_FUNCTION.findall(code):
                    if not full.split(".")[-1].startswith("_"):
                        names.setdefault(full, rel)
                opened = LUA_TABLE_OPEN.match(code)
                if opened:
                    table = opened.group(1)
                    continue
                if table:
                    field = LUA_TABLE_FIELD.match(code)
                    if field:
                        names.setdefault(table + "." + field.group(1), rel)
                    elif code.strip().startswith("}"):
                        table = None
    return names, problems


def documented(doc_text, name):
    return re.search(r"`" + re.escape(name) + r"(?![\w.])", doc_text) is not None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--list", action="store_true", help="print every registered name and exit")
    ap.add_argument("--doc", default=DOC)
    ap.add_argument("--src", default=SRC)
    args = ap.parse_args()
    names, problems = scan(args.src)
    if args.list:
        for n in sorted(names):
            print("%-36s %s" % (n, names[n]))
        return 0
    try:
        with open(args.doc, encoding="utf-8") as f:
            doc = f.read()
    except OSError:
        print("api docs: %s is missing" % args.doc)
        return 1
    missing = sorted(n for n in names if not documented(doc, n))
    for n in missing:
        problems.append("%s (%s) is not documented in %s" % (n, names[n], os.path.relpath(args.doc, ROOT)))
    for p in problems:
        print(p)
    print("api docs: %d function(s), %s" % (len(names), "all documented" if not problems else "%d problem(s)" % len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
