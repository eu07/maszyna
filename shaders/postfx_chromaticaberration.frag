in vec2 f_coords;

layout(location = 0) out vec4 out_color;

#texture (color_tex, 0, RGB)
uniform sampler2D iChannel0;

void main()
{
	float amount = 0.001;
	
	vec2 uv = f_coords;
	vec3 col;
	col.r = texture( iChannel0, vec2(uv.x+amount,uv.y) ).r;
	col.g = texture( iChannel0, uv ).g;
	col.b = texture( iChannel0, vec2(uv.x-amount,uv.y) ).b;

	col *= (1.0 - amount * 0.5);
	
	out_color = vec4(col,1.0);
}  
