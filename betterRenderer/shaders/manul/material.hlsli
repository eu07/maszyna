#ifndef MATERIAL_HLSLI
#define MATERIAL_HLSLI

#define MOTION_VECTORS (1 << 0)
#define FORWARD_LIGHTING (1 << 1)

#define PASS_GBUFFER (MOTION_VECTORS)
#define PASS_FORWARD (FORWARD_LIGHTING)
#define PASS_CUBEMAP (0)

struct PixelOutput {
#if PASS & FORWARD_LIGHTING
  float4 m_Color : SV_Target0;
  float m_CompositionMask : SV_Target1;
#if PASS & MOTION_VECTORS
  float2 m_Motion : SV_Target2;
#endif
#else
  float4 m_Color : SV_Target0;
  float4 m_Emission : SV_Target1;
  float4 m_Params : SV_Target2;
  float4 m_Normal : SV_Target3;
#if PASS & MOTION_VECTORS
  float2 m_Motion : SV_Target4;
#endif
#endif
};

struct PixelInput {
  float3 m_Position : Position;
  float3 m_Normal : Normal;
  float2 m_TexCoord : TexCoord;
  float4 m_Tangent : Tangent;
  float4 m_PositionSV : SV_Position;
  float4 m_PositionCS : PositionCS;
#if PASS & MOTION_VECTORS
  float4 m_HistoryPositionCS : HistoryPositionCS;
#endif
};

#include "draw_constants.hlsli"

#include "material_common.hlsli"

#if PASS & FORWARD_LIGHTING
#include "shadow.hlsli"
#include "lighting.hlsli"
#include "sky.hlsli"

Texture2D<float> g_GbufferDepth : register(t12);
#endif

void MaterialPass(inout MaterialData material);

PixelOutput main(in PixelInput ps_in) {
  float2 uv = (ps_in.m_PositionCS.xy/ps_in.m_PositionCS.w) * float2(.5, -.5) + .5;

  MaterialData material;
  material.m_Position = ps_in.m_Position;
  material.m_Normal = ps_in.m_Normal;
  material.m_Tangent = ps_in.m_Tangent.xyz;
  material.m_Bitangent = ps_in.m_Tangent.w * cross(ps_in.m_Normal, ps_in.m_Tangent.xyz);
  material.m_TexCoord = ps_in.m_TexCoord;
  material.m_PixelCoord = ps_in.m_PositionSV.xy;
  material.m_PositionNDC = ps_in.m_PositionCS / ps_in.m_PositionCS.w;
  material.m_MaterialAlbedoAlpha = float4(1., 1., 1., 1.);
  material.m_MaterialEmission = float3(0., 0., 0.);
  material.m_MaterialParams = float4(0., .5, 1., .5); // Metalness.Roughness.Occlusion.Specular
  material.m_MaterialNormal = float3(0., 0., 1.);
  MaterialPass(material);
  material.m_MaterialAlbedoAlpha.rgb = saturate(material.m_MaterialAlbedoAlpha.rgb);
  material.m_MaterialEmission = max(material.m_MaterialEmission, 0.);
  
  PixelOutput ps_out;
  
#if PASS & FORWARD_LIGHTING
#if LIGHTING_NEEDS_PIXELPOSITION
  // Should not really happen?
  ApplyMaterialLighting(ps_out.m_Color, material, (uint2)ps_in.m_PositionSV.xy);
#else
  ApplyMaterialLighting(ps_out.m_Color, material);
#endif
  float3 view = mul((float3x3)g_InverseModelView, material.m_Position);
  ApplyAerialPerspective(ps_out.m_Color.rgb, ps_out.m_Color.a, normalize(view), g_LightDir, length(material.m_Position)/2500.);
  //ps_out.m_Color.rgb += CalcAtmosphere(normalize(view), g_LightDir, g_Altitude, length(view)) * material.m_MaterialAlbedoAlpha.a;
  ps_out.m_CompositionMask = ps_out.m_Color.a > 0.;
#else
  ps_out.m_Color.rgb = material.m_MaterialAlbedoAlpha.rgb;
  ps_out.m_Emission.rgb = material.m_MaterialEmission;
  ps_out.m_Params = material.m_MaterialParams;
  ps_out.m_Normal.rgb = material.m_MaterialNormal * .5 + .5;
#endif
  
#if PASS & MOTION_VECTORS
  ps_out.m_Motion = (ps_in.m_HistoryPositionCS.xy / ps_in.m_HistoryPositionCS.w) - (ps_in.m_PositionCS.xy / ps_in.m_PositionCS.w);
  ps_out.m_Motion = ps_out.m_Motion * float2(.5, -.5);
#endif
  
  return ps_out;
}

#endif
