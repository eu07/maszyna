#pragma pack_matrix(column_major)

sampler diffuse_sampler : register(s0);
Texture2D diffuse : register(t0);

#include "manul/draw_constants_shadow.hlsli"

#include "manul/alpha_mask.hlsli"

struct VertexOutput {
  float2 m_TexCoord : TexCoord;
};

void main(in VertexOutput ps_in) {
  float4 color = diffuse.Sample(diffuse_sampler, ps_in.m_TexCoord);
  AlphaMask(color.a);
}