#include "manul/math.hlsli"
#include "manul/material.hlsli"
#include "manul/color_transform.hlsli"

sampler diffuse_sampler : register(s0);
Texture2D diffuse : register(t0);
Texture2D<float3> specgloss : register(t1);

#include "manul/alpha_mask.hlsli"

void MaterialPass(inout MaterialData material) {
  material.m_MaterialAlbedoAlpha = diffuse.Sample(diffuse_sampler, material.m_TexCoord);
  material.m_MaterialAlbedoAlpha.rgb = saturate(REC709_to_XYZ(material.m_MaterialAlbedoAlpha.rgb * g_DrawConstants.m_Diffuse));
  AlphaMask(material.m_MaterialAlbedoAlpha.a);
  material.m_MaterialEmission = g_DrawConstants.m_SelfIllum * material.m_MaterialAlbedoAlpha.rgb;

  float3 params = specgloss.Sample(diffuse_sampler, material.m_TexCoord);
  material.m_MaterialParams = float4(params.b, 1. - params.g, 1., .5);

  material.m_MaterialNormal = material.m_Normal;
}
