#pragma pack_matrix(column_major)

struct VertexInput {
  float3 m_Position : Position;
  float3 m_Normal : Normal;
  float2 m_TexCoord : TexCoord;
  float4 m_Tangent : Tangent;
  float4 m_InstanceTransform[3] : InstanceTransform;
};

struct VertexOutput {
  float2 m_TexCoord : TexCoord;
  float4 m_PositionSV : SV_Position;
};

#include "manul/draw_constants_shadow.hlsli"

VertexOutput main(in VertexInput vs_in) {
  VertexOutput result;
  float4x4 instance_transform = transpose(float4x4(vs_in.m_InstanceTransform[0], vs_in.m_InstanceTransform[1], vs_in.m_InstanceTransform[2], float4(0., 0., 0., 1.)));
  float3 position = mul(float4(vs_in.m_Position, 1.), instance_transform);
  result.m_TexCoord = vs_in.m_TexCoord;
  result.m_PositionSV = mul(g_DrawConstants.m_ModelViewProjection, float4(position, 1.));
  return result;
}
