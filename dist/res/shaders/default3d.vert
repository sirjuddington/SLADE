layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_tex_coord;
layout (location = 2) in vec4 in_colour;
layout (location = 3) in vec3 in_normal;

out VertexData
{
	vec2 tex_coord;
	vec4 colour;
	vec3 normal;
} vertex_out;

uniform mat4 mvp;

void main()
{
	vertex_out.tex_coord = in_tex_coord;
	vertex_out.normal = in_normal;
	vertex_out.colour = in_colour;

	gl_Position = mvp * vec4(in_position, 1.0);
}
