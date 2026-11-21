// 1stPRTS UI: 2D quads (menus, HUD, console, touch overlay, world surfaces). MIT License.
in vec2 a_pos;   // UI units, origin top-left
in vec2 a_uv;
in vec4 a_color;
uniform vec2 u_size;   // target size in UI units
uniform float u_flip;  // 1: the window; -1: a surface texture (texture row 0 is the top of the UI)
out vec2 v_uv;
out vec4 v_color;
void main() {
    v_uv = a_uv;
    v_color = a_color;
    vec2 p = a_pos / u_size * 2.0 - 1.0;
    gl_Position = vec4(p.x, -p.y * u_flip, 0.0, 1.0);
}
