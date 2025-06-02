#pragma pack_matrix(column_major)

struct VertexInput {
  float3 m_Position : Position;
  float3 m_Normal : Normal;
  float2 m_TexCoord : TexCoord;
  float4 m_Tangent : Tangent;
};

struct VertexOutput {
  float2 m_TexCoord : TexCoord;
  float4 m_PositionSV : SV_Position;
};

#include "manul/draw_constants_shadow.hlsli"

VertexOutput main(in VertexInput vs_in) {
  VertexOutput result;
  result.m_TexCoord = vs_in.m_TexCoord;
  result.m_PositionSV = mul(g_DrawConstants.m_ModelViewProjection, float4(vs_in.m_Position, 1.));
  return result;
}
