/* Unit tests: glTF and MagicaVoxel model loading. MIT License. */
#include <math.h>
#include <string.h>

#include "prts/prts_log.h"
#include "prts/prts_model.h"
#include "scene/vox.h"
#include "utest.h"

#define FIX PRTS_TEST_FIXTURES "/models"

static char g_log[2048];
static int g_errors, g_warnings;

static void sink(prts_log_level level, const char *message, void *user)
{
    (void)user;
    if (level == PRTS_LOG_ERROR) g_errors++;
    if (level == PRTS_LOG_WARN) g_warnings++;
    size_t at = strlen(g_log);
    if (at + strlen(message) + 2 < sizeof(g_log)) {
        strcat(g_log, message);
        strcat(g_log, "\n");
    }
}

static void capture(void)
{
    g_log[0] = '\0';
    g_errors = g_warnings = 0;
    prts_log_set_sink(sink, NULL);
}

static prts_vfs *fixtures(void)
{
    prts_vfs *v = prts_vfs_create();
    prts_vfs_mount(v, FIX, "models");
    return v;
}

static prts_model_place at(float x, float y, float z)
{
    prts_model_place p;
    memset(&p, 0, sizeof(p));
    p.pos = prts_v3(x, y, z);
    return p;
}

static size_t total_verts(const prts_scene *s)
{
    size_t n = 0;
    for (size_t i = 0; i < s->groups.len; i++) n += ((prts_mesh_group *)prts_array_at(&s->groups, i))->mesh.verts.len;
    return n;
}

static const prts_material *material_with(const prts_scene *s, const char *needle)
{
    for (size_t i = 0; i < s->materials.len; i++) {
        const prts_material *m = prts_array_at(&s->materials, i);
        if (strstr(m->id, needle)) return m;
    }
    return NULL;
}

UTEST(model, resolve_uri)
{
    char out[64];
    ASSERT_TRUE(prts_model_resolve_uri("models/a.glb", "../textures/x.png", out, sizeof(out)));
    ASSERT_STREQ("textures/x.png", out);
    ASSERT_TRUE(prts_model_resolve_uri("models/a.glb", "./b/./c.png", out, sizeof(out)));
    ASSERT_STREQ("models/b/c.png", out);
    ASSERT_TRUE(prts_model_resolve_uri("a.glb", "b//c.png", out, sizeof(out)));
    ASSERT_STREQ("b/c.png", out);
    ASSERT_FALSE(prts_model_resolve_uri("models/a.glb", "../../x.png", out, sizeof(out)));
    ASSERT_FALSE(prts_model_resolve_uri("models/a.glb", "", out, sizeof(out)));
    ASSERT_FALSE(prts_model_resolve_uri("", "C:/x.png", out, sizeof(out)));
    ASSERT_FALSE(prts_model_resolve_uri("models/a.glb", "textures/long_name.png", out, 8));
    ASSERT_FALSE(prts_model_resolve_uri("a.glb", "x.png", out, 0));
    char many[200] = "";
    for (int i = 0; i < 70; i++) strcat(many, "a/");
    strcat(many, "x");
    ASSERT_FALSE(prts_model_resolve_uri("", many, out, sizeof(out)));
    char huge[600];
    memset(huge, 'a', sizeof(huge) - 1);
    huge[sizeof(huge) - 1] = '\0';
    ASSERT_FALSE(prts_model_resolve_uri("", huge, out, sizeof(out)));
}

