/* 1stPRTS engine: glTF 2.0 and MagicaVoxel (.vox) model loading into scene geometry.
 * MIT License.
 *
 * Models are built into a temporary list of meshes in model space (meters, +Y up), then
 * placed: glTF keeps its origin and is scaled by `scale`; voxel models are centred on the
 * placement point with their lowest voxel on it, and `scale` is the size of one voxel. */
#include <cgltf.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "prts/prts_log.h"
#include "prts/prts_model.h"
#include "scene/vox.h"

#define MODEL_MAX_SEGMENTS 64
#define MODEL_MAX_VERTICES (4u * 1024u * 1024u)

typedef struct built_mesh {
    int material;
    bool flip; /* mirrored node transform: reverse the winding when placing */
    prts_mesh_cpu mesh;
} built_mesh;

typedef struct built_marker {
    char name[PRTS_SCENE_NAME_MAX];
    prts_vec3 pos;
    float yaw;
} built_marker;

typedef struct model_build {
    prts_scene *scene;
    prts_vfs *vfs;
    const char *path;
    prts_array meshes;  /* built_mesh */
    prts_array markers; /* built_marker */
    size_t vertex_count;
} model_build;

/* --- paths ---------------------------------------------------------------------------- */

bool prts_model_resolve_uri(const char *model_path, const char *uri, char *out, size_t out_size)
{
    char buf[512];
    const char *slash = strrchr(model_path, '/');
    size_t dir_len = slash ? (size_t)(slash - model_path) + 1 : 0;
    size_t uri_len = strlen(uri);
    if (out_size == 0 || uri_len == 0 || dir_len + uri_len >= sizeof(buf)) return false;
    memcpy(buf, model_path, dir_len);
    memcpy(buf + dir_len, uri, uri_len + 1);

    const char *segs[MODEL_MAX_SEGMENTS];
    size_t lens[MODEL_MAX_SEGMENTS];
    int n = 0;
    const char *p = buf;
    while (*p) {
        const char *end = strchr(p, '/');
        size_t len = end ? (size_t)(end - p) : strlen(p);
        if (len == 2 && p[0] == '.' && p[1] == '.') {
            if (n == 0) return false; /* would leave the virtual root */
            n--;
        } else if (!(len == 0 || (len == 1 && p[0] == '.'))) {
            if (n == MODEL_MAX_SEGMENTS) return false;
            segs[n] = p;
            lens[n] = len;
            n++;
        }
        p += len;
        if (*p == '/') p++;
    }
    size_t at = 0;
    for (int i = 0; i < n; i++) {
        size_t need = lens[i] + (i ? 1u : 0u);
        if (at + need >= out_size) return false;
        if (i) out[at++] = '/';
        memcpy(out + at, segs[i], lens[i]);
        at += lens[i];
    }
    out[at] = '\0';
    return n > 0 && prts_vfs_path_valid(out);
}

static bool has_suffix(const char *s, const char *suffix)
{
    size_t a = strlen(s), b = strlen(suffix);
    if (a < b) return false;
    for (size_t i = 0; i < b; i++) {
        char c = s[a - b + i];
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if (c != suffix[i]) return false;
    }
    return true;
}

static uint32_t hash_path(const char *s)
{
    uint32_t h = 2166136261u;
    for (; *s; s++) h = (h ^ (uint8_t)*s) * 16777619u;
    return h;
}

/* A material id unique to this model file: "<file name>#<hash>:<tag>". */
static void material_id(const char *path, const char *tag, char *out, size_t size)
{
    const char *slash = strrchr(path, '/');
    const char *base = slash ? slash + 1 : path;
    snprintf(out, size, "%.20s#%08x:%.12s", base, (unsigned)hash_path(path), tag);
}

/* --- building ------------------------------------------------------------------------- */

static built_mesh *build_mesh(model_build *b, int material, bool flip)
{
    for (size_t i = 0; i < b->meshes.len; i++) {
        built_mesh *m = prts_array_at(&b->meshes, i);
        if (m->material == material && m->flip == flip) return m;
    }
    built_mesh *m = prts_array_push(&b->meshes, NULL);
    m->material = material;
    m->flip = flip;
    prts_mesh_init(&m->mesh);
    return m;
}

