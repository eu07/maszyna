#version 330

in vec3 f_normal;
in vec2 f_coord;
in vec3 f_pos;
in mat3 f_tbn;
in vec4 f_tangent;

#texture (tex1, 0, sRGB_A)
uniform sampler2D tex1;

#texture (tex2, 1, RGB)
uniform sampler2D tex2;

#include <common>

vec3 apply_fog(vec3 color)
{
	float sun_amount = 0.0;
	if (lights_count >= 1U && lights[0].type == LIGHT_DIR)
		sun_amount = max(dot(normalize(f_pos), normalize(-lights[0].dir)), 0.0);
	vec3 fog_color_v = mix(fog_color, lights[0].color, pow(sun_amount, 30.0));
	float fog_amount_v = 1.0 - min(1.0, exp(-length(f_pos) * fog_density));
	return mix(color, fog_color_v, fog_amount_v);
}

float calc_light(vec3 light_dir)
{
	//vec3 normal = normalize(f_normal);
	vec3 normal = f_tbn * normalize(texture(tex2, f_coord).rgb * 2.0 - 1.0);
	vec3 view_dir = normalize(vec3(0.0f, 0.0f, 0.0f) - f_pos);
	vec3 halfway_dir = normalize(light_dir + view_dir);
	
	float diffuse_v = max(dot(normal, light_dir), 0.0);
	float specular_v = pow(max(dot(normal, halfway_dir), 0.0), 15.0) * 0.2;
	
	return specular_v + diffuse_v;
}

float calc_point_light(light_s light)
{
	vec3 light_dir = normalize(light.pos - f_pos);
	float val = calc_light(light_dir);
	
	float distance = length(light.pos - f_pos);
	float atten = 1.0f / (1.0f + light.linear * distance + light.quadratic * (distance * distance));
	
	return val * atten;
}

float calc_spot_light(light_s light)
{
	vec3 light_dir = normalize(light.pos - f_pos);
	
	float theta = dot(light_dir, normalize(-light.dir));
	float epsilon = light.in_cutoff - light.out_cutoff;
	float intensity = clamp((theta - light.out_cutoff) / epsilon, 0.0, 1.0);

	float point = calc_point_light(light);	
	return point * intensity;
}

float calc_dir_light(light_s light)
{
	vec3 light_dir = normalize(-light.dir);
	return calc_light(light_dir);
}

void main()
{
	vec3 result = ambient * 0.3 + param[0].rgb * emission;
	for (uint i = 0U; i < lights_count; i++)
	{
		light_s light = lights[i];
		float part = 0.0;

		if (light.type == LIGHT_SPOT)
			part = calc_spot_light(light);
		else if (light.type == LIGHT_POINT)
			part = calc_point_light(light);
		else if (light.type == LIGHT_DIR)
			part = calc_dir_light(light);

		result += light.color * part;
	}


	vec4 tex_color = texture(tex1, f_coord);
	vec3 c = apply_fog(result * tex_color.xyz);
	gl_FragColor = vec4(c, tex_color.w);


//	gl_FragColor = vec4(f_tangent.xyz, 1.0f);
}
