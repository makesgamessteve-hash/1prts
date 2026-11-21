#!/usr/bin/env python3
"""Self-test of tools/validate_assets.py: builds small fixture packs (valid and broken files of
every checked format, credits and manifests) in a temporary folder and checks what the validator
reports. Covers the spec scenario "Uncredited asset detected".

usage: python3 -I tools/tests/test_validate_assets.py"""
import sys
sys.dont_write_bytecode = True  # the tools are imported from the repository: keep it clean
import importlib.util  # noqa: E402
import io
import json
import os
import shutil
import struct
import tempfile
import unittest
import zlib
from contextlib import redirect_stdout

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("validate_assets", os.path.join(HERE, "..", "validate_assets.py"))
va = importlib.util.module_from_spec(spec)
spec.loader.exec_module(va)


def png(width=2, height=2, depth=8, color=6, crc_ok=True):
    ihdr = struct.pack(">IIBBBBB", width, height, depth, color, 0, 0, 0)
    crc = zlib.crc32(b"IHDR" + ihdr) & 0xffffffff
    if not crc_ok:
        crc ^= 1
    idat = zlib.compress(b"\0" * (width * 4 + 1) * height)
    chunk = lambda t, d: struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xffffffff)
    return b"\x89PNG\r\n\x1a\n" + struct.pack(">I", 13) + b"IHDR" + ihdr + struct.pack(">I", crc) + \
        chunk(b"IDAT", idat) + chunk(b"IEND", b"")


def wav(fmt_size=16, with_fmt=True):
    fmt = struct.pack("<HHIIHH", 1, 1, 22050, 44100, 2, 16) + b"\0" * (fmt_size - 16) if fmt_size >= 16 else b"\0" * fmt_size
    body = b"WAVE" + (b"LIST" + struct.pack("<I", 3) + b"abc\0" if True else b"")
    if with_fmt:
        body += b"fmt " + struct.pack("<I", fmt_size) + fmt
    body += b"data" + struct.pack("<I", 4) + b"\0\0\0\0"
    return b"RIFF" + struct.pack("<I", len(body)) + body


def ogg(version=0):
    return b"OggS" + bytes([version]) + b"\x02" + b"\0" * 21 + b"\x01" + b"\x1e" + b"\0" * 30


def glb(version=2, length=None):
    body = b"glTF" + struct.pack("<II", version, 0)
    return body[:8] + struct.pack("<I", length if length is not None else 12) + body[12:]


def vox(main=True):
    return b"VOX " + struct.pack("<I", 150) + (b"MAIN" if main else b"PACK") + struct.pack("<II", 0, 0)


MANIFEST = 'return { id = "%s", api = "1", version = "0.1.0", depends = { "base >= 0.1" }, load_after = { "x" } }\n'
CREDITS_HEAD = "| Path | Title | Author | License | Source | Changes |\n|---|---|---|---|---|---|\n"


class PackTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.mkdtemp(prefix="validate_assets_")

    def tearDown(self):
        shutil.rmtree(self.tmp)

    def pack(self, name="base", files=None, credits=("*/*",), manifest=None, license_="CC-BY-4.0"):
        root = os.path.join(self.tmp, name)
        os.makedirs(root)
        default = {
            "textures/a.png": png(), "sounds/a.wav": wav(), "music/a.ogg": ogg(), "models/a.glb": glb(),
            "models/b.gltf": json.dumps({"asset": {"version": "2.0"}}).encode(), "voxels/a.vox": vox(),
            "scripts/main.lua": b'prts.scene.material{ texture = "textures/a.png" }\n',
            "shaders/world.frag": b"void main() {}\n",
        }
        if files is not None:
            default.update(files)
        for rel, data in default.items():
            if data is None:
                continue
            path = os.path.join(root, rel)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "wb") as f:
                f.write(data)
        if manifest is not False:
            with open(os.path.join(root, "mod.lua"), "w") as f:
                f.write(manifest if manifest is not None else MANIFEST % name)
        if credits is not None:
            with open(os.path.join(root, "CREDITS.md"), "w") as f:
                f.write(CREDITS_HEAD + "".join("| `%s` | t | a | %s | original | none |\n" % (g, license_) for g in credits))
        return root

    def problems(self, root):
        return "\n".join(va.check_pack(root))

    def test_valid_pack_passes(self):
        self.assertEqual("", self.problems(self.pack()))

    def test_uncredited_asset_is_named(self):
        # Spec: Uncredited asset detected
        root = self.pack(files={"sprites/new_guard.png": png()}, credits=("textures/*", "sounds/*", "music/*",
                                                                           "models/*", "voxels/*"))
        out = self.problems(root)
        self.assertIn("sprites/new_guard.png has no CREDITS.md entry", out)
        self.assertEqual(1, out.count("no CREDITS.md entry"))

    def test_broken_formats_are_reported(self):
        cases = {
            "textures/sig.png": (b"GIF89a" + b"\0" * 40, "bad signature"),
            "textures/ihdr.png": (b"\x89PNG\r\n\x1a\n" + b"\0" * 30, "no IHDR"),
            "textures/crc.png": (png(crc_ok=False), "CRC mismatch"),
            "textures/zero.png": (png(width=0), "zero size"),
            "textures/depth.png": (png(depth=3, color=2), "invalid bit depth 3"),
            "sounds/riff.wav": (b"RIFX" + b"\0" * 20, "expected RIFF/WAVE"),
            "sounds/nofmt.wav": (wav(with_fmt=False), "no fmt chunk"),
            "sounds/shortfmt.wav": (wav(fmt_size=8), "fmt chunk is too short"),
            "music/magic.ogg": (b"RIFF" + b"\0" * 40, "expected an OggS page"),
            "music/version.ogg": (ogg(version=1), "page header is damaged"),
            "models/magic.glb": (b"GLTF" + b"\0" * 8, "expected glTF magic"),
            "models/v1.glb": (glb(version=1), "version 1"),
            "models/len.glb": (glb(length=99), "does not match the file size"),
            "models/json.gltf": (b"{ not json", "not valid JSON"),
            "models/old.gltf": (json.dumps({"asset": {"version": "1.0"}}).encode(), "asset.version 2.x"),
            "models/list.gltf": (b"[1, 2]", "asset.version 2.x"),
            "voxels/magic.vox": (b"VOXX" + b"\0" * 12, "expected VOX magic"),
            "voxels/main.vox": (vox(main=False), "no MAIN chunk"),
        }
        root = self.pack(files={k: v[0] for k, v in cases.items()})
        out = self.problems(root)
        for rel, (_, message) in cases.items():
            self.assertIn("%s: %s" % (rel, ""), out)
            line = [l for l in out.splitlines() if (" %s: " % rel) in l]
            self.assertEqual(1, len(line), rel)
            self.assertIn(message, line[0], rel)

    def test_manifest_problems(self):
        cases = [
            ('{ id = "x" }\n', "must return a table"),
            ('return { api = "1" }\n', "has no id"),
            ('return { id = "other", api = "1" }\n', "does not match the folder name"),
            ('return { id = "base" }\n', "has no api version"),
            ('return { id = "base", api = "1", version = "one" }\n', "is not major.minor.patch"),
            ('return { id = "base", api = "1", depends = { "base >>> 1" } }\n', "invalid depends entry 'base >>> 1'"),
            ('return { id = "base", api = "1", depends = "base" }\n', "depends must be a list of strings"),
            ('return { id = "base", api = "1", load_after = { "x >= 1" } }\n', "invalid load_after entry"),
        ]
        for i, (manifest, message) in enumerate(cases):
            sub = os.path.join(self.tmp, str(i))
            os.makedirs(sub)
            root = self.pack(name=os.path.join(str(i), "base"), manifest=manifest)
            self.assertIn(message, self.problems(root), manifest)
        # comments are ignored, numbers are fine for api
        root = self.pack(name="numeric", manifest='-- id = "nope"\nreturn { id = "numeric", api = 1 }\n')
        self.assertEqual("", self.problems(root))

    def test_missing_files_licenses_and_references(self):
        self.assertIn("missing CREDITS.md", self.problems(self.pack(name="a", credits=None)))
        self.assertIn("missing mod.lua", self.problems(self.pack(name="b", manifest=False)))
        self.assertIn("has license CC-BY-NC-4.0", self.problems(self.pack(name="c", license_="CC-BY-NC-4.0")))
        root = self.pack(name="d", files={"scripts/map.lua": b'x = "textures/missing.png"\n'})
        self.assertIn("scripts reference missing file textures/missing.png", self.problems(root))

    def test_main_reports_and_exits(self):
        self.pack(name="good")
        out = io.StringIO()
        with redirect_stdout(out):
            self.assertEqual(0, va.main([self.tmp]))
        self.assertIn("assets: 1 pack(s), ok", out.getvalue())
        self.pack(name="bad", files={"art/x.png": png()}, credits=("textures/*",))
        out = io.StringIO()
        with redirect_stdout(out):
            self.assertEqual(1, va.main([self.tmp]))
        self.assertIn("bad: art/x.png has no CREDITS.md entry", out.getvalue().replace(self.tmp + os.sep, ""))

    def test_repository_data_passes(self):
        out = io.StringIO()
        with redirect_stdout(out):
            self.assertEqual(0, va.main([os.path.join(HERE, "..", "..", "data")]), out.getvalue())


if __name__ == "__main__":
    unittest.main(verbosity=2)
