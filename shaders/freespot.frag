#version 330

#include <common>
#include <tonemapping.glsl>

in vec4 f_clip_pos;
in vec4 f_clip_future_pos;

void main()
{
	float x = (gl_PointCoord.x - 0.5f) * 2.0f;
	float y = (gl_PointCoord.y - 0.5f) * 2.0f;
	float dist = sqrt(x * x + y * y);
	if (dist > 0.5f)
		discard;
	vec4 color = vec4(param[0].rgb * emission, mix(param[0].a, 0.0f, dist * 2.0f));
#if POSTFX_ENABLED
    gl_FragData[0] = color;
#else
    gl_FragData[0] = tonemap(color);
#endif
#if MOTIONBLUR_ENABLED
	{
        vec2 a = (f_clip_future_pos.xy / f_clip_future_pos.w) * 0.5 + 0.5;;
        vec2 b = (f_clip_pos.xy / f_clip_pos.w) * 0.5 + 0.5;;
        
        gl_FragData[1] = vec4(a - b, 0.0f, 0.0f);
	}
#endif
}
