in VertexData
{
	vec2  tex_coord;
	float brightness;
	vec3  normal;
} vertex_in;

out vec4 f_colour;

uniform vec4 colour = vec4(1.0);

#ifdef ALPHA_TEST
uniform float alpha_threshold = 0.0;
#endif

uniform sampler2D tex_unit;

void main()
{
	vec4 tex_col = texture(tex_unit, vertex_in.tex_coord);

#ifdef ALPHA_TEST
	if (tex_col.a <= alpha_threshold) discard;
#endif

	f_colour = tex_col * colour * vec4(vec3(vertex_in.brightness), 1.0);
}
