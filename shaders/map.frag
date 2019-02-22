#include <common>

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(vec3(0.3f, time, 0.3f), 1.0f);
}
