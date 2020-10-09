in vec2 f_coord;

#texture (tex1, 0, sRGB_A)
uniform sampler2D tex1;

void main()
{
	if (texture(tex1, f_coord).a < 0.5f)
		discard;
}
