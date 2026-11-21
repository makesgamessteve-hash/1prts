#!/usr/bin/env python3
"""Extract-and-run smoke test of a release archive (spec: Extract and run, Licenses included).
MIT License.

usage: package_smoke.py <archive.zip|archive.tar.gz> <scenario.lua>

Extracts the archive into a new empty folder, checks that the licenses of the code, the art and
every third-party library are inside, then runs the extracted executable headless from another
working directory: a short run and a scenario. Exits non-zero on the first problem."""
import os
import subprocess
import sys
import tarfile
import tempfile
import zipfile

REQUIRED = ["LICENSE", "LICENSE-ASSETS", "LICENSING.md", "data/base/mod.lua", "data/base/CREDITS.md",
            "licenses/README.md", "licenses/lua", "licenses/stb", "licenses/cgltf", "licenses/SDL3"]


def extract(archive, dest):
    if archive.endswith(".zip"):
        with zipfile.ZipFile(archive) as z:
            z.extractall(dest)
            # zipfile drops the executable bit
            for info in z.infolist():
                mode = (info.external_attr >> 16) & 0o777
                if mode:
                    os.chmod(os.path.join(dest, info.filename), mode)
    else:
        with tarfile.open(archive) as t:
            t.extractall(dest, filter="data")
    tops = os.listdir(dest)
    return os.path.join(dest, tops[0]) if len(tops) == 1 else dest


def run(cmd, cwd):
    print("$", " ".join(cmd))
    p = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=300)
    print(p.stdout[-2000:], p.stderr[-2000:], sep="")
    return p.returncode


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    archive, scenario = os.path.abspath(sys.argv[1]), os.path.abspath(sys.argv[2])
    with tempfile.TemporaryDirectory() as tmp:
        root = extract(archive, os.path.join(tmp, "extracted"))
        missing = [p for p in REQUIRED if not os.path.exists(os.path.join(root, p))]
        for lib in ("lua", "stb", "cgltf", "SDL3"):
            folder = os.path.join(root, "licenses", lib)
            if os.path.isdir(folder) and not os.listdir(folder):
                missing.append("licenses/%s/<license file>" % lib)
        if missing:
            print("the package is missing: " + ", ".join(missing))
            return 1
        exe = os.path.join(root, "1stprts.exe" if os.name == "nt" else "1stprts")
        elsewhere = os.path.join(tmp, "cwd")
        os.makedirs(elsewhere)
        user = os.path.join(tmp, "user")
        if run([exe, "--headless", "--ticks", "60", "--user", user], elsewhere) != 0:
            print("the extracted game did not run")
            return 1
        if run([exe, "--headless", "--scenario", scenario, "--user", user], elsewhere) != 0:
            print("the scenario failed in the extracted game")
            return 1
    print("package ok: %s" % os.path.basename(archive))
    return 0


if __name__ == "__main__":
    sys.exit(main())
