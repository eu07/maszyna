#version 330

#include <common>

void main()
{
	gl_FragColor = vec4(param[0].rgb, 1.0);
}
