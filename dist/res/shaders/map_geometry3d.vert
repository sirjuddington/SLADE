layout (location = 0) in vec3  in_position;
layout (location = 1) in vec2  in_tex_coord;
layout (location = 2) in float in_brightness;
layout (location = 3) in vec3  in_normal;

out VertexData
{
	vec2  tex_coord;
	float brightness;
	vec3  normal;
} vertex_out;

uniform mat4 mvp;
uniform bool fullbright;

void main()
{
	vertex_out.tex_coord = in_tex_coord;
	vertex_out.normal = in_normal;

	if (fullbright)
		vertex_out.brightness = 1.0;
	else
		vertex_out.brightness = in_brightness;

	gl_Position = mvp * vec4(in_position, 1.0);
}
