in vec3 f_normal;
in vec2 f_coord;
in vec4 f_pos;
in mat3 f_tbn;
in vec4 f_light_pos;

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
#param (reflection, 1, 2, 1, zero)

#texture (diffuse, 0, sRGB_A)
uniform sampler2D diffuse;

#texture (normalmap, 1, RGBA)
uniform sampler2D normalmap;

#if SHADOWMAP_ENABLED
uniform sampler2DShadow shadowmap;
#endif

#if ENVMAP_ENABLED
uniform samplerCube envmap;
#endif

#define NORMALMAP
#include <light_common.glsl>
#include <tonemapping.glsl>

void main()
{
	vec4 tex_color = texture(diffuse, f_coord);

	if (tex_color.a < opacity)
		discard;

	vec3 normal = normalize(f_tbn * normalize(texture(normalmap, f_coord).rgb * 2.0 - 1.0));
	vec3 refvec = reflect(f_pos.xyz, normal);
#if ENVMAP_ENABLED
	vec3 envcolor = texture(envmap, refvec).rgb;
#else
    vec3 envcolor = vec3(0.5);
#endif

	vec3 result = ambient * 0.5 + param[0].rgb * emission;

	if (lights_count > 0U)
	{
		vec2 part = calc_dir_light(lights[0]);
		vec3 c = (part.x * param[1].x + part.y * param[1].y) * calc_shadow() * lights[0].color;
		result += mix(c, envcolor, param[1].z * texture(normalmap, f_coord).a);
	}

	for (uint i = 1U; i < lights_count; i++)
	{
		light_s light = lights[i];
		vec2 part = vec2(0.0);

		if (light.type == LIGHT_SPOT)
			part = calc_spot_light(light);
		else if (light.type == LIGHT_POINT)
			part = calc_point_light(light);
		else if (light.type == LIGHT_DIR)
			part = calc_dir_light(light);

		result += light.color * (part.x * param[1].x + part.y * param[1].y);
	}

	vec4 color = vec4(apply_fog(result * tex_color.rgb), tex_color.a * alpha_mult);
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
