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
	float distance = texture2D(tex_unit, vertex_in.tex_coord).a;
	float outline_factor = smoothstep(0.5 - softness, 0.5 + softness, distance);
	float alpha = smoothstep(outline_dist - softness, outline_dist + softness, distance);
	
	f_colour.rgb = mix(outline_colour.rgb, colour.rgb, outline_factor);
	f_colour.a = alpha;
}