UTEST(model, gltf_nodes_colors_normals_and_markers)
{
    capture();
    prts_vfs *v = fixtures();
    prts_scene *s = prts_scene_create();
    prts_model_place pl = at(10, 0, 0);
    pl.yaw = (float)PRTS_PI / 2;
    pl.scale = 2.0f;
    prts_aabb b;
    ASSERT_TRUE(prts_model_add(s, v, "tri.gltf", &pl, "thing", &b));
    /* the quad twice (one mirrored); the line list and the primitive without positions are
     * skipped (with a warning) in both nodes */
    ASSERT_EQ((size_t)8, total_verts(s));
    ASSERT_EQ(4, g_warnings);
    const prts_material *red = material_with(s, "tri.gltf#");
    ASSERT_TRUE(red != NULL);
    ASSERT_NEAR(1.0f, red->tint.x, 1e-6f);
    ASSERT_NEAR(0.0f, red->tint.y, 1e-6f);
    const prts_mesh_group *g = prts_array_at(&s->groups, 0);
    ASSERT_EQ((size_t)12, g->mesh.indices.len);
    for (size_t i = 0; i < g->mesh.verts.len; i++) {
        const prts_vertex *vx = prts_array_at(&g->mesh.verts, i);
        /* computed normals face +Z in model space; the placement yaw turns them to +X */
        ASSERT_NEAR(1.0f, vx->nx, 1e-5f);
        ASSERT_NEAR(0.5f, vx->r, 1e-6f);
        ASSERT_NEAR(0.0f, vx->g, 1e-6f); /* vertex color (0.5, 1, 1) times the red base color */
    }
    /* both copies wind counter-clockwise towards their normal */
    const uint32_t *ix = (const uint32_t *)g->mesh.indices.data;
    for (size_t t = 0; t < 4; t++) {
        const prts_vertex *a = prts_array_at(&g->mesh.verts, ix[t * 3]);
        const prts_vertex *bb = prts_array_at(&g->mesh.verts, ix[t * 3 + 1]);
        const prts_vertex *c = prts_array_at(&g->mesh.verts, ix[t * 3 + 2]);
        prts_vec3 n = prts_v3_cross(prts_v3(bb->px - a->px, bb->py - a->py, bb->pz - a->pz),
                                    prts_v3(c->px - a->px, c->py - a->py, c->pz - a->pz));
        ASSERT_GT(n.x, 0.0f);
    }
    const prts_marker *mk = prts_scene_find_marker(s, "thing.tip");
    ASSERT_TRUE(mk != NULL);
    ASSERT_NEAR(10.0, mk->pos.x, 1e-5);
    ASSERT_NEAR(2.0, mk->pos.y, 1e-5);
    ASSERT_NEAR(-2.0, mk->pos.z, 1e-5);
    ASSERT_NEAR((float)PRTS_PI, mk->yaw, 1e-5f);
    ASSERT_NEAR(2.0f, b.max.y, 1e-5f);
    ASSERT_NEAR(0.0f, b.min.y, 1e-5f);
    /* without a prefix the marker keeps its own name */
    prts_scene_clear(s);
    pl = at(0, 0, 0);
    ASSERT_TRUE(prts_model_add(s, v, "tri.gltf", &pl, NULL, NULL));
    ASSERT_TRUE(prts_scene_find_marker(s, "tip") != NULL);
    prts_scene_destroy(s);
    prts_vfs_destroy(v);
    prts_log_set_sink(NULL, NULL);
}

UTEST(model, gltf_external_buffer_textures_and_unlit)
{
    capture();
    prts_vfs *v = fixtures();
    prts_scene *s = prts_scene_create();
    prts_model_place pl = at(0, 0, 0);
    ASSERT_TRUE(prts_model_add(s, v, "ext.gltf", &pl, NULL, NULL));
    const prts_material *good = material_with(s, ":m0"), *bad = material_with(s, ":m1"), *glow = material_with(s, ":m2");
    ASSERT_TRUE(good && bad && glow);
    ASSERT_STREQ("tex/checker.png", good->texture);
    ASSERT_STREQ("", bad->texture); /* would leave the data folder */
    ASSERT_TRUE(strstr(g_log, "../../../outside.png") != NULL);
    ASSERT_TRUE(glow->emissive);
    ASSERT_FALSE(good->emissive);
    ASSERT_TRUE(material_with(s, ":default") != NULL);
    ASSERT_EQ((size_t)16, total_verts(s));
    ASSERT_EQ(0, g_errors);

    /* a model whose buffer file is missing shows the placeholder */
    prts_scene_clear(s);
    prts_aabb b;
    ASSERT_FALSE(prts_model_add(s, v, "missing_bin.gltf", &pl, NULL, &b));
    ASSERT_TRUE(strstr(g_log, "nowhere.bin") != NULL);
    ASSERT_TRUE(material_with(s, "missing-model") != NULL);
    prts_scene_destroy(s);
    prts_vfs_destroy(v);
    prts_log_set_sink(NULL, NULL);
}