static void build_free(model_build *b)
{
    for (size_t i = 0; i < b->meshes.len; i++) prts_mesh_free(&((built_mesh *)prts_array_at(&b->meshes, i))->mesh);
    prts_array_free(&b->meshes);
    prts_array_free(&b->markers);
}

static void push_vertex(prts_mesh_cpu *m, prts_vec3 p, prts_vec3 n, float u, float v, prts_vec3 c)
{
    prts_vertex vx = {p.x, p.y, p.z, n.x, n.y, n.z, u, v, c.x, c.y, c.z, 1.0f, 1.0f};
    prts_array_push(&m->verts, &vx);
}

static void push_index(prts_mesh_cpu *m, uint32_t i) { prts_array_push(&m->indices, &i); }

/* --- glTF ----------------------------------------------------------------------------- */

static cgltf_result gltf_read(const struct cgltf_memory_options *mem, const struct cgltf_file_options *file,
                              const char *path, cgltf_size *size, void **data)
{
    (void)mem;
    model_build *b = file->user_data;
    char resolved[PRTS_SCENE_PATH_MAX];
    /* cgltf joined the URI to the model path already; resolve "." and ".." */
    if (!prts_model_resolve_uri("", path, resolved, sizeof(resolved))) return cgltf_result_file_not_found;
    char *bytes = NULL;
    size_t n = 0;
    if (prts_vfs_read(b->vfs, resolved, &bytes, &n) != PRTS_VFS_OK) {
        prts_log_error("model '%s': buffer '%s' is missing", b->path, resolved);
        return cgltf_result_file_not_found;
    }
    *data = bytes;
    *size = n;
    return cgltf_result_success;
}

static void gltf_release(const struct cgltf_memory_options *mem, const struct cgltf_file_options *file, void *data,
                         cgltf_size size)
{
    (void)mem;
    (void)file;
    (void)size;
    prts_free(data);
}

static void *gltf_alloc(void *user, cgltf_size size)
{
    (void)user;
    return prts_malloc(size ? size : 1);
}

static void gltf_free(void *user, void *ptr)
{
    (void)user;
    prts_free(ptr);
}

/* The texture path of a glTF image: a file next to the model, or an embedded image. */
static bool gltf_image_texture(model_build *b, const cgltf_data *data, const cgltf_image *img, char *out,
                               size_t size)
{
    if (img->uri && strncmp(img->uri, "data:", 5) != 0) {
        char uri[PRTS_SCENE_PATH_MAX];
        snprintf(uri, sizeof(uri), "%s", img->uri);
        cgltf_decode_uri(uri);
        if (prts_model_resolve_uri(b->path, uri, out, size)) return true;
        prts_log_warn("model '%s': image uri '%s' is not a valid data path", b->path, img->uri);
        return false;
    }
    if (img->buffer_view && img->buffer_view->buffer->data) {
        const cgltf_buffer_view *bv = img->buffer_view;
        snprintf(out, size, "%s#image%u", b->path, (unsigned)cgltf_image_index(data, img));
        prts_scene_add_image(b->scene, out, (const char *)bv->buffer->data + bv->offset, bv->size);
        return true;
    }
    prts_log_warn("model '%s': image %u has no usable data", b->path, (unsigned)cgltf_image_index(data, img));
    return false;
}

static int gltf_material(model_build *b, const cgltf_data *data, const cgltf_material *gm)
{
    prts_material m;
    memset(&m, 0, sizeof(m));
    m.tint = prts_v3(1, 1, 1);
    m.uv_scale = 1.0f;
    char tag[16];
    if (gm) {
        snprintf(tag, sizeof(tag), "m%u", (unsigned)cgltf_material_index(data, gm));
        if (gm->has_pbr_metallic_roughness) {
            const cgltf_pbr_metallic_roughness *pbr = &gm->pbr_metallic_roughness;
            m.tint = prts_v3(pbr->base_color_factor[0], pbr->base_color_factor[1], pbr->base_color_factor[2]);
            const cgltf_texture *tex = pbr->base_color_texture.texture;
            if (tex && tex->image && !gltf_image_texture(b, data, tex->image, m.texture, sizeof(m.texture))) {
                m.texture[0] = '\0';
            }
        }
        m.emissive = gm->unlit;
    } else {
        snprintf(tag, sizeof(tag), "default");
    }
    material_id(b->path, tag, m.id, sizeof(m.id));
    return prts_scene_material(b->scene, &m);
}

