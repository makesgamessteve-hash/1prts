// deliberately broken shader: the engine must report it and fall back
out vec4 frag_color;
void main() { frag_color = vec4(1.0) +; }
