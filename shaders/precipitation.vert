layout(location = 0) in vec3 v_vert;
layout(location = 1) in vec2 v_coord;

out vec2 f_coord;

out vec4 f_clip_pos;
out vec4 f_clip_future_pos;

#include <common>

void main()
{
	f_clip_pos = (projection * modelview) * vec4(v_vert, 1.0f);
	f_clip_future_pos = (projection * future * modelview) * vec4(v_vert, 1.0f);
	
	gl_Position = f_clip_pos;
	f_coord = v_coord;
}
