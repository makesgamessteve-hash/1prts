// 1stPRTS world shader (fragment). MIT License.
in vec2 v_uv;
in vec3 v_uvw;
in vec3 v_light;
in float v_fog;

uniform sampler2D u_tex;
uniform vec3 u_fog_color;
uniform float u_affine;
uniform float u_magic;   // shimmering illusion surface (war table, scrying pools)
uniform float u_time;
uniform float u_flip_v;  // live surface textures are stored bottom-up

out vec4 frag_color;

void main() {
    // Affine mapping: interpolating (uv * w, w) and dividing undoes perspective correction.
    vec2 uv = mix(v_uv, v_uvw.xy / v_uvw.z, u_affine);
    uv.y = mix(uv.y, 1.0 - uv.y, u_flip_v);
    uv += u_magic * 0.006 * vec2(sin(uv.y * 37.0 + u_time * 2.1), cos(uv.x * 29.0 + u_time * 1.7));
    vec4 tex = texture(u_tex, uv);
    if (tex.a < 0.5) discard;
    vec3 color = tex.rgb * v_light;
    if (u_magic > 0.0) {
        // a golden, softly flickering illusion of the picture
        float grey = dot(color, vec3(0.3, 0.59, 0.11));
        float scan = 0.82 + 0.18 * sin(gl_FragCoord.y * 1.7 + u_time * 5.0);
        color = mix(color, grey * vec3(1.25, 1.0, 0.55), 0.5) * scan * 1.25;
    }
    frag_color = vec4(mix(color, u_fog_color, v_fog), 1.0);
}
