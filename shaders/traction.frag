#include <common>

in vec4 f_pos;
in vec4 f_clip_pos;
in vec4 f_clip_future_pos;

layout(location = 0) out vec4 out_color;
#if MOTIONBLUR_ENABLED
layout(location = 1) out vec4 out_motion;
#endif

#include <tonemapping.glsl>
#include <apply_fog.glsl>

void main()
{
	vec4 color = vec4(apply_fog(pow(param[0].rgb, vec3(2.2))), param[0].a);
#if POSTFX_ENABLED
    out_color = color;
#else
    out_color = tonemap(color);
#endif

#if MOTIONBLUR_ENABLED
	{
        vec2 a = (f_clip_future_pos.xy / f_clip_future_pos.w) * 0.5 + 0.5;;
        vec2 b = (f_clip_pos.xy / f_clip_pos.w) * 0.5 + 0.5;;
        
        out_motion = vec4(a - b, 0.0f, 0.0f);
	}
#endif
}
