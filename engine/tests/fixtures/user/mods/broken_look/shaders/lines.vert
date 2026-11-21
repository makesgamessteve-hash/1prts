// compiles, but declares v_height with a different type than lines.frag: a link error
in vec3 a_pos;
uniform mat4 u_view_proj;
uniform mat4 u_model;
out vec3 v_height;
void main() { v_height = a_pos; gl_Position = u_view_proj * u_model * vec4(a_pos, 1.0); }
