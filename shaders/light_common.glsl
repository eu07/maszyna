#if SHADOWMAP_ENABLED
in vec4 f_light_pos[MAX_CASCADES];
uniform sampler2DArrayShadow shadowmap;
#endif
uniform sampler2D headlightmap;

#include <envmapping.glsl>
#include <conversion.glsl>

float glossiness = 1.0;
float metalic = 0.0;

float length2(vec3 v)
{
        return dot(v, v);
}

float calc_shadow()
{
#if SHADOWMAP_ENABLED
	float distance = dot(f_pos.xyz, f_pos.xyz);
	uint cascade;
	for (cascade = 0U; cascade < MAX_CASCADES; cascade++)
		if (distance <= cascade_end[cascade])
			break;

	vec3 coords = f_light_pos[cascade].xyz / f_light_pos[cascade].w;
	if (coords.z < 0.0f)
		return 0.0f;

	float shadow = 0.0;
	//basic
//	shadow = texture(shadowmap, coords.xyz + vec3(0.0, 0.0, bias));
	//PCF
	float bias = 0.00005f * float(cascade + 1U);
	vec2 texel = vec2(1.0) / vec2(textureSize(shadowmap, 0));
	float radius = 1.0;
	for (float y = -1.5; y <= 1.5; y += 1.0)
		for (float x = -1.5; x <= 1.5; x += 1.0)
			shadow += texture( shadowmap, vec4(coords.xy + vec2(x, y) * radius * texel, cascade, coords.z + bias) );
	shadow /= 16.0;

	return shadow;
#else
	return 0.0;
#endif
}

vec2 calc_light(vec3 light_dir, vec3 fragnormal)
{
	vec3 view_dir = normalize(vec3(0.0f, 0.0f, 0.0f) - f_pos.xyz);
	vec3 halfway_dir = normalize(light_dir + view_dir);

	float diffuse_v = max(dot(fragnormal, light_dir), 0.0);
	float specular_v = pow(max(dot(fragnormal, halfway_dir), 0.0), max(glossiness, 0.01)) * diffuse_v;

	return vec2(diffuse_v, specular_v);
}

vec2 calc_point_light(light_s light, vec3 fragnormal)
{
	vec3 light_dir = normalize(light.pos - f_pos.xyz);
	vec2 val = calc_light(light_dir, fragnormal);
	val.x += light.ambient;
	val *= light.intensity;
	
	float distance = length(light.pos - f_pos.xyz);
	float atten = 1.0f / (1.0f + light.linear * distance + light.quadratic * (distance * distance));
	
	return val * atten;
}

vec2 calc_spot_light(light_s light, vec3 fragnormal)
{
	vec3 light_dir = normalize(light.pos - f_pos.xyz);
	
	float theta = dot(light_dir, normalize(-light.dir));
	float epsilon = light.in_cutoff - light.out_cutoff;
	float intensity = clamp((theta - light.out_cutoff) / epsilon, 0.0, 1.0);

	vec2 point = calc_point_light(light, fragnormal);	
	return point * intensity;
}

vec2 calc_dir_light(light_s light, vec3 fragnormal)
{
	vec3 light_dir = normalize(-light.dir);
	return calc_light(light_dir, fragnormal);
}

vec2 calc_headlights(light_s light, vec3 fragnormal)
{
	vec4 headlightpos = light.headlight_projection * f_pos;
	vec3 coords = headlightpos.xyz / headlightpos.w;

	if (coords.z > 1.0)
		return vec2(0.0);
	if (coords.z < 0.0)
		return vec2(0.0);

	vec3 light_dir = normalize(light.pos - f_pos.xyz);
	vec2 part = vec2(1.0) * clamp(dot(fragnormal, light_dir) + 0.25, 0.0, 1.0);
	float distance = length(light.pos - f_pos.xyz);
	float atten = 1.0f / (1.0f + light.linear * distance + light.quadratic * (distance * distance));
	atten *= mix(1.0, 0.0, clamp((coords.z - 0.998) * 500.0, 0.0, 1.0));
	vec3 lights = textureProj(headlightmap, headlightpos).rgb * light.headlight_weights.rgb;
	float lightintensity = max(max(lights.r, lights.g), lights.b);
	return part * atten * lightintensity;
}

// [0] - diffuse, [1] - specular
// do magic here
vec3 apply_lights(vec3 fragcolor, vec3 fragnormal, vec3 texturecolor, float reflectivity, float specularity, float shadowtone)
{
	vec3 basecolor = param[0].rgb;

	fragcolor *= basecolor;

	vec3 emissioncolor = basecolor * emission;
	vec3 envcolor = envmap_color(fragnormal);

// yuv path
	vec3 texturecoloryuv = rgb2yuv(texturecolor);
	vec3 texturecolorfullv = yuv2rgb(vec3(0.2176, texturecoloryuv.gb));
// hsl path
//	vec3 texturecolorhsl = rgb2hsl(texturecolor);
//	vec3 texturecolorfullv = hsl2rgb(vec3(texturecolorhsl.rg, 0.5));

	vec3 envyuv = rgb2yuv(envcolor);
	texturecolor = mix(texturecolor, texturecolorfullv, envyuv.r * reflectivity);

	if(lights_count == 0U) 
		return (fragcolor + emissioncolor + envcolor * reflectivity) * texturecolor;

//	fragcolor *= lights[0].intensity;
	vec2 sunlight = calc_dir_light(lights[0], fragnormal);

	float diffuseamount = (sunlight.x * param[1].x) * lights[0].intensity;
//	fragcolor += mix(lights[0].color * diffuseamount, envcolor, reflectivity);
	fragcolor += lights[0].color * diffuseamount;
	fragcolor = mix(fragcolor, envcolor, reflectivity);

	for (uint i = 1U; i < lights_count; i++)
	{
		light_s light = lights[i];
		vec2 part = vec2(0.0);

//		if (light.type == LIGHT_SPOT)
//			part = calc_spot_light(light, fragnormal);
//		else if (light.type == LIGHT_POINT)
//			part = calc_point_light(light, fragnormal);
//		else if (light.type == LIGHT_DIR)
//			part = calc_dir_light(light, fragnormal);
//		else if (light.type == LIGHT_HEADLIGHTS)
			part = calc_headlights(light, fragnormal);

		fragcolor += light.color * (part.x * param[1].x + part.y * param[1].y) * light.intensity;
	}

	float specularamount = (sunlight.y * param[1].y * specularity) * lights[0].intensity * clamp(1.0 - shadowtone, 0.0, 1.0);
	if (shadowtone < 1.0)
	{
		float shadow = calc_shadow();
		specularamount *= clamp(1.0 - shadow, 0.0, 1.0);
		fragcolor = mix(fragcolor,  fragcolor * shadowtone,  clamp(diffuseamount * shadow + specularamount, 0.0, 1.0));
	}
	fragcolor += emissioncolor;
	vec3 specularcolor = specularamount * lights[0].color;

	if (param[1].w < 0.0)
		{
		float metalic = 1.0;
		}
	fragcolor = mix(((fragcolor + specularcolor) * texturecolor),(fragcolor * texturecolor + specularcolor),metalic) ;

	return fragcolor;
}
