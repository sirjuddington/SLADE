in VertexData
{
	vec2  tex_coord;
	float brightness;
	vec3  normal;
	vec3  view_pos;
} vertex_in;

out vec4 f_colour;

uniform vec4  colour      = vec4(1.0);
uniform vec3  fog_colour  = vec3(0.0);
uniform float fog_density = 1.0;
uniform float max_dist    = 40000.0;

#ifdef ALPHA_TEST
uniform float alpha_threshold = 0.0;
#endif

uniform sampler2D tex_unit;

float fogFactor()
{
	float inv_brightness = 1.0 - vertex_in.brightness;

	float depth   = length(vertex_in.view_pos);
	float density = inv_brightness * fog_density;
	float factor  = exp(-(density * density * density) * depth * 0.01);
	
	return clamp(factor, vertex_in.brightness * vertex_in.brightness, 1.0);
}

void main()
{
	// Start with texture colour
	vec4 frag_colour = texture(tex_unit, vertex_in.tex_coord);

#ifdef ALPHA_TEST
	if (frag_colour.a <= alpha_threshold) discard;
#endif

	// Apply brightness and overall colour
	frag_colour = frag_colour * colour * vec4(vec3(vertex_in.brightness), 1.0);

	// Apply fog
	f_colour = vec4(mix(fog_colour, frag_colour.rgb, fogFactor()), frag_colour.a);

	// Apply alpha fade at 128 units from max distance
	float xy_dist = length(vertex_in.view_pos);
	float alpha_fade = clamp((max_dist - xy_dist) / 128.0, 0.0, 1.0);
	f_colour.a *= alpha_fade;
}
