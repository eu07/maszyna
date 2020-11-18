#include <common>

layout(location = 0) out vec4 out_color;
#if MOTIONBLUR_ENABLED
layout(location = 1) out vec4 out_motion;
#endif

void main()
{
out_color = vec4(vec3(0.0f), 1.0f);
#if MOTIONBLUR_ENABLED
	out_motion = vec4(0.0f);
#endif
gl_FragDepth = 1.0f;
}
