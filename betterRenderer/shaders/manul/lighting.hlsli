#ifndef LIGHTING_HLSLI
#define LIGHTING_HLSLI

#include "math.hlsli"

#include "material_common.hlsli"

#include "view_data.hlsli"

#include "lighting_functions.hlsli"

#include "forward_plus/light.hlsli"

SamplerState g_SamplerLinearClamp : register(s8);
TextureCube<float3> g_DiffuseEnvmap : register(t8);
TextureCube<float3> g_SpecularEnvmap : register(t9);
Texture2D<float2> g_BrdfLUT : register(t10);

Texture2D<uint2> g_LightGrid : register(t16);
StructuredBuffer<uint> g_LightIndexBuffer : register(t17);
StructuredBuffer<PackedLight> g_LightBuffer : register(t18);

#define LIGHTING_NEEDS_PIXELPOSITION 1

#if LIGHTING_NEEDS_PIXELPOSITION
void ApplyMaterialLighting(out float4 lit, in MaterialData material, in uint2 pixel_position);
#else
void ApplyMaterialLighting(out float4 lit, in MaterialData material);
#endif

void ApplyIBL(inout float3 lit, in SurfaceData material);
void ApplyForwardPlus(inout float3 lit, in SurfaceData material, in float3 position, in uint2 pixel_position);

#if LIGHTING_NEEDS_PIXELPOSITION
void ApplyMaterialLighting(out float4 lit, in MaterialData material, in uint2 pixel_position)
#else
void ApplyMaterialLighting(out float4 lit, in MaterialData material)
#endif
{
  // Camera-centric world position
  float3 view = mul((float3x3)g_InverseModelView, material.m_Position);
  
  // Convert material to surface data
  SurfaceData surface_data;
  surface_data.m_Albedo = clamp(material.m_MaterialAlbedoAlpha.rgb, .02, .9);
  surface_data.m_Alpha = material.m_MaterialAlbedoAlpha.a;
  surface_data.m_Metalness = material.m_MaterialParams.r;
  surface_data.m_Roughness = material.m_MaterialParams.g;
#ifdef GBUFFER_SSAO_HLSLI
  surface_data.m_DiffuseOcclusion = min(material.m_MaterialParams.b, GetSSAO(pixel_position));
#else
  surface_data.m_DiffuseOcclusion = material.m_MaterialParams.b;
#endif
  surface_data.m_Normal = mul((float3x3)g_InverseModelView, material.m_MaterialNormal);
  surface_data.m_View = -normalize(view);
  surface_data.m_Reflect = reflect(-surface_data.m_View, surface_data.m_Normal);
  surface_data.m_NdotV = max(dot(surface_data.m_Normal, surface_data.m_View), 0.);
  surface_data.m_SpecularF0 = float3(.04, .04, .04) * material.m_MaterialParams.a * 2.; 
  surface_data.m_SpecularF0 = lerp(surface_data.m_SpecularF0, surface_data.m_Albedo, surface_data.m_Metalness);
  surface_data.m_SpecularF = FresnelSchlickRoughness(surface_data.m_NdotV, surface_data.m_SpecularF0, surface_data.m_Roughness);
  surface_data.m_HorizonFading = 1.6;
  surface_data.m_SpecularOcclusion = ComputeSpecOcclusion(surface_data.m_NdotV, surface_data.m_DiffuseOcclusion, surface_data.m_Roughness);

  lit.rgb = material.m_MaterialEmission * surface_data.m_Alpha;
  lit.a = 1. - (saturate((1. - surface_data.m_Alpha) * lerp(1. - dot(surface_data.m_SpecularF, 1./3.), 0., surface_data.m_Metalness)));
  
  float shadow = 1.;
#ifdef SHADOW_HLSLI
  shadow = GetShadow(view, material);
#endif

#ifdef GBUFFER_CONTACT_SHADOWS_HLSLI
  shadow = min(shadow, GetContactShadows(pixel_position));
#endif
  
// Apply IBL cubemap
  ApplyIBL(lit.rgb, surface_data);
  
// Apply directional light
  DirectionalLight directionalLight;
  directionalLight.m_LightVector = g_LightDir.xyz;
  directionalLight.m_Color = g_LightColor.rgb;
  ApplyDirectionalLight(lit.rgb, directionalLight, surface_data, shadow);
  ApplyForwardPlus(lit.rgb, surface_data, view, pixel_position);
}

void ApplyIBL(inout float3 lit, in SurfaceData material) {
  float3 kS = material.m_SpecularF;
  float3 kD = 1. - kS;
  kD = lerp(kD, 0., material.m_Metalness);
  
  float2 brdf = g_BrdfLUT.SampleLevel(g_SamplerLinearClamp, float2(material.m_NdotV, material.m_Roughness), 0.);
  
  lit += material.m_Albedo * kD * g_DiffuseEnvmap.SampleLevel(g_SamplerLinearClamp, material.m_Normal, 0.) * material.m_DiffuseOcclusion * material.m_Alpha;
  lit += g_SpecularEnvmap.SampleLevel(g_SamplerLinearClamp, material.m_Reflect, sqrt(material.m_Roughness) * 5.) * (material.m_SpecularF * brdf.xxx + brdf.yyy) * material.m_SpecularOcclusion * HorizonFading(material.m_NdotV, material.m_HorizonFading);
}


void ApplyForwardPlus(inout float3 lit, in SurfaceData material, in float3 position, in uint2 pixel_position) {
  uint2 light_range = g_LightGrid[pixel_position / 8];
  for(int i = 0; i < light_range.y; ++i) {
    Light light = UnpackLight(g_LightBuffer[g_LightIndexBuffer[light_range.x + i]]);
    float3 L_offset = light.m_origin - position;
    float L_dist_sqr = dot(L_offset, L_offset);
    float3 L_dir = normalize(L_offset);
    //lit += light.m_color;
    if(L_dist_sqr > light.m_radius * light.m_radius) continue;

    float cos_point_deviation = dot(-L_dir, light.m_direction);
    if(light.m_cos_outer > -1.5 && cos_point_deviation < light.m_cos_outer) continue;


    float cone = light.m_cos_outer > -1.5 ? saturate((cos_point_deviation - light.m_cos_inner) / (light.m_cos_outer - light.m_cos_inner)) : 0.;
    float attenuation = 1. / dot(L_offset, L_offset) * (1. - cone);

    DirectionalLight directionalLight;
    directionalLight.m_LightVector = L_dir;
    directionalLight.m_Color = light.m_color * attenuation;

    ApplyDirectionalLight(lit.rgb, directionalLight, material, 1.);
  }
}

#endif
