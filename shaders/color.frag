in vec3 f_color;

#include <common>
#include <tonemapping.glsl>

layout(location = 0) out vec4 out_color;
#if MOTIONBLUR_ENABLED
layout(location = 1) out vec4 out_motion;
#endif

void main()
{
#if POSTFX_ENABLED
	out_color = vec4(f_color, 1.0f);
#else
    out_color = tonemap(vec4(f_color, 1.0f));
#endif
#if MOTIONBLUR_ENABLED
	out_motion = vec4(0.0f);
#endif
}
