#version 330

layout (location = 0) in vec3 v_vert;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_coord;

out vec3 f_normal;
out vec2 f_coord;
out vec3 f_pos;
out vec4 f_light_pos;

uniform mat4 lightview;
uniform mat4 modelview;
uniform mat3 modelviewnormal;
uniform mat4 projection;

void main()
{
	gl_Position = (projection * modelview) * vec4(v_vert, 1.0f);
	f_normal = modelviewnormal * v_normal;
	f_coord = v_coord;
	f_pos = vec3(modelview * vec4(v_vert, 1.0f));
	f_light_pos = lightview * vec4(f_pos, 1.0f);
}