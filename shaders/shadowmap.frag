#version 330

in vec2 f_coord;

void main()
{
	gl_FragDepth = gl_FragCoord.z;
}
