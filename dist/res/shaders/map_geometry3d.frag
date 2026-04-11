in VertexData
{
	vec2  tex_coord;
	float brightness;
	vec3  normal;
	vec3  view_pos;
	vec3  world_pos;
	vec4  colour;
} vertex_in;

out vec4 f_colour;


// Uniforms/Variables ----------------------------------------------------------

uniform vec4  colour      = vec4(1.0);
uniform vec3  fog_colour  = vec3(0.0);
uniform float fog_density = 1.0;
uniform float max_dist    = 40000.0;

#ifdef ALPHA_TEST
uniform float alpha_threshold = 0.0;
#endif

#ifdef CIRCLE_MASK
const float cm_circle_size       = 0.499;
const float cm_border_width      = 0.065;
const float cm_border_brightness = 0.35;
#endif

uniform sampler2D tex_unit;


// Point lights ----------------------------------------------------------------

#ifndef MAX_POINT_LIGHTS
#define MAX_POINT_LIGHTS 32
#endif

struct PointLight
{
	vec3  position;
	vec3  colour;
	float radius;
	int   type; // 0 = normal, 1 = attenuated, 2 = additive, 3 = subtractive
};
uniform PointLight point_lights[MAX_POINT_LIGHTS];
uniform int num_point_lights = 0;
uniform float point_light_intensity = 1.0;

void applyPointLights(inout vec3 light, inout vec3 additive_light)
{
	for (int i = 0; i < num_point_lights; ++i)
	{
		PointLight pl = point_lights[i];

		vec3 to_light = pl.position - vertex_in.world_pos;

		// Quick distance check to avoid sqrt below when not needed
		float dist_sq = dot(to_light, to_light);
		if (dist_sq > pl.radius * pl.radius)
			continue;

		// Find distance to light and intensity (1.0 at centre, 0.0 at radius)
		float dist = sqrt(dist_sq);
		float t = 1.0 - (dist / pl.radius);

#ifdef SPRITE
		// Don't attenuate on sprites
		// (they are billboarded so we don't have a normal)
		vec3 lc = pl.colour * t * point_light_intensity;
		if (pl.type == 2)
			additive_light += lc * 0.2;
		else if (pl.type == 3)
			light -= lc;
		else
			light += lc;
#else
		// Calculate diffuse factor for attenuated light type and surface check
		float nl = dot(vertex_in.normal, to_light);

		// Reject if light is clearly behind the surface
		if (nl < -0.0001)
			continue;

		// Calculate final intensity (with attenuation if type 1) and colour
		float intensity = (pl.type == 1) ? t * max(nl, 0.0) / dist : t;
		vec3 lc = pl.colour * intensity * point_light_intensity;

		if (pl.type == 2)
		{
			// Additive lights are added separately after the texture is applied
			additive_light += lc * 0.2; // scaled by 0.2 to match *ZDoom
		}
		else
		{
			if (pl.type == 3)
				light -= lc; // Subtractive
			else
				light += lc; // Normal/attenuated
		}
#endif
	}
}


// Depth fog -------------------------------------------------------------------

float fogFactor(float brightness)
{
	float inv_brightness = 1.0 - brightness;

	float depth   = length(vertex_in.view_pos);
	float density = inv_brightness * fog_density;
	float factor  = exp(-(density * density * density) * depth * 0.01);
	
	return clamp(factor, brightness * brightness, 1.0);
}


// Main ------------------------------------------------------------------------

void main()
{
	vec4 tex_colour = texture(tex_unit, vertex_in.tex_coord);
	float frag_alpha = tex_colour.a * vertex_in.colour.a * colour.a;

#ifdef ALPHA_TEST
	if (frag_alpha <= alpha_threshold) discard;
#endif

	// First up, calculate the base lighting for the fragment (no texture yet)
	vec3 light = vertex_in.colour.rgb * colour.rgb * vertex_in.brightness;

	// Apply point lights (additive lights separately)
	vec3 additive_light = vec3(0.0);
	if (num_point_lights > 0)
		applyPointLights(light, additive_light);

	// Determine overall brightness of the fragment (excluding texture) so that
	// brighter areas (including point lights) reduce fog density
	float brightness = clamp(max(vertex_in.brightness, dot(light, vec3(0.2126, 0.7152, 0.0722))), 0.0, 1.0);

	// Clamp light ('colour-correct', based on *ZDoom's implementation)
	float light_maxc    = max(max(light.r, light.g), light.b);
	vec3  clamped_light = max(light / max(light_maxc, 1.4) * 1.4, 0.0);

	// Apply fog and texture
	vec3 frag_rgb = mix(fog_colour, tex_colour.rgb * clamped_light, fogFactor(brightness));

	// Add additive lighting on top
	frag_rgb = clamp(frag_rgb + additive_light, 0.0, 1.0);

#ifdef CIRCLE_MASK
	// Mask to circle with darkened border (similar to 2d mode things), for thing icons
	float cm_dist         = distance(vertex_in.tex_coord, vec2(0.5));
	float cm_delta        = fwidth(cm_dist);
	float cm_alpha_border = smoothstep(cm_circle_size - cm_border_width - cm_delta,
									   cm_circle_size - cm_border_width + cm_delta,
									   cm_dist);
	float cm_alpha_edge   = smoothstep(cm_circle_size - cm_delta, cm_circle_size, cm_dist);
	frag_rgb    = mix(frag_rgb, frag_rgb * cm_border_brightness, cm_alpha_border);
	frag_alpha *= (1.0 - cm_alpha_edge);

	if (frag_alpha <= 0.0) discard;
#endif

	f_colour = vec4(frag_rgb, frag_alpha);

	// Apply alpha fade at 128 units from max distance
	float xy_dist = length(vertex_in.view_pos);
	f_colour.a   *= clamp((max_dist - xy_dist) / 128.0, 0.0, 1.0);
}
