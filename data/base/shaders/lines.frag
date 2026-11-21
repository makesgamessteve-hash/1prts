// 1stPRTS illusion lines: a soft magical shimmer. MIT License.
in float v_height;
uniform vec3 u_color;
uniform float u_time;
out vec4 frag_color;
void main() {
    float shimmer = 0.75 + 0.25 * sin(u_time * 3.0 + v_height * 40.0);
    frag_color = vec4(u_color * shimmer, 0.9);
}
