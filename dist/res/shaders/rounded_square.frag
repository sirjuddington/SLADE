// Fragment shader for rendering a smooth antialiased square with rounded edges

in VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_in;

in float i_radius;

out vec4 f_colour;

uniform vec4 colour;
uniform vec2 viewport_size;
uniform float point_radius;

const float corner_radius = 0.025;

float rounded_rect_dist(vec2 center, vec2 size, float radius)
{
    return length(max(abs(center) - size + radius, 0.0)) - radius;
}

#ifdef OUTLINE // Outline ------------------------------------------------------

uniform float outline_width = 2.0;
uniform float fill_opacity = 0.0;

void main()
{
	float outline_size = (outline_width / (point_radius * i_radius));

	float dist = rounded_rect_dist(vertex_in.tex_coord - vec2(0.5), vec2(0.5 - outline_size), corner_radius);
	float delta = fwidth(dist);// * 0.5;

    float alpha = smoothstep(0.0 - delta, delta, dist);
	float alpha_ol = smoothstep(outline_size - delta, outline_size + delta, dist);
    
	f_colour.rgb = colour.rgb;
    f_colour.a = mix(colour.a * fill_opacity, colour.a, alpha);
    f_colour.a *= 1.0 - alpha_ol;
}

#else // Filled ----------------------------------------------------------------

void main()
{
	float dist = rounded_rect_dist(vertex_in.tex_coord - vec2(0.5), vec2(0.5), corner_radius);
	float delta = fwidth(dist);
	f_colour = mix(vertex_in.colour * colour, vec4(0.0), smoothstep(0.0 - delta, 0.0 + delta, dist));
}

#endif
