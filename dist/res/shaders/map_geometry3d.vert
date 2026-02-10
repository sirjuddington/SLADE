layout (location = 0) in vec3  in_position;
layout (location = 1) in vec2  in_tex_coord;
layout (location = 2) in float in_brightness;
layout (location = 3) in vec3  in_normal;

out VertexData
{
	vec2  tex_coord;
	float brightness;
	vec3  normal;
	vec3  view_pos;
} vertex_out;

uniform mat4  projection;
uniform mat4  modelview;
uniform bool  fullbright;
uniform float brightness_mult = 1.0;
uniform vec3  fog_colour = vec3(0.0);
uniform bool  fake_contrast = false;

void main()
{
	vertex_out.tex_coord = in_tex_coord;
	vertex_out.normal = in_normal;

	if (fullbright)
		vertex_out.brightness = 1.0;
	else
	{
		float brightness = in_brightness * brightness_mult;

		if (fake_contrast)
		{
			if (in_normal.x >= 0.999 || in_normal.x <= -0.999)
				brightness += 0.03125;
			else if (in_normal.y >= 0.999 || in_normal.y <= -0.999)
				brightness -= 0.03125;
		}

		vertex_out.brightness = brightness;
	}

	vec4 view_position = modelview * vec4(in_position, 1.0);
	vertex_out.view_pos = view_position.xyz;

	gl_Position = projection * view_position;
}
