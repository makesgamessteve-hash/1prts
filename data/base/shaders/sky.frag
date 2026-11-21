// 1stPRTS sky: gradient dome, sun, the moons Ilen and Corvane, and stars. MIT License.
in vec2 v_ndc;
uniform vec3 u_forward;
uniform vec3 u_right;   // already scaled by tan(fov/2) * aspect
uniform vec3 u_up;      // already scaled by tan(fov/2)
uniform vec3 u_horizon;
uniform vec3 u_zenith;
uniform vec3 u_sun_dir;
uniform vec3 u_sun_color;
uniform float u_stars;
uniform float u_moons;
out vec4 frag_color;

float hash(vec3 p) {
    p = fract(p * 0.3183099 + 0.1);
    p *= 17.0;
    return fract(p.x * p.y * p.z * (p.x + p.y + p.z));
}

vec3 moon(vec3 dir, vec3 at, float size, vec3 color, vec3 sky) {
    float d = dot(dir, normalize(at));
    float disc = smoothstep(cos(size), cos(size * 0.92), d);
    float glow = pow(max(d, 0.0), 400.0) * 0.25;
    return mix(sky, color, disc) + color * glow;
}

void main() {
    vec3 dir = normalize(u_forward + v_ndc.x * u_right + v_ndc.y * u_up);
    float h = dir.y;
    vec3 sky = mix(u_horizon, u_zenith, pow(clamp(h, 0.0, 1.0), 0.55));
    if (h < 0.0) sky = mix(u_horizon, u_horizon * 0.55, clamp(-h * 4.0, 0.0, 1.0));
    float s = max(dot(dir, normalize(u_sun_dir)), 0.0);
    sky += u_sun_color * (pow(s, 600.0) * 2.0 + pow(s, 12.0) * 0.25);
    if (u_stars > 0.0 && h > 0.0) {
        vec3 cell = floor(dir * 220.0);
        float star = step(0.9965, hash(cell));
        sky += vec3(star * u_stars * smoothstep(0.0, 0.25, h));
    }
    if (u_moons > 0.0) {
        sky = moon(dir, vec3(-0.55, 0.42, -0.72), 0.060, vec3(0.86, 0.88, 0.95), sky);
        sky = moon(dir, vec3(0.70, 0.25, -0.67), 0.035, vec3(0.80, 0.33, 0.25), sky);
    }
    frag_color = vec4(sky, 1.0);
}
