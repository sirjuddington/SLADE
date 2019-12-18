#version 330 core

in vec4 v_colour;
in vec2 v_tex_coord;

out vec4 f_colour;

uniform vec4 colour;

void main()
{
	f_colour = v_colour * colour;
}
