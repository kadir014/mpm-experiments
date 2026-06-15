#version 330

in vec2 v_velocity;
flat in uint v_material;
in vec3 v_user_color;

out vec4 f_color;


// Color conversions:
// https://www.shadertoy.com/view/wt23Rt & https://www.shadertoy.com/view/XljGzV

vec3 hsl2rgb(vec3 c) {
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );
    return c.z + c.y * (rgb-0.5)*(1.0-abs(2.0*c.z-1.0));
}

vec3 rgb2hsl( in vec3 c ){
  float h = 0.0;
	float s = 0.0;
	float l = 0.0;
	float r = c.r;
	float g = c.g;
	float b = c.b;
	float cMin = min( r, min( g, b ) );
	float cMax = max( r, max( g, b ) );

	l = ( cMax + cMin ) / 2.0;
	if ( cMax > cMin ) {
		float cDelta = cMax - cMin;
        
        //s = l < .05 ? cDelta / ( cMax + cMin ) : cDelta / ( 2.0 - ( cMax + cMin ) ); Original
		s = l < .0 ? cDelta / ( cMax + cMin ) : cDelta / ( 2.0 - ( cMax + cMin ) );
        
		if ( r == cMax ) {
			h = ( g - b ) / cDelta;
		} else if ( g == cMax ) {
			h = 2.0 + ( b - r ) / cDelta;
		} else {
			h = 4.0 + ( r - g ) / cDelta;
		}

		if ( h < 0.0) {
			h += 6.0;
		}
		h = h / 6.0;
	}
	return vec3( h, s, l );
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

    vec3 color = vec3(0.0);
    if (!all(equal(v_user_color, vec3(0.0)))) {
        //vec3 hsl = rgb2hsl(v_user_color);
        //hsl.b = clamp(hsl.b + speed * 0.0001, 0.0, 0.85);
        //color = hsl2rgb(hsl);
        color = v_user_color;
    }
    else {
        float hue = colors[v_material] - clamp(speed * 0.00005, 0.0, 0.09);
        float light = clamp(0.5 + speed * 0.0001, 0.0, 0.85);
        color = hsl2rgb(vec3(hue, 1.0, light));
    }

    f_color = vec4(color, alpha);
}