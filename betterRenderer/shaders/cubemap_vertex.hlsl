#pragma pack_matrix(column_major)

struct VertexInput {
  float3 m_Position : Position;
  float3 m_Normal : Normal;
  float2 m_TexCoord : TexCoord;
  float4 m_Tangent : Tangent;
};

struct VertexOutput {
  float3 m_Position : Position;
  float3 m_Normal : Normal;
  float2 m_TexCoord : TexCoord;
  float4 m_Tangent : Tangent;
  float4 m_PositionSV : SV_Position;
  float4 m_PositionCS : PositionCS;
  float4 m_HistoryPositionCS : HistoryPositionCS;
};

cbuffer VertexConstants : register(b0) {
  float4x4 g_FaceProjection;
};

#include "manul/draw_constants.hlsli"

VertexOutput main(in VertexInput vs_in) {
  VertexOutput result;
  float4x3 model_view = GetModelView();
  result.m_Position = mul(float4(vs_in.m_Position, 1.), model_view).xyz;
  result.m_Normal = mul(float4(vs_in.m_Normal, 0.), model_view).xyz;
  result.m_Normal = vs_in.m_Normal;
  result.m_TexCoord = vs_in.m_TexCoord;
  result.m_Tangent.xyz = vs_in.m_Tangent.xyz;
  result.m_Tangent.w = vs_in.m_Tangent.w;
  result.m_PositionSV = mul(g_FaceProjection, float4(result.m_Position, 1.));
  result.m_PositionCS = result.m_PositionSV;
  result.m_HistoryPositionCS = result.m_PositionSV;
  return result;
}
