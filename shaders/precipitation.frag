#version 330

in vec2 f_coord;

#texture (tex1, 0, sRGB_A)
uniform sampler2D tex1;

#include <common>

in vec4 f_clip_pos;
in vec4 f_clip_future_pos;

void main()
{
	vec4 tex_color = texture(tex1, vec2(f_coord.x, f_coord.y + param[1].x));
	gl_FragData[0] = tex_color * param[0];
#if MOTIONBLUR_ENABLED
	{
        vec2 a = (f_clip_future_pos.xy / f_clip_future_pos.w) * 0.5 + 0.5;
        vec2 b = (f_clip_pos.xy / f_clip_pos.w) * 0.5 + 0.5;
        
        gl_FragData[1] = vec4(a - b, 0.0f, tex_color.a * alpha_mult);
	}
#endif
}
