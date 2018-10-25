in vec2 f_coords;

layout(location = 0) out vec4 out_color;

#texture (tex1, 0, sRGB)
uniform sampler2D tex1;

void main()
{
	vec2 texcoord = f_coords;
	vec3 color = texture(tex1, texcoord).rgb;

	out_color = FBOUT(vec4(color, 1.0));
}  
