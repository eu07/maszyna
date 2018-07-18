#version 330

#include <common>

#param (color, 0, 0, 4, diffuse)

void main()
{
	gl_FragColor = param[0];
}
