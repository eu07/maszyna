#version 330

#include <common>

in vec4 f_clip_pos;
in vec4 f_clip_future_pos;

void main()
{
	gl_FragData[0] = param[0];
#if MOTIONBLUR_ENABLED
	{
        vec2 a = (f_clip_future_pos.xy / f_clip_future_pos.w) * 0.5 + 0.5;;
        vec2 b = (f_clip_pos.xy / f_clip_pos.w) * 0.5 + 0.5;;
        
        gl_FragData[1] = vec4(a - b, 0.0f, 0.0f);
	}
#endif
}
