// Adapted from https://github.com/mhalber/Lines (instanced lines implementation)

uniform vec2 aa_radius;
uniform vec4 colour;

in vec4 v_col;
in /*noperspective*/ float v_u;
in /*noperspective*/ float v_v;
in /*noperspective*/ float v_line_width;
in /*noperspective*/ float v_line_length;

out vec4 f_colour;

void main()
{
	float au = 1.0 - smoothstep(1.0 - ((2.0 * aa_radius[0]) / v_line_width),
								1.0, abs(v_u / v_line_width));
	float av = 1.0 - smoothstep(1.0 - ((2.0 * aa_radius[1]) / v_line_length),
								1.0, abs(v_v / v_line_length));
	f_colour = v_col * colour;
	f_colour.a *= min(av, au);
}
