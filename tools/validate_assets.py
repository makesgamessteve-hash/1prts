#!/usr/bin/env python3
"""Asset checks for every pack under data/:
  - each pack has mod.lua and CREDITS.md
  - mod.lua is a valid manifest: returns a table with id (the folder name) and api, and a
    well-formed version, depends and load_after if present
  - every art/content file is covered by a CREDITS.md row (path or glob) with an allowed license
  - every texture path used by a script exists in the pack
  - files parse as their format: PNG (signature and IHDR), WAV (RIFF/WAVE with a fmt chunk),
    OGG ("OggS" pages), glTF (.glb header, .gltf JSON with asset.version 2.x), MagicaVoxel (.vox)
Code (scripts/**, shaders/**) is MIT and needs no credits row.

usage: validate_assets.py [data_dir]"""
import fnmatch
import json
import os
import re
import struct
import sys
import zlib

ALLOWED = {"CC-BY-4.0", "CC0-1.0"}
CODE_DIRS = ("scripts/", "shaders/")
META = {"mod.lua", "CREDITS.md"}
VERSION = re.compile(r"^\d+(\.\d+){0,2}$")
DEPENDENCY = re.compile(r"^\s*[A-Za-z0-9_-]+\s*(?:(>=|<=|==|>|<|=)\s*\d+(\.\d+){0,2}\s*)?$")


def read_text(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def credits(pack):
    rows = []
    for line in read_text(os.path.join(pack, "CREDITS.md")).splitlines():
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) >= 6 and cells[0].startswith("`"):
            rows.append((cells[0].strip("`"), cells[3]))
    return rows


# --- formats -----------------------------------------------------------------------------------

def check_png(data):
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        return "not a PNG file (bad signature)"
    if len(data) < 33 or data[12:16] != b"IHDR" or struct.unpack(">I", data[8:12])[0] != 13:
        return "PNG has no IHDR header chunk"
    width, height, depth, color = struct.unpack(">IIBB", data[16:26])
    if struct.unpack(">I", data[29:33])[0] != zlib.crc32(data[12:29]) & 0xffffffff:
        return "PNG header chunk is damaged (CRC mismatch)"
    if width == 0 or height == 0:
        return "PNG has a zero size"
    allowed = {0: (1, 2, 4, 8, 16), 2: (8, 16), 3: (1, 2, 4, 8), 4: (8, 16), 6: (8, 16)}
    if color not in allowed or depth not in allowed[color]:
        return "PNG has an invalid bit depth %d for color type %d" % (depth, color)
    return None


def check_wav(data):
    if len(data) < 12 or data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        return "not a WAV file (expected RIFF/WAVE)"
    pos = 12
    while pos + 8 <= len(data):
        chunk, size = data[pos:pos + 4], struct.unpack("<I", data[pos + 4:pos + 8])[0]
        if chunk == b"fmt ":
            return None if size >= 16 and pos + 8 + size <= len(data) else "WAV fmt chunk is too short"
        pos += 8 + size + (size & 1)
    return "WAV has no fmt chunk"


def check_ogg(data):
    if data[:4] != b"OggS":
        return "not an OGG file (expected an OggS page)"
    return None if len(data) >= 27 and data[4] == 0 else "OGG page header is damaged"


def check_glb(data):
    if len(data) < 12 or data[:4] != b"glTF":
        return "not a binary glTF file (expected glTF magic)"
    version, length = struct.unpack("<II", data[4:12])
    if version != 2:
        return "binary glTF version %d (only 2 is supported)" % version
    if length != len(data):
        return "binary glTF length field %d does not match the file size %d" % (length, len(data))
    return None


def check_gltf(data):
    try:
        doc = json.loads(data.decode("utf-8"))
    except (UnicodeDecodeError, ValueError) as err:
        return "glTF is not valid JSON (%s)" % err
    version = str(doc.get("asset", {}).get("version", "")) if isinstance(doc, dict) else ""
    return None if version.startswith("2") else "glTF needs asset.version 2.x"


