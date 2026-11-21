/* 1stPRTS engine: float and double vector math. MIT License. */
#ifndef PRTS_MATH_H
#define PRTS_MATH_H

#include "prts_base.h"

#define PRTS_PI 3.14159265358979323846

typedef struct prts_vec3 { float x, y, z; } prts_vec3;
typedef struct prts_dvec3 { double x, y, z; } prts_dvec3;
/* Column-major 4x4 matrix, as OpenGL expects. m[col * 4 + row]. */
typedef struct prts_mat4 { float m[16]; } prts_mat4;

prts_vec3 prts_v3(float x, float y, float z);
prts_vec3 prts_v3_add(prts_vec3 a, prts_vec3 b);
prts_vec3 prts_v3_sub(prts_vec3 a, prts_vec3 b);
prts_vec3 prts_v3_scale(prts_vec3 a, float s);
float prts_v3_dot(prts_vec3 a, prts_vec3 b);
prts_vec3 prts_v3_cross(prts_vec3 a, prts_vec3 b);
float prts_v3_length(prts_vec3 a);
/* Returns the zero vector for zero-length input. */
prts_vec3 prts_v3_normalize(prts_vec3 a);
prts_vec3 prts_v3_lerp(prts_vec3 a, prts_vec3 b, float t);

prts_dvec3 prts_dv3(double x, double y, double z);
prts_dvec3 prts_dv3_add(prts_dvec3 a, prts_dvec3 b);
prts_dvec3 prts_dv3_sub(prts_dvec3 a, prts_dvec3 b);
prts_dvec3 prts_dv3_from_v3(prts_vec3 v);
prts_dvec3 prts_dv3_lerp(prts_dvec3 a, prts_dvec3 b, double t);
/* Position of p relative to origin, converted to float (camera-relative rendering). */
prts_vec3 prts_dv3_relative(prts_dvec3 p, prts_dvec3 origin);

/* Rotates v around the Y axis by yaw radians (positive yaw turns +Z towards +X). */
prts_vec3 prts_v3_rotate_y(prts_vec3 v, float yaw);
/* Forward direction for a yaw/pitch view (yaw 0 looks along -Z, pitch up is positive). */
prts_vec3 prts_view_forward(float yaw, float pitch);

prts_mat4 prts_mat4_identity(void);
prts_mat4 prts_mat4_mul(prts_mat4 a, prts_mat4 b);
prts_mat4 prts_mat4_translate(prts_vec3 t);
prts_mat4 prts_mat4_rotate_x(float angle);
prts_mat4 prts_mat4_rotate_y(float angle);
prts_mat4 prts_mat4_perspective(float fov_y, float aspect, float z_near, float z_far);
/* View matrix for an eye at the origin looking along yaw/pitch (camera-relative). */
prts_mat4 prts_mat4_view_yaw_pitch(float yaw, float pitch);
prts_vec3 prts_mat4_mul_point(prts_mat4 m, prts_vec3 p);

float prts_wrap_angle(float a);
float prts_clampf(float v, float lo, float hi);

#endif
