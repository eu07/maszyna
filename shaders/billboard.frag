#version 330

in vec3 f_normal;
in vec2 f_coord;

uniform sampler2D tex1;

#include <common>

void main()
{
	vec4 tex_color = texture(tex1, f_coord);
	gl_FragColor = tex_color * param[0];
}
