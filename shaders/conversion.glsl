vec3 rgb2yuv(vec3 val)
{
	return
		clamp(
			vec3(
				 (0.257 * val.r) + (0.504 * val.g) + (0.098 * val.b) + 0.0625,
				-(0.148 * val.r) - (0.291 * val.g) + (0.439 * val.b) + 0.5,
				 (0.439 * val.r) - (0.368 * val.g) - (0.071 * val.b) + 0.5 ),
			0.0, 1.0);
}

vec3 yuv2rgb(vec3 val)
{
	val -= vec3(0.0625, 0.5, 0.5);
	return
		clamp(
			vec3(
				(1.164 * val.r) + (1.596 * val.b),
				(1.164 * val.r) - (0.391 * val.g) - (0.813 * val.b),
				(1.164 * val.r) + (2.018 * val.g) ),
			0.0, 1.0);
}

vec3 rgb2hsl( vec3 col )
{
	float red   = col.r;
	float green = col.g;
	float blue  = col.b;

	float minc  = min( col.r, min( col.g, col.b ));
	float maxc  = max( col.r, max( col.g, col.b ));
	float delta = maxc - minc;

	float lum = (minc + maxc) * 0.5;
	float sat = 0.0;
	float hue = 0.0;

	if (lum > 0.0 && lum < 1.0) {
		float mul = (lum < 0.5)  ?  (lum)  :  (1.0-lum);
		sat = delta / (mul * 2.0);
	}

	vec3 masks = vec3(
		(maxc == red   && maxc != green) ? 1.0 : 0.0,
		(maxc == green && maxc != blue)  ? 1.0 : 0.0,
		(maxc == blue  && maxc != red)   ? 1.0 : 0.0
	);

	vec3 adds = vec3(
		((green - blue ) / delta),
		2.0 + ((blue  - red  ) / delta),
		4.0 + ((red   - green) / delta)
	);

	float deltaGtz = (delta > 0.0) ? 1.0 : 0.0;

	hue += dot( adds, masks );
	hue *= deltaGtz;
	hue /= 6.0;

	if (hue < 0.0)
		hue += 1.0;

	return vec3( hue, sat, lum );
}

vec3 hsl2rgb( vec3 col )
{
	const float onethird = 1.0 / 3.0;
	const float twothird = 2.0 / 3.0;
	const float rcpsixth = 6.0;

	float hue = col.x;
	float sat = col.y;
	float lum = col.z;

	vec3 xt = vec3(
		rcpsixth * (hue - twothird),
		0.0,
		rcpsixth * (1.0 - hue)
	);

	if (hue < twothird) {
		xt.r = 0.0;
		xt.g = rcpsixth * (twothird - hue);
		xt.b = rcpsixth * (hue      - onethird);
	} 

	if (hue < onethird) {
		xt.r = rcpsixth * (onethird - hue);
		xt.g = rcpsixth * hue;
		xt.b = 0.0;
	}

	xt = min( xt, 1.0 );

	float sat2   =  2.0 * sat;
	float satinv =  1.0 - sat;
	float luminv =  1.0 - lum;
	float lum2m1 = (2.0 * lum) - 1.0;
	vec3  ct     = (sat2 * xt) + satinv;

	vec3 rgb;
	if (lum >= 0.5) { rgb = (luminv * ct) + lum2m1; }
	else            { rgb =  lum    * ct; }

	return rgb;
}
