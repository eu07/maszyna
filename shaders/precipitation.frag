#version 330

in vec2 f_coord;

#texture (tex1, 0, sRGB_A)
uniform sampler2D tex1;

#include <common>

void main()
{
	vec4 tex_color = texture(tex1, vec2(f_coord.x, f_coord.y + param[1].x));
	gl_FragData[0] = tex_color * param[0];
#if MOTIONBLUR_ENABLED
	gl_FragData[1] = vec4(0.0f);
#endif
}
