#version 330

in vec2 v_uv;

out vec4 f_color;

uniform sampler2D s_texture;

void main() {
    vec4 color = texture(s_texture, v_uv);

    f_color = color;
}