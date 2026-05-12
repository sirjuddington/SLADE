in VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_in;

out vec4 f_colour;

uniform vec4 colour = vec4(1.0);
uniform vec2 viewport_size;

#ifdef TEXTURED
uniform sampler2D tex_unit;
#endif

#ifdef LINE_STIPPLE
flat in vec2  line_start;
in vec2       line_pos;
uniform uint  stipple_pattern;
uniform float stipple_factor;
#endif

void main()
{
#ifdef TEXTURED
	vec4 col = vertex_in.colour * colour * texture(tex_unit, vertex_in.tex_coord);
#else
	vec4 col = vertex_in.colour * colour;
#endif

#ifdef LINE_STIPPLE
	vec2 dir = (line_pos - line_start) * viewport_size / 2.0;
	float dist = length(dir);

	uint bit = uint(round(dist / stipple_factor)) & 15U;
	if ((stipple_pattern & (1U << bit)) == 0U)
		discard;
#endif

	f_colour = col;
}
