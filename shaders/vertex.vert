layout(location = 0) in vec3 v_vert;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_coord;
layout(location = 3) in vec4 v_tangent;

#include <common>

out vec3 f_normal;
flat out vec3 f_normal_raw;
out vec2 f_coord;
out vec4 f_pos;
out mat3 f_tbn;
out vec4 f_light_pos[MAX_CASCADES];

out vec4 f_clip_pos;
out vec4 f_clip_future_pos;

//out vec3 TangentLightPos;
//out vec3 TangentViewPos;
out vec3 TangentFragPos;

void main()
{
	f_normal = normalize(modelviewnormal * v_normal);
	f_normal_raw = v_normal;
	f_coord = v_coord;
	f_pos = modelview * vec4(v_vert, 1.0);
	for (uint idx = 0U ; idx < MAX_CASCADES ; ++idx) {
		f_light_pos[idx] = lightview[idx] * f_pos;
	}	
	f_clip_pos = (projection * modelview) * vec4(v_vert, 1.0);
	f_clip_future_pos = (projection * future * modelview) * vec4(v_vert, 1.0);
	
	gl_Position = f_clip_pos;
	gl_PointSize = param[1].x;

	vec3 T = normalize(modelviewnormal * v_tangent.xyz);
	vec3 N = f_normal;
	vec3 B = normalize(cross(N, T));
	f_tbn = mat3(T, B, N);
	
	mat3 TBN = transpose(f_tbn);
//	TangentLightPos = TBN * f_light_pos.xyz;
//	TangentViewPos = TBN * vec3(0.0, 0.0, 0.0);
	TangentFragPos = TBN * f_pos.xyz;
}
