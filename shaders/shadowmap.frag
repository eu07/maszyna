#version 330

in vec2 f_coord;

uniform sampler2D tex;

void main()
{
	gl_FragDepth = gl_FragCoord.z + (1.0 - texture(tex, f_coord).w);
}
