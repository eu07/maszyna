
struct VertexOutput {
  float2 m_Position : Position;
  float4 m_PositionSV : SV_Position;
};

struct PixelOutput {
  float4 m_Output : SV_Target0;
};

sampler source_sampler : register(s0);
Texture2D source : register(t0);

PixelOutput main(VertexOutput ps_in) {
  PixelOutput result;
  result.m_Output = source.Sample(source_sampler, ps_in.m_Position);
  return result;
}
