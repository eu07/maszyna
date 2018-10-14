#version 330 core
out vec4 FragColor;
  
in vec2 f_coords;

#texture (tex1, 0, RGB)
uniform sampler2D tex1;

#include <tonemapping.glsl>

void main()
{
	vec2 texcoord = f_coords;
	vec3 hdr_color = texture(tex1, texcoord).xyz;

	vec3 mapped = tonemap(hdr_color);
	gl_FragColor = vec4(mapped, 1.0);
}  
