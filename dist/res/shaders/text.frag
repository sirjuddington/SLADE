in VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_in;

out vec4 f_colour;

uniform vec4 colour;
uniform sampler2D tex_unit;
uniform float softness;

void main()
{
	float sdf_dist = texture(tex_unit, vertex_in.tex_coord).a;
	float alpha = smoothstep(0.5 - softness, 0.5 + softness, sdf_dist);
	
	f_colour.rgb = colour.rgb;
	f_colour.a = colour.a * alpha;
}
