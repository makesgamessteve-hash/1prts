/* 1stPRTS engine: MagicaVoxel .vox parser. MIT License.
 * Format: https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt */
#include "scene/vox.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VOX_MAX_SIZE 256
#define VOX_MAX_DEPTH 64

typedef struct rd {
    const uint8_t *p;
    size_t n, at;
    bool bad;
} rd;

static uint32_t rd_u32(rd *r)
{
    if (r->bad || r->n - r->at < 4) {
        r->bad = true;
        return 0;
    }
    const uint8_t *b = r->p + r->at;
    r->at += 4;
    return (uint32_t)b[0] | (uint32_t)b[1] << 8 | (uint32_t)b[2] << 16 | (uint32_t)b[3] << 24;
}

static int32_t rd_i32(rd *r) { return (int32_t)rd_u32(r); }

static const uint8_t *rd_bytes(rd *r, size_t n)
{
    if (r->bad || r->n - r->at < n) {
        r->bad = true;
        return NULL;
    }
    const uint8_t *b = r->p + r->at;
    r->at += n;
    return b;
}

typedef struct vox_attrs {
    bool hidden;
    bool has_rot, has_trans;
    uint8_t rot;
    float trans[3];
} vox_attrs;

/* Reads a DICT, keeping the keys the scene graph needs. */
static void rd_dict(rd *r, vox_attrs *a)
{
    memset(a, 0, sizeof(*a));
    uint32_t pairs = rd_u32(r);
    for (uint32_t i = 0; i < pairs && !r->bad; i++) {
        uint32_t klen = rd_u32(r);
        const char *key = (const char *)rd_bytes(r, klen);
        uint32_t vlen = rd_u32(r);
        const char *val = (const char *)rd_bytes(r, vlen);
        if (r->bad) return;
        char v[64];
        size_t n = vlen < sizeof(v) - 1 ? vlen : sizeof(v) - 1;
        memcpy(v, val, n);
        v[n] = '\0';
        if (klen == 7 && memcmp(key, "_hidden", 7) == 0) {
            a->hidden = v[0] == '1';
        } else if (klen == 2 && memcmp(key, "_r", 2) == 0) {
            a->has_rot = true;
            a->rot = (uint8_t)strtol(v, NULL, 10);
        } else if (klen == 2 && memcmp(key, "_t", 2) == 0) {
            char *end = v;
            a->has_trans = true;
            for (int k = 0; k < 3; k++) a->trans[k] = (float)strtol(end, &end, 10);
        }
    }
}

enum { NODE_TRN = 1, NODE_GRP, NODE_SHP };

typedef struct vox_node {
    int32_t id;
    int type;
    int32_t child, layer;
    bool hidden;
    float rot[9];
    float trans[3];
    size_t first, count; /* into links: group children, or shape models */
} vox_node;

typedef struct vox_layer {
    int32_t id;
    bool hidden;
} vox_layer;

typedef struct vox_parse {
    prts_vox *out;
    prts_array nodes;  /* vox_node */
    prts_array links;  /* int32_t */
    prts_array layers; /* vox_layer */
    char *err;
    size_t err_size;
} vox_parse;

static void identity(float m[9])
{
    memset(m, 0, sizeof(float) * 9);
    m[0] = m[4] = m[8] = 1.0f;
}

/* The _r byte: bits 0-1 and 2-3 give the column of the non-zero entry in rows 0 and 1
 * (row 2 takes the remaining one); bits 4, 5 and 6 make rows 0, 1 and 2 negative. */
static void decode_rotation(uint8_t b, float m[9])
{
    int c0 = b & 3, c1 = (b >> 2) & 3;
    if (c0 > 2 || c1 > 2 || c0 == c1) {
        identity(m);
        return;
    }
    int c2 = 3 - c0 - c1;
    memset(m, 0, sizeof(float) * 9);
    m[c0] = (b & 16) ? -1.0f : 1.0f;
    m[3 + c1] = (b & 32) ? -1.0f : 1.0f;
    m[6 + c2] = (b & 64) ? -1.0f : 1.0f;
}

static const vox_node *find_node(const vox_parse *ps, int32_t id)
{
    for (size_t i = 0; i < ps->nodes.len; i++) {
        const vox_node *n = prts_array_at(&ps->nodes, i);
        if (n->id == id) return n;
    }
    return NULL;
}

static bool layer_hidden(const vox_parse *ps, int32_t id)
{
    for (size_t i = 0; i < ps->layers.len; i++) {
        const vox_layer *l = prts_array_at(&ps->layers, i);
        if (l->id == id) return l->hidden;
    }
    return false;
}

