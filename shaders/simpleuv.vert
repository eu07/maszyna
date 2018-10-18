layout(location = 0) in vec3 v_vert;
layout(location = 2) in vec2 v_coord;

out vec2 f_coord;

#include <common>

void main()
{
	gl_Position = (projection * modelview) * vec4(v_vert, 1.0f);
	f_coord = v_coord;
}
