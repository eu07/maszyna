#include "manul/math.hlsli"

struct VertexOutput {
  float2 m_Position : Position;
  float4 m_PositionSV : SV_Position;
};

struct PixelOutput {
  float4 m_Color : SV_Target0;
};

Texture2D<float3> gbuffer_diffuse : register(t0);
Texture2D<float3> gbuffer_emission : register(t1);
Texture2D<float4> gbuffer_params : register(t2);
Texture2D<float3> gbuffer_normal : register(t3);
Texture2D<float> gbuffer_depth : register(t4);

RWTexture2D<float4> output : register(u0);

#define DEFERRED_LIGHTING_PASS

#include "manul/gbuffer_ssao.hlsli"
//#include "manul/gbuffer_contact_shadows.hlsli"
#include "manul/shadow.hlsli"
#include "manul/lighting.hlsli"
#include "manul/sky.hlsli"

#define BLOCK_SIZE 8
#define TILE_BORDER 1
#define TILE_SIZE (BLOCK_SIZE + 2 * TILE_BORDER)

groupshared float2 tile_XY[TILE_SIZE*TILE_SIZE];
groupshared float tile_Z[TILE_SIZE*TILE_SIZE];

uint2 unflatten2D(uint idx, uint2 dim)
{
  return uint2(idx % dim.x, idx / dim.x);
}

uint flatten2D(uint2 coord, uint2 dim)
{
  return coord.x + coord.y * dim.x;
}

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void main(uint3 PixCoord : SV_DispatchThreadID, uint3 GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex) {
  uint2 gbuffer_dimensions;
  gbuffer_depth.GetDimensions(gbuffer_dimensions.x, gbuffer_dimensions.y);

  const int2 tile_upperleft = GroupID.xy * BLOCK_SIZE - TILE_BORDER;
  for (uint t = GroupIndex; t < TILE_SIZE * TILE_SIZE; t += BLOCK_SIZE * BLOCK_SIZE)
  {
    const uint2 pixel = tile_upperleft + unflatten2D(t, TILE_SIZE);
    const float depth = gbuffer_depth[pixel];
    const float3 position = ReconstructPos(PixelToCS(pixel, gbuffer_dimensions), depth);
    tile_XY[t] = position.xy;
    tile_Z[t] = position.z;
  }
  GroupMemoryBarrierWithGroupSync();
// Decode material data
  MaterialData material;

  material.m_PixelCoord = PixCoord.xy;
  float2 uv = ( material.m_PixelCoord + .5) / gbuffer_dimensions;

  uint2 tile_co = material.m_PixelCoord - tile_upperleft;
  
  //float depth = gbuffer_depth[ material.m_PixelCoord];
  uint co = flatten2D(tile_co, TILE_SIZE);

  uint co_px = flatten2D(tile_co + int2(1, 0), TILE_SIZE);
  uint co_nx = flatten2D(tile_co + int2(-1, 0), TILE_SIZE);
  uint co_py = flatten2D(tile_co + int2(0, 1), TILE_SIZE);
  uint co_ny = flatten2D(tile_co + int2(0, -1), TILE_SIZE);

  float depth = tile_Z[co];
  float depth_px = tile_Z[co_px];
  float depth_nx = tile_Z[co_nx];
  float depth_py = tile_Z[co_py];
  float depth_ny = tile_Z[co_ny];

  material.m_Position = float3(tile_XY[co], depth);
  if(abs(depth_px - depth) < abs(depth_nx - depth)) {
    material.m_PositionDDX.xy = tile_XY[co_px];
    material.m_PositionDDX.z = depth_px;
  }
  else{
    material.m_PositionDDX.xy = tile_XY[co_nx];
    material.m_PositionDDX.z = depth_nx;
  }
  if(abs(depth_py - depth) < abs(depth_ny - depth)) {
    material.m_PositionDDY.xy = tile_XY[co_py];
    material.m_PositionDDY.z = depth_py;
  }
  else{
    material.m_PositionDDY.xy = tile_XY[co_ny];
    material.m_PositionDDY.z = depth_ny;
  }
  
  material.m_MaterialAlbedoAlpha.rgb = gbuffer_diffuse[ material.m_PixelCoord];
  material.m_MaterialAlbedoAlpha.a = 1.;
  material.m_MaterialEmission = gbuffer_emission[ material.m_PixelCoord];
  material.m_MaterialParams = gbuffer_params[ material.m_PixelCoord];
  material.m_MaterialNormal = UnpackNormalXYZ(gbuffer_normal[ material.m_PixelCoord]);
  
  PixelOutput ps_out;
#if LIGHTING_NEEDS_PIXELPOSITION
  ApplyMaterialLighting(output[material.m_PixelCoord], material,  material.m_PixelCoord);
#else
  ApplyMaterialLighting(output[material.m_PixelCoord], material);
#endif
  float3 view_world = mul((float3x3)g_InverseModelView, material.m_Position);
  ApplyAerialPerspective(output[material.m_PixelCoord].rgb, 1., normalize(view_world), g_LightDir, length(view_world)/2500.);
  //if(depth < 1.){
  //  CalcAtmosphere(ps_out.m_Color.rgb, 1., normalize(mul((float3x3) g_InverseModelView, material.m_Position)), g_LightDir.xyz, g_Altitude, length(view.xyz), g_LightColor.rgb, 10);
  //}
  //return ps_out;
}
