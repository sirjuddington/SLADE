#version 330 core

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec4 in_colour;
layout (location = 2) in vec2 in_tex_coord;

out vec4 v_colour;
out vec2 v_tex_coord;

uniform mat4 projection;
uniform mat4 model;

void main()
{
	v_colour = in_colour;
	v_tex_coord = in_tex_coord;
	gl_Position = projection * model * vec4(in_position, 0.0, 1.0);
	//gl_Position = projection * vec4(in_position, 0.0, 1.0);
}
