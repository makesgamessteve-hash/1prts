// 1stPRTS illusion lines (war table map). MIT License.
in vec3 a_pos;
uniform mat4 u_view_proj;
uniform mat4 u_model;
out float v_height;
void main() {
    v_height = a_pos.y;
    gl_Position = u_view_proj * u_model * vec4(a_pos, 1.0);
}
