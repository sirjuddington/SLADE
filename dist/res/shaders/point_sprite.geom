layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_in[];

out VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_out;

uniform mat4 mvp;
uniform float point_radius;

void main()
{
	vertex_out.colour = vertex_in[0].colour;

	// Bottom left
	gl_Position = mvp * (gl_in[0].gl_Position + vec4(-point_radius, point_radius, 0.0, 0.0));
	vertex_out.tex_coord = vec2(0.0, 1.0);
	EmitVertex();

	// Bottom right
	gl_Position = mvp * (gl_in[0].gl_Position + vec4(point_radius, point_radius, 0.0, 0.0));
	vertex_out.tex_coord = vec2(1.0, 1.0);
	EmitVertex();

	// Top left
	gl_Position = mvp * (gl_in[0].gl_Position + vec4(-point_radius, -point_radius, 0.0, 0.0));
	vertex_out.tex_coord = vec2(0.0, 0.0);
	EmitVertex();

	// Top right
	gl_Position = mvp * (gl_in[0].gl_Position + vec4(point_radius, -point_radius, 0.0, 0.0));
	vertex_out.tex_coord = vec2(1.0, 0.0);
	EmitVertex();

	EndPrimitive();
}
