#version 330

in vec3 f_normal;
in vec2 f_coord;
in vec3 f_pos;

uniform sampler2D tex1;

#include <common>

void main()
{
	vec4 tex_color = texture(tex1, f_coord);
	gl_FragColor = vec4(tex_color.xyz * ambient, tex_color.a);
}
