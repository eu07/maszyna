
struct VertexInput {
  float2 m_Position : Position;
};

struct VertexOutput {
  float2 m_Position : Position;
  float4 m_PositionSV : SV_Position;
};

VertexOutput main(in VertexInput vs_in) {
  VertexOutput result;
  result.m_Position = vs_in.m_Position;
  result.m_PositionSV = float4(vs_in.m_Position * float2(2., -2.) + float2(-1., 1.), 0., 1.);
  return result;
}
