#include "manul/math.hlsli"
#include "manul/material.hlsli"
#include "manul/color_transform.hlsli"

sampler diffuse_sampler : register(s0);
Texture2D<float3> diffuse : register(t0);
Texture2D<float2> normalmap : register(t1);
Texture2D<float2> dudvmap : register(t2);

void MaterialPass(inout MaterialData material) {
  float2 tex_coord = material.m_TexCoord;

  float move_factor = 0.;
#if PASS & FORWARD_LIGHTING
	move_factor += (.02 * g_Time);
#endif
	move_factor %= 1.;
	float2 distorted_tex_coord = dudvmap.Sample(diffuse_sampler, float2(tex_coord.x + move_factor, tex_coord.y)) * .1;
	distorted_tex_coord = tex_coord + float2(distorted_tex_coord.x , distorted_tex_coord.y + move_factor);
	float2 total_distorted_tex_coord = (dudvmap.Sample(diffuse_sampler, distorted_tex_coord) * 2. - 1. ) * .05;
	tex_coord += total_distorted_tex_coord;

  material.m_MaterialAlbedoAlpha.rgb = diffuse.Sample(diffuse_sampler, tex_coord);
  material.m_MaterialAlbedoAlpha.rgb = saturate(REC709_to_XYZ(material.m_MaterialAlbedoAlpha.rgb));
  material.m_MaterialAlbedoAlpha.a = 1.;
  material.m_MaterialEmission = 0;
  material.m_MaterialParams = float4(0., .04, 1., .5);
#if PASS & FORWARD_LIGHTING
  float4 fragment_ndc = float4(material.m_PositionNDC.xy, g_GbufferDepth[material.m_PixelCoord], 1.);
  fragment_ndc = mul(g_InverseProjection, fragment_ndc);
  fragment_ndc /= fragment_ndc.w;
  float depth = min(0., (fragment_ndc.z - material.m_Position.z));
  material.m_MaterialAlbedoAlpha.a = 1. - exp(depth * 2.5);
#endif
  float3 normal = UnpackNormalXY(normalmap.Sample(diffuse_sampler, tex_coord));
  material.m_MaterialNormal = normalize(normal.x * material.m_Tangent + normal.y * material.m_Bitangent + normal.z * material.m_Normal);
}
