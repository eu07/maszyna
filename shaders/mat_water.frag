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
#param (reflection, 1, 2, 1, one)
#param (wave_strength, 2, 1, 1, one)
#param (wave_speed, 2, 2, 1, one)

#texture (normalmap, 0, RGBA)
uniform sampler2D normalmap;

#texture (dudvmap, 1, RGB)
uniform sampler2D dudvmap;

#texture (diffuse, 2, sRGB_A)
uniform sampler2D diffuse;

#if SHADOWMAP_ENABLED
uniform sampler2DShadow shadowmap;
#endif
#if ENVMAP_ENABLED
uniform samplerCube envmap;
#endif

//wave distortion variables
float move_factor = 0;
vec3 normal_d;

#define WATER
#include <light_common.glsl>
#include <tonemapping.glsl>

void main()
{
//wave distortion
	move_factor += (param[2].z * time);
	move_factor = mod(move_factor, 1);
	vec2 texture_coords = f_coord;
	vec2 distorted_tex_coord = texture(dudvmap, vec2(texture_coords.x + move_factor, texture_coords.y)).rg * 0.1;
	distorted_tex_coord = texture_coords + vec2(distorted_tex_coord.x , distorted_tex_coord.y + move_factor);
	vec2 total_distorted_tex_coord = (texture(dudvmap, distorted_tex_coord).rg * 2.0 - 1.0 ) * param[2].y;
	texture_coords += total_distorted_tex_coord;

	normal_d = f_tbn * normalize(texture(normalmap, texture_coords).rgb * 2.0 - 1.0);
	vec3 refvec = reflect(f_pos.xyz, normal_d);
#if ENVMAP_ENABLED
	vec3 envcolor = texture(envmap, refvec).rgb;
#else
	vec3 envcolor = vec3(0.5);
#endif
	vec4 tex_color = texture(diffuse, texture_coords);
//Fresnel effect
	vec3 view_dir = normalize(vec3(0.0f, 0.0f, 0.0f) - f_pos.xyz);
	float fresnel = pow ( dot (f_normal, view_dir), 0.2 );
	float fresnel_inv = ((fresnel - 1.0 ) * -1.0 );
	
	vec3 result = ambient * 0.5 + param[0].rgb * emission;

	if (lights_count > 0U)
	{
		vec2 part = calc_dir_light(lights[0]);
		vec3 c = (part.x * param[1].x + part.y * param[1].y) * calc_shadow() * lights[0].color;
			result += mix(c, envcolor, param[1].z * texture(normalmap, texture_coords ).a);
	}
	//result = mix(result, param[0].rgb, param[1].z);
	result = (result * tex_color.rgb * param[1].z) + (result * (1.0 - param[1].z)); //multiply
	//result = ( max(result + param[0].rgb -1.0,0.0) * param[1].z + result * (1.0 - param[1].z)); //linear burn
	//result = ( min(param[0].rgb,result) * param[1].z + result * (1.0 - param[1].z)); //darken
	
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

	vec4 color = vec4(apply_fog(clamp(result,0.0,1.0)), clamp((fresnel_inv + param[0].a),0.0,1.0));

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
