in vec2 f_coords;

#texture (tex1, 0, sRGB)
uniform sampler2D tex1;

layout(location = 0) out vec4 out_color;

void main()
{
	out_color = texture(tex1, f_coords);
}
