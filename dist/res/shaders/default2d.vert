layout (location = 0) in vec2 in_position;
layout (location = 1) in vec4 in_colour;
layout (location = 2) in vec2 in_tex_coord;

out VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_out;

uniform mat4 projection;
uniform mat4 model;
uniform vec2 viewport_size;

#ifdef THICK_LINES
varying vec2 line_center;
#endif

void main()
{
	vertex_out.colour = in_colour;
	vertex_out.tex_coord = in_tex_coord;

#ifdef GEOMETRY_SHADER
	// Perform matrix transformations in the geometry shader if it exists
	gl_Position = vec4(in_position, 0.0, 1.0);
#else
	gl_Position = projection * model * vec4(in_position, 0.0, 1.0);
#endif

#ifdef THICK_LINES
	line_center = 0.5 * (gl_Position.xy + vec2(1, 1)) * viewport_size;
#endif
}
