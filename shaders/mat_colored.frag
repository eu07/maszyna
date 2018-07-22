#version 330

in vec3 f_normal;
in vec2 f_coord;
in vec4 f_pos;
in vec4 f_light_pos;

in vec4 f_clip_pos;
in vec4 f_clip_future_pos;

#include <common>

#param (color, 0, 0, 4, diffuse)
#param (diffuse, 1, 0, 1, diffuse)
#param (specular, 1, 1, 1, specular)
#param (reflection, 1, 2, 1, zero)

uniform sampler2DShadow shadowmap;
uniform samplerCube envmap;

#include <light_common.glsl>

void main()
{
	vec4 tex_color = vec4(param[0].rgb, 1.0f);

	vec3 normal = normalize(f_normal);
	vec3 refvec = reflect(f_pos.xyz, normal);
	vec3 envcolor = texture(envmap, refvec).rgb;

	vec3 result = ambient * 0.5 + param[0].rgb * emission;

	if (lights_count > 0U)
	{
		vec2 part = calc_dir_light(lights[0]);
		vec3 c = (part.x * param[1].x + part.y * param[1].y) * calc_shadow() * lights[0].color;
		result += mix(c, envcolor, param[1].z);
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

	gl_FragData[0] = vec4(apply_fog(result * tex_color.rgb), tex_color.a);
	{
        vec2 a = (f_clip_future_pos.xy / f_clip_future_pos.w) * 0.5 + 0.5;;
        vec2 b = (f_clip_pos.xy / f_clip_pos.w) * 0.5 + 0.5;;
        
        gl_FragData[1] = vec4(a - b, 0.0f, 0.0f);
	}
}
