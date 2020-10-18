const vec2 vert[4] = vec2[]
(
  vec2(-1.0,  1.0),
  vec2(-1.0, -1.0),
  vec2( 1.0,  1.0),
  vec2( 1.0, -1.0)
);

const vec2 uv[4] = vec2[]
(
  vec2(0.0, 1.0),
  vec2(0.0, 0.0),
  vec2(1.0, 1.0),
  vec2(1.0, 0.0)
);

out vec2 f_coords;

void main()
{
    gl_Position = vec4(vert[gl_VertexID], 0.0, 1.0); 
    f_coords = uv[gl_VertexID];
}  
