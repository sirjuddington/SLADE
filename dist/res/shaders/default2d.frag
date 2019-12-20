in VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_in;

out vec4 f_colour;

uniform vec4 colour;

#ifdef TEXTURED
uniform sampler2D tex_unit;
#endif

#ifdef THICK_LINES
uniform float line_width;
varying vec2 line_center;
#endif

void main()
{
#ifdef TEXTURED
	vec4 col = vertex_in.colour * colour * texture(tex_unit, vertex_in.tex_coord);
#else
	vec4 col = vertex_in.colour * colour;
#endif

#ifdef THICK_LINES
	float d = length(line_center - gl_FragCoord.xy);
	if (d > line_width)
		col.a = 0;
	else
		col.a *= pow(float((line_width - d) / line_width), 1.2);
#endif

	f_colour = col;
}
