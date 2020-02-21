in vec2 f_coords;

layout(location = 0) out vec4 out_color;

#texture (tex1, 0, RGB)
uniform sampler2D tex1;

#include <tonemapping.glsl>

void main()
{
	vec2 texcoord = f_coords;
	vec3 hdr_color = texture(tex1, texcoord).xyz;
	out_color = tonemap(vec4(hdr_color, 1.0));
}  
