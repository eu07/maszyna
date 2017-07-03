#version 330

const uint LIGHT_SPOT = 0U;
const uint LIGHT_POINT = 1U;
const uint LIGHT_DIR = 2U;

struct light_s
{
	uint type;

	vec3 pos;
	vec3 dir;
	
	float in_cutoff;
	float out_cutoff;

	vec3 color;
	
	float linear;
	float quadratic;
};

in vec3 f_normal;
in vec2 f_coord;
in vec3 f_pos;
in vec4 f_light_pos;

out vec4 color;

uniform sampler2D tex;
uniform sampler2DShadow shadowmap;

uniform vec3 ambient;
uniform vec3 emission;
uniform vec3 fog_color;
uniform float fog_density;
uniform float specular;

uniform light_s lights[8];
uniform uint lights_count;

vec2 poissonDisk[16] = vec2[]( 
   vec2( -0.94201624, -0.39906216 ), 
   vec2( 0.94558609, -0.76890725 ), 
   vec2( -0.094184101, -0.92938870 ), 
   vec2( 0.34495938, 0.29387760 ), 
   vec2( -0.91588581, 0.45771432 ), 
   vec2( -0.81544232, -0.87912464 ), 
   vec2( -0.38277543, 0.27676845 ), 
   vec2( 0.97484398, 0.75648379 ), 
   vec2( 0.44323325, -0.97511554 ), 
   vec2( 0.53742981, -0.47373420 ), 
   vec2( -0.26496911, -0.41893023 ), 
   vec2( 0.79197514, 0.19090188 ), 
   vec2( -0.24188840, 0.99706507 ), 
   vec2( -0.81409955, 0.91437590 ), 
   vec2( 0.19984126, 0.78641367 ), 
   vec2( 0.14383161, -0.14100790 ) 
);

float random(vec3 seed, int i)
{
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

float calc_shadow()
{
	vec3 coords = f_light_pos.xyz;// / f_light_pos.w;
	coords = coords * 0.5 + 0.5;
	float bias = 0.005;

	//PCF
	//float shadow = texture(shadowmap, vec3(coords.xy, coords.z - bias));
	
	//PCF + stratified poisson sampling
	//float shadow = 1.0;
    //for (int i=0;i<4;i++)
	//{
	//	int index = int(16.0*random(gl_FragCoord.xyy, i))%16;
	//	shadow -= 0.25*(1.0-texture(shadowmap, vec3(coords.xy + poissonDisk[index]/5000.0, coords.z - bias)));
    //}

	//PCF + PCF
	float shadow = 0.0;
	vec2 texel = 1.0 / textureSize(shadowmap, 0);
	for (float y = -1.5; y <= 1.5; y += 1.0)
		for (float x = -1.5; x <= 1.5; x += 1.0)
			shadow += texture(shadowmap, coords.xyz + vec3(vec2(x, y) * texel, -bias));
	shadow /= 16.0;
	
	return shadow;
}

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
	vec3 normal = normalize(f_normal);
	vec3 view_dir = normalize(vec3(0.0f, 0.0f, 0.0f) - f_pos);
	vec3 halfway_dir = normalize(light_dir + view_dir);
	
	float diffuse_v = max(dot(normal, light_dir), 0.0);
	float specular_v = pow(max(dot(normal, halfway_dir), 0.0), 15.0) * specular;
	
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
	float shadow = calc_shadow();
	vec3 result = ambient * 0.3 + emission;
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
		
		result += light.color * part * shadow;
	}
	
	vec4 tex_color = texture(tex, f_coord);	
	color = vec4(apply_fog(result * tex_color.xyz), tex_color.w);
}