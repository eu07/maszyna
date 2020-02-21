in vec2 f_coord;

#texture (tex1, 0, sRGB_A)
uniform sampler2D tex1;

#include <common>
#include <tonemapping.glsl>

layout(location = 0) out vec4 out_color;
#if MOTIONBLUR_ENABLED
layout(location = 1) out vec4 out_motion;
#endif

void main()
{
	vec4 tex_color = texture(tex1, f_coord);
#if POSTFX_ENABLED
	out_color = tex_color * param[0];
#else
    out_color = tonemap(tex_color * param[0]);
#endif
#if MOTIONBLUR_ENABLED
	out_motion = vec4(0.0f);
#endif
}
