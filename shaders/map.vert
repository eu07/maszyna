#version 330

layout(location = 0) in vec3 v_vert;

#include <common>

void main()
{
    vec4 clip_pos = projection * vec4(v_vert, 1.0f);

	gl_Position = vec4(clip_pos.xz, 0.5, 1.0);
}
