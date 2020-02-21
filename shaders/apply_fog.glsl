vec3 get_fog_color()
{
	vec3 fog_color_v = fog_color;
	if (lights_count >= 1U && lights[0].type == LIGHT_DIR) {
		float sun_amount = max(dot(normalize(f_pos.xyz), normalize(-lights[0].dir)), 0.0);
		fog_color_v += lights[0].color * pow(sun_amount, 30.0);
	}
	return fog_color_v;
}

float get_fog_amount()
{
	float z = length(f_pos.xyz);
//	float fog_amount_v = 1.0 - z * fog_density; // linear
//	float fog_amount_v = exp( -fog_density * z ); // exp
	float fog_amount_v = exp2( -fog_density * fog_density * z * z * 1.442695 ); // exp2

	fog_amount_v = 1.0 - clamp(fog_amount_v, 0.0, 1.0);
	return fog_amount_v;
}

vec3 apply_fog(vec3 color)
{
	return mix(color, get_fog_color(), get_fog_amount());
}

vec3 add_fog(vec3 color, float amount)
{
	return color + get_fog_color() * get_fog_amount() * amount;
}
