// Input
in VertexData
{
	vec2  position;
	vec2  direction;
	vec2  tex_coord;
	float alpha;
} vertex_in;

// Output
out vec4 f_colour;

// Uniforms
uniform sampler2D tex_unit;
uniform vec4      colour;
#ifdef ARROW
uniform float shadow_opacity;
#endif

// Constants
#ifdef ARROW
const float outline_brightness = 0.35;
const float shadow_smoothing   = 0.14;
const vec2  arrow_sc           = vec2(sin(0.75), cos(0.75));

float sd_arrow(vec2 uv, vec2 p_start, vec2 p_end, float head_length, float thickness, vec2 sin_cos)
{
    vec2 pa = uv - p_start;
	vec2 ba = p_end - p_start;

    // body
    float h0 = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    vec2 q0 = pa - ba * h0;

    // rotation
	vec2 d = normalize(ba);
    vec2 q = mat2(d.x, -d.y, d.y, d.x) * (p_end - uv);
    q.y = abs(q.y);
    
    // head
    float h1 = clamp(dot(head_length * sin_cos - q, sin_cos) / head_length, 0.0, 1.0);
    vec2 q1 = q + sin_cos * head_length*(h1 - 1.0);
    
    return sqrt(min(dot(q0, q0), dot(q1, q1))) - thickness;
}

vec4 arrow(vec2 uv, vec2 direction, float length)
{
	float thickness = 0.02;

	vec2 start_point = vec2(0.5, 0.5) + direction * (length * 0.2);
    vec2 end_point = vec2(0.5, 0.5) + direction * (length * 0.6);
    
    float dist = sd_arrow(uv, start_point, end_point, 0.125, thickness, arrow_sc);
    float delta = fwidth(dist);
    
    float alpha_ol = smoothstep((thickness * 2.0) - delta, (thickness * 2.0) + delta, dist);
	float alpha = smoothstep(0.0 - delta, 0.0 + delta, dist);
    
    vec4 fill_colour = vec4(1.0, 1.0, 1.0, colour.a);
    vec4 out_colour = vec4(0.0);
    
    // Apply outline
	out_colour.rgb = mix(fill_colour.rgb, colour.rgb * outline_brightness, alpha);
	out_colour.a = fill_colour.a * (1.0 - alpha_ol);
    
    // Apply drop shadow
    float alpha_ds = smoothstep((thickness * 1.5) - (shadow_smoothing * 0.5), (thickness * 1.5) + (shadow_smoothing * 0.5), dist);
	vec4 shadow = vec4(vec3(0.0), (1.0 - alpha_ds) * (shadow_opacity * 0.75));
	out_colour = mix(shadow, out_colour, out_colour.a);
    
    return out_colour;
}
#endif // ARROW

void main()
{
	// Clip to sprite bounds
	vec2 tc = vertex_in.tex_coord;
	vec4 sprite_col = vec4(0.0);
	if (tc.x >= 0.0 && tc.x <= 1.0 && tc.y >= 0.0 && tc.y <= 1.0)
	{
		vec4 col_tex = texture(tex_unit, tc);
		sprite_col = vec4(col_tex.rgb, col_tex.a);
	}

	f_colour = sprite_col;

#ifdef ARROW
	vec4 c_arrow = arrow(vertex_in.position, vertex_in.direction, 0.55);
	f_colour = mix(f_colour, c_arrow, c_arrow.a);
#endif

	if (f_colour.a < 0.01)
		discard;

	f_colour.a *= colour.a * vertex_in.alpha;
}
