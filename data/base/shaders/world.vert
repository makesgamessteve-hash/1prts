// 1stPRTS world shader: per-vertex (Gouraud) lighting, fog, optional vertex snapping and
// affine texturing. Positions are camera-relative. MIT License.
in vec3 a_pos;
in vec3 a_normal;
in vec2 a_uv;
in vec3 a_color;
in float a_sun;      // baked sun visibility
in float a_sky;      // baked sky visibility (0 indoors)

uniform mat4 u_view_proj;
uniform mat4 u_model;
uniform vec3 u_sun_dir;
uniform vec3 u_sun_color;
uniform vec3 u_ambient;
uniform vec2 u_fog_range;
uniform vec2 u_snap;        // half the internal resolution, or 0 to disable snapping
uniform float u_emissive;
uniform int u_light_count;
uniform vec3 u_light_pos[8];
uniform vec3 u_light_color[8];
uniform float u_light_radius[8];

out vec2 v_uv;
out vec3 v_uvw;
out vec3 v_light;
out float v_fog;

void main() {
    vec4 world = u_model * vec4(a_pos, 1.0);
    vec3 n = normalize(mat3(u_model) * a_normal);
    vec3 light = u_ambient * mix(0.3, 1.0, a_sky) + u_sun_color * max(dot(n, normalize(u_sun_dir)), 0.0) * a_sun;
    for (int i = 0; i < 8; i++) {
        if (i >= u_light_count) break;
        vec3 d = u_light_pos[i] - world.xyz;
        float dist = length(d);
        float k = clamp(1.0 - dist / u_light_radius[i], 0.0, 1.0);
        float wrap = max(dot(n, d / max(dist, 0.001)), 0.0) * 0.7 + 0.3;
        light += u_light_color[i] * k * k * wrap;
    }
    v_light = mix(light, vec3(1.0), u_emissive) * a_color;  // unlit surfaces keep their vertex colors
    vec4 clip = u_view_proj * world;
    if (u_snap.x > 0.0) {
        vec2 ndc = clip.xy / clip.w;
        ndc = floor(ndc * u_snap + 0.5) / u_snap;
        clip.xy = ndc * clip.w;
    }
    gl_Position = clip;
    v_uv = a_uv;
    v_uvw = vec3(a_uv * clip.w, clip.w);
    float dist = length(world.xyz);
    v_fog = (1.0 - u_emissive) * clamp((dist - u_fog_range.x) / (u_fog_range.y - u_fog_range.x), 0.0, 1.0);
}
