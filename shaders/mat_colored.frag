in vec3 f_normal;
in vec2 f_coord;
in vec4 f_pos;

in vec4 f_clip_pos;
in vec4 f_clip_future_pos;

#include <common>

#param (color, 0, 0, 4, diffuse)
#param (diffuse, 1, 0, 1, diffuse)
#param (specular, 1, 1, 1, specular)
#param (reflection, 1, 2, 1, zero)
#param (glossiness, 1, 3, 1, glossiness)

layout(location = 0) out vec4 out_color;
#if MOTIONBLUR_ENABLED
layout(location = 1) out vec4 out_motion;
#endif

#include <light_common.glsl>
#include <apply_fog.glsl>
#include <tonemapping.glsl>

void main()
{
	vec4 tex_color = vec4(pow(param[0].rgb, vec3(2.2)), param[0].a);

//	if (tex_color.a < opacity)
//		discard;

	vec3 fragcolor = ambient;
	vec3 fragnormal = normalize(f_normal);
	float reflectivity = param[1].z;
	float specularity = (tex_color.r + tex_color.g + tex_color.b) * 0.5;
	glossiness = abs(param[1].w);
	
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
