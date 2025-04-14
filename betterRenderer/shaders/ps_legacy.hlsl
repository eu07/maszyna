#include "manul/material.hlsli"
#include "manul/color_transform.hlsli"

sampler diffuse_sampler : register(s0);
Texture2D diffuse : register(t0);

#include "manul/alpha_mask.hlsli"

void MaterialPass(inout MaterialData material) {
  material.m_MaterialAlbedoAlpha = diffuse.Sample(diffuse_sampler, material.m_TexCoord);
  material.m_MaterialAlbedoAlpha.rgb = saturate(REC709_to_XYZ(material.m_MaterialAlbedoAlpha.rgb * g_DrawConstants.m_Diffuse));
  AlphaMask(material.m_MaterialAlbedoAlpha.a);
  material.m_MaterialEmission = g_DrawConstants.m_SelfIllum * material.m_MaterialAlbedoAlpha.rgb;
  material.m_MaterialParams = float4(0., 1., 1., 0.);
  material.m_MaterialNormal = material.m_Normal;
}
