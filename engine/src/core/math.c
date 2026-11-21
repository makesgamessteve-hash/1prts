/* 1stPRTS engine: vector math. MIT License. */
#include "prts/prts_math.h"

#include <math.h>
#include <string.h>

prts_vec3 prts_v3(float x, float y, float z) { prts_vec3 v = {x, y, z}; return v; }
prts_vec3 prts_v3_add(prts_vec3 a, prts_vec3 b) { return prts_v3(a.x + b.x, a.y + b.y, a.z + b.z); }
prts_vec3 prts_v3_sub(prts_vec3 a, prts_vec3 b) { return prts_v3(a.x - b.x, a.y - b.y, a.z - b.z); }
prts_vec3 prts_v3_scale(prts_vec3 a, float s) { return prts_v3(a.x * s, a.y * s, a.z * s); }
float prts_v3_dot(prts_vec3 a, prts_vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

prts_vec3 prts_v3_cross(prts_vec3 a, prts_vec3 b)
{
    return prts_v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

float prts_v3_length(prts_vec3 a) { return sqrtf(prts_v3_dot(a, a)); }

prts_vec3 prts_v3_normalize(prts_vec3 a)
{
    float len = prts_v3_length(a);
    return len > 0.0f ? prts_v3_scale(a, 1.0f / len) : prts_v3(0, 0, 0);
}

prts_vec3 prts_v3_lerp(prts_vec3 a, prts_vec3 b, float t)
{
    return prts_v3_add(a, prts_v3_scale(prts_v3_sub(b, a), t));
}

prts_dvec3 prts_dv3(double x, double y, double z) { prts_dvec3 v = {x, y, z}; return v; }
prts_dvec3 prts_dv3_add(prts_dvec3 a, prts_dvec3 b) { return prts_dv3(a.x + b.x, a.y + b.y, a.z + b.z); }
prts_dvec3 prts_dv3_sub(prts_dvec3 a, prts_dvec3 b) { return prts_dv3(a.x - b.x, a.y - b.y, a.z - b.z); }
prts_dvec3 prts_dv3_from_v3(prts_vec3 v) { return prts_dv3(v.x, v.y, v.z); }

prts_dvec3 prts_dv3_lerp(prts_dvec3 a, prts_dvec3 b, double t)
{
    return prts_dv3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t, a.z + (b.z - a.z) * t);
}

prts_vec3 prts_dv3_relative(prts_dvec3 p, prts_dvec3 origin)
{
    return prts_v3((float)(p.x - origin.x), (float)(p.y - origin.y), (float)(p.z - origin.z));
}

prts_vec3 prts_v3_rotate_y(prts_vec3 v, float yaw)
{
    float c = cosf(yaw), s = sinf(yaw);
    return prts_v3(v.x * c + v.z * s, v.y, -v.x * s + v.z * c);
}

prts_vec3 prts_view_forward(float yaw, float pitch)
{
    float cp = cosf(pitch);
    return prts_v3(-sinf(yaw) * cp, sinf(pitch), -cosf(yaw) * cp);
}

prts_mat4 prts_mat4_identity(void)
{
    prts_mat4 r;
    memset(&r, 0, sizeof(r));
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

prts_mat4 prts_mat4_mul(prts_mat4 a, prts_mat4 b)
{
    prts_mat4 r;
    for (int c = 0; c < 4; c++) {
        for (int row = 0; row < 4; row++) {
            float s = 0.0f;
            for (int k = 0; k < 4; k++) s += a.m[k * 4 + row] * b.m[c * 4 + k];
            r.m[c * 4 + row] = s;
        }
    }
    return r;
}

prts_mat4 prts_mat4_translate(prts_vec3 t)
{
    prts_mat4 r = prts_mat4_identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}

prts_mat4 prts_mat4_rotate_x(float angle)
{
    prts_mat4 r = prts_mat4_identity();
    float c = cosf(angle), s = sinf(angle);
    r.m[5] = c;
    r.m[6] = s;
    r.m[9] = -s;
    r.m[10] = c;
    return r;
}

prts_mat4 prts_mat4_rotate_y(float angle)
{
    prts_mat4 r = prts_mat4_identity();
    float c = cosf(angle), s = sinf(angle);
    r.m[0] = c;
    r.m[2] = -s;
    r.m[8] = s;
    r.m[10] = c;
    return r;
}

prts_mat4 prts_mat4_perspective(float fov_y, float aspect, float z_near, float z_far)
{
    prts_mat4 r;
    memset(&r, 0, sizeof(r));
    float f = 1.0f / tanf(fov_y * 0.5f);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (z_far + z_near) / (z_near - z_far);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * z_far * z_near) / (z_near - z_far);
    return r;
}

prts_mat4 prts_mat4_view_yaw_pitch(float yaw, float pitch)
{
    /* Inverse of (rotate_y(yaw) * rotate_x(pitch)). */
    return prts_mat4_mul(prts_mat4_rotate_x(-pitch), prts_mat4_rotate_y(-yaw));
}

prts_vec3 prts_mat4_mul_point(prts_mat4 m, prts_vec3 p)
{
    float x = m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12];
    float y = m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13];
    float z = m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14];
    float w = m.m[3] * p.x + m.m[7] * p.y + m.m[11] * p.z + m.m[15];
    if (w != 0.0f && w != 1.0f) return prts_v3(x / w, y / w, z / w);
    return prts_v3(x, y, z);
}

float prts_wrap_angle(float a)
{
    const float two_pi = (float)(2.0 * PRTS_PI);
    a = fmodf(a, two_pi);
    if (a > (float)PRTS_PI) a -= two_pi;
    if (a < -(float)PRTS_PI) a += two_pi;
    return a;
}

float prts_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
