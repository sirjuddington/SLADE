in VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_in;

out vec4 f_colour;

uniform vec4 colour = vec4(1.0);

#ifdef TEXTURED
uniform sampler2D tex_unit;
#endif

void main()
{
#ifdef TEXTURED
	vec4 col = vertex_in.colour * colour * texture(tex_unit, vertex_in.tex_coord);
#else
	vec4 col = vertex_in.colour * colour;
#endif

	f_colour = col;
}
