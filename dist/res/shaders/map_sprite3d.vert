// Vertex data
layout (location = 0) in vec2 in_vertex;

// Instance data
layout (location = 1) in vec3  in_position;
layout (location = 2) in float in_brightness;
layout (location = 3) in vec4  in_colour;

out VertexData
{
	vec2  tex_coord;
	float brightness;
	vec3  normal;
	vec3  view_pos;
	vec4  colour;
} vertex_out;

uniform mat4  projection;
uniform mat4  modelview;
uniform bool  fullbright;
uniform float brightness_mult = 1.0;
uniform vec3  fog_colour = vec3(0.0);
uniform vec3  cam_right;
uniform vec2  sprite_size;

void main()
{
	vertex_out.normal = vec3(0.0, 0.0, 1.0);
	vertex_out.colour = in_colour;

	if (fullbright)
		vertex_out.brightness = 1.0;
	else
		vertex_out.brightness = in_brightness * brightness_mult;

	vec3 right = cam_right * in_vertex.x * sprite_size.x;
	vec3 up = vec3(0.0, 0.0, 1.0) * in_vertex.y * sprite_size.y;

	vec4 position = vec4(in_position - right + up, 1.0);
	vec4 view_position = modelview * position;
	vertex_out.view_pos = view_position.xyz;

	vertex_out.tex_coord = vec2(0.5 + in_vertex.x, 1.0 - in_vertex.y);

	gl_Position = projection * view_position;
}
