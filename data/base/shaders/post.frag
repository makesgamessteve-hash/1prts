// 1stPRTS post-process: palette quantization with 4x4 ordered dither, the minimal crosshair
// of the HUD and the XR comfort vignette. Sampling is nearest-neighbour, so pixels stay sharp.
// MIT License.
in vec2 v_uv;
uniform sampler2D u_scene;
uniform vec2 u_size;      // internal resolution
uniform float u_dither;
uniform float u_crosshair;
uniform float u_vignette; // comfort vignette strength, 0 (off) to 1
uniform vec2 u_center;    // the view's optical centre (uv)
out vec4 frag_color;

float bayer4(vec2 p) {
    int x = int(mod(p.x, 4.0)), y = int(mod(p.y, 4.0));
    int i = x + y * 4;
    int m[16] = int[16](0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5);
    return (float(m[i]) + 0.5) / 16.0;
}

void main() {
    vec2 px = floor(v_uv * u_size);
    vec3 c = texture(u_scene, (px + 0.5) / u_size).rgb;
    if (u_dither > 0.0) {
        float levels = 24.0;
        c = floor(c * levels + bayer4(px)) / levels;
    }
    if (u_crosshair > 0.0) {
        vec2 d = abs(px - floor(u_size * 0.5));
        if ((d.x < 0.5 && d.y < 2.5 && d.y > 0.5) || (d.y < 0.5 && d.x < 2.5 && d.x > 0.5)) c = mix(c, vec3(1.0), 0.6);
    }
    if (u_vignette > 0.0) {
        float d = length((v_uv - u_center) * 2.0);
        c *= 1.0 - u_vignette * smoothstep(0.3, 0.95, d);
    }
    frag_color = vec4(c, 1.0);
}
