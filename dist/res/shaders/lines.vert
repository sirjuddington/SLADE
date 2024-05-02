// Adapted from https://github.com/mhalber/Lines (instanced lines implementation)

layout(location = 0) in vec3 in_quad_pos;
layout(location = 1) in vec4 in_v1_pos_width;
layout(location = 2) in vec4 in_v1_colour;
layout(location = 3) in vec4 in_v2_pos_width;
layout(location = 4) in vec4 in_v2_colour;

uniform mat4 mvp;
uniform vec2 viewport_size;
uniform vec2 aa_radius;
uniform float line_width;

out vec4 v_col;
out /*noperspective*/ float v_u;
out /*noperspective*/ float v_v;
out /*noperspective*/ float v_line_width;
out /*noperspective*/ float v_line_length;

void main()
{
	float v1_width = in_v1_pos_width.w * line_width;
	float v2_width = in_v2_pos_width.w * line_width;
	float u_width = viewport_size[0];
	float u_height = viewport_size[1];
	float u_aspect_ratio = u_height / u_width;

	vec4 colors[2] = vec4[2](in_v1_colour, in_v2_colour);
	colors[0].a *= min(1.0, v1_width);
	colors[1].a *= min(1.0, v2_width);
	v_col = colors[int(in_quad_pos.x)];

	vec4 clip_pos_a = mvp * vec4(in_v1_pos_width.xyz, 1.0f);
	vec4 clip_pos_b = mvp * vec4(in_v2_pos_width.xyz, 1.0f);

	vec2 ndc_pos_0 = clip_pos_a.xy / clip_pos_a.w;
	vec2 ndc_pos_1 = clip_pos_b.xy / clip_pos_b.w;

	vec2 line_vector = ndc_pos_1 - ndc_pos_0;
	vec2 viewport_line_vector = line_vector * viewport_size;
	vec2 dir = normalize(vec2(line_vector.x, line_vector.y * u_aspect_ratio));

	float extension_length = aa_radius.y;
	float line_length = length(line_vector * viewport_size) + 2.0 * extension_length;
	float line_width_a = max(1.0, v1_width) + aa_radius.x;
	float line_width_b = max(1.0, v2_width) + aa_radius.x;

	vec2 normal = vec2(-dir.y, dir.x);
	vec2 normal_a = vec2(line_width_a / u_width, line_width_a / u_height) * normal;
	vec2 normal_b = vec2(line_width_b / u_width, line_width_b / u_height) * normal;
	vec2 extension = vec2(extension_length / u_width, extension_length / u_height) * dir;

	v_line_width = (1.0 - in_quad_pos.x) * line_width_a + in_quad_pos.x * line_width_b;
	v_line_length = 0.5 * line_length;
	v_v = (2.0 * in_quad_pos.x - 1.0) * v_line_length;
	v_u = in_quad_pos.y * v_line_width;

	vec2 zw_part = (1.0 - in_quad_pos.x) * clip_pos_a.zw + in_quad_pos.x * clip_pos_b.zw;
	vec2 dir_y = in_quad_pos.y * ((1.0 - in_quad_pos.x) * normal_a + in_quad_pos.x * normal_b);
	vec2 dir_x = in_quad_pos.x * line_vector + (2.0 * in_quad_pos.x - 1.0) * extension;

	gl_Position = vec4((ndc_pos_0 + dir_x + dir_y) * zw_part.y, zw_part);
}
