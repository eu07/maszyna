#version 330 core
out vec4 FragColor;
  
in vec2 f_coords;

uniform sampler2D tex1;

vec3 reinhard(vec3 x)
{
	return x / (x + vec3(1.0));
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return (x*(a*x+b))/(x*(c*x+d)+e);
}

// https://www.slideshare.net/ozlael/hable-john-uncharted2-hdr-lighting
vec3 filmicF(vec3 x)
{
	float A = 0.22f;
	float B = 0.30f;
	float C = 0.10f;
	float D = 0.20f;
	float E = 0.01f;
	float F = 0.30f;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F)) - E/F;
}

vec3 filmic(vec3 x)
{
	return filmicF(x) / filmicF(vec3(11.2f));
}

void main()
{
	vec2 texcoord = f_coords;
//	float x = texcoord.x;
//	texcoord.x += sin(texcoord.y * 4*2*3.14159 + 0) / 100;
//	texcoord.y += sin(x * 4*2*3.14159 + 0) / 100;
	vec3 hdr_color = texture(tex1, texcoord).xyz;

	vec3 mapped;
	//if (texcoord.x < 0.33f)
	//	mapped = reinhard(hdr_color);
	//else if (texcoord.x < 0.66f)
	//	mapped = filmic(hdr_color);
	//else
		mapped = ACESFilm(hdr_color);
	gl_FragColor = vec4(mapped, 1.0);
}  
