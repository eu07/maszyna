#version 330

in vec3 f_normal;
in vec2 f_coord;

#texture (tex1, 0, sRGB_A)
uniform sampler2D tex1;

#include <common>
#include <tonemapping.glsl>

void main()
{
	vec4 tex_color = texture(tex1, f_coord);
#if POSTFX_ENABLED
	gl_FragData[0] = tex_color * param[0];
#else
    gl_FragData[0] = tonemap(tex_color * param[0]);
#endif
#if MOTIONBLUR_ENABLED
	gl_FragData[1] = vec4(0.0f);
#endif
}
