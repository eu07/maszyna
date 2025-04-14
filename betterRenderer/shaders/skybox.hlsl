
struct VertexOutput {
  float2 m_Position : Position;
  float4 m_PositionSV : SV_Position;
};

struct PixelOutput {
  float3 m_Color : SV_Target0;
  float3 m_Emission : SV_Target1;
  float4 m_Params : SV_Target2;
  float3 m_Normal : SV_Target3;
  float3 m_Motion : SV_Target4;
};

cbuffer SkyboxPixelConstants : register(b0) {
  float4x4 g_InverseViewProjection;
  float4x4 g_HistoryReproject;
  float3 g_SunDirection;
  float g_Altitude;
  float3 g_MoonDirection;
}

sampler g_SkyboxSampler : register(s0);
TextureCube g_Skybox : register(t0);

#include "manul/sky.hlsli"

PixelOutput main(VertexOutput ps_in) {
  PixelOutput result;
  result.m_Color = 0..xxx;
  result.m_Emission = 1..xxx;
  result.m_Params = 0..xxxx;
  result.m_Normal = 0..xxx;
  result.m_Motion = 0..xxx;
  float4 positionNdc = float4((ps_in.m_Position - .5.xx) * float2(2., -2.), 1., 1.);
  float3 viewDir = normalize(mul(g_InverseViewProjection, positionNdc).xyz);
  result.m_Emission = 1.e-7;
  CalcSun(result.m_Emission, viewDir, g_SunDirection, g_Altitude);
  CalcMoon(result.m_Emission, viewDir, g_MoonDirection, g_SunDirection, g_Altitude);
  CalcAtmosphere(result.m_Emission, viewDir, g_SunDirection);
  //result.m_Emission = g_Skybox.Sample(g_SkyboxSampler, normalize(mul(g_InverseViewProjection, positionNdc).xyz)).rgb;
  //result.m_Emission = 0.; //Sky(normalize(mul(g_InverseViewProjection, positionNdc).xyz), g_SunDirection, g_Altitude);
  float4 positionReproject = mul(g_HistoryReproject, positionNdc);
  positionReproject.xyz /= positionReproject.w;
  result.m_Motion = (positionNdc - positionReproject).xyz;
  result.m_Motion.xy = result.m_Motion.xy * .5.xx;
  //result.m_Output = source.Sample(source_sampler, ps_in.m_Position);
  return result;
}
