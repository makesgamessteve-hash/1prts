#!/usr/bin/env python3
"""Generates 1stPRTS placeholder models: glTF 2.0 binary props (.glb) and MagicaVoxel voxel
props (.vox) into data/base/models/, plus the small model fixtures for the engine's unit
tests (--fixtures). The script is MIT-licensed code; the models are original placeholder
art under CC BY 4.0 (credited in data/base/CREDITS.md). Standard library only, deterministic.

Usage: python3 tools/gen_models.py [--fixtures]"""
import base64
import json
import math
import os
import struct
import sys
import zlib

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
OUT = os.path.join(ROOT, "data", "base", "models")
FIXTURES = os.path.join(ROOT, "engine", "tests", "fixtures", "models")


# --- PNG ---------------------------------------------------------------------------------

def png_bytes(w, h, rgba):
    raw = b"".join(b"\x00" + bytes(rgba[y * w * 4:(y + 1) * w * 4]) for y in range(h))

    def chunk(tag, data):
        c = struct.pack(">I", len(data)) + tag + data
        return c + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0))
    return png + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")


# --- geometry ----------------------------------------------------------------------------

class Mesh:
    def __init__(self):
        self.pos, self.nrm, self.uv, self.col, self.idx = [], [], [], [], []

    def quad(self, a, b, c, d, n, uvs=((0, 1), (1, 1), (1, 0), (0, 0)), color=(1, 1, 1, 1)):
        base = len(self.pos)
        for p, t in zip((a, b, c, d), uvs):
            self.pos.append(p)
            self.nrm.append(n)
            self.uv.append(t)
            self.col.append(color)
        self.idx += [base, base + 1, base + 2, base, base + 2, base + 3]

    def box(self, lo, hi, uv_scale=0.5, color=(1, 1, 1, 1)):
        x0, y0, z0 = lo
        x1, y1, z1 = hi
        sx, sy, sz = (x1 - x0) / uv_scale, (y1 - y0) / uv_scale, (z1 - z0) / uv_scale

        def uvs(w, h):
            return ((0, h), (w, h), (w, 0), (0, 0))
        q = self.quad
        q((x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (0, 0, 1), uvs(sx, sy), color)
        q((x1, y0, z0), (x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (0, 0, -1), uvs(sx, sy), color)
        q((x1, y0, z1), (x1, y0, z0), (x1, y1, z0), (x1, y1, z1), (1, 0, 0), uvs(sz, sy), color)
        q((x0, y0, z0), (x0, y0, z1), (x0, y1, z1), (x0, y1, z0), (-1, 0, 0), uvs(sz, sy), color)
        q((x0, y1, z1), (x1, y1, z1), (x1, y1, z0), (x0, y1, z0), (0, 1, 0), uvs(sx, sz), color)
        q((x0, y0, z0), (x1, y0, z0), (x1, y0, z1), (x0, y0, z1), (0, -1, 0), uvs(sx, sz), color)

    def frustum(self, y0, y1, r0, r1, sides, c0=(1, 1, 1, 1), c1=(1, 1, 1, 1), cap_top=True, cap_bottom=True):
        """Truncated cone around +Y. r1 = 0 makes a cone."""
        slope = (r0 - r1) / max(1e-6, y1 - y0)
        for i in range(sides):
            a0, a1 = 2 * math.pi * i / sides, 2 * math.pi * (i + 1) / sides
            am = (a0 + a1) / 2
            n = (math.sin(am), slope, math.cos(am))
            ln = math.sqrt(sum(v * v for v in n))
            n = tuple(v / ln for v in n)
            p = [(math.sin(a0) * r0, y0, math.cos(a0) * r0), (math.sin(a1) * r0, y0, math.cos(a1) * r0),
                 (math.sin(a1) * r1, y1, math.cos(a1) * r1), (math.sin(a0) * r1, y1, math.cos(a0) * r1)]
            base = len(self.pos)
            for k, pt in enumerate(p):
                self.pos.append(pt)
                self.nrm.append(n)
                self.uv.append((i / sides + (k in (1, 2)) / sides, 1.0 if k < 2 else 0.0))
                self.col.append(c0 if k < 2 else c1)
            self.idx += [base, base + 1, base + 2, base, base + 2, base + 3]
        for cap, y, r, up in ((cap_top, y1, r1, 1), (cap_bottom, y0, r0, -1)):
            if not cap or r <= 0:
                continue
            base = len(self.pos)
            self.pos.append((0, y, 0))
            self.nrm.append((0, up, 0))
            self.uv.append((0.5, 0.5))
            self.col.append(c1 if up > 0 else c0)
            for i in range(sides):
                a = 2 * math.pi * i / sides
                self.pos.append((math.sin(a) * r, y, math.cos(a) * r))
                self.nrm.append((0, up, 0))
                self.uv.append((0.5 + 0.5 * math.sin(a), 0.5 + 0.5 * math.cos(a)))
                self.col.append(c1 if up > 0 else c0)
            for i in range(sides):
                a, b = base + 1 + i, base + 1 + (i + 1) % sides
                self.idx += [base, a, b] if up > 0 else [base, b, a]


# --- glTF --------------------------------------------------------------------------------

class Gltf:
    def __init__(self):
        self.j = {"asset": {"version": "2.0", "generator": "1stPRTS tools/gen_models.py"},
                  "scene": 0, "scenes": [{"nodes": []}], "nodes": [], "meshes": [], "materials": [],
                  "accessors": [], "bufferViews": [], "buffers": []}
        self.bin = bytearray()

    def _view(self, data, target=None):
        while len(self.bin) % 4:
            self.bin.append(0)
        view = {"buffer": 0, "byteOffset": len(self.bin), "byteLength": len(data)}
        if target:
            view["target"] = target
        self.bin += data
        self.j["bufferViews"].append(view)
        return len(self.j["bufferViews"]) - 1

    def _accessor(self, values, kind, comp=5126, target=34962, minmax=False):
        width = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}[kind]
        flat = [v for item in values for v in (item if width > 1 else (item,))]
        fmt = {5126: "f", 5125: "I", 5123: "H"}[comp]
        view = self._view(struct.pack("<%d%s" % (len(flat), fmt), *flat), target)
        acc = {"bufferView": view, "componentType": comp, "count": len(values), "type": kind}
        if minmax:
            acc["min"] = [min(v[i] for v in values) for i in range(width)]
            acc["max"] = [max(v[i] for v in values) for i in range(width)]
        self.j["accessors"].append(acc)
        return len(self.j["accessors"]) - 1

    def material(self, name, color=(1, 1, 1, 1), uri=None, png=None, unlit=False):
        m = {"name": name, "pbrMetallicRoughness": {"baseColorFactor": list(color), "metallicFactor": 0.0}}
        if uri or png:
            self.j.setdefault("images", [])
            self.j.setdefault("textures", [])
            self.j.setdefault("samplers", [{"magFilter": 9728, "minFilter": 9728}])
            if uri:
                self.j["images"].append({"uri": uri})
            else:
                self.j["images"].append({"bufferView": self._view(png), "mimeType": "image/png"})
            self.j["textures"].append({"sampler": 0, "source": len(self.j["images"]) - 1})
            m["pbrMetallicRoughness"]["baseColorTexture"] = {"index": len(self.j["textures"]) - 1}
        if unlit:
            m["extensions"] = {"KHR_materials_unlit": {}}
            self.j.setdefault("extensionsUsed", [])
            if "KHR_materials_unlit" not in self.j["extensionsUsed"]:
                self.j["extensionsUsed"].append("KHR_materials_unlit")
        self.j["materials"].append(m)
        return len(self.j["materials"]) - 1

    def mesh(self, name, parts, normals=True, colors=False):
        prims = []
        for mesh, mat in parts:
            attrs = {"POSITION": self._accessor(mesh.pos, "VEC3", minmax=True)}
            if normals:
                attrs["NORMAL"] = self._accessor(mesh.nrm, "VEC3")
            attrs["TEXCOORD_0"] = self._accessor(mesh.uv, "VEC2")
            if colors:
                attrs["COLOR_0"] = self._accessor(mesh.col, "VEC4")
            ind = self._accessor(mesh.idx, "SCALAR", 5125 if len(mesh.pos) > 65535 else 5123, 34963)
            prims.append({"attributes": attrs, "indices": ind, "material": mat})
        self.j["meshes"].append({"name": name, "primitives": prims})
        return len(self.j["meshes"]) - 1

    def node(self, name, mesh=None, parent=None, t=None, r=None, s=None):
        n = {"name": name}
        if mesh is not None:
            n["mesh"] = mesh
        if t:
            n["translation"] = list(t)
        if r:
            n["rotation"] = list(r)
        if s:
            n["scale"] = list(s)
        self.j["nodes"].append(n)
        idx = len(self.j["nodes"]) - 1
        if parent is None:
            self.j["scenes"][0]["nodes"].append(idx)
        else:
            self.j["nodes"][parent].setdefault("children", []).append(idx)
        return idx

    def glb(self):
        while len(self.bin) % 4:
            self.bin.append(0)
        self.j["buffers"] = [{"byteLength": len(self.bin)}]
        js = json.dumps(self.j, separators=(",", ":"), sort_keys=True).encode()
        js += b" " * ((4 - len(js) % 4) % 4)
        total = 12 + 8 + len(js) + 8 + len(self.bin)
        return (struct.pack("<III", 0x46546C67, 2, total) + struct.pack("<II", len(js), 0x4E4F534A) + js +
                struct.pack("<II", len(self.bin), 0x004E4942) + bytes(self.bin))

    def gltf(self, bin_uri=None):
        """JSON glTF with the buffer as a data: URI (or an external file name)."""
        self.j["buffers"] = [{"byteLength": len(self.bin),
                              "uri": bin_uri or "data:application/octet-stream;base64," +
                              base64.b64encode(bytes(self.bin)).decode()}]
        return json.dumps(self.j, indent=1, sort_keys=True).encode() + b"\n"


def quat_axis(axis, angle):
    s = math.sin(angle / 2)
    return (axis[0] * s, axis[1] * s, axis[2] * s, math.cos(angle / 2))


def bench():
    g = Gltf()
    wood = g.material("oak", (0.85, 0.72, 0.58, 1), uri="../textures/wood_dark.png")
    seat, leg = Mesh(), Mesh()
    seat.box((-0.8, -0.04, -0.18), (0.8, 0.04, 0.18), 0.6)
    leg.box((-0.04, -0.21, -0.15), (0.04, 0.21, 0.15), 0.6)
    root = g.node("bench")
    g.node("seat", g.mesh("seat", [(seat, wood)]), root, t=(0, 0.45, 0))
    leg_mesh = g.mesh("leg", [(leg, wood)])
    g.node("leg_left", leg_mesh, root, t=(-0.62, 0.21, 0))
    g.node("leg_right", leg_mesh, root, t=(0.62, 0.21, 0))
    return g.glb()


def brazier():
    g = Gltf()
    iron = g.material("iron", (0.8, 0.8, 0.85, 1), uri="../textures/iron.png")
    fire = g.material("flame", (1, 1, 1, 1), unlit=True)
    bowl, leg, flame = Mesh(), Mesh(), Mesh()
    bowl.frustum(0.0, 0.22, 0.2, 0.36, 8)
    leg.box((-0.025, -0.42, -0.025), (0.025, 0.42, 0.025), 0.5)
    flame.frustum(0.0, 0.55, 0.28, 0.0, 6, c0=(1.0, 0.45, 0.08, 1), c1=(1.0, 0.92, 0.35, 1), cap_bottom=False)
    root = g.node("brazier")
    g.node("bowl", g.mesh("bowl", [(bowl, iron)]), root, t=(0, 0.78, 0))
    legs = g.mesh("leg", [(leg, iron)])
    for i in range(3):
        a = 2 * math.pi * i / 3
        tilt = quat_axis((math.cos(a), 0, -math.sin(a)), -0.22)
        g.node("leg_%d" % i, legs, root, t=(math.sin(a) * 0.22, 0.4, math.cos(a) * 0.22), r=tilt)
    g.node("flame", g.mesh("flame", [(flame, fire)], colors=True), root, t=(0, 0.95, 0))
    g.node("marker_light", None, root, t=(0, 1.35, 0))
    return g.glb()


def banner_image():
    """A Crown of Esterholt field banner: blue and gold per pale with a gold chevron."""
    w, h = 32, 64
    px = []
    for y in range(h):
        for x in range(w):
            left = x < w // 2
            c = (40, 62, 140) if left else (196, 156, 52)
            cx = abs(x - (w - 1) / 2)
            if abs((y - 20) - cx * 0.9) < 3.2:
                c = (226, 190, 80) if left else (40, 62, 140)
            if y > h - 9 and (x // 4) % 2 == (y // 4) % 2:
                c = (120, 24, 30)
            if x in (0, w - 1) or y == 0:
                c = (70, 50, 28)
            px += [c[0], c[1], c[2], 255]
    return png_bytes(w, h, px)


def banner_stand():
    g = Gltf()
    wood = g.material("pole", (0.75, 0.62, 0.48, 1), uri="../textures/wood_dark.png")
    cloth = g.material("banner", (1, 1, 1, 1), png=banner_image())
    pole, bar, flag = Mesh(), Mesh(), Mesh()
    pole.box((-0.05, 0.0, -0.05), (0.05, 3.2, 0.05), 0.5)
    bar.box((-0.55, -0.03, -0.03), (0.55, 0.03, 0.03), 0.5)
    w, top, bottom = 0.5, 2.95, 1.05
    flag.quad((-w, bottom, 0.07), (w, bottom, 0.07), (w, top, 0.07), (-w, top, 0.07), (0, 0, 1))
    flag.quad((w, bottom, 0.05), (-w, bottom, 0.05), (-w, top, 0.05), (w, top, 0.05), (0, 0, -1))
    root = g.node("banner_stand")
    g.node("pole", g.mesh("pole", [(pole, wood)]), root)
    g.node("bar", g.mesh("bar", [(bar, wood)]), root, t=(0, 2.98, 0.06))
    g.node("cloth", g.mesh("cloth", [(flag, cloth)]), root)
    return g.glb()


# --- MagicaVoxel -------------------------------------------------------------------------

def vox_chunk(tag, content, children=b""):
    return tag + struct.pack("<II", len(content), len(children)) + content + children


def vox_dict(d):
    out = struct.pack("<I", len(d))
    for k, v in d.items():
        out += struct.pack("<I", len(k)) + k.encode() + struct.pack("<I", len(v)) + v.encode()
    return out


def vox_file(models, palette, instances=None, layers=None, extra=b""):
    """models: list of (size, {(x, y, z): color index}); palette: list of up to 255 RGBA.
    instances: list of (model, translation, rotation byte or None, hidden, layer); None
    writes no scene graph (like old MagicaVoxel versions)."""
    body = b""
    for size, voxels in models:
        body += vox_chunk(b"SIZE", struct.pack("<III", *size))
        items = sorted(voxels.items())
        data = b"".join(struct.pack("<BBBB", x, y, z, c) for (x, y, z), c in items)
        body += vox_chunk(b"XYZI", struct.pack("<I", len(items)) + data)
    if instances is not None:
        n = len(instances)
        body += vox_chunk(b"nTRN", struct.pack("<i", 0) + vox_dict({}) + struct.pack("<iiii", 1, -1, -1, 1) +
                          vox_dict({}))
        body += vox_chunk(b"nGRP", struct.pack("<i", 1) + vox_dict({}) + struct.pack("<I", n) +
                          b"".join(struct.pack("<i", 2 + i * 2) for i in range(n)))
        for i, (model, t, rot, hidden, layer) in enumerate(instances):
            attrs = {"_hidden": "1"} if hidden else {}
            frame = {"_t": "%d %d %d" % t}
            if rot is not None:
                frame["_r"] = str(rot)
            body += vox_chunk(b"nTRN", struct.pack("<i", 2 + i * 2) + vox_dict(attrs) +
                              struct.pack("<iiii", 3 + i * 2, -1, layer, 1) + vox_dict(frame))
            body += vox_chunk(b"nSHP", struct.pack("<i", 3 + i * 2) + vox_dict({}) + struct.pack("<Ii", 1, model) +
                              vox_dict({}))
    for lid, hidden in (layers or []):
        body += vox_chunk(b"LAYR", struct.pack("<i", lid) + vox_dict({"_hidden": "1"} if hidden else {}) +
                          struct.pack("<i", -1))
    if palette is not None:
        pal = list(palette) + [(0, 0, 0, 255)] * (256 - len(palette))
        body += vox_chunk(b"RGBA", b"".join(struct.pack("<BBBB", *c) for c in pal[:256]))
    body += extra
    return b"VOX " + struct.pack("<I", 150) + vox_chunk(b"MAIN", b"", body)


def crate():
    """A 12-voxel wooden crate: planks, a dark frame and iron nail heads."""
    pal = [(150, 104, 58, 255), (128, 86, 46, 255), (92, 60, 32, 255), (70, 70, 76, 255)]
    n = 12
    vox = {}
    for x in range(n):
        for y in range(n):
            for z in range(n):
                edge = sum(v in (0, n - 1) for v in (x, y, z)) >= 2
                if edge:
                    c = 3
                elif z in (0, n - 1):
                    c = 1 + (x // 3) % 2
                else:
                    c = 1 + (z // 3) % 2
                face = sum(v in (0, n - 1) for v in (x, y, z)) == 1
                if face and (x in (1, n - 2) or z in (1, n - 2)) and (y in (1, n - 2) or (x in (1, n - 2) and z in (1, n - 2))):
                    c = 4
                vox[(x, y, z)] = c
    return vox_file([((n, n, n), vox)], pal, [(0, (0, 0, n // 2), None, False, 0)], [(0, False)])


def anvil():
    """An anvil on a tree stump: two models placed by the scene graph."""
    pal = [(96, 64, 36, 255), (122, 84, 50, 255), (60, 60, 66, 255), (84, 84, 92, 255), (40, 40, 46, 255)]
    stump = {}
    for x in range(10):
        for y in range(10):
            for z in range(8):
                if (x - 4.5) ** 2 + (y - 4.5) ** 2 <= 20.5:
                    ring = (x - 4.5) ** 2 + (y - 4.5) ** 2 < 9 and z == 7
                    stump[(x, y, z)] = 2 if ring else 1
    body = {}
    for x in range(16):
        for y in range(6):
            for z in range(8):
                top = z >= 5 and (x >= 3 or abs(y - 2.5) <= (x + 1) * 0.6) and not (x < 3 and z == 5)
                waist = 2 <= z < 5 and 5 <= x <= 11 and 1 <= y <= 4
                base = z < 2 and 3 <= x <= 13
                if top or waist or base:
                    body[(x, y, z)] = 4 if z == 7 else (5 if base else 3)
    return vox_file([((10, 10, 8), stump), ((16, 6, 8), body)], pal,
                    [(0, (0, 0, 4), None, False, 0), (1, (0, 0, 12), None, False, 0)], [(0, False)])


# --- the Custodians (M10): constructs of pale stone with glowing aether seams ------------

CUSTODIAN_PALETTE = [(206, 200, 184, 255), (156, 150, 138, 255), (112, 232, 255, 255), (58, 168, 222, 255),
                     (44, 46, 56, 255)]
STONE, DARK_STONE, GLOW, SEAM, CORE = 1, 2, 3, 4, 5


def fill(vox, x0, y0, z0, x1, y1, z1, c):
    """Fills the box of voxels [x0, x1) x [y0, y1) x [z0, z1) with colour c."""
    for x in range(x0, x1):
        for y in range(y0, y1):
            for z in range(z0, z1):
                vox[(x, y, z)] = c


def custodian(size, vox):
    return vox_file([(size, vox)], CUSTODIAN_PALETTE)


def warden():
    """A warden (12 x 8 x 28): pillar legs, a broad chest with an aether seam, a visored head and
    a blade of stone on its right arm."""
    v = {}
    fill(v, 3, 2, 0, 5, 6, 11, DARK_STONE)    # legs
    fill(v, 7, 2, 0, 9, 6, 11, DARK_STONE)
    fill(v, 2, 1, 11, 10, 7, 21, STONE)       # chest
    fill(v, 5, 0, 13, 7, 1, 19, GLOW)         # the seam down the chest
    fill(v, 0, 2, 18, 12, 6, 21, STONE)       # shoulders
    fill(v, 0, 2, 10, 2, 6, 18, DARK_STONE)   # arms
    fill(v, 10, 2, 10, 12, 6, 18, DARK_STONE)
    fill(v, 10, 0, 2, 12, 2, 12, SEAM)        # the blade
    fill(v, 4, 2, 21, 8, 6, 26, STONE)        # head
    fill(v, 4, 1, 23, 8, 2, 24, GLOW)         # visor
    fill(v, 5, 3, 26, 7, 5, 28, CORE)         # crest
    return custodian((12, 8, 28), v)


def lancer():
    """A lancer (10 x 8 x 30): slimmer, with a long lance tipped with aether."""
    v = {}
    fill(v, 3, 3, 0, 4, 5, 12, DARK_STONE)
    fill(v, 6, 3, 0, 7, 5, 12, DARK_STONE)
    fill(v, 2, 2, 12, 8, 6, 21, STONE)
    fill(v, 4, 1, 14, 6, 2, 20, GLOW)
    fill(v, 3, 3, 21, 7, 6, 25, STONE)
    fill(v, 3, 2, 22, 7, 3, 23, GLOW)
    fill(v, 8, 3, 2, 9, 5, 28, SEAM)          # the lance
    fill(v, 8, 3, 28, 9, 5, 30, GLOW)
    return custodian((10, 8, 30), v)


def mote():
    """A mote (9 x 9 x 9): a stone shell round a glowing core, a double pyramid."""
    v = {}
    for z in range(9):
        r = 4 - abs(z - 4)
        for x in range(4 - r, 5 + r):
            for y in range(4 - r, 5 + r):
                inner = abs(x - 4) + abs(y - 4) + abs(z - 4) <= 2
                v[(x, y, z)] = GLOW if inner else (STONE if (x + y + z) % 3 else SEAM)
    return custodian((9, 9, 9), v)


def colossus():
    """The colossus (24 x 16 x 40): a walking siege engine of stacked slabs with a glowing heart."""
    v = {}
    fill(v, 3, 4, 0, 9, 12, 16, DARK_STONE)   # legs
    fill(v, 15, 4, 0, 21, 12, 16, DARK_STONE)
    fill(v, 2, 2, 16, 22, 14, 32, STONE)      # body
    fill(v, 9, 1, 21, 15, 2, 28, GLOW)        # heart
    fill(v, 0, 3, 18, 2, 13, 30, DARK_STONE)  # arms
    fill(v, 22, 3, 18, 24, 13, 30, DARK_STONE)
    fill(v, 0, 3, 8, 2, 13, 18, SEAM)         # fists, the rams
    fill(v, 22, 3, 8, 24, 13, 18, SEAM)
    fill(v, 7, 4, 32, 17, 12, 38, STONE)      # head
    fill(v, 8, 3, 34, 16, 4, 36, GLOW)        # eye band
    fill(v, 10, 6, 38, 14, 10, 40, CORE)
    return custodian((24, 16, 40), v)


def spire():
    """A spire (12 x 12 x 60): a tapering obelisk with aether seams and a floating cap stone."""
    v = {}
    for z in range(50):
        r = 6 - z // 10
        for x in range(6 - r, 6 + r):
            for y in range(6 - r, 6 + r):
                edge = x in (6 - r, 6 + r - 1) or y in (6 - r, 6 + r - 1)
                seam = edge and (x == 6 or y == 6 or x == 5 or y == 5)
                v[(x, y, z)] = GLOW if seam else (DARK_STONE if z < 4 else STONE)
    fill(v, 4, 4, 53, 8, 8, 58, GLOW)         # the cap stone, floating
    fill(v, 5, 5, 58, 7, 7, 60, CORE)
    return custodian((12, 12, 60), v)


# --- fixtures ----------------------------------------------------------------------------

def fixtures():
    os.makedirs(FIXTURES, exist_ok=True)
    out = {}
    # tri.gltf: JSON with an embedded buffer; one triangle without normals but with vec4
    # colors, a mirrored copy, a line primitive and a primitive without positions.
    g = Gltf()
    red = g.material("red", (1, 0, 0, 1))
    tri = Mesh()
    tri.quad((0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (0, 0, 1), color=(0.5, 1, 1, 1))
    m = g.mesh("tri", [(tri, red)], normals=False, colors=True)
    g.j["meshes"][m]["primitives"].append(dict(g.j["meshes"][m]["primitives"][0], mode=1))
    uv_only = {"TEXCOORD_0": g.j["meshes"][m]["primitives"][0]["attributes"]["TEXCOORD_0"]}
    g.j["meshes"][m]["primitives"].append({"attributes": uv_only, "material": red})
    g.node("tri", m, t=(0, 0, -2))
    g.node("mirror", m, s=(-1, 1, 1), t=(0, 0, 2))
    g.node("marker_tip", None, t=(1, 1, 0), r=quat_axis((0, 1, 0), math.pi / 2))
    out["tri.gltf"] = g.gltf()
    # ext.gltf + ext.bin: external buffer, textures next to and outside the model folder,
    # an unlit material and an untextured primitive without a material
    g = Gltf()
    good = g.material("good", uri="tex/checker.png")
    bad = g.material("bad", uri="../../../outside.png")
    glow = g.material("glow", (0.2, 0.9, 0.3, 1), unlit=True)
    quad = Mesh()
    quad.quad((0, 0, 0), (1, 0, 0), (1, 1, 0), (0, 1, 0), (0, 0, 1))
    parts = [(quad, good), (quad, bad), (quad, glow)]
    mi = g.mesh("quads", parts)
    g.j["meshes"][mi]["primitives"].append({k: v for k, v in g.j["meshes"][mi]["primitives"][0].items()
                                            if k != "material"})
    g.node("quads", mi)
    out["ext.gltf"] = g.gltf("ext.bin")
    out["ext.bin"] = bytes(g.bin)
    out["missing_bin.gltf"] = out["ext.gltf"].replace(b'"ext.bin"', b'"nowhere.bin"')
    out["tex/checker.png"] = png_bytes(2, 2, [255, 255, 255, 255, 0, 0, 0, 255] * 2)
    out["props.glb"] = brazier()
    out["banner.glb"] = banner_stand()
    out["broken.glb"] = b"glTF this is not a model"
    # voxels
    pal = [(255, 0, 0, 255), (0, 255, 0, 255)]
    cube = {(x, y, z): 1 + (x + y + z) % 2 for x in range(2) for y in range(2) for z in range(2)}
    one = {(0, 0, 0): 2}
    out["graph.vox"] = vox_file([((2, 2, 2), cube), ((1, 1, 1), one)], pal,
                                [(0, (0, 0, 1), None, False, 0), (1, (4, 0, 0), 0b0010001, False, 0),
                                 (1, (9, 9, 9), None, True, 0), (1, (-9, 0, 0), None, False, 1),
                                 (7, (0, 0, 0), None, False, 0), (1, (0, 8, 0), 0, False, 0)],
                                [(0, False), (1, True)], extra=vox_chunk(b"NOTE", b"ignored"))
    out["solid.vox"] = vox_file([((4, 3, 2), {(x, y, z): 1 for x in range(4) for y in range(3) for z in range(2)})],
                                pal, [(0, (0, 0, 0), None, False, 0)])
    out["plain.vox"] = vox_file([((3, 1, 1), {(0, 0, 0): 5, (2, 0, 0): 9})], None)
    out["truncated.vox"] = out["graph.vox"][:60]
    out["no_models.vox"] = vox_file([], pal)
    out["bad_size.vox"] = b"VOX " + struct.pack("<I", 150) + vox_chunk(
        b"MAIN", b"", vox_chunk(b"SIZE", struct.pack("<III", 0, 4, 4)))
    out["xyzi_first.vox"] = b"VOX " + struct.pack("<I", 150) + vox_chunk(
        b"MAIN", b"", vox_chunk(b"XYZI", struct.pack("<I", 0)))
    out["not_vox.vox"] = b"PNG? no."
    for name, data in out.items():
        path = os.path.join(FIXTURES, name)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(data)
    print("wrote %d fixtures to %s" % (len(out), os.path.relpath(FIXTURES, ROOT)))


def main():
    if "--fixtures" in sys.argv[1:]:
        fixtures()
        return
    os.makedirs(OUT, exist_ok=True)
    files = {"bench.glb": bench(), "brazier.glb": brazier(), "banner_stand.glb": banner_stand(),
             "crate.vox": crate(), "anvil.vox": anvil(),
             "cu_warden.vox": warden(), "cu_lancer.vox": lancer(), "cu_mote.vox": mote(),
             "cu_colossus.vox": colossus(), "cu_spire.vox": spire()}
    for name, data in files.items():
        with open(os.path.join(OUT, name), "wb") as f:
            f.write(data)
    print("wrote %d models to %s" % (len(files), os.path.relpath(OUT, ROOT)))


if __name__ == "__main__":
    main()
