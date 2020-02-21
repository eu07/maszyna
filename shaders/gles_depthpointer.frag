in vec2 f_coords;

#texture (tex1, 0, R)
uniform sampler2D tex1;

layout(location = 0) out uvec4 out_color;

void main()
{
	out_color = uvec4(uint(texture(tex1, f_coords).r * 65535.0), 0, 0, 0xFFFF);
}
