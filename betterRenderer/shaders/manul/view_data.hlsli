#ifndef VIEW_DATA_HLSLI
#define VIEW_DATA_HLSLI

cbuffer DrawConstants : register(b2) {
  float4x4 g_InverseModelView;
  float4x4 g_InverseProjection;
  float3 g_LightDir;
  float g_Altitude;
  float3 g_LightColor;
  float g_Time;
}

float2 PixelToCS(in float2 pixel, in float2 size) {
  return ((pixel + .5) / size - .5) * float2(2., -2.);
}

float3 ReconstructPos(in float2 cs, in float depth) {
  float4 ndc = float4(cs, depth, 1.);
  ndc = mul(g_InverseProjection, ndc);
  return ndc.xyz / ndc.w;
}

#endif