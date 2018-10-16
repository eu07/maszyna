#include <common>

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = vec4(param[0].rgb, 1.0);
}
