#version 330

#include <common>

in vec3 f_normal;

void main()
{
	float x = (gl_PointCoord.x - 0.5f) * 2.0f;
	float y = (gl_PointCoord.y - 0.5f) * 2.0f;
	float dist2 = abs(x * x + y * y);
	if (dist2 > 0.5f * 0.5f)
		discard;

	// color data is shared with normals, ugh
	gl_FragColor = vec4(f_normal, 1.0f);
}
