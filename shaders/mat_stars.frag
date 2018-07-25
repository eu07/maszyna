#version 330

#include <common>

flat in vec3 f_normal_raw;

void main()
{
	float x = (gl_PointCoord.x - 0.5f) * 2.0f;
	float y = (gl_PointCoord.y - 0.5f) * 2.0f;
	float dist2 = abs(x * x + y * y);
	if (dist2 > 0.5f * 0.5f)
		discard;

	// color data is shared with normals, ugh
	gl_FragData[0] = vec4(f_normal_raw.bgr, 1.0f);
#ifdef MOTIONBLUR_ENABLED
	gl_FragData[1] = vec4(0.0f);
#endif
}
