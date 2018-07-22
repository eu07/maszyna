#version 330

#include <common>

in vec4 f_clip_pos;
in vec4 f_clip_future_pos;

void main()
{
	float x = (gl_PointCoord.x - 0.5f) * 2.0f;
	float y = (gl_PointCoord.y - 0.5f) * 2.0f;
	float dist2 = abs(x * x + y * y);
	if (dist2 > 0.5f * 0.5f)
		discard;
	gl_FragData[0] = vec4(param[0].rgb * emission, 1.0f);
	{
        vec2 a = (f_clip_future_pos.xy / f_clip_future_pos.w) * 0.5 + 0.5;;
        vec2 b = (f_clip_pos.xy / f_clip_pos.w) * 0.5 + 0.5;;
        
        gl_FragData[1] = vec4(a - b, 0.0f, 0.0f);
	}
}
