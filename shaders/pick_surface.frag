#include <common>

layout(location = 0) out vec4 out_color;
in vec2 f_coord;

void main()
{
	out_color = vec4(param[0].r, f_coord, 1.0);
}
