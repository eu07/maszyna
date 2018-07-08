#version 330

in vec3 v_vert;
in vec3 v_normal;
in vec2 v_coord;

#include <common>

void main()
{
	gl_Position = (projection * modelview) * vec4(v_vert, 1.0f);
}
