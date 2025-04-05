#include "manul/math.hlsli"
#include "manul/color_transform.hlsli"
#include "manul/material.hlsli"

sampler diffuse_sampler : register(s0);
Texture2D<float4> tex_albedo : register(t0);
Texture2D<float4> tex_params : register(t1);
Texture2D<float2> tex_normal : register(t2);
//Texture2D<float4> tex_paramx : register(t1);
//Texture2D<float4> tex_paramy : register(t2);

#include "manul/alpha_mask.hlsli"

void MaterialPass(inout MaterialData material) {
  material.m_MaterialAlbedoAlpha = tex_albedo.Sample(diffuse_sampler, material.m_TexCoord);
  material.m_MaterialAlbedoAlpha.rgb = saturate(ACEScg_to_XYZ(material.m_MaterialAlbedoAlpha.rgb));
  AlphaMask(material.m_MaterialAlbedoAlpha.a);
  material.m_MaterialEmission = float3(0., 0., 0.);
  //float4 params_nx = tex_paramx.Sample(diffuse_sampler, material.m_TexCoord);
  //float4 params_ny = tex_paramy.Sample(diffuse_sampler, material.m_TexCoord);
  //material.m_MaterialParams = float4(params_nx.xy, params_ny.yx);
  //float3 normal = UnpackNormalXY(float2(params_nx.a, params_ny.a));
  material.m_MaterialParams = tex_params.Sample(diffuse_sampler, material.m_TexCoord);
  float3 normal = UnpackNormalXY(tex_normal.Sample(diffuse_sampler, material.m_TexCoord));
  material.m_MaterialNormal = normalize(normal.x * material.m_Tangent + normal.y * material.m_Bitangent + normal.z * material.m_Normal);
//#if PASS & FORWARD_LIGHTING
//  float NdotV = saturate(dot(material.m_MaterialNormal, -normalize(material.m_Position)));
//  material.m_MaterialAlbedoAlpha.a = lerp(pow(1. - NdotV, 5.), 1., material.m_MaterialAlbedoAlpha.a);
//#endif
}
