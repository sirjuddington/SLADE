layout (location = 0) in vec2 in_position;
layout (location = 1) in vec4 in_colour;
layout (location = 2) in vec2 in_tex_coord;

out VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_out;

uniform mat4 mvp;

#ifdef LINE_STIPPLE
flat out vec2 line_start;
out vec2      line_pos;
#endif

void main()
{
	vertex_out.colour = in_colour;
	vertex_out.tex_coord = in_tex_coord;

#ifdef GEOMETRY_SHADER
	// Perform matrix transformations in the geometry shader if it exists
	gl_Position = vec4(in_position, 0.0, 1.0);
#else
	gl_Position = mvp * vec4(in_position, 0.0, 1.0);
#endif

#ifdef LINE_STIPPLE
	line_pos = gl_Position.xy / gl_Position.w;
	line_start = line_pos;
#endif
}
