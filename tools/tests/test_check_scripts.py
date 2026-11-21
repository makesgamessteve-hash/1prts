#!/usr/bin/env python3
"""Self-tests of tools/check_api_docs.py and tools/check_spec_coverage.py on small fixture trees.

usage: python3 -I tools/tests/test_check_scripts.py"""
import sys
sys.dont_write_bytecode = True  # the tools are imported from the repository: keep it clean
import importlib.util  # noqa: E402
import io
import os
import shutil
import tempfile
import unittest
from contextlib import redirect_stdout

HERE = os.path.dirname(os.path.abspath(__file__))


def load(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(HERE, "..", name + ".py"))
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


api = load("check_api_docs")
cov = load("check_spec_coverage")


def write(root, rel, text):
    path = os.path.join(root, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        f.write(text)


ENGINE_C = r'''
static const prts_script_fn audio[] = {{"play", l_play}, {"stop", l_stop}, {NULL, NULL}};
static const prts_script_fn root[] = {{"version_of", l_version}, {NULL, NULL}};
static const luaL_Reg libs[] = {{"_G", luaopen_base}, {"string", luaopen_string}};
/* api-docs: t */
static const luaL_Reg T[] = {{"place", t_place}, {"_hold", t_hold}, {NULL, NULL}};
static const char *PRELUDE =
    "function t.wait_until(fn, s)\n"
    "t.input = {\n"
    "  hold = function(a, s) end,\n"
    "}\n"
    "function t._private() end\n";
void open(prts_script *s)
{
    prts_script_register(s, "audio", audio);
    prts_script_register(s, NULL, root);
}
'''


class ApiDocsTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="check_api_docs_")
        write(self.tmp, "src/app/api.c", ENGINE_C)

    def tearDown(self):
        shutil.rmtree(self.tmp)

    def test_scan_finds_registered_and_prelude_functions(self):
        names, problems = api.scan(os.path.join(self.tmp, "src"))
        self.assertEqual([], problems)
        self.assertEqual(["prts.audio.play", "prts.audio.stop", "prts.version_of", "t.input.hold", "t.place",
                          "t.wait_until"], sorted(names))

    def test_tables_without_a_namespace_are_problems(self):
        write(self.tmp, "src/app/bad.c", 'static const prts_script_fn lost[] = {{"x", l_x}, {NULL, NULL}};\n'
                                         'static const luaL_Reg anon[] = {{"y", l_y}, {NULL, NULL}};\n')
        _, problems = api.scan(os.path.join(self.tmp, "src"))
        self.assertEqual(2, len(problems))
        self.assertIn("never passed to prts_script_register", problems[0] + problems[1])
        self.assertIn("api-docs", problems[0] + problems[1])

    def test_documented_needs_the_exact_name_in_backticks(self):
        doc = "`prts.audio.play{ sound }` and `prts.audio.stopper()` and prts.version_of"
        self.assertTrue(api.documented(doc, "prts.audio.play"))
        self.assertFalse(api.documented(doc, "prts.audio.stop"))
        self.assertFalse(api.documented(doc, "prts.version_of"))

    def test_main_fails_for_an_undocumented_function(self):
        import sys
        doc = os.path.join(self.tmp, "api.md")
        write(self.tmp, "api.md", "`prts.audio.play` `prts.version_of` `t.place` `t.wait_until` `t.input.hold`\n")
        argv = sys.argv
        try:
            for extra, want in (([], 1), (["--list"], 0)):
                sys.argv = ["check_api_docs.py", "--src", os.path.join(self.tmp, "src"), "--doc", doc] + extra
                out = io.StringIO()
                with redirect_stdout(out):
                    self.assertEqual(want, api.main())
            write(self.tmp, "api.md", "`prts.audio.play` `prts.audio.stop` `prts.version_of` `t.place` "
                                      "`t.wait_until` `t.input.hold`\n")
            sys.argv = ["check_api_docs.py", "--src", os.path.join(self.tmp, "src"), "--doc", doc]
            out = io.StringIO()
            with redirect_stdout(out):
                self.assertEqual(0, api.main())
            self.assertIn("6 function(s), all documented", out.getvalue())
            sys.argv = ["check_api_docs.py", "--src", os.path.join(self.tmp, "src"), "--doc", doc + ".none"]
            with redirect_stdout(io.StringIO()):
                self.assertEqual(1, api.main())
        finally:
            sys.argv = argv

    def test_the_repository_is_documented(self):
        names, problems = api.scan(api.SRC)
        self.assertEqual([], problems)
        with open(api.DOC, encoding="utf-8") as f:
            doc = f.read()
        self.assertEqual([], [n for n in names if not api.documented(doc, n)])


