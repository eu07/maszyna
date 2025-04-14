#define PI 3.1415926535897932384626433832795
#define TWO_PI 6.283185307179586476925286766559
#define ONE_OVER_PI 0.31830988618379067153776752674503
#define TWO_OVER_PI 0.63661977236758134307553505349006
#define ONE_OVER_TWO_PI 0.15915494309189533576888376337251

RWTexture2DArray<float4> g_OutCubemap : register(u0);

TextureCube<float3> g_Skybox : register(t0);
TextureCube<float3> g_Diffuse : register(t1);
TextureCube<float3> g_Normal : register(t2);
TextureCube<float4> g_Params : register(t3);
TextureCube<float> g_Depth : register(t4);
TextureCube<float3> g_DiffuseIBL : register(t5);

SamplerState g_SamplerLinearClamp : register(s0);
SamplerState g_SamplerPointClamp : register(s1);

#include "cubemap_utils.hlsli"
#include "manul/sky.hlsli"

struct FilterParameters {
  uint3 m_Offset;
  uint unused;
  float3 m_LightVector;
  float m_Altitude;
  float3 m_LightColor;
};

#ifdef SPIRV

[[vk::push_constant]] ConstantBuffer<FilterParameters> g_FilterParams;

#else

cbuffer g_Const : register(b0) { FilterParameters g_FilterParams; }

#endif

cbuffer FilterConstants : register(b1) {
  float4x4 g_InverseProjection;
};

[numthreads(32, 32, 1)]
void main(uint3 PixCoord : SV_DispatchThreadID) {
  float3 normal = CalcNormal(PixCoord + g_FilterParams.m_Offset);
  float3 size;
  g_OutCubemap.GetDimensions(size.x, size.y, size.z);

  //g_OutCubemap[PixCoord + g_Offset] = g_Skybox.SampleLevel(g_SamplerLinearClamp, normal, 0.);
  float3 color = 1.e-7;
  CalcAtmosphere(color, normal, g_FilterParams.m_LightVector);
  //CalcAtmosphere(g_OutCubemap[PixCoord + g_Offset], 1., normal, g_LightVector, g_Altitude, SKY_INF, g_LightColor.rgb, 10);
  float3 normal_flipped = normal * float3(-1., 1., 1.);
  float depth = g_Depth.SampleLevel(g_SamplerPointClamp, normal_flipped, 0.);
  float linear_depth = 2500.;
  if(depth > 0.) {
    float3 material_albedo = g_Diffuse.SampleLevel(g_SamplerPointClamp, normal_flipped, 0.);
    float4 material_params = g_Params.SampleLevel(g_SamplerPointClamp, normal_flipped, 0.);
    float3 material_normal = (g_Normal.SampleLevel(g_SamplerPointClamp, normal_flipped, 0.) - .5) * 2.;
    float NdotL = max(dot(material_normal, g_FilterParams.m_LightVector), 0.);   
    color = material_albedo * (g_DiffuseIBL.SampleLevel(g_SamplerPointClamp, material_normal, 0.) + ONE_OVER_PI * NdotL * g_FilterParams.m_LightColor.rgb) * material_params.b;
    float4 position_ndc = mul(g_InverseProjection, float4(PixCoordToFloat(PixCoord.xy + g_FilterParams.m_Offset.xy, size.xy) * 2. - 1., depth, 1.));
    position_ndc /= position_ndc.w;
    linear_depth = length(position_ndc.xyz);
  }
  ApplyAerialPerspective(color, 1., normal, g_FilterParams.m_LightVector, linear_depth/2500.);
  g_OutCubemap[PixCoord + g_FilterParams.m_Offset].rgb = color;
}
