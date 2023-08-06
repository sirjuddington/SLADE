in VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_in;

out vec4 f_colour;

uniform vec4 colour;
uniform vec4 shadow_colour;
uniform sampler2D tex_unit;
uniform float softness;

const vec2 shadow_offset = vec2(0.002, 0.002);
const float shadow_smoothing = 0.08;

void main()
{
	float distance = texture2D(tex_unit, vertex_in.tex_coord).a;
	float alpha = smoothstep(0.49 - softness, 0.49 + softness, distance);
	vec4 text = vec4(colour.rgb, alpha);

	float shadow_distance = texture2D(tex_unit, vertex_in.tex_coord - shadow_offset).a;
	float shadow_alpha = smoothstep(0.48 - shadow_smoothing, 0.48 + shadow_smoothing, shadow_distance);
	vec4 shadow = vec4(shadow_colour.rgb, shadow_colour.a * shadow_alpha);

	f_colour = mix(shadow, text, text.a);
}