def check_vox(data):
    if len(data) < 8 or data[:4] != b"VOX ":
        return "not a MagicaVoxel file (expected VOX magic)"
    return None if data[8:12] == b"MAIN" else "MagicaVoxel file has no MAIN chunk"


FORMATS = {".png": check_png, ".wav": check_wav, ".ogg": check_ogg, ".glb": check_glb, ".gltf": check_gltf,
           ".vox": check_vox}


def check_format(path):
    check = FORMATS.get(os.path.splitext(path)[1].lower())
    if not check:
        return None
    with open(path, "rb") as f:
        return check(f.read())


# --- manifest ----------------------------------------------------------------------------------

def lua_strings(text):
    return re.findall(r'"([^"]*)"', text)


def check_manifest(pack):
    """mod.lua is plain Lua data; the checks here mirror what the engine requires."""
    name = os.path.basename(os.path.normpath(pack))
    text = read_text(os.path.join(pack, "mod.lua"))
    code = re.sub(r"--[^\n]*", "", text)
    problems = []
    if not re.search(r"^\s*return\s*\{", code, re.M):
        return ["%s: mod.lua must return a table" % pack]
    m = re.search(r"\bid\s*=\s*\"([^\"]*)\"", code)
    if not m:
        problems.append("%s: mod.lua has no id" % pack)
    elif m.group(1) != name:
        problems.append("%s: mod.lua id '%s' does not match the folder name '%s'" % (pack, m.group(1), name))
    if not re.search(r"\bapi\s*=\s*(\"\d+\"|\d+)", code):
        problems.append("%s: mod.lua has no api version (for example api = \"1\")" % pack)
    m = re.search(r"\bversion\s*=\s*\"([^\"]*)\"", code)
    if m and not VERSION.match(m.group(1)):
        problems.append("%s: mod.lua version '%s' is not major.minor.patch" % (pack, m.group(1)))
    for key in ("depends", "load_after"):
        m = re.search(r"\b%s\s*=\s*\{([^}]*)\}" % key, code)
        if re.search(r"\b%s\s*=" % key, code) and not m:
            problems.append("%s: mod.lua %s must be a list of strings" % (pack, key))
            continue
        for entry in lua_strings(m.group(1)) if m else []:
            ok = DEPENDENCY.match(entry) if key == "depends" else re.fullmatch(r"[A-Za-z0-9_-]+", entry)
            if not ok:
                problems.append("%s: mod.lua has an invalid %s entry '%s'" % (pack, key, entry))
    return problems


# --- packs -------------------------------------------------------------------------------------

def check_pack(pack):
    problems = []
    for need in sorted(META):
        if not os.path.isfile(os.path.join(pack, need)):
            problems.append("%s: missing %s" % (pack, need))
    if problems:
        return problems
    problems += check_manifest(pack)
    rows = credits(pack)
    for glob_, lic in rows:
        if lic not in ALLOWED:
            problems.append("%s: CREDITS row %s has license %s (allowed: %s)" % (pack, glob_, lic, ", ".join(sorted(ALLOWED))))
    scripts = ""
    for dirpath, dirs, files in os.walk(pack):
        dirs.sort()
        for name in sorted(files):
            full = os.path.join(dirpath, name)
            rel = os.path.relpath(full, pack).replace("\\", "/")
            if rel in META:
                continue
            if rel.startswith(CODE_DIRS):
                if rel.endswith(".lua"):
                    scripts += read_text(full)
                continue
            if not any(fnmatch.fnmatch(rel, g) for g, _ in rows):
                problems.append("%s: %s has no CREDITS.md entry" % (pack, rel))
            bad = check_format(full)
            if bad:
                problems.append("%s: %s: %s" % (pack, rel, bad))
    for ref in sorted(set(re.findall(r'"(textures/[^"]+)"', scripts))):
        if not os.path.isfile(os.path.join(pack, ref)):
            problems.append("%s: scripts reference missing file %s" % (pack, ref))
    return problems


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    data = argv[0] if argv else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data")
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
