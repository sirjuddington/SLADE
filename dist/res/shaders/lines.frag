// Adapted from https://github.com/mhalber/Lines (instanced lines implementation)

uniform vec2 aa_radius;
uniform vec4 colour;

#ifdef DASHED_LINES
uniform float dash_size = 6.0;
uniform float gap_size = 6.0;
#endif

#ifdef DISTANCE_FADE
uniform float max_dist = 40000.0;
in /*noperspective*/ float v_view_depth;
#endif

in vec4 v_col;
in /*noperspective*/ float v_u;
in /*noperspective*/ float v_v;
in /*noperspective*/ float v_line_width;
in /*noperspective*/ float v_line_length;

out vec4 f_colour;

void main()
{
#ifdef DASHED_LINES
	if (fract(v_v / (dash_size + gap_size)) > dash_size / (dash_size + gap_size))
		discard;
#endif

	float au = 1.0 - smoothstep(1.0 - ((2.0 * aa_radius[0]) / v_line_width),
								1.0, abs(v_u / v_line_width));
	float av = 1.0 - smoothstep(1.0 - ((2.0 * aa_radius[1]) / v_line_length),
								1.0, abs(v_v / v_line_length));
	f_colour = v_col * colour;
	f_colour.a *= min(av, au);

#ifdef DISTANCE_FADE
	float alpha_fade = clamp((max_dist - v_view_depth) / 128.0, 0.0, 1.0);
	f_colour.a *= alpha_fade;
#endif
}
