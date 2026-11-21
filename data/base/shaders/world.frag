// 1stPRTS world shader (fragment). MIT License.
in vec2 v_uv;
in vec3 v_uvw;
in vec3 v_light;
in float v_fog;

uniform sampler2D u_tex;
uniform vec3 u_fog_color;
uniform float u_affine;

out vec4 frag_color;

void main() {
    // Affine mapping: interpolating (uv * w, w) and dividing undoes perspective correction.
    vec2 uv = mix(v_uv, v_uvw.xy / v_uvw.z, u_affine);
    vec4 tex = texture(u_tex, uv);
    if (tex.a < 0.5) discard;
    vec3 color = tex.rgb * v_light;
    frag_color = vec4(mix(color, u_fog_color, v_fog), 1.0);
}