static void visit(vox_parse *ps, int32_t id, const float rot[9], const float trans[3], int depth)
{
    const vox_node *n = find_node(ps, id);
    if (!n || depth > VOX_MAX_DEPTH) return;
    if (n->type == NODE_TRN) {
        if (n->hidden || layer_hidden(ps, n->layer)) return;
        float r[9], t[3];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                r[i * 3 + j] = rot[i * 3] * n->rot[j] + rot[i * 3 + 1] * n->rot[3 + j] + rot[i * 3 + 2] * n->rot[6 + j];
            }
            t[i] = rot[i * 3] * n->trans[0] + rot[i * 3 + 1] * n->trans[1] + rot[i * 3 + 2] * n->trans[2] + trans[i];
        }
        visit(ps, n->child, r, t, depth + 1);
    } else {
        for (size_t i = 0; i < n->count; i++) {
            int32_t link = *(const int32_t *)prts_array_at(&ps->links, n->first + i);
            if (n->type == NODE_GRP) {
                if (!n->hidden) visit(ps, link, rot, trans, depth + 1);
            } else if (link >= 0 && (size_t)link < ps->out->models.len) {
                prts_vox_instance *inst = prts_array_push(&ps->out->instances, NULL);
                inst->model = link;
                memcpy(inst->rot, rot, sizeof(inst->rot));
                memcpy(inst->trans, trans, sizeof(inst->trans));
            }
        }
    }
}

static bool fail(vox_parse *ps, const char *msg)
{
    snprintf(ps->err, ps->err_size, "%s", msg);
    return false;
}

static bool read_node(vox_parse *ps, rd *c, int type)
{
    vox_node n;
    memset(&n, 0, sizeof(n));
    n.type = type;
    n.id = rd_i32(c);
    vox_attrs a;
    rd_dict(c, &a);
    n.hidden = a.hidden;
    identity(n.rot);
    if (type == NODE_TRN) {
        n.child = rd_i32(c);
        rd_i32(c); /* reserved */
        n.layer = rd_i32(c);
        uint32_t frames = rd_u32(c);
        for (uint32_t f = 0; f < frames && !c->bad; f++) {
            vox_attrs fa;
            rd_dict(c, &fa);
            if (f) continue; /* only the first animation frame */
            if (fa.has_rot) decode_rotation(fa.rot, n.rot);
            if (fa.has_trans) memcpy(n.trans, fa.trans, sizeof(n.trans));
        }
    } else {
        uint32_t count = rd_u32(c);
        if (count > c->n) return fail(ps, "scene graph node has too many links");
        n.first = ps->links.len;
        n.count = count;
        for (uint32_t i = 0; i < count && !c->bad; i++) {
            int32_t link = rd_i32(c);
            prts_array_push(&ps->links, &link);
            if (type == NODE_SHP) rd_dict(c, &a); /* per-model attributes */
        }
    }
    if (c->bad) return fail(ps, "truncated scene graph chunk");
    prts_array_push(&ps->nodes, &n);
    return true;
}

