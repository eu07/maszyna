layout(location = 0) in vec3 v_vert;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_coord;
layout(location = 3) in vec4 v_tangent;

out vec3 f_normal;
flat out vec3 f_normal_raw;
out vec2 f_coord;
out vec4 f_pos;
out mat3 f_tbn;
out vec4 f_tangent;
out vec4 f_light_pos;

out vec4 f_clip_pos;
out vec4 f_clip_future_pos;

#include <common>

void main()
{
	f_normal = modelviewnormal * v_normal;
	f_normal_raw = v_normal;
	f_coord = v_coord;
	f_tangent = v_tangent;
	f_pos = modelview * vec4(v_vert, 1.0f);
	f_light_pos = lightview * f_pos;
	
	f_clip_pos = (projection * modelview) * vec4(v_vert, 1.0f);
	f_clip_future_pos = (projection * future * modelview) * vec4(v_vert, 1.0f);
	
	gl_Position = f_clip_pos;
	gl_PointSize = param[1].x;

	vec3 T = normalize(modelviewnormal * v_tangent.xyz);
	vec3 B = normalize(modelviewnormal * cross(v_normal, v_tangent.xyz) * v_tangent.w);
	vec3 N = normalize(modelviewnormal * v_normal);
	f_tbn = mat3(T, B, N);
}
