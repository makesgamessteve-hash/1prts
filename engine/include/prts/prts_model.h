/* 1stPRTS engine: glTF 2.0 and MagicaVoxel model loading into scene geometry. MIT License. */
#ifndef PRTS_MODEL_H
#define PRTS_MODEL_H

#include "prts_scene.h"
#include "prts_vfs.h"

typedef struct prts_model_place {
    prts_vec3 pos;
    float yaw;
    float scale; /* see prts_model_add */
    prts_entity owner; /* geometry follows this entity (pos/yaw are then in its frame) */
} prts_model_place;

/* Loads a model (.glb/.gltf or .vox) from the VFS and adds its geometry to the scene.
 * glTF: meshes of every node in the default scene with their world transforms, materials
 * (base color factor, base color texture from a file or embedded PNG, KHR_materials_unlit),
 * vertex colors and UVs. Nodes named "marker_<name>" become scene markers
 * "<marker_prefix>.<name>". scale 0 means 1.
 * .vox: every visible instance, greedy-meshed with palette colors, centred on pos with the
 * lowest voxel on it; scale is the size of one voxel (0 means 0.1 m).
 * Returns false (after logging the path) when the file is missing or broken; a glowing
 * magenta 1 m placeholder box is added instead. bounds receives the placed bounding box. */
bool prts_model_add(prts_scene *scene, prts_vfs *vfs, const char *vpath, const prts_model_place *place,
                    const char *marker_prefix, prts_aabb *bounds);

/* Joins a model-relative URI to the model's directory and resolves "." and ".." segments.
 * Returns false if the result would leave the virtual root. */
bool prts_model_resolve_uri(const char *model_path, const char *uri, char *out, size_t out_size);

#endif
