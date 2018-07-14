#version 330

#include <common>

void main()
{
	float x = (gl_PointCoord.x - 0.5f) * 2.0f;
	float y = (gl_PointCoord.y - 0.5f) * 2.0f;
	float dist2 = abs(x * x + y * y);
	if (dist2 > 0.5f * 0.5f)
		discard;
	gl_FragColor = vec4(param[0].rgb * emission, 1.0f);
}
