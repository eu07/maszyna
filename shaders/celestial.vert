const vec2 vert[4] = vec2[]
(
  vec2(-1.0,  1.0),
  vec2(-1.0, -1.0),
  vec2( 1.0,  1.0),
  vec2( 1.0, -1.0)
);

out vec2 f_coord;

#include <common>

void main()
{
	gl_Position = projection * (vec4(param[1].xyz, 1.0) + vec4(vert[gl_VertexID] * param[1].w, 0.0, 0.0));

	vec2 coord = param[2].xy;
	float size = param[2].z;

	if (gl_VertexID == 0)
		f_coord = vec2(coord.x, coord.y);
	if (gl_VertexID == 1)
		f_coord = vec2(coord.x, coord.y - size);
	if (gl_VertexID == 2)
		f_coord = vec2(coord.x + size, coord.y);
	if (gl_VertexID == 3)
		f_coord = vec2(coord.x + size, coord.y - size);
}
