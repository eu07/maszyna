#include <common>

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(scene_extra, 1.0f);
}