static prts_vec3 xform_point(const float *w, prts_vec3 p)
{
    return prts_v3(w[0] * p.x + w[4] * p.y + w[8] * p.z + w[12], w[1] * p.x + w[5] * p.y + w[9] * p.z + w[13],
                   w[2] * p.x + w[6] * p.y + w[10] * p.z + w[14]);
}

/* Normals use the cofactor matrix (inverse transpose up to scale) of the upper 3x3. */
static prts_vec3 xform_normal(const float *w, prts_vec3 n)
{
    prts_vec3 c0 = prts_v3(w[0], w[1], w[2]), c1 = prts_v3(w[4], w[5], w[6]), c2 = prts_v3(w[8], w[9], w[10]);
    prts_vec3 r0 = prts_v3_cross(c1, c2), r1 = prts_v3_cross(c2, c0), r2 = prts_v3_cross(c0, c1);
    prts_vec3 out = prts_v3_add(prts_v3_add(prts_v3_scale(r0, n.x), prts_v3_scale(r1, n.y)), prts_v3_scale(r2, n.z));
    float len = prts_v3_length(out);
    return len > 1e-12f ? prts_v3_scale(out, 1.0f / len) : prts_v3(0, 1, 0);
}

static float det3(const float *w)
{
    prts_vec3 c0 = prts_v3(w[0], w[1], w[2]), c1 = prts_v3(w[4], w[5], w[6]), c2 = prts_v3(w[8], w[9], w[10]);
    return prts_v3_dot(c0, prts_v3_cross(c1, c2));
}

static const cgltf_accessor *gltf_attribute(const cgltf_primitive *prim, cgltf_attribute_type type)
{
    for (cgltf_size i = 0; i < prim->attributes_count; i++) {
        if (prim->attributes[i].type == type && prim->attributes[i].index == 0) return prim->attributes[i].data;
    }
    return NULL;
}

static bool gltf_primitive(model_build *b, const cgltf_data *data, const cgltf_primitive *prim, const float *w)
{
    if (prim->type != cgltf_primitive_type_triangles) {
        prts_log_warn("model '%s': skipping a primitive that is not a triangle list", b->path);
        return true;
    }
    const cgltf_accessor *pos = gltf_attribute(prim, cgltf_attribute_type_position);
    if (!pos) {
        prts_log_warn("model '%s': skipping a primitive without positions", b->path);
        return true;
    }
    const cgltf_accessor *nrm = gltf_attribute(prim, cgltf_attribute_type_normal);
    const cgltf_accessor *uv = gltf_attribute(prim, cgltf_attribute_type_texcoord);
    const cgltf_accessor *col = gltf_attribute(prim, cgltf_attribute_type_color);
    size_t count = pos->count;
    if (b->vertex_count + count > MODEL_MAX_VERTICES) {
        prts_log_error("model '%s' has too many vertices", b->path);
        return false;
    }
    b->vertex_count += count;
    bool flip = det3(w) < 0.0f;
    int material = gltf_material(b, data, prim->material);
    /* like every scene mesh, the material tint (base color factor) is baked into the vertices */
    prts_vec3 tint = ((const prts_material *)prts_array_at(&b->scene->materials, (size_t)material))->tint;
    built_mesh *bm = build_mesh(b, material, flip);
    prts_mesh_cpu *m = &bm->mesh;
    uint32_t base = (uint32_t)m->verts.len;

    for (size_t i = 0; i < count; i++) {
        float p[3] = {0, 0, 0}, n[3] = {0, 1, 0}, t[2] = {0, 0}, c[4] = {1, 1, 1, 1};
        cgltf_accessor_read_float(pos, i, p, 3);
        if (nrm) cgltf_accessor_read_float(nrm, i, n, 3);
        if (uv) cgltf_accessor_read_float(uv, i, t, 2);
        if (col) cgltf_accessor_read_float(col, i, c, cgltf_num_components(col->type));
        push_vertex(m, xform_point(w, prts_v3(p[0], p[1], p[2])), xform_normal(w, prts_v3(n[0], n[1], n[2])), t[0],
                    t[1], prts_v3(c[0] * tint.x, c[1] * tint.y, c[2] * tint.z));
    }
    size_t first_index = m->indices.len;
    size_t icount = prim->indices ? prim->indices->count : count;
    for (size_t i = 0; i + 2 < icount; i += 3) {
        for (int k = 0; k < 3; k++) {
            size_t idx = prim->indices ? cgltf_accessor_read_index(prim->indices, i + (size_t)k) : i + (size_t)k;
            if (idx >= count) idx = 0; /* cgltf_validate rejects this; stay safe anyway */
            push_index(m, base + (uint32_t)idx);
        }
    }
    if (!nrm) { /* smooth normals from the triangles (area weighted) */
        prts_vertex *v = (prts_vertex *)m->verts.data;
        const uint32_t *ix = (const uint32_t *)m->indices.data;
        for (size_t i = base; i < m->verts.len; i++) v[i].nx = v[i].ny = v[i].nz = 0.0f;
        for (size_t i = first_index; i + 2 < m->indices.len; i += 3) {
            prts_vertex *a = &v[ix[i]], *bb = &v[ix[i + 1]], *cc = &v[ix[i + 2]];
            prts_vec3 e1 = prts_v3(bb->px - a->px, bb->py - a->py, bb->pz - a->pz);
            prts_vec3 e2 = prts_v3(cc->px - a->px, cc->py - a->py, cc->pz - a->pz);
            prts_vec3 fn = prts_v3_cross(e1, e2);
            if (flip) fn = prts_v3_scale(fn, -1.0f);
            prts_vertex *tri[3] = {a, bb, cc};
            for (int k = 0; k < 3; k++) {
                tri[k]->nx += fn.x;
                tri[k]->ny += fn.y;
                tri[k]->nz += fn.z;
            }
        }
        for (size_t i = base; i < m->verts.len; i++) {
            prts_vec3 n = prts_v3(v[i].nx, v[i].ny, v[i].nz);
            float len = prts_v3_length(n);
            n = len > 1e-12f ? prts_v3_scale(n, 1.0f / len) : prts_v3(0, 1, 0);
            v[i].nx = n.x;
            v[i].ny = n.y;
            v[i].nz = n.z;
        }
    }
    return true;
}

