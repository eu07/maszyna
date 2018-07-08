#version 330

in vec3 v_vert;
in vec3 v_normal;
in vec2 v_coord;
in vec4 v_tangent;

out vec3 f_normal;
out vec2 f_coord;
out vec3 f_pos;
out mat3 f_tbn;
out vec4 f_tangent;

#include <common>

void main()
{
	gl_Position = (projection * modelview) * vec4(v_vert, 1.0f);
	f_normal = modelviewnormal * v_normal;
	f_coord = v_coord;
	f_tangent = v_tangent;
	f_pos = vec3(modelview * vec4(v_vert, 1.0f));

	vec3 T = normalize(modelviewnormal * v_tangent.xyz);
	vec3 B = normalize(modelviewnormal * cross(v_normal, v_tangent.xyz) * v_tangent.w);
	vec3 N = normalize(modelviewnormal * v_normal);
	f_tbn = mat3(T, B, N);
}
