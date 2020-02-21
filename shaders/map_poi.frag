in vec2 f_coords;

layout(location = 0) out vec4 out_color;

#texture (tex1, 0, sRGB_A)
uniform sampler2D tex1;

void main()
{
	vec2 texcoord = f_coords;
	vec4 color = texture(tex1, texcoord);

	out_color = color;
}  
