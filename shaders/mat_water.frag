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
#param (wave_strength, 2, 1, 1, one)
#param (wave_speed, 2, 2, 1, one)

#texture (normalmap, 0, RGBA)
uniform sampler2D normalmap;

#texture (dudvmap, 1, RGB)
uniform sampler2D dudvmap;

#texture (diffuse, 2, sRGB_A)
uniform sampler2D diffuse;

//wave distortion variables
float move_factor = 0.0;

#define WATER
#include <light_common.glsl>
#include <apply_fog.glsl>
#include <tonemapping.glsl>

void main()
{
	//wave distortion
	move_factor += (param[2].z * time);
	move_factor = mod(move_factor, 1.0);
	vec2 texture_coords = f_coord;
	vec2 distorted_tex_coord = texture(dudvmap, vec2(texture_coords.x + move_factor, texture_coords.y)).rg * 0.1;
	distorted_tex_coord = texture_coords + vec2(distorted_tex_coord.x , distorted_tex_coord.y + move_factor);
	vec2 total_distorted_tex_coord = (texture(dudvmap, distorted_tex_coord).rg * 2.0 - 1.0 ) * param[2].y;
	texture_coords += total_distorted_tex_coord;

	vec4 tex_color = texture(diffuse, texture_coords);

	vec3 fragcolor = ambient * 0.25;
	
	vec3 normal;
	normal.xy = (texture(normalmap, texture_coords).rg * 2.0 - 1.0);
	normal.z = sqrt(1.0 - clamp((dot(normal.xy, normal.xy)), 0.0, 1.0));
	vec3 fragnormal = normalize(f_tbn * normalize(normal.xyz));
	float reflectivity = param[1].z * texture(normalmap, texture_coords ).a;
	float specularity = 1.0;
	glossiness = abs(param[1].w);
	
	fragcolor = apply_lights(fragcolor, fragnormal, tex_color.rgb, reflectivity, specularity, shadow_tone);

	//fragcolor = mix(fragcolor, param[0].rgb, param[1].z);
//	fragcolor = (fragcolor * tex_color.rgb * param[1].z) + (fragcolor * (1.0 - param[1].z)); //multiply
	//fragcolor = ( max(fragcolor + param[0].rgb -1.0,0.0) * param[1].z + fragcolor * (1.0 - param[1].z)); //linear burn
	//fragcolor = ( min(param[0].rgb,fragcolor) * param[1].z + fragcolor * (1.0 - param[1].z)); //darken

	// fresnel effect
	vec3 view_dir = normalize(vec3(0.0f, 0.0f, 0.0f) - f_pos.xyz);
	float fresnel = pow ( dot (f_normal, view_dir), 0.2 );
	float fresnel_inv = ((fresnel - 1.0 ) * -1.0 );
	
	vec4 color = vec4(apply_fog(clamp(fragcolor, 0.0, 1.0)), clamp((fresnel_inv + param[0].a), 0.0, 1.0));
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
