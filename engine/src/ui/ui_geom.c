/* 1stPRTS engine: ray hit tests onto UI surfaces (eye ray, touch point, XR pointer). MIT License. */
#include <math.h>

#include "prts/prts_ui.h"

bool prts_ui_ray_quad(const prts_ui_quad *q, prts_dvec3 origin, prts_vec3 dir, float max_dist, float *u, float *v,
                      float *dist)
{
    prts_vec3 r = q->right, d = q->down;
    prts_vec3 n = prts_v3_cross(r, d);
    float denom = prts_v3_dot(dir, n);
    if (fabsf(denom) < 1e-9f) return false; /* parallel to the surface (or a degenerate surface) */
    /* everything relative to the ray origin keeps float precision far from the world origin */
    prts_vec3 o = prts_dv3_relative(q->origin, origin);
    float t = prts_v3_dot(o, n) / denom;
    if (t < 0.0f || t > max_dist) return false;
    prts_vec3 p = prts_v3_sub(prts_v3_scale(dir, t), o); /* hit point relative to the surface origin */
    /* solve p = a * r + b * d in the surface plane (r and d need not be perpendicular) */
    float rr = prts_v3_dot(r, r), rd = prts_v3_dot(r, d), dd = prts_v3_dot(d, d);
    float pr = prts_v3_dot(p, r), pd = prts_v3_dot(p, d);
    float det = rr * dd - rd * rd;
    float a = (pr * dd - pd * rd) / det, b = (pd * rr - pr * rd) / det;
    if (a < 0.0f || a > 1.0f || b < 0.0f || b > 1.0f) return false;
    *u = a;
    *v = b;
    *dist = t;
    return true;
}

prts_vec3 prts_ui_screen_ray(float yaw, float pitch, float fov_y, float aspect, float sx, float sy)
{
    prts_vec3 fwd = prts_view_forward(yaw, pitch);
    prts_vec3 right = prts_v3_normalize(prts_v3_cross(fwd, prts_v3(0, 1, 0)));
    prts_vec3 up = prts_v3_cross(right, fwd);
    float t = tanf(fov_y * 0.5f);
    prts_vec3 dir = prts_v3_add(fwd, prts_v3_scale(right, (sx * 2.0f - 1.0f) * t * aspect));
    dir = prts_v3_add(dir, prts_v3_scale(up, (1.0f - sy * 2.0f) * t));
    return prts_v3_normalize(dir);
}
