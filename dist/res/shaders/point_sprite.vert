layout (location = 0) in vec2  in_position;
layout (location = 1) in float in_radius;

out float radius;

uniform mat4 mvp;
uniform vec2 viewport_size;

void main()
{
	radius = in_radius;
	
	// Perform matrix transformations in the geometry shader
	gl_Position = vec4(in_position, 0.0, 1.0);
}
