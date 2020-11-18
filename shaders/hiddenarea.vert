layout(location = 0) in vec3 v_vert;

void main()
{
	gl_Position = vec4(v_vert.xy * 2.0f - 1.0f, 0.5f, 1.0f);
}