UTEST(model, missing_and_broken_models_show_a_placeholder)
{
    /* Spec: Missing model */
    capture();
    prts_vfs *v = fixtures();
    prts_scene *s = prts_scene_create();
    prts_model_place pl = at(3, 1, -2);
    prts_aabb b;
    ASSERT_FALSE(prts_model_add(s, v, "models/missing.glb", &pl, NULL, &b));
    ASSERT_TRUE(strstr(g_log, "models/missing.glb") != NULL);
    ASSERT_EQ(1, g_errors);
    const prts_material *m = material_with(s, "missing-model");
    ASSERT_TRUE(m != NULL);
    ASSERT_TRUE(m->emissive);
    ASSERT_NEAR(1.0f, m->tint.x, 1e-6f);
    ASSERT_NEAR(0.0f, m->tint.y, 1e-6f);
    ASSERT_NEAR(2.5f, b.min.x, 1e-6f);
    ASSERT_NEAR(1.0f, b.min.y, 1e-6f);
    ASSERT_NEAR(2.0f, b.max.y, 1e-6f);
    ASSERT_GT(total_verts(s), (size_t)0);
    ASSERT_FALSE(prts_model_add(s, v, "broken.glb", &pl, NULL, NULL));
    ASSERT_FALSE(prts_model_add(s, v, "tri.obj", &pl, NULL, NULL));
    ASSERT_FALSE(prts_model_add(s, v, "not_vox.vox", &pl, NULL, NULL));
    ASSERT_FALSE(prts_model_add(s, v, "../escape.glb", &pl, NULL, NULL));
    ASSERT_EQ(5, g_errors);
    prts_scene_destroy(s);
    prts_vfs_destroy(v);
    prts_log_set_sink(NULL, NULL);
}

UTEST(model, glb_props_with_embedded_image_and_owner)
{
    capture();
    prts_vfs *v = fixtures();
    prts_scene *s = prts_scene_create();
    prts_model_place pl = at(0, 0, 0);
    ASSERT_TRUE(prts_model_add(s, v, "props.glb", &pl, "brazier", NULL));
    const prts_marker *light = prts_scene_find_marker(s, "brazier.light");
    ASSERT_TRUE(light != NULL);
    ASSERT_NEAR(1.35, light->pos.y, 1e-5);
    const prts_material *flame = material_with(s, ":m1");
    ASSERT_TRUE(flame && flame->emissive);

    prts_scene_clear(s);
    pl.owner = 7;
    ASSERT_TRUE(prts_model_add(s, v, "banner.glb", &pl, NULL, NULL));
    const prts_scene_image *im = prts_scene_find_image(s, "banner.glb#image1");
    ASSERT_TRUE(im != NULL);
    ASSERT_GT(im->size, (size_t)32);
    ASSERT_EQ(0, memcmp(im->data, "\x89PNG", 4));
    const prts_material *cloth = material_with(s, ":m1");
    ASSERT_STREQ("banner.glb#image1", cloth->texture);
    for (size_t i = 0; i < s->groups.len; i++) ASSERT_EQ((prts_entity)7, ((prts_mesh_group *)prts_array_at(&s->groups, i))->owner);
    /* loading again replaces the stored image instead of adding a second one */
    size_t images = s->images.len;
    ASSERT_TRUE(prts_model_add(s, v, "banner.glb", &pl, NULL, NULL));
    ASSERT_EQ(images, s->images.len);
    ASSERT_TRUE(prts_scene_find_image(s, "nope") == NULL);
    ASSERT_EQ(0, g_errors);
    prts_scene_destroy(s);
    prts_vfs_destroy(v);
    prts_log_set_sink(NULL, NULL);
}

