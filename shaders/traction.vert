#version 330

layout(location = 0) in vec3 v_vert;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_coord;

#include <common>

void main()
{
	gl_Position = (projection * modelview) * vec4(v_vert, 1.0f);
}
