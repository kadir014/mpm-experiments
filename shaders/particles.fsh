#version 330

in vec2 v_velocity;
flat in uint v_material;

out vec4 f_color;

vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );
    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}

float colors[2] = float[](
    0.4055,
    0.5861
);

void main() {
    vec2 uv = gl_PointCoord;
    float dist = length(uv - vec2(0.5));

    float radius = 0.5;
    float aa = fwidth(dist);
    //float aa = 0.5;

    float alpha = 1.0 - smoothstep(
        radius - aa,
        radius,
        dist
    );

    float speed = dot(v_velocity, v_velocity);

    float hue = colors[v_material] - clamp(speed * 0.00005, 0.0, 0.09);
    float light = clamp(0.5 + speed * 0.0001, 0.0, 0.85);
    vec3 color = hsl2rgb(vec3(hue, 1.0, light));

    f_color = vec4(color, alpha);
}