UTEST(model, vox_parser_scene_graph)
{
    char *data = NULL;
    size_t size = 0;
    prts_vfs *v = fixtures();
    ASSERT_EQ(PRTS_VFS_OK, prts_vfs_read(v, "graph.vox", &data, &size));
    prts_vox vox;
    char err[96];
    ASSERT_TRUE(prts_vox_parse(data, size, &vox, err, sizeof(err)));
    ASSERT_TRUE(vox.has_palette);
    ASSERT_EQ((size_t)2, vox.models.len);
    /* hidden node, hidden layer and a missing model are left out */
    ASSERT_EQ((size_t)3, vox.instances.len);
    const prts_vox_instance *cube = prts_array_at(&vox.instances, 0);
    const prts_vox_instance *turned = prts_array_at(&vox.instances, 1);
    const prts_vox_instance *bad_rot = prts_array_at(&vox.instances, 2);
    ASSERT_EQ(0, cube->model);
    ASSERT_NEAR(1.0f, cube->trans[2], 1e-6f);
    ASSERT_EQ(1, turned->model);
    ASSERT_NEAR(4.0f, turned->trans[0], 1e-6f);
    ASSERT_NEAR(-1.0f, turned->rot[1], 1e-6f); /* row 0: -y */
    ASSERT_NEAR(1.0f, turned->rot[3], 1e-6f);  /* row 1: +x */
    ASSERT_NEAR(1.0f, turned->rot[8], 1e-6f);  /* row 2: +z */
    ASSERT_NEAR(1.0f, bad_rot->rot[0], 1e-6f); /* an invalid rotation byte reads as identity */
    ASSERT_NEAR(8.0f, bad_rot->trans[1], 1e-6f);
    ASSERT_EQ(255, vox.palette[1][0]);
    ASSERT_EQ(255, vox.palette[2][1]);
    const prts_vox_model *m = prts_array_at(&vox.models, 0);
    ASSERT_EQ(2, m->size[0]);
    prts_vox_free(&vox);
    prts_free(data);
    prts_vfs_destroy(v);
}

UTEST(model, vox_parser_rejects_bad_files)
{
    static const char *bad[] = {"truncated.vox", "no_models.vox", "bad_size.vox", "xyzi_first.vox", "not_vox.vox"};
    static const char *why[] = {"truncated", "no models", "size", "without SIZE", "not a MagicaVoxel"};
    prts_vfs *v = fixtures();
    for (size_t i = 0; i < sizeof(bad) / sizeof(bad[0]); i++) {
        char *data = NULL;
        size_t size = 0;
        ASSERT_EQ(PRTS_VFS_OK, prts_vfs_read(v, bad[i], &data, &size));
        prts_vox vox;
        char err[96] = "";
        ASSERT_FALSE(prts_vox_parse(data, size, &vox, err, sizeof(err)));
        ASSERT_TRUE(strstr(err, why[i]) != NULL);
        prts_free(data);
    }
    prts_vox vox;
    char err[96];
    ASSERT_FALSE(prts_vox_parse("VOX ", 4, &vox, err, sizeof(err)));
    prts_vfs_destroy(v);
}

