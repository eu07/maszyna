#include "manul/math.hlsli"
#include "manul/material.hlsli"
#include "manul/color_transform.hlsli"

sampler diffuse_sampler : register(s0);
Texture2D diffuse : register(t0);
Texture2D<float4> normalmap : register(t1);

#include "manul/alpha_mask.hlsli"

void MaterialPass(inout MaterialData material) {
  material.m_MaterialAlbedoAlpha = diffuse.Sample(diffuse_sampler, material.m_TexCoord);
  material.m_MaterialAlbedoAlpha.rgb = saturate(REC709_to_XYZ(material.m_MaterialAlbedoAlpha.rgb * g_DrawConstants.m_Diffuse));
  AlphaMask(material.m_MaterialAlbedoAlpha.a);
  material.m_MaterialEmission = g_DrawConstants.m_SelfIllum * material.m_MaterialAlbedoAlpha.rgb;

  float4 normal_refl = normalmap.Sample(diffuse_sampler, material.m_TexCoord);
  material.m_MaterialParams = float4(0., (1. - normal_refl.a) * (1. - normal_refl.a), 1., 0.);

  float3 normal = UnpackNormalXY(normal_refl.xy);
  material.m_MaterialNormal = normalize(normal.x * material.m_Tangent + normal.y * material.m_Bitangent + normal.z * material.m_Normal);
}
