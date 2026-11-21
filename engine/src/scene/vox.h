/* 1stPRTS engine: MagicaVoxel .vox parser (engine-internal). MIT License. */
#ifndef PRTS_SCENE_VOX_H
#define PRTS_SCENE_VOX_H

#include "prts/prts_base.h"

typedef struct prts_vox_model {
    int size[3];     /* voxels along MagicaVoxel x, y, z (z is up) */
    uint8_t *voxels; /* color index per voxel, x + y*sx + z*sx*sy; 0 is empty */
} prts_vox_model;

/* A placed model: world = rot * (local - pivot) + trans, rot row-major. */
typedef struct prts_vox_instance {
    int model;
    float rot[9];
    float trans[3];
} prts_vox_instance;

typedef struct prts_vox {
    prts_array models;    /* prts_vox_model */
    prts_array instances; /* prts_vox_instance: the visible shapes of the scene graph */
    uint8_t palette[256][4]; /* RGBA by color index (index 0 unused) */
    bool has_palette;
} prts_vox;

/* Parses a .vox file: SIZE/XYZI models, the RGBA palette, and the nTRN/nGRP/nSHP scene
 * graph with LAYR visibility (flattened into instances). Without a scene graph every model
 * is one instance at the origin. Returns false and fills err on malformed input. */
bool prts_vox_parse(const void *data, size_t size, prts_vox *out, char *err, size_t err_size);
void prts_vox_free(prts_vox *v);

#endif
