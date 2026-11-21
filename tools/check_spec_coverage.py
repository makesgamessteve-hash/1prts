#!/usr/bin/env python3
"""Checks that every OpenSpec scenario is linked to automated tests in tests/README.md.

Scenarios come from openspec/specs/*/spec.md and openspec/changes/<id>/specs/*/spec.md (archived
changes are skipped; their specs live in openspec/specs/). The table in tests/README.md has the
columns | Capability | Scenario | Covered by |. The check fails if
  - a scenario has no row, or a row names a scenario that does not exist (or appears twice),
  - a row's references are missing: every backticked `suite.case` must be a UTEST in
    engine/tests/*.c, every backticked path must exist, and a row needs at least one of those or
    a "pending (task X.Y)" whose task exists in a tasks.md.

usage: check_spec_coverage.py [--root <repo>]"""
import argparse
import glob
import os
import re
import sys

SCENARIO = re.compile(r"^####\s+Scenario:\s*(.+?)\s*$")
UTEST = re.compile(r"\bUTEST\(\s*(\w+)\s*,\s*(\w+)\s*\)")
PENDING = re.compile(r"pending \(task (\d+\.\d+)\)")
TASK = re.compile(r"^\s*-\s*\[[ xX]\]\s*(\d+\.\d+)\s")


def read_text(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def spec_files(root):
    files = glob.glob(os.path.join(root, "openspec", "specs", "*", "spec.md"))
    for change in sorted(glob.glob(os.path.join(root, "openspec", "changes", "*"))):
        if os.path.basename(change) != "archive":
            files += glob.glob(os.path.join(change, "specs", "*", "spec.md"))
    return sorted(files)


def scenarios(root):
    found = {}
    for path in spec_files(root):
        capability = os.path.basename(os.path.dirname(path))
        for line in read_text(path).splitlines():
            m = SCENARIO.match(line)
            if m:
                found[(capability, m.group(1))] = os.path.relpath(path, root)
    return found


def unit_tests(root):
    names = set()
    for path in glob.glob(os.path.join(root, "engine", "tests", "*.c")):
        for suite, case in UTEST.findall(read_text(path)):
            names.add("%s.%s" % (suite, case))
    return names


def task_ids(root):
    ids = set()
    for path in glob.glob(os.path.join(root, "openspec", "changes", "*", "tasks.md")):
        for line in read_text(path).splitlines():
            m = TASK.match(line)
            if m:
                ids.add(m.group(1))
    return ids


def table_rows(readme):
    """Rows of the table whose header is | Capability | Scenario | Covered by |."""
    rows = []
    in_table = False
    for n, line in enumerate(read_text(readme).splitlines(), 1):
        if not line.lstrip().startswith("|"):
            in_table = False
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if cells == ["Capability", "Scenario", "Covered by"]:
            in_table = True
        elif in_table and len(cells) == 3 and not set(cells[0]) <= set("-"):
            rows.append((n, cells[0], cells[1], cells[2]))
    return rows


def check(root):
    readme = os.path.join(root, "tests", "README.md")
    if not os.path.isfile(readme):
        return ["tests/README.md is missing"], 0
    wanted = scenarios(root)
    tests = unit_tests(root)
    tasks = task_ids(root)
    problems = []
    seen = set()
    for n, capability, scenario, covered in table_rows(readme):
        where = "tests/README.md:%d" % n
        key = (capability, scenario)
        if key not in wanted:
            problems.append("%s: no scenario '%s' in capability '%s'" % (where, scenario, capability))
            continue
        if key in seen:
            problems.append("%s: scenario '%s' (%s) is listed twice" % (where, scenario, capability))
        seen.add(key)
        refs = 0
        for ref in re.findall(r"`([^`]+)`", covered):
            if "/" in ref:
                if not os.path.exists(os.path.join(root, ref)):
                    problems.append("%s: %s does not exist" % (where, ref))
                refs += 1
            elif re.fullmatch(r"\w+\.\w+", ref):
                if ref not in tests:
                    problems.append("%s: there is no unit test UTEST(%s)" % (where, ref.replace(".", ", ")))
                refs += 1
        for task in PENDING.findall(covered):
            if task not in tasks:
                problems.append("%s: task %s does not exist in any tasks.md" % (where, task))
            refs += 1
        if refs == 0:
            problems.append("%s: '%s' names no test and is not marked pending (task X.Y)" % (where, scenario))
    for key in sorted(set(wanted) - seen):
        problems.append("%s: scenario '%s' has no row in tests/README.md" % (wanted[key], key[1]))
    return problems, len(wanted)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
    args = ap.parse_args()
    problems, count = check(os.path.abspath(args.root))
    for p in problems:
        print(p)
    print("spec coverage: %d scenario(s), %s" % (count, "all linked" if not problems else "%d problem(s)" % len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