static bool node_in_scene(const cgltf_node *node, const cgltf_scene *scene)
{
    if (!scene) return true;
    while (node->parent) node = node->parent;
    for (cgltf_size i = 0; i < scene->nodes_count; i++) {
        if (scene->nodes[i] == node) return true;
    }
    return false;
}

#define MARKER_PREFIX "marker_"

static bool load_gltf(model_build *b, const char *bytes, size_t size)
{
    cgltf_options opt;
    memset(&opt, 0, sizeof(opt));
    opt.memory.alloc_func = gltf_alloc;
    opt.memory.free_func = gltf_free;
    opt.file.read = gltf_read;
    opt.file.release = gltf_release;
    opt.file.user_data = b;
    cgltf_data *data = NULL;
    cgltf_result r = cgltf_parse(&opt, bytes, size, &data);
    if (r == cgltf_result_success) r = cgltf_load_buffers(&opt, data, b->path);
    if (r == cgltf_result_success) r = cgltf_validate(data);
    if (r != cgltf_result_success) {
        prts_log_error("model '%s' is not a valid glTF 2.0 file (cgltf error %d)", b->path, (int)r);
        cgltf_free(data);
        return false;
    }
    const cgltf_scene *scene = data->scene ? data->scene : (data->scenes_count ? &data->scenes[0] : NULL);
    bool ok = true;
    for (cgltf_size i = 0; i < data->nodes_count && ok; i++) {
        const cgltf_node *node = &data->nodes[i];
        if (!node_in_scene(node, scene)) continue;
        float w[16];
        cgltf_node_transform_world(node, w);
        if (node->name && strncmp(node->name, MARKER_PREFIX, strlen(MARKER_PREFIX)) == 0) {
            built_marker *mk = prts_array_push(&b->markers, NULL);
            snprintf(mk->name, sizeof(mk->name), "%s", node->name + strlen(MARKER_PREFIX));
            mk->pos = xform_point(w, prts_v3(0, 0, 0));
            prts_vec3 fwd = prts_v3(-w[8], -w[9], -w[10]); /* the node looks down its -Z axis */
            mk->yaw = atan2f(-fwd.x, -fwd.z);
        }
        if (!node->mesh) continue;
        for (cgltf_size p = 0; p < node->mesh->primitives_count && ok; p++) {
            ok = gltf_primitive(b, data, &node->mesh->primitives[p], w);
        }
    }
    cgltf_free(data);
    return ok;
}

