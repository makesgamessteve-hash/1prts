/* 1stPRTS engine: OpenGL function loading. MIT License. */
#include <string.h>

#include "gfx/gl_api.h"

const char *prts_gl_load(prts_gl *gl, prts_gl_getproc getproc)
{
    memset(gl, 0, sizeof(*gl));
#define PRTS_GL_LOAD(ret, name, params)                       \
    {                                                         \
        void *p = getproc(#name);                             \
        if (!p) return #name;                                 \
        memcpy(&gl->name, &p, sizeof(p));                     \
    }
    PRTS_GL_FUNCTIONS(PRTS_GL_LOAD)
#undef PRTS_GL_LOAD
    return NULL;
}