UTEST(model, vox_greedy_meshing_and_placement)
{
    /* Spec: Voxel prop */
    capture();
    prts_vfs *v = fixtures();
    prts_scene *s = prts_scene_create();
    prts_model_place pl = at(5, 2, 0);
    prts_aabb b;
    ASSERT_TRUE(prts_model_add(s, v, "solid.vox", &pl, NULL, &b));
    /* one color: every side merges into a single quad */
    ASSERT_EQ((size_t)24, total_verts(s));
    /* 4 x 3 x 2 voxels of 0.1 m; MagicaVoxel z is up, centred on pos, resting on it */
    ASSERT_NEAR(0.4f, b.max.x - b.min.x, 1e-5f);
    ASSERT_NEAR(0.2f, b.max.y - b.min.y, 1e-5f);
    ASSERT_NEAR(0.3f, b.max.z - b.min.z, 1e-5f);
    ASSERT_NEAR(2.0f, b.min.y, 1e-5f);
    ASSERT_NEAR(5.0f, (b.min.x + b.max.x) * 0.5f, 1e-5f);
    const prts_mesh_group *g = prts_array_at(&s->groups, 0);
    for (size_t i = 0; i < g->mesh.verts.len; i++) {
        const prts_vertex *vx = prts_array_at(&g->mesh.verts, i);
        ASSERT_NEAR(1.0f, vx->r, 1e-6f); /* palette color 1 is red */
        ASSERT_NEAR(0.0f, vx->g, 1e-6f);
    }
    /* outward-facing, counter-clockwise faces: each normal points away from the centre */
    const uint32_t *ix = (const uint32_t *)g->mesh.indices.data;
    prts_vec3 centre = prts_v3((b.min.x + b.max.x) * 0.5f, (b.min.y + b.max.y) * 0.5f, (b.min.z + b.max.z) * 0.5f);
    for (size_t t = 0; t + 2 < g->mesh.indices.len; t += 3) {
        const prts_vertex *a = prts_array_at(&g->mesh.verts, ix[t]);
        const prts_vertex *bb = prts_array_at(&g->mesh.verts, ix[t + 1]);
        const prts_vertex *c = prts_array_at(&g->mesh.verts, ix[t + 2]);
        prts_vec3 n = prts_v3_cross(prts_v3(bb->px - a->px, bb->py - a->py, bb->pz - a->pz),
                                    prts_v3(c->px - a->px, c->py - a->py, c->pz - a->pz));
        prts_vec3 out = prts_v3(a->px - centre.x, a->py - centre.y, a->pz - centre.z);
        ASSERT_GT(prts_v3_dot(n, out), 0.0f);
        ASSERT_GT(prts_v3_dot(prts_v3(a->nx, a->ny, a->nz), n), 0.0f);
    }

    /* checkerboard colors cannot merge: 2x2x2 cube = 24 quads, plus 2 single voxels */
    prts_scene_clear(s);
    pl.scale = 1.0f;
    ASSERT_TRUE(prts_model_add(s, v, "graph.vox", &pl, NULL, &b));
    ASSERT_EQ((size_t)(24 + 6 + 6) * 4, total_verts(s));
    ASSERT_NEAR(2.0f, b.min.y, 1e-5f);

    /* no palette and no scene graph: grey, one instance at the origin */
    prts_scene_clear(s);
    g_warnings = 0;
    ASSERT_TRUE(prts_model_add(s, v, "plain.vox", &pl, NULL, &b));
    ASSERT_EQ(1, g_warnings);
    ASSERT_EQ((size_t)12 * 4, total_verts(s));
    g = prts_array_at(&s->groups, 0);
    const prts_vertex *vx = prts_array_at(&g->mesh.verts, 0);
    ASSERT_NEAR(vx->r, vx->g, 1e-6f);
    ASSERT_NEAR(3.0f, b.max.x - b.min.x, 1e-5f);
    ASSERT_EQ(0, g_errors);
    prts_scene_destroy(s);
    prts_vfs_destroy(v);
    prts_log_set_sink(NULL, NULL);
}

UTEST(model, base_game_models_load)
{
    capture();
    prts_vfs *v = prts_vfs_create();
    prts_vfs_mount(v, PRTS_TEST_DATA "/base", "base");
    prts_scene *s = prts_scene_create();
    static const char *files[] = {"models/bench.glb", "models/brazier.glb", "models/banner_stand.glb",
                                  "models/crate.vox", "models/anvil.vox"};
    for (size_t i = 0; i < sizeof(files) / sizeof(files[0]); i++) {
        prts_model_place pl = at(0, 0, 0);
        prts_aabb b;
        ASSERT_TRUE(prts_model_add(s, v, files[i], &pl, "m", &b));
        ASSERT_GT(b.max.y - b.min.y, 0.3f);
        ASSERT_LT(b.max.y - b.min.y, 3.5f);
    }
    ASSERT_EQ(0, g_errors);
    ASSERT_EQ(0, g_warnings);
    prts_scene_destroy(s);
    prts_vfs_destroy(v);
    prts_log_set_sink(NULL, NULL);
}
