/* 1stPRTS engine: the OpenGL 3.3 core / OpenGL ES 3.0 subset used by the renderer.
 * Declared here (instead of using system GL headers) and loaded at run time through a
 * getproc callback from the platform layer. MIT License. */
#ifndef PRTS_GL_API_H
#define PRTS_GL_API_H

#include <stddef.h>

typedef unsigned int GLenum;
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLboolean;
typedef float GLfloat;
typedef unsigned int GLbitfield;
typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef unsigned char GLubyte;

#if defined(_WIN32)
#define PRTS_GLAPI __stdcall
#else
#define PRTS_GLAPI
#endif

#define GL_DEPTH_BUFFER_BIT 0x0100
#define GL_COLOR_BUFFER_BIT 0x4000
#define GL_LINES 0x0001
#define GL_TRIANGLES 0x0004
#define GL_LEQUAL 0x0203
#define GL_SRC_ALPHA 0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE 1
#define GL_BACK 0x0405
#define GL_CULL_FACE 0x0B44
#define GL_DEPTH_TEST 0x0B71
#define GL_BLEND 0x0BE2
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ALIGNMENT 0x0D05
#define GL_TEXTURE_2D 0x0DE1
#define GL_UNSIGNED_BYTE 0x1401
#define GL_UNSIGNED_INT 0x1405
#define GL_FLOAT 0x1406
#define GL_RGB 0x1907
#define GL_RGBA 0x1908
#define GL_RENDERER 0x1F01
#define GL_VERSION 0x1F02
#define GL_NEAREST 0x2600
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_WRAP_S 0x2802
#define GL_TEXTURE_WRAP_T 0x2803
#define GL_REPEAT 0x2901
#define GL_RGBA8 0x8058
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_TEXTURE0 0x84C0
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT 0x8D00
#define GL_FRAMEBUFFER 0x8D40
#define GL_RENDERBUFFER 0x8D41