/* --- MagicaVoxel ---------------------------------------------------------------------- */

static int vox_material(model_build *b)
{
    prts_material m;
    memset(&m, 0, sizeof(m));
    m.tint = prts_v3(1, 1, 1);
    m.uv_scale = 1.0f;
    material_id(b->path, "vox", m.id, sizeof(m.id));
    return prts_scene_material(b->scene, &m);
}

static uint8_t vox_at(const prts_vox_model *m, int x, int y, int z)
{
    if (x < 0 || y < 0 || z < 0 || x >= m->size[0] || y >= m->size[1] || z >= m->size[2]) return 0;
    return m->voxels[(size_t)x + (size_t)y * (size_t)m->size[0] + (size_t)z * (size_t)m->size[0] * (size_t)m->size[1]];
}

/* MagicaVoxel is +Z up; the engine is +Y up (a proper rotation, so windings survive). */
static prts_vec3 vox_dir_to_engine(const prts_vox_instance *t, prts_vec3 d)
{
    const float *r = t->rot;
    prts_vec3 q = prts_v3(r[0] * d.x + r[1] * d.y + r[2] * d.z, r[3] * d.x + r[4] * d.y + r[5] * d.z,
                          r[6] * d.x + r[7] * d.y + r[8] * d.z);
    return prts_v3(q.x, q.z, -q.y);
}

static prts_vec3 vox_to_engine(const prts_vox_instance *t, prts_vec3 p)
{
    prts_vec3 q = vox_dir_to_engine(t, p);
    return prts_v3_add(q, prts_v3(t->trans[0], t->trans[2], -t->trans[1]));
}

/* Adds one quad with counter-clockwise winding as seen from the side its normal faces. */
static void emit_quad(prts_mesh_cpu *m, prts_vec3 c[4], prts_vec3 n, prts_vec3 color)
{
    prts_vec3 e1 = prts_v3_sub(c[1], c[0]), e2 = prts_v3_sub(c[2], c[0]);
    bool ccw = prts_v3_dot(prts_v3_cross(e1, e2), n) > 0.0f;
    uint32_t base = (uint32_t)m->verts.len;
    for (int i = 0; i < 4; i++) push_vertex(m, c[i], n, 0.0f, 0.0f, color);
    static const uint32_t fwd[6] = {0, 1, 2, 0, 2, 3}, rev[6] = {0, 2, 1, 0, 3, 2};
    for (int i = 0; i < 6; i++) push_index(m, base + (ccw ? fwd[i] : rev[i]));
}

/* Greedy meshing: per axis and side, merge visible faces of one color into rectangles. */
static void vox_mesh_model(prts_mesh_cpu *out, const prts_vox *vox, const prts_vox_model *vm,
                           const prts_vox_instance *t)
{
    const int *size = vm->size;
    float pivot[3] = {floorf((float)size[0] * 0.5f), floorf((float)size[1] * 0.5f), floorf((float)size[2] * 0.5f)};
    int max_area = 0;
    for (int d = 0; d < 3; d++) {
        int a = size[(d + 1) % 3] * size[(d + 2) % 3];
        if (a > max_area) max_area = a;
    }
    uint8_t *mask = prts_calloc((size_t)max_area, 1);
    for (int d = 0; d < 3; d++) {
        int u = (d + 1) % 3, v = (d + 2) % 3;
        for (int side = 0; side < 2; side++) {
            int step = side ? 1 : -1;
            float nd[3] = {0, 0, 0};
            nd[d] = (float)step;
            prts_vec3 normal = prts_v3_normalize(vox_dir_to_engine(t, prts_v3(nd[0], nd[1], nd[2])));
            for (int slice = 0; slice < size[d]; slice++) {
                for (int j = 0; j < size[v]; j++) {
                    for (int i = 0; i < size[u]; i++) {
                        int p[3], q[3];
                        p[d] = slice;
                        p[u] = i;
                        p[v] = j;
                        q[d] = slice + step;
                        q[u] = i;
                        q[v] = j;
                        uint8_t c = vox_at(vm, p[0], p[1], p[2]);
                        mask[j * size[u] + i] = (c && !vox_at(vm, q[0], q[1], q[2])) ? c : 0;
                    }
                }
                for (int j = 0; j < size[v]; j++) {
                    for (int i = 0; i < size[u];) {
                        uint8_t c = mask[j * size[u] + i];
                        if (!c) {
                            i++;
                            continue;
                        }
                        int w = 1, h = 1;
                        while (i + w < size[u] && mask[j * size[u] + i + w] == c) w++;
                        bool grow = true;
                        while (grow && j + h < size[v]) {
                            for (int k = 0; k < w && grow; k++) grow = mask[(j + h) * size[u] + i + k] == c;
                            if (grow) h++;
                        }
                        for (int jj = 0; jj < h; jj++) memset(&mask[(j + jj) * size[u] + i], 0, (size_t)w);
                        float plane = (float)(slice + (side ? 1 : 0));
                        prts_vec3 corners[4];
                        static const int du[4] = {0, 1, 1, 0}, dv[4] = {0, 0, 1, 1};
                        for (int k = 0; k < 4; k++) {
                            float f[3];
                            f[d] = plane - pivot[d];
                            f[u] = (float)(i + du[k] * w) - pivot[u];
                            f[v] = (float)(j + dv[k] * h) - pivot[v];
                            corners[k] = vox_to_engine(t, prts_v3(f[0], f[1], f[2]));
                        }
                        const uint8_t *rgba = vox->palette[c];
                        emit_quad(out, corners, normal, prts_v3(rgba[0] / 255.0f, rgba[1] / 255.0f, rgba[2] / 255.0f));
                        i += w;
                    }
                }
            }
        }
    }
    prts_free(mask);
}

