in VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_in;

out vec4 f_colour;

uniform vec4 colour;
uniform vec4 outline_colour;
uniform sampler2D tex_unit;
uniform float softness;

const float outline_dist = 0.42;

void main()
{
	float softness_ol = softness * 1.2;

	float sdf_dist = texture(tex_unit, vertex_in.tex_coord).a;
	float outline_factor = smoothstep(0.495 - softness_ol, 0.495 + softness_ol, sdf_dist);
	float alpha = smoothstep(outline_dist - softness_ol, outline_dist + softness_ol, sdf_dist);
	
	f_colour.rgb = mix(outline_colour.rgb, colour.rgb, outline_factor);
	f_colour.a = colour.a * alpha;
}