/* X-macro list: return type, name, parameters. */
#define PRTS_GL_FUNCTIONS(X)                                                                                    \
    X(const GLubyte *, glGetString, (GLenum name))                                                              \
    X(GLenum, glGetError, (void))                                                                               \
    X(void, glViewport, (GLint x, GLint y, GLsizei w, GLsizei h))                                               \
    X(void, glClearColor, (GLfloat r, GLfloat g, GLfloat b, GLfloat a))                                         \
    X(void, glClear, (GLbitfield mask))                                                                         \
    X(void, glEnable, (GLenum cap))                                                                             \
    X(void, glDisable, (GLenum cap))                                                                            \
    X(void, glDepthFunc, (GLenum func))                                                                         \
    X(void, glDepthMask, (GLboolean flag))                                                                      \
    X(void, glBlendFunc, (GLenum s, GLenum d))                                                                  \
    X(void, glCullFace, (GLenum mode))                                                                          \
    X(void, glGenVertexArrays, (GLsizei n, GLuint * arrays))                                                    \
    X(void, glBindVertexArray, (GLuint array))                                                                  \
    X(void, glDeleteVertexArrays, (GLsizei n, const GLuint *arrays))                                            \
    X(void, glGenBuffers, (GLsizei n, GLuint * buffers))                                                        \
    X(void, glBindBuffer, (GLenum target, GLuint buffer))                                                       \
    X(void, glBufferData, (GLenum target, GLsizeiptr size, const void *data, GLenum usage))                     \
    X(void, glDeleteBuffers, (GLsizei n, const GLuint *buffers))                                                \
    X(void, glVertexAttribPointer, (GLuint i, GLint size, GLenum type, GLboolean norm, GLsizei stride, const void *p)) \
    X(void, glEnableVertexAttribArray, (GLuint index))                                                          \
    X(GLuint, glCreateShader, (GLenum type))                                                                    \
    X(void, glShaderSource, (GLuint s, GLsizei count, const GLchar *const *str, const GLint *len))              \
    X(void, glCompileShader, (GLuint s))                                                                        \
    X(void, glGetShaderiv, (GLuint s, GLenum pname, GLint * params))                                            \
    X(void, glGetShaderInfoLog, (GLuint s, GLsizei max, GLsizei * len, GLchar * log))                           \
    X(void, glDeleteShader, (GLuint s))                                                                         \
    X(GLuint, glCreateProgram, (void))                                                                          \
    X(void, glAttachShader, (GLuint p, GLuint s))                                                               \
    X(void, glBindAttribLocation, (GLuint p, GLuint index, const GLchar *name))                                 \
    X(void, glLinkProgram, (GLuint p))                                                                          \
    X(void, glGetProgramiv, (GLuint p, GLenum pname, GLint * params))                                           \
    X(void, glGetProgramInfoLog, (GLuint p, GLsizei max, GLsizei * len, GLchar * log))                          \
    X(void, glDeleteProgram, (GLuint p))                                                                        \
    X(void, glUseProgram, (GLuint p))                                                                           \
    X(GLint, glGetUniformLocation, (GLuint p, const GLchar *name))                                              \
    X(void, glUniform1i, (GLint loc, GLint v))                                                                  \
    X(void, glUniform1f, (GLint loc, GLfloat v))                                                                \
    X(void, glUniform2f, (GLint loc, GLfloat a, GLfloat b))                                                     \
    X(void, glUniform3f, (GLint loc, GLfloat a, GLfloat b, GLfloat c))                                          \
    X(void, glUniform3fv, (GLint loc, GLsizei count, const GLfloat *v))                                         \
    X(void, glUniform1fv, (GLint loc, GLsizei count, const GLfloat *v))                                         \
    X(void, glUniformMatrix4fv, (GLint loc, GLsizei count, GLboolean transpose, const GLfloat *v))              \
    X(void, glGenTextures, (GLsizei n, GLuint * t))                                                             \
    X(void, glBindTexture, (GLenum target, GLuint t))                                                           \
    X(void, glTexImage2D, (GLenum t, GLint lvl, GLint ifmt, GLsizei w, GLsizei h, GLint b, GLenum f, GLenum ty, const void *p)) \
    X(void, glTexParameteri, (GLenum t, GLenum pname, GLint v))                                                 \
    X(void, glDeleteTextures, (GLsizei n, const GLuint *t))                                                     \
    X(void, glActiveTexture, (GLenum unit))                                                                     \
    X(void, glGenFramebuffers, (GLsizei n, GLuint * f))                                                         \
    X(void, glBindFramebuffer, (GLenum target, GLuint f))                                                       \
    X(void, glFramebufferTexture2D, (GLenum t, GLenum att, GLenum tt, GLuint tex, GLint lvl))                   \
    X(void, glFramebufferRenderbuffer, (GLenum t, GLenum att, GLenum rbt, GLuint rb))                           \
    X(GLenum, glCheckFramebufferStatus, (GLenum target))                                                        \
    X(void, glDeleteFramebuffers, (GLsizei n, const GLuint *f))                                                 \
    X(void, glGenRenderbuffers, (GLsizei n, GLuint * r))                                                        \
    X(void, glBindRenderbuffer, (GLenum target, GLuint r))                                                      \
    X(void, glRenderbufferStorage, (GLenum target, GLenum fmt, GLsizei w, GLsizei h))                           \
    X(void, glDeleteRenderbuffers, (GLsizei n, const GLuint *r))                                                \
    X(void, glDrawElements, (GLenum mode, GLsizei count, GLenum type, const void *idx))                         \
    X(void, glDrawArrays, (GLenum mode, GLint first, GLsizei count))                                            \
    X(void, glReadPixels, (GLint x, GLint y, GLsizei w, GLsizei h, GLenum f, GLenum t, void *p))                \
    X(void, glPixelStorei, (GLenum pname, GLint v))

typedef struct prts_gl {
#define PRTS_GL_DECLARE(ret, name, params) ret(PRTS_GLAPI *name) params;
    PRTS_GL_FUNCTIONS(PRTS_GL_DECLARE)
#undef PRTS_GL_DECLARE
} prts_gl;

typedef void *(*prts_gl_getproc)(const char *name);

/* Loads every function; returns the name of the first missing one, or NULL on success. */
const char *prts_gl_load(prts_gl *gl, prts_gl_getproc getproc);

#endif