static bool load_vox(model_build *b, const char *bytes, size_t size)
{
    prts_vox vox;
    char err[96];
    if (!prts_vox_parse(bytes, size, &vox, err, sizeof(err))) {
        prts_log_error("model '%s' is not a valid MagicaVoxel file: %s", b->path, err);
        return false;
    }
    if (!vox.has_palette) prts_log_warn("model '%s' has no palette; using grey", b->path);
    built_mesh *bm = build_mesh(b, vox_material(b), false);
    for (size_t i = 0; i < vox.instances.len; i++) {
        const prts_vox_instance *inst = prts_array_at(&vox.instances, i);
        vox_mesh_model(&bm->mesh, &vox, prts_array_at(&vox.models, (size_t)inst->model), inst);
    }
    prts_vox_free(&vox);
    /* centre on x/z with the lowest voxel at y = 0 */
    prts_vertex *v = (prts_vertex *)bm->mesh.verts.data;
    if (bm->mesh.verts.len == 0) return true;
    prts_vec3 lo = prts_v3(v[0].px, v[0].py, v[0].pz), hi = lo;
    for (size_t i = 0; i < bm->mesh.verts.len; i++) {
        lo = prts_v3(fminf(lo.x, v[i].px), fminf(lo.y, v[i].py), fminf(lo.z, v[i].pz));
        hi = prts_v3(fmaxf(hi.x, v[i].px), fmaxf(hi.y, v[i].py), fmaxf(hi.z, v[i].pz));
    }
    prts_vec3 shift = prts_v3(-(lo.x + hi.x) * 0.5f, -lo.y, -(lo.z + hi.z) * 0.5f);
    for (size_t i = 0; i < bm->mesh.verts.len; i++) {
        v[i].px += shift.x;
        v[i].py += shift.y;
        v[i].pz += shift.z;
    }
    return true;
}

/* --- placement ------------------------------------------------------------------------ */

static prts_vec3 place_point(const prts_model_place *pl, prts_vec3 p)
{
    return prts_v3_add(prts_v3_rotate_y(prts_v3_scale(p, pl->scale), pl->yaw), pl->pos);
}

