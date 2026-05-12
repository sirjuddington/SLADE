// Vertex data
layout (location = 0) in vec2 in_vertex_pos;

// Instance Data
layout (location = 1) in vec2  in_position;
layout (location = 2) in float in_radius;

out VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_out;

out float i_radius;

uniform mat4 mvp;
uniform vec2 viewport_size;
uniform float point_radius;

void main()
{
	i_radius = in_radius * point_radius;

	vertex_out.colour = vec4(1.0);
	vertex_out.tex_coord = (in_vertex_pos + vec2(1.0, 1.0)) * vec2(0.5, 0.5);

	gl_Position = mvp * vec4(in_position + (in_vertex_pos * i_radius), 0.0, 1.0);
}
