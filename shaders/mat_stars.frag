#version 330

#include <common>
#include <tonemapping.glsl>

flat in vec3 f_normal_raw;

void main()
{
	float x = (gl_PointCoord.x - 0.5f) * 2.0f;
	float y = (gl_PointCoord.y - 0.5f) * 2.0f;
	float dist2 = abs(x * x + y * y);
	if (dist2 > 0.5f * 0.5f)
		discard;

	// color data space is shared with normals, ugh
	vec4 color = vec4(pow(f_normal_raw.bgr, vec3(2.2)), 1.0f);
#if POSTFX_ENABLED
    gl_FragData[0] = color;
#else
    gl_FragData[0] = tonemap(color);
#endif
#if MOTIONBLUR_ENABLED
	gl_FragData[1] = vec4(0.0f);
#endif
}