static bool read_chunks(vox_parse *ps, rd *r)
{
    int pending[3] = {0, 0, 0};
    bool have_size = false;
    while (r->at < r->n) {
        const uint8_t *id = rd_bytes(r, 4);
        uint32_t content = rd_u32(r);
        uint32_t children = rd_u32(r);
        const uint8_t *body = rd_bytes(r, content);
        rd_bytes(r, children); /* only MAIN has children, and they are walked separately */
        if (r->bad) return fail(ps, "truncated chunk");
        rd c = {body, content, 0, false};
        if (memcmp(id, "SIZE", 4) == 0) {
            for (int k = 0; k < 3; k++) pending[k] = rd_i32(&c);
            if (c.bad) return fail(ps, "truncated SIZE chunk");
            for (int k = 0; k < 3; k++) {
                if (pending[k] < 1 || pending[k] > VOX_MAX_SIZE) return fail(ps, "model size out of range");
            }
            have_size = true;
        } else if (memcmp(id, "XYZI", 4) == 0) {
            if (!have_size) return fail(ps, "XYZI chunk without SIZE");
            have_size = false;
            uint32_t count = rd_u32(&c);
            const uint8_t *v = count <= content / 4 ? rd_bytes(&c, (size_t)count * 4) : NULL;
            if (!v) return fail(ps, "truncated XYZI chunk");
            prts_vox_model m;
            memcpy(m.size, pending, sizeof(m.size));
            m.voxels = prts_calloc((size_t)m.size[0] * (size_t)m.size[1] * (size_t)m.size[2], 1);
            for (uint32_t i = 0; i < count; i++) {
                const uint8_t *e = v + i * 4;
                if (e[0] >= m.size[0] || e[1] >= m.size[1] || e[2] >= m.size[2]) continue;
                m.voxels[(size_t)e[0] + (size_t)e[1] * (size_t)m.size[0] +
                         (size_t)e[2] * (size_t)m.size[0] * (size_t)m.size[1]] = e[3];
            }
            prts_array_push(&ps->out->models, &m);
        } else if (memcmp(id, "RGBA", 4) == 0) {
            const uint8_t *pal = rd_bytes(&c, 256 * 4);
            if (!pal) return fail(ps, "truncated RGBA chunk");
            for (int i = 0; i < 255; i++) memcpy(ps->out->palette[i + 1], pal + i * 4, 4);
            ps->out->has_palette = true;
        } else if (memcmp(id, "nTRN", 4) == 0) {
            if (!read_node(ps, &c, NODE_TRN)) return false;
        } else if (memcmp(id, "nGRP", 4) == 0) {
            if (!read_node(ps, &c, NODE_GRP)) return false;
        } else if (memcmp(id, "nSHP", 4) == 0) {
            if (!read_node(ps, &c, NODE_SHP)) return false;
        } else if (memcmp(id, "LAYR", 4) == 0) {
            vox_layer l;
            l.id = rd_i32(&c);
            vox_attrs a;
            rd_dict(&c, &a);
            if (c.bad) return fail(ps, "truncated LAYR chunk");
            l.hidden = a.hidden;
            prts_array_push(&ps->layers, &l);
        }
        /* other chunks (materials, cameras, notes...) are not needed */
    }
    return true;
}

bool prts_vox_parse(const void *data, size_t size, prts_vox *out, char *err, size_t err_size)
{
    memset(out, 0, sizeof(*out));
    prts_array_init(&out->models, sizeof(prts_vox_model));
    prts_array_init(&out->instances, sizeof(prts_vox_instance));
    /* without an RGBA chunk: a plain grey ramp, so the model is still visible */
    for (int i = 1; i < 256; i++) {
        uint8_t g = (uint8_t)(64 + (i * 191) / 255);
        out->palette[i][0] = out->palette[i][1] = out->palette[i][2] = g;
        out->palette[i][3] = 255;
    }
    vox_parse ps;
    memset(&ps, 0, sizeof(ps));
    ps.out = out;
    ps.err = err;
    ps.err_size = err_size;
    prts_array_init(&ps.nodes, sizeof(vox_node));
    prts_array_init(&ps.links, sizeof(int32_t));
    prts_array_init(&ps.layers, sizeof(vox_layer));

    rd r = {data, size, 0, false};
    const uint8_t *magic = rd_bytes(&r, 4);
    rd_u32(&r); /* version */
    const uint8_t *main_id = rd_bytes(&r, 4);
    uint32_t main_content = rd_u32(&r);
    uint32_t main_children = rd_u32(&r);
    bool ok;
    if (r.bad || memcmp(magic, "VOX ", 4) != 0 || memcmp(main_id, "MAIN", 4) != 0) {
        ok = fail(&ps, "not a MagicaVoxel file");
    } else if (!rd_bytes(&r, main_content) || r.n - r.at < main_children) {
        ok = fail(&ps, "truncated MAIN chunk");
    } else {
        rd kids = {r.p + r.at, main_children, 0, false};
        ok = read_chunks(&ps, &kids);
    }
    if (ok && out->models.len == 0) ok = fail(&ps, "the file has no models");
    if (ok) {
        float rot[9], trans[3] = {0, 0, 0};
        identity(rot);
        if (ps.nodes.len) visit(&ps, 0, rot, trans, 0);
        else {
            for (size_t i = 0; i < out->models.len; i++) {
                prts_vox_instance *inst = prts_array_push(&out->instances, NULL);
                inst->model = (int)i;
                memcpy(inst->rot, rot, sizeof(rot));
            }
        }
    }
    prts_array_free(&ps.nodes);
    prts_array_free(&ps.links);
    prts_array_free(&ps.layers);
    if (!ok) prts_vox_free(out);
    return ok;
}

void prts_vox_free(prts_vox *v)
{
    for (size_t i = 0; i < v->models.len; i++) prts_free(((prts_vox_model *)prts_array_at(&v->models, i))->voxels);
    prts_array_free(&v->models);
    prts_array_free(&v->instances);
}
