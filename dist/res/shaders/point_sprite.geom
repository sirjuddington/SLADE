layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in float radius[];

out VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_out;

varying float i_radius;

uniform mat4 mvp;
uniform float point_radius;

void main()
{
	float r = radius[0] * point_radius;
	vertex_out.colour = vec4(1.0);
	i_radius = radius[0];

	// Bottom left
	gl_Position = mvp * (gl_in[0].gl_Position + vec4(-r, r, 0.0, 0.0));
	vertex_out.tex_coord = vec2(0.0, 1.0);
	EmitVertex();

	// Bottom right
	gl_Position = mvp * (gl_in[0].gl_Position + vec4(r, r, 0.0, 0.0));
	vertex_out.tex_coord = vec2(1.0, 1.0);
	EmitVertex();

	// Top left
	gl_Position = mvp * (gl_in[0].gl_Position + vec4(-r, -r, 0.0, 0.0));
	vertex_out.tex_coord = vec2(0.0, 0.0);
	EmitVertex();

	// Top right
	gl_Position = mvp * (gl_in[0].gl_Position + vec4(r, -r, 0.0, 0.0));
	vertex_out.tex_coord = vec2(1.0, 0.0);
	EmitVertex();

	EndPrimitive();
}