SPEC = """# Spec
### Requirement: Doors
#### Scenario: Opening a door
- WHEN ...
#### Scenario: Closing a door
### Requirement: Lifts
#### Scenario: Riding the lift
"""
TASKS = "# Tasks\n- [x] 1.1 something\n- [ ] 2.3 later\n"
HEADER = "| Capability | Scenario | Covered by |\n|---|---|---|\n"


class SpecCoverageTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="check_spec_coverage_")
        write(self.tmp, "openspec/changes/demo/specs/doors/spec.md", SPEC)
        write(self.tmp, "openspec/changes/demo/tasks.md", TASKS)
        write(self.tmp, "openspec/changes/archive/old/specs/doors/spec.md", "#### Scenario: Archived one\n")
        write(self.tmp, "openspec/specs/frames/spec.md", "#### Scenario: Riding a moving frame\n")
        write(self.tmp, "engine/tests/test_doors.c", "UTEST(door, opens)\n")
        write(self.tmp, "tests/game/door.lua", "return {}\n")

    def tearDown(self):
        shutil.rmtree(self.tmp)

    def check(self, rows):
        write(self.tmp, "tests/README.md", "# Tests\n\n| Other | table |\n|---|---|\n| x | y |\n\n" + HEADER + rows)
        return cov.check(self.tmp)

    def test_complete_table_passes(self):
        problems, count = self.check(
            "| doors | Opening a door | `door.opens`, `tests/game/door.lua` |\n"
            "| doors | Closing a door | pending (task 2.3) |\n"
            "| doors | Riding the lift | `door.opens` |\n"
            "| frames | Riding a moving frame | `tests/game/door.lua` |\n")
        self.assertEqual([], problems)
        self.assertEqual(4, count)

    def test_every_problem_is_reported(self):
        problems, _ = self.check(
            "| doors | Opening a door | `door.shuts`, `tests/game/none.lua` |\n"
            "| doors | Opening a door | pending (task 9.9) |\n"
            "| doors | Closing a door | someday |\n"
            "| doors | Flying | `door.opens` |\n")
        text = "\n".join(problems)
        self.assertIn("there is no unit test UTEST(door, shuts)", text)
        self.assertIn("tests/game/none.lua does not exist", text)
        self.assertIn("is listed twice", text)
        self.assertIn("task 9.9 does not exist", text)
        self.assertIn("'Closing a door' names no test", text)
        self.assertIn("no scenario 'Flying'", text)
        self.assertIn("'Riding the lift' has no row", text)
        self.assertIn("'Riding a moving frame' has no row", text)
        self.assertNotIn("Archived one", text)

    def test_missing_readme_and_main(self):
        problems, _ = cov.check(self.tmp)
        self.assertEqual(["tests/README.md is missing"], problems)
        import sys
        argv = sys.argv
        try:
            sys.argv = ["check_spec_coverage.py", "--root", self.tmp]
            with redirect_stdout(io.StringIO()):
                self.assertEqual(1, cov.main())
            sys.argv = ["check_spec_coverage.py"]
            out = io.StringIO()
            with redirect_stdout(out):
                self.assertEqual(0, cov.main())  # the repository itself
            self.assertIn("all linked", out.getvalue())
        finally:
            sys.argv = argv


if __name__ == "__main__":
    unittest.main(verbosity=2)
