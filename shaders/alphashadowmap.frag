#version 330

in vec2 f_coord;

#texture (tex1, 0, sRGB_A)
uniform sampler2D tex1;

void main()
{
	//gl_FragColor = texture(tex, f_coord);
	//gl_FragDepth = gl_FragCoord.z;
	gl_FragDepth = gl_FragCoord.z + (1.0 - texture(tex1, f_coord).a);
}
