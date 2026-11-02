// Vertex data
layout (location = 0) in vec2 in_position;

// Instance data
layout (location = 1) in vec2  inst_position;
layout (location = 2) in vec2  inst_direction;
layout (location = 3) in float inst_alpha;

out VertexData
{
	vec2  position;
	vec2  direction;
	vec2  tex_coord;
	float alpha;
} vertex_out;

uniform mat4  mvp;
uniform vec2  viewport_size;
uniform float radius;
uniform vec2  tex_size;

const float padding      = 1.5;
const float half_padding = padding / 2.0;

void main()
{
	vertex_out.position = vec2(0.5 + (in_position.x * half_padding), 0.5 + (-in_position.y * half_padding));
	vertex_out.direction = vec2(inst_direction.x, -inst_direction.y);
	vertex_out.tex_coord = vec2(0.5 + (in_position.x * half_padding * tex_size.x), 0.5 + (-in_position.y * half_padding * tex_size.y));
	vertex_out.alpha = inst_alpha;

	gl_Position = mvp * vec4(inst_position + in_position * (radius * padding), 0.0, 1.0);
}
