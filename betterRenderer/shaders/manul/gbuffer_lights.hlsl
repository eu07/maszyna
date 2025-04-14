#include "math.hlsli"

#include "lighting_functions.hlsli"

#include "color_transform.hlsli"

#include "manul/gbuffer_ssao.hlsli"

struct ViewConstants {
    float4x4 m_inverse_model_view;
    float4x4 m_inverse_projection;
};

struct PushConstantsSpotLight {
    float3 m_origin;
    float m_radius_sqr;
    float3 m_direction;
    float m_cos_inner_cone;
    float3 m_color;
    float m_cos_outer_cone;
};

cbuffer g_ViewConst : register(b0) { ViewConstants g_View; }

#ifdef SPIRV
[[vk::push_constant]] ConstantBuffer<PushConstantsSpotLight> g_SpotLight;
#else
cbuffer g_SpotLightConst : register(b1) { PushConstantsSpotLight g_SpotLight; }
#endif

RWTexture2D<float4> g_OutDiffuse : register(u0);

Texture2D<float> g_GbufferDepth : register(t0);
Texture2D<float3> g_GbufferNormal : register(t1);
Texture2D<float4> g_GbufferParams : register(t2);
Texture2D<float3> g_GbufferColor : register(t3);

float2 PixelToCS(in float2 pixel, in float2 size) {
  return ((pixel + .5) / size - .5) * float2(2., -2.);
}

float3 ReconstructPos(in float2 cs, in float depth) {
  float4 ndc = float4(cs, depth, 1.);
  ndc = mul(g_View.m_inverse_projection, ndc);
  return ndc.xyz / ndc.w;
}

[numthreads(8, 8, 1)]
void CS_Clear(uint3 PixCoord : SV_DispatchThreadID, uint3 GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex) {
    uint2 pixel = PixCoord.xy;
    g_OutDiffuse[pixel].rgb = 0.;
}

[numthreads(8, 8, 1)]
void CS_Spotlight(uint3 PixCoord : SV_DispatchThreadID, uint3 GroupID : SV_GroupID, uint GroupIndex : SV_GroupIndex) {
    uint2 gbuffer_dimensions;
    uint2 pixel = PixCoord.xy;

    g_OutDiffuse.GetDimensions(gbuffer_dimensions.x, gbuffer_dimensions.y);
    float3 position = ReconstructPos(PixelToCS(PixCoord.xy, gbuffer_dimensions), g_GbufferDepth[pixel]);
    float3 view = mul((float3x3)g_View.m_inverse_model_view, position);

    float3 L_offset = g_SpotLight.m_origin - view;
    float3 L = normalize(L_offset);
    if(dot(L_offset, L_offset) > g_SpotLight.m_radius_sqr) return;

    float3 albedo = g_GbufferColor[pixel];
    float4 params = g_GbufferParams[pixel];
    float3 normal = UnpackNormalXYZ(g_GbufferNormal[pixel]);

    SurfaceData surface_data;
    surface_data.m_Albedo = albedo;
    surface_data.m_Alpha = 1.;
    surface_data.m_Metalness = params.r;
    surface_data.m_Roughness = params.g;
#ifdef GBUFFER_SSAO_HLSLI
    surface_data.m_DiffuseOcclusion = min(params.b, GetSSAO(pixel));
#else
    surface_data.m_DiffuseOcclusion = params.b;
#endif
    surface_data.m_Normal = mul((float3x3)g_View.m_inverse_model_view, normal);
    surface_data.m_View = -normalize(view);
    surface_data.m_Reflect = reflect(-surface_data.m_View, surface_data.m_Normal);
    surface_data.m_NdotV = max(dot(surface_data.m_Normal, surface_data.m_View), 0.);
    surface_data.m_SpecularF0 = float3(.04, .04, .04) * params.a * 2.; 
    surface_data.m_SpecularF0 = lerp(surface_data.m_SpecularF0, surface_data.m_Albedo, surface_data.m_Metalness);
    surface_data.m_SpecularF = FresnelSchlickRoughness(surface_data.m_NdotV, surface_data.m_SpecularF0, surface_data.m_Roughness);
    surface_data.m_HorizonFading = 1.6;
    surface_data.m_SpecularOcclusion = ComputeSpecOcclusion(surface_data.m_NdotV, surface_data.m_DiffuseOcclusion, surface_data.m_Roughness);

    float3 lit = float3(0., 0., 0.);

    DirectionalLight directionalLight;
    directionalLight.m_LightVector = L;
    directionalLight.m_Color = max(REC709_to_XYZ(g_SpotLight.m_color), 0.);
    ApplyDirectionalLight(lit, directionalLight, surface_data, 1.);


    float cone = saturate((dot(L, -g_SpotLight.m_direction) - g_SpotLight.m_cos_inner_cone) / (g_SpotLight.m_cos_outer_cone - g_SpotLight.m_cos_inner_cone));
    float attenuation = 1. / dot(L_offset, L_offset) * (1. - cone);

    g_OutDiffuse[pixel].rgb += lit * attenuation;
}



