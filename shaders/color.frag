#version 330

in vec3 f_color;

out vec4 color;

void main()
{
	color = vec4(f_color, 1.0f);
}
