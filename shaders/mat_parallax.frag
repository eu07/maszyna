in vec3 f_normal;
in vec2 f_coord;
in vec4 f_pos;
in mat3 f_tbn; //tangent matrix nietransponowany; mnożyć przez f_tbn dla TangentLightPos; TangentViewPos; TangentFragPos;
in vec4 f_clip_pos;
in vec4 f_clip_future_pos;
in vec3 TangentFragPos;

#include <common>

layout(location = 0) out vec4 out_color;
#if MOTIONBLUR_ENABLED
layout(location = 1) out vec4 out_motion;
#endif

#param (color, 0, 0, 4, diffuse)
#param (diffuse, 1, 0, 1, diffuse)
#param (specular, 1, 1, 1, specular)
#param (reflection, 1, 2, 1, one)
#param (glossiness, 1, 3, 1, glossiness)
#param (height_scale, 2, 1, 1, zero)
#param (height_offset, 2, 2, 1, zero)

#texture (diffuse, 0, sRGB_A)
uniform sampler2D diffuse;

#texture (normalmap, 1, RGBA)
uniform sampler2D normalmap;

#define PARALLAX
#include <light_common.glsl>
#include <apply_fog.glsl>
#include <tonemapping.glsl>

vec2 ParallaxMapping(vec2 f_coord, vec3 viewDir);

void main()
{
	//parallax mapping
	vec3 viewDir = normalize(vec3(0.0f, 0.0f, 0.0f) - TangentFragPos); //tangent view pos - tangent frag pos
	vec2 f_coord_p = ParallaxMapping(f_coord, viewDir);

	vec4 tex_color = texture(diffuse, f_coord_p);

	bool alphatestfail = ( opacity >= 0.0 ? (tex_color.a < opacity) : (tex_color.a >= -opacity) );
	if(alphatestfail)
		discard;
//	if (tex_color.a < opacity)
//		discard;

	vec3 fragcolor = ambient;

	vec3 normal;
	normal.xy = (texture(normalmap, f_coord_p).rg * 2.0 - 1.0);
	normal.z = sqrt(1.0 - clamp((dot(normal.xy, normal.xy)), 0.0, 1.0));
	vec3 fragnormal = normalize(f_tbn * normalize(normal.xyz));
	float reflectivity = param[1].z * texture(normalmap, f_coord_p).a;
	float specularity = (tex_color.r + tex_color.g + tex_color.b) * 0.5;	
	
	fragcolor = apply_lights(fragcolor, fragnormal, tex_color.rgb, reflectivity, specularity, shadow_tone);

	vec4 color = vec4(apply_fog(fragcolor), tex_color.a * alpha_mult);
#if POSTFX_ENABLED
	out_color = color;
#else
	out_color = tonemap(color);
#endif
#if MOTIONBLUR_ENABLED
	{
		vec2 a = (f_clip_future_pos.xy / f_clip_future_pos.w) * 0.5 + 0.5;;
		vec2 b = (f_clip_pos.xy / f_clip_pos.w) * 0.5 + 0.5;;

		out_motion = vec4(a - b, 0.0f, 0.0f);
	}
#endif
}

vec2 ParallaxMapping(vec2 f_coord, vec3 viewDir)
{
	float pos_len = length(f_pos.xyz);

	if (pos_len > 100.0) {
		return f_coord;
	}

#if EXTRAEFFECTS_ENABLED
	const float minLayers = 8.0;
	const float maxLayers = 32.0;
	float LayersWeight = pos_len / 20.0;
	vec2  currentTexCoords = f_coord;
	float currentDepthMapValue = texture(normalmap, currentTexCoords).b;
	LayersWeight = min(abs(dot(vec3(0.0, 0.0, 1.0), viewDir)),LayersWeight);
	float numLayers = mix(maxLayers, minLayers, clamp(LayersWeight, 0.0, 1.0)); // number of depth layers
	float layerDepth = 1.0 / numLayers; // calculate the size of each layer
	float currentLayerDepth = 0.0; // depth of current layer
	vec2 P = viewDir.xy * param[2].y; // the amount to shift the texture coordinates per layer (from vector P)
	vec2 deltaTexCoords = P / numLayers;

	  
	while(currentLayerDepth < currentDepthMapValue)
	{
		currentTexCoords -= deltaTexCoords;		// shift texture coordinates along direction of P
		currentDepthMapValue = texture(normalmap, currentTexCoords).b;		// get depthmap value at current texture coordinates
		currentLayerDepth += layerDepth;  		// get depth of next layer
	}
	
	vec2 prevTexCoords = currentTexCoords + deltaTexCoords; // get texture coordinates before collision (reverse operations)

	float afterDepth  = currentDepthMapValue - currentLayerDepth; // get depth after and before collision for linear interpolation
	float beforeDepth = texture(normalmap, prevTexCoords).b - currentLayerDepth + layerDepth;
	 
	float weight = afterDepth / (afterDepth - beforeDepth); // interpolation of texture coordinates
	vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

	return finalTexCoords;
#else
	float height = texture(normalmap, f_coord).b;
	vec2 p = viewDir.xy / viewDir.z * (height * (param[2].y - param[2].z) * 0.2);
	return f_coord - p;
#endif
} 
