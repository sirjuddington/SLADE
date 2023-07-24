in VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_in;

out vec4 f_colour;

uniform vec4 colour;

void main()
{
	float dist = distance(vertex_in.tex_coord, vec2(0.5));
	float delta = fwidth(dist) * 2;
	float alpha = smoothstep(0.45-delta, 0.45, dist);
	f_colour = mix(vertex_in.colour * colour, vec4(0.0), alpha);
}
