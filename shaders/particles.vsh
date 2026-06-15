#version 330

in vec2 in_position;
in vec2 in_velocity;
in uint in_material;
in vec3 in_user_color;

out vec2 v_velocity;
flat out uint v_material;
out vec3 v_user_color;

void main() {
    gl_Position = vec4(in_position, 0.0, 1.0);

    v_velocity = in_velocity;
    v_material = in_material;
    v_user_color = in_user_color;
}