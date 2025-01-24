layout (location = 0) in vec3 in_position;
layout (location = 1) in vec2 in_tex_coord;
layout (location = 2) in vec3 in_normal;

out VertexData
{
	vec2 tex_coord;
	vec3 normal;
} vertex_out;

uniform mat4 mvp;

void main()
{
	vertex_out.tex_coord = in_tex_coord;
	vertex_out.normal = in_normal;

	gl_Position = mvp * vec4(in_position, 1.0);
}
