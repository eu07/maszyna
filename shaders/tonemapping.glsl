const float pureWhite = 1.0;

vec3 reinhard(vec3 x)
{
//	return x / (x + vec3(1.0));

	// Reinhard tonemapping operator.
	// see: "Photographic Tone Reproduction for Digital Images", eq. 4
	float luminance = dot(x, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance = (luminance * (1.0 + luminance/(pureWhite*pureWhite))) / (1.0 + luminance);
	// Scale color by ratio of average luminances.
	return (mappedLuminance / luminance) * x;
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

vec4 tonemap(vec4 x)
{
//	return FBOUT(vec4(ACESFilm(x.rgb), x.a));
	return FBOUT(vec4(reinhard(x.rgb), x.a));
}
