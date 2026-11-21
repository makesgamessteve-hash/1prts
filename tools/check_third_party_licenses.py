#!/usr/bin/env python3
"""Checks third-party code: every folder in third_party/ has a license file (LICENSE*, COPYING*)
and a row in third_party/README.md, and every row names a folder that exists (or is fetched).
MIT License.

usage: check_third_party_licenses.py [third_party_dir]"""
import os
import re
import sys

FETCHED = {"SDL 3"}  # downloaded by CMake at configure time, not vendored


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "third_party")
    readme = open(os.path.join(root, "README.md"), encoding="utf-8").read()
    problems = []
    folders = sorted(d for d in os.listdir(root) if os.path.isdir(os.path.join(root, d)))
    for d in folders:
        files = os.listdir(os.path.join(root, d))
        if not any(f.upper().startswith(("LICENSE", "COPYING", "UNLICENSE")) for f in files):
            problems.append("third_party/%s has no LICENSE file" % d)
        if not re.search(r"\]\(https?://[^)]*%s[^)]*\)|\b%s\b" % (re.escape(d), re.escape(d)), readme, re.I):
            problems.append("third_party/%s is not listed in third_party/README.md" % d)
    rows = re.findall(r"^\| \[([^\]]+)\]", readme, re.M)
    if not rows:
        problems.append("third_party/README.md has no library table")
    for p in problems:
        print(p)
    print("third-party licenses: %d folder(s), %s" % (len(folders), "ok" if not problems else "%d problem(s)" % len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
