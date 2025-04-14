#ifndef MATERIAL_COMMON_HLSLI
#define MATERIAL_COMMON_HLSLI

struct MaterialData {
  float3 m_Position;
  float3 m_PositionDDX;
  float3 m_PositionDDY;
  float3 m_Tangent;
  float3 m_Bitangent;
  float3 m_Normal;
  float2 m_TexCoord;
  uint2 m_PixelCoord;
  float4 m_PositionNDC;
  float4 m_MaterialAlbedoAlpha;
  float3 m_MaterialEmission;
  float4 m_MaterialParams; // Metalness.Roughness.Occlusion.Specular
  float3 m_MaterialNormal;
};

#endif
