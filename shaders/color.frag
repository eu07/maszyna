in vec3 f_color;
in vec4 f_pos;

#include <common>
#include <apply_fog.glsl>
#include <tonemapping.glsl>

layout(location = 0) out vec4 out_color;
#if MOTIONBLUR_ENABLED
layout(location = 1) out vec4 out_motion;
#endif

void main()
{
	vec3 col = pow(f_color.rgb, vec3(2.2));
#if POSTFX_ENABLED
	out_color = vec4(apply_fog(col), 1.0f);
#else
    out_color = tonemap(vec4(apply_fog(col), 1.0f));
#endif
#if MOTIONBLUR_ENABLED
	out_motion = vec4(0.0f);
#endif
}