static void place(model_build *b, const prts_model_place *pl, const char *marker_prefix, prts_aabb *bounds)
{
    bool any = false;
    prts_vec3 lo = pl->pos, hi = pl->pos;
    for (size_t i = 0; i < b->meshes.len; i++) {
        const built_mesh *bm = prts_array_at(&b->meshes, i);
        prts_mesh_cpu *dst = &prts_scene_group(b->scene, bm->material, pl->owner)->mesh;
        uint32_t base = (uint32_t)dst->verts.len;
        for (size_t k = 0; k < bm->mesh.verts.len; k++) {
            prts_vertex vx = *(const prts_vertex *)prts_array_at(&bm->mesh.verts, k);
            prts_vec3 p = place_point(pl, prts_v3(vx.px, vx.py, vx.pz));
            prts_vec3 n = prts_v3_rotate_y(prts_v3(vx.nx, vx.ny, vx.nz), pl->yaw);
            vx.px = p.x;
            vx.py = p.y;
            vx.pz = p.z;
            vx.nx = n.x;
            vx.ny = n.y;
            vx.nz = n.z;
            prts_array_push(&dst->verts, &vx);
            if (!any) lo = hi = p;
            any = true;
            lo = prts_v3(fminf(lo.x, p.x), fminf(lo.y, p.y), fminf(lo.z, p.z));
            hi = prts_v3(fmaxf(hi.x, p.x), fmaxf(hi.y, p.y), fmaxf(hi.z, p.z));
        }
        const uint32_t *ix = (const uint32_t *)bm->mesh.indices.data;
        for (size_t k = 0; k + 2 < bm->mesh.indices.len; k += 3) {
            push_index(dst, base + ix[k]);
            push_index(dst, base + ix[k + (bm->flip ? 2 : 1)]);
            push_index(dst, base + ix[k + (bm->flip ? 1 : 2)]);
        }
    }
    for (size_t i = 0; i < b->markers.len; i++) {
        const built_marker *mk = prts_array_at(&b->markers, i);
        char name[PRTS_SCENE_NAME_MAX];
        if (marker_prefix && *marker_prefix) snprintf(name, sizeof(name), "%.23s.%.23s", marker_prefix, mk->name);
        else snprintf(name, sizeof(name), "%s", mk->name);
        prts_vec3 p = place_point(pl, mk->pos);
        prts_scene_add_marker(b->scene, name, prts_dv3(p.x, p.y, p.z), mk->yaw + pl->yaw);
    }
    if (bounds) *bounds = prts_aabb_make(lo, hi);
    b->scene->revision++;
}

static void add_placeholder(prts_scene *s, const prts_model_place *pl, prts_aabb *bounds)
{
    prts_material m;
    memset(&m, 0, sizeof(m));
    snprintf(m.id, sizeof(m.id), "missing-model");
    m.tint = prts_v3(1.0f, 0.0f, 1.0f);
    m.uv_scale = 1.0f;
    m.emissive = true; /* glowing magenta: impossible to miss */
    int mat = prts_scene_material(s, &m);
    prts_aabb box = prts_aabb_make(prts_v3(pl->pos.x - 0.5f, pl->pos.y, pl->pos.z - 0.5f),
                                   prts_v3(pl->pos.x + 0.5f, pl->pos.y + 1.0f, pl->pos.z + 0.5f));
    prts_scene_add_box(s, box, mat, pl->owner, 1.0f);
    if (bounds) *bounds = box;
}

bool prts_model_add(prts_scene *scene, prts_vfs *vfs, const char *vpath, const prts_model_place *place_in,
                    const char *marker_prefix, prts_aabb *bounds)
{
    prts_model_place pl = *place_in;
    bool vox = has_suffix(vpath, ".vox");
    if (pl.scale <= 0.0f) pl.scale = vox ? 0.1f : 1.0f;
    model_build b;
    memset(&b, 0, sizeof(b));
    b.scene = scene;
    b.vfs = vfs;
    b.path = vpath;
    prts_array_init(&b.meshes, sizeof(built_mesh));
    prts_array_init(&b.markers, sizeof(built_marker));

    bool ok = false;
    if (!vox && !has_suffix(vpath, ".glb") && !has_suffix(vpath, ".gltf")) {
        prts_log_error("model '%s': unknown model format (use .glb, .gltf or .vox)", vpath);
    } else {
        char *bytes = NULL;
        size_t size = 0;
        prts_vfs_result r = prts_vfs_read(vfs, vpath, &bytes, &size);
        if (r != PRTS_VFS_OK) {
            prts_log_error("model '%s' is missing or unreadable (%s); showing a placeholder", vpath,
                           prts_vfs_result_name(r));
        } else {
            ok = vox ? load_vox(&b, bytes, size) : load_gltf(&b, bytes, size);
        }
        prts_free(bytes);
    }
    if (ok) place(&b, &pl, marker_prefix, bounds);
    else add_placeholder(scene, &pl, bounds);
    build_free(&b);
    return ok;
}
