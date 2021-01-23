in vec3 f_normal;
in vec2 f_coord;
in vec4 f_pos;
in mat3 f_tbn;

in vec4 f_clip_pos;
in vec4 f_clip_future_pos;

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
#param (detail_scale, 2, 0, 1, one)
#param (detail_height_scale, 2, 1, 1, one)

#texture (diffuse, 0, sRGB_A)
uniform sampler2D diffuse;

#texture (normalmap, 1, RGBA)
uniform sampler2D normalmap;

#texture (detailnormalmap, 2, RGBA)
uniform sampler2D detailnormalmap;

#texture (specgloss, 3, RGBA)
uniform sampler2D specgloss;

#define NORMALMAP
#include <light_common.glsl>
#include <apply_fog.glsl>
#include <tonemapping.glsl>

void main()
{
	vec4 tex_color = texture(diffuse, f_coord);

	bool alphatestfail = ( opacity >= 0.0 ? (tex_color.a < opacity) : (tex_color.a >= -opacity) );
	if(alphatestfail)
		discard;
//	if (tex_color.a < opacity)
//		discard;

	vec3 fragcolor = ambient;
	
	vec4 normal_map = texture(normalmap, f_coord);
	vec4 detailnormal_map = texture(detailnormalmap, f_coord * param[2].x);
	vec4 specgloss_map = texture(specgloss, f_coord);
	vec3 normal;
	vec3 normaldetail;
	
	normaldetail.xy = detailnormal_map.rg* 2.0 - 1.0;
	normaldetail.z = sqrt(1.0 - clamp((dot(normaldetail.xy, normaldetail.xy)), 0.0, 1.0));
	normaldetail.xyz = normaldetail.xyz * param[2].y;
	normal.xy =       normal_map.rg* 2.0 - 1.0;
	normal.z = sqrt(1.0 - clamp((dot(normal.xy, normal.xy)), 0.0, 1.0));
	
	vec3 fragnormal = normalize(f_tbn * normalize(vec3(normal.xy + normaldetail.xy, normal.z)));
	
	float reflblend = (normal_map.a + detailnormal_map.a);
	float reflectivity = reflblend > 1.0 ? param[1].z * 1.0 : param[1].z * reflblend;
	float specularity = specgloss_map.r;
	glossiness = specgloss_map.g * abs(param[1].w);
	float metalic = specgloss_map.b;
	
	
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
