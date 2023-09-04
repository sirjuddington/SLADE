// Fragment shader for rendering a smooth antialiased circle

in VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_in;

out vec4 f_colour;

uniform vec4 colour;
uniform vec2 viewport_size;
uniform float point_radius;

varying float i_radius;


#ifdef OUTLINE // Circle Outline -----------------------------------------------

uniform float outline_width = 2.0;
uniform float fill_opacity = 0.0;

void main()
{
	float dist = distance(vertex_in.tex_coord, vec2(0.5));
	float delta = fwidth(dist);
    float alpha_ol = smoothstep(0.499 - delta, 0.499, dist);
	float outline_size = outline_width / (point_radius * i_radius);
	float alpha = smoothstep(0.499 - outline_size - delta, 0.499 - outline_size + delta, dist);
    
	f_colour.rgb = colour.rgb;
    f_colour.a = mix(colour.a * fill_opacity, colour.a, alpha);
    f_colour.a *= 1.0 - alpha_ol;
}

#else // Filled Circle ---------------------------------------------------------

void main()
{
	float dist = distance(vertex_in.tex_coord, vec2(0.5));
	float delta = fwidth(dist);
	f_colour = mix(vertex_in.colour * colour, vec4(0.0), smoothstep(0.499 - delta, 0.499 + delta, dist));
}

#endif
