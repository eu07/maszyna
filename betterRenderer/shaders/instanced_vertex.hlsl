#pragma pack_matrix(column_major)

struct VertexInput {
  float3 m_Position : Position;
  float3 m_Normal : Normal;
  float2 m_TexCoord : TexCoord;
  float4 m_Tangent : Tangent;
  float4 m_InstanceTransform[3] : InstanceTransform;
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
  float4x4 g_JitteredProjection;
  float4x4 g_Projection;
  float4x4 g_ProjectionHistory;
};

#include "manul/draw_constants.hlsli"

VertexOutput main(in VertexInput vs_in) {
  VertexOutput result;
  float4x3 model_view = GetModelView();
  float4x3 model_view_history = GetModelViewHistory();
  float4x4 instance_transform = transpose(float4x4(vs_in.m_InstanceTransform[0], vs_in.m_InstanceTransform[1], vs_in.m_InstanceTransform[2], float4(0., 0., 0., 1.)));
  float3 position = mul(float4(vs_in.m_Position, 1.), instance_transform).xyz;
  result.m_Position = mul(float4(position, 1.), model_view).xyz;
  result.m_Normal = mul(mul(float4(vs_in.m_Normal, 0.), instance_transform), model_view);
  result.m_TexCoord = vs_in.m_TexCoord;
  result.m_Tangent.xyz = mul(mul(float4(vs_in.m_Tangent.xyz, 0.), instance_transform), model_view);
  result.m_Tangent.w = vs_in.m_Tangent.w;
  result.m_PositionSV = mul(g_JitteredProjection, float4(result.m_Position, 1.));
  result.m_PositionCS = mul(g_Projection, float4(result.m_Position, 1.));
  float3 history_position = mul(float4(position, 1.), model_view_history).xyz;
  result.m_HistoryPositionCS = mul(g_ProjectionHistory, float4(history_position, 1.));
  return result;
}
