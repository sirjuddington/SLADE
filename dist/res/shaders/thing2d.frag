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
uniform vec4 colour;
uniform sampler2D tex_unit;
uniform float radius;

// Constants
const float outline_size = 0.065;
const float outline_brightness = 0.35;
const float circle_size = 0.499;
const float shadow_smoothing = 0.14;
const vec2 arrow_sc = vec2(sin(0.75), cos(0.75));
const float arrow_thickness = 0.02;

#ifdef SQUARE

// Square Shape
float sd_rounded_rect(vec2 center, vec2 size, float radius)
{
    return length(max(abs(center) - size + radius, 0.0)) - radius;
}
vec4 shape(vec2 uv, vec4 base_colour)
{
	float dist = sd_rounded_rect(uv - vec2(0.5), vec2(0.5 - outline_size), 0.025);
	float delta = fwidth(dist) * 0.5;
	float alpha_ol = smoothstep(outline_size - delta, outline_size + delta, dist);
	float alpha = smoothstep(0.0 - delta, 0.0 + delta, dist);
	float alpha_ds = smoothstep(outline_size - shadow_smoothing, outline_size + shadow_smoothing, dist);

	// Apply outline
	vec4 out_colour = vec4(
		mix(base_colour.rgb, base_colour.rgb * outline_brightness, alpha),
		base_colour.a * (1.0 - alpha_ol)
	);

	// Apply drop shadow
	vec4 shadow = vec4(vec3(0.0), (1.0 - alpha_ds) * 0.7);
	out_colour = mix(shadow, out_colour, out_colour.a);

	return out_colour;
}
#else

// Round Shape
vec4 shape(vec2 uv, vec4 base_colour)
{
	float dist = distance(uv, vec2(0.5));
	float delta = fwidth(dist);
	float size = circle_size;
	float alpha_ol = smoothstep(size - delta, size, dist);
	float alpha = smoothstep(size - outline_size - delta, size - outline_size + delta, dist);
	float alpha_ds = smoothstep(size - shadow_smoothing, size + shadow_smoothing, dist);

	// Apply outline
	vec4 out_colour = vec4(
		mix(base_colour.rgb, base_colour.rgb * outline_brightness, alpha),
		base_colour.a * (1.0 - alpha_ol)
	);

	// Apply drop shadow
	vec4 shadow = vec4(vec3(0.0), (1.0 - alpha_ds) * 0.7);
	out_colour = mix(shadow, out_colour, out_colour.a);

	return out_colour;
}

#endif

#ifdef ARROW
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
	vec2 start_point = vec2(0.5, 0.5) + direction * (length * 0.7);
    vec2 end_point = vec2(0.5, 0.5) + direction * (length * 1.1);
    
    float dist = sd_arrow(uv, start_point, end_point, 0.125, arrow_thickness, arrow_sc);
    float delta = fwidth(dist);
    
    float alpha_ol = smoothstep((arrow_thickness * 2.0) - delta, (arrow_thickness * 2.0) + delta, dist);
	float alpha = smoothstep(0.0 - delta, 0.0 + delta, dist);
    
    vec4 fill_colour = vec4(1.0, 1.0, 1.0, colour.a);
    vec4 out_colour = vec4(0.0);
    
    // Apply outline
	out_colour.rgb = mix(fill_colour.rgb, colour.rgb * outline_brightness, alpha);
	out_colour.a = fill_colour.a * (1.0 - alpha_ol);
    
    // Apply drop shadow
    float alpha_ds = smoothstep((arrow_thickness * 1.5) - (shadow_smoothing * 0.5), (arrow_thickness * 1.5) + (shadow_smoothing * 0.5), dist);
	vec4 shadow = vec4(vec3(0.0), (1.0 - alpha_ds) * 0.5);
	out_colour = mix(shadow, out_colour, out_colour.a);
    
    return out_colour;
}
#endif // ARROW


void main()
{
	// Base colour
	vec4 base_colour = colour + (vec4(vec3(-vertex_in.position.y), 0.0) * 0.2);
	vec4 col_tex = texture(tex_unit, vertex_in.tex_coord);
#ifdef ICON
	base_colour.rgb *= col_tex.rgb;
#endif
#ifdef SPRITE
	base_colour.rgb = mix(base_colour.rgb, col_tex.rgb, col_tex.a);
#endif

	// Apply shape
	f_colour = shape(vertex_in.position, base_colour);

#ifdef ARROW
	// Apply direction arrow
	vec4 c_arrow = arrow(vertex_in.position, vertex_in.direction, 0.55);
	f_colour = mix(f_colour, c_arrow, c_arrow.a);
#endif

	f_colour.a *= vertex_in.alpha;
}
