in vec3 f_normal;
in vec2 f_coord;
in vec4 f_pos;

in vec4 f_clip_pos;
in vec4 f_clip_future_pos;

#include <common>

#param (diffuse, 1, 0, 1, diffuse)

#texture (diffuse, 0, sRGB_A)
uniform sampler2D diffuse;

layout(location = 0) out vec4 out_color;
#if MOTIONBLUR_ENABLED
layout(location = 1) out vec4 out_motion;
#endif

#include <apply_fog.glsl>
#include <tonemapping.glsl>

vec3 apply_lights_clouds(vec3 fragcolor, vec3 fragnormal, vec3 texturecolor)
{
	if(lights_count == 0U) 
		return fragcolor * texturecolor;

	vec3 light_dir = normalize(-lights[0].dir);
	vec3 view_dir = normalize(vec3(0.0) - f_pos.xyz);
	vec3 halfway_dir = normalize(light_dir + view_dir);
	float diffuse_v = max(dot(fragnormal, light_dir), 0.0);
	float diffuseamount = (diffuse_v * param[1].x) * lights[0].intensity;
	fragcolor += lights[0].color * diffuseamount;
	fragcolor *= texturecolor;

	return fragcolor;
}

void main()
{
	vec4 tex_color = texture(diffuse, f_coord);
	tex_color.a = clamp(tex_color.a * 1.25, 0.0, 1.0);
	if(tex_color.a < 0.01)
		discard;

	vec3 fragcolor = param[0].rgb;
	vec3 fragnormal = normalize(f_normal);
	
	fragcolor = apply_lights_clouds(fragcolor, fragnormal, tex_color.rgb);
	vec4 color = vec4(apply_fog(fragcolor), tex_color.a);

#if POSTFX_ENABLED
    out_color = color;
#else
    out_color = tonemap(color);
#endif

#if MOTIONBLUR_ENABLED
	out_motion = vec4(0.0f);
#endif
}
