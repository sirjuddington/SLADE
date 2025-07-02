in VertexData
{
	vec2 tex_coord;
	vec4 colour;
	vec3 normal;
} vertex_in;

out vec4 f_colour;

uniform vec4 colour = vec4(1.0);
//uniform vec2 viewport_size;

uniform sampler2D tex_unit;

void main()
{
	vec4 col = vertex_in.colour * colour * texture(tex_unit, vertex_in.tex_coord);

	f_colour = col;
}
