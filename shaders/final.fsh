#version 330

in vec2 v_uv;

out vec4 f_color;

uniform sampler2D s_texture;

void main() {
    // Flip Y for pygame surface
    vec2 uv = vec2(v_uv.x, 1.0 - v_uv.y);

    vec4 color = texture(s_texture, uv);

    // get_view RGB -> BGR
    f_color = vec4(color.bgr, 1.0);
}