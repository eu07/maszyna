#ifndef DRAW_CONSTANTS_HLSLI
#define DRAW_CONSTANTS_HLSLI

struct VertexPushConstants {
  float4x3 m_ModelView;
  float4x3 m_ModelViewHistory;
  float3 m_Diffuse;
  float m_AlphaThreshold;
  float m_AlphaMult;
  float m_SelfIllum;
};

#ifdef SPIRV

[[vk::push_constant]] ConstantBuffer<VertexPushConstants> g_DrawConstants;

#else

cbuffer g_Const : register(b1) { VertexPushConstants g_DrawConstants; }

#endif

#endif

float4x3 GetModelView() {
  return g_DrawConstants.m_ModelView;
}

float4x3 GetModelViewHistory() {
  return g_DrawConstants.m_ModelViewHistory;
}
