#ifndef DRAW_CONSTANTS_SHADOW_HLSLI
#define DRAW_CONSTANTS_SHADOW_HLSLI

struct VertexPushConstantsShadow {
  float4x4 m_ModelViewProjection;
  float m_AlphaThreshold;
};

#ifdef SPIRV

[[vk::push_constant]] ConstantBuffer<VertexPushConstantsShadow> g_DrawConstants;

#else

cbuffer g_Const : register(b1) { VertexPushConstantsShadow g_DrawConstants; }

#endif

#endif
