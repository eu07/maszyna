layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

in vec2 g_coords[];
out vec2 f_coords;

#include <common>

void main() {
	vec4 position = gl_in[0].gl_Position;
	vec2 size = vec2(0.1) * cascade_end.xy;

	gl_Position = position + vec4(-size.x, -size.y, 0.0, 0.0);
	f_coords = vec2(g_coords[0].s, 0.0);
	EmitVertex();

	gl_Position = position + vec4( size.x, -size.y, 0.0, 0.0);
	f_coords = vec2(g_coords[0].t, 0.0);
	EmitVertex();

	gl_Position = position + vec4(-size.x,  size.y, 0.0, 0.0);
	f_coords = vec2(g_coords[0].s, 1.0);
	EmitVertex();

	gl_Position = position + vec4( size.x,  size.y, 0.0, 0.0);
	f_coords = vec2(g_coords[0].t, 1.0);
	EmitVertex();

    EndPrimitive();
}  
