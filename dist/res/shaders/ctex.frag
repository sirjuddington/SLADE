// Fragment shader for the composite texture canvas,
// has uniforms to define a 'viewable' area and colour for fragments outside it

in VertexData
{
	vec4 colour;
	vec2 tex_coord;
} vertex_in;

out vec4 f_colour;

uniform vec4 colour;
uniform sampler2D tex_unit;
uniform vec2 viewport_size;
uniform vec2 view_tl;
uniform vec2 view_br;
uniform vec4 outside_colour;

void main()
{
	vec4 col = vertex_in.colour * colour * texture(tex_unit, vertex_in.tex_coord);
	float fy = viewport_size.y - gl_FragCoord.y;

	if (gl_FragCoord.x < view_tl.x || gl_FragCoord.x > view_br.x ||
		fy < view_tl.y || fy > view_br.y)
		col *= outside_colour;

	f_colour = col;
}
