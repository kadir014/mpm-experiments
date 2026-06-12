#version 330

in vec2 v_uv;

out vec4 f_color;

void main() {
    vec2 uv = gl_PointCoord;
    float delta = length(uv - 0.5) - 0.5;
    if (delta > 0.0) discard;

    f_color = vec4(uv.x, uv.y, 0.0, 1.0);
}